#pragma once

#include "ast.h"
#include "symtab.h"
#include <cstring> // 添加 memcpy 支持
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// @Instruction types
enum LLVMIROpcode {
  OTHER = 0,
  LOAD = 1,
  STORE = 2,
  ADD = 3,
  SUB = 4,
  ICMP = 5,
  PHI = 6,
  ALLOCA = 7,
  MUL_OP = 8,
  DIV_OP = 9,
  BR_COND = 10,
  BR_UNCOND = 11,
  FADD = 12,
  FSUB = 13,
  FMUL = 14,
  FDIV = 15,
  FCMP = 16,
  MOD_OP = 17,
  BITXOR = 18,
  RET = 19,
  ZEXT = 20,
  SHL = 21,
  FPTOSI = 24,
  GETELEMENTPTR = 25,
  CALL = 26,
  SITOFP = 27,
  GLOBAL_VAR = 28,
  GLOBAL_STR = 29,
  LL_ADDMOD = 30,
  UMIN_I32 = 31,
  UMAX_I32 = 32,
  SMIN_I32 = 33,
  SMAX_I32 = 34,
  BITCAST = 35,
  FMIN_F32 = 36,
  FMAX_F32 = 37,
  BITAND = 38,
  BITOR = 39,
  FPEXT = 40,
  SELECT = 41,
  ASHR = 42,  // 算术右移
  LSHR = 43,  // 逻辑右移
  TRUNC = 44, // 截断
};

// @Operand datatypes
enum LLVMType {
  I32 = 1,
  FLOAT32 = 2,
  PTR = 3,
  VOID_TYPE = 4, // 改为 VOID_TYPE 避免冲突
  I8 = 5,
  I1 = 6,
  I64 = 7,
  DOUBLE = 8
};

// @ <cond> in icmp Instruction
enum IcmpCond {
  eq = 1,  //: equal
  ne = 2,  //: not equal
  ugt = 3, //: unsigned greater than
  uge = 4, //: unsigned greater or equal
  ult = 5, //: unsigned less than
  ule = 6, //: unsigned less or equal
  sgt = 7, //: signed greater than
  sge = 8, //: signed greater or equal
  slt = 9, //: signed less than
  sle = 10 //: signed less or equal
};

enum FcmpCond {
  FALSE = 1, //: no comparison, always returns false
  OEQ = 2,   // ordered and equal
  OGT = 3,   //: ordered and greater than
  OGE = 4,   //: ordered and greater than or equal
  OLT = 5,   //: ordered and less than
  OLE = 6,   //: ordered and less than or equal
  ONE = 7,   //: ordered and not equal
  ORD = 8,   //: ordered (no nans)
  UEQ = 9,   //: unordered or equal
  UGT = 10,  //: unordered or greater than
  UGE = 11,  //: unordered or greater than or equal
  ULT = 12,  //: unordered or less than
  ULE = 13,  //: unordered or less than or equal
  UNE = 14,  //: unordered or not equal
  UNO = 15,  //: unordered (either nans)
  TRUE = 16  //: no comparison, always returns true
};

long long Float_to_Byte(float f);
class VarAttribute {
public:
  BaseType type;
  bool ConstTag = 0;
  std::vector<size_t> dims{};
  std::vector<int> IntInitVals{}; // used for array
  std::vector<float> FloatInitVals{};
  VarAttribute() {
    type = BaseType::VOID;
    ConstTag = false;
  }
};

class BasicOperand;
typedef BasicOperand *Operand;

class BasicOperand {
public:
  enum operand_type {
    REG = 1,
    IMMI32 = 2,
    IMMF32 = 3,
    GLOBAL = 4,
    LABEL = 5,
    IMMI64 = 6
  };

protected:
  operand_type operandType;

public:
  BasicOperand() {}
  operand_type GetOperandType() { return operandType; }
  bool isIMM() {
    return operandType == IMMI32 || operandType == IMMF32 ||
           operandType == IMMI64;
  }
  virtual std::string GetFullName() = 0;
  virtual Operand CopyOperand() = 0;
};

