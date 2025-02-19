
---

# easynet - 轻量级 C++ 网络库 （持续更新中）🌐🚀

## 介绍

**easynet** 是一个轻量级的 C++ 网络库，专为 Linux 系统设计，旨在通过简单的接口帮助开发者在代码中快速实现常用的网络功能，包括 **TCP**、**UDP**（开发中）、**HTTP**、**WebSocket** 等协议，并支持 **SSL/TLS** 加密、**基于 EventLoop 的非阻塞 I/O**、**I/O 多路复用**、**简单定时器**（完善中）和 **小型日志** 等功能。🔒⚡

目前，项目适用于学习和个人项目使用，**不建议用于生产环境**（库仍在开发中，接口和功能可能会发生较大变化）。**欢迎在 Issue 中提出宝贵的意见与反馈**。💬

如果你觉得这个项目对你有所帮助，记得为本仓库点个 ⭐️ 吧！感谢支持！🙏

### 计划开发功能列表 🛠️

- [x] TCP
- [ ] UDP
- [x] HTTP
- [x] WebSocket
- [x] SSL/TLS 加密 🔐
- [x] 基于定时器的客户端 ⏲️
- [ ] 反向代理 🌍
- [ ] 正向代理 🌍
- [x] I/O 多路复用 🔄
- [x] 日志 📝
- [x] 线程池 🏋️‍♂️
- [ ] 负载均衡 ⚖️

## 环境需求 ⚙️

- C++20 编译器（如 gcc 15、clang 15 或更高版本）
- OpenSSL 3.0.2 或更高版本
- GTest 1.11.0 或更高版本

## 安装步骤 🔧

1. 克隆仓库：
   ```bash
   git clone https://github.com/JellyfishKnight/easynet.git
   cd easynet
   ```

2. 构建并安装：
   ```bash
   # (可选) 使用 Ninja 构建
   cmake -B build -G Ninja && ninja -C build -j4
   # 安装
   cmake -B build && make -C build -j4
   cd build && make install
   ```

## Demo 🎬

在 `demo` 目录下，你可以找到示例代码，默认安装路径为 `build` 目录。

## HTTP 性能测试效果 📊（持续优化中）

**测试机器配置：**  
- CPU: AMD Ryzen 5 5600H  
- 内存: 16 GB RAM  

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

### 主要功能 💡

- **TCP & UDP** 支持（待完成的 UDP 功能正在开发中）。
- **HTTP & WebSocket** 支持，适用于构建高效的 Web 服务和实时通信。
- **SSL/TLS 加密**，确保数据传输的安全性 🔒。
- **I/O 多路复用**，提高性能和并发处理能力 🔄。
- **线程池**，优化高并发处理 🏋️‍♂️。
- **简易日志系统**，记录关键操作和错误信息 📝。
