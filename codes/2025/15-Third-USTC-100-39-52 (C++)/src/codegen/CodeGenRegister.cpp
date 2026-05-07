#include "CodeGenRegister.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "ast.hpp"
#include "logging.hpp"
#include "regalloc.hpp"
#include "CodeGenUtil.hpp"
#include <queue>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
// int offsetnum=0;

void CodeGenRegister::getPhiMap()
{
    phi_map.clear();
    // TODO:对phi函数进行处理，为phi_map赋值
}

std::string CodeGenRegister::value2reg(Value *v, int i, std::string recommend, bool load_if_missing)
{
    bool is_float = v->get_type()->is_float_type();

    // 尝试直接获取分配的寄存器名
    auto [reg_name, find] = getRegName(v, i);
    if (find)
        return reg_name;

    // 未分配，使用推荐名或默认临时寄存器名
    if (recommend.empty())
        reg_name = is_float ? tmpregname(i, true) : tmpregname(i, false);
    else
        reg_name = recommend;
    // 如果不是变量（即不在 offset_map 中），说明是临时值，不用 mem_op，跳过 load_to_*，直接返回寄存器名
    if (dynamic_cast<Instruction *>(v) && context.offset_map.count(v) == 0)
    {
        return reg_name;
    }
    if (!load_if_missing)
    {
        // 不自动加载，直接返回寄存器名
        return reg_name;
    }
    // 整型 / 指针类型：使用封装的 load_to_greg
    if (v->get_type()->is_integer_type() || v->get_type()->is_pointer_type())
    {
        Reg reg = Reg::t(i);
        load_to_greg(v, reg);
        return reg_name;
    }

    // 浮点类型：使用封装的 load_to_freg 或常量处理
    if (is_float)
    {
        if (auto *cfp = dynamic_cast<ConstantFP *>(v))
        {
            FReg freg = FReg::ft(i);
            load_float_imm(cfp->get_value(), freg);
        }
        else
        {
            FReg freg = FReg::ft(i);
            load_to_freg(v, freg);
        }
        return reg_name;
    }
    // 类型未处理，触发断言
    assert(false && "Unhandled value type in value2reg");
    return reg_name; // 实际不会执行到
}
// void CodeGenRegister::copy_stmt() {
//     auto &copies = phi_map[context.bb];
//     if (copies.empty()) return;
//     const int word_size = 8;
//     int phi_cnt = copies.size();
//     int total_stack = phi_cnt * word_size;
//     if(total_stack%16!=0) total_stack+=8;
//     // append_inst("# === 开始处理 phi 拷贝语句 ===");
//     // append_inst("# phi 个数: " + std::to_string(phi_cnt));
//     // append_inst("# 为 phi 拷贝分配临时栈空间: -" + std::to_string(total_stack));
//      append_inst("addi sp, sp, -" + std::to_string(total_stack));
//     int offset = 0;
//     std::vector<Value*> defs;
//     std::map<Value*, bool> is_float;
//     std::map<Value*, int> def_offset;
//     for (auto &copy : copies) {
//         Value *def = std::get<0>(copy);
//         Value *val = std::get<1>(copy);
//         defs.push_back(def);
//         is_float[def] = def->get_type()->is_float_type();
//         def_offset[def] = offset;
//         std::string def_name = def->get_name();
//         std::string val_name = val ? val->get_name() : "undef";
//        // append_inst("# 保存 phi 拷贝: " + def_name + " = " + val_name);
//         if (val == nullptr || dynamic_cast<UndefValue*>(val) != nullptr) {
//             // append_inst("# val 为 undef，写入 0");
//             append_inst("li", {"t0", "0"});
//             append_inst("sd", {"t0", std::to_string(offset) + "(sp)"});
//         } else if (auto *CI = dynamic_cast<ConstantInt*>(val)) {
//             //append_inst("# val 为 ConstantInt: " + std::to_string(CI->get_value()));
//             append_inst("li", {"t0", std::to_string(CI->get_value())});
//             append_inst("sd", {"t0", std::to_string(offset) + "(sp)"});
//         } else if (auto *CFP = dynamic_cast<ConstantFP*>(val)) {
//             float fval = CFP->get_value();
//             int intval;
//             std::memcpy(&intval, &fval, sizeof(float));
//             //append_inst("# val 为 ConstantFP: " + std::to_string(fval) + " (int编码: " + std::to_string(intval) + ")");
//             append_inst("li", {"t0", std::to_string(intval)});
//             append_inst("sw", {"t0", std::to_string(offset) + "(sp)"});
//         } else {
//             bool is_float1 = val->get_type()->is_float_type();
//             std::string val_reg = value2reg(val, 1);
//             std::string suffix;
//             if (is_float1)
//                 suffix = "w";  // float
//             else if (val->get_type()->get_size() == 8||val->get_type()->is_pointer_type())
//                 suffix = "d";  // int64 或 pointer
//             else if (val->get_type()->get_size() == 4)
//                 suffix = "w";  // int32
//             else
//                 throw std::runtime_error("Unsupported store size");
//             std::string inst2 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
//             if(auto *arg = dynamic_cast<Argument *>(val)){
//                 int id = 0;
//                 for (auto &arg_test : arg->get_parent()->get_args()) {
//                     if (&arg_test == arg)
//                     break;
//                     id++;
//                 }
//                 //std::cerr << "This value is an argument. Argument index = " << id << std::endl;
//                 //int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
//                 int64_t offset = 8*id;
//                 if(!IS_IMM_12(offset)){
//                      if (val_reg == "t1") {
//                         if(is_float1) append_inst(inst2 + " " +"f"+ val_reg + ", " + "0(t0)");
//                         else append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
//                     } // 专门处理偏移过大的情况
//                 }
//             }else{
//                 if(!IS_IMM_12(context.offset_map[val])){
//                      if (val_reg == "t1") {
//                     if(is_float1) append_inst(inst2 + " " +"f"+ val_reg + ", " + "0(t0)");
//                     else append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
//                 } // 专门处理偏移过大的情况
//                 }
//             }
//             std::string inst1 = is_float1 ? ("fs" + suffix) : ("s" + suffix);
//             //append_inst("# val 是变量，寄存器: " + val_reg);
//             if (is_float[def])
//                 append_inst(inst1 +" "+val_reg+", "+std::to_string(offset) + "(sp)");
//             else
//                 append_inst(inst1 +" "+val_reg+", "+std::to_string(offset) + "(sp)");
//         }
//         offset += word_size;
//     }
//     //append_inst("# === 开始从栈加载到 def 寄存器 ===");
//     for (auto *def : defs) {
//         std::string def_reg = value2reg(def, 1, "", false);
//         int offset = def_offset[def];
//        // append_inst("# 加载 def: " + def->get_name() + " 到 " + def_reg);
//         if (is_float[def])
//             append_inst("flw", {def_reg, std::to_string(offset) + "(sp)"});
//         else
//             append_inst("ld", {def_reg, std::to_string(offset) + "(sp)"});
//         if (context.offset_map.count(def)) {
//            // append_inst("# 写回 def 到栈帧");
//             //if(def->get_type()->is_int32_type()) append_inst("# 这是i32");
//             if (is_float[def])
//                 store_from_freg_string(def, def_reg);
//             else
//                 store_from_greg_string(def, def_reg);
//         }
//     }
//     // append_inst("# 恢复栈空间");
//      append_inst("addi sp, sp, " + std::to_string(total_stack));
//     // append_inst("# === phi 拷贝处理完毕 ===");
// }
// void CodeGenRegister::copy_stmt() {
//     auto &copies = phi_map[context.bb];
//     if (copies.empty()) return;
//     for (auto &copy : copies) {
//         Value *def = std::get<0>(copy);
//         Value *val = std::get<1>(copy);
//         bool def_is_float=def->get_type()->is_float_type();
//         if (val == nullptr || dynamic_cast<UndefValue*>(val)) {
//             // undef 处理：直接写 0
//             //bool def_is_float=def->get_type()->is_float_type();
//             if(def_is_float){
//                 append_inst("li", {"t0", "0"});
//                 append_inst("fmv.w.x ft0, t0");
//             }
//             else{
//                 append_inst("li", {"t0", "0"});
//             }
//             int offset2 = context.offset_map[def];
//             if(offset2!=0){
//                 if(IS_IMM_12(offset2)){
//                     if (def->get_type()->is_float_type()) {
//                         append_inst("fsw", {"ft0", std::to_string(context.offset_map[def]) + "(fp)"});
//                     } else {
//                         append_inst("sd", {"t0", std::to_string(context.offset_map[def]) + "(fp)"});
//                     }
//                 }else{
//                     if (def->get_type()->is_float_type()) {
//                         append_inst("li t1, "+std::to_string(offset2));
//                         append_inst("add t1, fp, t1");
//                         append_inst("fsw ft0, 0(t1)");
//                     } else {
//                     // append_inst("sd", {"t0", std::to_string(context.offset_map[def]) + "(fp)"});
//                         append_inst("li t1, "+std::to_string(offset2));
//                         append_inst("add t1, fp, t1");
//                         append_inst("sd t0, 0(t1)");
//                     }
//                 }
//             }
//             continue;
//         }
//         std::string val_reg;
//         bool is_float = val->get_type()->is_float_type();
//         if (auto *CI = dynamic_cast<ConstantInt*>(val)) {
//             if(def_is_float){
//                 append_inst("li", {"t0", std::to_string(CI->get_value())});
//                 append_inst("fmv.w.x ft0, t0");
//                 val_reg = "ft0";
//             }  else{
//                 append_inst("li", {"t0", std::to_string(CI->get_value())});
//                 val_reg = "t0";
//             }
//         } else if (auto *CFP = dynamic_cast<ConstantFP*>(val)) {
//             float fval = CFP->get_value();
//             int intval;
//             std::memcpy(&intval, &fval, sizeof(float));
//             if(def_is_float){
//                 append_inst("li", {"t0", std::to_string(intval)});
//                 append_inst("fmv.w.x ft0, t0");
//                 val_reg = "ft0";
//             }  else{
//                 append_inst("li", {"t0", std::to_string(intval)});
//                 val_reg = "t0";
//             }
//         } else {
//             val_reg = value2reg(val, 0);
//         }
//         std::string suffix;
//         if (is_float)
//             suffix = "w";  // float
//         else if (val->get_type()->get_size() == 8||val->get_type()->is_pointer_type())
//             suffix = "d";  // int64 或 pointer
//         else if (val->get_type()->get_size() == 4)
//             suffix = "w";  // int32
//         else
//             throw std::runtime_error("Unsupported store size");
//         std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
//         if(auto *arg = dynamic_cast<Argument *>(val)){
//             int id = 0;
//             for (auto &arg_test : arg->get_parent()->get_args()) {
//                 if (&arg_test == arg)
//                 break;
//                 id++;
//             }
//             int64_t offset = 8*id;
//             if(!IS_IMM_12(offset)){
//                 if (val_reg == "t0") {
//                     if(is_float) append_inst(inst2 + " " +"f"+ val_reg + ", " + "0(t0)");
//                     else append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
//                 } // 专门处理偏移过大的情况
//             }
//         }else{
//             if(!IS_IMM_12(context.offset_map[val])){
//                 if (val_reg == "t0") {
//                     if(is_float) append_inst(inst2 + " " +"f"+ val_reg + ", " + "0(t0)");
//                     else append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
//                 } // 专门处理偏移过大的情况
//             }
//         }
//         int offset1 = context.offset_map[def];
//         std::string inst = is_float ? "fsw" : "sd";
//         if(offset1!=0){
//             if(IS_IMM_12(offset1)){
//                 append_inst(inst +" "+ val_reg+", "+std::to_string(offset1) + "(fp)");
//             }else{
//                 append_inst("li t1, "+std::to_string(offset1));
//                 append_inst("add t1, fp, t1");
//                 append_inst(inst +" "+ val_reg + ", " + "0(t1)");
//             }
//         }
//     }
// }