class RegOperand : public BasicOperand {
  int reg_no;
  RegOperand(int RegNo) {
    this->operandType = REG;
    this->reg_no = RegNo;
  }

public:
  int GetRegNo() { return reg_no; }
  friend RegOperand *GetNewRegOperand(int RegNo);
  virtual std::string GetFullName() { return "%r" + std::to_string(reg_no); }
  virtual Operand CopyOperand() { return new RegOperand(reg_no); }
};
RegOperand *GetNewRegOperand(int RegNo);

class ImmI32Operand : public BasicOperand {
  int immVal;

public:
  int GetIntImmVal() { return immVal; }
  ImmI32Operand(int immVal) {
    this->operandType = IMMI32;
    this->immVal = immVal;
  }
  virtual std::string GetFullName() { return std::to_string(immVal); }
  virtual Operand CopyOperand() { return new ImmI32Operand(immVal); }
};

class ImmI64Operand : public BasicOperand {
  long long immVal;

public:
  ImmI64Operand(long long immVal) {
    this->operandType = IMMI64;
    this->immVal = immVal;
  }
  virtual std::string GetFullName() { return std::to_string(immVal); }
  virtual Operand CopyOperand() { return new ImmI64Operand(immVal); }
};

class ImmF32Operand : public BasicOperand {
  float immVal;

public:
  float GetFloatVal() { return immVal; }
  long long GetFloatByteVal();
  ImmF32Operand(float immVal) {
    this->operandType = IMMF32;
    this->immVal = immVal;
  }
  virtual std::string GetFullName() { return std::to_string(immVal); }
  virtual Operand CopyOperand() { return new ImmF32Operand(immVal); }
};

class LabelOperand : public BasicOperand {
  int label_no;
  LabelOperand(int LabelNo) {
    this->operandType = LABEL;
    this->label_no = LabelNo;
  }

public:
  int GetLabelNo() { return label_no; }
  friend LabelOperand *GetNewLabelOperand(int LabelNo);
  virtual std::string GetFullName() { return "%L" + std::to_string(label_no); }
  virtual Operand CopyOperand() { return new LabelOperand(label_no); }
};

LabelOperand *GetNewLabelOperand(int RegNo);

class GlobalOperand : public BasicOperand {
  std::string name;
  GlobalOperand(std::string gloName) {
    this->operandType = GLOBAL;
    this->name = gloName;
  }

public:
  std::string GetName() { return name; }
  friend GlobalOperand *GetNewGlobalOperand(std::string name);
  virtual std::string GetFullName() { return "@" + name; }
  virtual Operand CopyOperand() { return new GlobalOperand(name); }
};

GlobalOperand *GetNewGlobalOperand(std::string name);

std::ostream &operator<<(std::ostream &s, LLVMType type);
std::ostream &operator<<(std::ostream &s, LLVMIROpcode type);
std::ostream &operator<<(std::ostream &s, IcmpCond type);
std::ostream &operator<<(std::ostream &s, FcmpCond type);
std::ostream &operator<<(std::ostream &s, Operand op);

class BasicInstruction;
typedef BasicInstruction *Instruction;

class BasicInstruction {
public:
  int BlockID = 0;

protected:
  LLVMIROpcode opcode;
  int insNo;

public:
  int GetOpcode() const { return opcode; }
  virtual void PrintIR(std::ostream &s) = 0;
};

class LoadInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand pointer;
  Operand result;

public:
  LoadInstruction(enum LLVMType type, Operand pointer, Operand result) {
    opcode = LLVMIROpcode::LOAD;
    this->type = type;
    this->result = result;
    this->pointer = pointer;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetPointer() const { return pointer; }
  Operand GetResult() const { return result; }

  void PrintIR(std::ostream &s) {
    // s<<"loadinstruction print\n";
    s << result->GetFullName() << " = load " << type << ", ptr "
      << pointer->GetFullName() << "\n";
  }
};

class StoreInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand pointer;
  Operand value;

