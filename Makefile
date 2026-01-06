PROJECT_NAME := Demo

SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include

GLFW_INCLUDE_DIR := /opt/homebrew/opt/glfw/include
GLFW_LIB_DIR := /opt/homebrew/opt/glfw/lib
CGLM_INCLUDE_DIR := /opt/homebrew/opt/cglm/include/cglm
SOKOL_INCLUDE_DIR := /Users/eugenehong/sokol/include  

CC := clang
CXX := clang++
CFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/imgui -I$(GLFW_INCLUDE_DIR) -I$(SOKOL_INCLUDE_DIR)
CXXFLAGS := -std=c++11 -Wall -Wextra -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/imgui -I$(GLFW_INCLUDE_DIR) -I$(SOKOL_INCLUDE_DIR)
LDFLAGS := -L$(GLFW_LIB_DIR) -lglfw -ldl -framework OpenGL -framework Cocoa

SRCS_C := $(wildcard $(SRC_DIR)/*.c)
SRCS_CXX := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/imgui/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS_C)) \
        $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS_CXX))

all: $(BUILD_DIR)/$(PROJECT_NAME)

$(BUILD_DIR)/$(PROJECT_NAME): $(OBJS)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJS) -o $(BUILD_DIR)/$(PROJECT_NAME) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/imgui/%.o: $(SRC_DIR)/imgui/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

exec: $(BUILD_DIR)/$(PROJECT_NAME)
	./$(BUILD_DIR)/$(PROJECT_NAME)

.PHONY: all clean exec
