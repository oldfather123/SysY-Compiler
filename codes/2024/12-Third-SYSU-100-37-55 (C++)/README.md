# 2024年全国大学生计算机系统能力大赛-编译系统设计赛-编译系统实现赛
# 中山大学-四个圣甲虫

## 项目结构

+ FrontEnd——使用SysY2022，并采用antlr进行前端的分析
+ IR（MiddleEnd）——仿照llvm的数据结构
+ Target（BackEnd）——首先将llvm compatible的IR翻译成Machine Instruction，然后进行寄存器分配，最终生成riscv的汇编代码
+ Transform——包含了多种类型的optimize方法
+ Driver——程序的入口和optimize控制入口

## 参考代码说明

+ mem2reg.cpp中与支配树相关的部分代码，参考了洛谷大佬@haochengw920大佬的实现
  - 跳转链接：https://www.luogu.com.cn/article/2d2v6vln
+ 寄存器分配中的部分代码，参考了现代编译原理-c语言描述（虎书）以及2023年参赛队伍@Yat-CC的实现
  - 跳转链接：https://gitlab.eduxiji.net/educg-group-17291-1894922/202310558201558-3109
+ loopParallel.cpp等代码文件（包括Target文件夹中的runtime.s文件的生成）中与并行化相关的部分代码，参考了2023年参赛队伍@CMMC的实现
  - 跳转链接：https://gitlab.eduxiji.net/educg-group-17291-1894922/202314325201374-1031


## 创新点说明
+ 部分循环展开
  - 对于一些较简单的循环，可以对其进行部分循环展开，将stride从1变成4（以此类推），由此减少跳转与比较的次数
+ 并行化
  - 自动生成4个不同的线程，并将其分配到不同的线程，让其执行相同的操作，实现类似于parallelfor的效果
+ branch2select
  - 将简单的块-branch组合自动转换为select指令，减少跳转开销
+ loopGEP
  - 对于之前gep指令计算出的相近的地址，我们不用重新用gep计算地址，而是计算与上一次gep的偏移值
+ double bandwith
  - 如果能判断两个sw或者lw指令的地址相邻，可以用sd或者ld来实现以减少开销