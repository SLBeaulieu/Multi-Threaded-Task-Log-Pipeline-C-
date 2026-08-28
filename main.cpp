/*
===============================================================================
  FILE:        main.cpp
  DESCRIPTION: Entry point for testing the asynchronous LoggerPipeline.
               Spawns multiple concurrent producer threads that submit log tasks 
               into a thread-safe queue processed by background worker threads.
               
  KEY FEATURES:
  - Producer-Consumer Test: Simulates real-world application workload across threads.
  - Reference Passing: Uses std::ref to pass non-copyable LoggerPipeline instances.
  - Safe Lifecycle Management: Ensures proper thread spawning, joining, and flushing.

  AUTHOR:      Samantha Beaulieu / Student Portfolio Project
===============================================================================
*/

// ============================================================================
//  HEADER INCLUDES
// ============================================================================

#include "LoggerPipeline.h" //Acess header file for LoggerPipeline class
#include <iostream> //(Input/Output Stream)
#include <vector> //(Dynamic Array Container)For std::vector container to hold producer threads
#include <thread> //(Multithreading Utilities)For std::thread to spawn concurrent producer threads
#include <string> //(Text Manipulation)For std::string to format log messages
#include <chrono> //(Time & Clock Utilities)For std::chrono::milliseconds to simulate work delays

// ============================================================================
//  PRODUCER WORKLOAD FUNCTION
// ============================================================================
// Executed concurrently by multiple application producer threads.
// Each thread generates log requests and pushes them into the pipeline.
// ----------------------------------------------------------------------------
void simulateApplicationActivity(LoggerPipeline& logger, int threadId, int logCount) {
    for (int i = 1; i <= logCount; ++i) {
        // Vary log levels across iterations to simulate dynamic system activity
        std::string level = (i % 3 == 0) ? "WARNING" : ((i % 5 == 0) ? "ERROR" : "INFO");
        // Construct unique log message payload
        std::string message = "Worker Thread " + std::to_string(threadId) + 
                              " processing task #" + std::to_string(i);

        // Non-blocking call: hands off work to the queue without waiting on disk writes
        logger.log(level, message);

        // Yield CPU briefly to simulate real work and interleave thread execution
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}
// ============================================================================
//  MAIN APPLICATION ENTRY POINT
// ============================================================================
int main() {
    std::cout << "===========================================\n";
    std::cout << "  Multi-Threaded C++ Logger Pipeline Test  \n";
    std::cout << "===========================================\n\n";

    // ------------------------------------------------------------------------
    // STEP 1: INITIALIZE LOGGER PIPELINE
    // ------------------------------------------------------------------------
    // Define log output file destination and thread pool count
    const std::string logFilename = "system.log";
    const size_t numWorkerThreads = 3;
    
    std::cout << "[MAIN] Initializing LoggerPipeline writing to '" << logFilename 
              << "' with " << numWorkerThreads << " worker threads...\n";
    
    LoggerPipeline logger(logFilename, numWorkerThreads);

    // ------------------------------------------------------------------------
    // STEP 2: SPAWN PRODUCER THREADS
    // ------------------------------------------------------------------------
    const int numProducers = 5;
    const int logsPerProducer = 10;
    std::vector<std::thread> producers;

    std::cout << "[MAIN] Spawning " << numProducers << " application threads...\n";

    for (int i = 1; i <= numProducers; ++i) {
        // Construct threads in-place. Use std::ref(logger) because LoggerPipeline 
        // contains std::mutex and std::ofstream, making it non-copyable.
        producers.emplace_back(simulateApplicationActivity, std::ref(logger), i, logsPerProducer);
    }

    // ------------------------------------------------------------------------
    // STEP 3: WAIT FOR PRODUCERS TO FINISH
    // ------------------------------------------------------------------------
    // Block main thread until all producer threads complete log submission
    for (auto& producer : producers) {
        if (producer.joinable()) {
            producer.join();
        }
    }

    std::cout << "[MAIN] All producer threads finished submitting logs.\n";
    std::cout << "[MAIN] Shutting down logger pipeline and flushing remaining queue tasks...\n";

    // ------------------------------------------------------------------------
    // STEP 4: SHUT DOWN LOGGER PIPELINE
    // ------------------------------------------------------------------------
    // Gracefully shut down background workers and close file stream
    logger.stop();

    std::cout << "[MAIN] Logger shutdown complete. All entries written to " << logFilename << ".\n";

    return 0;// Exit program successfully
}