public:
  StoreInstruction(enum LLVMType type, Operand pointer, Operand value) {
    opcode = LLVMIROpcode::STORE;
    this->type = type;
    this->pointer = pointer;
    this->value = value;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetPointer() const { return pointer; }
  Operand GetValue() const { return value; }

  // void PrintIR(std::ostream &s) {
  // // s << "storeinstruction print\n";
  //   s << "store " << type << " " << value->GetFullName() << ", ptr " <<
  //   pointer->GetFullName() << "\n";
  // }
  void PrintIR(std::ostream &s) {
    // 处理浮点立即数的特殊格式
    auto format_float_operand = [](Operand op) -> std::string {
      if (auto *imm_f32 = dynamic_cast<ImmF32Operand *>(op)) {
        // 浮点立即数：转换为十六进制格式
        float float_val = imm_f32->GetFloatVal();
        unsigned long long byte_val = Float_to_Byte(float_val);

        std::ostringstream oss;
        oss << "0x" << std::hex << byte_val << std::dec;
        return oss.str();
      } else if (auto *imm_i32 = dynamic_cast<ImmI32Operand *>(op)) {
        // 整数立即数：转换为浮点十六进制格式
        float float_val = static_cast<float>(imm_i32->GetIntImmVal());
        unsigned long long byte_val = Float_to_Byte(float_val);

        std::ostringstream oss;
        oss << "0x" << std::hex << byte_val << std::dec;
        return oss.str();
      } else {
        // 非立即数类型：保持原样
        return op->GetFullName();
      }
    };

    // 根据类型选择输出格式
    if (type == FLOAT32) {
      // 浮点类型：使用特殊格式输出
      s << "store float " << format_float_operand(value) << ", ptr "
        << pointer->GetFullName() << "\n";
    } else {
      // 非浮点类型：正常输出
      s << "store " << type << " " << value->GetFullName() << ", ptr "
        << pointer->GetFullName() << "\n";
    }
  }
};

class ArithmeticInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand op1;
  Operand op2;
  Operand op3;
  Operand result;

public:
  ArithmeticInstruction(LLVMIROpcode opcode, enum LLVMType type, Operand op1,
                        Operand op2, Operand result) {
    this->opcode = opcode;
    this->op1 = op1;
    this->op2 = op2;
    this->op3 = nullptr;
    this->result = result;
    this->type = type;
  }

  ArithmeticInstruction(LLVMIROpcode opcode, enum LLVMType type, Operand op1,
                        Operand op2, Operand op3, Operand result) {
    this->opcode = opcode;
    this->op1 = op1;
    this->op2 = op2;
    this->op3 = op3;
    this->result = result;
    this->type = type;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetOp1() const { return op1; }
  Operand GetOp2() const { return op2; }
  Operand GetOp3() const { return op3; }
  Operand GetResult() const { return result; }

  void PrintIR(std::ostream &s) {
    // s << "arithmeticinstruction print\n";
    if (opcode == LL_ADDMOD) {
      s << result->GetFullName() << " = call i32 @___llvm_ll_add_mod(i32 "
        << op1->GetFullName() << ",i32 " << op2->GetFullName() << ",i32 "
        << op3->GetFullName() << ")\n";
      return;
    } else if (opcode == UMIN_I32) {
      s << result->GetFullName() << " = call i32 @llvm.umin.i32(i32 "
        << op1->GetFullName() << ",i32 " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == UMAX_I32) {
      s << result->GetFullName() << " = call i32 @llvm.umax.i32(i32 "
        << op1->GetFullName() << ",i32 " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == SMIN_I32) {
      s << result->GetFullName() << " = call i32 @llvm.smin.i32(i32 "
        << op1->GetFullName() << ",i32 " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == SMAX_I32) {
      s << result->GetFullName() << " = call i32 @llvm.smax.i32(i32 "
        << op1->GetFullName() << ",i32 " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == FMIN_F32) {
      s << result->GetFullName() << " = call float @___llvm_fmin_f32(float "
        << op1->GetFullName() << ",float " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == FMAX_F32) {
      s << result->GetFullName() << " = call float @___llvm_fmax_f32(float "
        << op1->GetFullName() << ",float " << op2->GetFullName() << ")\n";
      return;
    } else if (opcode == FADD || opcode == FSUB || opcode == FMUL ||
               opcode == FDIV) {
      // 处理浮点运算指令
      auto format_float_operand = [](Operand op) -> std::string {
        if (auto *imm_op = dynamic_cast<ImmF32Operand *>(op)) {
          // 浮点立即数：转换为十六进制格式
          float float_val = imm_op->GetFloatVal();
          unsigned long long byte_val = Float_to_Byte(float_val);

          // 使用字符串流构建十六进制字符串
          std::ostringstream oss;
          oss << "0x" << std::hex << byte_val << std::dec;
          return oss.str();
        } else {
          // 非立即数或非浮点类型：保持原样
          return op->GetFullName();
        }
      };

      s << result->GetFullName() << " = " << opcode << " " << type << " "
        << format_float_operand(op1) << ", " << format_float_operand(op2)
        << "\n";
      return;
    }
    s << result->GetFullName() << " = " << opcode << " " << type << " "
      << op1->GetFullName() << "," << op2->GetFullName() << "\n";
  }
};

class IcmpInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand op1;
  Operand op2;
  IcmpCond cond;
  Operand result;

public:
  IcmpInstruction(enum LLVMType type, Operand op1, Operand op2, IcmpCond cond,
                  Operand result) {
    this->opcode = LLVMIROpcode::ICMP;
    this->type = type;
    this->op1 = op1;
    this->op2 = op2;
    this->cond = cond;
    this->result = result;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetOp1() const { return op1; }
  Operand GetOp2() const { return op2; }
  IcmpCond GetCond() const { return cond; }
  Operand GetResult() const { return result; }

  void PrintIR(std::ostream &s) {
    // s << "icmpinstruction print\n";
    s << result->GetFullName() << " = icmp " << cond << " " << type << " "
      << op1->GetFullName() << "," << op2->GetFullName() << "\n";
  }
};

class FcmpInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand op1;
  Operand op2;
  FcmpCond cond;
  Operand result;

public:
  FcmpInstruction(enum LLVMType type, Operand op1, Operand op2, FcmpCond cond,
                  Operand result) {
    this->opcode = LLVMIROpcode::FCMP;
    this->type = type;
    this->op1 = op1;
    this->op2 = op2;
    this->cond = cond;
    this->result = result;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetOp1() const { return op1; }
  Operand GetOp2() const { return op2; }
  FcmpCond GetCond() const { return cond; }
  Operand GetResult() const { return result; }

  void PrintIR(std::ostream &s) {
    // s << "fcmpinstruction print\n";
    s << result->GetFullName() << " = fcmp " << cond << " " << type << " "
      << op1->GetFullName() << "," << op2->GetFullName() << "\n";
  }
};

class SelectInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand op1;
  Operand op2;
  Operand cond;
  Operand result;

public:
  SelectInstruction(enum LLVMType t, Operand o1, Operand o2, Operand c,
                    Operand res) {
    this->opcode = LLVMIROpcode::SELECT;
    this->type = t;
    this->op1 = o1;
    this->op2 = o2;
    this->cond = c;
    this->result = res;
  }
  void PrintIR(std::ostream &s) {
    // s << "selectinstruction print\n";
    s << result->GetFullName() << " = ";
    s << "select i1 " << cond->GetFullName();
    s << ", " << type << " " << op1->GetFullName();
    s << ", " << type << " " << op2->GetFullName();
    s << "\n";
  }
};

class PhiInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand result;
  std::vector<std::pair<Operand, Operand>> phi_list;

public:
  PhiInstruction(enum LLVMType type, Operand result,
                 decltype(phi_list) val_labels) {
    this->opcode = LLVMIROpcode::PHI;
    this->type = type;
    this->result = result;
    this->phi_list = val_labels;
  }
  PhiInstruction(enum LLVMType type, Operand result) {
    this->opcode = LLVMIROpcode::PHI;
    this->type = type;
    this->result = result;
  }

  // Getter methods
  Operand GetResult() const { return result; }

  void PrintIR(std::ostream &s) {
    // s << "phiinstruction print\n";
    s << result->GetFullName() << " = phi " << type << " ";
    for (auto it = phi_list.begin(); it != phi_list.end(); ++it) {
      s << "[" << it->second->GetFullName() << "," << it->first->GetFullName()
        << "]";
      auto jt = it;
      if ((++jt) != phi_list.end())
        s << ",";
    }
    s << '\n';
  }
};

class AllocaInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand result;
  std::vector<int> dims;

public:
  AllocaInstruction(enum LLVMType dttype, Operand result) {
    this->opcode = LLVMIROpcode::ALLOCA;
    this->type = dttype;
    this->result = result;
  }
  AllocaInstruction(enum LLVMType dttype, std::vector<int> ArrDims,
                    Operand result) {
    this->opcode = LLVMIROpcode::ALLOCA;
    this->type = dttype;
    this->result = result;
    dims = ArrDims;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetResult() const { return result; }
  const std::vector<int> &GetDims() const { return dims; }

  void PrintIR(std::ostream &s) {
    // s << "allocainstruction print\n";
    s << result->GetFullName() << " = alloca ";
    if (dims.empty())
      s << type << "\n";
    else {
      for (std::vector<int>::iterator it = dims.begin(); it != dims.end(); ++it)
        s << "[" << *it << " x ";
      s << type << std::string(dims.size(), ']') << "\n";
    }
  }
};

class BrCondInstruction : public BasicInstruction {
public:
  Operand cond;
  Operand trueLabel;
  Operand falseLabel;

public:
  BrCondInstruction(Operand cond, Operand trueLabel, Operand falseLabel) {
    this->opcode = BR_COND;
    this->cond = cond;
    this->trueLabel = trueLabel;
    this->falseLabel = falseLabel;
  }

  // Getter methods
  Operand GetCond() const { return cond; }
  Operand GetTrueLabel() const { return trueLabel; }
  Operand GetFalseLabel() const { return falseLabel; }

  void PrintIR(std::ostream &s) {
    // s << "brcondinstruction print\n";
    s << "br i1 " << cond->GetFullName() << ", label "
      << trueLabel->GetFullName() << ", label " << falseLabel->GetFullName()
      << "\n";
  }
};

class BrUncondInstruction : public BasicInstruction {
public:
  Operand destLabel;

public:
  BrUncondInstruction(Operand destLabel) {
    this->opcode = BR_UNCOND;
    this->destLabel = destLabel;
  }

  // Getter methods
  Operand GetLabel() const { return destLabel; }

  void PrintIR(std::ostream &s) {
    // s << "bruncondinstruction print\n";
    s << "br label " << destLabel->GetFullName() << "\n";
  }
};

void recursive_print(std::ostream &s, LLVMType type, VarAttribute &v,
                     size_t dimDph, size_t beginPos, size_t endPos);
class GlobalVarDefineInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  std::string name;
  Operand init_val;
  VarAttribute arval;

  GlobalVarDefineInstruction(std::string nam, enum LLVMType typ, VarAttribute v)
      : type(typ), name(nam), init_val(nullptr), arval(v) {
    this->opcode = LLVMIROpcode::GLOBAL_VAR;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  std::string GetName() const { return name; }
  Operand GetInitVal() const { return init_val; }
  const VarAttribute &GetAttr() const { return arval; }

  void PrintIR(std::ostream &s) {
    if (arval.dims.empty()) {
      if (type == I32 && !arval.IntInitVals.empty()) {
        s << "@" << name << " = global i32 " << arval.IntInitVals[0] << "\n";
      } else if (type == FLOAT32 && !arval.FloatInitVals.empty()) {
        s << "@" << name << " = global float 0x" << std::hex
          << Float_to_Byte(arval.FloatInitVals[0]) << std::dec << "\n";
      } else {
        s << "@" << name << " = global " << type << " zeroinitializer\n";
      }
      return;
    }
    s << "@" << name << " = global ";
    size_t step = 1;
    for (size_t i = 0; i < arval.dims.size(); i++) {
      step *= arval.dims[i];
    }
    recursive_print(s, type, arval, 0, 0, step - 1);
    s << "\n";
  }
};

