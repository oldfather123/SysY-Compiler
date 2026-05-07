### 创建模块

```c++
ANTPIE::Module* module = new ANTPIE::Module();
```

### 创建类型

```c++
// 基础类型
Int1Type* i1Type = Type::getInt1Type();
Int32Type* i32Type = Type::getInt32Type();
FloatType* floatType = Type::getFloatType();
VoidType* voidType = Type::getVoidType();

// 指针类型
PointerType* ptrType = Type::getPointerType(i32Type); // 指向i32类型的指针

// 数组类型
ArrayType* arrType = Type::getArrayType(5, i32Type); // 长度为5的整数数组
```

### 创建函数

```c++
// 创建函数类型
vector<Argument*> args; // 参数列表
args.push_back(new Argument("x", i32Type)); // 参数 int x
args.push_back(new Argument("y", i32Type)); // 参数 int y
FuncType* funcType = Type::getFuncType(i32Type /* 返回值类型 */, args);

// 创建并插入函数
module->addFunction(funcType, "main");
```

### 创建基本块

```c++
// 在函数末尾插入基本块
BasicBlock* bb1 = module->addBasicBlock(fFunction, "foo.entry");
// 之后新增的指令都将在此基本块后追加
module->setCurrBasicBlock(bb1);
```

### 插入指令

插入指令的方法如下

```c++
AllocaInst* addAllocaInst(Type* type, string name);
BinaryOpInst* addBinaryOpInst(OpTag opType, Value* op1, Value* op2, string name);
BranchInst*  addBranchInst(Value* cond, BasicBlock* trueBlock, BasicBlock* falseBlock);
CallInst* addCallInst(Function* func, string name);
CallInst* addCallInst(Function* func, vector<Value*>& params, string name);
IcmpInst* addIcmpInst(OpTag opType, Value* op1, Value* op2, string name);
FcmpInst* addFcmpInst(OpTag opType, Value* op1, Value* op2, string name);
FptosiInst* addFptosiInst(Value* src, string name);
GetElemPtrInst* addGetElemPtrInst(Value* ptr, Value* idx1, Value* idx2, string name);
GetElemPtrInst* addGetElemPtrInst(Value* ptr, Value* idx1, string name);
JumpInst* addJumpInst(BasicBlock* block);
LoadInst* addLoadInst(Value* addr, string name);
PhiInst* addPhiInst(string name);
ReturnInst* addReturnInst(Value* retValue);
ReturnInst* addReturnInst();
SitofpInst* addSitofpInst(Value* src, string name);
StoreInst* addStoreInst(Value* value, Value* addr);
ZextInst* addZextInst(Value* src, Type* dstType, string name);
```

使用示例：

```c++
// 创建main函数
FuncType* mFuncType = Type::getFuncType(Type::getInt32Type()); // 参数列表可以缺省
Function* mFunction = module->addFunction(mFuncType, "main");
BasicBlock* basicBlock = module->addBasicBlock(mFunction, "entry");
module->setCurrBasicBlock(basicBlock);

AllocaInst* allocaInst = module->addAllocaInst(Type::getInt32Type(), "a.addr");
// 创建整数常量
IntegerConstant* intConstant = IntegerConstant::getConstInt(10);
StoreInst* storeInst = module->addStoreInst(intConstant, allocaInst);
LoadInst* LoadInst = module->addLoadInst(allocaInst, "a");

// call指令
// 方式一
vector<Value*> callArgs;
callArgs.push_back(IntegerConstant::getConstInt(4));
CallInst* callInst = module->addCallInst(fFunction, callArgs, "foo.ret");
// 方式二
CallInst* callInst = module->addCallInst(fFunction, "foo.ret");
callInst->pushArgument(IntegerConstant::getConstInt(4));
```

### 全局变量

