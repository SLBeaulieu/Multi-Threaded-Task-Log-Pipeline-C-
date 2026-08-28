
/*
===============================================================================
  FILE:        ThreadSafeQueue.h
  DESCRIPTION: Generic, multi-thread-safe First-In, First-Out (FIFO) queue.
               Provides synchronized push/pop operations across multiple threads.
               
  KEY FEATURES:
  - Templates: Allows storage of arbitrary data types (e.g., LogTask, std::string).
  - Mutex Locking: Protects internal queue state from data races.
  - Condition Variable: Puts consumer threads to sleep when empty to conserve CPU.

  AUTHOR:      Samantha Beaulieu / Student Portfolio Project
===============================================================================
*/
/*
 ============================================================================
 HEADER GUARDS AND INCLUDES
 ============================================================================
 Header guards prevent the file from accidentally being included 
multiple times during compilation.
 */

#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>              // For standard std::queue
#include <mutex>              // For std::mutex and std::lock_guard
#include <condition_variable> // For std::condition_variable
#include <chrono>            // For std::chrono::milliseconds
/*
 ============================================================================
  CLASS BLUEPRINT & MEMBER VARIABLES
  ============================================================================
  Class Declaration & Private Members
  I created a generic template class so this queue can hold any type of data (T).
  Underneath the includes, declare the class and its private variables:
*/


template <typename T> //Tells the C++ compiler, 
                      //"The code following this isn't a single rigid class/function yet; it's a reusable blueprint."
                      //The 'typename T' is a placeholder for any data type that will be specified when an object of this class is created.
class ThreadSafeQueue {
private:
    std::queue<T> queue;        // The underlying standard queue holding our items
    mutable std::mutex mtx;     // The lock protecting access to 'queue'
    std::condition_variable cv; // The signal channel to wake up sleeping threads

public:
    ThreadSafeQueue() = default;

/*
 ------------------------------------------------------------------------
 PRODUCER METHOD: push()
 Adds an item to the queue and wakes up a waiting worker thread
------------------------------------------------------------------------
*/
    void push(T value) {
        {
            // Acquire the mutex lock before modifying the queue
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(std::move(value));
        } // 'lock' goes out of scope here and automatically unlocks 'mtx'

        // Notify one waiting thread that work is available
        cv.notify_one();
    }
/*
 ------------------------------------------------------------------------
  CONSUMER METHOD: pop()
 Waits until an item is available, retrieves it, and removes it
------------------------------------------------------------------------
*/
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait up to 50ms for work to arrive. 
        // If timeout occurs, return false so workerLoop can check 'running'
        if (!cv.wait_for(lock, std::chrono::milliseconds(50), [this] { return !queue.empty(); })) {
            return false;
        }

        value = std::move(queue.front());
        queue.pop();
        return true;
    }
/*
------------------------------------------------------------------------
 HELPER METHOD: empty() & CLOSING GUARD
 Checks if the queue is currently empty in a thread-safe manner
------------------------------------------------------------------------
*/
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
};

#endif // THREADSAFEQUEUE_H

/*
===============================================================================
FILE SUMMARY: ThreadSafeQueue.h
-------------------------------------------------------------------------------
This header defines a template-based, thread-safe FIFO queue. 

Key Responsibilities:
1. Thread Synchronization: Uses std::mutex to protect shared memory operations.
2. Resource Optimization: Uses std::condition_variable to put worker threads
   to sleep when the queue is empty, preventing unnecessary CPU usage.
3. Generics: Employs C++ templates so it can manage any data type (e.g., LogTask).
===============================================================================
*/