class GlobalStringConstInstruction : public BasicInstruction {
public:
  std::string str_val;
  std::string str_name;
  GlobalStringConstInstruction(std::string strval, std::string strname)
      : str_val(strval), str_name(strname) {
    this->opcode = LLVMIROpcode::GLOBAL_STR;
  }

  // Getter methods
  std::string GetValue() const { return str_val; }
  std::string GetName() const { return str_name; }

  void PrintIR(std::ostream &s) {
    // s << "globalstringconstinstruction print\n";
    size_t str_len = str_val.size() + 1;
    for (char c : str_val) {
      if (c == '\\')
        str_len--;
    }
    s << "@" << str_name << " = public unnamed_addr constant [" << str_len
      << " x i8] c\"";
    for (size_t i = 0; i < str_val.size(); i++) {
      char c = str_val[i];
      if (c == '\\') {
        i++;
        c = str_val[i];
        if (c == 'n')
          s << "\\0A";
        else if (c == 't')
          s << "\\09";
        else if (c == '\"')
          s << "\\22";
        else if (c == 'r')
          s << "\\0D";
        else if (c == 'b')
          s << "\\08";
        else if (c == 'f')
          s << "\\0C";
        else if (c == 'v')
          s << "\\0B";
        else if (c == 'a')
          s << "\\07";
        else if (c == '?')
          s << "\?";
        else if (c == '0')
          s << "\\00";
        else if (c == '\'')
          s << "\'";
        else if (c == '\\')
          s << "\\\\";
        else
          s << c;
      } else
        s << c;
    }
    s << "\\00"
      << "\"\n";
  }
};

class CallInstruction : public BasicInstruction {
public:
  enum LLVMType ret_type;
  Operand result;
  std::string name;
  std::vector<std::pair<enum LLVMType, Operand>> args;

public:
  CallInstruction(enum LLVMType retType, Operand res, std::string FuncName,
                  std::vector<std::pair<enum LLVMType, Operand>> arguments)
      : ret_type(retType), result(res), name(FuncName), args(arguments) {
    this->opcode = CALL;
    if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
      if (((RegOperand *)res)->GetRegNo() == -1) {
        result = NULL;
      }
    }
  }
  CallInstruction(enum LLVMType retType, Operand res, std::string FuncName)
      : ret_type(retType), result(res), name(FuncName) {
    this->opcode = CALL;
    if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
      if (((RegOperand *)res)->GetRegNo() == -1) {
        result = NULL;
      }
    }
  }

  // Getter methods
  LLVMType GetType() const { return ret_type; }
  Operand GetResult() const { return result; }
  std::string GetFuncName() const { return name; }
  const std::vector<std::pair<enum LLVMType, Operand>> &GetArgs() const {
    return args;
  }

  // void PrintIR(std::ostream &s) {
  //   // s << "callinstruction print\n";
  //   if (ret_type != LLVMType::VOID_TYPE) {
  //     s << result->GetFullName() << " = ";
  //   }
  //   s << "call " << ret_type << " @" << name;
  //   s << "(";
  //   for (auto it = args.begin(); it != args.end(); ++it) {
  //     s << it->first << " " << it->second->GetFullName();
  //     if (it + 1 != args.end())
  //       s << ",";
  //   }
  //   s << ")\n";
  // }
  void PrintIR(std::ostream &s) {
    // 辅助lambda函数：根据目标类型格式化操作数
    auto format_operand = [](Operand op, LLVMType target_type) -> std::string {
      if (target_type == FLOAT32) {
        float float_val = 0.0f;

        // 处理浮点立即数
        if (auto *imm_f32 = dynamic_cast<ImmF32Operand *>(op)) {
          float_val = imm_f32->GetFloatVal();
        }
        // 处理整数立即数（整数转浮点）
        else if (auto *imm_i32 = dynamic_cast<ImmI32Operand *>(op)) {
          float_val = static_cast<float>(imm_i32->GetIntImmVal());
        }
        // 其他类型保持原样
        else {
          return op->GetFullName();
        }

        // 转换为十六进制格式
        unsigned long long byte_val = Float_to_Byte(float_val);
        std::ostringstream oss;
        oss << "0x" << std::hex << byte_val << std::dec;
        return oss.str();
      } else {
        // 非浮点类型：保持原样
        return op->GetFullName();
      }
    };

    // 输出结果寄存器（如果有）
    if (ret_type != LLVMType::VOID_TYPE) {
      s << result->GetFullName() << " = ";
    }

    // 输出函数调用指令
    s << "call " << ret_type << " @" << name << "(";

    // 遍历并输出所有参数
    for (auto it = args.begin(); it != args.end(); ++it) {
      s << it->first << " ";

      // 特殊处理浮点类型参数
      s << format_operand(it->second, it->first);

      if (it + 1 != args.end()) {
        s << ", ";
      }
    }
    s << ")\n";
  }
};

