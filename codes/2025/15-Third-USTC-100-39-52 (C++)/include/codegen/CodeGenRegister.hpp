#pragma once

#include "ASMInstruction.hpp"
#include "Module.hpp"
#include "Register.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "IRprinter.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "Value.hpp"
#include "liverange.hpp"
#include "regalloc.hpp"
#include <functional>
#include <unordered_map>

// #define STACK_ALIGN(x) (((x / 16) + (x % 16 ? 1 : 0)) * 16)
#define STACK_ALIGN(x) ALIGN(x, 16)
#define CONST_0 ConstantInt::get(0, m)
#define FP "fp"
#define SP "sp"
#define RA_reg "ra"
// // #a = 8, #t = 7, reserve $t0, $t1, $t6 for temporary
// #define R_USABLE (15 - 3)
// // #fa = 8, #ft=12, reserve $ft0, $ft1 for temporary
// #define FR_USABLE (20 - 2)
// #a = 8, #t = 7, reserve $t0, $t1, $t6 for temporary and s=11(不考虑s0)

#define R_USABLE (15 - 3 + 11)
// #fa = 8, #ft=12, reserve $ft0, $ft1 for temporary and fs=12
#define FR_USABLE (20 - 2 + 12)
#define ARG_R 8

class CodeGenRegister {
  public:
    explicit CodeGenRegister(Module *module) 
        : m(module), LRA(module, phi_map), RA_int(R_USABLE, false), RA_float(FR_USABLE, true) {}

    std::string print() const;

    void run();

    template <class... Args> void append_inst(Args... arg) {
        output.emplace_back(arg...);
    }

    // void
    // append_inst(const char *inst, std::initializer_list<std::string> args,
    //             ASMInstruction::InstType ty = ASMInstruction::Instruction) {
    //     auto content = std::string(inst) + " ";
    //     for (const auto &arg : args) {
    //         content += arg + ", ";
    //     }
    //     content.pop_back();
    //     content.pop_back();
    //     output.emplace_back(content, ty);
    // }
    void append_inst(const char *inst, std::initializer_list<std::string> args,
                 ASMInstruction::InstType ty = ASMInstruction::Instruction) {
    std::string content = inst;

    // 指令名称，用于识别需要特殊格式的类型
    static const std::set<std::string> mem_insts = {
        "lw", "sw", "ld", "sd", "flw", "fsw"
    };

    // 如果是内存指令且参数是 reg, base, offset → 处理成 reg, offset(base)
    if (mem_insts.count(inst) && args.size() == 3) {
        auto it = args.begin();
        std::string dst = *it++;
        std::string base = *it++;
        std::string offset = *it++;
        content += " " + dst + ", " + offset + "(" + base + ")";
    } else {
        content += " ";
        for (const auto &arg : args) {
            content += arg + ", ";
        }
        if (args.size() > 0) {
            content.pop_back();  // 去掉最后的空格
            content.pop_back();  // 去掉最后的逗号
        }
    }

    output.emplace_back(content, ty);
}





  private:
    //TODO: 添加你需要的函数
    void getPhiMap();
    __attribute__((warn_unused_result)) string value2reg(Value *, int i = 0, string = "",bool load_if_missing = true);
    void copy_stmt(); 
    void pass_arguments(CallInst *instr, int stack_arg_base);

    void resolve_and_emit_moves(const std::vector<std::pair<std::string, std::string>> &moves,
    const std::map<std::string, bool> &is_float_map);

    void makeSureInRange(string instr_ir,
                         string reg1,
                         string reg2,
                         int imm,
                         string tinstr,
                         int tid = 0,
                         int bits = 12,
                         bool u = false){
        /* this function will tranfser
            * `addi.d $a0, $fp, imm` to `add.d $a0, $fp, tmp_ireg` if imm
            * overfloats for `addi.d`. During the time we move `imm` to `tmp_ireg`,
            * another tmp ireg will be used, they can be same, but we must specify
            * it.
            */
        auto treg = tmpregname(tid, false);
        assert(treg != reg2 && "it's possible to write tid before reg2's use");
        auto [l, h] = immRange(bits, u);
        if (l <= imm and imm <= h){
            append_inst(instr_ir.c_str(),{reg1,reg2,to_string(imm)});
        } else {
            assert(value2reg(ConstantInt::get(imm, m), tid, treg) == treg);
            append_inst(tinstr.c_str(),{reg1,reg2,treg});
        }
    }
    // 通用内存操作类型，支持整型与浮点型的多种 load/store
    enum class MemOpType {
        LoadByte,     // lb
        LoadHalf,     // lh
        LoadInt,      // lw
        LoadDouble,   // ld (RV64)
        LoadFloat,    // flw
        StoreByte,    // sb
        StoreHalf,    // sh
        StoreInt,     // sw
        StoreDouble,  // sd (RV64)
        StoreFloat,   // fsw
    };
    //处理全局变量
    void emitGlobalVariables();
    void emitIntArray(ConstantArray *arr);
    void emitFloatArray(ConstantArray *arr);

