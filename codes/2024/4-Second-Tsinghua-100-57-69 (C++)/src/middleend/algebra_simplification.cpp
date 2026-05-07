#include "middleend/algebra_simplification.hpp"
#include "middleend/reverse_postorder.hpp"
#include <cmath>
#include <iostream>

namespace middleend {

void AlgebraSimplification::run() {
    for (auto func : *module_->get_functions()) {
        int next_temp_id_ = func->get_temp_used();
        std::unordered_map<Temp *, std::unordered_map<Temp *, int>> reg_count; // 记录每个寄存器中累加了哪些寄存器的值，以实现加减法的代数化简
        std::unordered_map<Temp *, int> const_count; // 记录每个寄存器中累加了哪些常数的值，以实现加减法的代数化简
        // std::unordered_map<Temp *, float> fconst_count; // 记录每个寄存器中累加了哪些浮点数的值，以实现加减法的代数化简
        CFG *cfg = new CFG(func);
        ReversePostOrder rpo(cfg);
        for (auto bb_idx : rpo.order) {
            auto bb = cfg->get_bb(bb_idx);
            for (auto inst_iter = bb->get_instructions()->begin(); inst_iter != bb->get_instructions()->end(); inst_iter++) {
                auto &inst = *inst_iter;
            // for (auto &inst : *bb->get_instructions()) {
                TypeCase(binary, ir::instruction::Binary *, inst) {
                    if (binary->get_type() != BinaryOp::Add && binary->get_type() != BinaryOp::Sub) { // 目前只针对加减法优化
                        continue;
                    }
                    if (binary->getdst()->get_type() != Type(0)) { // 目前只针对int优化
                        continue;
                    }
                    std::unordered_map<Temp *, int> dst_count; // 统计当前指令目的寄存器中累加的寄存器的值
                    if (reg_count.count(binary->getlhs())) { // 以左操作数为基础，统计累加的寄存器的值
                        dst_count = reg_count.at(binary->getlhs());
                    } else {
                        dst_count[binary->getlhs()] = 1; // 左操作数没有统计过则累计为1
                    }
                    if (reg_count.count(binary->getrhs())) {
                        for (auto &item : reg_count.at(binary->getrhs())) { // 将右操作数中累加的寄存器统计到目的寄存器中
                            if (binary->get_type() == BinaryOp::Add) {
                                dst_count[item.first] += item.second; // 加法
                            }
                            if (binary->get_type() == BinaryOp::Sub) {
                                dst_count[item.first] -= item.second; // 减法
                            }
                        }
                    } else {
                        if (binary->get_type() == BinaryOp::Add) {
                            dst_count[binary->getrhs()] += 1; // 右操作数没有统计过则累计为1
                        } else if (binary->get_type() == BinaryOp::Sub) {
                            dst_count[binary->getrhs()] -= 1; // 右操作数没有统计过则累计为-1
                        }
                    }
                    int dst_const = const_count[binary->getlhs()]; // 统计当前指令目的寄存器中累加的常数的值
                    if (binary->get_type() == BinaryOp::Add) {
                        dst_const += const_count[binary->getrhs()]; // 加法
                    } else if (binary->get_type() == BinaryOp::Sub) {
                        dst_const -= const_count[binary->getrhs()]; // 减法
                    }
                    reg_count[binary->getdst()] = dst_count; // 更新目的寄存器的统计结果
                    const_count[binary->getdst()] = dst_const; // 更新目的寄存器的统计结果
                    for (auto &item : dst_count) {
                        if (item.second == 0) { // 如果累加的次数为0（加减相抵消），则可以删除这个寄存器
                            reg_count[binary->getdst()].erase(item.first);
                        }
                    }
                    dst_count = reg_count.at(binary->getdst());
                    if (dst_const == 0) {
                        if (dst_count.size() == 0) { // 如果累加项为空，目的寄存器的值为0
                            inst = new ir::instruction::LoadImm4(binary->getdst(), 0);
                        } else if (dst_count.size() == 1) { // 如果只有一个累加项，则可以代数化简
                            auto item = *dst_count.begin();
                            if (item.second == 1) { // 累加次数为1，直接赋值
                                inst = new ir::instruction::Assign(binary->getdst(), item.first);
                            } else if (std::abs(item.second) > 100) { // 累加次数大于10, 用乘法替代加法
                                Temp *tmp = new Temp(next_temp_id_++, 0);
                                auto pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::LoadImm4(tmp, item.second));
                                inst_iter = std::next(pre);
                                // inst_iter = pre;
                                *inst_iter = new ir::instruction::Binary(BinaryOp::Mul, binary->getdst(), item.first, tmp);
                                // TODO: 释放删除后指令所占的空间
                            }
                        } else if (dst_count.size() == 2) { // 如果有两个累加项，则可以代数化简
                            auto item1 = *dst_count.begin();
                            auto item2 = *(++dst_count.begin());
                            if (item1.second == 1 && item2.second == 1) { // 两个累加项的累加次数都为1
                                inst = new ir::instruction::Binary(BinaryOp::Add, binary->getdst(), item1.first, item2.first);
                            } else if (item1.second == 1 && item2.second == -1) { // 一正一负可以化为减法
                                inst = new ir::instruction::Binary(BinaryOp::Sub, binary->getdst(), item1.first, item2.first);
                            } else if (item1.second == -1 && item2.second == 1) { // 一负一正可以化为减法
                                inst = new ir::instruction::Binary(BinaryOp::Sub, binary->getdst(), item2.first, item1.first);
                            }
                        } else {
                            continue;
                        }
                    } else {
                        if (dst_count.size() == 0) {
                            inst = new ir::instruction::LoadImm4(binary->getdst(), dst_const);
                        } else if (dst_count.size() == 1) {
                            auto item = *dst_count.begin();
                            if (item.second == 1) {
                                inst = new ir::instruction::BinaryImm(binary->get_type(), binary->getdst(), item.first, dst_const);
                            } else if (std::abs(item.second) > 100) {
                                Temp *tmp1 = new Temp(next_temp_id_++, 0);
                                Temp *tmp2 = new Temp(next_temp_id_++, 0);
                                auto pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::LoadImm4(tmp1, item.second));
                                inst_iter = std::next(pre);
                                pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::Binary(BinaryOp::Mul, tmp2, item.first, tmp1));
                                inst_iter = std::next(pre);
                                // inst_iter = pre;
                                *inst_iter = new ir::instruction::BinaryImm(binary->get_type(), binary->getdst(), tmp2, dst_const);
                                // TODO: 释放删除后指令所占的空间
                            }
                        } else {
                            continue;
                        }
                    }
                } else TypeCase(binaryimm, ir::instruction::BinaryImm *, inst) {
                    if (binaryimm->get_type() != BinaryOp::Add && binaryimm->get_type() != BinaryOp::Sub) {
                        continue;
                    }
                    if (binaryimm->getdst()->get_type() != Type(0)) { // 目前只针对int优化
                        continue;
                    }
                    std::unordered_map<Temp *, int> dst_count;
                    if (reg_count.count(binaryimm->getlhs())) {
                        dst_count = reg_count.at(binaryimm->getlhs());
                    } else {
                        dst_count[binaryimm->getlhs()] = 1;
                    }
                    int dst_const = const_count[binaryimm->getlhs()];
                    if (binaryimm->get_type() == BinaryOp::Add) {
                        dst_const += binaryimm->getimm();
                    } else if (binaryimm->get_type() == BinaryOp::Sub) {
                        dst_const -= binaryimm->getimm();
                    }
                    reg_count[binaryimm->getdst()] = dst_count;
                    const_count[binaryimm->getdst()] = dst_const;
                    if (dst_const == 0) {
                        if (dst_count.size() == 0) {
                            inst = new ir::instruction::LoadImm4(binaryimm->getdst(), 0);
                        } else if (dst_count.size() == 1) {
                            auto item = *dst_count.begin();
                            if (item.second == 1) {
                                inst = new ir::instruction::Assign(binaryimm->getdst(), item.first);
                            } else if (std::abs(item.second) > 100) {
                                Temp *tmp = new Temp(next_temp_id_++, 0);
                                auto pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::LoadImm4(tmp, item.second));
                                inst_iter = std::next(pre);
                                // inst_iter = pre;
                                *inst_iter = new ir::instruction::Binary(BinaryOp::Mul, binaryimm->getdst(), item.first, tmp);
                                // TODO: 释放删除后指令所占的空间
                            }
                        } else if (dst_count.size() == 2) {
                            auto item1 = *dst_count.begin();
                            auto item2 = *(++dst_count.begin());
                            if (item1.second == 1 && item2.second == 1) {
                                inst = new ir::instruction::Binary(BinaryOp::Add, binaryimm->getdst(), item1.first, item2.first);
                            } else if (item1.second == 1 && item2.second == -1) {
                                inst = new ir::instruction::Binary(BinaryOp::Sub, binaryimm->getdst(), item1.first, item2.first);
                            } else if (item1.second == -1 && item2.second == 1) {
                                inst = new ir::instruction::Binary(BinaryOp::Sub, binaryimm->getdst(), item2.first, item1.first);
                            }
                        } else {
                            continue;
                        }
                    } else {
                        if (dst_count.size() == 0) {
                            inst = new ir::instruction::LoadImm4(binaryimm->getdst(), dst_const);
                        } else if (dst_count.size() == 1) {
                            auto item = *dst_count.begin();
                            if (item.second == 1) {
                                inst = new ir::instruction::BinaryImm(binaryimm->get_type(), binaryimm->getdst(), item.first, dst_const);
                            } else if (std::abs(item.second) > 100) {
                                Temp *tmp1 = new Temp(next_temp_id_++, 0);
                                Temp *tmp2 = new Temp(next_temp_id_++, 0);
                                auto pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::LoadImm4(tmp1, item.second));
                                pre = bb->get_instructions()->insert(inst_iter, new ir::instruction::Binary(BinaryOp::Mul, tmp2, item.first, tmp1));
                                inst_iter = std::next(pre);
                                // inst_iter = pre;
                                *inst_iter = new ir::instruction::BinaryImm(binaryimm->get_type(), binaryimm->getdst(), tmp2, dst_const);
                                // TODO: 释放删除后指令所占的空间
                            }
                        } else {
                            continue;
                        }
                    }
                } else TypeCase(unary, ir::instruction::Unary *, inst) {
                    if (unary->get_type() != UnaryOp::Add && unary->get_type() != UnaryOp::Sub){
                        continue;
                    }
                    if (unary->getdst()->get_type() != Type(0)) { // 目前只针对int优化
                        continue;
                    }
                    std::unordered_map<Temp *, int> dst_count;
                    int dst_const = 0;
                    if (unary->get_type() == UnaryOp::Add) {
                        if (reg_count.count(unary->getsrc())) {
                            dst_count = reg_count.at(unary->getsrc());
                        } else {
                            dst_count[unary->getsrc()] = 1;
                        }
                        dst_const = const_count[unary->getsrc()];
                    } else if (unary->get_type() == UnaryOp::Sub) {
                        if (reg_count.count(unary->getsrc())) {
                            for (auto &item : reg_count.at(unary->getsrc())) {
                                dst_count[item.first] = -item.second;
                            }
                        } else {
                            dst_count[unary->getsrc()] = -1;
                        }
                        dst_const = -const_count[unary->getsrc()];
                    }
                    reg_count[unary->getdst()] = dst_count;
                    const_count[unary->getdst()] = dst_const;
                } else {
                    continue;
                }
            }
        }
        func->set_temp_used(next_temp_id_);
    }
}

} // namespace middleend