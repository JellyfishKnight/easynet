
---

# easynet - Lightweight C++ Network Library (Ongoing Development) 🌐🚀

[English](README.md) | [中文](README_zh.md)

## Introduction

**easynet** is a lightweight C++ network library designed specifically for Linux systems, aimed at helping developers quickly implement common network functionalities in their code through simple interfaces. It supports protocols such as **TCP**, **UDP** (in development), **HTTP**, **WebSocket**, and also features **SSL/TLS** encryption, **EventLoop-based non-blocking I/O**, **I/O multiplexing**, **simple timers** (in progress), and **small logging utilities**. 🔒⚡

Currently, the project is suitable for learning and personal projects, **not recommended for production environments** (the library is still under development and interfaces and features may change significantly). **Feel free to provide valuable feedback and suggestions in the Issues section**. 💬

If you find this project helpful, please give the repository a ⭐️ to show your support! Thank you! 🙏

### Planned Features List 🛠️

- [x] TCP
- [ ] UDP
- [x] HTTP
- [x] WebSocket
- [x] SSL/TLS Encryption 🔐
- [x] Timer-based Client ⏲️
- [ ] Reverse Proxy 🌍
- [ ] Forward Proxy 🌍
- [x] I/O Multiplexing 🔄
- [x] Logging 📝
- [x] Thread Pool 🏋️‍♂️
- [ ] Load Balancing ⚖️

## System Requirements ⚙️

- C++20 compatible compiler (such as gcc 15, clang 15 or newer)
- OpenSSL 3.0.2 or newer
- GTest 1.11.0 or newer

## Installation Steps 🔧

1. Clone the repository:
   ```bash
   git clone https://github.com/JellyfishKnight/easynet.git
   cd easynet
   ```

2. Build and Install:
   ```bash
   # (Optional) Use Ninja for building
   cmake -B build -G Ninja && ninja -C build -j4
   # Install
   cmake -B build && make -C build -j4
   cd build && make install
   ```

## Demo 🎬

You can find the sample code in the `demo` directory. The default installation path is the `build` directory.

## HTTP Performance Test Results 📊 (Ongoing Optimization)

**Test Machine Configuration:**  
- CPU: AMD Ryzen 5 5600H  
- RAM: 16 GB  

```bash
wrk -c 1000 -t 16 -d20 http://127.0.0.1:8080/
Running 20s test @ http://127.0.0.1:8080/
  16 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.41ms    3.09ms 216.70ms   88.53%
    Req/Sec     4.46k     3.25k   20.67k    72.12%
  539506 requests in 18.53s, 2.31GB read
  Socket errors: connect 0, read 0, write 0, timeout 96
Requests/sec:  29107.69
Transfer/sec:    127.89MB
```

---

### Key Features 💡

- **TCP & UDP** Support (UDP functionality is in progress).
- **HTTP & WebSocket** Support, suitable for building efficient web services and real-time communication.
- **SSL/TLS Encryption**, ensuring secure data transmission 🔒.
- **I/O Multiplexing**, improving performance and concurrency 🔄.
- **Thread Pool**, optimizing high concurrency 🏋️‍♂️.
- **Simple Logging System**, recording critical operations and errors 📝.
