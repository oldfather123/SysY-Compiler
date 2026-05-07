# SysY-Compiler 环境配置

本文记录在 WSL Ubuntu 24.04 环境下配置编译器开发工具链、构建本项目和运行 SysY 测试用例的步骤，包括 C/C++ 编译器、LLVM/Clang、ANTLR v4 和 CMake。

## 1. 安装 WSL Ubuntu 24.04

在 Windows 终端中执行：

```bash
wsl --install -d Ubuntu-24.04
```

## 2. 安装 LLVM/Clang 18

进入 Ubuntu 后，安装 C/C++ 编译器及 LLVM 相关工具：

```bash
sudo apt update
sudo apt install -y build-essential clang-18 lld-18 llvm-18 llvm-18-dev
```

可选：将默认 `clang` / `clang++` 指向 18 版本。

```bash
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100
```

## 3. 安装 ANTLR v4（非必须安装，已有生成文件）

### 3.1 安装 Java

ANTLR 4.13.2 需要 Java 11 或更高版本。

```bash
sudo apt install -y default-jdk curl
```

### 3.2 下载 ANTLR

```bash
cd /usr/local/lib
sudo curl -O https://www.antlr.org/download/antlr-4.13.2-complete.jar
```

也可以直接从浏览器下载：

```text
https://www.antlr.org/download.html
```

下载后将 `antlr-4.13.2-complete.jar` 放到合适位置，例如 `/usr/local/lib`。

### 3.3 配置 CLASSPATH

将 ANTLR jar 包加入 `CLASSPATH`：

```bash
echo 'export CLASSPATH=".:/usr/local/lib/antlr-4.13.2-complete.jar:$CLASSPATH"' >> ~/.bashrc
```

### 3.4 配置 ANTLR 命令别名

为 ANTLR 工具和 TestRig 创建别名：

```bash
echo 'alias antlr4='\''java -Xmx500M -cp "/usr/local/lib/antlr-4.13.2-complete.jar:$CLASSPATH" org.antlr.v4.Tool'\''' >> ~/.bashrc
echo 'alias grun='\''java -Xmx500M -cp "/usr/local/lib/antlr-4.13.2-complete.jar:$CLASSPATH" org.antlr.v4.gui.TestRig'\''' >> ~/.bashrc
```

### 3.5 使配置生效

```bash
source ~/.bashrc
```

## 4. 使用 ANTLR v4（非必须运行，已有生成文件）

本项目的语法文件位于 `src/frontend`，词法和语法拆分为 `SysYLexer.g4` 与 `SysYParser.g4`。重新生成 C++ 代码时执行：

```bash
cd src/frontend
antlr4 -Dlanguage=Cpp -visitor -no-listener -o generated SysYLexer.g4 SysYParser.g4
cd ../..
```

生成结果会写入 `src/frontend/generated`。

## 5. 安装并运行 CMake

```bash
sudo apt install -y cmake
cmake -S src -B src/build
cmake --build src/build
```

构建成功后，可执行文件为`src/build/compiler`。

## 6. 测试脚本

### 6.1 安装 Git LFS

```bash
sudo apt update
sudo apt install git-lfs
git lfs install
```

### 6.2 下载解压测试集

```bash
git lfs pull
unzip test.zip
```

### 6.3 运行测试脚本

运行并对比一个测试目录：

```bash
src/scripts/test.sh test/2026/functional
```

运行并对比单个 `.sy` 文件：

```bash
src/scripts/test.sh test/2026/functional/00_main.sy
```

脚本默认单个用例超时时间为 25 秒，可通过 `RUN_SY_TIMEOUT` 调整：

```bash
RUN_SY_TIMEOUT=60s src/scripts/test.sh test/2026/functional
```
