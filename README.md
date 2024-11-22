[![Release](https://img.shields.io/badge/Release-v1.0.0-blueviolet?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/releases)
[![Last Commit](https://img.shields.io/badge/Last%20Commit-November%202024-brightgreen?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/commits)
[![License](https://img.shields.io/badge/License-MIT-red?style=flat-square)](https://opensource.org/licenses/MIT)
[![Stars](https://img.shields.io/github/stars/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/stargazers)
[![Issues](https://img.shields.io/github/issues/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/issues)
[![Code Size](https://img.shields.io/github/languages/code-size/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool)
[![Contributors](https://img.shields.io/github/contributors/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/graphs/contributors)

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


## **Getting Started**

### **Prerequisites**
- A C++17 or newer compiler (e.g., GCC, Clang).
- CMake (version 3.12 or higher).

### **Clone the repository**

```bash
git clone https://github.com/lazy-cat-y/cpp-thread-pool.git
cd cpp-thread-pool
```

### Build the project using CMake:

This project supports configurable build options via `CMake`. You can customize the build by enabling or disabling specific features.

#### **Options**
| Option Name        | Description                                                                          | Default Value |
| ------------------ | ------------------------------------------------------------------------------------ | ------------- |
| `ENABLE_TEST`      | Enable building unit tests using Google Test.                                        | `OFF`         |
| `CMAKE_BUILD_TYPE` | Specify the build type. Options: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`. | `Debug`       |

---

#### **How to Configure Options**

You can configure these options by passing them as arguments to the `cmake` command.

##### Example 1: Disable tests
If you want to build the project without tests:
```bash
cmake -DENABLE_TEST=OFF ..
```

##### Example 2: Set build type to Release
If you want to build the project with optimizations:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### Example 3: Enable both options explicitly
To enable tests and set the build type to `Debug`:
```bash
cmake -DENABLE_TEST=ON -DCMAKE_BUILD_TYPE=Debug ..
```

---

#### **Checking Current Configuration**
Once CMake is configured, you can verify the applied options in the generated `CMakeCache.txt` file in your build directory. Look for lines like these:
```text
ENABLE_TEST:BOOL=OFF
CMAKE_BUILD_TYPE:STRING=Release
```


---

## **Usage Example**
Here's an example of how to use the thread pool:

```cpp
#include "worker-pool.h"
#include <iostream>

int main() {
    ThreadPool<4, 10, 2> pool;

    std::vector<std::future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }

    for (int i = 0; i < 10; ++i) {
        futures[i].get(); 
    }

    pool.shutdown();
}
```

---

## **Project Structure**
```
.
├── CMakeLists.txt           # Build configuration
├── src
│   ├── include              
│   │   ├── channel.hpp      # Channel implementation
│   │   ├── m-define.h        # Macros for thread pool
│   │   ├── worker.hpp       # Worker implementation
│   │   └── worker-pool.hpp  # ThreadPool implementation
│   └──  CMakelists.txt      # Source files
├── tests
│   ├── test-channel.cc      # Unit tests for the channel
│   ├── test-pool.cc         # Unit tests for the thread pool
│   └── test-worker.c        # Unit tests for the workers
├── third-party
│   └── ...                  # Dependencies
├── README.md
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
