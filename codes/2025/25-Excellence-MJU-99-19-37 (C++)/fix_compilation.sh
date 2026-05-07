#!/bin/bash

# 修复 CMakeCCompilerId.c
echo "/* Fixed for compilation */" > src/parser/CMakeFiles/3.22.1/CompilerIdC/CMakeCCompilerId.c
echo "int main() { return 0; }" >> src/parser/CMakeFiles/3.22.1/CompilerIdC/CMakeCCompilerId.c