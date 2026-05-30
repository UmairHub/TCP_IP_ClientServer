# Connecting PC to Raspberry Pi over Ethernet Cable

## Objective

Establish direct file transfer connection between Ubuntu PC and Raspberry Pi using an Ethernet cable connection (without DHCP router)

**Use Case:** Direct development, testing, and file transfer between single PC and single Raspberry Pi

---

## Prerequisites

- Ethernet cable (Cat5/Cat5e or better)
- Both PC and Raspberry Pi with Ethernet ports
- Ubuntu Linux on PC
- Raspberry Pi with network interface
- Root/sudo access on both machines
- Basic networking knowledge

---

## Step-by-Step Setup

### STEP 1: Verify Physical Connection

**Purpose:** Ensure the Ethernet cable is properly connected and active

**Command (run on both PC and Raspberry Pi):**
```bash
ip addr
```

**Expected Output - Look for your Ethernet interface:**
```
2: enp6s0: <BROADCAST,MULTICAST,UP,LOWER_UP>
    link/ether aa:bb:cc:dd:ee:ff brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.100/24 brd 192.168.1.255 scope global enp6s0
```

**Status Meanings:**
- ✓ `UP` - Interface is active
- ✓ `LOWER_UP` - Physical link detected
- ✗ `DOWN` - Cable disconnected or interface disabled

**Common Interface Names:**
| Device | Interface |
|--------|-----------|
| Ubuntu PC | `eth0`, `enp0s*`, `enp6s0` |
| Raspberry Pi | `eth0` |

**Troubleshooting:**
- If interface shows `DOWN`, try: `sudo ip link set <interface> up`
- Check cable connection on both ends
- Try different Ethernet cable if issue persists

---

### STEP 2: Assign Static IP Addresses

**Purpose:** Configure IP addresses manually (no DHCP server available)

**On Ubuntu PC:**
```bash
sudo ip addr add 192.168.10.1/24 dev enp6s0
```

**On Raspberry Pi:**
```bash
sudo ip addr add 192.168.10.2/24 dev eth0
```

**IP Configuration Details:**

| Component | Value | Purpose |
|-----------|-------|---------|
| PC IP | `192.168.10.1` | Ubuntu machine address |
| Pi IP | `192.168.10.2` | Raspberry Pi address |
| Subnet Mask | `/24` or `255.255.255.0` | Defines network range |
| Network Range | `192.168.10.0 - 192.168.10.255` | Available addresses |

**Verify Assignment:**
```bash
ip addr show
# Look for: inet 192.168.10.X/24
```

**Make Persistent (Optional):**

*On Ubuntu (using netplan):*
```bash
sudo nano /etc/netplan/99-ethernet.yaml
```

Add:
```yaml
network:
  version: 2
  ethernets:
    enp6s0:
      dhcp4: no
      addresses:
        - 192.168.10.1/24
```

Then apply:
```bash
sudo netplan apply
```

---

### STEP 3: Test Network Connectivity

**Purpose:** Verify bidirectional communication between PC and Raspberry Pi

**From Ubuntu PC (ping Raspberry Pi):**
```bash
ping 192.168.10.2
```

**From Raspberry Pi (ping Ubuntu PC):**
```bash
ping 192.168.10.1
```

**Expected Output:**
```
PING 192.168.10.2 (192.168.10.2) 56(84) bytes of data.
64 bytes from 192.168.10.2: icmp_seq=1 ttl=64 time=0.845 ms
64 bytes from 192.168.10.2: icmp_seq=2 ttl=64 time=0.726 ms
64 bytes from 192.168.10.2: icmp_seq=3 ttl=64 time=0.731 ms
```

**Stop ping:** Press `Ctrl+C`

**Troubleshooting:**
- If no response, check IP addresses: `ip addr show`
- Verify cable connection: `ip link show`
- Try pinging with `-c 3` flag to limit attempts: `ping -c 3 192.168.10.2`

---

### STEP 4: SSH Access (Optional)

**Purpose:** Remote terminal access to Raspberry Pi for command execution

**Enable SSH on Raspberry Pi:**
```bash
sudo systemctl start ssh
sudo systemctl enable ssh  # Auto-start on boot
```

**Connect from Ubuntu:**
```bash
ssh pi@192.168.10.2
```

