// Simple TCP client for localhost
// Connects to a server, sends text lines, and prints echoed responses.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    const char *host = "127.0.0.1"; //IPv4 loopback address (local host)
    int port = 5000;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::stoi(argv[2]);

    // -- Create Socket --
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) 
    { 
        perror("socket"); 
        return 1; 
    }

    // -- Configure Server Address --
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(port);
    if (inet_pton(AF_INET, host, &srv.sin_addr) <= 0)
    {
        std::cerr << "Invalid address: " << host << "\n";
        close(fd); return 1;
    }

    // -- Connect --
    if (connect(fd, (sockaddr*)&srv, sizeof(srv)) < 0)
    {
        perror("connect"); close(fd); return 1;
    }

    std::cout << "Connected to " << host << ":" << port << "  (type a message, Ctrl+D to quit)\n";

    std::string line;
    while (std::getline(std::cin, line)) 
    {
        // -- Send --
        line.push_back('\n');
        const char *buf = line.data();
        size_t remaining = line.size();
        while (remaining) 
        {
            ssize_t n = send(fd, buf, remaining, 0);
            if (n <= 0)
            { 
                perror("send"); close(fd); return 1; 
            }
            buf += n; 
            remaining -= n;
        }

        // -- Receive --
        std::string reply;
        char tmp[512];
        //when \n is not found, go inside the loop
        while (reply.find('\n') == std::string::npos)
        {
            ssize_t n = recv(fd, tmp, sizeof(tmp), 0);//pauses and waits here until server sends something
            if (n <= 0) 
            { 
                std::cout << "Server closed connection\n"; 
                close(fd); 
                return 0; 
            }
            reply.append(tmp, tmp + n);//copies exactly n bytes from tmp into reply
        }
        // trim newline
        while (!reply.empty() && (reply.back() == '\n' || reply.back() == '\r'))
            reply.pop_back();

        std::cout << reply << "\n";
    }

    // -- Cleanup --
    close(fd);
    return 0;
}
