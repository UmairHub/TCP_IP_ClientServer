// Simple TCP echo server for localhost
// Listens on a port, accepts one client, and echoes received lines back.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstring>
#include <iostream>
#include <string>

static volatile bool running = true;

static void sigint_handler(int) 
{ 
    running = false; 
}

int main(int argc, char **argv)
{
    int port = 5000;
    if (argc >= 2) port = std::stoi(argv[1]);

    signal(SIGINT, sigint_handler);

    // -- Create Socket --
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }

    // -- Set Socket Options --
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    // -- Configure Address --
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // -- Bind --
    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind"); close(listen_fd); return 1;
    }
    // -- Listen --
    if (listen(listen_fd, 5) < 0)
    {
        perror("listen"); close(listen_fd); return 1;
    }

    std::cout << "Server listening on 127.0.0.1:" << port << "  (Ctrl+C to quit)\n";

    while (running)
    {
        // -- Accept --
        sockaddr_in cli_addr{};
        socklen_t cli_len = sizeof(cli_addr);
        int client_fd = accept(listen_fd, (sockaddr*)&cli_addr, &cli_len);
        if (client_fd < 0)
        {
            if (errno == EINTR) break;
            perror("accept"); break;
        }

        std::cout << "Client connected (fd=" << client_fd << ")\n";

        std::string buf;
        char tmp[512];
        while (running)
        {
            // -- Read --
            ssize_t n = read(client_fd, tmp, sizeof(tmp));
            if (n > 0)
            {
                buf.append(tmp, tmp + n);
                while (true)
                {
                    size_t nl = buf.find('\n');
                    if (nl == std::string::npos) 
                        break;
                    std::string line = buf.substr(0, nl);
                    buf.erase(0, nl + 1);
                    if (!line.empty() && line.back() == '\r') 
                        line.pop_back();

                    std::cout << "Received: " << line << "\n";

                    std::string reply = "ECHO: " + line + "\n";
                    // -- Send --
                    send(client_fd, reply.data(), reply.size(), 0);
                }
            }
            else if (n == 0)
            {
                std::cout << "Client disconnected\n";
                break;
            }
            else
            {
                if (errno == EINTR)
                {
                    continue;
                } 
                    
                perror("read");
                {
                    break;
                }
                    
            }
        }

        // -- Close Client --
        close(client_fd);
    }

    // -- Close Server --
    close(listen_fd);
    std::cout << "Server shut down.\n";
    return 0;
}
