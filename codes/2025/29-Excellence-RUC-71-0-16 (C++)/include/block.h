#pragma once

#include "llvm_instruction.h"
#include <deque>
#include <iostream>
#include <set>
#include <vector>

class BasicBlock {
public:
  std::string comment; // used for debug
  int block_id = 0;
  std::deque<Instruction> Instruction_list{};

  void InsertInstruction(int pos, Instruction Ins);

  void printIR(std::ostream &s) {
    s << "L" << block_id << ":  " << comment << "\n";

    for (Instruction ins : Instruction_list) {
      s << "    ";
      ins->PrintIR(s); // Auto "\n" In Instruction::printIR()
    }
  }
  BasicBlock(int id) : block_id(id) {}
};
typedef BasicBlock *LLVMBlock;

class LLVMIR {
public:
  std::vector<Instruction> global_def{};
  std::vector<Instruction> function_declare{};
  std::map<FuncDefInstruction, std::map<int, LLVMBlock>>
      function_block_map; //<function,<id,block> >

  void NewFunction(FuncDefInstruction I) { function_block_map[I] = {}; }
  LLVMBlock GetBlock(FuncDefInstruction I, int now_label) {
    return function_block_map[I][now_label];
  }
  LLVMBlock NewBlock(FuncDefInstruction I, int &max_label) {
    ++max_label;
    function_block_map[I][max_label] = new BasicBlock(max_label);
    return GetBlock(I, max_label);
  }
  bool IsBr(Instruction ins) {
    int opcode = ins->GetOpcode();
    return opcode == BR_COND || opcode == BR_UNCOND;
  }

  bool IsRet(Instruction ins) {
    int opcode = ins->GetOpcode();
    return opcode == RET;
  }

  void printIR(std::ostream &s) {
    // output lib func decl
    for (Instruction lib_func_decl : function_declare) {
      lib_func_decl->PrintIR(s);
    }

    // output global
    for (Instruction global_decl_ins : global_def) {
      global_decl_ins->PrintIR(s);
    }

    // output Functions
    for (auto Func_Block_item : function_block_map) { //<function,<id,block> >
      FuncDefInstruction f = Func_Block_item.first;
      // output function Syntax
      f->PrintIR(s);

      // output Blocks in functions
      s << "{\n";
      auto &block_map = Func_Block_item.second;
      for (auto it = block_map.begin(); it != block_map.end(); ++it) {
        LLVMBlock block = it->second;
        // Check if block is empty
        if (block->Instruction_list.empty() ||
            (!IsRet(block->Instruction_list.back()) &&
             !IsBr(block->Instruction_list.back()))) {
          // Find the next block
          auto next_it = std::next(it);
          if (next_it != block_map.end()) {
            // Insert unconditional branch to the next block
            int next_label = next_it->first;
            block->InsertInstruction(
                1, new BrUncondInstruction(GetNewLabelOperand(next_label)));
          } else {
            // If this is the last block, insert a return instruction based on
            // return type
            if (f->return_type == VOID_TYPE) {
              block->InsertInstruction(
                  1, new RetInstruction(LLVMType::VOID_TYPE, nullptr));
            } else if (f->return_type == I32) {
              block->InsertInstruction(
                  1, new RetInstruction(LLVMType::I32, new ImmI32Operand(0)));
            } else if (f->return_type == FLOAT32) {
              block->InsertInstruction(
                  1, new RetInstruction(LLVMType::FLOAT32,
                                        new ImmF32Operand(0.0f)));
            }
          }
        }
        block->printIR(s);
      }
      s << "}\n";
    }
  }
};