    //进行copy操作
    bool gencopy(Value *lhs, string rhs_reg);
    void gencopy(string lhs_reg, string rhs_reg, bool is_float);
    //处理指针情况
    //void ptrContent2reg(Value *, string);
    pair<string, bool> getRegName(Value *, int = 0) const;
    string bool2branch(Instruction *);
    string getMemAddr(Value *val, const Reg &baseReg = Reg::fp());

    void mem_op(Value *val, const string &regName, MemOpType op,bool is_store_from_=false);

    // 向寄存器中装载数据
    void load_to_greg(Value *, const Reg &);
    void load_to_freg(Value *, const FReg &);
    void load_from_stack_to_greg(Value *, const Reg &);
    void load_to_greg_string(Value *val, const std::string &reg);
    void load_to_freg_string(Value *val, const std::string &freg);
    // 向寄存器中加载立即数
    void load_large_int32(int32_t, const Reg &);
    void load_large_int64(int64_t, const Reg &);
    void load_float_imm(float, const FReg &);

    // 将寄存器中的数据保存回栈上
    void store_from_greg(Value *, const Reg &);
    void store_from_freg(Value *, const FReg &);
    void store_from_greg_string(Value *val, const std::string &reg_name);
    void store_from_freg_string(Value *val, const std::string &reg_name);
    
    void allocate(const RA::RegAllocator &RA_int, const RA::RegAllocator &RA_float);

    void gen_prologue();
    void gen_ret();
    void gen_br();
    void gen_binary();
    void gen_alloca();
    void gen_load();
    void gen_store();
    void gen_icmp_expr();
    void gen_icmp_branch();
    void gen_fcmp_expr();
    void gen_fcmp_branch();
    void gen_zext();
    void gen_call(std::unordered_map<Instruction *, int> instr_id);
    void gen_gep();
    void gen_sitofp();
    void gen_fptosi();
    void gen_epilogue();
    void gen_bitcast();
    void mapReg(Value *v, int reg_id, bool is_float);
    int fregname_to_id(const std::string &name);
    int regname_to_id(const std::string &name);
    
    int get_stack_offset(AllocaInst *alloca);

    static std::string label_name(BasicBlock *bb) {
        return "." + bb->get_parent()->get_name() + "_" + bb->get_name();
    }

    static std::string func_exit_label_name(Function *func) {
        return func->get_name() + "_exit";
    }

    static std::string fcmp_label_name(BasicBlock *bb, unsigned cnt) {
        return label_name(bb) + "_fcmp_" + std::to_string(cnt);
    }

    // static string regname(uint i, bool is_float) {
    //     string name;
    //     if (is_float) {
    //         // assert(false && "not implemented!");
    //         if (1 <= i and i <= 8)
    //             name = "fa" + to_string(i - 1);
    //         else if (9 <= i and i <= FR_USABLE)
    //             name = "ft" + to_string(i - 9 + 2);
    //         else
    //             name = "WRONG_REG_" + to_string(i);
    //     } else {
    //         if (1 <= i and i <= 8)
    //             name = "a" + to_string(i - 1);
    //         else if (9 <= i and i <= R_USABLE)
    //             name = "t" + to_string(i - 9 + 2);
    //         else
    //             name = "WRONG_REG_" + to_string(i);
    //     }
    //     return name;
    // }
    static string regname(uint i, bool is_float) {
        string name;
        if (is_float) {
            // assert(false && "not implemented!");
            if (1 <= i and i <= 8)
                name = "fa" + to_string(i - 1);
            else if (9 <= i and i <= 18)
                name = "ft" + to_string(i - 9 + 2);
            else if(19<=i and i<=FR_USABLE)
                name = "fs" + to_string(i - 19);
            else
                name = "WRONG_REG_" + to_string(i);
        } else {
            if (1 <= i and i <= 8)
                name = "a" + to_string(i - 1);
            else if (9 <= i and i <= 12)
                name = "t" + to_string(i - 9 + 2);
            else if(13<=i and i<=R_USABLE)
                name = "s" + to_string(i - 13 + 1);
            else
                name = "WRONG_REG_" + to_string(i);
        }
        return name;
    }

