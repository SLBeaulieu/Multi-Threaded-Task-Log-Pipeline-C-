/*
===============================================================================
  FILE:        LoggerPipeline.h
  DESCRIPTION: Asynchronous logging pipeline using a multi-threaded worker pool.
               Receives log requests and processes file I/O on background threads.
               
  KEY FEATURES:
  - Thread Pool: Spawns dedicated worker threads to consume log tasks.
  - Non-blocking Input: Main thread hands off work without waiting on disk writes.
  - File Synchronization: Protects log file writes using a dedicated mutex.
  - Timestamping: Formats entries with ISO/system time during processing.

  DEPENDENCIES: ThreadSafeQueue.h
  AUTHOR:     Samantha Beaulieu / Student Portfolio Project
===============================================================================
*/

#ifndef LOGGERPIPELINE_H
#define LOGGERPIPELINE_H

#include "ThreadSafeQueue.h" // Includes our thread-safe queue!
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>     // Required for std::localtime / std::tm
#include <iomanip>   // Required for std::put_time formatting
#include <sstream>   // Required for std::ostringstream

// ============================================================================
//  DATA STRUCTURES
// ============================================================================
// Represents a single log entry request passed into the pipeline

struct LogTask {
    std::string level;   // e.g., "INFO", "WARNING", "ERROR"
    std::string message; // The actual log message text
};

// ============================================================================
//  CLASS DECLARATION & PRIVATE MEMBERS
// ============================================================================
class LoggerPipeline {
private:
    ThreadSafeQueue<LogTask> taskQueue; // Thread-safe queue for incoming log tasks
    std::vector<std::thread> workers;   // Pool of background worker threads
    std::ofstream logFile;               // Output file stream on disk
    std::mutex fileMutex;                // Mutex protecting disk writes to logFile
    bool running;                        // Control flag for the worker loop
// ------------------------------------------------------------------------
    // PRIVATE HELPER: getFormattedTimestamp()
    // Returns current system time in [YYYY-MM-DD HH:MM:SS] format
    // ------------------------------------------------------------------------
    std::string getFormattedTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTm;

        // Platform-safe thread local time conversion
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&localTm, &currentTime);
#else
        localtime_r(&currentTime, &localTm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    // ------------------------------------------------------------------------
    // PRIVATE WORKER METHOD: workerLoop()
    // The main loop executed by each worker thread in the background
    // ------------------------------------------------------------------------
    void workerLoop() { //The background worker thread keeps running as long as the pipeline is active, 
                        //OR as long as there are leftover log tasks waiting in the queue to be written
        while (running || !taskQueue.empty()) {
            LogTask task;
            
            //Sleeps when the queue is empty, then wakes up and fills task as soon as work arrives.
            if (taskQueue.pop(task)) {
                // Generate timestamp right before writing to disk
                std::string timestamp = getFormattedTimestamp(); 
                //Ensures only one worker thread writes to the text file at a time so log lines don't become garbled or corrupted.
                std::lock_guard<std::mutex> lock(fileMutex);
                if (logFile.is_open()) {
                    logFile << "[" << timestamp << "] [" << task.level << "] " << task.message << "\n";
                    logFile.flush(); // Ensure text hits disk immediately
                }
            }
        }
    }
public:
    // ============================================================================
    //  CONSTRUCTOR & DESTRUCTOR
    // ============================================================================

    // ------------------------------------------------------------------------
    // CONSTRUCTOR: Initializes log file and spawns the worker thread pool
    // ------------------------------------------------------------------------
    LoggerPipeline(const std::string& filename, size_t numThreads = 2) 
        : running(true) {
        
        // Open the log file for writing
        logFile.open(filename, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            std::cerr << "Error: Failed to open log file: " << filename << std::endl;
        }

        // Spawn worker threads into our vector
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back(&LoggerPipeline::workerLoop, this);
        }
    }

    // ------------------------------------------------------------------------
    // DESTRUCTOR: Gracefully shuts down workers and flushes remaining tasks
    // ------------------------------------------------------------------------
    ~LoggerPipeline() {
        stop();
    }
    // ============================================================================
    //  PUBLIC API & LIFECYCLE MANAGEMENT
    // ============================================================================

    // ------------------------------------------------------------------------
    // METHOD: log()
    // Non-blocking call to push a log message into the processing queue
    // ------------------------------------------------------------------------
    void log(const std::string& level, const std::string& message) {
        taskQueue.push({level, message});
    }

    // ------------------------------------------------------------------------
    // METHOD: stop()
    // Flushes remaining queue items, shuts down worker threads, and closes file
    // ------------------------------------------------------------------------
    void stop() {
        if (!running) return; // Prevent stopping multiple times
        
        running = false;

        // Wait for all worker threads to finish processing leftover tasks
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        // Clean up log file stream
        if (logFile.is_open()) {
            logFile.close();
        }
    }
};

#endif // LOGGERPIPELINE_H

/*
===============================================================================
FILE SUMMARY: LoggerPipeline.h
-------------------------------------------------------------------------------
This header implements an asynchronous logging system using a producer-consumer 
pattern.

Key Responsibilities:
1. Non-blocking I/O: The main application thread calls log() without waiting 
   for slow disk file operations.
2. Worker Pool: Multiple background std::thread instances pop tasks from the 
   ThreadSafeQueue and perform the actual file writes.
3. Thread Safety: Uses fileMutex to prevent multiple worker threads from 
   interleaving text output into the log file.
4. Graceful Shutdown: The stop() method ensures all queued logs are flushed to 
   disk before background threads are joined and the file is closed.
===============================================================================
*/