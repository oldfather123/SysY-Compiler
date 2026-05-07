#include "ADT/CFG.h"
#include "frontend.h"
#include "sym.h"
#include <algorithm>
#include <vector>

namespace aaac {
using namespace frontend;
namespace optimization {

class StrengthReduction {
public:
    void run(frontend::ControlFlowGraph *cfg) const;
private:
    static bool isPowerOfTwo(int c) { return c > 0 && (c & (c - 1)) == 0; }
    static int powerOfTwoLog(int c) { return __builtin_ctz(c); /* builtins 理论上会快很多 */}
    static std::vector<int> decomposeToPowers(int c) {
        std::vector<int> bits;
        for (int i = 0; c; ++i) {
            if (c & 1) bits.push_back(i);
            c >>= 1;
        }
        return bits;
    }
    // 以防后面需要重启对除法的优化
    // static std::pair<uint64_t, int> magicForDiv(uint64_t d) {
    //     int l = 63 - __builtin_clzll(d);
    //     uint64_t m = ((1ULL << (32 + l)) + d - 1) / d;
    //     return {m, l};
    // }
};

void StrengthReduction::run(frontend::ControlFlowGraph *cfg) const {
    for (auto bb_ptr : cfg->getBBList()) {
        auto bb = bb_ptr.get();
        for (auto it = bb->begin(); it != bb->end();) {
            auto node = *it;
            auto assign = std::dynamic_pointer_cast<frontend::AssignNode>(node);
            if (!assign) {
                ++it;
                continue;
            }

            if (assign->getOpCode() == frontend::OpCode::Mul) {
                auto ops = assign->getOperands();
                std::shared_ptr<sym::Var> varOp;
                int constant = 0;
                bool matched = false;
                if (std::holds_alternative<std::shared_ptr<sym::Var>>(ops[0]) &&
                    std::holds_alternative<int>(ops[1])) {
                    varOp = std::get<std::shared_ptr<sym::Var>>(ops[0]);
                    constant = std::get<int>(ops[1]);
                    matched = true;
                } else if (std::holds_alternative<int>(ops[0]) &&
                           std::holds_alternative<std::shared_ptr<sym::Var>>(ops[1])) {
                    varOp = std::get<std::shared_ptr<sym::Var>>(ops[1]);
                    constant = std::get<int>(ops[0]);
                    matched = true;
                }
                if (!matched || constant <= 0) {
                    ++it;
                    continue;
                }
                auto destVar = assign->getResult();
                auto insertIt = it;
                auto parts = decomposeToPowers(constant);
                std::sort(parts.rbegin(), parts.rend());

                // 使用临时变量来累积结果，保证SSA形式
                std::shared_ptr<sym::Var> acc;
                int firstShift = parts.front();
                if (firstShift != 0) {
                    acc = sym::Var::generateTemp(varOp->getType(), "sr");
                    bb->insertNodeBefore(insertIt,
                        frontend::LShiftAssignNode::create(
                            acc,
                            frontend::OperandList{frontend::Operand(varOp), frontend::Operand(firstShift)}));
                } else {
                    acc = sym::Var::generateTemp(varOp->getType(), "sr");
                    bb->insertNodeBefore(insertIt,
                        frontend::CopyAssignNode::create(
                            acc, frontend::OperandList{frontend::Operand(varOp)}));
                }

                for (size_t idx = 1; idx < parts.size(); ++idx) {
                    // 处理每个分解出的部分
                    std::shared_ptr<sym::Var> shifted;
                    if (parts[idx] != 0) {
                        shifted = sym::Var::generateTemp(varOp->getType(), "sr");
                        bb->insertNodeBefore(insertIt,
                            frontend::LShiftAssignNode::create(
                                shifted,
                                frontend::OperandList{frontend::Operand(varOp), frontend::Operand(parts[idx])}));
                    } else {
                        shifted = sym::Var::generateTemp(varOp->getType(), "sr");
                        bb->insertNodeBefore(insertIt,
                            frontend::CopyAssignNode::create(
                                shifted, frontend::OperandList{frontend::Operand(varOp)}));
                    }
                    auto newAcc = sym::Var::generateTemp(varOp->getType(), "sr");
                    bb->insertNodeBefore(insertIt,
                        frontend::AddAssignNode::create(
                            newAcc,
                            frontend::OperandList{frontend::Operand(acc), frontend::Operand(shifted)}));
                    acc = newAcc;
                }

                // 最终将累积的结果赋值给原目标变量
                bb->insertNodeBefore(insertIt,
                    frontend::CopyAssignNode::create(
                        destVar, frontend::OperandList{frontend::Operand(acc)}));

                it = bb->removeNode(it);
                continue;
            } else if (assign->getOpCode() == frontend::OpCode::Div) {
                auto ops = assign->getOperands();
                std::shared_ptr<sym::Var> varOp;
                int constant = 0;
                bool matched = false;
                if (std::holds_alternative<std::shared_ptr<sym::Var>>(ops[0]) &&
                    std::holds_alternative<int>(ops[1])) {
                    varOp = std::get<std::shared_ptr<sym::Var>>(ops[0]);
                    constant = std::get<int>(ops[1]);
                    matched = true;
                }
                if (!matched || constant <= 0 || !isPowerOfTwo(constant)) {
                    ++it;
                    continue;
                }
                auto destVar = assign->getResult();
                auto insertIt = it;
                int shift = powerOfTwoLog(constant);
                int bias = constant - 1;
                int signShift = varOp->getType()->getTypeSize() * 8 - 1;

                auto signVar = sym::Var::generateTemp(varOp->getType(), "sr");
                bb->insertNodeBefore(insertIt,
                    frontend::RShiftAssignNode::create(
                        signVar,
                        frontend::OperandList{frontend::Operand(varOp), frontend::Operand(signShift)}));

                auto maskVar = sym::Var::generateTemp(varOp->getType(), "sr");
                bb->insertNodeBefore(insertIt,
                    frontend::BitAndAssignNode::create(
                        maskVar,
                        frontend::OperandList{frontend::Operand(signVar), frontend::Operand(bias)}));

                auto addVar = sym::Var::generateTemp(varOp->getType(), "sr");
                bb->insertNodeBefore(insertIt,
                    frontend::AddAssignNode::create(
                        addVar,
                        frontend::OperandList{frontend::Operand(varOp), frontend::Operand(maskVar)}));

                auto resultVar = sym::Var::generateTemp(varOp->getType(), "sr");
                bb->insertNodeBefore(insertIt,
                    frontend::RShiftAssignNode::create(
                        resultVar,
                        frontend::OperandList{frontend::Operand(addVar), frontend::Operand(shift)}));

                bb->insertNodeBefore(insertIt,
                    frontend::CopyAssignNode::create(
                        destVar, frontend::OperandList{frontend::Operand(resultVar)}));

                it = bb->removeNode(it);
                continue;
            }
            ++it;
        }
    }
}

} // namespace optimization
} // namespace aaac

void StrengthReduction(aaac::frontend::ControlFlowGraph *cfg) {
    aaac::optimization::StrengthReduction().run(cfg);
}
