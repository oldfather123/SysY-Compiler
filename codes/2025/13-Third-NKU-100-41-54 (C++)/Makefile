# 更新SRCDIR包含所有必要目录
SRCDIR += .
SRCDIR += ./tools
SRCDIR += ./llvm/generate
SRCDIR += ./llvm/semant
SRCDIR += ./llvm/optimize/analysis
SRCDIR += ./llvm/optimize/transform
SRCDIR += ./back_end/basic
SRCDIR += ./back_end/inst_process/inst_print
SRCDIR += ./back_end/inst_process/inst_select
SRCDIR += ./back_end/inst_process/phi_processing
SRCDIR += ./back_end/inst_process
SRCDIR += ./back_end/register_allocation/linear_scan
SRCDIR += ./back_end/register_allocation
SRCDIR += ./back_end/optimize

# 显式列出所有源文件（确保包含实现文件）
SRCS := $(wildcard *.cc)
SRCS += $(wildcard llvm/generate/*.cc)
SRCS += $(wildcard llvm/semant/*.cc)
SRCS += front_end/sysy_lexer.cc 
SRCS += front_end/sysy_parser.tab.cc
SRCS += $(wildcard back_end/basic/*.cc)
SRCS += $(wildcard back_end/inst_process/inst_print/*.cc)
SRCS += $(wildcard back_end/inst_process/inst_select/*.cc)
SRCS += $(wildcard back_end/inst_process/*.cc)
SRCS += $(wildcard back_end/inst_process/phi_processing/*.cc)
SRCS += $(wildcard back_end/register_allocation/linear_scan/*.cc)
SRCS += $(wildcard back_end/register_allocation/*.cc)
SRCS += $(wildcard back_end/optimize/*.cc)

NAME = compiler
BINARY = $(NAME)  # 直接生成在根目录

OBJDIR ?= ./obj

CC = clang++
LD = clang++
INCLUDES = $(addprefix -I, $(SRCDIR))
# 内存检测 + 未定义行为检测
# CFLAGS += -O2 -MMD -std=c++17 $(INCLUDES) -fsanitize=address,undefined -fno-omit-frame-pointer 
# LDFLAGS += -fsanitize=address,undefined

# 未定义行为检测
# CFLAGS += -O2 -MMD -std=c++17 $(INCLUDES) -fsanitize=undefined -fno-omit-frame-pointer -g
# LDFLAGS += -fsanitize=undefined -g


CFLAGS += -O2 -MMD -std=c++17 $(INCLUDES)

SRCS := $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.cc))
SRCS += front_end/sysy_lexer.cc front_end/sysy_parser.tab.cc

OBJS := $(patsubst %.cc, $(OBJDIR)/%.o, $(SRCS))

DEPS := $(OBJS:.o=.d)
-include $(DEPS)

.DEFAULT_GOAL = $(BINARY)

$(OBJDIR)/%.o : %.cc
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BINARY): $(OBJS)
	@echo + LD $@
	@$(LD) $(LDFLAGS) $(OBJS) -o $@ -O2 -std=c++17

lexer: front_end/sysy_lexer.l
	@echo "[FLEX] Generating lexer..."
	@flex -o front_end/sysy_lexer.cc front_end/sysy_lexer.l
	@echo "[FLEX] Lexer generated successfully"

parser: front_end/sysy_parser.y
	@echo "[BISON] Generating parser..."
	@bison -dv front_end/sysy_parser.y -o front_end/sysy_parser.tab.cc
	@rm -f front_end/sysy_parser.output
	@sed -i '1s/^/#include "..\/include\/ast.h"\n/' front_end/sysy_parser.tab.hh
	@echo "[BISON] Parser generated successfully"

clean-obj:
	@echo "Cleaning object files..."
	@rm -rf $(OBJDIR)/*

clean-all: clean-obj
	@echo "Cleaning executable..."
	@rm -f $(BINARY)

format:
	@echo "Formatting source files..."
	@clang-format -style=file -i $(SRCS)
	@clang-format -style=file -i $(foreach dir,$(SRCDIR),$(wildcard $(dir)/*.h))

.PHONY: all clean-obj clean-all lexer parser format

##################### Debug ###################
# Debug 编译参数
CFLAGS_DBG := -g -O2 -MMD -std=c++17 $(INCLUDES)
LDFLAGS_DBG := -g -O2 -std=c++17

BINARY := compiler
BINARY_DBG := $(BINARY)-dbg

# 从原始 OBJS 推导对应的 .dbg.o 中间目标
OBJS := $(patsubst %.cc, $(OBJDIR)/%.o, $(SRCS))
DBG_OBJS := $(patsubst %.cc, %.dbg.o, $(SRCS)) 

# Debug 编译规则
$(OBJDIR)/%.dbg.o : %.cc
	@echo + DBG_CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS_DBG) -c -o $@ $<

# Debug 构建目标
debug: $(OBJS:.o=.dbg.o)
	@echo + LD $(BINARY_DBG)
	@$(LD) $(LDFLAGS_DBG) $^ -o $(BINARY_DBG)

# Debug 清理目标
clean-dbg:
	@echo "Cleaning debug object files and executable..."
	@find . -name "*.dbg.d" -delete
	@rm -rf $(OBJDIR)
	@rm -f $(BINARY_DBG)

