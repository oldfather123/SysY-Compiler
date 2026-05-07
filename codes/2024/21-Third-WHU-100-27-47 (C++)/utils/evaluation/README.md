# 测试用例

[测试用例链接](https://gitlab.eduxiji.net/csc1/nscscc/compiler2024/-/blob/main/testdata.zip)

请将 `funcitonal` 和 `performance` 文件夹放在 `test_cases` 目录下。

目录结构如下：

```
test_cases
├── functional
│   ├── 00_main.out
│   ├── 00_main.sy
|   └── ...
├── performance
│   ├── 01_mm1.in
│   ├── 01_mm1.out
│   ├── 01_mm1.sy
│   └── ...
├── case1.sy
├── case2.sy
└── ...
```

# 测试流程

## 安装

-   安装 RISC-V 虚拟机

    ```shell
    sudo apt install qemu-user
    ```

-   安装 gcc 的 RISC-V 编译工具

    ```shell
    sudo apt update
    sudo apt install build-essential gcc make perl dkms git gcc-riscv64-unknown-elf gdb-multiarch qemu-system-misc
    sudo apt install gcc-riscv64-linux-gnu
    ```

    如果报错 `/lib/ld-linux-riscv64-lp64d.so.1: No such file or directory`，执行下面命令：

    ```shell
    sudo cp /usr/riscv64-linux-gnu/lib/* /lib/
    ```

## 手动编译并执行

以下为先编译为汇编再编译为机器码过程

-   使用 `gcc-riscv64-linux-gnu` 编译 C 代码为 RISC-V 汇编（或者用 `compiler` 将 SysY 代码编译为 RISC-V 汇编）

    ```shell
    riscv64-linux-gnu-gcc -S -O2 test.c -o test.risc.s
    ```

-   使用 `riscv64-linux-gnu-gcc` 将 RISC-V 汇编转换为机器码

    ```shell
    riscv64-linux-gnu-gcc test.risc.s -o test.risc.bin
    ```

-   使用虚拟机运行该机器码

    ```shell
    qemu-riscv64 test.risc.bin
    ```

---

以下为直接编译成机器码过程

-   直接使用 `riscv64-linux-gnu-gcc` 编译 `.c` 文件为 `.out` 可执行文件

    ```shell
    riscv64-linux-gnu-gcc test.c
    ```

-   运行

    ```shell
    qemu-riscv64 a.out
    ```

## 使用本地评测脚本

-   使用 VSCode Task

    在 `Tasks: Run Task` 中，选择 `运行本地测评`。

-   手动运行

    以超级用户权限运行 `utils/evaluation/evaluator.py`（可以使用 Code Runner 插件）：

    ```shell
    cd utils/evaluation && sudo python evaluator.py
    ```

    并按提示操作即可。
