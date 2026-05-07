#include "Value.hh"
#include "Module.hh"
#include "Function.hh"
#include "GlobalVariable.hh"
#include "Constant.hh"
#include "BasicBlock.hh"
#include "Instruction.hh"
#include "User.hh"
#include "Type.hh"

std::string print_as_op(Value *v, bool print_ty);
std::string print_cmp_type(CmpOp op);
std::string print_fcmp_type(CmpOp op);