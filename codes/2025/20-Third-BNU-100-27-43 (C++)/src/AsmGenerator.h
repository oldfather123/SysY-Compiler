#ifndef ASM_GENERATOR_H
#define ASM_GENERATOR_H

#include "IR.h"
#include <string>
#include <map>
#include <vector>
#include <set>
#include <sstream>

class AsmGenerator {
public:
    std::string generate(const MyIR::IRUnit &unit);
private:
    void genGlobal(const MyIR::GlobalVariable *gv, std::stringstream &ss);
    void genFunction(const MyIR::Function *func, std::stringstream &ss);
    void genBasicBlock(const MyIR::BasicBlock *bb, std::stringstream &ss);
    void genInstruction(const MyIR::Instruction *inst, std::stringstream &ss);
    void genArrayInitializer(const std::shared_ptr<MyIR::ConstantArray>& array_init, std::stringstream& ss);

    // 寄存器分配和临时变量管理
    std::set<std::string> used_regs_;              // 当前正在使用的寄存器
    std::map<const MyIR::Value*, std::string> value2reg_;  // 值到寄存器的映射
    std::map<std::string, const MyIR::Value*> reg2value_;  // 寄存器到值的映射
    std::vector<std::string> avail_temp_regs_;     // 可用的临时寄存器列表
    std::vector<std::string> avail_float_regs_;    // 可用的浮点寄存器列表
    std::map<const MyIR::Value*, int> spill_locations_; // 新增：溢出位置映射
    std::map<const MyIR::Argument*, int> arg_offset; // 函数调用时，存储在栈中的参数的偏移量
    std::map<const MyIR::Argument*, int> float_arg_idx;
    std::map<const MyIR::Argument*, int> int_arg_idx;
    int reg_counter_ = 0;
    int temp_reg_counter_ = 0;     // 整数临时寄存器计数器
    int float_reg_counter_ = 0;    // 浮点临时寄存器计数器
    void initRegisterPool();
    std::string allocTempReg(bool is_float = false);
    std::string getReg(const MyIR::Value* val);
    void freeReg(const std::string& reg);
    std::string spillRegister(bool is_float);
    void saveActiveTempRegs(std::stringstream &ss);
    void restoreActiveTempRegs(std::stringstream &ss);
    int caller_save_offset_ = 0;                            // 用于保存临时寄存器的栈偏移

    // 栈帧管理
    std::map<const MyIR::Value *, int> stack_offset_;
    int stack_size_ = 0;
    void allocStack(const MyIR::Function *func);
    int arg_stack_size = 0;

    // 辅助
    std::string getLabel(const MyIR::BasicBlock *bb);
    std::string getGlobalName(const MyIR::GlobalVariable *gv);
    void genMemset(const MyIR::CallInst *call, std::stringstream &ss);
    std::string getArrayElementAddr(const MyIR::GetElementPtrInst *gep, std::stringstream &ss);

    std::map<const MyIR::Value*, size_t> array_base_;  // 数组基地址映射
    size_t getTypeSize(std::shared_ptr<MyIR::Type>);
    void genArrayAccess(const MyIR::GetElementPtrInst* gep, std::stringstream& ss);
    std::vector<size_t> getArrayDimensions(const MyIR::GetElementPtrInst* gep);

    // 添加成员变量跟踪浮点常量
    std::map<double, std::string> float_constants_;
    int float_const_counter_ = 0;
    
    // 添加获取浮点常量标签的辅助函数
    std::string getFloatConstLabel(double value) {
        auto it = float_constants_.find(value);
        if (it != float_constants_.end()) {
            return it->second;
        }
        std::string label = ".LC" + std::to_string(float_const_counter_++);
        float_constants_[value] = label;
        return label;
    }
};

#endif