class IRgenerator : public ASTVisitor {
public:
  LLVMIR llvmIR;
  Operand current_ptr;
  LLVMType current_llvm_type;
  BaseType current_type;
  int current_reg_counter = -1;
  int max_label = 0;
  int now_label = 1;
  int before_label = 1;
  int cond_label = 1;
  int body_label = 1;
  int end_label = 1;
  // std::map<std::string, int> function_name_to_maxreg;
  //  std::vector<std::vector<std::pair<std::string, int>>> scope_restore_list;
  //    std::vector<std::map<std::string, int>> scope_name_to_reg_stack;

private:
  bool require_address = false; // Flag to indicate if address is needed
public:
  LLVMIR &getLLVMIR() { return llvmIR; }
  void AddLibFunctionDeclare();
  LLVMBlock getCurrentBlock();
  int newReg();
  int newLabel();
  bool isGlobalScope();
  bool isPointer(int reg);
  void flattenInitVal(InitVal *init,
                      std::vector<std::variant<int, float>> &result,
                      BaseType type);
  std::optional<int> evaluateConstExpression(Exp *expr, int isdef);
  std::optional<float> evaluateConstExpressionFloat(Exp *expr);
  std::optional<double> evaluateGlobalInitializer(InitVal *init);
  std::shared_ptr<Type> inferExpressionType(Exp *expression);
  // void handleArrayInitializer(InitVal *init, int base_reg, VarAttribute
  // &attr, const std::vector<int> &dims, size_t dim_idx);
  //  void handleArrayInitializer(InitVal* init, int base_reg, VarAttribute&
  //  attr, const std::vector<int>& dims);
  void handleArrayInitializer(InitVal *init, int base_reg, VarAttribute &attr,
                              const std::vector<int> &dims, size_t dim_idx,
                              size_t &current_index);
  void handleArrayInitializer(ConstInitVal *init, int base_reg,
                              VarAttribute &attr, const std::vector<int> &dims,
                              size_t dim_idx, size_t &current_index);
  void flattenConstInit(ConstInitVal *init,
                        std::vector<std::variant<int, float>> &result,
                        BaseType type, const std::vector<int> &dims);
  void flattenInitVal(InitVal *init,
                      std::vector<std::variant<int, float>> &result,
                      BaseType type, const std::vector<int> &dims);
  void IRgenGetElementptr(LLVMBlock B, LLVMType type, int result_reg,
                          Operand ptr, std::vector<int> dims,
                          std::vector<Operand> indices);
  int convertToType(LLVMBlock B, int src_reg, LLVMType target_type);
  void IRgenIcmp(LLVMBlock B, IcmpCond cmp_op, int reg1, int reg2,
                 int result_reg);
  void IRgenFcmp(LLVMBlock B, FcmpCond cmp_op, int reg1, int reg2,
                 int result_reg);
  void IRgenIcmpImmRight(LLVMBlock B, IcmpCond cmp_op, int reg1, int val2,
                         int result_reg);
  void IRgenFcmpImmRight(LLVMBlock B, FcmpCond cmp_op, int reg1, float val2,
                         int result_reg);
  void IRgenArithmeticI32ImmRight(LLVMBlock B, LLVMIROpcode opcode, int reg1,
                                  int val2, int result_reg);
  void IRgenArithmeticI32(LLVMBlock B, LLVMIROpcode opcode, int reg1, int reg2,
                          int result_reg);
  void IRgenArithmeticF32(LLVMBlock B, LLVMIROpcode opcode, int reg1, int reg2,
                          int result_reg);
  void IRgenArithmeticI32ImmLeft(LLVMBlock B, LLVMIROpcode opcode, int val1,
                                 int reg2, int result_reg);
  void IRgenArithmeticF32ImmLeft(LLVMBlock B, LLVMIROpcode opcode, float val1,
                                 int reg2, int result_reg);
  void IRgenStore(LLVMBlock B, LLVMType type, int value_reg, Operand ptr);
  void IRgenStore(LLVMBlock B, LLVMType type, Operand value, Operand ptr);
  void IRgenBRUnCond(LLVMBlock B, int dst_label);
  void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label);
  void IRgenAlloca(LLVMType type, int reg);
  void IRgenLoad(LLVMBlock B, LLVMType type, int result_reg, Operand ptr);
  void visit(CompUnit &node) override;
  void visit(ConstDecl &node) override;
  void visit(ConstDef &node) override;
  void visit(VarDecl &node) override;
  void visit(VarDef &node) override;
  void visit(FuncDef &node) override;
  void visit(FuncFParam &node) override;
  void visit(Block &node) override;
  void visit(IfStmt &node) override;
  void visit(WhileStmt &node) override;
  void visit(ExpStmt &node) override;
  void visit(AssignStmt &node) override;
  void visit(ReturnStmt &node) override;
  void visit(BreakStmt &node) override;
  void visit(ContinueStmt &node) override;
  void visit(UnaryExp &node) override;
  void visit(BinaryExp &node) override;
  void visit(LVal &node) override;
  void visit(FunctionCall &node) override;
  void visit(Number &node) override;
  void visit(StringLiteral &node) override;
  void visit(InitVal &node) override;
  void visit(ConstInitVal &node) override;
};