#!/bin/bash

# 定义颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 定义项目根目录
PROJECT_DIR="/mnt/d/桌面/编译原理/compiler2025-x"
cd "$PROJECT_DIR"

echo -e "${BLUE}正在编译程序...${NC}"

# 编译程序
g++ -I"$PROJECT_DIR/src" \
   "$PROJECT_DIR/src/main/main.cpp" \
   "$PROJECT_DIR/src/frontend/type.cpp" \
   -o "$PROJECT_DIR/main"

# 检查编译是否成功
if [ $? -eq 0 ]; then
    echo -e "${GREEN}编译成功！正在运行程序...${NC}"
    echo -e "${GREEN}------------------------${NC}"
    
    # 运行程序
    "$PROJECT_DIR/main"
    
    echo -e "${GREEN}------------------------${NC}"
    echo -e "${GREEN}程序执行完毕！${NC}"
else
    echo -e "${RED}编译失败，请检查错误信息。${NC}"
    exit 1
fi 