Build command:

cross-compile tcp_client (raspberry pi app):
aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -pthread -o test_app_aarch64 /home/umair/Data/04_C++/03_Demo/test_app.cpp


compile tcp_server:
g++ -std=c++17 -O2 -Wall -pthread -o tcp_server tcp_server.cpp