class RetInstruction : public BasicInstruction {
public:
  enum LLVMType ret_type;
  Operand ret_val;

public:
  RetInstruction(enum LLVMType retType, Operand res)
      : ret_type(retType), ret_val(res) {
    this->opcode = RET;
  }

  // Getter methods
  LLVMType GetType() const { return ret_type; }
  Operand GetRetVal() const { return ret_val; }

  void PrintIR(std::ostream &s) {
    // s << "retinstruction print\n";
    s << "ret " << ret_type;
    if (ret_val != nullptr) {
      s << " " << ret_val->GetFullName();
    }
    s << "\n";
  }
};

class GetElementptrInstruction : public BasicInstruction {
public:
  enum LLVMType type;
  Operand result;
  Operand ptrval;
  std::vector<int> dims;
  std::vector<Operand> indexes;

public:
  GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr)
      : type(typ), result(res), ptrval(ptr) {
    opcode = GETELEMENTPTR;
  }
  GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr,
                           std::vector<int> dim)
      : type(typ), result(res), ptrval(ptr), dims(dim) {
    opcode = GETELEMENTPTR;
  }
  GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr,
                           std::vector<int> dim, std::vector<Operand> index)
      : type(typ), result(res), ptrval(ptr), dims(dim), indexes(index) {
    opcode = GETELEMENTPTR;
  }

  // Getter methods
  LLVMType GetType() const { return type; }
  Operand GetResult() const { return result; }
  Operand GetPtrVal() const { return ptrval; }
  const std::vector<int> &GetDims() const { return dims; }
  const std::vector<Operand> &GetIndexes() const { return indexes; }

  void PrintIR(std::ostream &s) {
    // s << "getelementptrinstruction print\n";
    s << result->GetFullName() << " = getelementptr ";
    if (dims.empty())
      s << type;
    else {
      for (int dim : dims) {
        s << "[" << dim << " x ";
      }
      s << type;
      s << std::string(dims.size(), ']');
    }
    s << ", ptr " << ptrval->GetFullName();
    for (Operand idx : indexes)
      s << ", i32 " << idx->GetFullName();
    s << "\n";
  }
};

class FunctionDefineInstruction : public BasicInstruction {
public:
  enum LLVMType return_type;
  std::string Func_name;

public:
  std::vector<enum LLVMType> formals;
  std::vector<Operand> formals_reg;
  FunctionDefineInstruction(enum LLVMType t, std::string n) {
    return_type = t;
    Func_name = n;
  }
  void InsertFormal(enum LLVMType t) {
    formals.push_back(t);
    formals_reg.push_back(GetNewRegOperand(formals_reg.size()));
  }
  void PrintIR(std::ostream &s) {
    // s << "functiondefineinstruction print\n";
    s << "define " << return_type << " @" << Func_name;
    s << "(";
    for (uint32_t i = 0; i < formals.size(); ++i) {
      s << formals[i] << " " << formals_reg[i]->GetFullName();
      if (i + 1 < formals.size()) {
        s << ",";
      }
    }
    s << ")\n";
  }
};
typedef FunctionDefineInstruction *FuncDefInstruction;

