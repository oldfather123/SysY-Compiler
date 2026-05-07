#ifndef __IR_HPP__
#define __IR_HPP__

#include <iostream>

using namespace std;

// ir_type.hpp
class Type;
class IntegerType;
class ArrayType;
class PointerType;
class FunctionType;

// ir_value.hpp
class Value;
class ValUndef;
class Const;
class ConstInt;
class ConstFloat;
class ConstArray;
class ConstZero;
class GlobalVariable;

// ir_instruction.hpp
class Instruction;
class BinaryInst;
class ICmpInst;
class FCmpInst;
class AllocaInst;
class GetElementPtrInst;
class LoadInst;
class StoreInst;
class UnaryInst;
class CallInst;
class BranchInst;
class ReturnInst;
class PhiInst;

// ir_basic_block.hpp
class BasicBlock;

// ir_function.hpp
class Argument;
class Function;

// ir_module.hpp
class Module;

// ir_builder.hpp
class IRBuilder;

// ir_scope.hpp
class Scope;

// ir_generator.hpp
class IRGenerator;

#endif // __IR_HPP__