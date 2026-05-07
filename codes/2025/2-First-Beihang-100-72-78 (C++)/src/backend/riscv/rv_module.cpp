#include "rv_module.hpp"

#include <cassert>
#include <cstring>
#include <random>

#include "rv_instruction.hpp"
#include "rv_reg_info.hpp"

namespace backend::riscv {

RVModule::RVModule() = default;

RVModule::~RVModule() {
    for (auto &kv : float_constants) {
        delete kv.second;
    }
    float_constants.clear();
    for (auto &kv : functions) {
        delete kv.second;
    }
    functions.clear();
    for (auto *var : data_variables) {
        delete var;
    }
    data_variables.clear();
    for (auto *var : bss_variables) {
        delete var;
    }
    bss_variables.clear();
}

RVFunction *RVModule::new_function(const std::string &name) {
    auto func = RVFunction::create(name);
    functions[name] = func;
    return func;
}

std::string RVModule::emit() const {
    std::string output;

    // 添加汇编代码开头的指令
    output += ".option pic\n";
    output += ".attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n";
    output += ".attribute unaligned_access, 0\n";
    output += ".attribute stack_align, 16\n";
    output += ".global main\n\n";

    // 生成函数类型声明
    for (const auto &[name, func] : functions) {
        if (!func->get_blocks().empty()) {
            // 去掉函数名前面的@符号
            std::string func_name = name;
            if (func_name[0] == '@') {
                func_name = func_name.substr(1);
            }
            output += ".type " + func_name + ", @function\n";
        }
    }
    output += "\n";

    // 生成.bss段
    if (!bss_variables.empty()) {
        output += ".bss\n";
        for (const auto &gv : bss_variables) {
            output += ".align 3\n";
            output += gv->to_string();
        }
        output += "\n";
    }

    // 生成.data段
    if (!data_variables.empty()) {
        output += ".data\n";
        for (const auto &gv : data_variables) {
            output += ".align 3\n";
            output += gv->to_string();
        }
        output += "\n";
    }

    // 生成浮点常量数据段
    if (!float_constants.empty()) {
        output += ".section .srodata.cst4, \"aM\", @progbits, 4\n";
        for (const auto &[val, f_const] : float_constants) {
            output += ".align 2\n";
            output += f_const->getName() + ":\n";
            int i_val;
            std::memcpy(&i_val, &val, sizeof(int));
            output += "    .word " + std::to_string(i_val) + "\n";
        }
        output += "\n";
    }

    // 生成.text段
    output += ".text\n";

    // 先输出main函数
    for (const auto &[name, func] : functions) {
        if (name == "@main") {
            output += func->to_string();
            output += "\n";
            break;
        }
    }

    // 再输出其他函数
    for (const auto &[name, func] : functions) {
        if (name != "@main" && !func->get_blocks().empty()) {
            output += func->to_string();
            output += "\n";
        }
    }

    return output;
}

RVFunction *RVModule::get_function(const std::string &name) const {
    auto it = functions.find(name);
    if (it != functions.end()) {
        return it->second;
    }
    return nullptr;
}

const std::map<std::string, RVFunction *> &RVModule::get_functions() const { return functions; }

void RVModule::add_data_variable(RVGlobalVar *var) { data_variables.push_back(var); }

void RVModule::add_bss_variable(RVGlobalVar *var) { bss_variables.push_back(var); }

void RVModule::check_memory_offsets() const {
    for (const auto &[name, func] : functions) {
        for (const auto &block : func->get_blocks()) {
            for (const auto &inst : block->get_insts()) {
                if (!inst->is_memory_ins()) {
                    continue;
                }

                auto *imm = dynamic_cast<RVImmediate *>(inst->get_operands()[2]);
                int offset = static_cast<int>(imm->value());
                auto inst_type = inst->get_type();

                // 检查word类型指令（lw, sw, flw, fsw）的偏移量是否为4的倍数
                if (inst_type == RVInstType::LW || inst_type == RVInstType::SW || inst_type == RVInstType::FLW ||
                    inst_type == RVInstType::FSW) {
                    if (offset % 4 != 0) {
                        assert(false && "Word类型指令(lw/sw/flw/fsw)的偏移量必须是4的倍数");
                    }
                }
                // 检查double类型指令（ld, sd, fld, fsd）的偏移量是否为8的倍数
                else if (inst_type == RVInstType::LD || inst_type == RVInstType::SD || inst_type == RVInstType::FLD ||
                         inst_type == RVInstType::FSD) {
                    if (offset % 8 != 0) {
                        assert(false && "Double类型指令(ld/sd/fld/fsd)的偏移量必须是8的倍数");
                    }
                }
                // 检查half word类型指令（lh, lhu, sh）的偏移量是否为2的倍数
                else if (inst_type == RVInstType::LH || inst_type == RVInstType::LHU || inst_type == RVInstType::SH) {
                    if (offset % 2 != 0) {
                        assert(false && "Half word类型指令(lh/lhu/sh)的偏移量必须是2的倍数");
                    }
                }
                // byte类型指令（lb, lbu, sb）的偏移量可以是任意值，不需要检查
            }
        }
    }
}

void RVModule::simulate_random_context() {
    // 查找main函数
    auto main_func = get_function("@main");
    if (!main_func) {
        return;  // 没有main函数则不做处理
    }

    // 获取第一个基本块
    const auto &blocks = main_func->get_blocks();
    if (blocks.empty()) {
        return;  // 没有基本块则不做处理
    }

    auto *first_block = blocks.front();
    auto &instructions = first_block->get_insts();

    // 获取所有CPU寄存器
    auto all_cpu_regs = reg_info::get_all_cpu_regs();

    // 排除前5个寄存器 (x0-x4: zero, ra, sp, gp, tp)
    std::vector<RVPhyReg *> target_regs;
    for (auto *reg : all_cpu_regs) {
        int reg_id = reg->get_phys_id();
        if (reg_id >= 5) {  // 只考虑x5及以后的寄存器
            target_regs.push_back(reg);
        }
    }

    // 初始化随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(-1000, 1000);  // 随机值范围

    // 在第一个基本块开头插入li指令
    auto insert_pos = instructions.begin();
    for (auto *reg : target_regs) {
        int random_value = dis(gen);
        auto *imm = RVImmediate::create(random_value);
        auto *li_inst = RVLi::create(reg, imm);

        // 插入指令到基本块开头
        insert_pos = first_block->insert_inst(insert_pos, li_inst);
        ++insert_pos;  // 移动到下一个位置
    }
}

}  // namespace backend::riscv
