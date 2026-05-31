# Build & Deployment Commands

## Prerequisites

- **For Cross-Compilation (Raspberry Pi ARM64):**
  - `aarch64-linux-gnu-g++` installed on Ubuntu
  - Installation: `sudo apt-get install g++-aarch64-linux-gnu`
  
- **For Local Build (Ubuntu x86/x64):**
  - `g++` compiler (typically pre-installed)
  - Check: `which g++`

- **For File Transfer:**
  - SSH client/server configured
  - Network connectivity established

---

## Build Commands

### 1. Cross-compile TCP Client (Raspberry Pi ARM64 Application)

**Purpose:** Compile the client application for ARM architecture (Raspberry Pi)

```bash
aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -pthread -o test_app_aarch64 \
  /home/umair/Data/04_C++/03_Demo/test_app.cpp
```

**Compiler Flags Explained:**

| Flag | Purpose |
|------|---------|
| `-std=c++17` | Use C++17 standard |
| `-O2` | Optimization level 2 (balance speed/compile-time) |
| `-Wall` | Enable all warnings |
| `-pthread` | Enable POSIX threading support |
| `-o test_app_aarch64` | Output executable name |

**Output:** `test_app_aarch64` (ARM64 executable, run on Raspberry Pi)

---

### 2. Compile TCP Server (Local Ubuntu Machine)

**Purpose:** Compile the server application for x86/x64 architecture

```bash
g++ -std=c++17 -O2 -Wall -pthread -o tcp_server tcp_server.cpp
```

**Compiler Flags Explained:**

| Flag | Purpose |
|------|---------|
| `-std=c++17` | Use C++17 standard |
| `-O2` | Optimization level 2 |
| `-Wall` | Enable all warnings |
| `-pthread` | Enable POSIX threading support |
| `-o tcp_server` | Output executable name |

**Output:** `tcp_server` (x86/x64 executable, run on Ubuntu)

---

## File Transfer Command

### Copy Compiled Binary to Raspberry Pi

**Purpose:** Transfer the compiled ARM64 executable to the Raspberry Pi

```bash
scp /home/umair/Data/04_C++/03_Demo/test_app_aarch64 \
    umair@192.168.178.37:/home/umair/
```

**Transfer Details:**

| Component | Value |
|-----------|-------|
| Source | `/home/umair/Data/04_C++/03_Demo/test_app_aarch64` (on Ubuntu) |
| Destination | `/home/umair/` (on Raspberry Pi) |
| Destination IP | `192.168.178.37` |
| Username | `umair` |
| Protocol | SSH Secure Copy |

**Expected Output:**
```
test_app_aarch64                              100% [████████████████████] 2.5 MB
```

---

## Quick Reference

### Compile Everything Locally (x86/x64)
```bash
cd /home/umair/Data/gitHub/TCP_IP_ClientServer
g++ -std=c++17 -O2 -Wall -pthread -o tcp_server tcp_server.cpp
g++ -std=c++17 -O2 -Wall -pthread -o tcp_client tcp_client.cpp
```

### Cross-Compile for Raspberry Pi
```bash
aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -pthread -o test_app_aarch64 \
  /home/umair/Data/04_C++/03_Demo/test_app.cpp
```

### Transfer to Raspberry Pi
```bash
# Transfer compiled binary
scp test_app_aarch64 umair@192.168.10.2:/home/umair/

# Transfer any file to Pi
scp <local_file> pi@192.168.10.2:~/

# Retrieve file from Pi
scp pi@192.168.10.2:~/<remote_file> .
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `command not found: aarch64-linux-gnu-g++` | Install ARM cross-compiler: `sudo apt-get install g++-aarch64-linux-gnu` |
| Permission denied during SCP | Ensure Pi is running SSH: `sudo service ssh start` on Pi |
| Connection timeout | Verify network connectivity: `ping 192.168.10.2` |
| Compilation errors | Check file paths and C++ syntax in source files |

---

## Notes

- All executables are listed in `.gitignore` and won't be tracked by git
- Optimization level `-O2` provides good balance; use `-O3` for maximum optimization (slower compile)
- The `-pthread` flag is essential for multithreaded TCP communication
- Replace IP address `192.168.178.37` with your actual Raspberry Pi IP if different
