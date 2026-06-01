CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: tcp_server tcp_client

tcp_server: tcp_server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

tcp_client: tcp_client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

debug: CXXFLAGS += -g -O0
debug: tcp_server tcp_client

clean:
	rm -f tcp_server tcp_client
