.CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic
TARGETS := UDPClient UDPServer

ifneq ($(OS),Windows_NT)
RM := rm -f
else
LDLIBS += -lws2_32
RM := del /Q
endif

.PHONY: all clean
all: $(TARGETS)

UDPClient: UdpClient.cpp UdpServer.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

UDPServer: UdpServer.cpp UdpServer.hpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) UDPClient UDPServer UDPClient.exe UDPServer.exe
