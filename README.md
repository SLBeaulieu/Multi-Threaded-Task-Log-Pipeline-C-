Status: Work in Progress / Active Development
# Multi-Threaded C++ Logger

A simple C++ project I built to practice multi-threading and thread synchronization. It uses a thread-safe queue to take incoming tasks and process them across 3 background worker threads.

## Features
- **Thread-Safe Queue:** Protects shared data using `std::mutex` so multiple threads don't crash the app.
- **Worker Threads:** Background threads wait for work and process log messages as they come in.
- **File Logging:** Automatically writes formatted log timestamps to `system.log`.
