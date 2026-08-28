# C++ Asynchronous Logger Pipeline — Design & Architecture Document

## 1. System Requirements

### Functional Requirements
* **Asynchronous Execution:** Main application threads (producers) must dispatch log messages without blocking on file I/O operations.
* **Thread Safety:** Multiple concurrent threads must be able to push log requests simultaneously without data corruption or memory races.
* **ISO-8601 Timestamping:** Every log entry must record a human-readable, thread-safe timestamp (`[YYYY-MM-DD HH:MM:SS]`).
* **Controlled Shutdown:** The logging system must drain all pending log entries in the queue before terminating worker threads to prevent log loss.

### Non-Functional Requirements
* **Low Latency:** Minimize mutex contention on the producer side to ensure logging overhead does not slow down critical path execution.
* **Resource Efficiency:** Utilize condition variables (`std::condition_variable`) to put worker threads to sleep when the queue is empty, eliminating CPU idle-spinning.
* **Cross-Platform Compatibility:** Target standard C++17 threading interfaces (`std::thread`, `std::mutex`, `std::unique_lock`, `std::lock_guard`) to ensure clean compilation across GCC, Clang, and MSVC.

---

## 2. Core Algorithms & Pseudocode

### A. Producer Threading: `ThreadSafeQueue::push()`
Producers append items to the queue and notify waiting background workers.

```text
FUNCTION push(item):
    ACQUIRE lock using std::lock_guard on queueMutex
    APPEND item to internal std::queue
    RELEASE lock automatically (RAII)
    NOTIFY one waiting thread via conditionVariable.notify_one()
END FUNCTION 
```

### B. Consumer Worker Loop: ThreadSafeQueue::pop()
Worker threads block safely on the condition variable until work is available or a timeout occurs during shutdown.
```text
FUNCTION pop(item, timeoutDuration):
    ACQUIRE lock using std::unique_lock on queueMutex
    
    WAIT on conditionVariable UNTIL:
        - queue is NOT empty, OR
        - timeoutDuration expires
        
    IF queue is empty THEN
        RETURN false (Timed out / Queue empty)
    END IF
    
    MOVE front element into item
    REMOVE front element from internal std::queue
    RELEASE lock automatically (RAII)
    
    RETURN true (Item successfully retrieved)
END FUNCTION
```

### C. Worker Execution & Disk Flushing Loop
Each background worker thread continuously pops tasks and writes them out safely to disk.

```text
FUNCTION workerLoop():
    WHILE isRunning IS true OR queue IS NOT empty:
        IF queue.pop(task, timeout = 50ms) THEN
            FORMAT ISO-8601 timestamp string
            CONSTRUCT log line format: [TIMESTAMP] [LEVEL] MESSAGE
            
            ACQUIRE fileMutex lock
            WRITE log line to output file stream
            FLUSH output stream
            RELEASE fileMutex lock
        END IF
    END WHILE
END FUNCTION
```

### 3. Concurrency & Synchronization Design Decisions

lock_guard vs. unique_lock:

Used std::lock_guard inside push() for lightweight, non-blocking lock management where no conditional waiting is required.

Used std::unique_lock inside pop() because std::condition_variable::wait_for requires the ability to unlock and relock the mutex dynamically while waiting.

Timed Waiting (wait_for) over Indefinite Waiting (wait):

Using wait_for prevents worker threads from entering an unrecoverable deadlocked sleep state if the application signals shutdown while the queue is completely empty.

Isolated File Mutex:

Disk I/O synchronization (fileMutex) is separated from queue state synchronization (queueMutex). This design ensures producer threads pushing to the queue are never blocked by a worker thread currently writing to disk.