    string tmpregname(int i, bool is_float) const {
        if(is_float) {assert(i == 0 or i == 1);
            return (is_float ? "ft" : "t") + to_string(i);
        }else{
            assert(i == 0 or i == 1 or i == 6);
            return (is_float ? "ft" : "t") + to_string(i);
        }
    }

    static pair<int, int> immRange(int bit, bool u) {
        pair<int, int> res;
        if (u) {
            res.first = 0;
            res.second = (1 << bit) - 1;
        } else {
            bit--;
            res.first = -(1 << bit);
            res.second = (1 << bit) - 1;
        }

        return res;
    };

    static int typeLen(Type *type) {
        if (type->is_float_type())
            return 4;
        else if (type->is_integer_type()) {
            if (static_cast<IntegerType *>(type)->get_num_bits() == 32)
                return 4;
            else
                return 1;
        } else if (type->is_pointer_type())
            return 8;
        else if (type->is_array_type()) {
            auto arr_tp = static_cast<ArrayType *>(type);
            int n = arr_tp->get_num_of_elements();
            return n * typeLen(arr_tp->get_element_type());
        } else {
            assert(false && "unexpected case while computing type-length");
        }
    }

    static string suffix(Type *type) {
        int len = typeLen(type);
        switch (len) {
        case 1: return ".b";
        case 2: return ".h";
        case 4: return type->is_float_type() ? ".s" : ".w";
        case 8: return ".d";
        }
        assert(false && "no such suffix");
    }

    bool no_stack_alloca(Instruction *instr) const {
        if (instr->is_void())
            return true;
        if (instr->is_fcmp() or instr->is_cmp() or instr->is_zext())
            return true;
        auto regmap = (instr->get_type()->is_float_type() ? RA_float.get() : RA_int.get());
        if (regmap.find(instr) != regmap.end())
            return true;

        return false;
    }

    string label_in_assem(BasicBlock *bb) const { return (context.func)->get_name() + bb->get_name().substr(5); }

    struct {
        /* 随着 IR 遍历设置 */
        Function *func{nullptr};        // 当前函数
        BasicBlock *bb{nullptr};        // 当前基本块
        Instruction *inst{nullptr};     // 当前指令

        /* 在 allocate() 中设置 */
        unsigned origin_frame_size{0};  // 初始栈帧大小（如保存 ra/fp 之后）
        unsigned frame_size{0};         // 当前函数的最终栈帧大小（16 对齐）
        std::unordered_map<Value *, int> offset_map{}; // Value → 栈偏移（负数，相对 fp）
        std::unordered_map<string, int> offset_map_call{};
        unsigned fcmp_cnt{0};           // fcmp 计数器，用于生成 label
        unsigned copy_stmt_start{0};    // phi 拷贝指令区域起始偏移
        int max_cp_stmt{0};             // 所有 phi_path 中最大的拷贝语句数（×8 bytes）

        std::unordered_map<AllocaInst *, int> array_start_offset{}; // alloca 的数组类型偏移（不对齐）

        PhiMap phi_path;            // 所有路径（bb_from → bb_to）对应的 phi 拷贝映射
        std::map<std::pair<BasicBlock*,BasicBlock*>,std::set<std::pair<Value*,Value*>>> phi_path1;

        // 可选（建议保留）：寄存器分配映射（方便生成代码）
        std::unordered_map<Value *, int> reg_map_int;
        std::unordered_map<Value *, int> reg_map_float;

        std::string true_label;
        std::string false_label;

        void clear() {
            func = nullptr;
            bb = nullptr;
            inst = nullptr;

            origin_frame_size = 0;
            frame_size = 0;
            fcmp_cnt = 0;
            copy_stmt_start = 0;
            max_cp_stmt = 0;

            offset_map.clear();
            array_start_offset.clear();
            phi_path.clear();
            phi_path1.clear();

            reg_map_int.clear();
            reg_map_float.clear();

            true_label.clear();
            false_label.clear();
        }

    } context;


    Module *m;
    std::list<ASMInstruction> output;

    LRA::LiveRangeAnalyzer LRA;
    LRA::LVITS LVITS_int, LVITS_float;
    RA::RegAllocator RA_int, RA_float;

    std::map<BasicBlock*, std::vector<std::tuple<Value*, Value*, BasicBlock*> > > phi_map;
    std::map<Constant *, std::string> ROdata;

    //TODO:添加你需要的变量
};
//TODO: 本次实验为开放性实验，你可以自行设计实验框架并自行对提供的实验框架进行修改。本框架只作为参考，不要让它束缚住你的设计思路。
//TODO: 对框架不满可尽情修改