**First-time Connection:**
- Answer `yes` to host key verification prompt
- Enter Raspberry Pi password (default: `raspberry`)

**Verify SSH Connection:**
```bash
ssh pi@192.168.10.2 'uname -a'
# Should return Raspberry Pi system info without full login
```

**SSH Configuration Tips:**

Create SSH key for password-less login:
```bash
ssh-keygen -t rsa -b 4096
ssh-copy-id pi@192.168.10.2
ssh pi@192.168.10.2  # Now no password required
```

---

### STEP 5: File Transfer

**Purpose:** Copy files between Ubuntu PC and Raspberry Pi

#### Copy File TO Raspberry Pi:
```bash
scp myfile.txt pi@192.168.10.2:~/
scp myfile.txt pi@192.168.10.2:/home/pi/Documents/
```

#### Copy File FROM Raspberry Pi:
```bash
scp pi@192.168.10.2:~/myfile.txt .
scp pi@192.168.10.2:~/myfile.txt ~/Downloads/
```

#### Copy Entire Directory:
```bash
scp -r ~/my_project/ pi@192.168.10.2:~/
```

**Transfer Details:**

| Component | Description |
|-----------|-------------|
| `scp` | SSH Secure Copy Protocol |
| `~` | Home directory (`/home/pi/` or `/home/umair/`) |
| `.` | Current directory |
| `-r` | Recursive (copy entire directory) |

**Example - Practical Workflow:**

```bash
# Compile on Ubuntu
g++ -std=c++17 -O2 -Wall -pthread -o test_app tcp_client.cpp

# Transfer to Pi
scp test_app pi@192.168.10.2:~/

# SSH into Pi and run
ssh pi@192.168.10.2
./test_app
```

---

## Quick Reference Cheat Sheet

```bash
# 1. Check connection
ip addr

# 2. Assign IPs (Ubuntu)
sudo ip addr add 192.168.10.1/24 dev enp6s0

# 2. Assign IPs (Raspberry Pi)
sudo ip addr add 192.168.10.2/24 dev eth0

# 3. Test connectivity
ping 192.168.10.2

# 4. SSH into Pi
ssh pi@192.168.10.2

# 5. Copy files TO Pi
scp myfile.txt pi@192.168.10.2:~/

# 5. Copy files FROM Pi
scp pi@192.168.10.2:~/myfile.txt .
```

---

## Troubleshooting Guide

| Issue | Cause | Solution |
|-------|-------|----------|
| Interface shows `DOWN` | Cable disconnected or disabled | Check cable; `sudo ip link set <iface> up` |
| Ping fails | IP not assigned correctly | Verify with `ip addr show`; reassign if needed |
| `ping: connect: Network unreachable` | Routing issue | Check subnet mask: `/24` = 255.255.255.0 |
| SSH connection refused | SSH not running on Pi | Enable: `sudo systemctl start ssh` |
| `Permission denied` on SCP | SSH authentication failed | Use correct username; try SSH password login first |
| Slow file transfer | Network congestion | No fix for direct cable; this is normal |
| Can't reach after reboot | Static IP not persistent | Edit `/etc/netplan/` on Ubuntu for permanent config |

---

## Summary

✓ **Direct Ethernet Connection Established**
- Physical cable connected (UP/LOWER_UP status)
- Static IP addresses assigned (no DHCP needed)
- Network connectivity verified (ping successful)
- SSH access enabled (optional)
- File transfer working (SCP ready)

**Next Steps:**
- Use `ssh pi@192.168.10.2` for remote command execution
- Use `scp` for transferring compiled binaries and data files
- Monitor connection with `ping` to ensure stability

---

## Network Architecture

```
Ubuntu PC                    Raspberry Pi
┌─────────────────────┐      ┌─────────────────────┐
│ IP: 192.168.10.1    │◄────►│ IP: 192.168.10.2    │
│ Interface: enp6s0   │   ══ │ Interface: eth0     │
│                     │  eth │                     │
│ SSH Client          │      │ SSH Server (sshd)   │
│ SCP Client          │      │ SCP Server (sshd)   │
└─────────────────────┘      └─────────────────────┘
         PC                          Raspberry Pi
      (Ubuntu)                      (ARM64/Pi OS)
```
