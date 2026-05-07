#pragma once

#include "ASMInstruction.hpp"
#include "Module.hpp"
#include "Register.hpp"
#include <unordered_map>


#define STACK_ALIGN(x) ALIGN(x, 16)

class CodeGen {
  public:
    explicit CodeGen(Module *module) : m(module) {}

    std::string print() const;

    void run();

    void append_inst(const ASMInstruction &inst) {
        output.push_back(inst);
    }

    void append_inst(const char *inst, std::initializer_list<std::string> args,
            ASMInstruction::InstType ty = ASMInstruction::Instruction) {
        std::string content = inst;
        if (args.size() > 0) {
            content += " ";
            for (auto it = args.begin(); it != args.end(); ) {
                content += *it;
                if (++it != args.end()) {
                    content += ", ";
                }
            }
        }

        switch (ty) {
            case ASMInstruction::Instruction:
                output.push_back(ASMInstruction::makeInstruction(content));
                break;
            case ASMInstruction::Attribute:
                output.push_back(ASMInstruction::makeAttribute(content));
                break;
            case ASMInstruction::Label:
                output.push_back(ASMInstruction::makeLabel(content));
                break;
            case ASMInstruction::Comment:
                output.push_back(ASMInstruction::makeComment(content));
                break;
            default:
                throw std::runtime_error("Unknown instruction type");
        }
    }

  private:
    void allocate();// 寄存器分配 + 栈帧布局
    void copy_stmt();// 处理 phi 结点的 copy 操作
    void gen_phi_copy(BasicBlock *target_bb);

    // 向寄存器中装载数据
    void load_to_greg(Value *, const Reg &);// 加载数据到通用寄存器（整数）
    void load_to_freg(Value *, const FReg &);// 加载数据到浮点寄存器
    void load_from_stack_to_greg(Value *, const Reg &);

    // 向寄存器中加载立即数
    void load_large_int32(int32_t imm, const Reg &);
    void load_large_int64(int64_t imm, const Reg &);
    void load_float_imm(float val, const FReg &);

    // 将寄存器中的数据保存回栈上
    void store_from_greg(Value *, const Reg &);// 从寄存器保存回内存
    void store_from_freg(Value *, const FReg &);

    // ========== 指令生成 ==========
    void gen_prologue();      // 函数序言
    void gen_ret();           // return
    void gen_br();            // 分支跳转
    void gen_binary();        // 整型二元运算（add/sub/...）
    void gen_float_binary();  // 浮点运算（fadd/fsu/...）
    void gen_alloca();        // 分配空间（stack frame）
    void gen_load();          // load 指令
    void gen_store();         // store 指令
    void gen_icmp();          // 整数比较
    void gen_fcmp();          // 浮点比较
    void gen_zext();          // 零扩展
    void gen_call();          // 函数调用
    void gen_gep();           // GEP 指令
    void gen_sitofp();        // 整转浮点
    void gen_fptosi();        // 浮点转整
    void gen_epilogue();      // 函数结尾
    void gen_initializer(Constant *init); // 递归生成初始化数据

    // ========== 标签名生成 ==========
    std::string label_name(BasicBlock *bb) {
        // Check if we already have a unique label for this basic block
        auto it = bb_label_map.find(bb);
        if (it != bb_label_map.end()) {
            return it->second;
        }
        
        // Generate a new unique label
        std::string base_label = "." + bb->get_parent()->get_name() + "_" + bb->get_name();
        std::string unique_label = base_label;
        
        // If this label already exists, append a counter to make it unique
        auto label_it = used_labels.find(base_label);
        if (label_it != used_labels.end()) {
            unique_label = base_label + "_" + std::to_string(label_it->second);
            label_it->second++;
        } else {
            used_labels[base_label] = 1;
        }
        
        // Cache the label for this basic block
        bb_label_map[bb] = unique_label;
        return unique_label;
    }

    static std::string func_exit_label_name(Function *func) {
        return func->get_name() + "_exit";
    }

    static std::string fcmp_label_name(BasicBlock *bb, unsigned cnt) {
        return "." + bb->get_parent()->get_name() + "_" + bb->get_name() + "_fcmp_" + std::to_string(cnt);
    }

    // ========== 当前上下文 ==========
    struct {
        /* 随着ir遍历设置 */
        Function *func{nullptr};    // 当前函数
        BasicBlock *bb{nullptr};    // 当前基本块
        Instruction *inst{nullptr}; // 当前指令
        /* 在allocate()中设置 */
        unsigned frame_size{0}; // 当前函数的栈帧大小
        std::unordered_map<Value *, int> offset_map; // 指针相对 fp 的偏移
        std::unordered_map<AllocaInst*, int> alloca_offset_map; // Offsets for alloca'd memory blocks
        unsigned fcmp_cnt{0}; // fcmp 的计数器, 用于创建 fcmp 需要的 label
         std::unordered_map<AllocaInst*, unsigned> array_start_offset;

        void clear() {
            func = nullptr;
            bb = nullptr;
            inst = nullptr;
            frame_size = 0;
            fcmp_cnt = 0;
            offset_map.clear();
            array_start_offset.clear(); 
            alloca_offset_map.clear();
            
        }
    } context;

    Module *m;
    std::list<ASMInstruction> output;
    
   
    std::unordered_map<BasicBlock*, std::string> bb_label_map;
    std::unordered_map<std::string, int> used_labels;
};
