CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -Iinclude
LDFLAGS = -lfl -lpthread
SRCDIR = src
INCDIR = include

# Debug/Release modes
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g -O0 -DDEBUG
    BUILD_DIR = build/debug
else
    CXXFLAGS += -O2 -DNDEBUG
    BUILD_DIR = build/release
endif

# 生成的文件
LEXER_DIR = lexer
PARSER_DIR = parser
LEX_SRC = $(LEXER_DIR)/lex.yy.c
PARSER_SRC = $(PARSER_DIR)/parser.tab.c
PARSER_HDR = $(PARSER_DIR)/parser.tab.h

# 源文件
MAIN_SRC = main.cpp
ASTPRINTER_SRC = $(SRCDIR)/astprinter.cpp
SEMANTIC_SRC = $(SRCDIR)/semantic.cpp
IRGENERATE_SRC = $(SRCDIR)/irgenerate.cpp
IRTRANSLATER_SRC = $(SRCDIR)/irtranslater.cpp
REGISTER_ALLOCATOR_SRC = $(SRCDIR)/register_allocator.cpp
# SSA模块源文件
SSA_TRANSFORM_SRC = $(SRCDIR)/ssa_transform.cpp
SSA_OPTIMIZER_SRC = $(SRCDIR)/ssa_optimizer.cpp  
SSA_DESTROYER_SRC = $(SRCDIR)/ssa_destroyer.cpp

# 目标文件
MAIN_OBJ = $(BUILD_DIR)/main.o
ASTPRINTER_OBJ = $(BUILD_DIR)/astprinter.o
SEMANTIC_OBJ = $(BUILD_DIR)/semantic.o
IRGENERATE_OBJ = $(BUILD_DIR)/irgenerate.o
IRTRANSLATER_OBJ = $(BUILD_DIR)/irtranslater.o
REGISTER_ALLOCATOR_OBJ = $(BUILD_DIR)/register_allocator.o
SSA_TRANSFORM_OBJ = $(BUILD_DIR)/ssa_transform.o
SSA_OPTIMIZER_OBJ = $(BUILD_DIR)/ssa_optimizer.o
SSA_DESTROYER_OBJ = $(BUILD_DIR)/ssa_destroyer.o
LEX_OBJ = $(BUILD_DIR)/lex.yy.o
PARSER_OBJ = $(BUILD_DIR)/parser.tab.o

# 所有目标文件
OBJS = $(MAIN_OBJ) $(ASTPRINTER_OBJ) $(SEMANTIC_OBJ) $(IRGENERATE_OBJ) $(IRTRANSLATER_OBJ) $(REGISTER_ALLOCATOR_OBJ) $(SSA_TRANSFORM_OBJ) $(SSA_OPTIMIZER_OBJ) $(SSA_DESTROYER_OBJ) $(LEX_OBJ) $(PARSER_OBJ) 

# 主编译器目标
TARGET = compiler
# 默认目标
all: $(TARGET)

# 主编译器构建
$(TARGET): $(OBJS) | $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"


# 编译规则
$(MAIN_OBJ): $(MAIN_SRC) $(PARSER_HDR) | $(BUILD_DIR)
	@echo "Compiling main.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ASTPRINTER_OBJ): $(ASTPRINTER_SRC) $(INCDIR)/astprinter.h $(INCDIR)/ast.h | $(BUILD_DIR)
	@echo "Compiling astprinter.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SEMANTIC_OBJ): $(SEMANTIC_SRC) $(INCDIR)/semantic.h $(INCDIR)/ast.h $(INCDIR)/symtab.h $(INCDIR)/types.h | $(BUILD_DIR)
	@echo "Compiling semantic.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IRGENERATE_OBJ): $(IRGENERATE_SRC) $(INCDIR)/block.h $(INCDIR)/ast.h $(INCDIR)/symtab.h | $(BUILD_DIR)
	@echo "Compiling irgenerate.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IRTRANSLATER_OBJ): $(IRTRANSLATER_SRC) $(INCDIR)/irtranslater.h $(INCDIR)/block.h $(INCDIR)/riscv_instruction.h | $(BUILD_DIR)
	@echo "Compiling irtranslater.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(REGISTER_ALLOCATOR_OBJ): $(REGISTER_ALLOCATOR_SRC) $(INCDIR)/register_allocator.h $(INCDIR)/irtranslater.h $(INCDIR)/riscv_instruction.h | $(BUILD_DIR)
	@echo "Compiling register_allocator.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SSA_TRANSFORM_OBJ): $(SSA_TRANSFORM_SRC) $(INCDIR)/ssa.h $(INCDIR)/block.h $(INCDIR)/llvm_instruction.h | $(BUILD_DIR)
	@echo "Compiling ssa_transform.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SSA_OPTIMIZER_OBJ): $(SSA_OPTIMIZER_SRC) $(INCDIR)/ssa.h $(INCDIR)/block.h $(INCDIR)/llvm_instruction.h | $(BUILD_DIR)
	@echo "Compiling ssa_optimizer.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SSA_DESTROYER_OBJ): $(SSA_DESTROYER_SRC) $(INCDIR)/ssa.h $(INCDIR)/block.h $(INCDIR)/llvm_instruction.h | $(BUILD_DIR)
	@echo "Compiling ssa_destroyer.cpp..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LEX_OBJ): $(LEX_SRC) $(PARSER_HDR) | $(BUILD_DIR)
	@echo "Compiling lexer..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PARSER_OBJ): $(PARSER_SRC) | $(BUILD_DIR)
	@echo "Compiling parser..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 生成词法分析器
