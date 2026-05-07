# Bison Parser.y 文件找不到问题解决方案

## 问题描述
```
bison: parser.y: cannot open: No such file or directory
```

## 问题分析
项目中实际的语法分析文件是 `syntax_analyzer.y`，但某些工具或脚本期望找到 `parser.y` 文件。

## 项目文件结构
```
src/parser/
├── syntax_analyzer.y    # 实际的bison语法文件
├── lexical_analyzer.l   # flex词法文件
├── parser.c            # parser主程序
├── lexer.c             # lexer主程序
└── CMakeLists.txt      # CMake配置文件
```

## 解决方案

### 方案1: 创建符号链接 ✅ 已实施
```bash
cd /home/suo/compiler-project/src/parser
ln -sf syntax_analyzer.y parser.y
```

**优点**: 
- 简单快速
- 保持原文件不变
- 满足期望找到parser.y的工具需求

**结果**: 
```
lrwxrwxrwx 1 suo suo 17 Jul 24 08:57 parser.y -> syntax_analyzer.y
```

### 方案2: 使用CMake构建系统 ✅ 推荐
```bash
cd /home/suo/compiler-project
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .
cd build
make parser -j$(nproc)
```

**优点**:
- 使用项目标准构建流程
- 自动处理bison/flex依赖
- 生成正确的目标文件

**结果**:
- 生成可执行文件: `/home/suo/compiler-project/build/parser`
- 大小: 71,992 bytes

### 方案3: 直接使用bison命令
如果需要手动运行bison：
```bash
cd /home/suo/compiler-project/src/parser
bison -d syntax_analyzer.y  # 使用正确的文件名
# 或者现在可以使用
bison -d parser.y           # 通过符号链接
```

## 验证结果

✅ **符号链接创建成功**: `parser.y -> syntax_analyzer.y`  
✅ **CMake构建成功**: parser可执行文件正常生成  
✅ **文件完整性**: 原始文件未被修改  
✅ **兼容性**: 满足不同工具的文件名期望  

## 最佳实践建议

1. **优先使用CMake**: 项目已配置好完整的构建系统
2. **避免直接调用bison**: 使用 `make parser` 而不是手动bison命令
3. **保持文件名一致**: 如果多个工具期望相同文件名，使用符号链接是好方案

## 相关文件
- **语法文件**: `src/parser/syntax_analyzer.y`
- **词法文件**: `src/parser/lexical_analyzer.l`  
- **构建配置**: `src/parser/CMakeLists.txt`
- **可执行文件**: `build/parser`

问题已完全解决，项目现在可以正常使用bison进行语法分析文件的处理。
