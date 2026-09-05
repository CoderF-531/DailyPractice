CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -pthread -Iinclude -MMD -MP

BUILD_DIR := build
BIN_DIR := bin

COMMON_SOURCES := $(wildcard src/common/*.cpp)
SERVER_SOURCES := $(COMMON_SOURCES) $(wildcard src/server/*.cpp)
CLIENT_SOURCES := $(COMMON_SOURCES) $(wildcard src/client/*.cpp)
TEST_SOURCES := $(COMMON_SOURCES) tests/protocol_test.cpp

SERVER_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SERVER_SOURCES))
CLIENT_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CLIENT_SOURCES))
TEST_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
DEPENDENCIES := $(SERVER_OBJECTS:.o=.d) $(CLIENT_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

.PHONY: all server client test clean

all: server client

server: $(BIN_DIR)/chat_server

client: $(BIN_DIR)/chat_client

$(BIN_DIR)/chat_server: $(SERVER_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/chat_client: $(CLIENT_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/protocol_test: $(TEST_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(BIN_DIR)/protocol_test
	./$(BIN_DIR)/protocol_test

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEPENDENCIES)