class FunctionDeclareInstruction : public BasicInstruction {
public:
  enum LLVMType return_type;
  std::string Func_name;
  bool is_more_args = false;

public:
  std::vector<enum LLVMType> formals;
  FunctionDeclareInstruction(enum LLVMType t, std::string n,
                             bool is_more = false) {
    return_type = t;
    Func_name = n;
    is_more_args = is_more;
  }
  void InsertFormal(enum LLVMType t) { formals.push_back(t); }
  void PrintIR(std::ostream &s) {
    // s << "functiondeclareinstruction print\n";
    s << "declare " << return_type << " @" << Func_name << "(";
    for (uint32_t i = 0; i < formals.size(); ++i) {
      s << formals[i];
      if (i + 1 < formals.size()) {
        s << ",";
      }
    }
    if (is_more_args) {
      s << ", ...";
    }
    s << ")\n";
  }
};

class FptosiInstruction : public BasicInstruction {
public:
  Operand result;
  Operand value;

public:
  FptosiInstruction(Operand result_receiver, Operand value_for_cast)
      : result(result_receiver), value(value_for_cast) {
    this->opcode = FPTOSI;
  }
  void PrintIR(std::ostream &s) {
    // s << "fptosiinstruction print\n";
    s << result->GetFullName() << " = fptosi float"
      << " " << value->GetFullName() << " to "
      << "i32"
      << "\n";
  }
};

class FpextInstruction : public BasicInstruction {
public:
  Operand result;
  Operand value;

public:
  FpextInstruction(Operand result_receiver, Operand value_for_cast)
      : result(result_receiver), value(value_for_cast) {
    this->opcode = FPEXT;
  }
  void PrintIR(std::ostream &s) {
    // s << "fpextinstruction print\n";
    s << result->GetFullName() << " = fpext float"
      << " " << value->GetFullName() << " to "
      << "double"
      << "\n";
  }
};

class BitCastInstruction : public BasicInstruction {
public:
  Operand src;
  Operand dst;
  LLVMType src_type;
  LLVMType dst_type;

public:
  BitCastInstruction(Operand src_op, Operand dst_op, LLVMType src_t,
                     LLVMType dst_t)
      : src(src_op), dst(dst_op), src_type(src_t), dst_type(dst_t) {
    this->opcode = BITCAST;
  }
  void PrintIR(std::ostream &s) {
    // s << "bitcastinstruction print\n";
    s << dst->GetFullName() << " = bitcast " << src_type << " "
      << src->GetFullName() << " to " << dst_type << "\n";
  }
};

class SitofpInstruction : public BasicInstruction {
public:
  Operand result;
  Operand value;

public:
  SitofpInstruction(Operand result_receiver, Operand value_for_cast)
      : result(result_receiver), value(value_for_cast) {
    this->opcode = SITOFP;
  }
  void PrintIR(std::ostream &s) {
    // s << "sitofpinstruction print\n";
    s << result->GetFullName() << " = sitofp i32"
      << " " << value->GetFullName() << " to "
      << "float"
      << "\n";
  }
};

class ZextInstruction : public BasicInstruction {
public:
  LLVMType from_type;
  LLVMType to_type;
  Operand result;
  Operand value;

public:
  ZextInstruction(LLVMType to_type, Operand result_receiver, LLVMType from_type,
                  Operand value_for_cast)
      : from_type(from_type), to_type(to_type), result(result_receiver),
        value(value_for_cast) {
    this->opcode = ZEXT;
  }
  void PrintIR(std::ostream &s) {
    // s << "zextinstruction print\n";
    s << result->GetFullName() << " = zext " << from_type << " "
      << value->GetFullName() << " to " << to_type << "\n";
  }
};

class TruncInstruction : public BasicInstruction {
public:
  LLVMType from_type;
  LLVMType to_type;
  Operand result;
  Operand value;

public:
  TruncInstruction(LLVMType from_type, Operand value_for_cast, LLVMType to_type,
                   Operand result_receiver)
      : from_type(from_type), to_type(to_type), result(result_receiver),
        value(value_for_cast) {
    this->opcode = TRUNC;
  }
  void PrintIR(std::ostream &s) {
    s << result->GetFullName() << " = trunc " << from_type << " "
      << value->GetFullName() << " to " << to_type << "\n";
  }
};
