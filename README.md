[![Release](https://img.shields.io/badge/Release-v1.0.0-blueviolet?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/releases)
[![Last Commit](https://img.shields.io/badge/Last%20Commit-November%202024-brightgreen?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/commits)
[![License](https://img.shields.io/badge/License-MIT-red?style=flat-square)](https://opensource.org/licenses/MIT)
[![Stars](https://img.shields.io/github/stars/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/stargazers)
[![Issues](https://img.shields.io/github/issues/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/issues)
[![Code Size](https://img.shields.io/github/languages/code-size/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool)
[![Contributors](https://img.shields.io/github/contributors/lazy-cat-y/cpp-thread-pool?style=flat-square)](https://github.com/lazy-cat-y/cpp-thread-pool/graphs/contributors)

# **Thread Pool with Worker and Channel Implementation**

## **Description**
This project implements a thread pool using a MPMC (Multiple Producers, Multiple Consumers) model in C++. It includes:

- A lock-free, thread-safe **MPMC Queue** for task queuing based on Vyukov's design.
- **Thread Pool** that manages worker threads.

## **Features**
- **Lock-Free**: Utilizes lock-free data structures to minimize contention and improve performance.
- **Multi Wait Strategy**: Supports multiple wait strategies for worker threads, allowing for flexibility in task execution.

<p align="center"> <table> <tr> <th>Wait Strategy</th> <th>Description</th> <th>Lock-Free</th> <th>Use Case</th> </tr> <tr> <td align="center"><code>PassiveWaitStrategy</code></td> <td>Uses <code>std::this_thread::sleep_for()</code> to sleep for a fixed duration. Simple and low CPU usage, but high latency.</td> <td align="center">✅</td> <td>Low-power scenarios or non-latency-critical tasks</td> </tr> <tr> <td align="center"><code>SpinBackOffWaitStrategy</code></td> <td>Busy-spins and yields gradually. Good tradeoff between latency and CPU usage.</td> <td align="center">✅</td> <td>High-throughput systems under moderate load</td> </tr> <tr> <td align="center"><code>AtomicWaitStrategy</code></td> <td>Waits on <code>std::atomic::wait()</code> and notifies via <code>notify_one</code>/<code>notify_all</code>. Lock-free and fast.</td> <td align="center">✅</td> <td>Modern platforms with support for C++20 atomics</td> </tr> <tr> <td align="center"><code>ConditionVariableWaitStrategy</code></td> <td>Uses <code>std::condition_variable</code>. Slightly higher overhead due to locks, but more portable.</td> <td align="center">❌</td> <td>Generic platforms or when lock-based waiting is needed</td> </tr> </table> </p>

## **Getting Started**

### **Prerequisites**
- A C++20 or newer compiler (e.g., GCC, Clang).
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
| `CMAKE_BUILD_TYPE` | Specify the build type. Options: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`. | `Release`     |

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

#include "lc_thread_pool.h"

using namespace lc;

// Define metadata type
struct MyMetadata {
    int priority;
};

// Create a thread pool with 4 workers and default wait strategy
int main() {
    auto queue = std::make_shared<MPMCQueue<Context<MyMetadata, std::function<void()>>>>(1024);
    ThreadPool<4, MyMetadata> pool(queue);

    // Example 1: Submit a simple task with no return value and metadata
    pool.submit(MyMetadata{.priority = 1}, [] {
        std::cout << "Task with metadata executed\n";
    });

    // Example 2: Submit a task with return value
    auto future = pool.submit([]() -> int {
        return 42;
    });
    std::cout << "Returned: " << future.get() << "\n";

    // Example 3: Submit a task with arguments and metadata
    auto sum_future = pool.submit(MyMetadata{.priority = 2}, [](int a, int b) {
        return a + b;
    }, 10, 32);
    std::cout << "Sum: " << sum_future.get() << "\n";

    pool.shutdown();
    return 0;
}

```

---

## **Project Structure**
```
.
├── CMakeLists.txt           # Build configuration
├── src
│   ├── include              
│   │   ├── lc_config.hpp         # Configuration header
│   │   ├── lc_context.h         # Context header
│   │   ├── lc_mpmc_queue.hpp    # Multi-producer, multi-consumer queue
│   │   ├── lc_thread_pool.hpp   # ThreadPool implementation
│   │   └── lc_wait_strategy.hpp # Wait strategy implementation
│   └──  CMakelists.txt      # Source files
├── tests
│   ├── base-test      # Unit tests for the base functionality
│   │   ├── mpmc_queue_test.cc      # MPMC Queue tests
│   │   └── thread_pool_test.cc     # ThreadPool tests
│   ├── benchmark      # Performance tests for the thread pool
│   │   └── performance_test.cc     # ThreadPool performance tests
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
