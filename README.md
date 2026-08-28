# Multi-Threaded C++ Asynchronous Logger Pipeline

A lightweight, non-blocking asynchronous logging system implemented in modern C++17. The pipeline utilizes a producer-consumer architecture backed by a template-based thread-safe queue, enabling multiple application threads to log events without waiting for disk file I/O operations.

---

## Key Features

* **Asynchronous / Non-Blocking Logging:** Producer threads submit log tasks directly to an in-memory queue and instantly resume execution, completely decoupling application performance from disk I/O.
* **Generic Thread-Safe Queue:** Template-based FIFO queue (`ThreadSafeQueue<T>`) protected by `std::mutex` and optimized using `std::condition_variable` to put idle worker threads to sleep and conserve CPU cycles.
* **Worker Thread Pool:** Configurable pool of background worker threads that consume and format log entries asynchronously.
* **Thread-Safe File Stream:** Disk writes are synchronized across worker threads using a dedicated file mutex to eliminate data races and text interleaving.
* **ISO-8601 Timestamps:** Thread-safe local time formatting (`[YYYY-MM-DD HH:MM:SS]`) generated dynamically before disk flushing.
* **Graceful Shutdown:** Drains remaining queued log messages and safely joins worker threads before closing the output file stream.

---

## Architecture & Data Flow

```text
[ Producer Thread 1 ] \
[ Producer Thread 2 ] -- ( Non-Blocking log() ) --> [ ThreadSafeQueue ]
[ Producer Thread 3 ] /                                   |
                                                   ( cv.wait_for )
                                                          v
                                                 [ Worker Pool (3 Threads) ]
                                                          |
                                                    ( fileMutex )
                                                          v
                                                  [ system.log ]

                                                  ---
```

## File Structure & Responsibilities

| File | Purpose / Responsibility |
| :--- | :--- |
| `ThreadSafeQueue.h` | Template class providing thread-safe FIFO queuing with `std::mutex` and `std::condition_variable`. |
| `LoggerPipeline.h` | Core logger managing worker thread creation, log task enqueueing, dynamic ISO timestamp formatting, and disk writes. |
| `main.cpp` | Benchmark harness simulating multiple producer threads submitting concurrent log requests. |
| `CMakeLists.txt` | Cross-platform build recipe configuring C++17, threading libraries, and targets. |
| `DESIGN.md` | System requirements, concurrency analysis, and pseudocode documentation. |
| `.gitignore` | Filters build artifacts (`/build/`, `.exe`) and output logs (`system.log`) from Git tracking. |

---

## Getting Started & Building

### Prerequisites
* C++17 compliant compiler (`g++`, `clang++`, or MSVC)
* CMake 3.14+

### Building with CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
./logger_test
```
-------
### Sample Output 
```text
[2026-08-28 11:11:53] [INFO] Worker Thread 2 processing task #1
[2026-08-28 11:11:53] [INFO] Worker Thread 3 processing task #1
[2026-08-28 11:11:53] [INFO] Worker Thread 5 processing task #1
[2026-08-28 11:11:53] [WARNING] Worker Thread 1 processing task #3
[2026-08-28 11:11:53] [ERROR] Worker Thread 4 processing task #5
```
--------
