#############################################
##### Makefile for C++ Compiler Project #####
#############################################

# C++ compiler, for the main program
CXX = clang++
CXXFLAGS = --std=c++17 -O2 -MMD -MP -g
LDFLAGS = -lm

# C compiler, for riscv library
CC = riscv64-linux-gnu-gcc
CFLAGS = -std=c99 -O2 -g -Wall -Wextra

INCLUDES = \
    -Iinclude/ir \
    -Iinclude/backend \
    -Iinclude/front \
    -Iinclude/json \
    -Iinclude/tools \
    -Idebug.h

TARGET = compiler

# 源代码文件
SRC := \
    $(wildcard src/**/*.cpp) \
    $(wildcard src/*.cpp) \
    main.cpp

# 构建目录
BUILD_DIR = build

# 在build目录下创建相同的目录结构
OBJ := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# 自动创建必要的目录
$(shell mkdir -p $(dir $(OBJ)))

# 默认使用所有CPU核心
JOBS := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

all: build

build: $(TARGET)

lib:
	$(MAKE) -j$(JOBS) -C test/lib libsy_riscv.a CC="$(CC)" CFLAGS="$(CFLAGS)"

# 编译规则
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 链接规则
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# 包含自动生成的依赖
-include $(DEP)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all build clean lib