void CodeGenRegister::copy_stmt()
{
    auto &copies = phi_map[context.bb];
    if (copies.empty())
        return;
    std::vector<std::tuple<Value *, Value *>> need_stack_copy;
    for (auto &copy : copies)
    {
        Value *def = std::get<0>(copy);
        Value *val = std::get<1>(copy);
        bool def_is_float = def->get_type()->is_float_type();
        if (val == nullptr || dynamic_cast<UndefValue *>(val))
        {
            // undef 处理：直接写 0
            // bool def_is_float=def->get_type()->is_float_type();
            if (def_is_float)
            {
                append_inst("li", {"t0", "0"});
                append_inst("fmv.w.x ft0, t0");
            }
            else
            {
                append_inst("li", {"t0", "0"});
            }
            int offset2 = context.offset_map[def];
            if (offset2 != 0)
            {
                if (IS_IMM_12(offset2))
                {
                    if (def->get_type()->is_float_type())
                    {
                        append_inst("fsw", {"ft0", std::to_string(context.offset_map[def]) + "(fp)"});
                    }
                    else
                    {
                        append_inst("sd", {"t0", std::to_string(context.offset_map[def]) + "(fp)"});
                    }
                }
                else
                {
                    if (def->get_type()->is_float_type())
                    {
                        append_inst("li t1, " + std::to_string(offset2));
                        append_inst("add t1, fp, t1");
                        append_inst("fsw ft0, 0(t1)");
                    }
                    else
                    {
                        // append_inst("sd", {"t0", std::to_string(context.offset_map[def]) + "(fp)"});
                        append_inst("li t1, " + std::to_string(offset2));
                        append_inst("add t1, fp, t1");
                        append_inst("sd t0, 0(t1)");
                    }
                }
            }
            continue;
        }
        std::string val_reg;
        bool is_float = val->get_type()->is_float_type();
        if (auto *CI = dynamic_cast<ConstantInt *>(val))
        {
            if (def_is_float)
            {
                append_inst("li", {"t0", std::to_string(CI->get_value())});
                append_inst("fmv.w.x ft0, t0");
                val_reg = "ft0";
            }
            else
            {
                append_inst("li", {"t0", std::to_string(CI->get_value())});
                val_reg = "t0";
            }
        }
        else if (auto *CFP = dynamic_cast<ConstantFP *>(val))
        {
            float fval = CFP->get_value();
            int intval;
            std::memcpy(&intval, &fval, sizeof(float));
            if (def_is_float)
            {
                append_inst("li", {"t0", std::to_string(intval)});
                append_inst("fmv.w.x ft0, t0");
                val_reg = "ft0";
            }
            else
            {
                append_inst("li", {"t0", std::to_string(intval)});
                val_reg = "t0";
            }
        }
        else
        {
            val_reg = value2reg(val, 0);
        }
        std::string suffix;
        if (is_float)
            suffix = "w"; // float
        else if (val->get_type()->get_size() == 8 || val->get_type()->is_pointer_type())
            suffix = "d"; // int64 或 pointer
        else if (val->get_type()->get_size() == 4)
            suffix = "w"; // int32
        else
            throw std::runtime_error("Unsupported store size");
        std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
        if (auto *arg = dynamic_cast<Argument *>(val))
        {
            int id = 0;
            for (auto &arg_test : arg->get_parent()->get_args())
            {
                if (&arg_test == arg)
                    break;
                id++;
            }
            int64_t offset = 8 * id;
            if (!IS_IMM_12(offset))
            {
                if (val_reg == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + val_reg + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        else
        {
            if (!IS_IMM_12(context.offset_map[val]))
            {
                if (val_reg == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + val_reg + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + val_reg + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        auto [dest1, find1] = getRegName(def, 0);
        if (!find1)
        {
            int offset1 = context.offset_map[def];
            std::string inst = is_float ? "fsw" : "sd";
            if (offset1 != 0)
            {
                if (IS_IMM_12(offset1))
                {
                    append_inst(inst + " " + val_reg + ", " + std::to_string(offset1) + "(fp)");
                }
                else
                {
                    append_inst("li t1, " + std::to_string(offset1));
                    append_inst("add t1, fp, t1");
                    append_inst(inst + " " + val_reg + ", " + "0(t1)");
                }
            }
        }
        else
        {
            if (is_float)
            {
                append_inst("fmv.s " + dest1 + ", " + val_reg);
            }
            else
            {
                append_inst("mv " + dest1 + ", " + val_reg);
            }
        }
    }
}

// void CodeGenRegister::pass_arguments(CallInst *instr, int stack_arg_base) {
//     const int N = 8; // a0-a7/fa0-fa7
//     int arg_cnt = instr->get_num_operand() - 1;
//     // 所有参数都先写入栈
//     for (int i = 1; i <= arg_cnt; ++i) {
//         Value *arg = instr->get_operand(i);
//         std::string src = value2reg(arg, 0); // 获取当前值的寄存器或临时值
//         bool is_float = arg->get_type()->is_float_type();
//         std::string suffix;
//         if (is_float)
//             suffix = "w";  // float
//         else if (arg->get_type()->get_size() == 8||arg->get_type()->is_pointer_type())
//             suffix = "d";  // int64 或 pointer
//         else if (arg->get_type()->get_size() == 4)
//             suffix = "w";  // int32
//         else
//             throw std::runtime_error("Unsupported store size");
//         std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
//         if(auto *arg1 = dynamic_cast<Argument *>(arg)){
//             int id = 0;
//             for (auto &arg_test : arg1->get_parent()->get_args()) {
//                 if (&arg_test == arg1)
//                 break;
//                 id++;
//             }
//             //std::cerr << "This value is an argument. Argument index = " << id << std::endl;
//             //int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
//             int64_t offset = 8*id;
//             if(!IS_IMM_12(offset)){
//                 if(src=="t0") append_inst(inst2 + " " + src + ", " + "0(t0)"); //专门处理偏移过大的情况
//             }
//         }else{
//             if(!IS_IMM_12(context.offset_map[arg])){
//                 if(src=="t0") append_inst(inst2 + " " + src + ", " + "0(t0)"); //专门处理偏移过大的情况
//             }
//         }
//         int offset = stack_arg_base + (i - 1) * 8;
//         if(offset >= -2048 && offset <= 2047){
//             if (arg->get_type()->is_float_type())
//                 append_inst("fsd " + src + ", " + std::to_string(offset) + "(sp)");
//             else
//                 append_inst("sd " + src + ", " + std::to_string(offset) + "(sp)");
//         }else{
//             std::string src1="t6";
//             if (arg->get_type()->is_float_type()){
//                 append_inst("li " + src1 + ", " + std::to_string(offset));
//                 append_inst("add", {"t6", "sp", "t6"});
//                 append_inst("fsd " + src + ", " +  "0(t6)");
//             }
//             else{
//                 append_inst("li " + src1 + ", " + std::to_string(offset));
//                 append_inst("add", {"t6", "sp", "t6"});
//                 append_inst("sd " + src + ", " +  "0(t6)");
//             }
//         }
//     }
//     int int_cnt = 0, float_cnt = 0;
//     for (int i = 1; i <= arg_cnt; ++i) {
//         Value *arg = instr->get_operand(i);
//         bool is_fp = arg->get_type()->is_float_type();
//         int offset = stack_arg_base + (i - 1) * 8;
//         if (is_fp) {
//             if (float_cnt < 8) {
//                 std::string dst = "fa" + std::to_string(float_cnt++);
//                 append_inst("flw " + dst + ", " + std::to_string(offset) + "(sp)");
//             }
//         // 否则溢出 float 参数不加载寄存器，直接在 stack 上使用
//         } else {
//             if (int_cnt < 8) {
//                 std::string dst = "a" + std::to_string(int_cnt++);
//                 append_inst("ld " + dst + ", " + std::to_string(offset) + "(sp)");
//             }
//         // 否则溢出 int 参数不加载寄存器，直接在 stack 上使用
//         }
//     }
// }

void CodeGenRegister::pass_arguments(CallInst *instr, int stack_arg_base)
{
    const int N = 8;                            // a0-a7/fa0-fa7
    int arg_cnt = instr->get_num_operand() - 1; // 参数数量
    int int_cnt = 0, float_cnt = 0;
    for (int i = 1; i <= arg_cnt; ++i)
    {
        Value *arg = instr->get_operand(i);
        std::string src = value2reg(arg, 0); // 获取当前值的寄存器或临时值
        bool is_float = arg->get_type()->is_float_type();
        std::string suffix;
        if (is_float)
            suffix = "w"; // float
        else if (arg->get_type()->get_size() == 8 || arg->get_type()->is_pointer_type())
            suffix = "d"; // int64 或 pointer
        else if (arg->get_type()->get_size() == 4)
            suffix = "w"; // int32
        else
            throw std::runtime_error("Unsupported store size");
        std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
        std::string inst1 = is_float ? ("fs" + suffix) : ("s" + suffix);
        // 如果是函数参数
        int id = 0;
        int64_t offset;
        if (auto *arg1 = dynamic_cast<Argument *>(arg))
        {
            auto [dest, find] = getRegName(arg1, 0);
            // int id = 0;
            for (auto &arg_test : arg1->get_parent()->get_args())
            {
                if (&arg_test == arg1)
                    break;
                id++;
            }
            offset = 8 * id;
            // if(context.offset_map[arg]&&find){
            //     append_inst(inst1 + " " + src + ", " + std::to_string(context.offset_map[arg]) + "(fp)");
            // }
            // else
            if (!IS_IMM_12(offset))
            {
                if (src == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + src + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + src + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        else
        {
            if (!IS_IMM_12(context.offset_map[arg]))
            {
                if (src == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + src + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + src + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        // 直接传递参数到寄存器 a0-a7 或 fa0-fa7
        if (is_float)
        {
            if (float_cnt < N)
            {
                std::string dst = "fa" + std::to_string(float_cnt++);
                // append_inst("fsgnj " + dst + ", " + src + ", " + src); // 将浮点数移动到目标寄存器
                if (auto *arg1 = dynamic_cast<Argument *>(arg))
                {
                    // int a=-context.origin_frame_size-31*8;
                    //  append_inst("sd " + src + ", " + std::to_string(a) + "(fp)");
                    //  append_inst("ld " + dst + ", " + std::to_string(a) + "(fp)");
                    // std::string name = arg1->get_name(); // 例如 "arg12"
                    //  int index;
                    //  if (name.rfind("arg", 0) == 0 && name.size() > 3) { // 以 "arg" 开头，后面还有数字
                    //      try {
                    //          index = std::stoi(name.substr(3)); // 提取 "12" 并转为整数
                    //          std::cout << "这是第 " << index << " 个参数" << std::endl;
                    //      } catch (const std::exception &e) {
                    //          std::cerr << "参数名格式错误: " << name << std::endl;
                    //      }
                    //  }
                    auto [dest1, find1] = getRegName(arg1, 0);
                    if (find1)
                    {
                        // int a = -index*8-context.origin_frame_size;
                        // string regnow="fa"+to_string(index);
                        int a = context.offset_map_call[dest1];
                        if (IS_IMM_12(a))
                            // append_inst("sd " + src + ", " + std::to_string(a) + "(fp)");
                            append_inst("fld " + dst + ", " + std::to_string(a) + "(fp)");
                        else
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("fld " + dst + ", " + "0(t1)");
                        }
                    }
                    else
                    {
                        // //int a=-context.origin_frame_size-31*8;
                        // if(IS_IMM_12(a)){
                        //     // append_inst("fsd " + src + ", " + std::to_string(a) + "(fp)");
                        //     // append_inst("fld " + dst + ", " + std::to_string(a) + "(fp)");
                        //     append_inst("fmv.s " +dst+" "+src);
                        // }else{
                        //     append_inst("li t1, "+std::to_string(a));
                        //     append_inst("add t1, fp, t1");
                        //     append_inst("fsd " + src + ", " + "0(t1)");
                        //     append_inst("fld " + dst + ", " + "0(t1)");
                        // }
                        append_inst("fmv.s " + dst + ", " + src);
                    }
                }
                else
                {
                    // int a=-context.origin_frame_size-31*8;
                    // if(IS_IMM_12(a)){
                    //     append_inst("fsd " + src + ", " + std::to_string(a) + "(fp)");
                    //     append_inst("fld " + dst + ", " + std::to_string(a) + "(fp)");
                    // }else{
                    //     append_inst("li t1, "+std::to_string(a));
                    //     append_inst("add t1, fp, t1");
                    //     append_inst("fsd " + src + ", " + "0(t1)");
                    //     append_inst("fld " + dst + ", " + "0(t1)");
                    // }
                    append_inst("fmv.s " + dst + ", " + src);
                }
            }
            else
            {
                // 如果超过 8 个浮点参数，直接在栈上处理
                int offset = stack_arg_base + (i - 1) * 8;
                if (auto *arg1 = dynamic_cast<Argument *>(arg))
                {
                    // std::string name = arg1->get_name(); // 例如 "arg12"
                    // int index;
                    // if (name.rfind("arg", 0) == 0 && name.size() > 3) { // 以 "arg" 开头，后面还有数字
                    //     try {
                    //         index = std::stoi(name.substr(3)); // 提取 "12" 并转为整数
                    //         std::cout << "这是第 " << index << " 个参数" << std::endl;
                    //     } catch (const std::exception &e) {
                    //         std::cerr << "参数名格式错误: " << name << std::endl;
                    //     }
                    // }
                    // int a = -index*8-context.origin_frame_size;
                    auto [dest1, find1] = getRegName(arg1, 0);
                    if (find1)
                    {
                        // int a = context.offset_map[arg1];
                        // string regnow="fa"+to_string(index);
                        int a = context.offset_map_call[dest1];
                        string lin = "ft0";
                        if (IS_IMM_12(offset) && IS_IMM_12(a))
                        {
                            append_inst("fld " + lin + ", " + std::to_string(a) + "(fp)");
                            append_inst("fsd " + lin + ", " + std::to_string(offset) + "(sp)");
                        }
                        else if (IS_IMM_12(offset) && !IS_IMM_12(a))
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("fld " + lin + ", " + "0(t1)");
                            append_inst("fsd " + lin + ", " + std::to_string(offset) + "(sp)");
                        }
                        else if (!IS_IMM_12(offset) && IS_IMM_12(a))
                        {
                            append_inst("fld " + lin + ", " + std::to_string(a) + "(fp)");
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");

                            append_inst("fsd " + lin + ", " + "0(t1)");
                        }
                        else
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("fld " + lin + ", " + "0(t1)");
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");

                            append_inst("fsd " + lin + ", " + "0(t1)");
                        }
                    }
                    else
                    {
                        if (IS_IMM_12(offset))
                        {
                            append_inst("fsd " + src + " , " + std::to_string(offset) + "(sp)");
                        }
                        else
                        {
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");

                            append_inst("fsd " + src + ", " + "0(t1)");
                        }
                    }
                }
                else if (IS_IMM_12(offset))
                {
                    append_inst("fsd " + src + ", " + std::to_string(offset) + "(sp)");
                }
                else
                {
                    append_inst("li t1, " + std::to_string(offset));
                    // append_inst("li t1, "+std::to_string(offset));
                    append_inst("add t1, sp, t1");
                    // append_inst("fsd " + reg + ", " + "0(t1)");

                    append_inst("fsd " + src + ", " + "0(t1)");
                }
            }
        }
        else
        {
            if (int_cnt < N)
            {
                std::string dst = "a" + std::to_string(int_cnt++);
                if (auto *arg1 = dynamic_cast<Argument *>(arg))
                {
                    // int a=-context.origin_frame_size-31*8;
                    //  append_inst("sd " + src + ", " + std::to_string(a) + "(fp)");
                    //  append_inst("ld " + dst + ", " + std::to_string(a) + "(fp)");
                    //  std::string name = arg1->get_name(); // 例如 "arg12"
                    //  int index;
                    //  if (name.rfind("arg", 0) == 0 && name.size() > 3) { // 以 "arg" 开头，后面还有数字
                    //      try {
                    //          index = std::stoi(name.substr(3)); // 提取 "12" 并转为整数
                    //          std::cout << "这是第 " << index << " 个参数" << std::endl;
                    //      } catch (const std::exception &e) {
                    //          std::cerr << "参数名格式错误: " << name << std::endl;
                    //      }
                    //  }
                    // string regnow="a"+to_string(index);
                    // int a = context.offset_map_call[regnow];
                    auto [dest1, find1] = getRegName(arg1, 0);
                    if (find1)
                    {
                        // int a = -index*8-context.origin_frame_size;
                        // int a = context.offset_map[arg1];
                        // string regnow="a"+to_string(index);
                        int a = context.offset_map_call[dest1];
                        if (IS_IMM_12(a))
                            // append_inst("sd " + src + ", " + std::to_string(a) + "(fp)");
                            append_inst("ld " + dst + ", " + std::to_string(a) + "(fp)");
                        else
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("ld " + dst + ", " + "0(t1)");
                        }
                    }
                    else
                    {
                        append_inst("mv " + dst + ", " + src);
                    }
                }
                else
                {
                    append_inst("mv " + dst + ", " + src); // 将整数移动到目标寄存器
                }
            }
            else
            {
                // 如果超过 8 个整数参数，直接在栈上处理
                int offset = stack_arg_base + (i - 1) * 8;
                if (auto *arg1 = dynamic_cast<Argument *>(arg))
                {
                    // std::string name = arg1->get_name(); // 例如 "arg12"
                    // int index;
                    // if (name.rfind("arg", 0) == 0 && name.size() > 3) { // 以 "arg" 开头，后面还有数字
                    //     try {
                    //         index = std::stoi(name.substr(3)); // 提取 "12" 并转为整数
                    //         std::cout << "这是第 " << index << " 个参数" << std::endl;
                    //     } catch (const std::exception &e) {
                    //         std::cerr << "参数名格式错误: " << name << std::endl;
                    //     }
                    // }
                    auto [dest1, find1] = getRegName(arg1, 0);
                    if (find1)
                    {
                        // int a = -index*8-context.origin_frame_size;
                        // string regnow="a"+to_string(index);
                        int a = context.offset_map_call[dest1];
                        string lin = "t0";
                        if (IS_IMM_12(offset) && IS_IMM_12(a))
                        {
                            append_inst("ld " + lin + ", " + std::to_string(a) + "(fp)");
                            append_inst("sd " + lin + ", " + std::to_string(offset) + "(sp)");
                        }
                        else if (IS_IMM_12(offset) && !IS_IMM_12(a))
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("ld " + lin + ", " + "0(t1)");
                            append_inst("sd " + lin + ", " + std::to_string(offset) + "(sp)");
                        }
                        else if (!IS_IMM_12(offset) && IS_IMM_12(a))
                        {
                            append_inst("ld " + lin + ", " + std::to_string(a) + "(fp)");
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");

                            append_inst("sd " + lin + ", " + "0(t1)");
                        }
                        else
                        {
                            append_inst("li t1, " + std::to_string(a));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, fp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("ld " + lin + ", " + "0(t1)");
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");
                            append_inst("sd " + lin + ", " + "0(t1)");
                        }
                    }
                    else
                    {
                        if (IS_IMM_12(offset))
                        {
                            append_inst("sd " + src + ", " + std::to_string(offset) + "(sp)");
                        }
                        else
                        {
                            append_inst("li t1, " + std::to_string(offset));
                            // append_inst("li t1, "+std::to_string(offset));
                            append_inst("add t1, sp, t1");
                            // append_inst("fsd " + reg + ", " + "0(t1)");

                            append_inst("sd " + src + ", " + "0(t1)");
                        }
                    }
                }
                else if (IS_IMM_12(offset))
                {
                    append_inst("sd " + src + ", " + std::to_string(offset) + "(sp)");
                }
                else
                {
                    append_inst("li t1, " + std::to_string(offset));
                    // append_inst("li t1, "+std::to_string(offset));
                    append_inst("add t1, sp, t1");
                    // append_inst("fsd " + reg + ", " + "0(t1)");

                    append_inst("sd " + src + ", " + "0(t1)");
                }
                // append_inst("sd " + src + ", " + std::to_string(offset) + "(sp)");
            }
        }
    }
    // // 直接传递完参数后，将溢出的参数从栈中加载（如果有的话）
    // float_cnt=0;
    // int_cnt=0;
    // for (int i = 1; i <= arg_cnt; ++i) {
    //     Value *arg = instr->get_operand(i);
    //     bool is_fp = arg->get_type()->is_float_type();
    //     int offset = stack_arg_base + (i - 1) * 8;

    //     if (is_fp) {
    //         if (!(float_cnt < N)) {
    //             std::string dst = "fa" + std::to_string(float_cnt++);
    //             append_inst("flw " + dst + ", " + std::to_string(offset) + "(sp)");
    //         }
    //     } else {
    //         if (!(int_cnt < N)) {
    //             std::string dst = "a" + std::to_string(int_cnt++);
    //             append_inst("ld " + dst + ", " + std::to_string(offset) + "(sp)");
    //         }
    //     }
    // }
}

// 计算内存地址字符串，返回类似 "baseReg, offset" 或 临时寄存器地址字符串
string CodeGenRegister::getMemAddr(Value *val, const Reg &baseReg)
{
    if (context.offset_map.count(val) == 0)
    {
        // std::cerr << "Error: val not found in offset_map: ";
        // val->print(); // 打印 Value 的内容
        // std::cerr << "  [Type: " << val->get_type()->print() << "]" << std::endl;
        // std::cout << "Value name: " << val->get_name() << std::endl;

        throw std::runtime_error("offset_map missing value");
    }
    // 检查是否是参数
    if (auto *arg = dynamic_cast<Argument *>(val))
    {
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args())
        {
            if (&arg_test == arg)
                break;
            id++;
        }
        // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
        // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
        int64_t offset = 8 * id;
        // printf("\n\n%d\n\n",id);
        if (IS_IMM_12(offset))
        {
            return std::to_string(offset) + "(" + baseReg.print() + ")";
        }
        else
        {
            auto tmpReg = Reg::t(0);
            load_large_int64(offset, tmpReg);                                      // 加载大偏移到 tmpReg
            append_inst("add", {tmpReg.print(), baseReg.print(), tmpReg.print()}); // tmpReg = baseReg + offset
            return "0(" + tmpReg.print() + ")";
        }
    }
    else
    {
        int64_t offset = context.offset_map.at(val);
        if (IS_IMM_12(offset))
        {
            return std::to_string(offset) + "(" + baseReg.print() + ")";
        }
        else
        {
            auto tmpReg = Reg::t(0);
            load_large_int64(offset, tmpReg);                                      // 加载大偏移到 tmpReg
            append_inst("add", {tmpReg.print(), baseReg.print(), tmpReg.print()}); // tmpReg = baseReg + offset
            return "0(" + tmpReg.print() + ")";
        }
    }
}

void CodeGenRegister::mem_op(Value *val, const string &regName, MemOpType op, bool is_store_from)
{
    string addr;
    bool load = false;
    if (val == nullptr)
    {
        // std::cerr << "[Error] val is nullptr in mem_op!" << std::endl;
        throw std::runtime_error("Null val pointer");
    }

    // if (context.offset_map.find(val) != 0&&context.offset_map.at(val)!=0) {
    auto [dest1, find1] = getRegName(val, 0);
    if (context.offset_map.count(val) != 0 && !find1)
    {
        // 变量，使用基址寄存器 + 偏移访问内存
        addr = getMemAddr(val); // e.g., "fp, 8" ✅ 这个我们稍后再检查
    }
    else
    {
        // 临时值，先生成寄存器保存结果，然后用寄存器寻址
        string tmpReg = value2reg(val, 0); // 生成或获取寄存器名
        addr = tmpReg;                     // ✅ 改为合法格式
        append_inst("mv", {regName, addr});
        return;
    }
    if (addr == "0(t0)" && !is_store_from)
        return;
    switch (op)
    {
    case MemOpType::LoadByte:
        append_inst("lb", {regName, addr});
        break;
    case MemOpType::LoadHalf:
        append_inst("lh", {regName, addr});
        break;
    case MemOpType::LoadInt:
        append_inst("lw", {regName, addr});
        break;
    case MemOpType::LoadDouble:
        append_inst("ld", {regName, addr});
        break;
    case MemOpType::LoadFloat:
        append_inst("flw", {regName, addr});
        break;

    case MemOpType::StoreByte:
        append_inst("sb", {regName, addr});
        break;
    case MemOpType::StoreHalf:
        append_inst("sh", {regName, addr});
        break;
    case MemOpType::StoreInt:
        append_inst("sw", {regName, addr});
        break;
    case MemOpType::StoreDouble:
        append_inst("sd", {regName, addr});
        break;
    case MemOpType::StoreFloat:
        append_inst("fsw", {regName, addr});
        break;

    default:
        assert(false && "Unsupported MemOpType in mem_op");
    }
}

void CodeGenRegister::load_to_greg(Value *val, const Reg &reg)
{
    assert(val->get_type()->is_integer_type() || val->get_type()->is_pointer_type());

    if (auto *constant = dynamic_cast<ConstantInt *>(val))
    {
        int32_t val_int = constant->get_value();
        if (IS_IMM_12(val_int))
        {
            append_inst(LI, {reg.print(), std::to_string(val_int)});
        }
        else
        {
            load_large_int32(val_int, reg);
        }
    }
    else if (auto *global = dynamic_cast<GlobalVariable *>(val))
    {
        append_inst(LOAD_ADDR, {reg.print(), global->get_name()});
    }
    else
    {
        // 根据类型决定加载指令
        if (val->get_type()->is_pointer_type() || val->get_type()->is_int64_type())
        {
            mem_op(val, reg.print(), MemOpType::LoadDouble); // 64位加载
        }
        else if (val->get_type()->is_integer_type() || val->get_type()->is_int32_type())
        {
            mem_op(val, reg.print(), MemOpType::LoadInt); // 32位加载
        }
        else
        {
            assert(false && "Unsupported type in load_to_greg");
        }
    }
}
void CodeGenRegister::load_to_greg_string(Value *val, const std::string &reg)
{
    assert(val->get_type()->is_integer_type() || val->get_type()->is_pointer_type());

    if (auto *constant = dynamic_cast<ConstantInt *>(val))
    {
        int32_t val_int = constant->get_value();
        if (IS_IMM_12(val_int))
        {
            append_inst(LI, {reg, std::to_string(val_int)});
        }
        else
        {
            append_inst("li", {reg, std::to_string(val_int)});
        }
    }
    else if (auto *global = dynamic_cast<GlobalVariable *>(val))
    {
        append_inst(LOAD_ADDR, {reg, global->get_name()});
    }
    else
    {
        // 根据类型决定加载指令
        if (val->get_type()->is_pointer_type() || val->get_type()->is_int64_type())
        {
            mem_op(val, reg, MemOpType::LoadDouble); // 64位加载
        }
        else if (val->get_type()->is_integer_type() || val->get_type()->is_int32_type())
        {
            mem_op(val, reg, MemOpType::LoadInt); // 32位加载
        }
        else
        {
            assert(false && "Unsupported type in load_to_greg");
        }
    }
}

// 调用示例：将浮点寄存器的值存回内存
void CodeGenRegister::store_from_freg(Value *val, const FReg &freg)
{
    mem_op(val, freg.print(), MemOpType::StoreFloat, true);
}
void CodeGenRegister::store_from_freg_string(Value *val, const std::string &reg_name)
{
    mem_op(val, reg_name, MemOpType::StoreFloat, true);
}

void CodeGenRegister::gencopy(std::string lhs_reg, std::string rhs_reg, bool is_float)
{
    if (rhs_reg != lhs_reg)
    {
        if (is_float)
        {
            // 浮点寄存器复制，单精度用 fsgnj.s
            append_inst("fmv.s " + lhs_reg + ", " + rhs_reg);
        }
        else
        {
            // 整数寄存器复制，使用伪指令 mv
            append_inst("mv " + lhs_reg + ", " + rhs_reg);
        }
    }
}

bool CodeGenRegister::gencopy(Value *lhs, string rhs_reg)
{
    auto [lhs_reg, find] = getRegName(lhs);
    bool is_float = lhs->get_type()->is_float_type();

    if (find)
    {
        // 如果 lhs 已有寄存器分配，则直接进行寄存器复制
        gencopy(lhs_reg, rhs_reg, is_float);
    }
    else if (context.offset_map.count(lhs))
    {
        // 否则说明 lhs 是一个栈上变量，把 rhs_reg 中的值写回内存
        if (is_float)
        {
            store_from_freg_string(lhs, rhs_reg);
        }
        else
        {
            store_from_greg_string(lhs, rhs_reg);
        }
    }
    return true;
}

std::pair<std::string, bool> CodeGenRegister::getRegName(Value *v, int i) const
{
    // assert(i == 0 || i == 1);  // 0 表示 def，1 表示 use
    // bool is_def = (i == 0);

    // 判断是否是浮点类型
    bool is_float = v->get_type()->is_float_type();
    std::string name;
    bool find = false;

    // 查询对应 RegAllocator
    if (is_float)
    {
        auto it = RA_float.get().find(v);
        if (it != RA_float.get().end())
        {
            name = regname(it->second, true); // 例如 f0, f1
            find = true;
        }
    }
    else
    {
        auto it = RA_int.get().find(v);
        if (it != RA_int.get().end())
        {
            name = regname(it->second, false); // 例如 x10, t0 等
            find = true;
        }
    }

    return {name, find};
}

std::string CodeGenRegister::bool2branch(Instruction *cond)
{
    assert(cond->get_type() == m->get_int1_type());

    auto op_id = cond->get_instr_type(); // 使用已有的 OpID

    switch (op_id)
    {
    case Instruction::eq:
        return "eq";
    case Instruction::ne:
        return "ne";
    case Instruction::gt:
        return "gt";
    case Instruction::ge:
        return "ge";
    case Instruction::lt:
        return "lt";
    case Instruction::le:
        return "le";
    case Instruction::fgt:
        return "gt";
    case Instruction::fge:
        return "ge";
    case Instruction::flt:
        return "lt";
    case Instruction::fle:
        return "le";
    case Instruction::feq:
        return "eq";
    case Instruction::fne:
        return "ne";
    default:
        assert(false && "Unsupported condition instruction");
        return "";
    }
}

void CodeGenRegister::allocate(const RA::RegAllocator &RA_int,
                               const RA::RegAllocator &RA_float)
{
    unsigned offset = 24;
    // context.origin_frame_size = offset;
    // LOG_INFO << "Origin frame size: " << context.origin_frame_size;
    // offset += (R_USABLE + FR_USABLE) * 8;
    // offset = ALIGN(offset, PROLOGUE_ALIGN);

    auto *func = context.func;
    const auto &reg_int_map = RA_int.get();
    const auto &reg_float_map = RA_float.get();
    const auto &spill_int = RA_int.get_spill();
    const auto &spill_float = RA_float.get_spill();
    // int int_num=0;
    // int float_num=0;
    // for (auto &arg : context.func->get_args()) {
    //     if(float_num < 8){
    //         if(arg.get_type()->is_float_type()){
    //             auto size = arg.get_type()->get_size();
    //             offset = ALIGN(offset + size, size);
    //             context.offset_map[&arg] = -static_cast<int>(offset);
    //             float_num++;
    //             continue;
    //         }

    //     }
    //     if(int_num < 8){
    //         if(!arg.get_type()->is_float_type()){
    //             auto size = arg.get_type()->get_size();
    //             offset = ALIGN(offset + size, size);
    //             context.offset_map[&arg] = -static_cast<int>(offset);
    //             int_num++;
    //             continue;
    //         }
    //     }
    //    // if(float_num>=8||int_num>=8) break;
    // }
    context.origin_frame_size = offset + 8;
    // LOG_INFO << "Origin frame size: " << context.origin_frame_size;
    offset += (R_USABLE + FR_USABLE + 1) * 8;
    offset = ALIGN(offset, PROLOGUE_ALIGN);
    // 参数和 spill 的变量
    for (const auto &[val, spill_offset] : spill_int)
    {
        context.offset_map[val] = -static_cast<int>(offset + spill_offset);
        LOG_INFO << "Spilled integer " << val->get_name() << " at offset: "
                 << context.offset_map[val];
    }
    // 更新偏移
    offset += spill_int.size() * 8; // 根据需要调整大小
    for (const auto &[val, spill_offset] : spill_float)
    {
        context.offset_map[val] = -static_cast<int>(offset + spill_offset);
        LOG_INFO << "Spilled float " << val->get_name() << " at offset: "
                 << context.offset_map[val];
    }
    // 更新偏移
    offset += spill_float.size() * 8; // 根据需要调整大小
    // printf("\n\n12%d\n\n",offset);
    //  allocas
    for (auto &bb : func->get_basic_blocks())
    {
        for (auto &instr : bb.get_instructions())
        {
            if (instr.is_alloca())
            {
                auto size = instr.get_type()->get_size();
                offset = ALIGN(offset + size, size);
                // printf("13%d\n",offset);
                context.offset_map[&instr] = -static_cast<int>(offset);
                auto *alloca_inst = static_cast<AllocaInst *>(&instr);
                auto size1 = alloca_inst->get_alloca_type()->get_size();
                offset += size1;
                // printf("14%d\n",offset);
                context.array_start_offset[alloca_inst] = offset;
                // context.offset_map[alloca_inst] = -static_cast<int>(offset);
                LOG_INFO << "Allocated alloca variable " << alloca_inst->get_name()
                         << " at offset: " << context.offset_map[alloca_inst];
            }

            // 分析 phi_path，统计 max_cp_stmt
            if (instr.is_br())
            {
                auto *branchInst = static_cast<BranchInst *>(&instr);
                std::vector<BasicBlock *> targets;
                if (branchInst->is_cond_br())
                {
                    targets = {static_cast<BasicBlock *>(branchInst->get_operand(1)),
                               static_cast<BasicBlock *>(branchInst->get_operand(2))};
                }
                else
                {
                    targets = {static_cast<BasicBlock *>(branchInst->get_operand(0))};
                }
                for (auto *target : targets)
                {
                    auto p = std::make_pair(&bb, target);
                    if (context.phi_path1.find(p) != context.phi_path1.end())
                    {
                        context.max_cp_stmt = std::max(context.max_cp_stmt,
                                                       int(context.phi_path1.at(p).size()));
                    }
                }
            }
        }
    }
    // unsigned offset = PROLOGUE_OFFSET_BASE;
    if (offset % 8 != 0)
        offset += 4;
    // context.origin_frame_size = offset + 16;
    // // LOG_INFO << "Origin frame size: " << context.origin_frame_size;
    // offset += (R_USABLE + FR_USABLE + 4) * 8;
    // offset = ALIGN(offset, PROLOGUE_ALIGN);

    context.copy_stmt_start = offset;
    // LOG_INFO << "Copy statement start at offset: " << context.copy_stmt_start;
    offset += context.max_cp_stmt * 8;
    // LOG_INFO << "Max copy statements: " << context.max_cp_stmt;

    context.frame_size = ALIGN(offset + RA_int.get_stack_size() + RA_float.get_stack_size(),
                               PROLOGUE_ALIGN);
    // LOG_INFO << "Final frame size after allocation: " << context.frame_size;
}

void CodeGenRegister::gen_prologue()
{
    output.emplace_back("# prolog");
    // 分配栈空间，addi对应寄存器版本是add
    // makeSureInRange("addi", SP, SP, -context.frame_size, "add");
    // // 保存返回地址和帧指针，sd是存寄存器指令，寄存器版本同名
    // makeSureInRange("sd", RA_reg, SP, context.frame_size - 8, "sd");
    // makeSureInRange("sd", FP, SP, context.frame_size - 16, "sd");
    // 设置新的帧指针，fp = sp
    // 这里直接用 mv 更清晰，也不用考虑立即数范围
    // append_inst("mv fp, sp");
    if (IS_IMM_12(-static_cast<int>(context.frame_size)))
    {
        // 保存 ra 和 fp 到栈
        append_inst("sd", {"ra", "-8(sp)"});
        append_inst("sd", {"fp", "-16(sp)"});
        // 设置帧指针 fp = sp
        append_inst("addi", {"fp", "sp", "0"});
        // 栈指针下移
        append_inst("addi", {"sp", "sp", "-" + std::to_string(context.frame_size)});
    }
    else
    {
        // 栈帧大小大于 12-bit 立即数范围，先装载到寄存器 t0
        load_large_int64(context.frame_size, Reg::t(0));
        // 保存 ra 和 fp
        append_inst("sd", {"ra", "-8(sp)"});
        append_inst("sd", {"fp", "-16(sp)"});
        // sp = sp - t0
        append_inst("sub", {"sp", "sp", "t0"});
        // fp = sp + t0
        append_inst("add", {"fp", "sp", "t0"});
    }
}

void CodeGenRegister::gen_epilogue()
{
    append_inst(func_exit_label_name(context.func), ASMInstruction::Label);
    // output.emplace_back("# epilog");
    // // 恢复 ra 和 fp，ld是加载寄存器，寄存器版本同名
    // makeSureInRange("ld", RA_reg, SP, context.frame_size - 8, "ld");
    // makeSureInRange("ld", FP, SP, context.frame_size - 16, "ld");
    // // 回收栈空间
    // makeSureInRange("addi", SP, SP, context.frame_size, "add");
    // // 返回指令
    // append_inst("ret");
    // append_inst(".Lfunc_exit_" + context.func->get_name(), ASMInstruction::Label);

    if (IS_IMM_12(-static_cast<int>(context.frame_size)))
    {
        // frame_size 不大，直接恢复
        append_inst("addi", {"sp", "sp", std::to_string(context.frame_size)});
        append_inst("ld", {"fp", "-16(sp)"}); // 恢复旧帧指针
        append_inst("ld", {"ra", "-8(sp)"});  // 恢复返回地址
    }
    else
    {
        // frame_size 大，用寄存器恢复
        load_large_int64(context.frame_size, Reg::t(0));
        append_inst("add", {"sp", "sp", "t0"}); // 恢复栈指针
        append_inst("ld", {"fp", "-16(sp)"});   // 恢复旧帧指针
        append_inst("ld", {"ra", "-8(sp)"});    // 恢复返回地址
    }

    append_inst("ret"); // 返回调用者，RISC-V的返回伪指令
}

void CodeGenRegister::load_large_int32(int32_t val, const Reg &reg)
{
    append_inst("li", {reg.print(), std::to_string(val)});
}

void CodeGenRegister::load_large_int64(int64_t val, const Reg &reg)
{
    append_inst("li", {reg.print(), std::to_string(val)});
}

void CodeGenRegister::store_from_greg(Value *val, const Reg &reg)
{
    auto *type = val->get_type();
    MemOpType op;

    if (type->is_int1_type())
    {
        op = MemOpType::StoreByte;
    }
    else if (type->is_int32_type())
    {
        op = MemOpType::StoreInt;
    }
    else
    {
        op = MemOpType::StoreDouble; // pointer 或其他类型（假定为 64-bit）
    }

    mem_op(val, reg.print(), op, true);
}
void CodeGenRegister::store_from_greg_string(Value *val, const std::string &reg_name)
{
    auto *type = val->get_type();
    MemOpType op;

    if (type->is_int1_type())
    {
        op = MemOpType::StoreByte;
    }
    else if (type->is_int32_type())
    {
        op = MemOpType::StoreInt;
    }
    else
    {
        op = MemOpType::StoreDouble; // 假设 pointer 或更大整数是 64-bit
    }

    mem_op(val, reg_name, op, true);
}

void CodeGenRegister::load_to_freg(Value *val, const FReg &freg)
{
    assert(val->get_type()->is_float_type());

    if (auto *constant = dynamic_cast<ConstantFP *>(val))
    {
        // ✅ 处理浮点常量
        float imm = constant->get_value();
        load_float_imm(imm, freg);
    }
    else if (auto *gv = dynamic_cast<GlobalVariable *>(val))
    {
        // ✅ 处理全局浮点变量
        std::string label = gv->get_name();
        Reg tmp = Reg::t(0);                                   // 临时寄存器 t0
        append_inst(LOAD_ADDR, {tmp.print(), gv->get_name()}); // flw freg, 0(t0)
    }
    else
    {
        // ✅ 处理局部变量（栈上的浮点数）
        mem_op(val, freg.print(), MemOpType::LoadFloat);
    }
}
void CodeGenRegister::load_to_freg_string(Value *val, const std::string &freg)
{
    // assert(val->get_type()->is_float_type());

    if (auto *constant = dynamic_cast<ConstantFP *>(val))
    {
        // ✅ 处理浮点常量
        float imm = constant->get_value();
        int32_t imm1 = *reinterpret_cast<int32_t *>(&imm);

        // 1. 将 float 的 bit pattern 作为 int32_t 加载到临时整数寄存器（如 t0）
        auto tmp = Reg::t(0);        // 用 $t0 作为中转寄存器
        load_large_int32(imm1, tmp); // 会调用 lui+addi
        // 2. 用 fmv.w.x 将整数寄存器内容移动到浮点寄存器
        append_inst(FMV_W_X, {freg, tmp.print()});
    }
    else if (auto *gv = dynamic_cast<GlobalVariable *>(val))
    {
        // ✅ 处理全局浮点变量
        std::string label = gv->get_name();
        Reg tmp = Reg::t(0);                                   // 临时寄存器 t0
        append_inst(LOAD_ADDR, {tmp.print(), gv->get_name()}); // flw freg, 0(t0)
    }
    else
    {
        // ✅ 处理局部变量（栈上的浮点数）
        mem_op(val, freg, MemOpType::LoadFloat);
    }
}
void CodeGenRegister::load_float_imm(float val, const FReg &r)
{
    int32_t imm = *reinterpret_cast<int32_t *>(&val);

    // 1. 将 float 的 bit pattern 作为 int32_t 加载到临时整数寄存器（如 t0）
    auto tmp = Reg::t(0);       // 用 $t0 作为中转寄存器
    load_large_int32(imm, tmp); // 会调用 lui+addi
    // 2. 用 fmv.w.x 将整数寄存器内容移动到浮点寄存器
    append_inst(FMV_W_X, {r.print(), tmp.print()});
}

void CodeGenRegister::gen_ret()
{
    auto return_inst = static_cast<ReturnInst *>(context.inst);
    if (!return_inst->is_void_ret())
    {
        auto value = return_inst->get_operand(0);
        bool is_float = value->get_type()->is_float_type();
        std::string reg = value2reg(value); // 获取此值目前在哪个寄存器
        std::string suffix;
        if (is_float)
            suffix = "w"; // float
        else if (value->get_type()->get_size() == 8 || value->get_type()->is_pointer_type())
            suffix = "d"; // int64 或 pointer
        else if (value->get_type()->get_size() == 4)
            suffix = "w"; // int32
        else
            throw std::runtime_error("Unsupported store size");
        std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
        if (auto *arg = dynamic_cast<Argument *>(value))
        {
            int id = 0;
            for (auto &arg_test : arg->get_parent()->get_args())
            {
                if (&arg_test == arg)
                    break;
                id++;
            }
            // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
            // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
            int64_t offset = 8 * id;
            if (!IS_IMM_12(offset))
            {
                if (reg == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + reg + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + reg + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        else
        {
            if (!IS_IMM_12(context.offset_map[value]))
            {
                if (reg == "t0")
                {
                    if (is_float)
                        append_inst(inst2 + " " + "f" + reg + ", " + "0(t0)");
                    else
                        append_inst(inst2 + " " + reg + ", " + "0(t0)");
                } // 专门处理偏移过大的情况
            }
        }
        if (is_float && reg != "fa0")
        {
            // 浮点返回值 → 放入 fa0
            append_inst("fmv.s fa0, " + reg); // 单精度
            // 或 fmv.d fa0, reg  ← 如果是 double 类型，请根据类型改为 fmv.d
        }
        else if (!is_float && reg != "a0")
        {
            // 整型返回值 → 放入 a0
            append_inst("mv a0, " + reg); // 寄存器间 move
        }
    }
    else
    {
        // void 函数返回（可省略，也可置 0）
        append_inst("li a0, 0");
    }
    // 跳转到 epilogue 统一 return 出口
    append_inst("j " + func_exit_label_name(context.func));
}
// void CodeGenRegister::gen_ret() {
//     auto return_inst = static_cast<ReturnInst *>(context.inst);
//     if (!return_inst->is_void_ret()) {
//         auto value = return_inst->get_operand(0);
//         bool is_float = value->get_type()->is_float_type();
//         std::string reg = value2reg(value);  // 获取此值目前在哪个寄存器
//         if (is_float && reg != "fa0") {
//             // 浮点返回值 → 放入 fa0
//             append_inst("fmv.s fa0, " + reg);  // 单精度
//             // 或 fmv.d fa0, reg  ← 如果是 double 类型，请根据类型改为 fmv.d
//         } else if (!is_float && reg != "a0") {
//             // 整型返回值 → 放入 a0
//             append_inst("mv a0, " + reg);  // 寄存器间 move
//         }
//     } else {
//         // void 函数返回（可省略，也可置 0）
//         append_inst("li a0, 0");
//     }
//     // 跳转到 epilogue 统一 return 出口
//     append_inst("j " + func_exit_label_name(context.func));
// }

void CodeGenRegister::gen_br()
{
    auto branch_inst = static_cast<BranchInst *>(context.inst);

    if (branch_inst->is_cond_br())
    {
        // 条件跳转
        auto cond = branch_inst->get_operand(0); // 条件值
        auto true_bb = static_cast<BasicBlock *>(branch_inst->get_operand(1));
        auto false_bb = static_cast<BasicBlock *>(branch_inst->get_operand(2));
        context.true_label = label_name(true_bb);
        context.false_label = label_name(false_bb);

        // 判断cond的具体类型，分别调用生成跳转代码
        if (auto *icmp = dynamic_cast<ICmpInst *>(cond))
        {
            context.inst = icmp;
            gen_icmp_branch();
        }
        else if (auto *fcmp = dynamic_cast<FCmpInst *>(cond))
        {
            context.inst = fcmp;
            gen_fcmp_branch();
        }
        else
        {
            // 非比较指令，直接判断寄存器是否非零跳转
            std::string cond_reg = value2reg(cond);
            append_inst("bnez " + cond_reg + ", " + context.true_label);
            append_inst("j " + context.false_label);
        }
    }
    else
    {
        // 无条件跳转
        auto bb = static_cast<BasicBlock *>(branch_inst->get_operand(0));
        append_inst("j " + label_name(bb));
    }
}

// void CodeGenRegister::gen_binary()
// {
//     auto *binary_inst = static_cast<IBinaryInst *>(context.inst);
//     auto op = binary_inst->get_instr_type(); // 操作类型，例如 add, sub, mul, ...
//     auto lhs = binary_inst->get_operand(0);  // 左操作数
//     auto rhs = binary_inst->get_operand(1);  // 右操作数
//     auto dest = binary_inst;                 // 指令本身就是目标变量

//     bool is_float = lhs->get_type()->is_float_type();
//     std::string lhs_reg;
//     // 分配寄存器名（value2reg 会自动考虑栈/寄存器）
//     if (is_float)
//         lhs_reg = value2reg(lhs, 0);
//     else
//         lhs_reg = value2reg(lhs, 6);
//     std::string suffix;
//     if (is_float)
//         suffix = "w"; // float
//     else if (lhs->get_type()->get_size() == 8 || lhs->get_type()->is_pointer_type())
//         suffix = "d"; // int64 或 pointer
//     else if (lhs->get_type()->get_size() == 4)
//         suffix = "w"; // int32
//     else
//         throw std::runtime_error("Unsupported store size");
//     std::string inst = is_float ? ("fl" + suffix) : ("l" + suffix);
//     if (auto *arg = dynamic_cast<Argument *>(lhs))
//     {
//         int id = 0;
//         for (auto &arg_test : arg->get_parent()->get_args())
//         {
//             if (&arg_test == arg)
//                 break;
//             id++;
//         }
//         // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
//         // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
//         int64_t offset = 8 * id;
//         if (!IS_IMM_12(offset))
//         {
//             if (is_float)
//                 append_inst(inst + " " + "f" + lhs_reg + ", " + "0(t0)");
//             else
//                 append_inst(inst + " " + lhs_reg + ", " + "0(t0)");
//         }
//     }
//     else
//     {
//         if (!IS_IMM_12(context.offset_map[lhs]))
//         {
//             if (is_float)
//                 append_inst(inst + " " + "f" + lhs_reg + ", " + "0(t0)");
//             else
//                 append_inst(inst + " " + lhs_reg + ", " + "0(t0)");
//         }
//     }
//     std::string rhs_reg = value2reg(rhs, 1);
//     bool is_float1 = rhs->get_type()->is_float_type();
//     // std::string suffix;
//     if (is_float1)
//         suffix = "w"; // float
//     else if (rhs->get_type()->get_size() == 8 || rhs->get_type()->is_pointer_type())
//         suffix = "d"; // int64 或 pointer
//     else if (rhs->get_type()->get_size() == 4)
//         suffix = "w"; // int32
//     else
//         throw std::runtime_error("Unsupported store size");
//     std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
//     if (auto *arg = dynamic_cast<Argument *>(rhs))
//     {
//         int id = 0;
//         for (auto &arg_test : arg->get_parent()->get_args())
//         {
//             if (&arg_test == arg)
//                 break;
//             id++;
//         }
//         // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
//         // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
//         int64_t offset = 8 * id;
//         if (!IS_IMM_12(offset))
//         {
//             if (rhs_reg == "t1")
//             {
//                 if (is_float)
//                     append_inst(inst1 + " " + "f" + rhs_reg + ", " + "0(t0)");
//                 else
//                     append_inst(inst1 + " " + rhs_reg + ", " + "0(t0)");
//             }
//         }
//     }
//     else
//     {
//         if (!IS_IMM_12(context.offset_map[rhs]))
//         {
//             if (rhs_reg == "t1")
//             {
//                 if (is_float)
//                     append_inst(inst1 + " " + "f" + rhs_reg + ", " + "0(t0)");
//                 else
//                     append_inst(inst1 + " " + rhs_reg + ", " + "0(t0)");
//             }
//         }
//     }
//     auto [dest_reg, find] = getRegName(dest, 0);
//     if (lhs_reg == "t0")
//         lhs_reg = "t6"; // 这里要区分好t0和t6
//     if (!find)
//         dest_reg = is_float ? "ft6" : "t6"; // 目标寄存器
//     // 生成汇编代码
//     if (!is_float)
//     {
//         switch (op)
//         {
//         case Instruction::add:
//             append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::sub:
//             append_inst("subw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::mul:
//             append_inst("mulw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::sdiv:
//             append_inst("divw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::srem:
//             append_inst("remw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::sand:
//             append_inst("and " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::shl:
//             append_inst("shlw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::ashr:
//             append_inst("sraw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         default:
//             assert(false && "Unsupported integer binary operation.");
//         }
//     }
//     else
//     {
//         // 浮点运算（假设是单精度 float）
//         std::string fop;
//         switch (op)
//         {
//         case Instruction::fadd:
//             fop = "fadd.s";
//             break;
//         case Instruction::fsub:
//             fop = "fsub.s";
//             break;
//         case Instruction::fmul:
//             fop = "fmul.s";
//             break;
//         case Instruction::fdiv:
//             fop = "fdiv.s";
//             break;
//         default:
//             assert(false && "Unsupported float binary operation.");
//         }
//         append_inst(fop + " " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//     }
//     // 额外：如果目标是栈上的变量，需要写回内存
//     if (context.offset_map.count(dest) != 0)
//     {
//         // 获取目标变量的内存地址字符串，比如 "fp, -308"
//         std::string addr = getMemAddr(dest);
//         // 生成存指令，整型用 sw，浮点用 fsw
//         if (!is_float)
//         {
//             append_inst("sw", {dest_reg, addr});
//         }
//         else
//         {
//             append_inst("fsw", {dest_reg, addr});
//         }
//     }
// }

void CodeGenRegister::gen_binary() {
    auto *binary_inst = static_cast<IBinaryInst *>(context.inst);
    auto op = binary_inst->get_instr_type();        // 操作类型，例如 add, sub, mul, ...
    auto lhs = binary_inst->get_operand(0);         // 左操作数
    auto rhs = binary_inst->get_operand(1);         // 右操作数
    auto dest = binary_inst;                        // 指令本身就是目标变量
    std::cout<<binary_inst->print()<<std::endl;
    bool is_float = lhs->get_type()->is_float_type();
    std::string lhs_reg;
    // 分配寄存器名（value2reg 会自动考虑栈/寄存器）
    if(is_float) lhs_reg = value2reg(lhs, 0);
    else lhs_reg = value2reg(lhs, 6);
    std::string suffix;
    if (is_float)
        suffix = "w";  // float
    else if (lhs->get_type()->get_size() == 8||lhs->get_type()->is_pointer_type())
        suffix = "d";  // int64 或 pointer
    else if (lhs->get_type()->get_size() == 4)
        suffix = "w";  // int32
    else
        throw std::runtime_error("Unsupported store size");
    std::string inst = is_float ? ("fl" + suffix) : ("l" + suffix);
    if(auto *arg = dynamic_cast<Argument *>(lhs)){
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args()) {
                if (&arg_test == arg)
                break;
                id++;
        }
            //std::cerr << "This value is an argument. Argument index = " << id << std::endl;
            //int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
        int64_t offset = 8*id;
        if(!IS_IMM_12(offset)){
            if(is_float) append_inst(inst + " " +"f"+ lhs_reg + ", " + "0(t0)");
            else append_inst(inst + " " + lhs_reg + ", " + "0(t0)");
        }
     }else{
        if(!IS_IMM_12(context.offset_map[lhs])){
            if(is_float) append_inst(inst + " " +"f"+ lhs_reg + ", " + "0(t0)");
            else append_inst(inst + " " + lhs_reg + ", " + "0(t0)");
        }
    }
    bool can_imm;
    switch (op) {
        case Instruction::add:
            //append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
            can_imm=true;
            break;
        case Instruction::sub:
           // append_inst("subw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
            can_imm=false;
            break;
        case Instruction::mul:
           // append_inst("mulw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
           can_imm=false; 
           break;
        case Instruction::sdiv:
            //append_inst("divw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
            can_imm=false;
            break;
        case Instruction::srem:
           // append_inst("remw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
           can_imm=false; 
           break;
        case Instruction::fadd:
            //op = "fadd.s"; break;
            can_imm=false;
            break;
        case Instruction::fsub:
           // op = "fsub.s"; break;
           can_imm=false;
           break;
        case Instruction::fmul:
           // op = "fmul.s"; break;
           can_imm=false;
           break;
        case Instruction::fdiv:
           // op = "fdiv.s"; break;
           can_imm=false;
           break;
        case Instruction::sand:
           // op = "fdiv.s"; break;
           can_imm=true;
           break;
        case Instruction::shl:
           // op = "fdiv.s"; break;
           can_imm=true;
           break;
        case Instruction::ashr:
        // op = "fdiv.s"; break;
            can_imm=true;
           break;
        default:
            assert(false && "Unsupported float binary operation.");
    }
    bool can_imm1=false;
    if(dynamic_cast<ConstantInt *>(rhs)){
        auto *is_big=dynamic_cast<ConstantInt *>(rhs);
        int imm=std::stoi(is_big->print());
        if(IS_IMM_12(imm)){
            can_imm1=true;
        }else{
            can_imm1=false;
        }
    }
    if(!dynamic_cast<ConstantInt *>(rhs)||!can_imm||!can_imm1){
        std::string rhs_reg = value2reg(rhs, 1);
        bool is_float1 = rhs->get_type()->is_float_type();
        // std::string suffix;
        if (is_float1)
            suffix = "w";  // float
        else if (rhs->get_type()->get_size() == 8||rhs->get_type()->is_pointer_type())
            suffix = "d";  // int64 或 pointer
        else if (rhs->get_type()->get_size() == 4)
            suffix = "w";  // int32
        else
            throw std::runtime_error("Unsupported store size");
        std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
        if(auto *arg = dynamic_cast<Argument *>(rhs)){
            int id = 0;
            for (auto &arg_test : arg->get_parent()->get_args()) {
                if (&arg_test == arg)
                break;
                id++;
            }
                //std::cerr << "This value is an argument. Argument index = " << id << std::endl;
                //int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
            int64_t offset = 8*id;
            if(!IS_IMM_12(offset)){
                if(rhs_reg=="t1") {
                    if(is_float) append_inst(inst1 + " " +"f"+ rhs_reg + ", " + "0(t0)");
                    else append_inst(inst1 + " " + rhs_reg + ", " + "0(t0)");
                }
            }
        }else{
            if(!IS_IMM_12(context.offset_map[rhs])){
                if(rhs_reg=="t1") {
                    if(is_float) append_inst(inst1 + " " +"f"+ rhs_reg + ", " + "0(t0)");
                    else append_inst(inst1 + " " + rhs_reg + ", " + "0(t0)");
                }
            }
        }
        auto [dest_reg, find] = getRegName(dest, 0);
        if(lhs_reg=="t0") lhs_reg="t6"; //这里要区分好t0和t6
        if(!find) dest_reg = is_float?"ft6":"t6"; // 目标寄存器
        // 生成汇编代码
        if (!is_float) {
            switch (op) {
            case Instruction::add:
                append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::sub:
                append_inst("subw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::mul:
                append_inst("mulw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::sdiv:
                append_inst("divw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::srem:
                append_inst("remw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::sand:
                append_inst("and " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::shl:
                append_inst("sllw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            case Instruction::ashr:
                append_inst("sraw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                break;
            default:
                assert(false && "Unsupported integer binary operation.");
            }
        } else {
            // 浮点运算（假设是单精度 float）
            std::string fop;
            switch (op) {
            case Instruction::fadd:
                fop = "fadd.s"; break;
            case Instruction::fsub:
                fop = "fsub.s"; break;
            case Instruction::fmul:
                fop = "fmul.s"; break;
            case Instruction::fdiv:
                fop = "fdiv.s"; break;
            default:
                assert(false && "Unsupported float binary operation.");
            }
            append_inst(fop + " " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
        }
        // 额外：如果目标是栈上的变量，需要写回内存
        if (context.offset_map.count(dest) != 0) {
            // 获取目标变量的内存地址字符串，比如 "fp, -308"
            std::string addr = getMemAddr(dest);
            // 生成存指令，整型用 sw，浮点用 fsw
            if (!is_float) {
                append_inst("sw", {dest_reg, addr});
            } else {
                append_inst("fsw", {dest_reg, addr});
            }
        }
    }else{
        auto [dest_reg, find] = getRegName(dest, 0);
        if(lhs_reg=="t0") lhs_reg="t6"; //这里要区分好t0和t6
        if(!find) dest_reg = is_float?"ft6":"t6"; // 目标寄存器
        // 生成汇编代码
        switch (op) {
            std::cout<<op<<std::endl;
            std::cout<<binary_inst->print()<<std::endl;
            case Instruction::add:{
                //append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                auto *is_big=dynamic_cast<ConstantInt *>(rhs);
                append_inst("addiw "+ dest_reg + ", " + lhs_reg + ", " + is_big->print());
                break;
            }
            case Instruction::sand:{
                //append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                auto *is_big=dynamic_cast<ConstantInt *>(rhs);
                append_inst("andi "+ dest_reg + ", " + lhs_reg + ", " + is_big->print());
                break;
            }
            case Instruction::shl:{
                //append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                auto *is_big=dynamic_cast<ConstantInt *>(rhs);
                append_inst("slliw "+ dest_reg + ", " + lhs_reg + ", " + is_big->print());
                break;
            }
            case Instruction::ashr:{
                //append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
                auto *is_big=dynamic_cast<ConstantInt *>(rhs);
                append_inst("sraiw "+ dest_reg + ", " + lhs_reg + ", " + is_big->print());
                break;
            }
            default:
                assert(false && "Unsupported integer binary operation.");
        }
        // 额外：如果目标是栈上的变量，需要写回内存
        if (context.offset_map.count(dest) != 0) {
            // 获取目标变量的内存地址字符串，比如 "fp, -308"
            std::string addr = getMemAddr(dest);
            // 生成存指令，整型用 sw，浮点用 fsw
            if (!is_float) {
                append_inst("sw", {dest_reg, addr});
            } else {
                append_inst("fsw", {dest_reg, addr});
            }
        }
    }
}


// void CodeGenRegister::gen_binary() {
//     auto *binary_inst = static_cast<IBinaryInst *>(context.inst);
//     auto op = binary_inst->get_instr_type();        // 操作类型，例如 add, sub, mul, ...
//     auto lhs = binary_inst->get_operand(0);         // 左操作数
//     auto rhs = binary_inst->get_operand(1);         // 右操作数
//     auto dest = binary_inst;                        // 指令本身就是目标变量
//     bool is_float = lhs->get_type()->is_float_type();
//     // 分配寄存器名（value2reg 会自动考虑栈/寄存器）
//     std::string lhs_reg = value2reg(lhs, 0);
//     std::string rhs_reg = value2reg(rhs, 1);
//     auto [dest_reg, find] = getRegName(dest, 0);
//     if(!find) dest_reg = is_float?"ft6":"t6"; // 目标寄存器
//     // 生成汇编代码
//     if (!is_float) {
//         switch (op) {
//         case Instruction::add:
//             append_inst("addw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::sub:
//             append_inst("subw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::mul:
//             append_inst("mulw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::sdiv:
//             append_inst("divw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         case Instruction::srem:
//             append_inst("remw " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//             break;
//         default:
//             assert(false && "Unsupported integer binary operation.");
//         }
//     } else {
//         // 浮点运算（假设是单精度 float）
//         std::string fop;
//         switch (op) {
//         case Instruction::fadd:
//             fop = "fadd.s"; break;
//         case Instruction::fsub:
//             fop = "fsub.s"; break;
//         case Instruction::fmul:
//             fop = "fmul.s"; break;
//         case Instruction::fdiv:
//             fop = "fdiv.s"; break;
//         default:
//             assert(false && "Unsupported float binary operation.");
//         }
//         append_inst(fop + " " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
//     }
//     // 额外：如果目标是栈上的变量，需要写回内存
//     if (context.offset_map.count(dest) != 0) {
//         // 获取目标变量的内存地址字符串，比如 "fp, -308"
//         std::string addr = getMemAddr(dest);
//         // 生成存指令，整型用 sw，浮点用 fsw
//         if (!is_float) {
//             append_inst("sw", {dest_reg, addr});
//         } else {
//             append_inst("fsw", {dest_reg, addr});
//         }
//     }
// }

void CodeGenRegister::gen_alloca()
{
    auto *alloca_inst = static_cast<AllocaInst *>(context.inst);
    // output.emplace_back("# alloca " + alloca_inst->get_name() +
    //                     " at fp - " + std::to_string(offset));
    //  alloca 分配出来的实际地址
    int alloc_addr_offset = context.offset_map[alloca_inst];
    auto alloc_size = alloca_inst->get_alloca_type()->get_size();
    // alloca 指令本身（即那个指针变量）对应的位置
    // result_offset这种算法经过我的严密验证，就是这样算的
    int result_offset = context.offset_map[context.inst] - alloc_size;

    // 将分配地址装到寄存器 t0
    if (IS_IMM_12(result_offset))
    {
        append_inst("addi t0, fp, " + std::to_string(result_offset));
    }
    else
    {
        load_large_int64(result_offset, Reg::t(0));
        append_inst("add t0, fp, t0");
    }

    // 把分配地址写到指令自身（即“指针变量”）位置
    if (IS_IMM_12(alloc_addr_offset))
    {
        append_inst("sd t0, " + std::to_string(alloc_addr_offset) + "(fp)");
    }
    else
    {
        load_large_int64(alloc_addr_offset, Reg::t(1));
        append_inst("add t1, fp, t1");
        append_inst("sd t0, 0(t1)");
    }
}

void CodeGenRegister::gen_load()
{
    // auto *ptr = context.inst->get_operand(0);
    auto *type = context.inst->get_type();
    auto load_inst = static_cast<LoadInst *>(context.inst);
    auto ptr = load_inst->get_lval();                 // 获取被加载的指针
    auto [dest_reg, find] = getRegName(context.inst); // 加载目标值寄存器
    // auto [dest_reg1, find1] = getRegName(ptr);
    // auto *type = context.inst->get_type();
    if (find)
    {
        if (type->is_float_type())
        {
            // ptr 是 float* 类型，不是 float，不能直接 load_to_freg
            Reg addr = Reg::t(0);
            load_to_greg(ptr, addr); // 加载地址

            // FReg freg = FReg::ft(0);
            append_inst("flw " + dest_reg + ", 0(" + addr.print() + ")"); // 取值
            // store_from_freg(context.inst, freg); // 存入 context.inst 对应位置
        }
        else
        {
            Reg addr = Reg::t(0);
            load_to_greg(ptr, addr); //  加载地址
            // Reg val = Reg::t(1);
            if (type->is_integer_type())
            {
                append_inst("lw " + dest_reg + ", 0(" + addr.print() + ")"); // 取值
            }
            else
            {
                append_inst("ld " + dest_reg + ", 0(" + addr.print() + ")");
            }
            // store_from_greg(context.inst, val); // 存入 context.inst 对应位置
        }
    }
    if (!find)
    {
        // Reg dest_reg1=type->is_float_type()?FReg::ft(0):Reg::t(6);
        if (type->is_float_type())
        {
            // ptr 是 float* 类型，不是 float，不能直接 load_to_freg
            Reg addr = Reg::t(0);
            FReg dest_reg1 = FReg::ft(0);
            load_to_greg(ptr, addr); // 加载地址

            // FReg freg = FReg::ft(0);
            append_inst("flw " + dest_reg1.print() + ", 0(" + addr.print() + ")"); // 取值
            store_from_freg(context.inst, dest_reg1);                              // 存入 context.inst 对应位置
        }
        else
        {
            Reg addr = Reg::t(0);
            Reg dest_reg1 = Reg::t(6);
            load_to_greg(ptr, addr); //  加载地址
            // Reg val = Reg::t(1);
            if (type->is_integer_type())
            {
                append_inst("lw " + dest_reg1.print() + ", 0(" + addr.print() + ")"); // 取值
            }
            else
            {
                append_inst("ld " + dest_reg1.print() + ", 0(" + addr.print() + ")");
            }
            store_from_greg(context.inst, dest_reg1); // 存入 context.inst 对应位置
        }
    }
}
// void CodeGenRegister::gen_load() {
//    // auto *ptr = context.inst->get_operand(0);
//     auto *type = context.inst->get_type();
//     auto load_inst = static_cast<LoadInst *>(context.inst);
//     auto ptr = load_inst->get_lval();  // 获取被加载的指针
//     auto [dest_reg, find] = getRegName(context.inst);  // 加载目标值寄存器
//     //auto [dest_reg1, find1] = getRegName(ptr);
//     //auto *type = context.inst->get_type();
//     if(find){
//         if (type->is_float_type()) {
//             // ptr 是 float* 类型，不是 float，不能直接 load_to_freg
//             load_to_freg_string(ptr, dest_reg); // 加载地址
//         } else {
//             load_to_greg_string(ptr, dest_reg);
//         //store_from_greg(context.inst, val); // 存入 context.inst 对应位置
//         }
//     }
//     if(!find){
//         //Reg dest_reg1=type->is_float_type()?FReg::ft(0):Reg::t(6);
//         if (type->is_float_type()) {
//             // ptr 是 float* 类型，不是 float，不能直接 load_to_freg
//             load_to_freg_string(ptr, dest_reg); // 加载地址
//             FReg dest_reg1=FReg::ft(0);
//             store_from_freg(context.inst, dest_reg1); // 存入 context.inst 对应位置
//         } else {
//             load_to_greg_string(ptr, dest_reg);
//             Reg dest_reg1=Reg::t(0);
//             store_from_greg(context.inst, dest_reg1); // 存入 context.inst 对应位置
//         }
//     }
// }

// void CodeGenRegister::gen_store() {
//     auto *store_inst = static_cast<StoreInst *>(context.inst);
//     auto *rval = store_inst->get_rval();  // 要存的值
//     auto *lval = store_inst->get_lval();  // 存到哪里
//     bool is_float = rval->get_type()->is_float_type();
//     bool is_pointer = rval->get_type()->is_pointer_type();
//     // 判断数据大小（float=4, double/pointer=8）
//     std::string suffix;
//     if (is_float)
//         suffix = "w";  // float
//     else if (rval->get_type()->get_size() == 8||rval->get_type()->is_pointer_type())
//         suffix = "d";  // int64 或 pointer
//     else if (rval->get_type()->get_size() == 4)
//         suffix = "w";  // int32
//     else
//         throw std::runtime_error("Unsupported store size");
//     // 确定使用哪个汇编指令
//     std::string inst = is_float ? ("fs" + suffix) : ("s" + suffix);
//     std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
//     //int offset = context.offset_map[lval];
//     std::string inst1="ld";
//     //append_inst(inst1+ " " + "t6" + ", " + std::to_string(offset) + "(fp)");
//     //auto [dest_reg, find] = getRegName(context.inst);
//     std::string src_reg = value2reg(rval, 0); // 获取源值所在寄存器（可能生成 load），此处若没有寄存器，不会进行load操作。
//     if (auto *gep = dynamic_cast<GetElementPtrInst *>(lval)) {
//         // 如果是 GEP，先计算地址
//         std::string addr_reg = value2reg(lval, 1);
//         append_inst(inst + " " + src_reg + ", 0(" + addr_reg + ")");
//     } else if (auto *global_var = dynamic_cast<GlobalVariable *>(lval)) {
//         // 全局变量：la + store
//         append_inst("la t1, " + global_var->get_name());
//         append_inst(inst + " " + src_reg + ", 0(t1)");
//     } else {
//         // 普通局部变量（包括 spill 变量和 alloca）
//         if (context.offset_map.count(lval) == 0) {
//             std::cerr << "[ERROR] Invalid store target: lval has no offset.\n";
//             std::cerr << "  lval name: " << lval->get_name() << "\n";
//             std::cerr << "  lval type: " << lval->get_type()->print() << "\n";
//             throw std::runtime_error("store target not in offset_map");
//         }
//         int offset = context.offset_map[lval];
//         if(is_float){
//             if (offset >= -2048 && offset<= 2047) {
//                 append_inst(inst1+ " " + "t6" + ", " + std::to_string(offset) + "(fp)");
//                 append_inst(inst + " " + src_reg + ", " + "0(t6)");
//             } else {
//                 append_inst("li", {"t1", std::to_string(offset)});
//                 append_inst("add", {"t1", "fp", "t1"});
//                 append_inst(inst + " " + src_reg + ", 0(t1)");
//             }
//         }else{
//             if (offset >= -2048 && offset<= 2047) {
//                 append_inst(inst1+ " " + "t6" + ", " + std::to_string(offset) + "(fp)");
//                 append_inst(inst + " " + src_reg + ", " + "0(t6)");
//             } else {
//                 append_inst("li", {"t1", std::to_string(offset)});
//                 append_inst("add", {"t1", "fp", "t1"});
//                 append_inst(inst + " " + src_reg + ", 0(t1)");
//             }
//         }
//     }
// }
void CodeGenRegister::gen_store()
{
    auto *store_inst = static_cast<StoreInst *>(context.inst);
    auto *rval = store_inst->get_rval(); // 要存的值
    auto *lval = store_inst->get_lval(); // 存到哪里
    bool is_float = rval->get_type()->is_float_type();
    bool is_pointer = rval->get_type()->is_pointer_type();
    // 判断数据大小（float=4, double/pointer=8）
    std::string suffix;
    if (is_float)
        suffix = "w"; // float
    else if (rval->get_type()->get_size() == 8 || rval->get_type()->is_pointer_type())
        suffix = "d"; // int64 或 pointer
    else if (rval->get_type()->get_size() == 4)
        suffix = "w"; // int32
    else
        throw std::runtime_error("Unsupported store size");
    // 确定使用哪个汇编指令
    std::string inst = is_float ? ("fs" + suffix) : ("s" + suffix);
    std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
    // int offset = context.offset_map[lval];
    std::string inst1 = "ld";
    // append_inst(inst1+ " " + "t6" + ", " + std::to_string(offset) + "(fp)");
    // auto [dest_reg, find] = getRegName(context.inst);
    std::string src_reg = value2reg(rval, 0); // 获取源值所在寄存器（可能生成 load），此处若没有寄存器，不会进行load操作。
    if (auto *arg = dynamic_cast<Argument *>(rval))
    {
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args())
        {
            if (&arg_test == arg)
                break;
            id++;
        }
        // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
        // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
        int64_t offset = 8 * id;
        if (!IS_IMM_12(offset))
        {
            if (src_reg == "t0")
            {
                if (is_float)
                    append_inst(inst2 + " " + "f" + src_reg + ", " + "0(t0)");
                else
                    append_inst(inst2 + " " + src_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    else
    {
        if (!IS_IMM_12(context.offset_map[rval]))
        {
            if (src_reg == "t0")
            {
                if (is_float)
                    append_inst(inst2 + " " + "f" + src_reg + ", " + "0(t0)");
                else
                    append_inst(inst2 + " " + src_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    if (auto *gep = dynamic_cast<GetElementPtrInst *>(lval))
    {
        // 如果是 GEP，先计算地址
        std::string addr_reg = value2reg(lval, 1);
        append_inst(inst + " " + src_reg + ", 0(" + addr_reg + ")");
    }
    else if (auto *global_var = dynamic_cast<GlobalVariable *>(lval))
    {
        // 全局变量：la + store
        append_inst("la t1, " + global_var->get_name());
        append_inst(inst + " " + src_reg + ", 0(t1)");
    }
    else
    {
        // 普通局部变量（包括 spill 变量和 alloca）
        if (context.offset_map.count(lval) == 0)
        {
            // std::cerr << "[ERROR] Invalid store target: lval has no offset.\n";
            // std::cerr << "  lval name: " << lval->get_name() << "\n";
            // std::cerr << "  lval type: " << lval->get_type()->print() << "\n";
            throw std::runtime_error("store target not in offset_map");
        }
        int offset = context.offset_map[lval];
        if (is_float)
        {
            if (offset >= -2048 && offset <= 2047)
            {
                append_inst(inst1 + " " + "t6" + ", " + std::to_string(offset) + "(fp)");
                append_inst(inst + " " + src_reg + ", " + "0(t6)");
            }
            else
            {
                append_inst("li", {"t1", std::to_string(offset)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst(inst + " " + src_reg + ", 0(t1)");
            }
        }
        else
        {
            if (offset >= -2048 && offset <= 2047)
            {
                append_inst(inst1 + " " + "t6" + ", " + std::to_string(offset) + "(fp)");
                append_inst(inst + " " + src_reg + ", " + "0(t6)");
            }
            else
            {
                append_inst("li", {"t1", std::to_string(offset)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst(inst + " " + src_reg + ", 0(t1)");
            }
        }
    }
}

void CodeGenRegister::gen_bitcast()
{
    auto *inst = static_cast<BitCastInst *>(context.inst);
    Value *src = inst->get_operand(0);
    Type *dst_ty = inst->get_type();
//  printf("\n1958\n");
    // 查找目标寄存器映射（或栈偏移）
    auto [dest_reg, found_dest] = getRegName(inst, 0);
    // 源操作数寄存器查找
    auto [src_reg, found_src] = getRegName(src, 0);
    if (!found_dest && !found_src)
    {
        // 如果之前没分配寄存器，尝试找寄存器或者用栈偏移
        if (context.offset_map.count(inst) && context.offset_map.count(src))
        {
            int offset_inst = context.offset_map[inst];
            int offset_src = context.offset_map[src];
            // 把内存地址加载到某个寄存器（这里示例用t0）
            if (IS_IMM_12(offset_src))
            {
                append_inst("ld t0 , " + std::to_string(offset_src) + "(fp)");
            }
            else
            {
                append_inst("li", {"t1 ,", std::to_string(offset_src)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst("ld t0, 0(t1)");
            }
            if (IS_IMM_12(offset_inst))
            {
                append_inst("sd t0 , " + std::to_string(offset_inst) + "(fp)");
            }
            else
            {
                append_inst("li", {"t1 ,", std::to_string(offset_inst)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst("sd t0, 0(t1)");
            }
            // if(IS_IMM_12(offset_src)){
            //     append_inst("sd t0 , " + std::to_string(offset_src)+"(fp)");
            // }else{
            //     append_inst("li", {"t1 ,", std::to_string(offset_src)});
            //     append_inst("add", {"t1", "fp", "t1"});
            //     append_inst("sd t0, 0(t1)");
            // }

            // mapReg(inst, regname_to_id("t0"), dst_ty->is_float_type());
        }
        else
        {
            std::cerr << "[bitcast] 目标变量没有寄存器或栈偏移\n";
            return;
        }
    }
    else if (found_dest && !found_src)
    {
        // 如果之前没分配寄存器，尝试找寄存器或者用栈偏移
        if (context.offset_map.count(src))
        {
            // int offset_inst = context.offset_map[inst];
            int offset_src = context.offset_map[src];
            // 把内存地址加载到某个寄存器（这里示例用t0）
            if (IS_IMM_12(offset_src))
            {
                append_inst("ld " + dest_reg + "," + std::to_string(offset_src) + "(fp)");
            }
            else
            {
                append_inst("li", {"t1", std::to_string(offset_src)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst("ld " + dest_reg + ", 0(t1)");
            }
            // mapReg(inst, regname_to_id("t0"), dst_ty->is_float_type());
        }
        else
        {
            std::cerr << "[bitcast] 目标变量没有寄存器或栈偏移\n";
            return;
        }
    }
    else if (found_dest && found_src)
    {
        append_inst("mv " + dest_reg + ", " + src_reg);
    }
    else
    {
        if (context.offset_map.count(inst))
        {
            // int offset_inst = context.offset_map[inst];
            int offset_src = context.offset_map[inst];
            // 把内存地址加载到某个寄存器（这里示例用t0）
            if (IS_IMM_12(offset_src))
            {
                append_inst("sd " + src_reg + "," + std::to_string(offset_src) + "(fp)");
            }
            else
            {
                append_inst("li", {"t1", std::to_string(offset_src)});
                append_inst("add", {"t1", "fp", "t1"});
                append_inst("sd " + src_reg + ", 0(t1)");
            }
            // mapReg(inst, regname_to_id("t0"), dst_ty->is_float_type());
        }
        else
        {
            std::cerr << "[bitcast] 目标变量没有寄存器或栈偏移\n";
            return;
        }
    }
//  printf("\n1958\n");
}

int CodeGenRegister::regname_to_id(const std::string &name)
{
    if (name == "zero")
        return 0;
    if (name == "ra")
        return 1;
    if (name == "sp")
        return 2;
    if (name == "gp")
        return 3;
    if (name == "tp")
        return 4;

    if (name[0] == 't')
    {
        int tid = std::stoi(name.substr(1));
        if (tid <= 2)
            return tid + 5; // t0~t2 -> x5~x7
        else
            return tid + 25; // t3~t6 -> x28~x31
    }

    if (name[0] == 's')
    {
        int sid = std::stoi(name.substr(1));
        if (sid <= 1)
            return sid + 8; // s0~s1 -> x8~x9
        else
            return sid + 16; // s2~s11 -> x18~x27
    }

    if (name[0] == 'a')
    {
        int aid = std::stoi(name.substr(1));
        return aid + 10; // a0~a7 -> x10~x17
    }

    if (name[0] == 'x')
    {
        return std::stoi(name.substr(1)); // xN -> N
    }

    throw std::invalid_argument("Unknown register name: " + name);
}
int CodeGenRegister::fregname_to_id(const std::string &name)
{
    if (name.substr(0, 2) == "ft")
    {
        int tid = std::stoi(name.substr(2));
        if (tid <= 7)
            return tid; // ft0 ~ ft7 → 0 ~ 7
        else
            return tid + 20; // ft8 ~ ft11 → 28 ~ 31
    }
    if (name.substr(0, 2) == "fs")
    {
        int sid = std::stoi(name.substr(2));
        if (sid <= 1)
            return sid + 8; // fs0 ~ fs1 → 8, 9
        else
            return sid + 16; // fs2 ~ fs11 → 18 ~ 27
    }
    if (name.substr(0, 2) == "fa")
    {
        int aid = std::stoi(name.substr(2)); // fa0 ~ fa7 → 10 ~ 17
        return aid + 10;
    }
    throw std::invalid_argument("Unknown FReg name: " + name);
}

void CodeGenRegister::mapReg(Value *v, int reg_id, bool is_float)
{
    if (is_float)
    {
        RA_float.assign(v, reg_id);
    }
    else
    {
        RA_int.assign(v, reg_id);
    }
}

void CodeGenRegister::gen_icmp_branch()
{
    auto icmp = static_cast<ICmpInst *>(context.inst);
    // 1. 触发生成比较指令，并获取结果值所在寄存器
    std::string cond_reg = value2reg(icmp, 0); // 比如结果存在 a0
    // 2. 生成跳转指令，直接判断这个结果寄存器是否非零
    append_inst("bnez " + cond_reg + ", " + context.true_label);
    append_inst("j " + context.false_label);
}

void CodeGenRegister::gen_icmp_expr()
{
    auto &instr = *context.inst;
    auto lhs = instr.get_operand(0);
    auto rhs = instr.get_operand(1);
    std::string lhs_reg = value2reg(lhs, 1);
    std::string suffix;
    bool is_float1 = lhs->get_type()->is_float_type();
    // std::string suffix;
    if (is_float1)
        suffix = "w"; // float
    else if (lhs->get_type()->get_size() == 8 || lhs->get_type()->is_pointer_type())
        suffix = "d"; // int64 或 pointer
    else if (lhs->get_type()->get_size() == 4)
        suffix = "w"; // int32
    else if (lhs->get_type()->get_size() == 1)
        suffix = "b"; 
    else
        throw std::runtime_error("Unsupported store size");
    std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
    if (auto *arg = dynamic_cast<Argument *>(lhs))
    {
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args())
        {
            if (&arg_test == arg)
                break;
            id++;
        }
        // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
        // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
        int64_t offset = 8 * id;
        if (!IS_IMM_12(offset))
        {
            if (lhs_reg == "t1")
            {
                // append_inst(inst1 + " " + lhs_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
                if (is_float1)
                    append_inst(inst1 + " " + "f" + lhs_reg + ", " + "0(t0)");
                else
                    append_inst(inst1 + " " + lhs_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    else
    {
        if (!IS_IMM_12(context.offset_map[lhs]))
        {
            if (lhs_reg == "t1")
            {
                if (is_float1)
                    append_inst(inst1 + " " + "f" + lhs_reg + ", " + "0(t0)");
                else
                    append_inst(inst1 + " " + lhs_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    std::string rhs_reg = value2reg(rhs, 0);
    // std::string suffix;
    bool is_float2 = rhs->get_type()->is_float_type();
    // std::string suffix;
    if (is_float2)
        suffix = "w"; // float
    else if (rhs->get_type()->get_size() == 8 || rhs->get_type()->is_pointer_type())
        suffix = "d"; // int64 或 pointer
    else if (rhs->get_type()->get_size() == 4)
        suffix = "w"; // int32
    else if (rhs->get_type()->get_size() == 1)
        suffix = "b"; 
    else
        throw std::runtime_error("Unsupported store size");
    std::string inst2 = is_float2 ? ("fl" + suffix) : ("l" + suffix);
    if (auto *arg = dynamic_cast<Argument *>(rhs))
    {
        int id = 0;
        for (auto &arg_test : arg->get_parent()->get_args())
        {
            if (&arg_test == arg)
                break;
            id++;
        }
        // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
        // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
        int64_t offset = 8 * id;
        if (!IS_IMM_12(offset))
        {
            if (rhs_reg == "t0")
            {
                // append_inst(inst2 + " " + rhs_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
                if (is_float2)
                    append_inst(inst2 + " " + "f" + rhs_reg + ", " + "0(t0)");
                else
                    append_inst(inst2 + " " + rhs_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    else
    {
        if (!IS_IMM_12(context.offset_map[rhs]))
        {
            if (rhs_reg == "t0")
            {
                // append_inst(inst2 + " " + rhs_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
                if (is_float2)
                    append_inst(inst2 + " " + "f" + rhs_reg + ", " + "0(t0)");
                else
                    append_inst(inst2 + " " + rhs_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
            }
        }
    }
    auto [dest_reg, find] = getRegName(&instr, 0);
    if (!find)
        dest_reg = "t6";
    switch (instr.get_instr_type())
    {
    case Instruction::eq:
        append_inst("xor t0, " + lhs_reg + ", " + rhs_reg);
        append_inst("seqz " + dest_reg + ", t0"); // dest_reg = (lhs == rhs) ? 1 : 0
        break;
    case Instruction::ne:
        append_inst("xor t0, " + lhs_reg + ", " + rhs_reg);
        append_inst("snez " + dest_reg + ", t0"); // dest_reg = (lhs != rhs) ? 1 : 0
        break;
    case Instruction::lt:
        append_inst("slt " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
        break;
    case Instruction::le:
        append_inst("slt t0, " + rhs_reg + ", " + lhs_reg);
        append_inst("xori " + dest_reg + ", t0, 1"); // dest_reg = !(rhs < lhs)
        break;
    case Instruction::gt:
        append_inst("slt " + dest_reg + ", " + rhs_reg + ", " + lhs_reg);
        break;
    case Instruction::ge:
        append_inst("slt t0, " + lhs_reg + ", " + rhs_reg);
        append_inst("xori " + dest_reg + ", t0, 1"); // dest_reg = !(lhs < rhs)
        break;
    case Instruction::iiand:
        append_inst("and " + dest_reg + ", " + lhs_reg + ", " + rhs_reg);
        break;
    default:
        assert(false && "Unsupported icmp op in expr mode");
    }

    if (!find)
        store_from_greg_string(&instr, dest_reg);
}

void CodeGenRegister::gen_fcmp_expr()
{
    auto &instr = *context.inst;
    auto lhs = instr.get_operand(0);
    auto rhs = instr.get_operand(1);
    std::string lhs_reg = value2reg(lhs, 0);
    std::string rhs_reg = value2reg(rhs, 1);
    auto [dest_reg, find] = getRegName(&instr, 0);

    std::string tmp = "t0"; // 中间寄存器，用于转换浮点比较结果

    switch (instr.get_instr_type())
    {
    case Instruction::feq:
        append_inst("feq.s " + tmp + ", " + lhs_reg + ", " + rhs_reg);
        break;
    case Instruction::fne:
        append_inst("feq.s " + tmp + ", " + lhs_reg + ", " + rhs_reg);
        append_inst("xori " + tmp + ", " + tmp + ", 1");
        break;
    case Instruction::flt:
        append_inst("flt.s " + tmp + ", " + lhs_reg + ", " + rhs_reg);
        break;
    case Instruction::fgt:
        append_inst("flt.s " + tmp + ", " + rhs_reg + ", " + lhs_reg);
        break;
    case Instruction::fle:
        append_inst("fle.s " + tmp + ", " + lhs_reg + ", " + rhs_reg);
        break;
    case Instruction::fge:
        append_inst("fle.s " + tmp + ", " + rhs_reg + ", " + lhs_reg);
        break;
    default:
        assert(false && "Unsupported fcmp op in expr mode");
    }

    append_inst("mv " + dest_reg + ", " + tmp);
    if (!find)
        store_from_greg_string(&instr, dest_reg);
}

void CodeGenRegister::gen_fcmp_branch()
{
    auto fcmp = static_cast<FCmpInst *>(context.inst);

    // 获取 fcmp 指令的结果所在寄存器，触发 value2reg 生成比较指令
    std::string cond_reg = value2reg(fcmp, 0); // fcmp 的结果是整数 0/1，存在 t0/a0/等整数寄存器中

    // 根据结果非零跳转
    append_inst("bnez " + cond_reg + ", " + context.true_label);
    append_inst("j " + context.false_label);
}

void CodeGenRegister::gen_zext()
{
    // TODO 将窄位宽的整数数据进行零扩展
    auto *inst = context.inst;

    // 操作数
    Value *val = inst->get_operand(0);
    Type *src_type = val->get_type();

    // 临时寄存器
    Reg src_reg = Reg::t(0);
    Reg dst_reg = Reg::t(1);

    // 1. 将原始值加载到 src_reg
    load_to_greg(val, src_reg);

    // 2. 零扩展（只对小位宽整数有效）
    if (src_type->is_int1_type())
    {
        // i1 只有 0 或 1，用 AND 清掉高位
        append_inst("andi", {dst_reg.print(), src_reg.print(), "1"});
    }
    else if (src_type->is_int8_type())
    {
        // i8 → i32：保留低8位
        append_inst("andi", {dst_reg.print(), src_reg.print(), "255"});
    }
    else
    {
        throw std::runtime_error("zext only supports i1 and i8 to i32");
    }
    // 3. 存回结果
    store_from_greg(inst, dst_reg);
}

// void CodeGenRegister::gen_call(std::unordered_map<Instruction *, int> instr_id) {
//     //offsetnum = 0;
//     printf("1957");
//     auto call_inst = static_cast<CallInst *>(context.inst);
//     printf("1957");
//     auto func = static_cast<Function *>(call_inst->get_operand(0));
//     int call_pos = instr_id[context.inst];
//     int save_stack_space = 0;
//     std::vector<std::tuple<Value *, std::string>> store_record;
//     // 获取需要保存的寄存器变量（用于 call 后恢复）
//     for (auto &[val, interval] : LRA.get_interval_map()) {
//         if (val == call_inst) continue;
//         if (RA::RegAllocator::no_reg_alloca(val)) continue;
//         auto [reg_name, found] = getRegName(val);
//         if (!found) continue;
//         if (interval.i > call_pos || interval.j <= call_pos) continue;
//         store_record.emplace_back(val, reg_name);
//         save_stack_space += 8;
//     }
//     // 参数区大小（8个之后的通过栈传递）
//     int arg_cnt = call_inst->get_num_operand() - 1;
//     int param_stack_space = arg_cnt * 8;
//     //int param_stack_space = extra_stack_args * 8+8;
//    // printf("\n%d\n",param_stack_space);
//    // printf("\n%d\n",save_stack_space);
//     // 分配总栈空间
//     int total_stack = save_stack_space + param_stack_space;
//     if(total_stack%16!=0) total_stack+=8;
//     // 预留栈空间（sp -= stackBytes）
//     if (total_stack > 0) {
//         if (total_stack >= -2048 && total_stack <= 2047) {
//         append_inst("addi", {"sp", "sp", "-" + std::to_string(total_stack)});
//         } else {
//             load_large_int64(total_stack, Reg::t(0));
//             append_inst("sub", {"sp", "sp", "t0"});
//         }
//     }
//     // ⏬ 保存寄存器变量（在参数区之后往 sp 更低地址压栈）
//     int offset = param_stack_space;
//     for (auto &[val, reg] : store_record) {
//         bool is_float = val->get_type()->is_float_type();
//         if (is_float)
//             append_inst("fsw " + reg + ", " + std::to_string(offset) + "(sp)");
//         else
//             append_inst("sd " + reg + ", " + std::to_string(offset) + "(sp)");
//         offset += 8;
//         //offsetnum += 8;
//     }
//     // 🔼 参数栈区域在 sp 上方，先传参数（此时 sp 已下移，可以正确传栈参数）
//     pass_arguments(call_inst, 0);  // 栈上传参区域直接从当前 sp 开始
//      printf("\n1958\n");
//     // 🔁 发起函数调用
//     append_inst("call " + func->get_name());
//     // ⬅️ 处理返回值
//     if (!call_inst->get_type()->is_void_type()) {
//         std::string ret_reg = call_inst->get_type()->is_float_type() ? "fa0" : "a0";
//         gencopy(call_inst, ret_reg);
//     }
//     // 🔁 恢复寄存器
//     offset = param_stack_space;
//     for (auto &[val, reg] : store_record) {
//         bool is_float = val->get_type()->is_float_type();
//         if (is_float)
//             append_inst("flw " + reg + ", " + std::to_string(offset) + "(sp)");
//         else
//             append_inst("ld " + reg + ", " + std::to_string(offset) + "(sp)");
//         offset += 8;
//     }
//     // 回收栈空间
//     if (total_stack > 0) {
//         if (total_stack >= -2048 && total_stack <= 2047) {
//             append_inst("addi sp, sp, " + std::to_string(total_stack));
//         } else {
//             load_large_int64(total_stack, Reg::t(0));
//             append_inst("add", {"sp", "sp", "t0"});
//         }
//     }
// }

void CodeGenRegister::gen_call(std::unordered_map<Instruction *, int> instr_id)
{
    // offsetnum = 0;
    // printf("1957");
    auto call_inst = static_cast<CallInst *>(context.inst);
    // printf("1957");
    auto func = static_cast<Function *>(call_inst->get_operand(0));
    int call_pos = instr_id[context.inst];
    int save_stack_space = 0;
    std::vector<std::tuple<Value *, std::string>> store_record;
    // 获取需要保存的寄存器变量（用于 call 后恢复）
    for (auto &[val, interval] : LRA.get_interval_map())
    {
        if (val == call_inst)
            continue;
        if (RA::RegAllocator::no_reg_alloca(val))
            continue;
        auto [reg_name, found] = getRegName(val);
        if (!found)
            continue;
        // if(auto *arg1 = dynamic_cast<Argument *>(val)){
        //     if(found){
        //         store_record.emplace_back(val, reg_name);
        //         save_stack_space += 8;
        //         continue;
        //     }
        // }
        if (auto *arg = dynamic_cast<Argument *>(val))
        {
            if (interval.i > call_pos || interval.j < call_pos - 1)
                continue;
        }
        else
        {
            if (interval.i > call_pos || interval.j < call_pos)
                continue;
        }
        store_record.emplace_back(val, reg_name);
        save_stack_space += 8;
    }
    // 参数区大小（8个之后的通过栈传递）
    int arg_cnt = call_inst->get_num_operand() - 1;
    int param_stack_space;
    if (arg_cnt > 7)
        param_stack_space = (arg_cnt - 7) * 8;
    // else param_stack_space = 0;
    // int param_stack_space = extra_stack_args * 8+8;
    // 分配总栈空间
    int total_stack = param_stack_space + 56;
    if (total_stack % 16 != 0)
        total_stack += 8;
    // // 预留栈空间（sp -= stackBytes）
    if (arg_cnt > 7)
    {
        if (total_stack >= -2048 && total_stack <= 2047)
        {
            append_inst("addi", {"sp", "sp", "-" + std::to_string(total_stack)});
        }
        else
        {
            load_large_int64(total_stack, Reg::t(0));
            append_inst("sub", {"sp", "sp", "t0"});
        }
    }
    // ⏬ 保存寄存器变量（在参数区之后往 sp 更低地址压栈）
    int offset = -context.origin_frame_size;
    for (auto &[val, reg] : store_record)
    {
        bool is_float = val->get_type()->is_float_type();
        if (is_float)
        {
            if (IS_IMM_12(offset))
            {
                append_inst("fsd " + reg + ", " + std::to_string(offset) + "(fp)");
            }
            else
            {
                append_inst("li t1, " + std::to_string(offset));
                append_inst("add t1, fp, t1");
                append_inst("fsd " + reg + ", " + "0(t1)");
            }
        }
        else
        {
            if (IS_IMM_12(offset))
            {
                std::cout << func->print() << std::endl;
                append_inst("sd " + reg + ", " + std::to_string(offset) + "(fp)");
            }
            else
            {
                append_inst("li t1, " + std::to_string(offset));
                append_inst("add t1, fp, t1");
                append_inst("sd " + reg + ", " + "0(t1)");
            }
        }
        context.offset_map_call[reg] = offset;
        offset -= 8;
        // offsetnum += 8;
    }
    // 🔼 参数栈区域在 sp 上方，先传参数（此时 sp 已下移，可以正确传栈参数）
    pass_arguments(call_inst, 0); // 栈上传参区域直接从当前 sp 开始
    // 🔁 发起函数调用
    append_inst("call " + func->get_name());
    // int arg_cnt = call_inst->get_num_operand() - 1; // 参数数量
    int int_cnt = 0, float_cnt = 0;
    for (int i = 1; i <= arg_cnt; ++i)
    {
        Value *arg = call_inst->get_operand(i);
        // std::string src = value2reg(arg, 0); // 获取当前值的寄存器或临时值
        bool is_float = arg->get_type()->is_float_type();
        std::string suffix;
        if (is_float)
            suffix = "w"; // float
        else if (arg->get_type()->get_size() == 8 || arg->get_type()->is_pointer_type())
            suffix = "d"; // int64 或 pointer
        else if (arg->get_type()->get_size() == 4)
            suffix = "w"; // int32
        else
            throw std::runtime_error("Unsupported store size");
        std::string inst2 = is_float ? ("fl" + suffix) : ("l" + suffix);
        std::string inst1 = is_float ? ("fs" + suffix) : ("s" + suffix);
        // 如果是函数参数
        if (auto *arg1 = dynamic_cast<Argument *>(arg))
        {
            auto [dest, find] = getRegName(arg1, 0);
            int id = 0;
            for (auto &arg_test : arg1->get_parent()->get_args())
            {
                if (&arg_test == arg1)
                    break;
                id++;
            }
            int64_t offset = 8 * id;
            if (context.offset_map[arg] && find)
            {
                append_inst(inst2 + " " + dest + ", " + std::to_string(context.offset_map[arg]) + "(fp)");
            }
        }
    }
    bool is_float = call_inst->get_type()->is_float_type();
    std::string ret_regx;
    if (is_float)
    {
        ret_regx = "ft0";
    }
    else
    {
        ret_regx = "t0";
    }
    // ⬅️ 处理返回值
    if (!call_inst->get_type()->is_void_type())
    {
        std::string ret_reg = call_inst->get_type()->is_float_type() ? "fa0" : "a0";

        if (is_float)
        {
            append_inst("fmv.s " + ret_regx + ", fa0");
        }
        else
        {
            append_inst("mv " + ret_regx + ", a0");
        }
    }
    // 🔁 恢复寄存器
    offset = -context.origin_frame_size;
    for (auto &[val, reg] : store_record)
    {
        bool is_float = val->get_type()->is_float_type();
        if (is_float)
        {
            if (IS_IMM_12(offset))
            {
                append_inst("fld " + reg + ", " + std::to_string(offset) + "(fp)");
            }
            else
            {
                append_inst("li t1, " + std::to_string(offset));
                append_inst("add t1, fp, t1");
                append_inst("fld " + reg + ", " + "0(t1)");
            }
        }
        else
        {
            if (IS_IMM_12(offset))
            {
                append_inst("ld " + reg + ", " + std::to_string(offset) + "(fp)");
            }
            else
            {
                append_inst("li t1, " + std::to_string(offset));
                append_inst("add t1, fp, t1");
                append_inst("ld " + reg + ", " + "0(t1)");
            }
        }
        offset -= 8;
    }
    // ⬅️ 处理返回值
    if (!call_inst->get_type()->is_void_type())
    {
        if (is_float)
        {
            gencopy(call_inst, ret_regx);
        }
        else
        {
            gencopy(call_inst, ret_regx);
        }
    }
    // 回收栈空间
    if (arg_cnt > 7)
    {
        if (total_stack >= -2048 && total_stack <= 2047)
        {
            append_inst("addi sp, sp, " + std::to_string(total_stack));
        }
        else
        {
            load_large_int64(total_stack, Reg::t(0));
            append_inst("add", {"sp", "sp", "t0"});
        }
    }
}

/*
 * %op = getelementptr [10 x i32], [10 x i32]* %op, i32 0, i32 %op
 * %op = getelementptr        i32,        i32* %op, i32 %op
 *
 * Memory layout
 *       -            ^
 * +-----------+      | Smaller address
 * |  arg ptr  |---+  |
 * +-----------+   |  |
 * |           |   |  |
 * +-----------+   /  |
 * |           |<--   |
 * |           |   \  |
 * |           |   |  |
 * |   Array   |   |  |
 * |           |   |  |
 * |           |   |  |
 * |           |   |  |
 * +-----------+   |  |
 * |  Pointer  |---+  |
 * +-----------+      |
 * |           |      |
 * +-----------+      |
 * |           |      |
 * +-----------+      |
 * |           |      |
 * +-----------+      | Larger address
 *       +
 */

// void CodeGenRegister::gen_gep()
// {
//     auto *inst = context.inst;
//     int num_idx = inst->get_num_operand() - 1;
//     auto [dest_reg, find] = getRegName(inst, 0);
//     Value *base_val = inst->get_operand(0);
//     Type *current_type = base_val->get_type()->get_pointer_element_type();
//     std::string base_reg;
//     if (auto alloca_inst = dynamic_cast<AllocaInst *>(base_val))
//     {
//         int offset = get_stack_offset(alloca_inst);
//         if (IS_IMM_12(offset))
//         {
//             append_inst("ld t6, " + std::to_string(offset) + "(fp)");
//             base_reg = "t6";
//         }
//         else
//         {
//             append_inst("li", {"t1", std::to_string(offset)});
//             append_inst("add", {"t1", "fp", "t1"});
//             append_inst("ld t6, 0(t1)");
//             base_reg = "t6";
//         }
//     }
//     else
//     {
//         base_reg = value2reg(base_val, 0);
//         append_inst("mv t6, " + base_reg);
//     }
//     for (int i = 1; i <= num_idx; ++i)
//     {
//         Value *idx_val = inst->get_operand(i);
//         std::string idx_reg = value2reg(idx_val, 1);
//         if (auto *arg = dynamic_cast<Argument *>(idx_val))
//         {
//             int id = 0;
//             for (auto &arg_test : arg->get_parent()->get_args())
//             {
//                 if (&arg_test == arg)
//                     break;
//                 id++;
//             }
//             // std::cerr << "This value is an argument. Argument index = " << id << std::endl;
//             // int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
//             std::string suffix;
//             bool is_float1 = idx_val->get_type()->is_float_type();
//             // std::string suffix;
//             if (is_float1)
//                 suffix = "w"; // float
//             else
//                 suffix = "w"; // int32
//             std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
//             int64_t offset = 8 * id;
//             if (!IS_IMM_12(offset))
//             {
//                 if (idx_reg == "t1")
//                 {
//                     // append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
//                     if (is_float1)
//                         append_inst(inst1 + " " + "f" + idx_reg + ", " + "0(t0)");
//                     else
//                         append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
//                 }
//             }
//         }
//         else
//         {
//             if (!IS_IMM_12(context.offset_map[idx_val]))
//             {
//                 std::string suffix;
//                 bool is_float1 = idx_val->get_type()->is_float_type();
//                 // std::string suffix;
//                 if (is_float1)
//                     suffix = "w"; // float
//                 else
//                     suffix = "w"; // int32
//                 std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
//                 if (idx_reg == "t1")
//                 {
//                     // append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
//                     if (is_float1)
//                         append_inst(inst1 + " " + "f" + idx_reg + ", " + "0(t0)");
//                     else
//                         append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); // 专门处理偏移过大的情况
//                 }
//             }
//         }
//         // ✅ 计算 elem_size（必须在更新 current_type 之前）
//         size_t elem_size = 0;
//         if (current_type->is_array_type())
//         {
//             elem_size = current_type->get_size();
//         }
//         else if (current_type->is_pointer_type())
//         {
//             elem_size = current_type->get_pointer_element_type()->get_size();
//         }
//         else
//         {
//             elem_size = current_type->get_size();
//         }
//         // ✅ 计算偏移量
//         if (elem_size == 0)
//         {
//             append_inst("li t0, 0");
//         }
//         else if ((elem_size & (elem_size - 1)) == 0)
//         {
//             int shift = __builtin_ctz(elem_size); // 使用内置函数计算 log2
//             append_inst("slli t0, " + idx_reg + ", " + std::to_string(shift));
//         }
//         else
//         {
//             append_inst("li t0, " + std::to_string(elem_size));
//             append_inst("mul t0, " + idx_reg + ", t0");
//         }
//         append_inst("add t6, t6, t0");
//         // ✅ 最后更新 current_type
//         if (current_type->is_array_type())
//         {
//             current_type = current_type->get_array_element_type();
//         }
//         else if (current_type->is_pointer_type())
//         {
//             current_type = current_type->get_pointer_element_type();
//         }
//     }
//     if (!find)
//         dest_reg = "t1";
//     if (dest_reg != "t0")
//         append_inst("mv " + dest_reg + ", t6");
//     if (!find)
//     {
//         store_from_greg_string(inst, dest_reg);
//     }
// }

void CodeGenRegister::gen_gep() {
    auto *inst = context.inst;
    int num_idx = inst->get_num_operand() - 1;
    auto [dest_reg, find] = getRegName(inst, 0);
    Value *base_val = inst->get_operand(0);
    Type *current_type = base_val->get_type()->get_pointer_element_type();

    std::string base_reg;
    if (auto alloca_inst = dynamic_cast<AllocaInst *>(base_val)) {
        int offset = get_stack_offset(alloca_inst);
        if(IS_IMM_12(offset)){
            append_inst("ld t6, " + std::to_string(offset)+"(fp)");
            base_reg = "t6";
        }else{
            append_inst("li", {"t1", std::to_string(offset)});
            append_inst("add", {"t1", "fp", "t1"});
            append_inst("ld t6, 0(t1)");
            base_reg = "t6";
        }
        
    } else {
        base_reg = value2reg(base_val, 0);
        append_inst("mv t6, " + base_reg);
    }

    for (int i = 1; i <= num_idx; ++i) {
        Value *idx_val = inst->get_operand(i);
        if(dynamic_cast<Constant *>(idx_val)){
            auto *is_zero=dynamic_cast<Constant *>(idx_val);
            std::cout<<is_zero->print()<<std::endl;
            if(is_zero->print()=="0") {
                if (current_type->is_array_type()) {
                    current_type = current_type->get_array_element_type();
                } else if (current_type->is_pointer_type()) {
                    current_type = current_type->get_pointer_element_type();
                }
                continue;
            }
            else{
                int immk=std::stoi(is_zero->print());
                size_t elem_size = 0;
                if (current_type->is_array_type()) {
                    elem_size = current_type->get_size();
                } else if (current_type->is_pointer_type()) {
                    elem_size = current_type->get_pointer_element_type()->get_size();
                } else {
                    elem_size = current_type->get_size();
                }
                if (elem_size == 0) {
                    append_inst("li t0, 0");
                    append_inst("add t6, t6, t0");
                } else if ((elem_size & (elem_size - 1)) == 0) {
                    int shift = __builtin_ctz(elem_size);  // 使用内置函数计算 log2
                    int immp=immk<<shift;
                    //append_inst("slli t0, " + idx_reg + ", " + std::to_string(shift));
                    if(IS_IMM_12(immp)){
                        append_inst("addi t6, t6, " + std::to_string(immp));
                    }else{
                        append_inst("li t0, "+std::to_string(immp));
                        append_inst("add t6, t6, t0");
                    } 
                } else {
                    append_inst("li t1, " + std::to_string(immk));
                    append_inst("li t0, " + std::to_string(elem_size));
                    append_inst("mul t0, t1, t0");
                    append_inst("add t6, t6, t0");
                }
                if (current_type->is_array_type()) {
                    current_type = current_type->get_array_element_type();
                } else if (current_type->is_pointer_type()) {
                    current_type = current_type->get_pointer_element_type();
                }
                continue;
            }
        }
        std::string idx_reg = value2reg(idx_val, 1);
        if(auto *arg = dynamic_cast<Argument *>(idx_val)){
            int id = 0;
            for (auto &arg_test : arg->get_parent()->get_args()) {
                if (&arg_test == arg)
                break;
                id++;
            }
                //std::cerr << "This value is an argument. Argument index = " << id << std::endl;
                //int64_t frame_size_signed = static_cast<int64_t>(context.frame_size);
            std::string suffix;
            bool is_float1 = idx_val->get_type()->is_float_type();
                // std::string suffix;
            if (is_float1)
                suffix = "w";  // float
            else
                suffix = "w";  // int32
            std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
            int64_t offset = 8*id;
            if(!IS_IMM_12(offset)){
                if(idx_reg=="t1") {
                    //append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
                    if(is_float1) append_inst(inst1 + " " +"f"+ idx_reg  + ", " + "0(t0)");
                    else append_inst(inst1 + " " + idx_reg  + ", " + "0(t0)"); //专门处理偏移过大的情况
                }
            }
        }else{
            if(!IS_IMM_12(context.offset_map[idx_val])){
                std::string suffix;
                bool is_float1 = idx_val->get_type()->is_float_type();
                    // std::string suffix;
                if (is_float1)
                    suffix = "w";  // float
                else 
                    suffix = "w";  // int32
                std::string inst1 = is_float1 ? ("fl" + suffix) : ("l" + suffix);
                if(idx_reg=="t1"){ 
                    //append_inst(inst1 + " " + idx_reg + ", " + "0(t0)"); //专门处理偏移过大的情况
                    if(is_float1) append_inst(inst1 + " " +"f"+ idx_reg  + ", " + "0(t0)");
                    else append_inst(inst1 + " " + idx_reg  + ", " + "0(t0)"); //专门处理偏移过大的情况
                }
            }
        }

        // ✅ 计算 elem_size（必须在更新 current_type 之前）
        size_t elem_size = 0;
        if (current_type->is_array_type()) {
            elem_size = current_type->get_size();
        } else if (current_type->is_pointer_type()) {
            elem_size = current_type->get_pointer_element_type()->get_size();
        } else {
            elem_size = current_type->get_size();
        }

        // ✅ 计算偏移量
        if (elem_size == 0) {
            append_inst("li t0, 0");
        } else if ((elem_size & (elem_size - 1)) == 0) {
            int shift = __builtin_ctz(elem_size);  // 使用内置函数计算 log2
            append_inst("slli t0, " + idx_reg + ", " + std::to_string(shift));
        } else {
            append_inst("li t0, " + std::to_string(elem_size));
            append_inst("mul t0, " + idx_reg + ", t0");
        }

        append_inst("add t6, t6, t0");
        // ✅ 最后更新 current_type
        if (current_type->is_array_type()) {
            current_type = current_type->get_array_element_type();
        } else if (current_type->is_pointer_type()) {
            current_type = current_type->get_pointer_element_type();
        }
    }
    if(!find) dest_reg="t1";
    if (dest_reg != "t0")
        append_inst("mv " + dest_reg + ", t6");

    if (!find) {
        store_from_greg_string(inst, dest_reg);
    }
}

int CodeGenRegister::get_stack_offset(AllocaInst *alloca)
{
    if (context.offset_map.count(alloca))
    {
        return context.offset_map[alloca];
    }
    if (context.array_start_offset.count(alloca))
    {
        return context.array_start_offset[alloca];
    }
    // 没找到，说明没有分配栈空间
    // LOG(ERROR) << "alloca offset not found for value: " << alloca->get_name();
    throw std::runtime_error("alloca offset not found");
}

void CodeGenRegister::gen_sitofp()
{
    auto sitofp_inst = static_cast<SiToFpInst *>(context.inst);

    // 断言确保类型正确
    assert(context.inst->get_operand(0)->get_type() == m->get_int32_type());
    assert(sitofp_inst->get_dest_type() == m->get_float_type());
    // 获取源整数寄存器和目标浮点寄存器
    std::string i_reg = value2reg(context.inst->get_operand(0));
    auto [f_reg, find] = getRegName(context.inst);
    // RISC-V 指令：整数转单精度浮点数
    if (!find)
        f_reg = "ft0";
    append_inst("fcvt.s.w " + f_reg + ", " + i_reg);

    gencopy(context.inst, f_reg);
}

void CodeGenRegister::gen_fptosi()
{
    auto fptosi_inst = static_cast<FpToSiInst *>(context.inst);

    assert(context.inst->get_operand(0)->get_type() == m->get_float_type());
    assert(fptosi_inst->get_dest_type() == m->get_int32_type());

    std::string f_reg = value2reg(context.inst->get_operand(0));
    auto [i_reg, find] = getRegName(context.inst);
    if (!find)
        i_reg = "t0";
    // RISC-V 指令：单精度浮点转有符号整数，默认舍入模式为最近偶数
    // 如果需要向零舍入，需先设置 fcsr 寄存器舍入模式
    append_inst("fcvt.w.s " + i_reg + ", " + f_reg + ", rtz");
    gencopy(context.inst, i_reg);
}

void CodeGenRegister::emitGlobalVariables()
{
    if (m->get_global_variable().empty())
        return;

    // 先处理未初始化或全0的全局变量，放 .bss 段
    append_inst("# Uninitialized or zero-initialized globals (BSS segment)", ASMInstruction::Comment);
    append_inst(".section .bss", ASMInstruction::Atrribute);
    append_inst(".align 3", ASMInstruction::Atrribute); // 8字节对齐

    for (auto &global : m->get_global_variable())
    {
        auto *initVal = global.get_init();
        auto size = global.get_type()->get_pointer_element_type()->get_size();

        bool isZeroInit = (initVal == nullptr) || (dynamic_cast<ConstantZero *>(initVal) != nullptr);
        if (isZeroInit)
        {
            append_inst(".globl", {global.get_name()}, ASMInstruction::Atrribute);
            append_inst(".type", {global.get_name(), "@object"}, ASMInstruction::Atrribute);
            append_inst(".size", {global.get_name(), std::to_string(size)}, ASMInstruction::Atrribute);
            append_inst(global.get_name(), ASMInstruction::Label);
            append_inst(".zero", {std::to_string(size)}, ASMInstruction::Atrribute);
        }
    }

    // 处理已初始化的全局变量，放 .data 段
    append_inst("# Initialized globals (DATA segment)", ASMInstruction::Comment);
    append_inst(".section .data", ASMInstruction::Atrribute);

    for (auto &global : m->get_global_variable())
    {
        auto *initVal = global.get_init();
        auto size = global.get_type()->get_pointer_element_type()->get_size();

        bool isZeroInit = (initVal == nullptr) || (dynamic_cast<ConstantZero *>(initVal) != nullptr);
        if (isZeroInit)
            continue; // 跳过已经放bss的

        append_inst(".globl", {global.get_name()}, ASMInstruction::Atrribute);
        append_inst(".type", {global.get_name(), "@object"}, ASMInstruction::Atrribute);
        append_inst(".size", {global.get_name(), std::to_string(size)}, ASMInstruction::Atrribute);
        append_inst(global.get_name(), ASMInstruction::Label);

        auto &elemType = *global.get_type()->get_pointer_element_type();

        // 处理元素为整数类型
        if (elemType.is_integer_type())
        {
            if (auto *CI = dynamic_cast<ConstantInt *>(initVal))
            {
                append_inst(".word", {std::to_string(CI->get_value())}, ASMInstruction::Atrribute);
            }
            else if (auto *CA = dynamic_cast<ConstantArray *>(initVal))
            {
                emitIntArray(CA);
            }
            else
            {
                append_inst(".zero", {std::to_string(size)}, ASMInstruction::Atrribute);
            }
        }
        // 处理元素为浮点类型
        else if (elemType.is_float_type())
        {
            if (auto *CFP = dynamic_cast<ConstantFP *>(initVal))
            {
                float val = CFP->get_value();
                int intval;
                std::memcpy(&intval, &val, sizeof(float));
                append_inst(".word", {std::to_string(intval)}, ASMInstruction::Atrribute);
            }
            else if (auto *CA = dynamic_cast<ConstantArray *>(initVal))
            {
                emitFloatArray(CA);
            }
            else
            {
                append_inst(".zero", {std::to_string(size)}, ASMInstruction::Atrribute);
            }
        }
        // 递归支持数组元素是数组（多维数组）
        else if (elemType.is_array_type())
        {
            if (auto *CA = dynamic_cast<ConstantArray *>(initVal))
            {
                // 递归打印子数组
                emitIntArray(CA); // 如果元素是整数数组，或者
                // 或者改成一个更通用的函数，支持根据元素类型递归调用
            }
            else
            {
                append_inst(".zero", {std::to_string(size)}, ASMInstruction::Atrribute);
            }
        }
        else
        {
            append_inst(".zero", {std::to_string(size)}, ASMInstruction::Atrribute);
        }
    }
}

void CodeGenRegister::emitIntArray(ConstantArray *arr)
{
    unsigned n = arr->get_size_of_array();
    for (unsigned i = 0; i < n; i++)
    {
        auto *val = arr->get_element_value(i);
        if (auto *CI = dynamic_cast<ConstantInt *>(val))
        {
            append_inst(".word", {std::to_string(CI->get_value())}, ASMInstruction::Atrribute);
        }
        else if (auto *CA = dynamic_cast<ConstantArray *>(val))
        {
            emitIntArray(CA);
        }
        else
        {
            append_inst(".word", {"0"}, ASMInstruction::Atrribute);
        }
    }
}

void CodeGenRegister::emitFloatArray(ConstantArray *arr)
{
    unsigned n = arr->get_size_of_array();
    for (unsigned i = 0; i < n; i++)
    {
        auto *val = arr->get_element_value(i);
        if (auto *CFP = dynamic_cast<ConstantFP *>(val))
        {
            float fval = CFP->get_value();
            int intval;
            std::memcpy(&intval, &fval, sizeof(float));
            append_inst(".word", {std::to_string(intval)}, ASMInstruction::Atrribute);
        }
        else if (auto *CA = dynamic_cast<ConstantArray *>(val))
        {
            emitFloatArray(CA);
        }
        else
        {
            append_inst(".word", {"0"}, ASMInstruction::Atrribute);
        }
    }
}

void CodeGenRegister::run()
{
    // 设置函数和全局变量打印名（debug用）
//  printf("1957");
    m->set_print_name();

    emitGlobalVariables();
    // 函数代码段
    append_inst(".section .text", ASMInstruction::Atrribute); // 代码段
    append_inst(".align  2", ASMInstruction::Atrribute);      // 4字节对齐（2^2）

    for (auto &func : m->get_functions())
    {
        if (!func.is_declaration())
        {
            // 活跃变量分析
            // printf("1957");
            LRA.run(&func);
            // printf("1957");
            phi_map = LRA.get_phi_map();
            // for (auto &[range, value] : LRA.get()) {
            //     std::cout << "Value: " << value->get_name()
            //     << ", Range: [" << range.i << ", " << range.j << "]\n";
            // }

            RA_int.LinearScan(LRA.get(), &func);
            // printf("1957");
            RA_float.LinearScan(LRA.get(), &func);
            // printf("1957");
            // 打印寄存器分配调试信息（可选）
            // LOG_DEBUG << "Integer register allocation for function: " << func.get_name() << "\n";
            // RA_int.print([](int i) { return regname(i, false); });
            // LOG_DEBUG << "Float register allocation for function: " << func.get_name() << "\n";
            // RA_float.print([](int i) { return regname(i, true); });

            // 打印寄存器分配调试信息（可选）
            // std::cout << "[INFO] Integer register allocation for function: " << func.get_name() << std::endl;
            // RA_int.print([](int i)
            //              { return regname(i, false); });

            // std::cout << "[INFO] Float register allocation for function: " << func.get_name() << std::endl;
            // RA_float.print([](int i)
            //                { return regname(i, true); });

            context.clear();
            context.func = &func;

            // 函数全局符号和类型
            append_inst(".globl " + func.get_name(), ASMInstruction::Atrribute);
            append_inst(".type " + func.get_name() + ", @function", ASMInstruction::Atrribute);
            append_inst(func.get_name(), ASMInstruction::Label);
            for (auto &bb : func.get_basic_blocks())
            {
                for (auto &inst : bb.get_instructions())
                {
                    if (inst.is_phi())
                    {
                        auto *phi = static_cast<PhiInst *>(&inst);
                        // 对phi的每个前驱进行处理
                        for (auto &incoming : phi->get_incoming())
                        {
                            BasicBlock *pred = incoming.second;
                            Value *val = incoming.first;
                            // 插入 phi_path
                            auto key = std::make_pair(pred, &bb);
                            context.phi_path1[key].insert(std::make_pair(phi, val));
                        }
                    }
                }
            }
            // for (auto &[edge, phi_set] : context.phi_path1) {
            //     BasicBlock *pred = edge.first;
            //     BasicBlock *succ = edge.second;
            //     for (auto &[phi_def, val] : phi_set) {
            //         phi_map[pred].emplace_back(phi_def, val, succ);
            //     }
            // }

            // 分配栈帧
            allocate(RA_int, RA_float);
            // 生成函数入口 prologue
            // printf("1957");
            gen_prologue();
            // printf("1957");
            //  遍历基本块和指令生成代码
            for (auto &bb : func.get_basic_blocks())
            {
                context.bb = &bb;
                append_inst(label_name(context.bb), ASMInstruction::Label);
                for (auto &instr : bb.get_instructions())
                {
                   // append_inst("# " + instr.print(), ASMInstruction::Comment);
                    context.inst = &instr;
                    switch (instr.get_instr_type())
                    {
                    case Instruction::ret:
                        copy_stmt();
                        gen_ret();
                        break;
                    case Instruction::br:
                        copy_stmt();
                        gen_br();
                        break;
                    case Instruction::add:
                    case Instruction::sub:
                    case Instruction::mul:
                    case Instruction::sdiv:
                    case Instruction::srem:
                    case Instruction::fadd:
                    case Instruction::fsub:
                    case Instruction::fmul:
                    case Instruction::fdiv:
                    case Instruction::sand:
                    case Instruction::shl:
                    case Instruction::ashr:
                        gen_binary();
                        break;
                    case Instruction::alloca:
                        gen_alloca();
                        break;
                    case Instruction::load:
                        gen_load();
                        break;
                    case Instruction::store:
                        gen_store();
                        break;
                    case Instruction::ge:
                    case Instruction::gt:
                    case Instruction::le:
                    case Instruction::lt:
                    case Instruction::eq:
                    case Instruction::ne:
                    case Instruction::iiand:
                        gen_icmp_expr();
                        break;
                    case Instruction::fge:
                    case Instruction::fgt:
                    case Instruction::fle:
                    case Instruction::flt:
                    case Instruction::feq:
                    case Instruction::fne:
                        gen_fcmp_expr();
                        break;
                    case Instruction::phi:
                        // Phi节点不生成代码
                        break;
                    case Instruction::call:
                        gen_call(LRA.get_instr_id());
                        break;
                    case Instruction::getelementptr:
                        gen_gep();
                        break;
                    case Instruction::zext:
                        gen_zext();
                        break;
                    case Instruction::fptosi:
                        gen_fptosi();
                        break;
                    case Instruction::sitofp:
                        gen_sitofp();
                        break;
                    case Instruction::bitcast:
                        gen_bitcast();
                        break;              
                    default:
                        // 处理未覆盖的指令类型
                        LOG_WARNING << "Unhandled instruction type in codegen: " << instr.get_instr_type() << "\n";
                        break;
                    }
                }
            }
            // 生成函数结束 epilogue
            gen_epilogue();
        }
    }

    // TODO: 处理只读数据段 .rodata
}

std::string CodeGenRegister::print() const
{
    std::string result;
    for (const auto &inst : output)
    {
        result += inst.format();
    }
    return result;
}
// TODO: 对框架不满可尽情修改