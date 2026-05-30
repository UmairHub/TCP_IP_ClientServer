# TCP/IP Client-Server Application

A C++ implementation of TCP/IP client-server communication with support for cross-compilation to ARM64 (Raspberry Pi) and standard Linux systems.

## Overview

This project includes:
- **TCP Server**: A multi-threaded server application
- **TCP Client**: A client application for Raspberry Pi (ARM64 architecture)

## Build Instructions

### Prerequisites

- C++17 compatible compiler
- For cross-compilation: `aarch64-linux-gnu-g++` toolchain
- POSIX-compliant system with pthread support

### Compiling TCP Client (Raspberry Pi - ARM64)

Cross-compile for ARM64 architecture:

```bash
aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -pthread -o test_app_aarch64 /home/umair/Data/04_C++/03_Demo/test_app.cpp
```

**Flags explanation:**
- `-std=c++17`: Use C++17 standard
- `-O2`: Optimization level 2
- `-Wall`: Enable all compiler warnings
- `-pthread`: Enable POSIX threading

### Compiling TCP Server

Build for the local system:

```bash
g++ -std=c++17 -O2 -Wall -pthread -o tcp_server tcp_server.cpp
```

## Usage

After successful compilation, run the applications:

```bash
# Start the server
./tcp_server

# Connect with the client
./test_app_aarch64
```

## Project Structure

- `tcp_server.cpp`: Server implementation
- `test_app.cpp`: Client implementation (Raspberry Pi target)

---

**Language:** C++17
