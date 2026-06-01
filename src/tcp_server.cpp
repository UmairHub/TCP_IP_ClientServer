// ─── Includes ────────────────────────────────────────────────────────────────

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

// ─── Globals ─────────────────────────────────────────────────────────────────

static std::atomic<bool> running{true};
static std::mutex cout_mtx;

// ─── Client Handler ──────────────────────────────────────────────────────────

void handle_client(int client_fd, sockaddr_in addr)
{
    // -- Resolve client address --
    char addr_str[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr.sin_addr, addr_str, sizeof(addr_str));
    int port = ntohs(addr.sin_port);

    {
        std::lock_guard<std::mutex> lg(cout_mtx);
        std::cout << "Client connected: " << addr_str << ":" << port << " (fd=" << client_fd << ")\n";
    }

    // -- Read loop --
    std::string buffer;
    buffer.reserve(1024);
    char tmp[512];
    while (running) {
        ssize_t n = read(client_fd, tmp, sizeof(tmp));
        if (n > 0) {
            buffer.append(tmp, tmp + n);
            // process lines
            size_t pos = 0;
            while (true) {
                size_t nl = buffer.find('\n', pos);
                if (nl == std::string::npos) break;
                std::string line = buffer.substr(0, nl);
                buffer.erase(0, nl + 1);

                // trim carriage return
                if (!line.empty() && line.back() == '\r') line.pop_back();

                // print timestamp and payload
                auto now = std::chrono::system_clock::now();
                std::time_t t = std::chrono::system_clock::to_time_t(now);
                std::lock_guard<std::mutex> lock(cout_mtx);
                std::cout << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "] ";
                std::cout << addr_str << ":" << port << " -> " << line << std::endl;
            }
        } else if (n == 0) {
            // EOF: client closed
            break;
        } else {
            if (errno == EINTR) continue;
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cerr << "Read error from " << addr_str << ":" << port << " -> " << strerror(errno) << std::endl;
            break;
        }
    }

    // -- Cleanup --
    close(client_fd);
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << "Client disconnected: " << addr_str << ":" << port << "\n";
}

// ─── Signal Handler ──────────────────────────────────────────────────────────

void sigint_handler(int)
{
    running = false;
}

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char **argv)
{
    // -- Parse port argument --
    int port = 4000;
    if (argc >= 2) {
        port = std::stoi(argv[1]);
        if (port <= 0 || port > 65535) port = 4000;
    }

    signal(SIGINT, sigint_handler);

    // -- Create and configure socket --
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    
    // -- Bind --
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all network interfaces (eth0, wlan0, lo)
    addr.sin_port = htons(port);

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        return 1;
    }

    // -- Listen --
    if (listen(listen_fd, 10) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        close(listen_fd);
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "TCP server listening on port " << port << "\n";
    }

    // -- Accept loop --
    std::vector<std::thread> threads;
    while (running) {
        sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(listen_fd, (sockaddr*)&cli_addr, &cli_len);
        if (client_fd < 0) {
            if (errno == EINTR) break;
            std::cerr << "accept() failed: " << strerror(errno) << std::endl;
            break;
        }

        threads.emplace_back(handle_client, client_fd, cli_addr);
        // detach small number of threads to avoid join complexity; we keep them in vector and join on shutdown
        threads.back().detach();
    }

    // -- Shutdown --
    close(listen_fd);
    running = false;
    std::cout << "Server shutting down...\n";
    // threads are detached; give brief time for handlers to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return 0;
}
