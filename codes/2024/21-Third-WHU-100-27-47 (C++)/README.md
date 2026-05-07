# 编译系统设计赛项目仓库

## 文档

[设计分析文档](设计分析文档.md)

## 如何生成

-   在仓库根目录 `sudo bash ./run.sh` 以进行编译并运行

## 约定

### 通用头文件

[`common/Common.h`](/src/common/Common.h)

### 命名规范

按 Java 规范。

-   变量：`camelCase`，例：`int blockId`
-   函数：`camelCase`，例：`void addItem()`
-   类型：`PascalCase`，例：`class Expr`，`struct Value`，`enum Type`
-   枚举值：`PascalCase`，例：`Operator::Add`
-   私有成员：`_camelCase`（前置下划线）

### 格式规范

见 [/.clang-format](/.clang-format)。

VSCode 环境下的文档格式化工具已经配置好，见 [/.vscode/settings.json](/.vscode/settings.json)。

### 指针

自己定义的指针都使用 `Ptr<>`（`std::shared_ptr<>`）

只有 `class` 才能用指针，`struct` 只能用引用。

### 调试输出

使用 `dbgout << ...` 进行调试输出。

## 前端部分

**目录**：[`frontend/`](/src/frontend)

## IR 部分

**目录（含 [README](/src/midend/ir/README.md) 文档）**：[`midend/ir/`](/src/midend/ir)

## ANTLR

antlr4 4.12.0 JAR 包

```text
//安装jdk
sudo apt-get install openjdk-17-jre-headless

//安装antlr4
cd /usr/local/lib
wget https://www.antlr.org/download/antlr-4.12.0-complete.jar
export CLASSPATH=".:/usr/local/lib/antlr-4.12.0-complete.jar:$CLASSPATH"

//配置别名
alias antlr4='java -jar /usr/local/lib/antlr-4.12.0-complete.jar'
alias grun='java org.antlr.v4.gui.TestRig'
```

然后为方便使用，在`.bashrc`里面添加后三行：

```text
vim /home/zeroregister/.bashrc
export CLASSPATH=".:/usr/local/lib/antlr-4.12.0-complete.jar:$CLASSPATH"
alias antlr4='java -jar /usr/local/lib/antlr-4.12.0-complete.jar'
alias grun='java org.antlr.v4.gui.TestRig'
```

使 bashrc 生效

```
cd /home/zeroregister
source ./bashrc
```

antlr4 4.12.0 C++ runtime
安装运行时库的方法是，去官网找 C++的 runtime library 源码，然后自己编译。

```
wget https://www.antlr.org/download/antlr4-cpp-runtime-4.12.0-source.zip
unzip antlr4-cpp-runtime-4.12.0-source.zip -d antlr4-cpp-runtime-4.12.0

//开始编译
cd antlr4-cpp-runtime-4.12.0/
mkdir build && mkdir run && cd build
cmake ..
make install DESTDIR=../run

//把生成的文件放在系统文件中
cd ../run/usr/local/include
sudo \cp -r antlr4-runtime/* /usr/local/include
cd ../lib
sudo \cp -r * /usr/local/lib
sudo ldconfig
```

## 运行时库相关

### 将 sylib.c 和 sylib.h 编译链接为 libsysy.a

```shell
riscv64-linux-gnu-gcc -Wall -c sylib.c -o sylib.o
riscv64-linux-gnu-ar cr libsysy.a sylib.o
```

### 将运行时库调用 getint.sy 进行编译、链接

-   getint.sy

```c
int a;
int main()
{
	a=getint();
	return a;
}
```

-   getint.s

```c
	.text
	.section .bss, "aw", @nobits
	.globl a
	.type a, @object
	.size a, 4
a:
	.space 4
	.data
	.text
	.globl main
	.type main, @function
main:
	sd fp, -8(sp)
	ori fp, sp, 0
	addi sp, sp, -32
	sd ra, -16(fp)
main_label_entry:
	call getint
	lla t6, a
	sw a0, 0(t6)
	lla t6, a
	lw a0, 0(t6)
main_exit:
	ld ra, -16(fp)
	ori sp, fp, 0
	ld fp, -8(sp)
	jr ra
```

-   编译

```shell
compiler -S -o getint.s getint.sy
```

-   汇编并链接运行时库

```shell
riscv64-linux-gnu-gcc getint.s -o getint.bin -Lsysy_runtime -lsysy
```

-   运行

```shell
(base) root@ZeroRegister:/home/zeroregister/CompilerCompetition/sysy-compiler/test_cases# qemu-riscv64 getint.bin
123
TOTAL: 0H-0M-0S-0us
(base) root@ZeroRegister:/home/zeroregister/CompilerCompetition/sysy-compiler/test_cases# echo $?
123
```
