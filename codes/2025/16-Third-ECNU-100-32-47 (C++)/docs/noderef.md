# 总体结构
Node分为两类，一类是BindNode一类是InsnNode
BindNode用于声明全局的符号并将其与函数(FunctionNode)/初始值(InitializeNode)绑定在一起。
InsnNode（几乎）对应于程序的指令，如mov,add,sub等

## BindNode相关
### 创建函数
通过FunctionNode完成
FunctionNode维护着一个InsnNode的List，通过addNode向其中添加Node。
### 全局变量的初始化
通过InitializeNode完成，
如果全局变量不是一个数组，那么就不需要为初始化提供indexes参数
如果是一个数组，则需要提供indexes参数
如
``` cpp
InitializeNode::create(var1,{},value1);// 将var1代表的全局变量的值设置为value1
InitializeNode::create(var2,{1,2},value2);// 将var2代表的全局变量的值设置为value2
```

## InsnNode相关
### LabelNode
接受一个string作为标号的名称，也可以通过generateTempLabel生成一个Label而无需手动命名。

### CallNode
接受一个函数和一系列的参数，还有一个可选的result
``` cpp
CallNode::create(func,{arg1,arg2});// 调用func(arg1,arg2); 它的返回值被丢弃
CallNode::create(func,result,{arg1,arg2});// 调用func(arg1,arg2); 返回值存入result
```

### ConditionNode
根据 op1 OP op2的结果，判断跳转到true_label还是false_label

### JumpNode
无条件跳转到某个标号

### ReturnNode
返回，可以带有一个值，如果是void，那么无需指明返回值
``` cpp
ReturnNode::create();// void类型的函数的返回
ReturnNode::create(value);// 返回result
```
### AssignNode
完成result = op1 OP op2这样的操作
OP可以是ADD SUB MUL等
同时也可以是一元运算符，如Copy，Neg等
### LoadNode/StoreNode
非数组全局变量视为一个内存地址，需要使用load/store读取到sym::Var或者从sym::Var写入到内存中
全局数组和局部数组一样，都要使用Load/Store加上indexes的方式来读取，参考全局变量的初始化。
