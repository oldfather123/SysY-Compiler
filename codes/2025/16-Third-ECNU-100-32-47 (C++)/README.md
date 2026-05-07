# accc
`accc` stands for `accc ain't a compiler`

具体文档请见 `doc.pdf`，本文件目前已过时

## 项目结构
```bash
./src/include       #头文件相关
./src/reg_allocate  #寄存器分配相关
./src/aaac-main.cc  #主函数

./tests             #测试相关内容
./docs              #项目文档
```

## 构建
```bash
mkdir build && cd build
cmake ..
make
cd ./src && ./aaac-main
```



## TODO
1. 后端Function生成的时候要有Var注册，方便对于逃逸变量和数组在栈上的分配
2. t0,t1,t2暂时被用于处理寄存器溢出时的临时变量
3. 可以增加一个强绑定的Insn

## 有关栈帧大小计算以及callee-save, local var, spilled register的offset计算
先做emit_backend，此时可以计算出callee-save的大小和local var的大小。后者在lowring的时候需要注册到对应的function中。目前感觉也只有数组需要在栈上。
再寄存器分配，spilled register，基于sp做偏移
最后codegen

## 一些改动
AddressOf这个操作变为基于一个var的fp_offset和s0计算地址，由于此时作为src的var实际上是一个占位符，所以我们不会在寄存器分配的时候考虑var的问题

## ops
1. 这里为了方便有一个较强的约束，保留t0,t1,t2不用于寄存器分配，只用于溢出和一些算术中间量（如li等），且t0用于dest，t1用于src0，t2用于src1

## misc
`cmake --build build --target install -- -j32`可以编译加安装`main`到output/bin/main
