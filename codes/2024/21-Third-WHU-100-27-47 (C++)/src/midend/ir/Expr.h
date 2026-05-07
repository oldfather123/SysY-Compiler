#pragma once

#include "Common.h"
#include "Instruction.h"

namespace ir {
    // TODO Better hash? Only support DefInst?
    struct Expr {
        static unsigned counter;

        String str = "";
        String hashStr = "";
        bool canBeAvailable = false;
        Set<Ptr<DefInst>> evalInsts{};

        Expr() { }

        explicit Expr(Ptr<DefInst> inst) {
            evalInsts.insert(inst);
            canBeAvailable = true;
            if (inst->is<MoveInst>() || inst->is<AllocInst>()) {
                // 复制指令由于可以被传播，就不作为可用表达式来分析了
                // 内存分配由于其副作用，不可作为可用表达式
                // 字符串头部加入内容混淆
                canBeAvailable = false;
            }
            if (inst->is<CallInst>()) {
                // 函数调用如果有副作用，则不可作为可用表达式
                auto callInst = inst->as<CallInst>();
                if (!callInst->function()->isPure()) {
                    canBeAvailable = false;
                }
            }
            if (!canBeAvailable) {
                hashStr += std::to_string(counter) + "\n";
            }
            str = inst->toString(true);
            canBeAvailable &= !str.empty();
            hashStr += str;
            if (auto operInst = inst->as<OperInst>()) {
                // 交换律
                switch (operInst->op()) {
                    case OperInst::Operator::Add:
                    case OperInst::Operator::Mul: {
                        String altHashStr = OperInst::createBinary(nullptr, operInst->destReg(), operInst->op(), operInst->rhs(), operInst->lhs())->toString(true);
                        // 字典序
                        if (hashStr < altHashStr) {
                            hashStr += "\n" + altHashStr;
                        } else {
                            hashStr = altHashStr + "\n" + hashStr;
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            ++counter;
        }

        bool operator==(const Expr &other) const {
            return this->hashStr == other.hashStr && !this->hashStr.empty();
        }
        bool operator!=(const Expr &other) const {
            return !(*this == other);
        }
        operator bool() const {
            return !str.empty();
        }

        String toString() const { return str.empty() ? "null" : str; }
    };
}

namespace std {
    template <>
    struct hash<ir::Expr> {
        size_t operator()(const ir::Expr &expr) const {
            size_t result = 0;
            hashCombine(result, expr.hashStr);
            return result;
        }
    };
}
