7.9日更新：
1.加入了BasicBlock,（暂时）内含前驱，后继，块名，以及IR代码（code），接下来我们所有生成的代码都将存到各个基本块里
理论上你无需调用这里面的任何函数，我们向外封装了一层，详见builder

2.加入了Builder,其主要作用是基本块的创建及切换，另外最后输出IR也是用他
```C
void Emit(std::vector<Function *> funcTable);
// 最后输出IR用，需要把MyVisitor的函数表传给他

void AppendCode(std::string& code);
//这个是你用的输出指令的东西，替换Utils::Emit函数，他会将代码直接输出到当前的基本块中

BasicBlock * AppendBasicBlock(Function * func, std::string& blockName);
// 这个是添加基本块，选择添加到哪个函数里

void PositionBuilderAtEnd(BasicBlock * block); //切换到对应的基本块(若为nullptr,则切换到全局块)

```

我们已经在MyVisitor里声明了一个属性为builder,你只需要使用builder.函数名 来调用即可

需要注意的一点是，创建一个基本块并不会使你直接切换到这个基本块中，你还需用PositionBuilderAtEnd来作切换

3. addFunc和buildCall也写好了，在MyVisitor.h里可以看到声明

如果一切顺利，你现在可以重新修改一下函数声明，以及循环分支

7.10日更新
尝试增加Array和Pointer,目前已知的信息是：
1.由于语法里没有*，所以最多可能出现一维指针(传参时)，如果形参是n维数组，对应的其实也是一个指向(n-1)维数组的指针，例如：
```C
void func(int x[][2])
```
被翻译为
```C
define dso_local void @func([2 x i32]* %0) #0 
```
也就是说多维指针实际上解引用一次后，就变成了数组
因此，作为参数的多维数组，可以看作一个pointer,其指向的元素为n-1维数组

2.在O0编译中，多维数组的取元素实际上是一层一层操作的，例如
```C
int a[2][2] = {{1,2},{3}};
a[1][1]  = 3;
```
被翻译为
```C
%4 = getelementptr inbounds [2 x [2 x i32]], [2 x [2 x i32]]* %2, i64 0, i64 1
%5 = getelementptr inbounds [2 x i32], [2 x i32]* %4, i64 0, i64 1
store i32 3, i32* %5, align 4
```
这样的做法对应在visitor中，就是看到一个[x],就生成一条指令

7.11日更新
最终还是不太能理解Array在GEP中多出来的一维，因此按照自己的想法写了。。
1.首先是创建数组，我们使用 ArrayType(Type* elementType,int ele_cnt)来进行创建，1st参数为数组元素的类型，ele_cnt为数组大小（如果是多个维度，ele_cnt指的仅是这个维度的大小）
对于多维数组，你应该从最后一个维度开始倒过来创建，如int[2][3]，应先创建Array(int,3),然后创建Array(Array(int,3),2);

2.局部数组使用BuildAlloc，全局数组使用AddGlobal,这两个应该和作业一样，需要注意的是，数组初始化赋值有一些麻烦，我不会写，所以就不要初始化了，先看看其他功能

3.最后是GEP,传入的参数为一个数组/指针ref，以及一个vector<ValueRef*>,注意，你无需因为ref是数组而多加一个0，我们已经实现了这一步，
例如a[1][2][3],对应的就是BuildGEP(a,<1,2,3>);

注意，n维数组/指针取一个下标后会变成n-1维，但是1维数组/指针再怎么GEP还是一维指针，所以别忘了GEP完了之后还要Store/Load

8.1
LLVM IR的浮点数如果用16进制表示的话，必须是64位的，因此Float_Const的value我用double表示，方便转化

8.3
如果你在开了优化后，运行程序提示你标号没有按顺序(例如定义了%2，却没有%1)，在Utils::getTemp里面，将
```C
 return "%" + std::to_string(tempCounter++);
```
改为
```C
 return "%t_" + std::to_string(tempCounter++);
```
即可，但请注意，在你合并到main分支前，务必要改回来，因为和后端有点冲突

8.5
有些pass不止需要作一遍，例如计算def-use,做完mem2reg之后，由于删除/修改了一些指令，du已经有一部分内容不是很准确了，所以如果需要，请重新计算
别忘了对于多次计算的pass,每次计算前都要把上次内容清空。

8.6
mem2reg相当于已经作了一部分常量折叠/传播，再加上你也要做，一定会留下一堆常量表达式(例如%2 = add i32 5,3),建议是为每个指令都生成一个ExecuteConst()
，判断这个指令能否在编译时就确定，若能就直接执行他。

addDef和addUse都稍微修改了一下，instruction的def_list和use_list只会加入symbol变量，如果加入非symbol型变量的话，实在是太慢了
而且我个人觉得du链应该也只会在mem2reg中使用到，所以这样该应该没问题？
如果你还需要用到CalDefUse这个函数的结果并且发现现在算的结果不对，请告诉我。
```C
void addDef(ValueRef* ref) {
        if(ref->type == SYMBOL && !dynamic_cast<Symbol*>(ref)->is_global){
            this->def_list.insert(ref);
        }
        ref->def.push_back(this);
    }

    void addUse(ValueRef* ref) {
        if(ref->type == SYMBOL && !dynamic_cast<Symbol*>(ref)->is_global){
            this->use_list.insert(ref);
        }
        ref->use.push_back(this);
    }
```