$(LEX_SRC): $(LEXER_DIR)/lex.l
	@echo "Generating lexer with flex..."
	cd $(LEXER_DIR) && flex lex.l

# 生成语法分析器
$(PARSER_SRC) $(PARSER_HDR): $(PARSER_DIR)/parser.y
	@echo "Generating parser with bison..."
	cd $(PARSER_DIR) && bison -d parser.y

# 目录创建
$(BUILD_DIR):
	@echo "Creating build directory..."
	mkdir -p $(BUILD_DIR)

# Debug 构建
debug:
	$(MAKE) DEBUG=1

# Release 构建
release:
	$(MAKE) DEBUG=0

# 测试目标
test: $(TARGET)
	@echo "Running tests..."
	@chmod +x test_batch.sh
	@./test_batch.sh

# 单个测试
test-single: $(TARGET)
	@if [ -n "$(FILE)" ]; then \
		echo "Testing single file: $(FILE)"; \
		./$(TARGET) -S $(FILE); \
	else \
		echo "Usage: make test-single FILE=path/to/test.sy"; \
	fi


clean:
	@echo "Cleaning build files..."
	rm -rf build/
	rm -f $(TARGET)
	rm -f tests/h_functional/*.ll
	rm -f tests/functional/*.ll
	rm -f tests/h_functional/*.ast
	rm -f tests/functional/*.ast
	rm -f tests/h_functional/*.mir
	rm -f tests/functional/*.mir
	rm -f tests/h_functional/*.s
	rm -f tests/functional/*.s
	rm -rf test_logs/

# 深度清理
distclean: clean
	@echo "Deep cleaning generated files..."
	rm -f $(LEX_SRC)
	rm -f $(PARSER_SRC) $(PARSER_HDR)
	rm -f $(PARSER_DIR)/parser.output

# ===--------------------------------------------------------------------=== #
# 开发辅助目标
# ===--------------------------------------------------------------------=== #

# 重新构建
rebuild: clean all

# 检查语法分析器冲突
check-conflicts: $(PARSER_SRC)
	@echo "Checking parser conflicts..."
	@if [ -f $(PARSER_DIR)/parser.output ]; then \
		grep -E "(conflict|error)" $(PARSER_DIR)/parser.output || echo "No conflicts found!"; \
	else \
		echo "Parser output file not found"; \
	fi

# 显示构建信息
info:
	@echo "SYS Language Compiler Build Information"
	@echo "======================================="
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Build Dir: $(BUILD_DIR)"
	@echo "Debug Mode: $(DEBUG)"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the compiler"
	@echo "  debug        - Build debug version"
	@echo "  release      - Build release version"
	@echo "  test         - Run test suite"
	@echo "  test-single  - Test single file (use FILE=...)"
	@echo "  clean        - Remove build files"
	@echo "  distclean    - Remove all generated files"
	@echo "  rebuild      - Clean and build"
	@echo "  help         - Show this help"

# 帮助信息
help: info

# ===--------------------------------------------------------------------=== #
# 依赖关系
# ===--------------------------------------------------------------------=== #

# 确保头文件改变时重新编译
$(MAIN_OBJ): $(INCDIR)/ast.h $(INCDIR)/astprinter.h
$(ASTPRINTER_OBJ): $(INCDIR)/ast.h $(INCDIR)/astprinter.h

# 确保语法分析器改变时重新编译词法分析器
$(LEX_OBJ): $(PARSER_HDR)

# ===--------------------------------------------------------------------=== #
# 文件模式匹配（防止与文件名冲突）
# ===--------------------------------------------------------------------=== #

# 防止目标名与文件名冲突
.PHONY: all clean test debug release help rebuild check-conflicts info distclean test-single 