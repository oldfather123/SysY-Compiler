#include "../include/ast.h"
#include "../include/block.h"
#include "../include/llvm_instruction.h"
#include "../include/symtab.h"
#include <assert.h>
#include <math.h>
#include <optional>
#include <stdexcept>

std::ostream &operator<<(std::ostream &s, LLVMType type) {
  switch (type) {
  case I32:
    s << "i32";
    break;
  case FLOAT32:
    s << "float";
    break;
  case PTR:
    s << "ptr";
    break;
  case VOID_TYPE:
    s << "void";
    break;
  case I8:
    s << "i8";
    break;
  case I1:
    s << "i1";
    break;
  case I64:
    s << "i64";
    break;
  case DOUBLE:
    s << "double";
    break;
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, LLVMIROpcode type) {
  switch (type) {
  case LOAD:
    s << "load";
    break;
  case STORE:
    s << "store";
    break;
  case ADD:
    s << "add";
    break;
  case SUB:
    s << "sub";
    break;
  case ICMP:
    s << "icmp";
    break;
  case PHI:
    s << "phi";
    break;
  case ALLOCA:
    s << "alloca";
    break;
  case MUL_OP:
    s << "mul";
    break;
  case DIV_OP:
    s << "sdiv";
    break;
  case BR_COND:
    s << "br";
    break;
  case BR_UNCOND:
    s << "br";
    break;
  case FADD:
    s << "fadd";
    break;
  case FSUB:
    s << "fsub";
    break;
  case FMUL:
    s << "fmul";
    break;
  case FDIV:
    s << "fdiv";
    break;
  case FCMP:
    s << "fcmp";
    break;
  case MOD_OP:
    s << "srem";
    break;
  case BITXOR:
    s << "xor";
    break;
  case BITAND:
    s << "and";
    break;
  case BITOR:
    s << "or";
    break;
  case SHL:
    s << "shl";
    break;
  case ASHR:
    s << "ashr";
    break;
  case LSHR:
    s << "lshr";
    break;
  case TRUNC:
    s << "trunc";
    break;
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, IcmpCond type) {
  switch (type) {
  case eq:
    s << "eq";
    break;
  case ne:
    s << "ne";
    break;
  case ugt:
    s << "ugt";
    break;
  case uge:
    s << "uge";
    break;
  case ult:
    s << "ult";
    break;
  case ule:
    s << "ule";
    break;
  case sgt:
    s << "sgt";
    break;
  case sge:
    s << "sge";
    break;
  case slt:
    s << "slt";
    break;
  case sle:
    s << "sle";
    break;
  }
  return s;
}

std::ostream &operator<<(std::ostream &s, FcmpCond type) {
  switch (type) {
  case FALSE:
    s << "false";
    break;
  case OEQ:
    s << "oeq";
    break;
  case OGT:
    s << "ogt";
    break;
  case OGE:
    s << "oge";
    break;
  case OLT:
    s << "olt";
    break;
  case OLE:
    s << "ole";
    break;
  case ONE:
    s << "one";
    break;
  case ORD:
    s << "ord";
    break;
  case UEQ:
    s << "ueq";
    break;
  case UGT:
    s << "ugt";
    break;
  case UGE:
    s << "uge";
    break;
  case ULT:
    s << "ult";
    break;
  case ULE:
    s << "ule";
    break;
  case UNE:
    s << "une";
    break;
  case UNO:
    s << "uno";
    break;
  case TRUE:
    s << "true";
    break;
  }
  return s;
}

class IRgenTable {
public:
  Operand current_strptr = nullptr;
  std::map<int, VarAttribute> RegTable;
  std::map<int, int> FormalArrayTable;
  // std::map<std::string, int> name_to_reg;
  // IRgenTable() {}
  std::vector<std::map<std::string, int>> name_to_reg;
  std::map<std::string, int> name_to_value;
  // std::map<std::string, int> global_name_to_reg;  // 新增全局变量映射

  IRgenTable() {
    // Initialize with global scope
    name_to_reg.push_back({});
  }

  // Enter a new scope
  void enterScope() { name_to_reg.push_back({}); }

  // Exit the current scope
  void exitScope() {
    if (!name_to_reg.empty()) {
      name_to_reg.pop_back();
    }
  }

  // Look up a variable's register, searching from innermost to outermost scope
  // int lookupReg(const std::string& name) {
  //     for (auto it = name_to_reg.rbegin(); it != name_to_reg.rend(); ++it) {
  //         auto found = it->find(name);
  //         if (found != it->end()) {
  //             return found->second;
  //         }
  //     }
  //     return -1; // Not found
  // }
  int lookupReg(const std::string &name) {
    for (auto it = name_to_reg.rbegin(); it != name_to_reg.rend(); ++it) {
      auto found = it->find(name);
      if (found != it->end()) {
        return found->second;
      }
    }
    // 新增：检查全局变量
    // auto global_it = global_name_to_reg.find(name);
    // if (global_it != global_name_to_reg.end()) {
    //     return global_it->second;
    // }
    return -1; // Not found
  }

  // Insert a variable into the current scope
  void insertReg(const std::string &name, int reg) {
    if (!name_to_reg.empty()) {
      name_to_reg.back()[name] = reg;
    }
  }

  // 新增：插入全局变量
  // void insertGlobalReg(const std::string& name, int reg) {
  //     global_name_to_reg[name] = reg;
  // }
};

SymbolTable str_table;
static FuncDefInstruction function_now;
static BaseType function_returntype = BaseType::VOID;
// static int now_label = 0;
static int loop_start_label = -1;
static int loop_end_label = -1;
static std::map<int, LLVMType> RegLLVMTypeMap;

std::map<FuncDefInstruction, int> max_label_map{};
std::map<FuncDefInstruction, int> max_reg_map{};

int max_reg = -1;
// int max_label = -1;

IRgenTable irgen_table;
LLVMIR llvmIR;

extern std::map<std::string, int> function_name_to_maxreg;

std::map<BaseType, LLVMType> Type2LLvm = {{BaseType::INT, LLVMType::I32},
                                          {BaseType::FLOAT, LLVMType::FLOAT32},
                                          {BaseType::VOID, LLVMType::VOID_TYPE},
                                          {BaseType::STRING, LLVMType::PTR}};
RegOperand *GetNewRegOperand(int RegNo);

static std::unordered_map<int, RegOperand *> RegOperandMap;
static std::map<int, LabelOperand *> LabelOperandMap;
static std::map<std::string, GlobalOperand *> GlobalOperandMap;

RegOperand *GetNewRegOperand(int RegNo) {
  auto it = RegOperandMap.find(RegNo);
  if (it == RegOperandMap.end()) {
    auto R = new RegOperand(RegNo);
    RegOperandMap[RegNo] = R;
    return R;
  } else {
    return it->second;
  }
}

LabelOperand *GetNewLabelOperand(int LabelNo) {
  auto it = LabelOperandMap.find(LabelNo);
  if (it == LabelOperandMap.end()) {
    auto L = new LabelOperand(LabelNo);
    LabelOperandMap[LabelNo] = L;
    return L;
  } else {
    return it->second;
  }
}

GlobalOperand *GetNewGlobalOperand(std::string name) {
  auto it = GlobalOperandMap.find(name);
  if (it == GlobalOperandMap.end()) {
    auto G = new GlobalOperand(name);
    GlobalOperandMap[name] = G;
    return G;
  } else {
    return it->second;
  }
}

LLVMBlock IRgenerator::getCurrentBlock() {
  return llvmIR.GetBlock(function_now, now_label);
}

int IRgenerator::newReg() { return (++current_reg_counter); }

int IRgenerator::newLabel() { return (++max_label); }

bool IRgenerator::isGlobalScope() { return function_now == nullptr; }

long long Float_to_Byte(float f) {
  unsigned int int_representation;
  // Ensure that float and unsigned int are both 32 bits.
  static_assert(sizeof(float) == sizeof(unsigned int),
                "Float and unsigned int must be the same size for this "
                "conversion to be valid.");
  memcpy(&int_representation, &f, sizeof(float));
  return int_representation;
}

// long long Float_to_Byte(float f)
// {
//     float rawFloat = f;
//     unsigned long long rawFloatByte = *((int *)&rawFloat);
//     unsigned long long signBit = rawFloatByte >> 31;
//     unsigned long long expBits = (rawFloatByte >> 23) & ((1 << 8) - 1);
//     unsigned long long part1 = rawFloatByte & ((1 << 23) - 1);

//     unsigned long long out_signBit = signBit << 63;
//     unsigned long long out_sigBits = part1 << 29;
//     unsigned long long expBits_highestBit = (expBits & (1 << 7)) << 3;
//     unsigned long long expBits_lowerBit = (expBits & (1 << 7) - 1);
//     unsigned long long expBits_lowerBit_highestBit = expBits_lowerBit & (1 <<
//     6); unsigned long long expBits_lowerBit_ext =
//     (expBits_lowerBit_highestBit) | (expBits_lowerBit_highestBit << 1) |
//                                               (expBits_lowerBit_highestBit <<
//                                               2) |
//                                               (expBits_lowerBit_highestBit <<
//                                               3);
//     unsigned long long expBits_full = expBits_highestBit | expBits_lowerBit |
//     expBits_lowerBit_ext; unsigned long long out_expBits = expBits_full <<
//     52; unsigned long long out_rawFloatByte = out_signBit | out_expBits |
//     out_sigBits;
//     /*
//         Example: Float Value 114.514

//         llvm Double:
//             0                               ---1 bit    (sign bit)
//             1000 0000 101                   ---11 bits  (exp bits)
//             1100 1010 0000 1110 0101 011    ---23 bits  (part 1)
//             00000000000000000000000000000   ---29 bits  (part 2 All zero)

//         IEEE Float:
//             0                               ---1 bit    (sign bit)
//             1    0000 101                   ---8 bits   (exp bits)
//             1100 1010 0000 1110 0101 011    ---23 bits  (part 1)

//     */

//     return out_rawFloatByte;
// }

void recursive_print(std::ostream &s, LLVMType type, VarAttribute &v,
                     size_t dimDph, size_t beginPos, size_t endPos) {
  if (dimDph == 0) {
    int allzero = 1;
    if (v.type == BaseType::INT) {
      for (auto x : v.IntInitVals) {
        if (x != 0) {
          allzero = 0;
          break;
        }
      }
    } else {
      for (auto x : v.FloatInitVals) {
        if (x != 0) {
          allzero = 0;
          break;
        }
      }
    }
    if (allzero) {
      for (size_t dim : v.dims) {
        s << "[" << dim << "x ";
      }
      s << type << std::string(v.dims.size(), ']') << " zeroinitializer";
      return;
    }
  }
  if (beginPos == endPos) {
    if (type == I32) {
      s << type << " " << v.IntInitVals[beginPos];
    } else if (type == FLOAT32) {
      float rawFloat = v.FloatInitVals[beginPos];
      unsigned long long rawFloatByte;
      std::memcpy(&rawFloatByte, &rawFloat, sizeof(float));
      unsigned long long signBit = rawFloatByte >> 31;
      unsigned long long expBits = (rawFloatByte >> 23) & ((1 << 8) - 1);
      unsigned long long part1 = rawFloatByte & ((1 << 23) - 1);
      unsigned long long out_signBit = signBit << 63;
      unsigned long long out_sigBits = part1 << 29;
      unsigned long long expBits_highestBit = (expBits & (1 << 7)) << 3;
      unsigned long long expBits_lowerBit = (expBits & ((1 << 7) - 1));
      unsigned long long expBits_lowerBit_highestBit =
          expBits_lowerBit & (1 << 6);
      unsigned long long expBits_lowerBit_ext =
          (expBits_lowerBit_highestBit) | (expBits_lowerBit_highestBit << 1) |
          (expBits_lowerBit_highestBit << 2) |
          (expBits_lowerBit_highestBit << 3);
      unsigned long long expBits_full =
          expBits_highestBit | expBits_lowerBit | expBits_lowerBit_ext;
      unsigned long long out_expBits = expBits_full << 52;
      unsigned long long out_rawFloatByte =
          out_signBit | out_expBits | out_sigBits;
      s << type << " 0x" << std::hex << out_rawFloatByte;
      s << std::dec;
    }
    return;
  }
  for (size_t i = dimDph; i < v.dims.size(); i++) {
    s << "[" << v.dims[i] << " x ";
  }
  s << type << std::string(v.dims.size() - dimDph, ']') << " [";
  size_t step = 1;
  for (size_t i = dimDph + 1; i < v.dims.size(); i++) {
    step *= v.dims[i];
  }
  for (size_t i = 0; i < v.dims[dimDph]; i++) {
    recursive_print(s, type, v, dimDph + 1, beginPos + i * step,
                    beginPos + (i + 1) * step - 1);
    if (i != v.dims[dimDph] - 1)
      s << ",";
  }
  s << "]";
}

// void IRgenArithmeticI32ImmRight(LLVMBlock B, LLVMIROpcode opcode, int reg1,
// int val2, int result_reg) {
//     B->InsertInstruction(1, new ArithmeticInstruction(opcode, LLVMType::I32,
//     GetNewRegOperand(reg1),
//                                                       new
//                                                       ImmI32Operand(val2),
//                                                       GetNewRegOperand(result_reg)));
// }
void IRgenerator ::IRgenArithmeticI32ImmRight(LLVMBlock B, LLVMIROpcode opcode,
                                              int reg1, int val2,
                                              int result_reg) {
  // 确保左操作数是I32类型
  int conv_reg1 = IRgenerator::convertToType(B, reg1, I32);

  B->InsertInstruction(
      1, new ArithmeticInstruction(opcode, I32, GetNewRegOperand(conv_reg1),
                                   new ImmI32Operand(val2),
                                   GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = I32;
}

// void IRgenArithmeticI32(LLVMBlock B, LLVMIROpcode opcode,
//     int reg1, int reg2, int result_reg) {
// B->InsertInstruction(1, new ArithmeticInstruction(opcode,
//                                   LLVMType::I32,
//                                   GetNewRegOperand(reg1),
//                                   GetNewRegOperand(reg2),
//                                   GetNewRegOperand(result_reg)));
// RegLLVMTypeMap[result_reg] = LLVMType::I32;
// }

void IRgenerator ::IRgenArithmeticI32(LLVMBlock B, LLVMIROpcode opcode,
                                      int reg1, int reg2, int result_reg) {
  // 确保两个操作数都是I32类型
  int conv_reg1 = IRgenerator::convertToType(B, reg1, I32);
  int conv_reg2 = IRgenerator::convertToType(B, reg2, I32);

  B->InsertInstruction(
      1, new ArithmeticInstruction(opcode, I32, GetNewRegOperand(conv_reg1),
                                   GetNewRegOperand(conv_reg2),
                                   GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = I32;
}

// void IRgenArithmeticF32(LLVMBlock B, LLVMIROpcode opcode, int reg1, int reg2,
// int result_reg) {
//     B->InsertInstruction(1, new ArithmeticInstruction(opcode,
//     LLVMType::FLOAT32, GetNewRegOperand(reg1),
//                                                       GetNewRegOperand(reg2),
//                                                       GetNewRegOperand(result_reg)));
// }
void IRgenerator ::IRgenArithmeticF32(LLVMBlock B, LLVMIROpcode opcode,
                                      int reg1, int reg2, int result_reg) {
  // 确保两个操作数都是FLOAT32类型
  int conv_reg1 = IRgenerator::convertToType(B, reg1, FLOAT32);
  int conv_reg2 = IRgenerator::convertToType(B, reg2, FLOAT32);

  B->InsertInstruction(
      1, new ArithmeticInstruction(opcode, FLOAT32, GetNewRegOperand(conv_reg1),
                                   GetNewRegOperand(conv_reg2),
                                   GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = FLOAT32;
}

// void IRgenArithmeticI32ImmLeft(LLVMBlock B, LLVMIROpcode opcode, int val1,
// int reg2, int result_reg) {
//     B->InsertInstruction(1, new ArithmeticInstruction(opcode, LLVMType::I32,
//     new ImmI32Operand(val1),
//                                                       GetNewRegOperand(reg2),
//                                                       GetNewRegOperand(result_reg)));
// }
void IRgenerator ::IRgenArithmeticI32ImmLeft(LLVMBlock B, LLVMIROpcode opcode,
                                             int val1, int reg2,
                                             int result_reg) {
  // 确保右操作数是I32类型
  int conv_reg2 = IRgenerator::convertToType(B, reg2, I32);

  B->InsertInstruction(
      1, new ArithmeticInstruction(opcode, I32, new ImmI32Operand(val1),
                                   GetNewRegOperand(conv_reg2),
                                   GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = I32;
}

// void IRgenArithmeticF32ImmLeft(LLVMBlock B, LLVMIROpcode opcode, float val1,
// int reg2, int result_reg) {
//     B->InsertInstruction(1, new ArithmeticInstruction(opcode,
//     LLVMType::FLOAT32, new ImmF32Operand(val1),
//                                                       GetNewRegOperand(reg2),
//                                                       GetNewRegOperand(result_reg)));
// }
void IRgenerator ::IRgenArithmeticF32ImmLeft(LLVMBlock B, LLVMIROpcode opcode,
                                             float val1, int reg2,
                                             int result_reg) {
  // 确保右操作数是FLOAT32类型
  int conv_reg2 = IRgenerator::convertToType(B, reg2, FLOAT32);

  B->InsertInstruction(
      1, new ArithmeticInstruction(opcode, FLOAT32, new ImmF32Operand(val1),
                                   GetNewRegOperand(conv_reg2),
                                   GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = FLOAT32;
}

void IRgenArithmeticI32ImmAll(LLVMBlock B, LLVMIROpcode opcode, int val1,
                              int val2, int result_reg) {
  B->InsertInstruction(1, new ArithmeticInstruction(
                              opcode, LLVMType::I32, new ImmI32Operand(val1),
                              new ImmI32Operand(val2),
                              GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32ImmAll(LLVMBlock B, LLVMIROpcode opcode, float val1,
                              float val2, int result_reg) {
  B->InsertInstruction(1, new ArithmeticInstruction(
                              opcode, LLVMType::FLOAT32,
                              new ImmF32Operand(val1), new ImmF32Operand(val2),
                              GetNewRegOperand(result_reg)));
}

// void IRgenIcmp(LLVMBlock B, IcmpCond cmp_op, int reg1, int reg2, int
// result_reg) {
//     B->InsertInstruction(1, new IcmpInstruction(LLVMType::I32,
//                                                 GetNewRegOperand(reg1),
//                                                 new ImmI32Operand(reg2),
//                                                 cmp_op,
//                                                 GetNewRegOperand(result_reg)));
//     // 结果是 i1
//     RegLLVMTypeMap[result_reg] = LLVMType::I1;
// }
void IRgenerator ::IRgenIcmp(LLVMBlock B, IcmpCond cmp_op, int reg1, int reg2,
                             int result_reg) {
  // 确保两个操作数都是I32类型
  int conv_reg1 = convertToType(B, reg1, I32);
  int conv_reg2 = convertToType(B, reg2, I32);

  B->InsertInstruction(1,
                       new IcmpInstruction(I32, GetNewRegOperand(conv_reg1),
                                           GetNewRegOperand(conv_reg2), cmp_op,
                                           GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = I1;
}

void IRgenerator ::IRgenIcmpImmRight(LLVMBlock B, IcmpCond cmp_op, int reg1,
                                     int val2, int result_reg) {

  // 确保reg1是I32类型
  int conv_reg1 = convertToType(B, reg1, I32);

  B->InsertInstruction(1, new IcmpInstruction(I32, GetNewRegOperand(conv_reg1),
                                              new ImmI32Operand(val2), cmp_op,
                                              GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = I1;
}

// void IRgenFcmp(LLVMBlock B, FcmpCond cmp_op, int reg1, int reg2, int
// result_reg) {
//     B->InsertInstruction(1, new FcmpInstruction(LLVMType::FLOAT32,
//                                                 GetNewRegOperand(reg1),
//                                                 GetNewRegOperand(reg2),
//                                                 cmp_op,
//                                                 GetNewRegOperand(result_reg)));
//     // 结果也是 i1
//     RegLLVMTypeMap[result_reg] = LLVMType::I1;
// }

void IRgenerator::IRgenFcmp(LLVMBlock B, FcmpCond cmp_op, int reg1, int reg2,
                            int result_reg) {
  // 确保两个操作数都是FLOAT32类型
  int conv_reg1 = convertToType(B, reg1, FLOAT32);
  int conv_reg2 = convertToType(B, reg2, FLOAT32);

  // 生成浮点比较指令
  B->InsertInstruction(1,
                       new FcmpInstruction(FLOAT32, GetNewRegOperand(conv_reg1),
                                           GetNewRegOperand(conv_reg2), cmp_op,
                                           GetNewRegOperand(result_reg)));

  // 结果是i1类型
  RegLLVMTypeMap[result_reg] = I1;
}

// void IRgenIcmpImmRight(LLVMBlock B, IcmpCond cmp_op, int reg1, int val2, int
// result_reg) {
//     B->InsertInstruction(1, new IcmpInstruction(LLVMType::I32,
//     GetNewRegOperand(reg1), new ImmI32Operand(val2), cmp_op,
//                                                 GetNewRegOperand(result_reg)));
// }

// void IRgenFcmpImmRight(LLVMBlock B, FcmpCond cmp_op, int reg1, float val2,
// int result_reg) {
//     B->InsertInstruction(1, new FcmpInstruction(LLVMType::FLOAT32,
//     GetNewRegOperand(reg1), new ImmF32Operand(val2),
//                                                 cmp_op,
//                                                 GetNewRegOperand(result_reg)));
// }
void IRgenerator::IRgenFcmpImmRight(LLVMBlock B, FcmpCond cmp_op, int reg1,
                                    float val2, int result_reg) {
  // 确保reg1是FLOAT32类型
  int conv_reg1 = convertToType(B, reg1, FLOAT32);

  // 生成浮点比较指令
  B->InsertInstruction(1,
                       new FcmpInstruction(FLOAT32, GetNewRegOperand(conv_reg1),
                                           new ImmF32Operand(val2), cmp_op,
                                           GetNewRegOperand(result_reg)));

  // 结果是i1类型
  RegLLVMTypeMap[result_reg] = I1;
}

void IRgenFptosi(LLVMBlock B, int src, int dst) {
  B->InsertInstruction(
      1, new FptosiInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
  RegLLVMTypeMap[dst] = I32; // 确保目标寄存器类型为I32
}

void IRgenFpext(LLVMBlock B, int src, int dst) {
  B->InsertInstruction(
      1, new FpextInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
  RegLLVMTypeMap[dst] = FLOAT32; // 确保目标寄存器类型为FLOAT32
}

void IRgenSitofp(LLVMBlock B, int src, int dst) {
  B->InsertInstruction(
      1, new SitofpInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
  RegLLVMTypeMap[dst] = FLOAT32; // 确保目标寄存器类型为FLOAT32
}

void IRgenZextI1toI32(LLVMBlock B, int src, int dst) {
  B->InsertInstruction(
      1, new ZextInstruction(LLVMType::I32, GetNewRegOperand(dst), LLVMType::I1,
                             GetNewRegOperand(src)));
  RegLLVMTypeMap[dst] = I32; // 确保目标寄存器类型为I32
}

void IRgenerator::IRgenLoad(LLVMBlock B, LLVMType type, int result_reg,
                            Operand ptr) {
  // 确保 ptr 是指针类型
  if (auto reg_op = dynamic_cast<RegOperand *>(ptr)) {
    int ptr_reg = reg_op->GetRegNo();
    if (RegLLVMTypeMap.find(ptr_reg) != RegLLVMTypeMap.end() &&
        RegLLVMTypeMap[ptr_reg] != PTR) {
      int temp_reg = convertToType(B, ptr_reg, PTR);
      ptr = GetNewRegOperand(temp_reg); // 使用新分配的内存地址
    }
  }
  B->InsertInstruction(
      1, new LoadInstruction(type, ptr, GetNewRegOperand(result_reg)));
  RegLLVMTypeMap[result_reg] = type;
}

void IRgenerator::IRgenGetElementptr(LLVMBlock B, LLVMType type, int result_reg,
                                     Operand ptr, std::vector<int> dims,
                                     std::vector<Operand> indices) {
  // 确保所有索引操作数是整数类型
  for (auto &index : indices) {
    if (auto reg_op = dynamic_cast<RegOperand *>(index)) {
      int reg = reg_op->GetRegNo();
      if (RegLLVMTypeMap[reg] == PTR) {
        // 如果是指针类型，加载其值
        int value_reg = newReg();
        IRgenLoad(B, I32, value_reg, index);
        index = GetNewRegOperand(value_reg);
        RegLLVMTypeMap[value_reg] = I32;
      }
    }
  }
  B->InsertInstruction(
      1, new GetElementptrInstruction(type, GetNewRegOperand(result_reg), ptr,
                                      dims, indices));
  RegLLVMTypeMap[result_reg] = PTR;
}

void IRgenerator::IRgenStore(LLVMBlock B, LLVMType type, int value_reg,
                             Operand ptr) {
  int conv_value_reg = convertToType(B, value_reg, type);
  value_reg = conv_value_reg; // 更新 value_reg 为转换后的寄存器
  // 确保 ptr 是指针类型
  if (auto reg_op = dynamic_cast<RegOperand *>(ptr)) {
    int ptr_reg = reg_op->GetRegNo();
    if (RegLLVMTypeMap.find(ptr_reg) != RegLLVMTypeMap.end() &&
        RegLLVMTypeMap[ptr_reg] != PTR) {
      // 非指针类型，转换为指针
      int temp_reg = newReg();
      IRgenAlloca(type, temp_reg);      // 分配内存
      ptr = GetNewRegOperand(temp_reg); // 使用新分配的内存地址
    }
  }
  B->InsertInstruction(
      1, new StoreInstruction(type, ptr, GetNewRegOperand(value_reg)));
}

void IRgenerator::IRgenStore(LLVMBlock B, LLVMType type, Operand value,
                             Operand ptr) {
  // 确保 ptr 是指针类型
  if (auto reg_op = dynamic_cast<RegOperand *>(ptr)) {
    int ptr_reg = reg_op->GetRegNo();
    if (RegLLVMTypeMap.find(ptr_reg) != RegLLVMTypeMap.end() &&
        RegLLVMTypeMap[ptr_reg] != PTR) {
      // 非指针类型，转换为指针
      int temp_reg = newReg();
      IRgenAlloca(type, temp_reg);      // 分配内存
      ptr = GetNewRegOperand(temp_reg); // 使用新分配的内存地址
    }
  }
  B->InsertInstruction(1, new StoreInstruction(type, ptr, value));
}

void IRgenCall(LLVMBlock B, LLVMType type, int result_reg,
               std::vector<std::pair<LLVMType, Operand>> args,
               std::string name) {
  B->InsertInstruction(
      1, new CallInstruction(type, GetNewRegOperand(result_reg), name, args));
  if (result_reg >= 0)
    RegLLVMTypeMap[result_reg] = type;
}

void IRgenCallVoid(LLVMBlock B, LLVMType type,
                   std::vector<std::pair<enum LLVMType, Operand>> args,
                   std::string name) {
  B->InsertInstruction(
      1, new CallInstruction(type, GetNewRegOperand(-1), name, args));
}

void IRgenCallNoArgs(LLVMBlock B, LLVMType type, int result_reg,
                     std::string name) {
  B->InsertInstruction(
      1, new CallInstruction(type, GetNewRegOperand(result_reg), name));
}

void IRgenCallVoidNoArgs(LLVMBlock B, LLVMType type, std::string name) {
  B->InsertInstruction(1,
                       new CallInstruction(type, GetNewRegOperand(-1), name));
}

void IRgenRetReg(LLVMBlock B, LLVMType type, int reg) {
  B->InsertInstruction(1, new RetInstruction(type, GetNewRegOperand(reg)));
}

void IRgenRetImmInt(LLVMBlock B, LLVMType type, int val) {
  B->InsertInstruction(1, new RetInstruction(type, new ImmI32Operand(val)));
}

void IRgenRetImmFloat(LLVMBlock B, LLVMType type, float val) {
  B->InsertInstruction(1, new RetInstruction(type, new ImmF32Operand(val)));
}

void IRgenRetVoid(LLVMBlock B) {
  B->InsertInstruction(1, new RetInstruction(LLVMType::VOID_TYPE, nullptr));
}

void IRgenerator::IRgenBRUnCond(LLVMBlock B, int dst_label) {
  // 确保目标标签存在
  if (dst_label < 0) {
    throw std::runtime_error(
        "Invalid destination label for unconditional branch");
  }

  // 获取标签操作数
  LabelOperand *label_op = GetNewLabelOperand(dst_label);

  // 生成无条件分支指令
  B->InsertInstruction(1, new BrUncondInstruction(label_op));
}

void IRgenerator::IRgenBrCond(LLVMBlock B, int cond_reg, int true_label,
                              int false_label) {
  // 确保条件寄存器有效
  if (cond_reg < 0) {
    throw std::runtime_error(
        "Invalid condition register for conditional branch");
  }

  // 确保标签有效
  if (true_label < 0 || false_label < 0) {
    throw std::runtime_error("Invalid labels for conditional branch");
  }

  // 确保条件寄存器是布尔类型 (i1)
  int conv_cond_reg = convertToType(B, cond_reg, I1);

  // 获取标签操作数
  LabelOperand *true_label_op = GetNewLabelOperand(true_label);
  LabelOperand *false_label_op = GetNewLabelOperand(false_label);

  // 生成条件分支指令
  B->InsertInstruction(1, new BrCondInstruction(GetNewRegOperand(conv_cond_reg),
                                                true_label_op, false_label_op));
}

void IRgenerator::IRgenAlloca(LLVMType type, int reg) {
  if (function_now == nullptr) {
    throw std::runtime_error("Cannot allocate local variable in global scope");
  }
  // 获取当前函数的入口块
  auto entry_block_it = llvmIR.function_block_map[function_now].begin();
  if (entry_block_it == llvmIR.function_block_map[function_now].end()) {
    throw std::runtime_error("No blocks in function");
  }
  LLVMBlock entry_block = entry_block_it->second;
  // 在入口块的开头插入 alloca 指令
  entry_block->InsertInstruction(
      0, new AllocaInstruction(type, GetNewRegOperand(reg)));
  RegLLVMTypeMap[reg] = LLVMType::PTR; // 设置指针类型
}

void IRgenAllocaArray(LLVMBlock B, LLVMType type, int reg,
                      std::vector<int> dims) {
  B->InsertInstruction(
      0, new AllocaInstruction(type, dims, GetNewRegOperand(reg)));
  RegLLVMTypeMap[reg] = PTR; // 添加类型记录
}

void BasicBlock::InsertInstruction(int pos, Instruction Ins) {
  assert(pos == 0 || pos == 1);
  if (pos == 0) {
    Instruction_list.push_front(Ins);
  } else if (pos == 1) {
    Instruction_list.push_back(Ins);
  }
}

int IRgenerator::convertToType(LLVMBlock B, int src_reg, LLVMType target_type) {
  LLVMType src_type = RegLLVMTypeMap[src_reg];
  if (src_type == target_type) {
    return src_reg; // 类型相同无需转换
  }

  int result_reg = newReg();

  // 指针转整数：通过加载指针指向的值
  if (src_type == PTR && target_type == I32) {
    int value_reg = newReg();
    IRgenLoad(B, I32, value_reg, GetNewRegOperand(src_reg));
    RegLLVMTypeMap[value_reg] = I32; // 记录新寄存器类型
    return value_reg;
  }

  // 整数转指针：分配内存并存储值
  if (src_type == I32 && target_type == PTR) {
    int temp_reg = newReg();
    IRgenAlloca(I32, temp_reg);
    RegLLVMTypeMap[temp_reg] = PTR; // 记录新寄存器类型
    IRgenStore(B, I32, GetNewRegOperand(src_reg), GetNewRegOperand(temp_reg));
    return temp_reg;
  }

  // 指针转布尔：加载值后与0比较
  if (src_type == PTR && target_type == I1) {
    int value_reg = newReg();
    IRgenLoad(B, I32, value_reg, GetNewRegOperand(src_reg));
    RegLLVMTypeMap[value_reg] = I32; // 记录新寄存器类型

    int cmp_reg = newReg();
    // IRgenIcmp(B, IcmpCond::ne, value_reg, 0, cmp_reg);
    IRgenIcmpImmRight(B, IcmpCond::ne, value_reg, 0, cmp_reg);
    RegLLVMTypeMap[cmp_reg] = I1; // 记录新寄存器类型
    return cmp_reg;
  }

  // 布尔转指针：分配内存并存储值
  if (src_type == I1 && target_type == PTR) {
    int temp_reg = newReg();
    IRgenAlloca(I1, temp_reg);
    RegLLVMTypeMap[temp_reg] = PTR; // 记录新寄存器类型
    IRgenStore(B, I1, GetNewRegOperand(src_reg), GetNewRegOperand(temp_reg));
    return temp_reg;
  }

  // 指针转浮点：先加载为整数再转浮点
  if (src_type == PTR && target_type == FLOAT32) {
    int int_reg = newReg();
    IRgenLoad(B, I32, int_reg, GetNewRegOperand(src_reg));
    RegLLVMTypeMap[int_reg] = I32; // 记录新寄存器类型

    int float_reg = newReg();
    IRgenSitofp(B, int_reg, float_reg);
    RegLLVMTypeMap[float_reg] = FLOAT32; // 记录新寄存器类型
    return float_reg;
  }

  // 浮点转指针：先转整数再存储
  if (src_type == FLOAT32 && target_type == PTR) {
    int int_reg = newReg();
    IRgenFptosi(B, src_reg, int_reg);
    RegLLVMTypeMap[int_reg] = I32; // 记录新寄存器类型

    int temp_reg = newReg();
    IRgenAlloca(I32, temp_reg);
    RegLLVMTypeMap[temp_reg] = PTR; // 记录新寄存器类型
    IRgenStore(B, I32, GetNewRegOperand(int_reg), GetNewRegOperand(temp_reg));
    return temp_reg;
  }

  // 其他类型转换保持不变
  if (src_type == I1 && target_type == I32) {
    IRgenZextI1toI32(B, src_reg, result_reg);
    RegLLVMTypeMap[result_reg] = I32; // 记录新寄存器类型
  } else if (src_type == I32 && target_type == I1) {
    // IRgenIcmp(B, IcmpCond::ne, src_reg, 0, result_reg);
    IRgenIcmpImmRight(B, IcmpCond::ne, src_reg, 0, result_reg);
    RegLLVMTypeMap[result_reg] = I1; // 记录新寄存器类型
  } else if (src_type == FLOAT32 && target_type == I32) {
    IRgenFptosi(B, src_reg, result_reg);
    RegLLVMTypeMap[result_reg] = I32; // 记录新寄存器类型
  } else if (src_type == I32 && target_type == FLOAT32) {
    IRgenSitofp(B, src_reg, result_reg);
    RegLLVMTypeMap[result_reg] = FLOAT32; // 记录新寄存器类型
  } else if (src_type == FLOAT32 && target_type == I1) {
    IRgenFcmpImmRight(B, FcmpCond::OEQ, src_reg, 0.0f, result_reg);
    RegLLVMTypeMap[result_reg] = I1; // 记录新寄存器类型
  } else if (src_type == I1 && target_type == FLOAT32) {
    int temp_reg = newReg();
    IRgenZextI1toI32(B, src_reg, temp_reg);
    IRgenSitofp(B, temp_reg, result_reg);
    RegLLVMTypeMap[result_reg] = FLOAT32; // 记录新寄存器类型
  } else {
    result_reg = src_reg; // 默认不转换
  }

  return result_reg;
}

bool IsBr(Instruction ins) {
  int opcode = ins->GetOpcode();
  return opcode == BR_COND || opcode == BR_UNCOND;
}

bool IsRet(Instruction ins) {
  int opcode = ins->GetOpcode();
  return opcode == RET;
}

void AddNoReturnBlock() {
  for (auto block : llvmIR.function_block_map[function_now]) {
    LLVMBlock B = block.second;
    if (B->Instruction_list.empty() || (!IsRet(B->Instruction_list.back()) &&
                                        !IsBr(B->Instruction_list.back()))) {
      if (function_returntype == BaseType::VOID) {
        IRgenRetVoid(B);
      } else if (function_returntype == BaseType::INT) {
        IRgenRetImmInt(B, LLVMType::I32, 0);
      } else if (function_returntype == BaseType::FLOAT) {
        IRgenRetImmFloat(B, LLVMType::FLOAT32, 0);
      }
    }
  }
}

void IRgenerator ::AddLibFunctionDeclare() {
  FunctionDeclareInstruction *getint =
      new FunctionDeclareInstruction(I32, "getint");
  llvmIR.function_declare.push_back(getint);

  FunctionDeclareInstruction *getchar =
      new FunctionDeclareInstruction(I32, "getch");
  llvmIR.function_declare.push_back(getchar);

  FunctionDeclareInstruction *getfloat =
      new FunctionDeclareInstruction(FLOAT32, "getfloat");
  llvmIR.function_declare.push_back(getfloat);

  FunctionDeclareInstruction *getarray =
      new FunctionDeclareInstruction(I32, "getarray");
  getarray->InsertFormal(PTR);
  llvmIR.function_declare.push_back(getarray);

  FunctionDeclareInstruction *getfloatarray =
      new FunctionDeclareInstruction(I32, "getfarray");
  getfloatarray->InsertFormal(PTR);
  llvmIR.function_declare.push_back(getfloatarray);

  FunctionDeclareInstruction *putint =
      new FunctionDeclareInstruction(VOID_TYPE, "putint");
  putint->InsertFormal(I32);
  llvmIR.function_declare.push_back(putint);

  FunctionDeclareInstruction *putch =
      new FunctionDeclareInstruction(VOID_TYPE, "putch");
  putch->InsertFormal(I32);
  llvmIR.function_declare.push_back(putch);

  FunctionDeclareInstruction *putfloat =
      new FunctionDeclareInstruction(VOID_TYPE, "putfloat");
  putfloat->InsertFormal(FLOAT32);
  llvmIR.function_declare.push_back(putfloat);

  FunctionDeclareInstruction *putarray =
      new FunctionDeclareInstruction(VOID_TYPE, "putarray");
  putarray->InsertFormal(I32);
  putarray->InsertFormal(PTR);
  llvmIR.function_declare.push_back(putarray);

  FunctionDeclareInstruction *putfarray =
      new FunctionDeclareInstruction(VOID_TYPE, "putfarray");
  putfarray->InsertFormal(I32);
  putfarray->InsertFormal(PTR);
  llvmIR.function_declare.push_back(putfarray);

  FunctionDeclareInstruction *putf =
      new FunctionDeclareInstruction(VOID_TYPE, "putf");
  putf->InsertFormal(PTR);
  llvmIR.function_declare.push_back(putf);

  FunctionDeclareInstruction *starttime =
      new FunctionDeclareInstruction(VOID_TYPE, "_sysy_starttime");
  starttime->InsertFormal(I32);
  llvmIR.function_declare.push_back(starttime);

  FunctionDeclareInstruction *stoptime =
      new FunctionDeclareInstruction(VOID_TYPE, "_sysy_stoptime");
  stoptime->InsertFormal(I32);
  llvmIR.function_declare.push_back(stoptime);

  FunctionDeclareInstruction *llvm_memset =
      new FunctionDeclareInstruction(VOID_TYPE, "llvm.memset.p0.i32");
  llvm_memset->InsertFormal(PTR);
  llvm_memset->InsertFormal(I8);
  llvm_memset->InsertFormal(I32);
  llvm_memset->InsertFormal(I1);
  llvmIR.function_declare.push_back(llvm_memset);

  FunctionDeclareInstruction *llvm_ll_add_mod =
      new FunctionDeclareInstruction(VOID_TYPE, "___llvm_ll_add_mod");
  llvm_ll_add_mod->InsertFormal(I32);
  llvm_ll_add_mod->InsertFormal(I32);
  llvm_ll_add_mod->InsertFormal(I32);
  llvmIR.function_declare.push_back(llvm_ll_add_mod);

  FunctionDeclareInstruction *llvm_umax =
      new FunctionDeclareInstruction(I32, "llvm.umax.i32");
  llvm_umax->InsertFormal(I32);
  llvm_umax->InsertFormal(I32);
  llvmIR.function_declare.push_back(llvm_umax);

  FunctionDeclareInstruction *llvm_umin =
      new FunctionDeclareInstruction(I32, "llvm.umin.i32");
  llvm_umin->InsertFormal(I32);
  llvm_umin->InsertFormal(I32);
  llvmIR.function_declare.push_back(llvm_umin);

  FunctionDeclareInstruction *llvm_smax =
      new FunctionDeclareInstruction(I32, "llvm.smax.i32");
  llvm_smax->InsertFormal(I32);
  llvm_smax->InsertFormal(I32);
  llvmIR.function_declare.push_back(llvm_smax);

  FunctionDeclareInstruction *llvm_smin =
      new FunctionDeclareInstruction(I32, "llvm.smin.i32");
  llvm_smin->InsertFormal(I32);
  llvm_smin->InsertFormal(I32);
  llvmIR.function_declare.push_back(llvm_smin);

  FunctionDeclareInstruction *llvm_fmin =
      new FunctionDeclareInstruction(FLOAT32, "___llvm_fmin_f32");
  llvm_fmin->InsertFormal(FLOAT32);
  llvm_fmin->InsertFormal(FLOAT32);
  llvmIR.function_declare.push_back(llvm_fmin);

  FunctionDeclareInstruction *llvm_fmax =
      new FunctionDeclareInstruction(FLOAT32, "___llvm_fmax_f32");
  llvm_fmax->InsertFormal(FLOAT32);
  llvm_fmax->InsertFormal(FLOAT32);
  llvmIR.function_declare.push_back(llvm_fmax);
}

void IRgenerator::handleArrayInitializer(InitVal *init, int base_reg,
                                         VarAttribute &attr,
                                         const std::vector<int> &dims,
                                         size_t dim_idx,
                                         size_t &current_index) // 当前索引位置
{
  if (!init) {
    return;
  }
  LLVMBlock B = getCurrentBlock();
  LLVMType type = Type2LLvm.at(attr.type);

  // 单个值初始化
  if (std::holds_alternative<std::unique_ptr<Exp>>(init->value)) {
    require_address = false;
    std::get<std::unique_ptr<Exp>>(init->value)->accept(*this);
    int init_reg = max_reg;

    // 计算当前元素的线性位置
    size_t pos = current_index;
    std::vector<Operand> indices;
    // indices.push_back(new ImmI32Operand(0)); // 数组基址

    // 转换为多维索引
    size_t temp = pos;
    for (size_t i = 0; i < dims.size(); i++) {
      size_t stride = 1;
      for (size_t j = i + 1; j < dims.size(); j++) {
        stride *= dims[j];
      }
      size_t idx = temp / stride;
      temp %= stride;
      indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
    }

    // 生成GEP指令
    int ptr_reg = newReg();
    IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                       indices);

    // 存储值
    if (attr.type == BaseType::INT) {
      int value = 0;
      if (irgen_table.RegTable[init_reg].IntInitVals.size() > 0) {
        value = irgen_table.RegTable[init_reg].IntInitVals[0];
        IRgenStore(B, type, new ImmI32Operand(value),
                   GetNewRegOperand(ptr_reg));
      } else {
        int temp_reg = convertToType(B, init_reg, I32);
        // 如果没有初始化值，使用寄存器的值
        init_reg = temp_reg; // 更新 init_reg 为转换后的寄存器
        IRgenStore(B, type, GetNewRegOperand(init_reg),
                   GetNewRegOperand(ptr_reg));
      }
      // IRgenStore(B, type, new ImmI32Operand(value),
      // GetNewRegOperand(ptr_reg));
    } else {
      float value = 0.0f;
      if (irgen_table.RegTable[init_reg].FloatInitVals.size() > 0) {
        value = irgen_table.RegTable[init_reg].FloatInitVals[0];
        IRgenStore(B, type, new ImmF32Operand(value),
                   GetNewRegOperand(ptr_reg));
      } else {
        int temp_reg = convertToType(B, init_reg, FLOAT32);
        // 如果没有初始化值，使用寄存器的值
        init_reg = temp_reg; // 更新 init_reg 为转换后的寄存器
        IRgenStore(B, type, GetNewRegOperand(init_reg),
                   GetNewRegOperand(ptr_reg));
      }
      // IRgenStore(B, type, new ImmF32Operand(value),
      // GetNewRegOperand(ptr_reg));
    }
    current_index++; // 移动到下一个位置
  } else {
    // 处理初始化列表
    auto &list = std::get<std::vector<std::unique_ptr<InitVal>>>(init->value);
    if (list.size() == 0 && current_index == 0 && dim_idx == 0) {
      int whole_size = 1;
      for (int dim : dims) {
        whole_size *= dim;
      }
      for (int j = 0; j < whole_size; j++) {
        size_t pos = current_index;
        std::vector<Operand> indices;
        // indices.push_back(new ImmI32Operand(0));

        // 转换为多维索引
        size_t temp = pos;
        for (size_t i = 0; i < dims.size(); i++) {
          size_t stride = 1;
          for (size_t j = i + 1; j < dims.size(); j++) {
            stride *= dims[j];
          }
          size_t idx = temp / stride;
          temp %= stride;
          indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
        }

        int ptr_reg = newReg();
        IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                           indices);

        if (attr.type == BaseType::INT) {
          IRgenStore(B, type, new ImmI32Operand(0), GetNewRegOperand(ptr_reg));
        } else {
          IRgenStore(B, type, new ImmF32Operand(0.0f),
                     GetNewRegOperand(ptr_reg));
        }
        current_index++;
      }
      return; // 结束处理
    }

    // if (dim_idx < dims.size()) {
    //  当前维度处理
    // for (size_t i = 0; i < static_cast<size_t>(dims[dim_idx]); i++) {
    for (size_t i = 0; i < list.size(); i++) {
      // 递归处理子初始化器
      handleArrayInitializer(list[i].get(), base_reg, attr, dims, dim_idx + 1,
                             current_index);
      //}
      // else {
      if (!std::holds_alternative<std::unique_ptr<Exp>>(list[i].get()->value)) {
        auto &listtemp = std::get<std::vector<std::unique_ptr<InitVal>>>(
            list[i].get()->value);
        int missnum = listtemp.size() % dims[dims.size() - 1];
        if (missnum == 0 && listtemp.size() != 0) {
          continue;
        }
        for (int j = 0; j < dims[dims.size() - 1] - missnum; j++) {
          // 没有足够初始化值，填充0
          size_t pos = current_index;
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0));

          // 转换为多维索引
          size_t temp = pos;
          for (size_t j = 0; j < dims.size(); j++) {
            size_t stride = 1;
            for (size_t k = j + 1; k < dims.size(); k++) {
              stride *= dims[k];
            }
            size_t idx = temp / stride;
            temp %= stride;
            indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
          }

          int ptr_reg = newReg();
          IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                             indices);

          if (attr.type == BaseType::INT) {
            IRgenStore(B, type, new ImmI32Operand(0),
                       GetNewRegOperand(ptr_reg));
          } else {
            IRgenStore(B, type, new ImmF32Operand(0.0f),
                       GetNewRegOperand(ptr_reg));
          }
          current_index++;
        }
      }
    }
    //}
    //}
  }
}
void IRgenerator::handleArrayInitializer(ConstInitVal *init, int base_reg,
                                         VarAttribute &attr,
                                         const std::vector<int> &dims,
                                         size_t dim_idx,
                                         size_t &current_index) // 当前索引位置
{
  if (!init) {
    return;
  }
  LLVMBlock B = getCurrentBlock();
  LLVMType type = Type2LLvm.at(attr.type);

  // 单个值初始化
  if (std::holds_alternative<std::unique_ptr<Exp>>(init->value)) {
    require_address = false;
    std::get<std::unique_ptr<Exp>>(init->value)->accept(*this);
    int init_reg = max_reg;

    // 计算当前元素的线性位置
    size_t pos = current_index;
    std::vector<Operand> indices;
    // indices.push_back(new ImmI32Operand(0)); // 数组基址

    // 转换为多维索引
    size_t temp = pos;
    for (size_t i = 0; i < dims.size(); i++) {
      size_t stride = 1;
      for (size_t j = i + 1; j < dims.size(); j++) {
        stride *= dims[j];
      }
      size_t idx = temp / stride;
      temp %= stride;
      indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
    }

    // 生成GEP指令
    int ptr_reg = newReg();
    IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                       indices);

    // 存储值
    if (attr.type == BaseType::INT) {
      int value = 0;
      if (irgen_table.RegTable[init_reg].IntInitVals.size() > 0) {
        value = irgen_table.RegTable[init_reg].IntInitVals[0];
        IRgenStore(B, type, new ImmI32Operand(value),
                   GetNewRegOperand(ptr_reg));
      } else {
        int temp_reg = convertToType(B, init_reg, I32);
        // 如果没有初始化值，使用寄存器的值
        init_reg = temp_reg; // 更新 init_reg 为转换后的寄存器
        IRgenStore(B, type, GetNewRegOperand(init_reg),
                   GetNewRegOperand(ptr_reg));
      }
      // IRgenStore(B, type, new ImmI32Operand(value),
      // GetNewRegOperand(ptr_reg));
    } else {
      float value = 0.0f;
      if (irgen_table.RegTable[init_reg].FloatInitVals.size() > 0) {
        value = irgen_table.RegTable[init_reg].FloatInitVals[0];
        IRgenStore(B, type, new ImmF32Operand(value),
                   GetNewRegOperand(ptr_reg));
      } else {
        int temp_reg = convertToType(B, init_reg, FLOAT32);
        // 如果没有初始化值，使用寄存器的值
        init_reg = temp_reg; // 更新 init_reg 为转换后的寄存器
        IRgenStore(B, type, GetNewRegOperand(init_reg),
                   GetNewRegOperand(ptr_reg));
      }
      // IRgenStore(B, type, new ImmF32Operand(value),
      // GetNewRegOperand(ptr_reg));
    }
    current_index++; // 移动到下一个位置
  } else {
    // 处理初始化列表
    auto &list =
        std::get<std::vector<std::unique_ptr<ConstInitVal>>>(init->value);
    if (list.size() == 0 && current_index == 0 && dim_idx == 0) {
      int whole_size = 1;
      for (int dim : dims) {
        whole_size *= dim;
      }
      for (int j = 0; j < whole_size; j++) {
        size_t pos = current_index;
        std::vector<Operand> indices;
        // indices.push_back(new ImmI32Operand(0));

        // 转换为多维索引
        size_t temp = pos;
        for (size_t i = 0; i < dims.size(); i++) {
          size_t stride = 1;
          for (size_t j = i + 1; j < dims.size(); j++) {
            stride *= dims[j];
          }
          size_t idx = temp / stride;
          temp %= stride;
          indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
        }

        int ptr_reg = newReg();
        IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                           indices);

        if (attr.type == BaseType::INT) {
          IRgenStore(B, type, new ImmI32Operand(0), GetNewRegOperand(ptr_reg));
        } else {
          IRgenStore(B, type, new ImmF32Operand(0.0f),
                     GetNewRegOperand(ptr_reg));
        }
        current_index++;
      }
      return; // 结束处理
    }

    // if (dim_idx < dims.size()) {
    //  当前维度处理
    // for (size_t i = 0; i < static_cast<size_t>(dims[dim_idx]); i++) {
    for (size_t i = 0; i < list.size(); i++) {
      // 递归处理子初始化器
      handleArrayInitializer(list[i].get(), base_reg, attr, dims, dim_idx + 1,
                             current_index);
      //}
      // else {
      if (!std::holds_alternative<std::unique_ptr<Exp>>(list[i].get()->value)) {
        auto &listtemp = std::get<std::vector<std::unique_ptr<ConstInitVal>>>(
            list[i].get()->value);
        int missnum = listtemp.size() % dims[dims.size() - 1];
        if (missnum == 0 && listtemp.size() != 0) {
          continue;
        }
        for (int j = 0; j < dims[dims.size() - 1] - missnum; j++) {
          // 没有足够初始化值，填充0
          size_t pos = current_index;
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0));

          // 转换为多维索引
          size_t temp = pos;
          for (size_t j = 0; j < dims.size(); j++) {
            size_t stride = 1;
            for (size_t k = j + 1; k < dims.size(); k++) {
              stride *= dims[k];
            }
            size_t idx = temp / stride;
            temp %= stride;
            indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
          }

          int ptr_reg = newReg();
          IRgenGetElementptr(B, type, ptr_reg, GetNewRegOperand(base_reg), dims,
                             indices);

          if (attr.type == BaseType::INT) {
            IRgenStore(B, type, new ImmI32Operand(0),
                       GetNewRegOperand(ptr_reg));
          } else {
            IRgenStore(B, type, new ImmF32Operand(0.0f),
                       GetNewRegOperand(ptr_reg));
          }
          current_index++;
        }
      }
    }
    //}
    //}
  }
}

// New helper function to evaluate constant expressions
std::optional<int> IRgenerator::evaluateConstExpression(Exp *expr, int isdef) {
  if (auto *num = dynamic_cast<Number *>(expr)) {
    if (std::holds_alternative<int>(num->value)) {
      return std::get<int>(num->value);
    }
  } else if (auto *unary = dynamic_cast<UnaryExp *>(expr)) {
    auto operand_val = evaluateConstExpression(unary->operand.get(), isdef);
    if (operand_val) {
      switch (unary->op) {
      case UnaryOp::PLUS:
        return *operand_val;
      case UnaryOp::MINUS:
        return -*operand_val;
      case UnaryOp::NOT:
        return !*operand_val;
      }
    }
  } else if (auto *binary = dynamic_cast<BinaryExp *>(expr)) {
    auto lhs_val = evaluateConstExpression(binary->lhs.get(), isdef);
    auto rhs_val = evaluateConstExpression(binary->rhs.get(), isdef);
    if (lhs_val && rhs_val) {
      switch (binary->op) {
      case BinaryOp::ADD:
        return *lhs_val + *rhs_val;
      case BinaryOp::SUB:
        return *lhs_val - *rhs_val;
      case BinaryOp::MUL:
        return *lhs_val * *rhs_val;
      case BinaryOp::DIV:
        if (*rhs_val != 0)
          return *lhs_val / *rhs_val;
        break;
      case BinaryOp::MOD:
        if (*rhs_val != 0)
          return *lhs_val % *rhs_val;
        break;
      default:
        break;
      }
    }
  } else if (auto *lval = dynamic_cast<LVal *>(expr)) {
    SymbolInfo *sym = str_table.lookup(lval->name);
    if (sym && sym->kind == SymbolKind::CONSTANT && lval->indices.empty()) {
      if (auto val = sym->getConstantValue<int>()) {
        return *val;
      }
    }
    if (isdef) {
      if (sym && irgen_table.name_to_value[lval->name]) {
        return irgen_table.name_to_value[lval->name];
      }
    }
  }
  return std::nullopt;
}

// 浮点常量表达式求值
std::optional<float> IRgenerator::evaluateConstExpressionFloat(Exp *expr) {
  if (auto *num = dynamic_cast<Number *>(expr)) {
    if (std::holds_alternative<float>(num->value)) {
      return std::get<float>(num->value);
    } else if (std::holds_alternative<int>(num->value)) {
      // 整数到浮点数转换
      return static_cast<float>(std::get<int>(num->value));
    }
  } else if (auto *unary = dynamic_cast<UnaryExp *>(expr)) {
    auto operand_val = evaluateConstExpressionFloat(unary->operand.get());
    if (operand_val) {
      switch (unary->op) {
      case UnaryOp::PLUS:
        return *operand_val;
      case UnaryOp::MINUS:
        return -*operand_val;
      case UnaryOp::NOT:
        return static_cast<float>(!static_cast<bool>(*operand_val));
      }
    }
  } else if (auto *binary = dynamic_cast<BinaryExp *>(expr)) {
    auto lhs_val = evaluateConstExpressionFloat(binary->lhs.get());
    auto rhs_val = evaluateConstExpressionFloat(binary->rhs.get());
    if (lhs_val && rhs_val) {
      switch (binary->op) {
      case BinaryOp::ADD:
        return *lhs_val + *rhs_val;
      case BinaryOp::SUB:
        return *lhs_val - *rhs_val;
      case BinaryOp::MUL:
        return *lhs_val * *rhs_val;
      case BinaryOp::DIV:
        if (*rhs_val != 0.0f)
          return *lhs_val / *rhs_val;
        break;
      case BinaryOp::MOD:
        if (*rhs_val != 0.0f)
          return fmodf(*lhs_val, *rhs_val);
        break;
      default:
        break;
      }
    }
  } else if (auto *lval = dynamic_cast<LVal *>(expr)) {
    SymbolInfo *sym = str_table.lookup(lval->name);
    if (sym && sym->kind == SymbolKind::CONSTANT && lval->indices.empty()) {
      if (auto val = sym->getConstantValue<float>()) {
        return *val;
      } else if (auto val = sym->getConstantValue<int>()) {
        // 整数常量转换为浮点数
        return static_cast<float>(*val);
      }
    }
  }
  return std::nullopt;
}

// 全局变量初始化器求值函数，在编译时处理常量表达式
std::optional<double> IRgenerator::evaluateGlobalInitializer(InitVal *init) {
  // 处理单一表达式初始化
  if (std::holds_alternative<std::unique_ptr<Exp>>(init->value)) {
    Exp *expr = std::get<std::unique_ptr<Exp>>(init->value).get();

    // 优先尝试整数常量表达式求值
    auto int_val = evaluateConstExpression(expr, 0);
    if (int_val.has_value()) {
      return static_cast<double>(int_val.value());
    }

    // 再尝试浮点常量表达式求值
    auto float_val = evaluateConstExpressionFloat(expr);
    if (float_val.has_value()) {
      return static_cast<double>(float_val.value());
    }
  }

  return std::nullopt;
}

// New helper function to infer expression type
std::shared_ptr<Type> IRgenerator::inferExpressionType(Exp *expression) {
  if (auto *lval = dynamic_cast<LVal *>(expression)) {
    SymbolInfo *sym = str_table.lookup(lval->name);
    if (sym) {
      if (lval->indices.empty()) {
        return sym->type;
      } else {
        auto *arr_type = dynamic_cast<ArrayType *>(sym->type.get());
        if (arr_type && arr_type->dimensions.size() > lval->indices.size()) {
          std::vector<int> remaining_dims(arr_type->dimensions.begin() +
                                              lval->indices.size(),
                                          arr_type->dimensions.end());
          return makeArrayType(arr_type->element_type, remaining_dims);
        } else if (arr_type &&
                   arr_type->dimensions.size() == lval->indices.size()) {
          return arr_type->element_type;
        }
      }
    }
  } else if (auto *num = dynamic_cast<Number *>(expression)) {
    if (std::holds_alternative<int>(num->value)) {
      return makeBasicType(BaseType::INT);
    } else {
      return makeBasicType(BaseType::FLOAT);
    }
  } else if (auto *unary = dynamic_cast<UnaryExp *>(expression)) {
    return inferExpressionType(unary->operand.get());
  } else if (auto *binary = dynamic_cast<BinaryExp *>(expression)) {
    auto lhs_type = inferExpressionType(binary->lhs.get());
    auto rhs_type = inferExpressionType(binary->rhs.get());
    if (lhs_type && rhs_type) {
      if (dynamic_cast<BasicType *>(lhs_type.get()) &&
          dynamic_cast<BasicType *>(rhs_type.get())) {
        return lhs_type; // Simplified: assumes compatible types
      }
    }
  } else if (auto *call = dynamic_cast<FunctionCall *>(expression)) {
    SymbolInfo *sym = str_table.lookup(call->function_name);
    if (sym && sym->kind == SymbolKind::FUNCTION) {
      auto *func_type = dynamic_cast<FunctionType *>(sym->type.get());
      if (func_type)
        return func_type->return_type;
    }
  }
  return nullptr;
}

// New helper function to check if a register is a pointer
bool IRgenerator::isPointer(int reg) {
  auto it = irgen_table.RegTable.find(reg);
  if (it == irgen_table.RegTable.end())
    return false;

  // Check if the register is associated with a variable in the symbol table
  for (const auto &scope : irgen_table.name_to_reg) {
    for (const auto &[name, reg_id] : scope) {
      if (reg_id == reg) {
        SymbolInfo *sym = str_table.lookup(name);
        if (sym && (sym->kind == SymbolKind::VARIABLE ||
                    sym->kind == SymbolKind::PARAMETER)) {
          // If it's a variable or parameter with no indices, check if it's an
          // array or explicitly a pointer
          auto *arr_type = dynamic_cast<ArrayType *>(sym->type.get());
          if (arr_type || sym->is_array_pointer ||
              sym->kind == SymbolKind::VARIABLE) {
            return true;
          }
        }
        break;
      }
    }
  }
  // If the register has dimensions, it's a pointer (e.g., array base)
  return !it->second.dims.empty();
}

void IRgenerator::visit(CompUnit &node) {
  // AddLibFunctionDeclare();
  for (auto &item : node.items) {
    if (auto decl = std::get_if<std::unique_ptr<Decl>>(&item)) {
      (*decl)->accept(*this);
    } else if (auto funcDef = std::get_if<std::unique_ptr<FuncDef>>(&item)) {
      (*funcDef)->accept(*this);
    }
  }
}

void IRgenerator::visit(ConstDecl &node) {
  BaseType prev_type = current_type;
  current_type = node.type;
  for (auto &def : node.definitions) {
    def->accept(*this);
  }
  current_type = prev_type;
}

// 辅助函数：递归展平常量初始化列表
void IRgenerator::flattenConstInit(
    ConstInitVal *init, std::vector<std::variant<int, float>> &result,
    BaseType type, const std::vector<int> &dims) {
  if (!init)
    return;

  if (std::holds_alternative<std::unique_ptr<Exp>>(init->value)) {
    auto &expr = std::get<std::unique_ptr<Exp>>(init->value);
    auto int_val = evaluateConstExpression(expr.get(), 0); // 移除类名前缀
    auto float_val = evaluateConstExpressionFloat(expr.get()); // 移除类名前缀

    if (int_val && type == BaseType::INT) {
      result.push_back(*int_val);
    } else if (float_val && type == BaseType::FLOAT) {
      result.push_back(*float_val);
    } else {
      // 默认值
      result.push_back(type == BaseType::INT ? 0 : 0.0f);
    }
  } else {
    auto &list =
        std::get<std::vector<std::unique_ptr<ConstInitVal>>>(init->value);
    for (auto &item : list) {
      flattenConstInit(item.get(), result, type, dims);
      if (!std::holds_alternative<std::unique_ptr<Exp>>(item.get()->value)) {
        auto &listtemp = std::get<std::vector<std::unique_ptr<ConstInitVal>>>(
            item.get()->value);
        int missnum = listtemp.size() % dims[dims.size() - 1];
        if (missnum == 0 && listtemp.size() != 0) {
          continue; // 如果没有缺失元素，继续处理下一个
        }
        for (int i = 0; i < dims[dims.size() - 1] - missnum; i++) {
          result.push_back(type == BaseType::INT ? std::variant<int, float>(0)
                                                 : std::variant<int, float>(
                                                       0.0f)); // 添加缺失元素
        }
      }
    }
  }
}

void IRgenerator::visit(ConstDef &node) {
  if (isGlobalScope()) {
    VarAttribute attr;
    attr.type = current_type;
    attr.ConstTag = true;

    // 处理数组维度
    for (auto &dim : node.array_dimensions) {
      auto val = evaluateConstExpression(dim.get(), 1);
      attr.dims.push_back(val.value_or(1));
    }

    // 处理初始化器
    if (auto *const_init =
            dynamic_cast<ConstInitVal *>(node.initializer.get())) {
      if (!attr.dims.empty()) {
        // 多维数组初始化
        std::vector<std::variant<int, float>> flat_values;
        flattenConstInit(const_init, flat_values, current_type,
                         std::vector<int>(attr.dims.begin(), attr.dims.end()));

        // 计算总元素数量
        size_t total_elements = 1;
        for (auto d : attr.dims)
          total_elements *= d;

        // 填充初始化值
        for (size_t i = 0; i < total_elements; i++) {
          if (i < flat_values.size()) {
            if (auto *ival = std::get_if<int>(&flat_values[i])) {
              attr.IntInitVals.push_back(*ival);
            } else if (auto *fval = std::get_if<float>(&flat_values[i])) {
              attr.FloatInitVals.push_back(*fval);
            }
          } else {
            // 填充默认值
            if (current_type == BaseType::INT) {
              attr.IntInitVals.push_back(0);
            } else {
              attr.FloatInitVals.push_back(0.0f);
            }
          }
        }
      } else {
        // 标量初始化
        if (std::holds_alternative<std::unique_ptr<Exp>>(const_init->value)) {
          auto &expr = std::get<std::unique_ptr<Exp>>(const_init->value);
          if (current_type == BaseType::INT) {
            auto int_val = evaluateConstExpression(expr.get(), 0);
            if (int_val) {
              attr.IntInitVals.push_back(*int_val);
            } else {
              std::cerr << "Error: Cannot evaluate constant expression for "
                        << node.name << std::endl;
              attr.IntInitVals.push_back(0);
            }
          } else if (current_type == BaseType::FLOAT) {
            auto float_val = evaluateConstExpressionFloat(expr.get());
            if (float_val) {
              attr.FloatInitVals.push_back(*float_val);
            } else {
              std::cerr << "Error: Cannot evaluate constant expression for "
                        << node.name << std::endl;
              attr.FloatInitVals.push_back(0.0f);
            }
          }
        }
      }
    }

    llvmIR.global_def.push_back(
        new GlobalVarDefineInstruction(node.name, Type2LLvm[attr.type], attr));
    // 添加到符号表
    SymbolInfo *sym = str_table.lookup(node.name);
    if (!sym) {
      auto sym_info = std::make_shared<SymbolInfo>(
          SymbolKind::CONSTANT, makeBasicType(attr.type), node.name, true);

      // 设置常量值
      if (attr.IntInitVals.size() > 0) {
        sym_info->setConstantValue(attr.IntInitVals[0]);
      } else if (attr.FloatInitVals.size() > 0) {
        sym_info->setConstantValue(attr.FloatInitVals[0]);
      }

      // 设置数组维度
      sym_info->setArrayDimensions(
          std::vector<int>(attr.dims.begin(), attr.dims.end()));
      str_table.insert(node.name, sym_info);
    }
  } else {
    // 局部常量初始化
    int reg = newReg();
    VarAttribute attr;
    attr.type = current_type;
    attr.ConstTag = true;

    // 分配内存
    if (!node.array_dimensions.empty()) {
      std::vector<int> dims;
      for (auto &dim : node.array_dimensions) {
        auto val = evaluateConstExpression(dim.get(), 1);
        dims.push_back(val.value_or(1));
      }
      IRgenAllocaArray(getCurrentBlock(), Type2LLvm[attr.type], reg, dims);
      attr.dims = std::vector<size_t>(dims.begin(), dims.end());
    } else {
      IRgenAlloca(Type2LLvm[attr.type], reg);
    }

    irgen_table.RegTable[reg] = attr;
    irgen_table.insertReg(node.name, reg);
    // irgen_table.name_to_reg[node.name] = reg;

    // 处理初始化
    if (node.initializer) {
      if (attr.dims.empty()) {
        // Scalar variable initialization
        require_address = false;
        node.initializer->accept(*this);
        int init_reg = max_reg;
        VarAttribute init_attr = irgen_table.RegTable[init_reg];
        Operand value;
        // +++ 修复：全局变量直接使用全局名称 +++
        // SymbolInfo* sym = str_table.lookup(node.name);
        // if (sym && sym->kind == SymbolKind::CONSTANT) {
        //     // 全局变量 - 直接存储到全局地址
        //     IRgenStore(getCurrentBlock(), Type2LLvm.at(attr.type),
        //     GetNewRegOperand(init_reg),
        //                GetNewGlobalOperand(node.name));
        // }
        // else
        // 将常量值存储到当前常量的属性中
        if (init_attr.IntInitVals.size() > 0) {
          irgen_table.RegTable[reg].IntInitVals = init_attr.IntInitVals;
        } else if (init_attr.FloatInitVals.size() > 0) {
          irgen_table.RegTable[reg].FloatInitVals = init_attr.FloatInitVals;
        }
        if (init_attr.IntInitVals.size() > 0) {
          value = new ImmI32Operand(init_attr.IntInitVals[0]);
        } else if (init_attr.FloatInitVals.size() > 0) {
          value = new ImmF32Operand(init_attr.FloatInitVals[0]);
        } else if (isPointer(init_reg)) {
          int value_reg = newReg();
          // IRgenLoad(getCurrentBlock(),
          // Type2LLvm[static_cast<int>(attr.type)], value_reg,
          // GetNewRegOperand(init_reg));
          IRgenLoad(getCurrentBlock(), Type2LLvm.at(attr.type), value_reg,
                    GetNewRegOperand(init_reg));
          value = GetNewRegOperand(value_reg);
        } else {
          value = GetNewRegOperand(init_reg);
        }
        if (auto reg_op = dynamic_cast<RegOperand *>(value)) {
          int value_reg = reg_op->GetRegNo();
          // 确保 value 是正确的类型
          int conv_value_reg = convertToType(getCurrentBlock(), value_reg,
                                             Type2LLvm.at(attr.type));
          value =
              GetNewRegOperand(conv_value_reg); // 更新 value 为转换后的寄存器
        }
        IRgenStore(getCurrentBlock(), Type2LLvm.at(attr.type), value,
                   GetNewRegOperand(reg));
        // IRgenStore(getCurrentBlock(), Type2LLvm[static_cast<int>(attr.type)],
        // value, GetNewRegOperand(reg));
      } else {
        // Array initialization
        // handleArrayInitializer(node.initializer->get(), reg, attr, dims, 0);
        // handleArrayInitializer(node.initializer->get(), reg, attr, dims);
        size_t start_index = 0; // 从0开始索引
        handleArrayInitializer(
            node.initializer.get(), reg, attr,
            std::vector<int>(attr.dims.begin(), attr.dims.end()),
            0,          // 从第一个维度开始
            start_index // 当前索引位置
        );

        // 计算总元素数量
        size_t total_elements = 1;
        for (auto d : attr.dims)
          total_elements *= d;

        // 填充初始化值
        for (size_t i = start_index; i < total_elements; i++) {
          LLVMType type = Type2LLvm.at(attr.type);
          // 计算当前元素的线性位置
          size_t pos = i;
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0)); // 数组基址

          // 转换为多维索引
          size_t temp = pos;
          for (size_t i = 0;
               i < std::vector<int>(attr.dims.begin(), attr.dims.end()).size();
               i++) {
            size_t stride = 1;
            for (size_t j = i + 1;
                 j <
                 std::vector<int>(attr.dims.begin(), attr.dims.end()).size();
                 j++) {
              stride *= std::vector<int>(attr.dims.begin(), attr.dims.end())[j];
            }
            size_t idx = temp / stride;
            temp %= stride;
            indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
          }

          // 生成GEP指令
          int ptr_reg = newReg();
          IRgenGetElementptr(
              getCurrentBlock(), type, ptr_reg, GetNewRegOperand(reg),
              std::vector<int>(attr.dims.begin(), attr.dims.end()), indices);

          // 存储值
          if (attr.type == BaseType::INT) {
            int value = 0;
            IRgenStore(getCurrentBlock(), type, new ImmI32Operand(value),
                       GetNewRegOperand(ptr_reg));
          } else {
            float value = 0.0f;
            IRgenStore(getCurrentBlock(), type, new ImmF32Operand(value),
                       GetNewRegOperand(ptr_reg));
          }
        }
      }
    }

    // 添加到符号表
    SymbolInfo *sym = str_table.lookupCurrent(node.name);
    if (!sym) {
      auto sym_info = std::make_shared<SymbolInfo>(
          SymbolKind::CONSTANT, makeBasicType(attr.type), node.name, true);

      // 设置常量值
      if (irgen_table.RegTable[reg].IntInitVals.size() > 0) {
        sym_info->setConstantValue(irgen_table.RegTable[reg].IntInitVals[0]);
      } else if (irgen_table.RegTable[reg].FloatInitVals.size() > 0) {
        sym_info->setConstantValue(irgen_table.RegTable[reg].FloatInitVals[0]);
      }

      // 设置数组维度
      std::vector<int> dims_int(attr.dims.begin(), attr.dims.end());
      sym_info->setArrayDimensions(dims_int);

      str_table.insert(node.name, sym_info);
    }
  }
}

void IRgenerator::visit(VarDecl &node) {
  BaseType prev_type = current_type;
  current_type = node.type;
  for (auto &def : node.definitions) {
    def->accept(*this);
  }
  current_type = prev_type;
}

// 新增函数：展平初始化列表
void IRgenerator::flattenInitVal(InitVal *init,
                                 std::vector<std::variant<int, float>> &result,
                                 BaseType type, const std::vector<int> &dims) {
  if (!init)
    return;

  if (std::holds_alternative<std::unique_ptr<Exp>>(init->value)) {
    auto &expr = std::get<std::unique_ptr<Exp>>(init->value);
    if (auto int_val = evaluateConstExpression(expr.get(), 0)) {
      result.push_back(*int_val);
    } else if (auto float_val = evaluateConstExpressionFloat(expr.get())) {
      result.push_back(*float_val);
    } else {
      result.push_back(type == BaseType::INT ? 0 : 0.0f);
    }
  } else {
    auto &list = std::get<std::vector<std::unique_ptr<InitVal>>>(init->value);
    for (auto &item : list) {
      flattenInitVal(item.get(), result, type, dims);
      if (!std::holds_alternative<std::unique_ptr<Exp>>(item.get()->value)) {
        auto &listtemp =
            std::get<std::vector<std::unique_ptr<InitVal>>>(item.get()->value);
        int missnum = listtemp.size() % dims[dims.size() - 1];
        if (missnum == 0 && listtemp.size() != 0) {
          continue; // 如果没有缺失元素，继续处理下一个
        }
        for (int i = 0; i < dims[dims.size() - 1] - missnum; i++) {
          result.push_back(type == BaseType::INT ? std::variant<int, float>(0)
                                                 : std::variant<int, float>(
                                                       0.0f)); // 添加缺失元素
        }
      }
    }
  }
}

void IRgenerator::visit(VarDef &node) {
  if (isGlobalScope()) {
    VarAttribute attr;
    attr.type = current_type;
    for (auto &dim : node.array_dimensions) {
      auto val = evaluateConstExpression(dim.get(), 1);
      attr.dims.push_back(val.value_or(1));
    }
    if (node.initializer) {
      if (auto *init = dynamic_cast<InitVal *>(node.initializer->get())) {
        // 展平初始化列表
        std::vector<std::variant<int, float>> flat_values;
        flattenInitVal(init, flat_values, current_type,
                       std::vector<int>(attr.dims.begin(), attr.dims.end()));

        // 填充初始化值
        size_t total_elements = 1;
        for (auto d : attr.dims)
          total_elements *= d;

        for (size_t i = 0; i < total_elements; i++) {
          if (i < flat_values.size()) {
            if (auto *ival = std::get_if<int>(&flat_values[i])) {
              attr.IntInitVals.push_back(*ival);
            } else if (auto *fval = std::get_if<float>(&flat_values[i])) {
              attr.FloatInitVals.push_back(*fval);
            }
          } else {
            // 填充默认值
            if (current_type == BaseType::INT) {
              attr.IntInitVals.push_back(0);
            } else {
              attr.FloatInitVals.push_back(0.0f);
            }
          }
        }
        if (attr.dims.empty()) {
          if (current_type == BaseType::INT) {
            irgen_table.name_to_value[node.name] = attr.IntInitVals[0];
          } else {
            irgen_table.name_to_value[node.name] = attr.FloatInitVals[0];
          }
        }
      } else {
        // 单个初始化表达式
        auto init_val = evaluateGlobalInitializer(node.initializer->get());
        if (init_val.has_value()) {
          if (current_type == BaseType::INT) {
            attr.IntInitVals.push_back(static_cast<int>(init_val.value()));
            irgen_table.name_to_value[node.name] =
                static_cast<int>(init_val.value());
          } else if (current_type == BaseType::FLOAT) {
            attr.FloatInitVals.push_back(static_cast<float>(init_val.value()));
            irgen_table.name_to_value[node.name] =
                static_cast<float>(init_val.value());
          }
        }
      }
    }
    // llvmIR.global_def.push_back(new GlobalVarDefineInstruction(node.name,
    // Type2LLvm[static_cast<int>(attr.type)], attr));
    llvmIR.global_def.push_back(
        new GlobalVarDefineInstruction(node.name, Type2LLvm[attr.type], attr));

    // Add to symbol table
    SymbolInfo *sym = str_table.lookup(node.name);
    if (!sym) {
      auto sym_info = std::make_shared<SymbolInfo>(
          SymbolKind::VARIABLE, makeBasicType(attr.type), node.name,
          node.initializer.has_value());
      sym_info->setArrayDimensions(
          std::vector<int>(attr.dims.begin(), attr.dims.end()));
      str_table.insert(node.name, sym_info);
    }
  } else {
    int reg = newReg();
    VarAttribute attr;
    attr.type = current_type;
    attr.ConstTag = false; // 明确标记为非常量
    std::vector<int> dims;
    for (auto &dim : node.array_dimensions) {
      auto val = evaluateConstExpression(dim.get(), 1);
      if (val) {
        dims.push_back(*val);
      } else {
        dims.push_back(1); // 非常量维度的回退值
      }
    }
    attr.dims = std::vector<size_t>(dims.begin(), dims.end());
    if (!dims.empty()) {
      // IRgenAllocaArray(getCurrentBlock(),
      // Type2LLvm[static_cast<int>(attr.type)], reg, dims);
      IRgenAllocaArray(getCurrentBlock(), Type2LLvm.at(attr.type), reg, dims);
    } else {
      // IRgenAlloca(getCurrentBlock(), Type2LLvm[static_cast<int>(attr.type)],
      // reg);
      IRgenAlloca(Type2LLvm.at(attr.type), reg);
    }
    irgen_table.RegTable[reg] = attr;
    irgen_table.insertReg(node.name, reg);
    // irgen_table.name_to_reg[node.name] = reg;
    if (node.initializer) {
      if (dims.empty()) {
        // Scalar variable initialization
        require_address = false;
        (*node.initializer)->accept(*this);
        int init_reg = max_reg;
        VarAttribute init_attr = irgen_table.RegTable[init_reg];
        Operand value;
        // +++ 修复：全局变量直接使用全局名称 +++
        // SymbolInfo* sym = str_table.lookup(node.name);
        // if (sym && sym->kind == SymbolKind::CONSTANT) {
        //     // 全局变量 - 直接存储到全局地址
        //     IRgenStore(getCurrentBlock(), Type2LLvm.at(attr.type),
        //     GetNewRegOperand(init_reg),
        //                GetNewGlobalOperand(node.name));
        // }
        // else
        if (init_attr.IntInitVals.size() > 0) {
          value = new ImmI32Operand(init_attr.IntInitVals[0]);
          irgen_table.name_to_value[node.name] = init_attr.IntInitVals[0];
        } else if (init_attr.FloatInitVals.size() > 0) {
          value = new ImmF32Operand(init_attr.FloatInitVals[0]);
          irgen_table.name_to_value[node.name] = init_attr.FloatInitVals[0];
        } else if (isPointer(init_reg)) {
          int value_reg = newReg();
          // IRgenLoad(getCurrentBlock(),
          // Type2LLvm[static_cast<int>(attr.type)], value_reg,
          // GetNewRegOperand(init_reg));
          IRgenLoad(getCurrentBlock(), Type2LLvm.at(attr.type), value_reg,
                    GetNewRegOperand(init_reg));
          value = GetNewRegOperand(value_reg);
        } else {
          value = GetNewRegOperand(init_reg);
        }
        if (auto reg_op = dynamic_cast<RegOperand *>(value)) {
          int value_reg = reg_op->GetRegNo();
          // 确保 value 是正确的类型
          int conv_value_reg = convertToType(getCurrentBlock(), value_reg,
                                             Type2LLvm.at(attr.type));
          value =
              GetNewRegOperand(conv_value_reg); // 更新 value 为转换后的寄存器
        }
        IRgenStore(getCurrentBlock(), Type2LLvm.at(attr.type), value,
                   GetNewRegOperand(reg));
        // IRgenStore(getCurrentBlock(), Type2LLvm[static_cast<int>(attr.type)],
        // value, GetNewRegOperand(reg));
      } else {
        // Array initialization
        // handleArrayInitializer(node.initializer->get(), reg, attr, dims, 0);
        // handleArrayInitializer(node.initializer->get(), reg, attr, dims);
        size_t start_index = 0; // 从0开始索引
        handleArrayInitializer(
            dynamic_cast<InitVal *>(node.initializer.value().get()), reg, attr,
            dims,
            0,          // 从第一个维度开始
            start_index // 当前索引位置
        );
        size_t total_elements = 1;
        for (auto d : dims)
          total_elements *= d;

        for (size_t i = start_index; i < total_elements; i++) {
          LLVMType type = Type2LLvm.at(attr.type);
          // 计算当前元素的线性位置
          size_t pos = i;
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0)); // 数组基址

          // 转换为多维索引
          size_t temp = pos;
          for (size_t i = 0; i < dims.size(); i++) {
            size_t stride = 1;
            for (size_t j = i + 1; j < dims.size(); j++) {
              stride *= dims[j];
            }
            size_t idx = temp / stride;
            temp %= stride;
            indices.push_back(new ImmI32Operand(static_cast<int>(idx)));
          }

          // 生成GEP指令
          int ptr_reg = newReg();
          IRgenGetElementptr(getCurrentBlock(), type, ptr_reg,
                             GetNewRegOperand(reg), dims, indices);

          // 存储值
          if (attr.type == BaseType::INT) {
            int value = 0;
            IRgenStore(getCurrentBlock(), type, new ImmI32Operand(value),
                       GetNewRegOperand(ptr_reg));
          } else {
            float value = 0.0f;
            IRgenStore(getCurrentBlock(), type, new ImmF32Operand(value),
                       GetNewRegOperand(ptr_reg));
          }
          // IRgenStore(B, type, new ImmF32Operand(value),
          // GetNewRegOperand(ptr_reg));
        }
      }
    }

    // Add to symbol table
    SymbolInfo *sym = str_table.lookupCurrent(node.name);
    if (!sym) {
      auto sym_info = std::make_shared<SymbolInfo>(
          SymbolKind::VARIABLE, makeBasicType(attr.type), node.name,
          node.initializer.has_value());
      sym_info->setArrayDimensions(dims);
      str_table.insert(node.name, sym_info);
    }
  }
}

void IRgenerator::visit(FuncDef &node) {
  irgen_table.enterScope();
  // str_table.enterScope();
  //  function_now = new
  //  FunctionDefineInstruction(Type2LLvm[static_cast<int>(node.return_type)],
  //  node.name);
  function_now =
      new FunctionDefineInstruction(Type2LLvm.at(node.return_type), node.name);
  function_returntype = node.return_type;
  llvmIR.NewFunction(function_now);
  now_label = 0;
  current_reg_counter = -1; // Reset register counter
  // max_label = 0;            // 重置标签计数器
  //  max_reg = -1;
  //  llvmIR.NewBlock(function_now, now_label);
  if (max_label != 0) {
    max_label++;
  }
  llvmIR.NewBlock(function_now, max_label);
  now_label = max_label;

  // Symbol table setup
  std::vector<std::shared_ptr<Type>> param_types;
  for (const auto &param : node.parameters) {
    if (param->is_array_pointer || !param->array_dimensions.empty()) {
      // Array parameter decays to a pointer
      auto element_type = makeBasicType(param->type);
      // Use empty dimensions to indicate a pointer type
      param_types.push_back(makeArrayType(element_type, {}));
    } else {
      // Scalar parameter
      param_types.push_back(makeBasicType(param->type));
    }
  }
  auto func_type =
      makeFunctionType(makeBasicType(node.return_type), param_types);
  auto sym_info = std::make_shared<SymbolInfo>(SymbolKind::FUNCTION, func_type,
                                               node.name, true);
  str_table.insert(node.name, sym_info);

  // 第一步：处理参数声明
  for (auto &param : node.parameters) {
    param->accept(*this);
  }
  // 第二步：在入口块为标量参数分配空间并存储初始值
  for (auto &param : node.parameters) {
    std::string param_name = param->name;
    int it = irgen_table.lookupReg(param_name);
    if (it == -1)
      continue;

    int param_reg = it;
    VarAttribute &attr = irgen_table.RegTable[param_reg];

    // 判断是否为数组参数
    bool is_array_param = !attr.dims.empty() || param->is_array_pointer;

    if (is_array_param) {
      // 数组参数 - 分配指针空间
      int alloca_reg = newReg();
      IRgenAlloca(PTR, alloca_reg);

      // 存储指针参数
      IRgenStore(getCurrentBlock(), PTR, GetNewRegOperand(param_reg),
                 GetNewRegOperand(alloca_reg));

      // 更新符号表映射
      irgen_table.insertReg(param_name, alloca_reg);
      // irgen_table.name_to_reg[param_name] = alloca_reg;

      // 更新寄存器属性
      irgen_table.RegTable[alloca_reg] = attr;
    } else {
      // 标量参数 - 原有逻辑不变
      int alloca_reg = newReg();
      LLVMType param_type = Type2LLvm.at(attr.type);
      IRgenAlloca(param_type, alloca_reg);
      IRgenStore(getCurrentBlock(), param_type, GetNewRegOperand(param_reg),
                 GetNewRegOperand(alloca_reg));
      irgen_table.insertReg(param_name, alloca_reg);
      // irgen_table.name_to_reg[param_name] = alloca_reg;
      irgen_table.RegTable[alloca_reg] = attr;
    }
  }
  // 处理函数体
  if (node.body) {
    node.body->accept(*this);
  }
  AddNoReturnBlock();
  max_reg_map[function_now] = current_reg_counter;
  max_label_map[function_now] = max_label;
  function_now = nullptr;
  RegOperandMap.clear();        // 清除函数寄存器映射
  LabelOperandMap.clear();      // 清除标签映射
  irgen_table.RegTable.clear(); // 清除寄存器属性表
  irgen_table.exitScope();
  while (str_table.scopes.size() > 1) {
    str_table.scopes.pop_back(); // 确保只保留全局作用域
  }
  function_name_to_maxreg[node.name] = current_reg_counter;
  // str_table.exitScope();
}

void IRgenerator::visit(FuncFParam &node) {
  str_table.enterScope();
  int reg = newReg();
  VarAttribute attr;
  attr.type = node.type;

  // 修改点：正确设置数组参数类型
  if (node.is_array_pointer || !node.array_dimensions.empty()) {
    function_now->InsertFormal(PTR);
    for (auto &dim : node.array_dimensions) {
      auto val = evaluateConstExpression(dim.get(), 1);
      if (val) {
        attr.dims.push_back(*val);
      }
    }
  } else {
    function_now->InsertFormal(Type2LLvm.at(node.type));
  }

  irgen_table.RegTable[reg] = attr;
  irgen_table.insertReg(node.name, reg);
  // irgen_table.name_to_reg[node.name] = reg;

  auto param_type = node.is_array_pointer || !node.array_dimensions.empty()
                        ? makeArrayType(makeBasicType(node.type), {-1})
                        : makeBasicType(node.type);
  auto sym_info = std::make_shared<SymbolInfo>(SymbolKind::PARAMETER,
                                               param_type, node.name, true);
  sym_info->is_array_pointer =
      node.is_array_pointer || !node.array_dimensions.empty();
  str_table.insert(node.name, sym_info);
}

void IRgenerator::visit(Block &node) {
  irgen_table.enterScope();
  str_table.enterScope();
  for (auto &item : node.items) {
    if (auto stmt = std::get_if<std::unique_ptr<Stmt>>(&item)) {
      (*stmt)->accept(*this);
    } else if (auto decl = std::get_if<std::unique_ptr<Decl>>(&item)) {
      (*decl)->accept(*this);
    }
  }
  str_table.exitScope();
  irgen_table.exitScope();
}

void IRgenerator::visit(IfStmt &node) {
  // 生成条件表达式
  int now_label_temp = now_label;
  llvmIR.NewBlock(function_now, now_label_temp);
  now_label_temp = now_label;

  node.condition->accept(*this);
  int cond_reg_if = max_reg;
  LLVMBlock B_if = getCurrentBlock();

  // 如果条件类型是i32（整数），转换为i1（布尔）
  if (RegLLVMTypeMap[cond_reg_if] == LLVMType::I32) {
    int tmp_reg = newReg();
    // IRgenIcmp(B_if, IcmpCond::ne, cond_reg_if, 0, tmp_reg);
    IRgenIcmpImmRight(B_if, IcmpCond::ne, cond_reg_if, 0, tmp_reg);
    cond_reg_if = tmp_reg;
    RegLLVMTypeMap[cond_reg_if] = LLVMType::I1;
  }
  if (RegLLVMTypeMap[cond_reg_if] == LLVMType::FLOAT32) {
    int tmp_reg = newReg();
    IRgenFcmpImmRight(B_if, FcmpCond::ONE, cond_reg_if, 0.0, tmp_reg);
    cond_reg_if = tmp_reg;
    RegLLVMTypeMap[cond_reg_if] = LLVMType::I1;
  }

  int then_label = newLabel();
  // 生成then块
  // llvmIR.NewBlock(function_now, now_label);
  int then_label_temp = then_label;
  llvmIR.NewBlock(function_now, then_label_temp);
  then_label_temp = then_label;
  now_label = then_label;
  node.then_statement->accept(*this);
  LLVMBlock B_then = getCurrentBlock();
  // 如果then块没有终止指令，跳转到合并块
  // if (B->Instruction_list.empty() ||
  //     (!IsBr(B->Instruction_list.back()) &&
  //     !IsRet(B->Instruction_list.back()))) { IRgenBRUnCond(B, merge_label);
  // }

  int else_label = newLabel();
  // 生成else块
  // llvmIR.NewBlock(function_now, now_label);
  int else_label_temp = else_label;
  llvmIR.NewBlock(function_now, else_label_temp);
  else_label_temp = else_label;
  now_label = else_label;
  if (node.else_statement) {
    node.else_statement->get()->accept(*this);
  }
  LLVMBlock B_else = getCurrentBlock();
  // 如果else块没有终止指令，跳转到合并块
  // if (B->Instruction_list.empty() ||
  //     (!IsBr(B->Instruction_list.back()) &&
  //     !IsRet(B->Instruction_list.back()))) { IRgenBRUnCond(B, merge_label);
  // }

  int merge_label = newLabel();
  // 生成合并块
  int merge_label_temp = merge_label;
  llvmIR.NewBlock(function_now, merge_label_temp);
  merge_label_temp = merge_label;
  now_label = merge_label;
  // 恢复外层状态
  // now_label = saved_now_label;
  // max_label = saved_max_label;
  IRgenBrCond(B_if, cond_reg_if, then_label, else_label);
  // 如果then块没有终止指令，跳转到合并块
  if (B_then->Instruction_list.empty() ||
      (!IsBr(B_then->Instruction_list.back()) &&
       !IsRet(B_then->Instruction_list.back()))) {
    IRgenBRUnCond(B_then, merge_label);
  }
  // 如果else块没有终止指令，跳转到合并块
  if (B_else->Instruction_list.empty() ||
      (!IsBr(B_else->Instruction_list.back()) &&
       !IsRet(B_else->Instruction_list.back()))) {
    IRgenBRUnCond(B_else, merge_label);
  }
}

void IRgenerator::visit(WhileStmt &node) {
  // if (max_label == 0) max_label = 1;
  int old_loop_start = loop_start_label;
  int old_loop_end = loop_end_label;
  int old_before_label = before_label;
  int old_cond_label = cond_label;
  int old_body_label = body_label;
  int old_end_label = end_label;

  // if(body_label!=1){
  //     before_label=body_label;
  // }
  // else{
  // before_label = newLabel();
  //}
  // int before_label_temp = before_label;

  // llvmIR.NewBlock(function_now, before_label_temp);
  //  before_label_temp=before_label;
  //  now_label = before_label;
  int now_label_temp = now_label;
  llvmIR.NewBlock(function_now, now_label_temp);
  now_label_temp = now_label;
  LLVMBlock B_before = getCurrentBlock();
  // 1. 创建条件块
  cond_label = newLabel();
  int cond_label_temp = cond_label;
  loop_start_label = cond_label;
  llvmIR.NewBlock(function_now, cond_label_temp);
  cond_label_temp = cond_label;
  now_label = cond_label;
  // LLVMBlock B_cond = getCurrentBlock();

  // // 2. 创建体块和结束块
  // body_label = newLabel();
  // int body_label_temp = body_label;
  // end_label = newLabel();
  // int end_label_temp = end_label;
  // loop_end_label = end_label;
  // llvmIR.NewBlock(function_now, body_label_temp);
  // body_label_temp = body_label;
  // LLVMBlock B_body = llvmIR.GetBlock(function_now, body_label_temp);
  // body_label_temp = body_label;
  // llvmIR.NewBlock(function_now, end_label_temp);
  // end_label_temp = end_label;

  // if(now_label!=cond_label_temp){
  //  3. 入口跳转到条件块
  if (B_before->Instruction_list.empty() ||
      (!IsBr(B_before->Instruction_list.back()) &&
       !IsRet(B_before->Instruction_list.back())))
    IRgenBRUnCond(B_before, cond_label_temp);
  cond_label_temp = cond_label;
  //}

  // 4. 在条件块插入条件判断和跳转
  now_label = cond_label;
  node.condition->accept(*this);
  LLVMBlock B_cond = getCurrentBlock();
  int cond_reg = max_reg;
  if (RegLLVMTypeMap[cond_reg] == LLVMType::I32) {
    int tmp = newReg();
    IRgenIcmpImmRight(B_cond, IcmpCond::ne, cond_reg, 0, tmp);
    cond_reg = tmp;
    RegLLVMTypeMap[cond_reg] = LLVMType::I1;
  }
  if (RegLLVMTypeMap[cond_reg] == LLVMType::FLOAT32) {
    int tmp = newReg();
    IRgenFcmpImmRight(B_cond, FcmpCond::ONE, cond_reg, 0.0, tmp);
    cond_reg = tmp;
    RegLLVMTypeMap[cond_reg] = LLVMType::I1;
  }

  // 2. 创建体块和结束块
  body_label = newLabel();
  int body_label_temp = body_label;
  end_label = newLabel();
  int end_label_temp = end_label;
  loop_end_label = end_label;
  llvmIR.NewBlock(function_now, body_label_temp);
  body_label_temp = body_label;
  LLVMBlock B_body = llvmIR.GetBlock(function_now, body_label_temp);
  body_label_temp = body_label;
  llvmIR.NewBlock(function_now, end_label_temp);
  end_label_temp = end_label;

  IRgenBrCond(B_cond, cond_reg, body_label_temp, end_label_temp);
  body_label_temp = body_label;
  end_label_temp = end_label;

  // 5. 生成循环体
  now_label = body_label;
  node.body->accept(*this);
  // 体块末尾跳回条件块
  B_body = getCurrentBlock();
  if (B_body->Instruction_list.empty() ||
      (!IsBr(B_body->Instruction_list.back()) &&
       !IsRet(B_body->Instruction_list.back()))) {
    IRgenBRUnCond(B_body, cond_label_temp);
  }
  cond_label_temp = cond_label;

  // 6. 恢复外层状态
  now_label = end_label;
  // before_label = old_before_label;
  // cond_label = old_cond_label;
  // body_label = end_label;
  // end_label = old_end_label;
  // loop_start_label = old_loop_start;
  // loop_end_label = old_loop_end;

  end_label_temp = end_label;
  LLVMBlock B_end = llvmIR.GetBlock(function_now, end_label_temp);
  end_label_temp = end_label;

  if (end_label != max_label) {
    int end_end_label = newLabel();
    int end_end_label_temp = end_end_label;
    llvmIR.NewBlock(function_now, end_end_label_temp);
    end_end_label_temp = end_end_label;
    now_label = end_end_label;
    IRgenBRUnCond(B_end, end_end_label_temp);
  }
  before_label = old_before_label;
  cond_label = old_cond_label;
  body_label = end_label;
  end_label = old_end_label;
  loop_start_label = old_loop_start;
  loop_end_label = old_loop_end;
}

void IRgenerator::visit(ExpStmt &node) {
  if (node.expression) {
    (*node.expression)->accept(*this);
  }
}

void IRgenerator::visit(AssignStmt &node) {
  require_address = true;
  node.lvalue->accept(*this);
  int lvalue_reg = max_reg;
  VarAttribute lvalue_attr = irgen_table.RegTable[lvalue_reg];

  // 获取左值的基本类型
  // LLVMType type;
  // SymbolInfo* sym = str_table.lookup(node.lvalue->name);
  // if (sym) {
  //     if (auto* basic_type = dynamic_cast<BasicType*>(sym->type.get())) {
  //         type = Type2LLvm.at(basic_type->type);
  //     } else {
  //         type = LLVMType::I32; // 默认类型
  //     }
  // } else {
  //     type = Type2LLvm.at(lvalue_attr.type);
  // }

  require_address = false;
  node.rvalue->accept(*this);
  int rvalue_reg = max_reg;
  // type=RegLLVMTypeMap[rvalue_reg];
  LLVMType type;
  SymbolInfo *sym = str_table.lookup(node.lvalue->name);
  if (sym) {
    if (auto *basic_type = dynamic_cast<BasicType *>(sym->type.get())) {
      type = Type2LLvm.at(basic_type->type);
    } else {
      type = RegLLVMTypeMap[rvalue_reg];
      // type = LLVMType::I32; // 默认类型
    }
  } else {
    type = Type2LLvm.at(lvalue_attr.type);
  }

  VarAttribute rvalue_attr = irgen_table.RegTable[rvalue_reg];

  Operand value;
  if (rvalue_attr.IntInitVals.size() > 0) {
    value = new ImmI32Operand(rvalue_attr.IntInitVals[0]);
  } else if (rvalue_attr.FloatInitVals.size() > 0) {
    value = new ImmF32Operand(rvalue_attr.FloatInitVals[0]);
  } else if (isPointer(rvalue_reg)) {
    int value_reg = newReg();
    IRgenLoad(getCurrentBlock(), type, value_reg, GetNewRegOperand(rvalue_reg));
    value = GetNewRegOperand(value_reg);
  } else {
    value = GetNewRegOperand(rvalue_reg);
  }

  // +++ 新增：类型检查与转换 +++
  int final_value_reg = -1;
  // if (lvalue_attr.type == BaseType::INT && rvalue_attr.type ==
  // BaseType::FLOAT) {
  // if (type == LLVMType::I32 && rvalue_attr.type == BaseType::FLOAT)
  if (type == LLVMType::I32 &&
      RegLLVMTypeMap[rvalue_reg] == LLVMType::FLOAT32) {
    // 浮点转整数
    final_value_reg = newReg();
    IRgenFptosi(getCurrentBlock(), rvalue_reg, final_value_reg);
    value = GetNewRegOperand(final_value_reg);
    type = LLVMType::I32; // 确保类型正确
  }
  // else if (lvalue_attr.type == BaseType::FLOAT && rvalue_attr.type ==
  // BaseType::INT) {
  // else if (type == LLVMType::FLOAT32 && rvalue_attr.type == BaseType::INT)
  else if (type == LLVMType::FLOAT32 &&
           RegLLVMTypeMap[rvalue_reg] == LLVMType::I32) {
    // 整数转浮点
    final_value_reg = newReg();
    IRgenSitofp(getCurrentBlock(), rvalue_reg, final_value_reg);
    value = GetNewRegOperand(final_value_reg);
    type = LLVMType::FLOAT32; // 确保类型正确
  } else {
    // 类型相同，无需转换
    final_value_reg = rvalue_reg;
  }

  // 直接使用左值寄存器作为指针
  IRgenStore(getCurrentBlock(), type, value, GetNewRegOperand(lvalue_reg));
}

void IRgenerator::visit(ReturnStmt &node) {
  if (node.expression) {
    require_address = false;
    (*node.expression)->accept(*this);
    int ret_reg = max_reg;
    LLVMType ret_type = Type2LLvm.at(function_returntype);
    VarAttribute ret_attr = irgen_table.RegTable[ret_reg];

    if (ret_attr.IntInitVals.size() > 0) {
      if (ret_type == LLVMType::FLOAT32) {
        // IRgenRetImmFloat(getCurrentBlock(), ret_type, 0.0f);
        //  处理浮点返回类型：将整数常量转换为浮点数
        // int temp_reg = newReg();
        LLVMBlock B = getCurrentBlock();

        // 创建临时寄存器存储整数常量
        // B->InsertInstruction(1, new ArithmeticInstruction(
        //                             ADD, LLVMType::I32, new ImmI32Operand(0),
        //                             new
        //                             ImmI32Operand(ret_attr.IntInitVals[0]),
        //                             GetNewRegOperand(temp_reg)));

        // 将整数转换为浮点数
        int float_reg = newReg();
        // IRgenSitofp(B, temp_reg, float_reg);
        IRgenSitofp(B, ret_reg, float_reg);

        // 返回浮点数
        IRgenRetReg(B, ret_type, float_reg);
      } else {
        // IRgenRetImmInt(getCurrentBlock(), ret_type, ret_attr.IntInitVals[0]);
        IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
      }
    } else if (ret_attr.FloatInitVals.size() > 0) {
      if (ret_type == LLVMType::I32) {
        int temp_reg = newReg();
        LLVMBlock B = getCurrentBlock();
        IRgenFptosi(B, ret_reg, temp_reg);
        IRgenRetReg(B, ret_type, temp_reg);
      } else {
        IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
      }
      // IRgenRetImmFloat(getCurrentBlock(), ret_type,
      // ret_attr.FloatInitVals[0]);
    } else if (node.expression.has_value() &&
               dynamic_cast<FunctionCall *>((*node.expression).get())) {
      IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
    } else if (isPointer(ret_reg)) {
      int value_reg = newReg();
      IRgenLoad(getCurrentBlock(), ret_type, value_reg,
                GetNewRegOperand(ret_reg));
      IRgenRetReg(getCurrentBlock(), ret_type, value_reg);
    } else if (dynamic_cast<LVal *>((*node.expression).get())) {
      // 检查是否是全局常量数组元素
      LVal *lval = dynamic_cast<LVal *>((*node.expression).get());
      SymbolInfo *sym = str_table.lookup(lval->name);

      int final_reg = ret_reg;
      LLVMBlock B = getCurrentBlock();

      // 如果实际类型是整数但需要返回浮点
      if (RegLLVMTypeMap[ret_reg] == I32 && ret_type == FLOAT32) {
        int float_reg = newReg();
        IRgenSitofp(B, ret_reg, float_reg);
        final_reg = float_reg;
      }
      // 如果实际类型是浮点但需要返回整数
      else if (RegLLVMTypeMap[ret_reg] == FLOAT32 && ret_type == I32) {
        int int_reg = newReg();
        IRgenFptosi(B, ret_reg, int_reg);
        final_reg = int_reg;
      }

      ret_reg = final_reg;

      if (sym && sym->kind == SymbolKind::CONSTANT && !lval->indices.empty()) {
        // 全局常量数组元素 - 直接返回值
        IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
      } else {
        // 局部常量变量 - 需要加载值
        // int value_reg = newReg();
        // IRgenLoad(getCurrentBlock(), ret_type, value_reg,
        // GetNewRegOperand(ret_reg)); IRgenRetReg(getCurrentBlock(), ret_type,
        // value_reg);
        IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
      }
    } else {
      // IRgenRetReg(getCurrentBlock(), ret_type, ret_reg);
      //  非函数调用、非常量、非左值：检查是否需要类型转换
      int final_reg = ret_reg;
      LLVMBlock B = getCurrentBlock();

      // 如果实际类型是整数但需要返回浮点
      if (RegLLVMTypeMap[ret_reg] == I32 && ret_type == FLOAT32) {
        int float_reg = newReg();
        IRgenSitofp(B, ret_reg, float_reg);
        final_reg = float_reg;
      }
      // 如果实际类型是浮点但需要返回整数
      else if (RegLLVMTypeMap[ret_reg] == FLOAT32 && ret_type == I32) {
        int int_reg = newReg();
        IRgenFptosi(B, ret_reg, int_reg);
        final_reg = int_reg;
      }

      IRgenRetReg(B, ret_type, final_reg);
    }
  } else {
    IRgenRetVoid(getCurrentBlock());
  }
}

void IRgenerator::visit(BreakStmt &) {
  if (loop_end_label != -1) {
    IRgenBRUnCond(getCurrentBlock(), loop_end_label);
  }
  return;
}

void IRgenerator::visit(ContinueStmt &) {
  if (loop_start_label != -1) {
    IRgenBRUnCond(getCurrentBlock(), loop_start_label);
  }
  return;
}

void IRgenerator::visit(UnaryExp &node) {
  node.operand->accept(*this);
  int operand_reg = max_reg;
  int result_reg = newReg();
  VarAttribute operand_attr = irgen_table.RegTable[operand_reg];

  // 处理嵌套的一元操作
  int sign = 1;
  UnaryOp effective_op = node.op;
  Exp *current = &node;
  while (auto *unary = dynamic_cast<UnaryExp *>(current)) {
    if (unary->op == UnaryOp::PLUS) {
      // 正号不改变符号
    } else if (unary->op == UnaryOp::MINUS) {
      sign *= -1;
    } else if (unary->op == UnaryOp::NOT) {
      effective_op = UnaryOp::NOT;
      break;
    }
    current = unary->operand.get();
  }

  // 准备操作数
  Operand operand;
  LLVMType operand_type;
  if (isPointer(operand_reg)) {
    // 指针类型需要加载值
    int value_reg = newReg();
    operand_type = Type2LLvm.at(operand_attr.type);
    IRgenLoad(getCurrentBlock(), operand_type, value_reg,
              GetNewRegOperand(operand_reg));
    operand = GetNewRegOperand(value_reg);
    operand_reg = value_reg;
  } else if (operand_attr.IntInitVals.size() > 0) {
    // 整数常量
    operand = new ImmI32Operand(operand_attr.IntInitVals[0]);
    operand_type = LLVMType::I32;
  } else if (operand_attr.FloatInitVals.size() > 0) {
    // 浮点数常量
    operand = new ImmF32Operand(operand_attr.FloatInitVals[0]);
    operand_type = LLVMType::FLOAT32;
  } else {
    // 直接使用寄存器
    operand = GetNewRegOperand(operand_reg);
    operand_type = Type2LLvm.at(operand_attr.type);
  }

  // switch (effective_op) {
  switch (node.op) {
  case UnaryOp::PLUS:
    result_reg = operand_reg; // Reuse operand register
    RegLLVMTypeMap[result_reg] = operand_type;
    break;
  case UnaryOp::MINUS:
    if (sign == -1) {
      // 确保类型有效
      if (operand_type == LLVMType::VOID_TYPE) {
        operand_type = LLVMType::I32;
      }

      // 创建零值操作数
      Operand zero;
      if (operand_type == LLVMType::FLOAT32) {
        zero = new ImmF32Operand(0.0f);
        if (auto reg_op = dynamic_cast<RegOperand *>(operand)) {
          int ptr_reg = reg_op->GetRegNo();
          if (RegLLVMTypeMap.find(ptr_reg) != RegLLVMTypeMap.end() &&
              RegLLVMTypeMap[ptr_reg] != LLVMType::FLOAT32) {
            // 非指针类型，转换为指针
            int temp_reg = newReg();
            temp_reg = convertToType(getCurrentBlock(), ptr_reg, FLOAT32);
            operand = GetNewRegOperand(temp_reg);
          }
        }
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(FSUB, operand_type, zero, operand,
                                         GetNewRegOperand(result_reg)));
      } else {
        zero = new ImmI32Operand(0);
        if (auto reg_op = dynamic_cast<RegOperand *>(operand)) {
          int ptr_reg = reg_op->GetRegNo();
          if (RegLLVMTypeMap.find(ptr_reg) != RegLLVMTypeMap.end() &&
              RegLLVMTypeMap[ptr_reg] != LLVMType::I32) {
            // 非指针类型，转换为指针
            int temp_reg = newReg();
            temp_reg = convertToType(getCurrentBlock(), ptr_reg, I32);
            operand = GetNewRegOperand(temp_reg);
          }
        }
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(SUB, operand_type, zero, operand,
                                         GetNewRegOperand(result_reg)));
      }

      // 生成减法指令
      // getCurrentBlock()->InsertInstruction(1,
      //     new ArithmeticInstruction(SUB, operand_type, zero, operand,
      //     GetNewRegOperand(result_reg)));
      RegLLVMTypeMap[result_reg] = operand_type;
    } else {
      // 正号不需要操作，但需要确保操作数已加载
      result_reg = operand_reg; // 直接使用操作数寄存器
    }
    break;
  case UnaryOp::NOT:
    if (operand_attr.IntInitVals.size() > 0) {
      // 整数常量取反
      // int val = operand_attr.IntInitVals[0];
      // irgen_table.RegTable[result_reg].type = BaseType::INT;
      // irgen_table.RegTable[result_reg].IntInitVals.push_back(!val);
      // RegLLVMTypeMap[result_reg] = I1;
      int val = operand_attr.IntInitVals[0];
      int result_val = !val; // 计算取反结果

      // 创建新的寄存器并存储结果值
      int temp_reg = newReg();
      getCurrentBlock()->InsertInstruction(
          1, new ArithmeticInstruction(ADD, I1, new ImmI32Operand(0),
                                       new ImmI32Operand(result_val),
                                       GetNewRegOperand(temp_reg)));

      // 设置寄存器属性
      irgen_table.RegTable[temp_reg].type = BaseType::INT;
      irgen_table.RegTable[temp_reg].IntInitVals.push_back(result_val);
      RegLLVMTypeMap[temp_reg] = I1;
      result_reg = temp_reg; // 更新当前最大寄存器
    } else if (operand_attr.FloatInitVals.size() > 0) {
      // 浮点数常量取反
      // float val = operand_attr.FloatInitVals[0];
      // irgen_table.RegTable[result_reg].type = BaseType::INT;
      // irgen_table.RegTable[result_reg].IntInitVals.push_back(val == 0.0f ? 1
      // : 0); RegLLVMTypeMap[result_reg] = I1;
      float val = operand_attr.FloatInitVals[0];
      int result_val = (val == 0.0f) ? 1 : 0; // 计算取反结果

      // 创建新的寄存器并存储结果值
      int temp_reg = newReg();
      getCurrentBlock()->InsertInstruction(
          1, new ArithmeticInstruction(ADD, I1, new ImmI32Operand(0),
                                       new ImmI32Operand(result_val),
                                       GetNewRegOperand(temp_reg)));

      // 设置寄存器属性
      irgen_table.RegTable[temp_reg].type = BaseType::INT;
      irgen_table.RegTable[temp_reg].IntInitVals.push_back(result_val);
      RegLLVMTypeMap[temp_reg] = I1;
      result_reg = temp_reg; // 更新当前最大寄存器
    } else {
      // 生成比较指令（结果始终是i1）
      int cmp_reg = newReg();
      if (operand_type == LLVMType::FLOAT32) {
        IRgenFcmpImmRight(getCurrentBlock(), FcmpCond::OEQ, operand_reg, 0.0f,
                          cmp_reg);
        // getCurrentBlock()->InsertInstruction(1,
        //     new FcmpInstruction(operand_type, operand, new
        //     ImmF32Operand(0.0f), FcmpCond::OEQ, GetNewRegOperand(cmp_reg)));
      } else {
        IRgenIcmpImmRight(getCurrentBlock(), IcmpCond::eq, operand_reg, 0,
                          cmp_reg);
        // 默认使用整数比较
        // getCurrentBlock()->InsertInstruction(1,
        //     new IcmpInstruction(LLVMType::I32, operand, new ImmI32Operand(0),
        //     IcmpCond::eq, GetNewRegOperand(cmp_reg)));
      }
      // 记录结果为i1类型
      RegLLVMTypeMap[cmp_reg] = LLVMType::I1;

      // +++ 新增：将i1结果转换为i32（0或1） +++
      // int zext_reg = newReg();
      // IRgenZextI1toI32(getCurrentBlock(), cmp_reg, zext_reg);
      // result_reg = zext_reg;
      result_reg = cmp_reg;
    }
    break;
  }

  // 更新寄存器属性
  if (effective_op == UnaryOp::PLUS && sign == 1) {
    // 正号操作直接传递原寄存器属性
    irgen_table.RegTable[result_reg] = operand_attr;
  } else {
    irgen_table.RegTable[result_reg].type = operand_attr.type;
  }

  max_reg = result_reg;
}

void IRgenerator::visit(BinaryExp &node) {
  if (node.op == BinaryOp::AND || node.op == BinaryOp::OR) {
    int now_label_temp = now_label;
    llvmIR.NewBlock(function_now, now_label_temp);
    now_label_temp = now_label;
    // int lsh_label=now_label;

    node.lhs->accept(*this);
    int lsh_label = now_label;
    int lhs_reg = max_reg;
    LLVMBlock B_lsh = getCurrentBlock();
    int lhs_conv = convertToType(getCurrentBlock(), lhs_reg, I1);

    int rsh_label = newLabel();
    int rsh_label_temp = rsh_label;
    llvmIR.NewBlock(function_now, rsh_label_temp);
    rsh_label_temp = rsh_label;

    now_label = rsh_label;
    node.rhs->accept(*this);
    int rhs_label_goto = now_label;
    int rhs_reg = max_reg;
    LLVMBlock B_rsh = getCurrentBlock();
    int rhs_conv = convertToType(getCurrentBlock(), rhs_reg, I1);

    int end_label = newLabel();
    int end_label_temp = end_label;
    llvmIR.NewBlock(function_now, end_label_temp);
    end_label_temp = end_label;
    now_label = end_label;
    LLVMBlock B_end = getCurrentBlock();
    now_label = end_label;
    std::vector<std::pair<Operand, Operand>> incoming_values;

    if (node.op == BinaryOp::AND) {
      // From LHS false: false (0)
      incoming_values.push_back(
          std::make_pair(GetNewLabelOperand(lsh_label), new ImmI32Operand(0)));
      // From RHS: RHS value
      incoming_values.push_back(std::make_pair(
          GetNewLabelOperand(rhs_label_goto), GetNewRegOperand(rhs_conv)));
    } else { // BinaryOp::OR
      // From LHS true: true (1)
      incoming_values.push_back(
          std::make_pair(GetNewLabelOperand(lsh_label), new ImmI32Operand(1)));
      // From RHS: RHS value
      incoming_values.push_back(std::make_pair(
          GetNewLabelOperand(rhs_label_goto), GetNewRegOperand(rhs_conv)));
    }

    if (node.op == BinaryOp::AND) {
      // For AND: if LHS is false, go to end with false; else evaluate RHS
      IRgenBrCond(B_lsh, lhs_conv, rsh_label, end_label);
    } else { // BinaryOp::OR
      // For OR: if LHS is true, go to end with true; else evaluate RHS
      IRgenBrCond(B_lsh, lhs_conv, end_label, rsh_label);
    }

    IRgenBRUnCond(B_rsh, end_label);

    int result_reg = newReg();
    B_end->InsertInstruction(
        1,
        new PhiInstruction(I1, GetNewRegOperand(result_reg), incoming_values));
    RegLLVMTypeMap[result_reg] = I1;
    irgen_table.RegTable[result_reg].type = BaseType::INT;
    max_reg = result_reg;
  } else {
    node.lhs->accept(*this);
    int lhs_reg = max_reg;
    VarAttribute lhs_attr = irgen_table.RegTable[lhs_reg];
    node.rhs->accept(*this);
    int rhs_reg = max_reg;
    VarAttribute rhs_attr = irgen_table.RegTable[rhs_reg];
    int result_reg = newReg();

    LLVMType type;

    if (RegLLVMTypeMap[lhs_reg] == LLVMType::FLOAT32 ||
        RegLLVMTypeMap[rhs_reg] == LLVMType::FLOAT32) {
      // 如果一个操作数是浮点类型，另一个是整数类型，将整数转换为浮点类型
      RegLLVMTypeMap[result_reg] = LLVMType::FLOAT32;
      type = LLVMType::FLOAT32;
    } else if (RegLLVMTypeMap[lhs_reg] == LLVMType::I32 ||
               RegLLVMTypeMap[rhs_reg] == LLVMType::I32) {
      // 如果两个操作数都是整数类型，直接使用I32
      RegLLVMTypeMap[result_reg] = LLVMType::I32;
      type = LLVMType::I32;
    } else if (RegLLVMTypeMap[lhs_reg] == LLVMType::I1 &&
               RegLLVMTypeMap[rhs_reg] == LLVMType::I1) {
      // 如果两个操作数都是布尔类型，直接使用I1
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      type = LLVMType::I1;
    } else {
      // 混合类型处理，默认使用I32
      RegLLVMTypeMap[result_reg] = LLVMType::I32;
      type = LLVMType::I32;
    }

    // BaseType result_base_type;
    // if (lhs_attr.type != BaseType::VOID) {
    //     result_base_type = lhs_attr.type;
    // } else if (rhs_attr.type != BaseType::VOID) {
    //     result_base_type = rhs_attr.type;
    // } else {
    //     // 默认使用INT类型防止void,TODO
    //     result_base_type = BaseType::INT;
    // }
    // LLVMType type = Type2LLvm.at(result_base_type);

    // Determine the type of operation
    // LLVMType type = Type2LLvm.at(lhs_attr.type);

    // LLVMType type = Type2LLvm.at(rhs_attr.type);
    //  LLVMType type = Type2LLvm[static_cast<int>(lhs_attr.type)];
    LLVMType result_type = (node.op == BinaryOp::AND || node.op == BinaryOp::OR)
                               ? LLVMType::I1
                               : type;
    type = result_type;
    // Prepare operands
    Operand lhs_operand, rhs_operand;
    bool is_lhs_constant = !lhs_attr.IntInitVals.empty();
    bool is_rhs_constant = !rhs_attr.IntInitVals.empty();
    bool is_lhs_constant_float = !lhs_attr.FloatInitVals.empty();
    bool is_rhs_constant_float = !rhs_attr.FloatInitVals.empty();
    bool is_lhs_pointer = isPointer(lhs_reg);
    bool is_rhs_pointer = isPointer(rhs_reg);

    // Helper function to handle global constants
    auto handleGlobalConstant = [&](const std::string &name,
                                    LLVMType type) -> Operand {
      int value_reg = newReg();
      VarAttribute attr;
      SymbolInfo *sym = str_table.lookup(name);
      if (sym) {
        if (auto *basic_type = dynamic_cast<BasicType *>(sym->type.get())) {
          attr.type = basic_type->type;
          // 确保生成加载指令
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewGlobalOperand(name));
          RegLLVMTypeMap[value_reg] = type;
        }
      }
      irgen_table.RegTable[value_reg] = attr;
      return GetNewRegOperand(value_reg);
    };

    // Handle LHS
    if (is_lhs_constant) {
      if (type == LLVMType::FLOAT32) {
        // lhs_operand = new ImmF32Operand(0.0f);
        //  创建临时寄存器存储整数常量
        int temp_reg = newReg();
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(
                   ADD, LLVMType::I32, new ImmI32Operand(0),
                   new ImmI32Operand(lhs_attr.IntInitVals[0]),
                   GetNewRegOperand(temp_reg)));

        // 将整数转换为浮点数
        int float_reg = newReg();
        IRgenSitofp(getCurrentBlock(), temp_reg, float_reg);
        lhs_operand = GetNewRegOperand(float_reg);
        RegLLVMTypeMap[float_reg] = LLVMType::FLOAT32;
      } else {
        lhs_operand = new ImmI32Operand(lhs_attr.IntInitVals[0]);
      }
    } else if (is_lhs_constant_float) {
      lhs_operand = new ImmF32Operand(lhs_attr.FloatInitVals[0]);
    } else if (is_lhs_pointer) {
      int value_reg = newReg();
      IRgenLoad(getCurrentBlock(), type, value_reg, GetNewRegOperand(lhs_reg));
      lhs_operand = GetNewRegOperand(value_reg);
      RegLLVMTypeMap[value_reg] = type;
    } else if (auto *lval = dynamic_cast<LVal *>(node.lhs.get())) {
      auto it = irgen_table.lookupReg(lval->name);
      SymbolInfo *sym = str_table.lookup(lval->name);
      if (sym && sym->kind == SymbolKind::CONSTANT && lval->indices.empty()) {
        if (auto int_val = sym->getConstantValue<int>()) {
          lhs_operand = new ImmI32Operand(*int_val);
        } else if (auto float_val = sym->getConstantValue<float>()) {
          lhs_operand = new ImmF32Operand(*float_val);
        } else {
          int value_reg = newReg();
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewGlobalOperand(lval->name));
          lhs_operand = GetNewRegOperand(value_reg);
        }
      } else if (it == -1) {
        int value_reg = newReg();
        if (!lval->indices.empty()) {
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0));
          for (auto &idx : lval->indices) {
            idx->accept(*this);
            int idx_reg = max_reg;
            if (irgen_table.RegTable[idx_reg].IntInitVals.size() > 0) {
              indices.push_back(new ImmI32Operand(
                  irgen_table.RegTable[idx_reg].IntInitVals[0]));
            } else {
              indices.push_back(GetNewRegOperand(idx_reg));
            }
          }
          std::vector<int> dims_int(sym->array_dimensions.begin(),
                                    sym->array_dimensions.end());
          int ptr_reg = newReg();
          IRgenGetElementptr(getCurrentBlock(), type, ptr_reg,
                             GetNewGlobalOperand(lval->name), dims_int,
                             indices);
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewRegOperand(ptr_reg));
        } else {
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewGlobalOperand(lval->name));
        }
        lhs_operand = GetNewRegOperand(value_reg);
      } else {
        lhs_operand = GetNewRegOperand(lhs_reg);
      }
    } else {
      lhs_operand = GetNewRegOperand(lhs_reg);
    }

    if (!is_lhs_constant) {
      int lhs_true = convertToType(getCurrentBlock(), lhs_reg, type);
      lhs_operand = GetNewRegOperand(lhs_true);
      RegLLVMTypeMap[lhs_true] = type;
      lhs_reg = lhs_true;
    }

    // Handle RHS
    if (is_rhs_constant) {
      if (type == LLVMType::FLOAT32) {
        // rhs_operand = new ImmF32Operand(0.0f);
        //  创建临时寄存器存储整数常量
        int temp_reg = newReg();
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(
                   ADD, LLVMType::I32, new ImmI32Operand(0),
                   new ImmI32Operand(rhs_attr.IntInitVals[0]),
                   GetNewRegOperand(temp_reg)));

        // 将整数转换为浮点数
        int float_reg = newReg();
        IRgenSitofp(getCurrentBlock(), temp_reg, float_reg);
        rhs_operand = GetNewRegOperand(float_reg);
      } else {
        rhs_operand = new ImmI32Operand(rhs_attr.IntInitVals[0]);
      }
    } else if (is_rhs_constant_float) {
      rhs_operand = new ImmF32Operand(rhs_attr.FloatInitVals[0]);
    } else if (is_rhs_pointer) {
      int value_reg = newReg();
      IRgenLoad(getCurrentBlock(), type, value_reg, GetNewRegOperand(rhs_reg));
      rhs_operand = GetNewRegOperand(value_reg);
    } else if (auto *lval = dynamic_cast<LVal *>(node.rhs.get())) {
      auto it = irgen_table.lookupReg(lval->name);
      SymbolInfo *sym = str_table.lookup(lval->name);
      if (sym && sym->kind == SymbolKind::CONSTANT && lval->indices.empty()) {
        if (auto int_val = sym->getConstantValue<int>()) {
          rhs_operand = new ImmI32Operand(*int_val);
        } else if (auto float_val = sym->getConstantValue<float>()) {
          rhs_operand = new ImmF32Operand(*float_val);
        } else {
          int value_reg = newReg();
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewGlobalOperand(lval->name));
          rhs_operand = GetNewRegOperand(value_reg);
        }
      } else if (it == -1) {
        int value_reg = newReg();
        if (!lval->indices.empty()) {
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0));
          for (auto &idx : lval->indices) {
            idx->accept(*this);
            int idx_reg = max_reg;
            if (irgen_table.RegTable[idx_reg].IntInitVals.size() > 0) {
              indices.push_back(new ImmI32Operand(
                  irgen_table.RegTable[idx_reg].IntInitVals[0]));
            } else {
              indices.push_back(GetNewRegOperand(idx_reg));
            }
          }
          std::vector<int> dims_int(sym->array_dimensions.begin(),
                                    sym->array_dimensions.end());
          int ptr_reg = newReg();
          IRgenGetElementptr(getCurrentBlock(), type, ptr_reg,
                             GetNewGlobalOperand(lval->name), dims_int,
                             indices);
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewRegOperand(ptr_reg));
        } else {
          IRgenLoad(getCurrentBlock(), type, value_reg,
                    GetNewGlobalOperand(lval->name));
        }
        rhs_operand = GetNewRegOperand(value_reg);
      } else {
        rhs_operand = GetNewRegOperand(rhs_reg);
      }
    } else {
      rhs_operand = GetNewRegOperand(rhs_reg);
    }
    if (!is_rhs_constant) {
      int rhs_true = convertToType(getCurrentBlock(), rhs_reg, type);
      rhs_operand = GetNewRegOperand(rhs_true);
      RegLLVMTypeMap[rhs_true] = type;
      rhs_reg = rhs_true;
    }

    // Generate the appropriate instruction
    switch (node.op) {
    case BinaryOp::ADD: {
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(FADD, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(ADD, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = result_type;
      break;
    }
    case BinaryOp::SUB: {
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(FSUB, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(SUB, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = result_type;
      break;
    }
    case BinaryOp::MUL: {
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(FMUL, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(MUL_OP, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = result_type;
      break;
    }
    case BinaryOp::DIV: {
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(FDIV, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new ArithmeticInstruction(DIV_OP, type, lhs_operand, rhs_operand,
                                         GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = result_type;
      break;
    }
    case BinaryOp::MOD:
      getCurrentBlock()->InsertInstruction(
          1, new ArithmeticInstruction(MOD_OP, type, lhs_operand, rhs_operand,
                                       GetNewRegOperand(result_reg)));
      RegLLVMTypeMap[result_reg] = result_type;
      break;
    case BinaryOp::GT:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::OGT,
                                GetNewRegOperand(result_reg)));
      } else {
        // 对于整数类型，使用整数比较
        getCurrentBlock()->InsertInstruction(
            1,
            new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::sgt,
                                GetNewRegOperand(result_reg)));
      }
      // getCurrentBlock()->InsertInstruction(1, new IcmpInstruction(type,
      // lhs_operand, rhs_operand, IcmpCond::sgt,
      // GetNewRegOperand(result_reg)));
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::GTE:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::OGE,
                                GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1,
            new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::sge,
                                GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::LT:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::OLT,
                                GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1,
            new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::slt,
                                GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::LTE:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::OLE,
                                GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1,
            new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::sle,
                                GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::EQ:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::OEQ,
                                GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::eq,
                                   GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::NEQ:
      if (result_type == LLVMType::FLOAT32) {
        getCurrentBlock()->InsertInstruction(
            1,
            new FcmpInstruction(type, lhs_operand, rhs_operand, FcmpCond::ONE,
                                GetNewRegOperand(result_reg)));
      } else {
        getCurrentBlock()->InsertInstruction(
            1, new IcmpInstruction(type, lhs_operand, rhs_operand, IcmpCond::ne,
                                   GetNewRegOperand(result_reg)));
      }
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::AND:
      getCurrentBlock()->InsertInstruction(
          1,
          new ArithmeticInstruction(BITAND, LLVMType::I1, lhs_operand,
                                    rhs_operand, GetNewRegOperand(result_reg)));
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    case BinaryOp::OR:
      getCurrentBlock()->InsertInstruction(
          1,
          new ArithmeticInstruction(BITOR, LLVMType::I1, lhs_operand,
                                    rhs_operand, GetNewRegOperand(result_reg)));
      RegLLVMTypeMap[result_reg] = LLVMType::I1;
      break;
    }
    VarAttribute result_attr;
    result_attr.type = (node.op == BinaryOp::AND || node.op == BinaryOp::OR)
                           ? BaseType::INT
                           : lhs_attr.type;
    irgen_table.RegTable[result_reg] = result_attr;
    max_reg = result_reg;
  }
}

void IRgenerator::visit(LVal &node) {
  int it = irgen_table.lookupReg(node.name);
  if (it != -1) {
    // Local variable or parameter
    int reg = it;
    VarAttribute &attr = irgen_table.RegTable[reg];
    if (!require_address && !node.indices.empty()) {
      std::vector<Operand> indices;
      // Only add leading 0 index for non-parameter arrays
      SymbolInfo *sym = str_table.lookup(node.name);
      if (!(sym && sym->kind == SymbolKind::PARAMETER)) {
        // indices.push_back(new ImmI32Operand(0));
      }
      for (auto &idx : node.indices) {
        bool old_require = require_address;
        require_address = false; // Indices need values, not addresses
        if (auto const_val = evaluateConstExpression(idx.get(), 0)) {
          indices.push_back(new ImmI32Operand(*const_val));
        } else {
          idx->accept(*this);
          int idx_reg = max_reg;
          // Ensure index is a value, not a pointer
          if (isPointer(idx_reg)) {
            int value_reg = newReg();
            IRgenLoad(getCurrentBlock(), LLVMType::I32, value_reg,
                      GetNewRegOperand(idx_reg));
            indices.push_back(GetNewRegOperand(value_reg));
          } else {
            indices.push_back(GetNewRegOperand(idx_reg));
          }
        }
        require_address = old_require;
      }
      int ptr_reg = newReg();
      std::vector<int> dims_int;
      for (auto dim : irgen_table.RegTable[reg].dims) {
        dims_int.push_back(static_cast<int>(dim));
      }
      // Ensure the base is a pointer
      Operand base = GetNewRegOperand(reg);
      if (RegLLVMTypeMap[reg] != PTR) {
        // If not a pointer, load the pointer
        int temp_reg = newReg();
        IRgenLoad(getCurrentBlock(), PTR, temp_reg, GetNewRegOperand(reg));
        base = GetNewRegOperand(temp_reg);
      }
      IRgenGetElementptr(getCurrentBlock(), Type2LLvm.at(attr.type), ptr_reg,
                         base, dims_int, indices);
      if (!require_address) {
        // Load the value if a value is required
        int value_reg = newReg();
        IRgenLoad(getCurrentBlock(), Type2LLvm.at(attr.type), value_reg,
                  GetNewRegOperand(ptr_reg));
        // Set new register attributes
        VarAttribute new_attr;
        new_attr.type = attr.type;
        irgen_table.RegTable[value_reg] = new_attr;
        max_reg = value_reg;
      } else {
        max_reg = ptr_reg;
      }
      return;
    }

    // For local variables, always load the value if not requiring address
    if (!require_address) {
      int value_reg = newReg();
      LLVMType type = Type2LLvm.at(attr.type);
      IRgenLoad(getCurrentBlock(), type, value_reg, GetNewRegOperand(reg));
      // Update register attributes
      VarAttribute new_attr;
      new_attr.type = attr.type;
      irgen_table.RegTable[value_reg] = new_attr;
      RegLLVMTypeMap[value_reg] = type;
      max_reg = value_reg;
      return;
    }

    // Handle local constants
    if (attr.ConstTag && node.indices.empty() && !require_address) {
      // Local constant scalar - directly use the stored value
      if (attr.IntInitVals.size() > 0) {
        int value_reg = newReg();
        irgen_table.RegTable[value_reg] = attr; // Copy constant attributes
        max_reg = value_reg;
        return;
      }
    }

    if (!node.indices.empty()) {
      std::vector<Operand> indices;
      // Only add leading 0 index for non-parameter arrays
      SymbolInfo *sym = str_table.lookup(node.name);
      if (!(sym && sym->kind == SymbolKind::PARAMETER)) {
        // indices.push_back(new ImmI32Operand(0));
      }
      for (auto &idx : node.indices) {
        bool old_require = require_address;
        require_address = false; // Indices need values, not addresses
        if (auto const_val = evaluateConstExpression(idx.get(), 0)) {
          indices.push_back(new ImmI32Operand(*const_val));
        } else {
          idx->accept(*this);
          int idx_reg = max_reg;
          // Ensure index is a value, not a pointer
          if (isPointer(idx_reg)) {
            int value_reg = newReg();
            IRgenLoad(getCurrentBlock(), LLVMType::I32, value_reg,
                      GetNewRegOperand(idx_reg));
            indices.push_back(GetNewRegOperand(value_reg));
          } else {
            indices.push_back(GetNewRegOperand(idx_reg));
          }
        }
        require_address = old_require;
      }
      int ptr_reg = newReg();
      std::vector<int> dims_int;
      for (auto dim : irgen_table.RegTable[reg].dims) {
        dims_int.push_back(static_cast<int>(dim));
      }
      // Ensure the base is a pointer
      Operand base = GetNewRegOperand(reg);
      if (RegLLVMTypeMap[reg] != PTR) {
        // If not a pointer, load the pointer
        int temp_reg = newReg();
        IRgenLoad(getCurrentBlock(), PTR, temp_reg, GetNewRegOperand(reg));
        base = GetNewRegOperand(temp_reg);
      }
      IRgenGetElementptr(getCurrentBlock(), Type2LLvm.at(attr.type), ptr_reg,
                         base, dims_int, indices);
      if (!require_address) {
        // Load the value if a value is required
        int value_reg = newReg();
        IRgenLoad(getCurrentBlock(), Type2LLvm.at(attr.type), value_reg,
                  GetNewRegOperand(ptr_reg));
        // Set new register attributes
        VarAttribute new_attr;
        new_attr.type = attr.type;
        irgen_table.RegTable[value_reg] = new_attr;
        max_reg = value_reg;
      } else {
        max_reg = ptr_reg;
      }
    } else {
      if (require_address) {
        // Directly return the pointer
        max_reg = reg;
      } else {
        // Scalar variable needs to load the value
        if (irgen_table.RegTable[reg].dims.empty()) {
          int value_reg = newReg();
          IRgenLoad(getCurrentBlock(),
                    Type2LLvm.at(irgen_table.RegTable[reg].type), value_reg,
                    GetNewRegOperand(reg));
          // Set new register attributes
          VarAttribute new_attr;
          new_attr.type = attr.type;
          irgen_table.RegTable[value_reg] = new_attr;
          max_reg = value_reg;
        } else {
          // Array directly returns the base address
          max_reg = reg;
        }
      }
    }
  } else {
    // Check if it's a global variable or constant
    SymbolInfo *sym = str_table.lookup(node.name);
    if (sym && (sym->kind == SymbolKind::VARIABLE ||
                sym->kind == SymbolKind::CONSTANT)) {
      auto *basic_type = dynamic_cast<BasicType *>(sym->type.get());
      if (!basic_type) {
        throw std::runtime_error("Global variable " + node.name +
                                 " has invalid type");
      }
      if (sym->kind == SymbolKind::CONSTANT && node.indices.empty() &&
          sym->constant_value.has_value()) {
        // Handle constant scalar value
        int value_reg = newReg();
        VarAttribute attr;
        attr.type = basic_type->type;
        irgen_table.RegTable[value_reg] = attr;
        if (auto int_val = sym->getConstantValue<int>()) {
          irgen_table.RegTable[value_reg].IntInitVals.push_back(*int_val);
        } else if (auto float_val = sym->getConstantValue<float>()) {
          irgen_table.RegTable[value_reg].FloatInitVals.push_back(*float_val);
        }
        RegLLVMTypeMap[value_reg] = Type2LLvm.at(basic_type->type);
        IRgenLoad(getCurrentBlock(), Type2LLvm.at(basic_type->type), value_reg,
                  GetNewGlobalOperand(node.name));
        max_reg = value_reg;
      } else if (node.indices.empty()) {
        if (require_address) {
          // Get global variable address
          int ptr_reg = newReg();
          std::vector<int> dims_int; // Scalar, no dimensions
          std::vector<Operand> indices;
          // indices.push_back(new ImmI32Operand(0));
          IRgenGetElementptr(getCurrentBlock(), Type2LLvm.at(basic_type->type),
                             ptr_reg, GetNewGlobalOperand(node.name), dims_int,
                             indices);
          max_reg = ptr_reg;
        } else {
          // Load value from global variable
          int value_reg = newReg();
          VarAttribute attr;
          attr.type = basic_type->type;
          irgen_table.RegTable[value_reg] = attr;
          IRgenLoad(getCurrentBlock(), Type2LLvm.at(basic_type->type),
                    value_reg, GetNewGlobalOperand(node.name));
          max_reg = value_reg;
        }
      } else {
        // Handle array access for global variable
        std::vector<Operand> indices;
        // indices.push_back(new ImmI32Operand(0)); // First index for global
        // array
        for (auto &idx : node.indices) {
          if (auto const_val = evaluateConstExpression(idx.get(), 0)) {
            indices.push_back(new ImmI32Operand(*const_val));
          } else {
            idx->accept(*this);
            int idx_reg = max_reg;
            indices.push_back(GetNewRegOperand(idx_reg));
          }
        }
        int ptr_reg = newReg();
        std::vector<int> dims_int(sym->array_dimensions.begin(),
                                  sym->array_dimensions.end());
        IRgenGetElementptr(getCurrentBlock(), Type2LLvm.at(basic_type->type),
                           ptr_reg, GetNewGlobalOperand(node.name), dims_int,
                           indices);
        if (!require_address) {
          // Load the value if a value is required
          int value_reg = newReg();
          IRgenLoad(getCurrentBlock(), Type2LLvm.at(basic_type->type),
                    value_reg, GetNewRegOperand(ptr_reg));
          // Set new register attributes
          VarAttribute new_attr;
          new_attr.type = basic_type->type;
          irgen_table.RegTable[value_reg] = new_attr;
          max_reg = value_reg;
        } else {
          max_reg = ptr_reg;
        }
      }
    } else {
      throw std::runtime_error("Undefined variable: " + node.name);
    }
  }
}

void IRgenerator::visit(FunctionCall &node) {
  // 检查是否是库函数
  bool is_lib_func =
      (node.function_name == "getint" || node.function_name == "getch" ||
       node.function_name == "getfloat" || node.function_name == "getarray" ||
       node.function_name == "getfarray" || node.function_name == "putint" ||
       node.function_name == "putch" || node.function_name == "putfloat" ||
       node.function_name == "putarray" || node.function_name == "putfarray" ||
       node.function_name == "putf" ||
       node.function_name == "_sysy_starttime" ||
       node.function_name == "_sysy_stoptime");

  // 获取函数返回类型
  BaseType return_type = BaseType::VOID;
  std::vector<bool> is_pointer_param;
  std::vector<BaseType> param_types;
  if (is_lib_func) {
    // 硬编码库函数的类型信息
    if (node.function_name == "getint" || node.function_name == "getch") {
      return_type = BaseType::INT;
      is_pointer_param.push_back(false);
    } else if (node.function_name == "getfloat") {
      return_type = BaseType::FLOAT;
      is_pointer_param.push_back(false);
    } else if (node.function_name == "getarray" ||
               node.function_name == "getfarray") {
      return_type = BaseType::INT;
      param_types.push_back(BaseType::STRING); // ptr参数
      is_pointer_param.push_back(true);
    } else if (node.function_name == "putint" ||
               node.function_name == "putch") {
      return_type = BaseType::VOID;
      param_types.push_back(BaseType::INT);
      is_pointer_param.push_back(false);
    } else if (node.function_name == "putfloat") {
      return_type = BaseType::VOID;
      param_types.push_back(BaseType::FLOAT);
      is_pointer_param.push_back(false);
    } else if (node.function_name == "putarray" ||
               node.function_name == "putfarray") {
      param_types.push_back(BaseType::INT);
      param_types.push_back(BaseType::STRING); // ptr参数
      return_type = BaseType::VOID;
      is_pointer_param.push_back(false); // First param: int
      is_pointer_param.push_back(true);  // Second param: ptr
    } else if (node.function_name == "putf") {
      param_types.push_back(BaseType::STRING); // ptr参数
      return_type = BaseType::VOID;
      is_pointer_param.push_back(true); // Pointer parameter
    } else if (node.function_name == "_sysy_starttime" ||
               node.function_name == "_sysy_stoptime") {
      param_types.push_back(BaseType::INT);
      return_type = BaseType::VOID;
      is_pointer_param.push_back(false);
    }
  } else {
    // 普通函数：从符号表获取类型信息
    SymbolInfo *sym = str_table.lookup(node.function_name);
    if (sym && sym->kind == SymbolKind::FUNCTION) {
      FunctionType *func_type = dynamic_cast<FunctionType *>(sym->type.get());
      if (func_type) {
        return_type =
            dynamic_cast<BasicType *>(func_type->return_type.get())->type;
        for (auto &param : func_type->parameter_types) {
          BasicType *basic_param = dynamic_cast<BasicType *>(param.get());
          if (basic_param) {
            param_types.push_back(basic_param->type);
          }
          if (dynamic_cast<ArrayType *>(param.get())) {
            is_pointer_param.push_back(true); // Array decays to pointer
          } else {
            is_pointer_param.push_back(false);
          }
        }
      }
    }
  }
  // 准备参数
  std::vector<std::pair<enum LLVMType, Operand>> args;
  for (size_t i = 0; i < node.arguments.size(); i++) {
    // if (param_types[i] == BaseType::STRING) {
    //   require_address = true; // 如果是指针类型，设置为需要地址
    // } else {
    //   require_address = false;
    // }
    require_address =
        (i < is_pointer_param.size()) ? is_pointer_param[i] : false;
    // require_address = false;
    node.arguments[i]->accept(*this);
    // require_address =
    //     (i < is_pointer_param.size()) ? is_pointer_param[i] : false;
    int arg_reg = max_reg;
    VarAttribute arg_attr = irgen_table.RegTable[arg_reg];

    // 确定参数类型：优先使用函数声明的正式类型
    BaseType expected_type =
        (i < param_types.size()) ? param_types[i] : arg_attr.type;
    LLVMType llvm_type = Type2LLvm.at(expected_type);
    if (require_address) {
      llvm_type = PTR;
    }
    LLVMType arg_type;
    Operand arg_operand;
    if (require_address) {
      // For pointer parameters, use the pointer register directly
      arg_type = PTR;
      arg_operand = GetNewRegOperand(arg_reg);
    } else {
      // For non-pointer parameters, determine the type and load if necessary
      arg_type = Type2LLvm.at(arg_attr.type);
      if (arg_attr.IntInitVals.size() > 0) {
        // arg_operand = new ImmI32Operand(arg_attr.IntInitVals[0]);
        if (expected_type == BaseType::FLOAT) {
          // 如果期望类型是浮点数，但实际是整数，则需要转换
          int float_reg = newReg();
          IRgenSitofp(getCurrentBlock(), arg_reg, float_reg);
          arg_operand = GetNewRegOperand(float_reg);
          RegLLVMTypeMap[float_reg] = FLOAT32; // 更新类型映射
        } else {
          arg_operand = new ImmI32Operand(arg_attr.IntInitVals[0]);
          RegLLVMTypeMap[arg_reg] = I32; // 确保整数类型正确
        }
      } else if (arg_attr.FloatInitVals.size() > 0) {
        // arg_operand = new ImmF32Operand(arg_attr.FloatInitVals[0]);
        if (expected_type == BaseType::INT) {
          // 如果期望类型是整数，但实际是浮点数，则需要转换
          int int_reg = newReg();
          IRgenFptosi(getCurrentBlock(), arg_reg, int_reg);
          arg_operand = GetNewRegOperand(int_reg);
          RegLLVMTypeMap[int_reg] = I32; // 更新类型映射
        } else {
          RegLLVMTypeMap[arg_reg] = FLOAT32; // 确保浮点类型正确
          arg_operand = new ImmF32Operand(arg_attr.FloatInitVals[0]);
        }
      } else if (isPointer(arg_reg)) {
        int value_reg = newReg();
        IRgenLoad(getCurrentBlock(), arg_type, value_reg,
                  GetNewRegOperand(arg_reg));
        arg_operand = GetNewRegOperand(value_reg);
      } else {
        arg_operand = GetNewRegOperand(arg_reg);
      }
    }
    int value_reg = newReg();
    value_reg = convertToType(getCurrentBlock(), arg_reg, llvm_type);
    RegLLVMTypeMap[value_reg] = llvm_type; // 更新寄存器类型映射
    arg_operand = GetNewRegOperand(value_reg);
    args.push_back(std::make_pair(llvm_type, arg_operand));
  }
  // 生成调用指令
  if (return_type == BaseType::VOID) {
    // void函数：无返回值
    IRgenCallVoid(getCurrentBlock(), Type2LLvm.at(return_type), args,
                  node.function_name);
    max_reg = -1; // void调用没有结果寄存器
  } else {
    // 非void函数：有返回值
    int result_reg = newReg();
    IRgenCall(getCurrentBlock(), Type2LLvm.at(return_type), result_reg, args,
              node.function_name);
    VarAttribute result_attr;
    result_attr.type = return_type;
    irgen_table.RegTable[result_reg] = result_attr;
    max_reg = result_reg;
  }
}

void IRgenerator::visit(Number &node) {
  max_reg = newReg();
  if (std::holds_alternative<int>(node.value)) {
    int value = std::get<int>(node.value);
    irgen_table.RegTable[max_reg].type = BaseType::INT;
    irgen_table.RegTable[max_reg].IntInitVals.push_back(value);
    RegLLVMTypeMap[max_reg] = I32;

    // 生成实际的常量加载指令
    if (!isGlobalScope()) {
      LLVMBlock B = getCurrentBlock();
      B->InsertInstruction(
          1, new ArithmeticInstruction(
                 ADD, I32,
                 new ImmI32Operand(0), // 使用0 + value的方式创建常量
                 new ImmI32Operand(value), GetNewRegOperand(max_reg)));
    }
  } else {
    float value = std::get<float>(node.value);
    irgen_table.RegTable[max_reg].type = BaseType::FLOAT;
    irgen_table.RegTable[max_reg].FloatInitVals.push_back(value);
    RegLLVMTypeMap[max_reg] = FLOAT32;

    // 生成实际的常量加载指令
    if (!isGlobalScope()) {
      LLVMBlock B = getCurrentBlock();
      B->InsertInstruction(
          1, new ArithmeticInstruction(FADD, FLOAT32, new ImmF32Operand(0.0f),
                                       new ImmF32Operand(value),
                                       GetNewRegOperand(max_reg)));
    }
  }
}

void IRgenerator::visit(StringLiteral &node) {
  std::string str_name = "str_" + std::to_string(newLabel());
  llvmIR.global_def.push_back(
      new GlobalStringConstInstruction(node.value, str_name));
  int reg = newReg();
  IRgenLoad(getCurrentBlock(), PTR, reg, GetNewGlobalOperand(str_name));
  max_reg = reg;
}

// 用于调试
bool iserror(std::string output_file) {
  if (output_file == "/testcases/84_long_array2.sy.s" ||
      // output_file == "/testcases/66_exgcd.sy.s" ||
      output_file == "/testcases/78_side_effect.sy.s" ||
      output_file == "/testcases/77_substr.sy.s" ||
      output_file == "/testcases/98_matrix_mul.sy.s" ||
      // output_file == "/testcases/68_brainfk.sy.s" ||
      output_file == "/testcases/89_many_globals.sy.s" ||
      // output_file == "/testcases/64_calculator.sy.s" ||
      output_file == "/testcases/87_many_params.sy.s" ||
      // output_file == "/testcases/74_kmp.sy.s" ||
      output_file == "/testcases/99_matrix_tran.sy.s" ||
      output_file == "/testcases/82_long_func.sy.s" ||
      output_file == "/testcases/90_many_locals.sy.s" ||
      output_file == "/testcases/73_int_io.sy.s" ||
      // output_file == "/testcases/69_expr_eval.sy.s" ||
      // output_file == "/testcases/67_reverse_output.sy.s" ||
      output_file == "/testcases/76_n_queens.sy.s" ||

      output_file == "/testcases/72_hanoi.sy.s" ||
      // output_file == "/testcases/62_percolation.sy.s" ||
      output_file == "/testcases/94_nested_loops.sy.s" ||
      output_file == "/testcases/93_nested_calls.sy.s" ||
      output_file == "/testcases/95_float.sy.s" ||
      output_file == "/testcases/88_many_params2.sy.s" ||
      // output_file == "/testcases/65_color.sy.s" ||
      output_file == "/testcases/75_max_flow.sy.s" ||
      output_file == "/testcases/70_dijkstra.sy.s" ||
      output_file == "/testcases/71_full_conn.sy.s" ||
      output_file == "/testcases/85_long_code.sy.s" ||
      output_file == "/testcases/96_matrix_add.sy.s" ||

      output_file == "/testcases/35_math.sy.s" ||
      output_file == "/testcases/30_many_dimensions.sy.s" ||
      output_file == "/testcases/29_long_line.sy.s" ||
      output_file == "/testcases/36_rotate.sy.s" ||
      output_file == "/testcases/09_BFS.sy.s" ||
      output_file == "/testcases/26_scope4.sy.s" ||

      output_file == "/testcases/39_fp_params.sy.s" ||
      output_file == "/testcases/16_k_smallest.sy.s" ||
      output_file == "/testcases/22_matrix_multiply.sy.s" ||
      output_file == "/testcases/24_array_only.sy.s" ||
      output_file == "/testcases/10_DFS.sy.s" ||
      output_file == "/testcases/21_union_find.sy.s" ||
      output_file == "/testcases/23_json.sy.s" ||
      output_file == "/testcases/13_LCA.sy.s" ||
      output_file == "/testcases/37_dct.sy.s" ||
      output_file == "/testcases/15_graph_coloring.sy.s" ||
      output_file == "/testcases/32_many_params3.sy.s" ||
      output_file == "/testcases/12_DSU.sy.s" ||
      output_file == "/testcases/11_BST.sy.s" ||
      output_file == "/testcases/14_dp.sy.s" ||
      output_file == "/testcases/38_light2d.sy.s") {
    return 1;
  } else {
    return 0;
  }
  // return 0;
}

void IRgenerator::visit(InitVal &node) {
  if (std::holds_alternative<std::unique_ptr<Exp>>(node.value)) {
    std::get<std::unique_ptr<Exp>>(node.value)->accept(*this);
  } else {
    auto &init_list =
        std::get<std::vector<std::unique_ptr<InitVal>>>(node.value);
    VarAttribute attr;
    attr.type = current_type;
    for (auto &init : init_list) {
      init->accept(*this);
      int init_reg = max_reg;
      if (irgen_table.RegTable[init_reg].IntInitVals.size() > 0) {
        attr.IntInitVals.insert(
            attr.IntInitVals.end(),
            irgen_table.RegTable[init_reg].IntInitVals.begin(),
            irgen_table.RegTable[init_reg].IntInitVals.end());
      }
    }
    irgen_table.RegTable[max_reg] = attr;
  }
}

void IRgenerator::visit(ConstInitVal &node) {
  if (std::holds_alternative<std::unique_ptr<Exp>>(node.value)) {
    std::get<std::unique_ptr<Exp>>(node.value)->accept(*this);
  } else {
    auto &init_list =
        std::get<std::vector<std::unique_ptr<ConstInitVal>>>(node.value);
    VarAttribute attr;
    attr.type = current_type;
    for (auto &init : init_list) {
      init->accept(*this);
      int init_reg = max_reg;
      if (irgen_table.RegTable[init_reg].IntInitVals.size() > 0) {
        attr.IntInitVals.insert(
            attr.IntInitVals.end(),
            irgen_table.RegTable[init_reg].IntInitVals.begin(),
            irgen_table.RegTable[init_reg].IntInitVals.end());
      }
    }
    irgen_table.RegTable[max_reg] = attr;
  }
}
