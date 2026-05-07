# RISC-V 汇编语言建模

## 静态变量

注：float和int的标记格式基本相同，区别仅为存储数值的格式，因此以下不再区分

### 基本结构

+ .globl label
+ 段描述符（见下）
+ .align k -> 按照2^k对齐
+ .type label, @object
+ .size label, size
+ label:
+ 数据内容

### 未初始化普通变量

+ .section .sbss,"aw",@nobits
+ sbss->small data bss section
+ aw->readable & writable
+ @nobits 未初始化变量不占用内存空间
+ 对齐为数据大小，通常是2^2

### 已初始化普通变量

+ .section .sdata,"aw"
+ 对齐为数据大小，通常为2^2

### 未初始化数组变量

+ .bss
+ .align 3

### 已初始化数组变量

+ .data
+ .align 3

### 普通常量

+ .section .srodata,"a"
+ 对齐为数据大小，通常为2^2

### 数组常量

+ .section .rodata
+ .align 3

## 函数

### 函数参数规范

前8个参数使用寄存器a0~a7(x10~x17)与浮点参数直接传输，剩余参数直接倒序压入栈中，靠近栈顶的为靠前的参数。

注意，参数均占据8个字节（视为64位）

### 函数头

+ .text
+ .align 1
+ .globl functionName
+ .type functionName, @function
+ functionName:
+ 函数体内容
+ .size functionName, .-functionName

### 栈帧规范

参考[这里](https://lhtin.github.io/01world/blog/riscv-function-frame.html)
```
high memory addressContent

|===============================|
|                               | 属于函数调用者的outgoing stack arguments区域，
|  incoming stack arguments     | 放在这里便于理解。
|                               | （从下往上分配，padding在上面）
|===============================| <-- caller sp
|                               |
|  callee-allocated save area   | 当参数需要两个寄存器但是只有一个可用时，需要将
|  for argument that are        | 通过寄存器传递的部分（参数中的低位）存入该区域。
|  split between register and   | 从而和存储在incoming stack arguments中的
|  the stack (x)                | 另一半组成一个整体。简称partial register区域
|                               | （从上往下分配，padding在下面）
|===============================|
|                               |
|  callee-allocated save area   | 匿名参数通过寄存器传递时，需要将其存储到该区域。
|  for register varargs (x)     | 会跟incoming stack arguments区域衔接。简称varargs区域
|                               | （从下往上分配，padding在下面）
|===============================|
|                               |
|  GPR save area                | 用于存储需要存储的callee-saved整数寄存器。
|                               | （从上往下分配，padding在下面）
|===============================|
|                               |
|  FPR save area                | 用于存储需要存储的callee-saved浮点数寄存器。
|                               |
|===============================|
|                               |
|  local variables              | 用于存储局部变量。
|                               |
|===============================|
|                               |
|  dynamic allocation           | 调用__builtin_alloca动态分配的栈内存。
|                               |
|===============================|
|                               |
|  outgoing stack arguments     | 用于存储在调用函数时通过栈传递的参数。
|                               | （从下往上分配，padding在上面）
|===============================| <-- callee sp

low memory addressContent
```


### 寄存器保存规范
在调用中不保存的寄存器需要自行在栈中保存寄存器值，并在call命令执行后恢复
寄存器分配算法执行过程中应注意call命令出现的节点，跨越节点前后的寄存器引用均需使用栈作为中介保存变量

![riscv.png](https://i.postimg.cc/y6j1vXpw/riscv.png)


# phi 指令翻译技术方案

## 