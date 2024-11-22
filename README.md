[![Release](https://img.shields.io/badge/Release-v1.0.0-blueviolet?style=flat-square)](https://github.com/lazy-cat-y/thread-pool/releases)
[![Last Commit](https://img.shields.io/badge/Last%20Commit-November%202024-brightgreen?style=flat-square)](https://github.com/lazy-cat-y/thread-pool/commits)
[![License](https://img.shields.io/badge/License-MIT-red?style=flat-square)](https://opensource.org/licenses/MIT)
[![Stars](https://img.shields.io/github/stars/lazy-cat-y/thread-pool?style=flat-square)](https://github.com/lazy-cat-y/thread-pool/stargazers)
[![Issues](https://img.shields.io/github/issues/lazy-cat-y/thread-pool?style=flat-square)](https://github.com/lazy-cat-y/thread-pool/issues)
[![Code Size](https://img.shields.io/github/languages/code-size/lazy-cat-y/thread-pool?style=flat-square)](https://github.com/lazy-cat-y/thread-pool)
[![Contributors](https://img.shields.io/github/contributors/lazy-cat-y/thread-pool?style=flat-square)](https://github.com/lazy-cat-y/thread-pool/graphs/contributors)


# **Thread Pool with Worker and Channel Implementation**

## **Description**
This project implements a thread pool using a worker-based design pattern with a channel for task management and inter-thread communication. It is designed for efficient task execution in a multithreaded environment, featuring:

- A thread-safe **Channel** for task queuing and communication.
- A **Worker** class to process tasks.
- A **ThreadPool** class to manage multiple workers and distribute tasks.

## **Features**
- **Worker Management**: Automatically starts and manages workers in a pool.
- **Thread-Safe Channel**: Implements a thread-safe queue using condition variables and mutexes.
- **Dynamic Task Assignment**: Supports submitting tasks to the pool and redistributing workloads.
- **Monitoring and Recovery**:
  - Detects deadlocks.
  - Monitors worker heartbeats.
  - Restarts workers if they become unresponsive.

## **Code Structure**
### **Classes**
1. **Channel**  
   A generic, thread-safe channel for task queuing and communication.
2. **Worker**  
   A worker thread that executes tasks from its queue.
3. **ThreadPool**  
   A thread pool that manages multiple workers and distributes tasks.

### **Core Functions**
#### Channel
- `send` and `receive`: For pushing tasks into and retrieving tasks from the channel.
- `close`: To stop accepting new tasks.

#### Worker
- `start`, `stop`, and `join`: For lifecycle management of the worker thread.
- `submit`: For submitting tasks to the worker's queue.

#### ThreadPool
- `submit`: For submitting tasks to the pool.
- `shutdown`: Gracefully stops all workers.
- `restart_worker`: Restarts a specific worker in case of failure.

---

## **Getting Started**

### **Prerequisites**
- A C++17 or newer compiler (e.g., GCC, Clang).
- CMake (version 3.12 or higher).

### **Build Instructions**
1. Clone the repository:
   ```bash
   git clone https://github.com/lazy-cat-y/thread-pool.git
   cd thread-pool
   ```

2. Create a build directory and navigate to it:
   ```bash
   mkdir build && cd build
   ```

3. Build the project using CMake:
   ```bash
   cmake ..
   make
   ```

4. Run the tests (if implemented):
   ```bash
   ./test-channel
   ```

---

## **Usage Example**
Here's an example of how to use the thread pool:

```cpp
#include "ThreadPool.h"
#include <iostream>

void exampleTask(int id) {
    std::cout << "Executing task " << id << " on thread " 
              << std::this_thread::get_id() << std::endl;
}

int main() {
    ThreadPool<4, 50> threadPool;  // 4 workers, queue size of 50

    for (int i = 0; i < 10; ++i) {
        threadPool.submit(exampleTask, i);
    }

    threadPool.shutdown();  // Gracefully stop all workers
    return 0;
}
```

---

## **Project Structure**
```
.
├── CMakeLists.txt           # Build configuration
├── include
│   ├── Channel.h            # Channel implementation
│   ├── Worker.h             # Worker implementation
│   └── ThreadPool.h         # ThreadPool implementation
├── src
│   ├── Channel.cpp          # Channel source
│   ├── Worker.cpp           # Worker source
│   └── ThreadPool.cpp       # ThreadPool source
├── tests
│   └── test-channel.cpp     # Unit tests for the channel and workers
└── LICENSE                  # License file (MIT)
```

---

## **Contributing**
Contributions are welcome! Please follow these steps:
1. Fork this repository.
2. Create a new branch:
   ```bash
   git checkout -b feature-name
   ```
3. Commit your changes:
   ```bash
   git commit -m "Add feature-name"
   ```
4. Push to your branch:
   ```bash
   git push origin feature-name
   ```
5. Open a pull request.

---

## **License**
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

## **Author**
- **Haoxin Yang**  
  GitHub: [lazy-cat-y](https://github.com/lazy-cat-y)