```c++
// 创建全局变量
GlobalVariable* gv = module->addGlobalVariable(Type::getInt32Type(), "gx"); // 无初始值
GlobalVariable* gv = module->addGlobalVariable(Type::getInt32Type(), IntegerConstant::getConstInt(5), "gx"); // 有初始值
// 使用全局变量
LoadInst* gxLoad = module->addLoadInst(gv, "gv.val"); // gv是一个int*指针，必须先load
BinaryOpInst* retAddInstr = module->addBinaryOpInst(ADD, IntegerConstant::getConstInt(5), gxLoad, "ret2");

// 全局数组
// 有初始值
ArrayType* arrType = Type::getArrayType(10, Type::getInt32Type());
ArrayConstant* array = ArrayConstant::getConstArray(arrType);
array->put(2, new IntegerConstant(5));
array->put(3, new IntegerConstant(3));
GlobalVariable* garr = module->addGlobalVariable(arrType, array, "garr");
// 无初始值
GlobalVariable* emptyArr = module->addGlobalVariable(arrType, "empty");
```

### 输出IR

```c++
std::ofstream out_file;
out_file.open("tests/test.ll");
module->printIR(out_file);
```

### 示例

```c++
int main() {
  // ir使用示例

  ANTPIE::Module* module = new ANTPIE::Module();

  /**
   * int gx = 5;
   * int garr[10] = {0, 0, 5, 3};
   * int empty[10];
   * float gf = 1.5;
   * int foo(int x) {
   *   int arr[10];
   *   arr[1] = x;
   *   int ret = arr[1];
   *   if (5 <= ret) {
   *     ret = 5;
   *   }
   *   ret = ret + gx;
   *   return ret;
   * }
   */

  // add global variable
  // @gx = dso_local global i32 0
  GlobalVariable* gv = module->addGlobalVariable(Type::getInt32Type(), "gx");
  // gloabl array
  ArrayType* arrType = Type::getArrayType(10, Type::getInt32Type());
  ArrayConstant* array = ArrayConstant::getConstArray(arrType);
  array->put(2, new IntegerConstant(5));
  array->put(3, new IntegerConstant(3));
  // @garr = dso_local global [10 x i32] [i32 0, i32 0, i32 5, i32 3, i32 0, i32
  // 0, i32 0, i32 0, i32 0, i32 0]
  GlobalVariable* garr = module->addGlobalVariable(arrType, array, "garr");
  // global empty array
  // @empty = dso_local global [10 x i32] zeroinitializer
  GlobalVariable* emptyArr = module->addGlobalVariable(arrType, "empty");
  // @gf = dso_local global float 1.500000
  GlobalVariable* gf = module->addGlobalVariable(
      Type::getFloatType(), FloatConstant::getConstFloat(1.5), "gf");

  vector<Argument*> args;
  Argument* argument = new Argument("x", Type::getInt32Type());
  args.push_back(argument);
  FuncType* funcType = Type::getFuncType(Type::getInt32Type(), args);
  // define dso_local i32 @foo(i32 %x) {
  Function* fFunction = module->addFunction(funcType, "foo");
  // foo.entry:
  BasicBlock* bb1 = module->addBasicBlock(fFunction, "foo.entry");
  module->setCurrBasicBlock(bb1);
  // %arr.addr = alloca [10 x i32]
  AllocaInst* arrAlloc = module->addAllocaInst(arrType, "arr.addr");
  // %arr1 = getelementptr inbounds [10 x i32], [10 x i32]* %arr.addr, i32 0, i32 1
  GetElemPtrInst* gepInst =
      module->addGetElemPtrInst(arrAlloc, IntegerConstant::getConstInt(0),
                                IntegerConstant::getConstInt(1), "arr1");
  // store i32 %x, i32* %arr1
  StoreInst* arrStore = module->addStoreInst(argument, gepInst);
  // %arr1.val = load i32, i32* %arr1
  LoadInst* arrLoad = module->addLoadInst(gepInst, "arr1.val");
  // %icmp.ret1 = icmp sle i32 5, %arr1.val
  IcmpInst* icmp = module->addIcmpInst(SLE, IntegerConstant::getConstInt(5),
                                       arrLoad, "icmp.ret1");

  BasicBlock* ifThenBB = module->addBasicBlock(fFunction, "if.then");
  BasicBlock* ifEndBB = module->addBasicBlock(fFunction, "if.end");
  // br i1 %icmp.ret1, label %if.then, label %if.end
  BranchInst* br = module->addBranchInst(icmp, ifThenBB, ifEndBB);

  // In if.then block, let ret1 = 5
  module->setCurrBasicBlock(ifThenBB);
  // %ret1 = add i32 5, 0
  BinaryOpInst* ret1 =
      module->addBinaryOpInst(ADD, IntegerConstant::getConstInt(5),
                              IntegerConstant::getConstInt(0), "ret1");
  // br label %if.end
  JumpInst* jump = module->addJumpInst(ifEndBB);

  // In if.end block, let ret = phi [arrLoad entry], [ret1 if.then]
  module->setCurrBasicBlock(ifEndBB);
  // %ret = phi i32[ %arr1.val, %foo.entry ], [ %ret1, %if.then ]
  PhiInst* phi = module->addPhiInst("ret");
  phi->pushIncoming(arrLoad, bb1);
  phi->pushIncoming(ret1, ifThenBB);
  // %gv.val = load i32, i32* @gx
  LoadInst* gxLoad = module->addLoadInst(gv, "gv.val");
  // %ret2 = add i32 %ret, %gv.val
  BinaryOpInst* retAddInstr = module->addBinaryOpInst(ADD, phi, gxLoad, "ret2");
  // ret i32 %ret2
  ReturnInst* rInst = module->addReturnInst(retAddInstr);

  // main function, call foo() at the end
  FuncType* mFuncType = Type::getFuncType(Type::getInt32Type());
  // define dso_local i32 @main() {
  Function* mFunction = module->addFunction(mFuncType, "main");
  BasicBlock* basicBlock = module->addBasicBlock(mFunction, "entry");
  module->setCurrBasicBlock(basicBlock);
  // %a.addr = alloca i32
  AllocaInst* allocaInst =
      module->addAllocaInst(Type::getInt32Type(), "a.addr");

  IntegerConstant* intConstant = IntegerConstant::getConstInt(10);
  // store i32 10, i32* %a.addr  
  StoreInst* storeInst = module->addStoreInst(intConstant, allocaInst);
  // %a = load i32, i32* %a.addr
  LoadInst* LoadInst = module->addLoadInst(allocaInst, "a");
  // %addInstr = add i32 %a, 10
  BinaryOpInst* addInstr =
      module->addBinaryOpInst(ADD, LoadInst, intConstant, "addInstr");

  BasicBlock* basicBlock2 = module->addBasicBlock(mFunction, "L1");
  // br label %L1
  JumpInst* jumpInstr = module->addJumpInst(basicBlock2);
  // L1:
  module->setCurrBasicBlock(basicBlock2);
  // %icmp.ret = icmp eq i32 %a, 10
  IcmpInst* icmpInst =
      module->addIcmpInst(EQ, LoadInst, intConstant, "icmp.ret");

  BasicBlock* basicBlock3 = module->addBasicBlock(mFunction, "L2");
  // br i1 %icmp.ret, label %L2, label %L1
  BranchInst* brInstr =
      module->addBranchInst(icmpInst, basicBlock3, basicBlock2);
  // L2:
  module->setCurrBasicBlock(basicBlock3);
  // %fp10 = sitofp i32 10 to float
  SitofpInst* i2fInst = module->addSitofpInst(intConstant, "fp10");
  // %fcmp.ret = fcmp ole float %fp10, %fp10
  FcmpInst* fcmpInst = module->addFcmpInst(OLE, i2fInst, i2fInst, "fcmp.ret");
  // %ext.ret = zext i1 %fcmp.ret to i32
  ZextInst* zextInst =
      module->addZextInst(fcmpInst, Type::getInt32Type(), "ext.ret");

  vector<Value*> callArgs;
  callArgs.push_back(IntegerConstant::getConstInt(4));
  // %foo.ret = call i32 @foo(i32 4)
  CallInst* callInst = module->addCallInst(fFunction, callArgs, "foo.ret");
  // ret i32 %foo.ret
  ReturnInst* returnInst = module->addReturnInst(callInst);

  std::ofstream out_file;
  out_file.open("tests/test.ll");
  module->printIR(out_file);
}
```

