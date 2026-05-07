#include "../../include/ir/instcombine.h"
#include "../../include/ir/cfg.h"
#include <set>
#include <cmath>
#include <iostream>
#include <functional>
#if __cpp_lib_bitops
#include <bit>
#endif
#include <cassert>
#include <unordered_map>

namespace llvm_ir {
namespace instcombine {

// instcombine 临时变量命名计数器
static int instcombine_temp_counter = 0;
static std::string getInstCombineTempName(std::string prefix = "") {
    if (prefix.empty()) prefix = "t";
    return "%" + prefix + "_ic_" + std::to_string(instcombine_temp_counter++);
}

// 判断是否为可交换的二元操作
static bool isCommutative(Opcode op) {
    return op == Opcode::Add || op == Opcode::Mul || op == Opcode::And || op == Opcode::Or || op == Opcode::Xor;
}

// 判断是否为常量
static bool isConstant(Value* v) {
    return dynamic_cast<ConstantInt*>(v) || dynamic_cast<ConstantFloat*>(v);
}

// 获取常量整数值
static int32_t getConstantInt(Value* v) {
    if (auto ci = dynamic_cast<ConstantInt*>(v)) return ci->value;
    return 0;
}

// 获取常量浮点数值
static float getConstantFloat(Value* v) {
    if (auto cf = dynamic_cast<ConstantFloat*>(v)) return cf->value;
    return 0.0f;
}

// 判断是否为2的幂
static bool isPowerOfTwo(int32_t x) {
    return x > 0 && (x & (x - 1)) == 0;
}

// 判断是否为布尔类型
static bool isBoolType(Value* v) {
    return v && v->type == Type::I1;
}

// =============== 连续加法优化 ===============
// 判断两个加法指令是否可以合并（连续加法优化）
// 例如：(a + c1) + c2 -> a + (c1 + c2)
static bool canCombineAddInstructions(BinaryOperator* add1, BinaryOperator* add2) {
    if (!add1 || !add2) return false;
    
    // 检查是否都是加法指令（整数或浮点）
    if ((add1->opcode != Opcode::Add && add1->opcode != Opcode::FAdd) || 
        (add2->opcode != Opcode::Add && add2->opcode != Opcode::FAdd)) return false;
    
    // 检查第二个操作数是否都是常量
    if (!isConstant(add1->operands[1]) || !isConstant(add2->operands[1])) return false;
    
    // 检查第二个指令的第一个操作数是否是第一个指令的结果
    if (add2->operands[0] != add1) return false;
    
    // 检查类型一致性
    if (add1->opcode != add2->opcode) return false;
    
    // 根据操作类型检查溢出
    if (add1->opcode == Opcode::Add) {
        // 整数加法溢出检查
        int32_t const1 = getConstantInt(add1->operands[1]);
        int32_t const2 = getConstantInt(add2->operands[1]);
        
        // 检查溢出（简化版本）
        if (const1 > 0 && const2 > 0 && const1 > INT32_MAX - const2) return false;
        if (const1 < 0 && const2 < 0 && const1 < INT32_MIN - const2) return false;
    } else {
        // 浮点加法，不需要溢出检查
        // 但可以检查是否为NaN或无穷大（这里简化处理）
    }
    
    return true;
}

// 合并两个连续的加法指令
// 返回新创建的指令指针，如果合并失败返回nullptr
static BinaryOperator* combineConsecutiveAdds(BinaryOperator* add1, BinaryOperator* add2) {
    if (!canCombineAddInstructions(add1, add2)) return nullptr;
    
    BinaryOperator* newAddPtr = nullptr;
    
    if (add1->opcode == Opcode::Add) {
        // 整数加法合并
        int32_t const1 = getConstantInt(add1->operands[1]);
        int32_t const2 = getConstantInt(add2->operands[1]);
        int32_t combinedConst = const1 + const2;
        
        // 创建新的合并后的整数加法指令
        auto newAdd = std::make_unique<BinaryOperator>(
            Opcode::Add, 
            add1->operands[0],  // 第一个指令的第一个操作数
            new ConstantInt(combinedConst), 
            getInstCombineTempName(add1->name.substr(1))
        );
        
        newAddPtr = newAdd.get();
        
        std::cout << "[instcombine] 连续整数加法合并: (" << add1->toString() << ") + " 
                  << getConstantInt(add2->operands[1]) << " -> " << newAdd->toString() << std::endl;
        
        add2->parent->replaceInstructionWith(add2, std::move(newAdd));
    } else {
        // 浮点加法合并
        float const1 = getConstantFloat(add1->operands[1]);
        float const2 = getConstantFloat(add2->operands[1]);
        float combinedConst = const1 + const2;
        
        // 创建新的合并后的浮点加法指令
        auto newAdd = std::make_unique<BinaryOperator>(
            Opcode::FAdd, 
            add1->operands[0],  // 第一个指令的第一个操作数
            new ConstantFloat(combinedConst), 
            getInstCombineTempName(add1->name.substr(1))
        );
        
        newAddPtr = newAdd.get();
        
        std::cout << "[instcombine] 连续浮点加法合并: (" << add1->toString() << ") + " 
                  << getConstantFloat(add2->operands[1]) << " -> " << newAdd->toString() << std::endl;

        add2->parent->replaceInstructionWith(add2, std::move(newAdd));
    }
    
    // add2->parent->replaceInstructionWith(add2, std::move(std::unique_ptr<Instruction>(newAddPtr))); // 会删除add2，自动维护use关系
    
    return newAddPtr;
}

// 基于支配树的加法指令合并优化
static bool domTreeAddCombine(Function& F, cfg::DominatorTree& DT) {
    bool changed = false;
    std::set<BinaryOperator*> addInstConstantSet;
    
    // DFS遍历支配树
    std::function<void(BasicBlock*)> dfs = [&](BasicBlock* bb) {
        std::set<BinaryOperator*> tmpSet;
        
        // 遍历当前基本块中的指令
        for (auto& inst : bb->instructions) {
            if (auto bin = dynamic_cast<BinaryOperator*>(inst.get())) {
                if ((bin->opcode == Opcode::Add || bin->opcode == Opcode::FAdd) && isConstant(bin->operands[1])) {
                    bool isCombined = false;
                    // 检查是否可以与已有的加法指令合并（连续加法优化）
                    for (auto oldInst : addInstConstantSet) {
                        if (canCombineAddInstructions(oldInst, bin)) {
                            BinaryOperator* newAdd = combineConsecutiveAdds(oldInst, bin);
                            if (newAdd) {
                                changed = true;
                                isCombined = true;
                                // 从集合中移除旧指令，因为已经被合并
                                addInstConstantSet.erase(oldInst);
                                tmpSet.erase(oldInst);
                                // 将新创建的指令添加到集合中，以便后续可能的合并
                                addInstConstantSet.insert(newAdd);
                                tmpSet.insert(newAdd);
                                break;
                            }
                        }
                    }
                    
                    // 如果没有合并，添加到集合中
                    if (!isCombined && addInstConstantSet.find(bin) == addInstConstantSet.end()) {
                        addInstConstantSet.insert(bin);
                        tmpSet.insert(bin);
                    }
                }
            }
        }
        
        // 递归处理支配的子节点
        for (auto child : DT.getChildren(bb)) {
            dfs(child);
        }
        
        // 从全局集合中移除当前块添加的指令
        for (auto inst : tmpSet) {
            addInstConstantSet.erase(inst);
        }
    };
    
    // 从入口块开始DFS
    BasicBlock* entry = F.getEntryBlock();
    if (entry) {
        dfs(entry);
    }
    return changed;
}

// =============== 规则匹配与替换 ===============
// 规则匹配与替换
static bool tryCombine(BasicBlock* bb, std::list<std::unique_ptr<Instruction>>::iterator& it, IRBuilder& builder, std::set<Instruction*>& toDelete) {
    Instruction* inst = it->get();
    if (auto bin = dynamic_cast<BinaryOperator*>(inst)) {
        // 1. 常量右置
        if (isCommutative(bin->opcode) && isConstant(bin->operands[0]) && !isConstant(bin->operands[1])) {
            std::swap(bin->operands[0], bin->operands[1]);
            std::cout << "[instcombine] 1. 常量右置: " << bin->toString() << std::endl;
            return true;
        }
        // 2. %r1 = sub i32 %r0,4 -> %r1 = add i32 %r0,-4
        if (bin->opcode == Opcode::Sub && isConstant(bin->operands[1])) {
            int32_t c = getConstantInt(bin->operands[1]);
            if (c) {
                auto newAdd = std::make_unique<BinaryOperator>(Opcode::Add, bin->operands[0], new ConstantInt(-c), bin->name);
                BinaryOperator* newAddPtr = newAdd.get();
                newAddPtr->parent = bb;
                bb->instructions.insert(it, std::move(newAdd));
                bin->replaceAllUsesWith(newAddPtr);
                toDelete.insert(bin);
                std::cout << "[instcombine] sub X, const -> add X, -const: " << newAddPtr->toString() << std::endl;
                return true;
            }
        }

        // =============== 算术表达式化简规则 ===============
        // 规则1: %r = {a - (a - b)} -> %r = {(a - a) + b} -> %r = {b + 0} -> %r = {b}
        if (bin->opcode == Opcode::Sub) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::Sub && bin->operands[0] == subOp->operands[0]) {
                    // a - (a - b) -> b
                    std::cout << "[instcombine] a - (a - b) -> b: " << bin->toString() << " -> " << subOp->operands[1]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[1]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }
        
        // 规则2: %r = {(a - b) + b} -> %r = {a - (b - b)} -> %r = {a + 0} -> %r = {a}
        if (bin->opcode == Opcode::Add) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                if (subOp->opcode == Opcode::Sub && subOp->operands[1] == bin->operands[1]) {
                    // (a - b) + b -> a
                    std::cout << "[instcombine] (a - b) + b -> a: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::Sub && subOp->operands[1] == bin->operands[0]) {
                    // b + (a - b) -> a
                    std::cout << "[instcombine] b + (a - b) -> a: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }
        
        // 规则3: %r = {a - (a + b)} -> %r = {(a - a) - b} -> %r = {0 - b} -> %r = {-b}
        if (bin->opcode == Opcode::Sub) {
            if (auto addOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (addOp->opcode == Opcode::Add && bin->operands[0] == addOp->operands[0]) {
                    // a - (a + b) -> -b
                    auto negB = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), addOp->operands[1], getInstCombineTempName());
                    BinaryOperator* negBPtr = negB.get();
                    negB->parent = bb;
                    bb->instructions.insert(it, std::move(negB));
                    bin->replaceAllUsesWith(negBPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] a - (a + b) -> -b: " << bin->toString() << " -> " << negBPtr->toString() << std::endl;
                    return true;
                }
                if (addOp->opcode == Opcode::Add && bin->operands[0] == addOp->operands[1]) {
                    // a - (b + a) -> -b
                    auto negB = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), addOp->operands[0], getInstCombineTempName());
                    BinaryOperator* negBPtr = negB.get();
                    negB->parent = bb;
                    bb->instructions.insert(it, std::move(negB));
                    bin->replaceAllUsesWith(negBPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] a - (b + a) -> -b: " << bin->toString() << " -> " << negBPtr->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 规则4: %r = {a + (b - a)} -> %r = {(a - a) + b} -> %r = {b + 0} -> %r = {b}
        if (bin->opcode == Opcode::Add) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::Sub && subOp->operands[1] == bin->operands[0]) {
                    // a + (b - a) -> b
                    std::cout << "[instcombine] a + (b - a) -> b: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                if (subOp->opcode == Opcode::Sub && subOp->operands[1] == bin->operands[1]) {
                    // (b - a) + a -> b
                    std::cout << "[instcombine] (b - a) + a -> b: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }

        // =============== 强度削减 ===============
        // udiv X, pow2 -> lshr X, log2(pow2)
        if (bin->opcode == Opcode::UDiv && isConstant(bin->operands[1])) {
            std::cerr << "[instcombine][Warning] UDiv Detected!" << std::endl;
//             int32_t c = getConstantInt(bin->operands[1]);
//             std::cout << "[instcombine][debug] div pow2 check: c=" << c << std::endl;
//             if (c > 1 && isPowerOfTwo(c)) {
//                 int shift;
// #if __cpp_lib_bitops
//                 shift = std::countr_zero(static_cast<unsigned int>(c));
// #elif defined(__GNUC__) || defined(__clang__)
//                 shift = __builtin_ctz(static_cast<unsigned int>(c));
// #else
//                 int countTrailingZeros(int32_t n) {
//                     if (n == 0) return 32;
//                     int count = 0;
//                     while ((n & 1) == 0) { n >>= 1; count++; }
//                     return count;
//                 }
//                 shift = countTrailingZeros(c);
// #endif
//                 auto newShr = std::make_unique<BinaryOperator>(Opcode::LShr, bin->operands[0], new ConstantInt(shift), getInstCombineTempName());
//                 BinaryOperator* newShrPtr = newShr.get();
//                 newShr->parent = bb;
//                 bb->instructions.insert(it, std::move(newShr));
//                 bin->replaceAllUsesWith(newShrPtr);
//                 toDelete.insert(bin);
//                 std::cout << "[instcombine] udiv X, pow2 -> lshr X, log2(pow2): " << bin->toString() << std::endl;
//                 return true;
//             }
            
//             // 无符号除法常量合并: a / c1 / c2 -> a / (c1*c2)
//             // 无符号除法常量合并: a / c1 / c2 -> a / (c1*c2)
//             if (auto divOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
//                 if (divOp->opcode == Opcode::UDiv && isConstant(divOp->operands[1])) {
//                     int32_t c1 = getConstantInt(divOp->operands[1]);
//                     int32_t c2 = c;
                    
//                     // 检查乘法是否会溢出（无符号除法，常量应该都是正数）
//                     if (c1 > 0 && c2 > 0) {
//                         if (c1 > INT32_MAX / c2) return false; // 溢出
                        
//                         int32_t combinedConst = c1 * c2;
//                         auto newDiv = std::make_unique<BinaryOperator>(Opcode::UDiv, divOp->operands[0], new ConstantInt(combinedConst), getInstCombineTempName());
//                         BinaryOperator* newDivPtr = newDiv.get();
//                         newDivPtr->parent = bb;
//                         bb->instructions.insert(it, std::move(newDiv));
//                         bin->replaceAllUsesWith(newDivPtr);
//                         toDelete.insert(bin);
//                         std::cout << "[instcombine] a / c1 / c2 -> a / (c1*c2) [unsigned]: " << bin->toString() << " -> " << newDivPtr->toString() << std::endl;
//                         return true;
//                     }
//                 }
//             }
        }
        // urem X, pow2 -> and X, pow2-1
        if (bin->opcode == Opcode::URem && isConstant(bin->operands[1])) {
            int32_t c = getConstantInt(bin->operands[1]);
            std::cout << "[instcombine][debug] rem pow2 check: c=" << c << std::endl;
            if (c > 1 && isPowerOfTwo(c)) {
                int32_t mask = c - 1;
                auto newAnd = std::make_unique<BinaryOperator>(Opcode::And, bin->operands[0], new ConstantInt(mask), getInstCombineTempName());
                BinaryOperator* newAndPtr = newAnd.get();
                newAndPtr->parent = bb;
                bb->instructions.insert(it, std::move(newAnd));
                bin->replaceAllUsesWith(newAndPtr);
                toDelete.insert(bin);
                std::cout << "[instcombine] urem X, pow2 -> and X, pow2-1: " << bin->toString() << std::endl;
                return true;
            }
        }
        // Granlund-Montgomery sdiv by constant 优化
        if (bin->opcode == Opcode::SDiv && isConstant(bin->operands[1])) {
            int32_t d = getConstantInt(bin->operands[1]);
            Value* x = bin->operands[0];
            if (d == 0) return false; // 除零未定义
            if (d == 1) {
                std::cout << "[instcombine][sdiv] sdiv X, 1 -> X: " << bin->toString() << std::endl;
                bin->replaceAllUsesWith(x);
                toDelete.insert(bin);
                return true;
            }
            if (d == -1) {
                std::cout << "[instcombine][sdiv] sdiv X, -1 -> neg X: " << bin->toString() << std::endl;
                auto neg = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), x, getInstCombineTempName());
                BinaryOperator* negPtr = neg.get();
                negPtr->parent = bb;
                bb->instructions.insert(it, std::move(neg));
                bin->replaceAllUsesWith(negPtr);
                toDelete.insert(bin);
                return true;
            }
            
            // 除法常量合并: a / c1 / c2 -> a / (c1*c2)
            if (auto divOp = dynamic_cast<BinaryOperator*>(x)) {
                if (divOp->opcode == Opcode::SDiv && isConstant(divOp->operands[1])) {
                    int32_t c1 = getConstantInt(divOp->operands[1]);
                    int32_t c2 = d;
                    
                    // 检查乘法是否会溢出
                    if (c1 != 0 && c2 != 0) {
                        // 检查乘法溢出
                        if (c1 > 0 && c2 > 0) {
                            if (c1 > INT32_MAX / c2) return false; // 溢出
                        } else if (c1 < 0 && c2 < 0) {
                            if (c1 < INT32_MAX / c2) return false; // 溢出
                        } else if (c1 > 0 && c2 < 0) {
                            if (c1 > INT32_MIN / c2) return false; // 溢出
                        } else if (c1 < 0 && c2 > 0) {
                            if (c1 < INT32_MIN / c2) return false; // 溢出
                        }
                        
                        int32_t combinedConst = c1 * c2;
                        auto newDiv = std::make_unique<BinaryOperator>(Opcode::SDiv, divOp->operands[0], new ConstantInt(combinedConst), getInstCombineTempName());
                        BinaryOperator* newDivPtr = newDiv.get();
                        newDivPtr->parent = bb;
                        bb->instructions.insert(it, std::move(newDiv));
                        bin->replaceAllUsesWith(newDivPtr);
                        toDelete.insert(bin);
                        std::cout << "[instcombine] a / c1 / c2 -> a / (c1*c2): " << bin->toString() << " -> " << newDivPtr->toString() << std::endl;
                        return true;
                    }
                }
            }
            int32_t ad = std::abs(d);
            // 2的幂分支
            if (isPowerOfTwo(ad)) {
                int k;
    #if __cpp_lib_bitops
                k = std::countr_zero(static_cast<unsigned int>(ad));
    #elif defined(__GNUC__) || defined(__clang__)
                k = __builtin_ctz(static_cast<unsigned int>(ad));
    #else
                int countTrailingZeros(int32_t n) {
                    if (n == 0) return 32;
                    int count = 0;
                    while ((n & 1) == 0) { n >>= 1; count++; }
                    return count;
                }
                k = countTrailingZeros(ad);
    #endif
                std::cout << "[instcombine][sdiv] sdiv X, pow2: d=" << d << ", k=" << k << std::endl;
                // sign = ashr x, 31
                auto sign = std::make_unique<BinaryOperator>(Opcode::AShr, x, new ConstantInt(31), getInstCombineTempName());
                BinaryOperator* signPtr = sign.get();
                signPtr->parent = bb;
                bb->instructions.insert(it, std::move(sign));
                // mask = and sign, (2^k-1)
                auto mask = std::make_unique<BinaryOperator>(Opcode::And, signPtr, new ConstantInt((1 << k) - 1), getInstCombineTempName());
                BinaryOperator* maskPtr = mask.get();
                maskPtr->parent = bb;
                bb->instructions.insert(it, std::move(mask));
                // added = add x, mask
                auto added = std::make_unique<BinaryOperator>(Opcode::Add, x, maskPtr, getInstCombineTempName());
                BinaryOperator* addedPtr = added.get();
                addedPtr->parent = bb;
                bb->instructions.insert(it, std::move(added));
                // res = ashr added, k
                auto res = std::make_unique<BinaryOperator>(Opcode::AShr, addedPtr, new ConstantInt(k), getInstCombineTempName());
                BinaryOperator* resPtr = res.get();
                resPtr->parent = bb;
                bb->instructions.insert(it, std::move(res));
                if (d < 0) {
                    // 结果取负
                    auto neg = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), resPtr, getInstCombineTempName());
                    BinaryOperator* negPtr = neg.get();
                    negPtr->parent = bb;
                    bb->instructions.insert(it, std::move(neg));
                    bin->replaceAllUsesWith(negPtr);
                } else {
                    bin->replaceAllUsesWith(resPtr);
                }
                toDelete.insert(bin);
                return true;
            }
            // 负常量分支，转为 -x / -d
            if (d < 0) {
                std::cout << "[instcombine][sdiv] sdiv X, -d: 先取-x, 再除以正数: " << bin->toString() << std::endl;
                auto negx = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), x, getInstCombineTempName());
                BinaryOperator* negxPtr = negx.get();
                negxPtr->parent = bb;
                bb->instructions.insert(it, std::move(negx));
                // 构造新的 sdiv 指令: sdiv -x, |d|
                auto newSdiv = std::make_unique<BinaryOperator>(Opcode::SDiv, negxPtr, new ConstantInt(-d), getInstCombineTempName());
                BinaryOperator* newSdivPtr = newSdiv.get();
                newSdivPtr->parent = bb;
                bb->instructions.insert(it, std::move(newSdiv));
                bin->replaceAllUsesWith(newSdivPtr);
                toDelete.insert(bin);
                // 递归优化会自动处理 newSdiv
                return true;
            }
            // Granlund-Montgomery魔数分支（此时 d > 0, 非2的幂）
            // struct MagicInfo {
            //     int32_t multiplier;
            //     int32_t shift;
            // };
            
            // // ** 使用 Hacker's Delight (《算法心得》) 中的标准算法进行修正 **
            // auto compute_magic_signed = [](int32_t d) -> MagicInfo {
            //     // 该算法适用于正除数 d > 1
            //     assert(d > 1);
                
            //     // 1. 计算 l = floor(log2(d-1))
            //     // 使用 __builtin_clz (count leading zeros) 可以高效计算
            //     int l = 31 - __builtin_clz(static_cast<uint32_t>(d - 1));

            //     // 2. 使用128位整数计算魔数 M，避免溢出
            //     // M = floor( (2^(31+l)) / d ) + 1
            //     int p = 31 + l;
            //     __int128_t one = 1;
            //     __int128_t m_unsigned = (one << p) / d + 1;

            //     // 3. 将无符号魔数转换为有符号32位整数，并返回位移量
            //     int32_t multiplier = static_cast<int32_t>(m_unsigned);
            //     return {multiplier, l};
            // };
            
            // MagicInfo magic = compute_magic_signed(d);
            // int32_t m = magic.multiplier;
            // int32_t s = magic.shift;
            // std::cout << "[instcombine][sdiv] sdiv X, const: d=" << d << ", m=" << m << ", shift=" << s << std::endl;
            
            // // --- 指令序列生成 (这部分原先就是正确的) ---
            // // 最终目标是实现 result = ( (mulhi(x, m) + (x >> 31)) >> s )

            // // 为了计算 mulhi(x, m)，我们将 x 和 m 都扩展到64位，然后做乘法，再取高32位
            // // 1. sext x to i64
            // auto x64 = std::make_unique<SExtInst>(x, Type::I64, getInstCombineTempName());
            // Value* x64Ptr = x64.get();
            // bb->instructions.insert(it, std::move(x64));
            
            // // 2. prod = mul i64 x64, (sext i32 m to i64)
            // auto prod = std::make_unique<BinaryOperator>(Opcode::Mul, x64Ptr, new ConstantInt(static_cast<int64_t>(m), Type::I64), getInstCombineTempName());
            // Value* prodPtr = prod.get();
            // bb->instructions.insert(it, std::move(prod));
            
            // // 3. q_i64 = ashr i64 prod, 32  (这就是 mulhi(x, m) 的结果，保持为i64)
            // auto q_i64 = std::make_unique<BinaryOperator>(Opcode::AShr, prodPtr, new ConstantInt(32, Type::I64), getInstCombineTempName());
            // Value* q_i64_ptr = q_i64.get();
            // bb->instructions.insert(it, std::move(q_i64));
            
            // // 4. 计算符号修正值: sign_corr = ashr i32 x, 31
            // auto sign_corr_i32 = std::make_unique<BinaryOperator>(Opcode::AShr, x, new ConstantInt(31), getInstCombineTempName());
            // Value* sign_corr_i32_ptr = sign_corr_i32.get();
            // bb->instructions.insert(it, std::move(sign_corr_i32));

            // // 5a. sext sign_corr_i32 to i64
            // auto sign_corr_i64 = std::make_unique<SExtInst>(sign_corr_i32_ptr, Type::I64, getInstCombineTempName());
            // Value* sign_corr_i64_ptr = sign_corr_i64.get();
            // bb->instructions.insert(it, std::move(sign_corr_i64));
            
            // // 5b. 将 q 和符号修正值相加: added = add q_i64, (sext sign_corr_i32 to i64)
            // auto added = std::make_unique<BinaryOperator>(Opcode::Add, q_i64_ptr, sign_corr_i64_ptr, getInstCombineTempName());
            // Value* addedPtr = added.get();
            // bb->instructions.insert(it, std::move(added));
            
            // // 6. 对相加后的结果进行最后的算术右移
            // auto shifted = std::make_unique<BinaryOperator>(Opcode::AShr, addedPtr, new ConstantInt(s, Type::I64), getInstCombineTempName());
            // Value* shiftedPtr = shifted.get();
            // bb->instructions.insert(it, std::move(shifted));
            
            // // 7. 将 i64 结果截断回 i32
            // auto finalRes = std::make_unique<TruncInst>(shiftedPtr, Type::I32, getInstCombineTempName());
            // Value* finalResPtr = finalRes.get();
            // bb->instructions.insert(it, std::move(finalRes));
            
            // bin->replaceAllUsesWith(finalResPtr);
            // toDelete.insert(bin);
            // return true;
        }
        
        if (bin->opcode == Opcode::Mul && isConstant(bin->operands[1])) {
            int32_t c = getConstantInt(bin->operands[1]);
            // mul X, 0 -> 0
            if (c == 0) {
                if (bin->type == Type::Float) {
                    bin->replaceAllUsesWith(new ConstantFloat(0.0f));
                } else {
                    bin->replaceAllUsesWith(new ConstantInt(0, bin->type));
                }
                toDelete.insert(bin);
                std::cout << "[instcombine] mul X, 0 -> 0: " << bin->toString() << std::endl;
                return true;
            }
            // mul X, 1 -> X
            if (c == 1) {
                bin->replaceAllUsesWith(bin->operands[0]);
                toDelete.insert(bin);
                std::cout << "[instcombine] mul X, 1 -> X: " << bin->toString() << std::endl;
                return true;
            }
            // mul X, -1 -> sub 0, X
            if (c == -1) {
                auto newSub = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), bin->operands[0], getInstCombineTempName());
                BinaryOperator* newSubPtr = newSub.get();
                newSubPtr->parent = bb;
                bb->instructions.insert(it, std::move(newSub));
                bin->replaceAllUsesWith(newSubPtr);
                toDelete.insert(bin);
                std::cout << "[instcombine] mul X, -1 -> sub 0, X: " << bin->toString() << std::endl;
                return true;
            }
            // mul X, 2^k -> shl X, k
            if (c > 1 && isPowerOfTwo(c)) {
                int shift;
#if __cpp_lib_bitops
                shift = std::countr_zero(static_cast<unsigned int>(c));
#elif defined(__GNUC__) || defined(__clang__)
                shift = __builtin_ctz(static_cast<unsigned int>(c));
#else
                int countTrailingZeros(int32_t n) {
                    if (n == 0) return 32;
                    int count = 0;
                    while ((n & 1) == 0) { n >>= 1; count++; }
                    return count;
                }
                shift = countTrailingZeros(c);
#endif
                auto newShl = std::make_unique<BinaryOperator>(Opcode::Shl, bin->operands[0], new ConstantInt(shift), getInstCombineTempName());
                BinaryOperator* newShlPtr = newShl.get();
                newShlPtr->parent = bb;
                bb->instructions.insert(it, std::move(newShl));
                bin->replaceAllUsesWith(newShlPtr);
                toDelete.insert(bin);
                std::cout << "[instcombine] mul X, pow2 -> shl X, log2(pow2): " << bin->toString() << std::endl;
                return true;
            }
            // mul X, -2^k -> shl X, k，再取负
            if (c < 0 && isPowerOfTwo(-c)) {
                int shift;
#if __cpp_lib_bitops
                shift = std::countr_zero(static_cast<unsigned int>(-c));
#elif defined(__GNUC__) || defined(__clang__)
                shift = __builtin_ctz(static_cast<unsigned int>(-c));
#else
                int countTrailingZeros(int32_t n) {
                    if (n == 0) return 32;
                    int count = 0;
                    while ((n & 1) == 0) { n >>= 1; count++; }
                    return count;
                }
                shift = countTrailingZeros(-c);
#endif
                auto newShl = std::make_unique<BinaryOperator>(Opcode::Shl, bin->operands[0], new ConstantInt(shift), getInstCombineTempName());
                BinaryOperator* newShlPtr = newShl.get();
                newShlPtr->parent = bb;
                bb->instructions.insert(it, std::move(newShl));
                auto newNeg = std::make_unique<BinaryOperator>(Opcode::Sub, new ConstantInt(0), newShlPtr, getInstCombineTempName());
                BinaryOperator* newNegPtr = newNeg.get();
                newNegPtr->parent = bb;
                bb->instructions.insert(it, std::move(newNeg));
                bin->replaceAllUsesWith(newNegPtr);
                toDelete.insert(bin);
                std::cout << "[instcombine] mul X, -pow2 -> shl X, log2(pow2) 再取负: " << bin->toString() << std::endl;
                return true;
            }

        }
        // X << 0 -> X, X >> 0 -> X
        if ((bin->opcode == Opcode::Shl || bin->opcode == Opcode::LShr || bin->opcode == Opcode::AShr) && isConstant(bin->operands[1]) && getConstantInt(bin->operands[1]) == 0) {
            bin->replaceAllUsesWith(bin->operands[0]);
            toDelete.insert(bin);
            std::cout << "[instcombine] shift by 0 -> X: " << bin->toString() << std::endl;
            return true;
        }
        // (X & X) -> X
        if ((bin->opcode == Opcode::And || bin->opcode == Opcode::Or) && bin->operands[0] == bin->operands[1]) {
            bin->replaceAllUsesWith(bin->operands[0]);
            toDelete.insert(bin);
            std::cout << "[instcombine] (X op X) -> X: " << bin->toString() << std::endl;
            return true;
        }
        // (X ^ 0) -> X
        if (bin->opcode == Opcode::Xor && isConstant(bin->operands[1]) && getConstantInt(bin->operands[1]) == 0) {
            bin->replaceAllUsesWith(bin->operands[0]);
            toDelete.insert(bin);
            std::cout << "[instcombine] (X ^ 0) -> X: " << bin->toString() << std::endl;
            return true;
        }
        
        // =============== 恒等运算优化 ===============
        // %rx = %ry + 0 -> %rx = %ry
        if (bin->opcode == Opcode::Add && isConstant(bin->operands[1])) {
            if (auto constVal = dynamic_cast<ConstantInt*>(bin->operands[1])) {
                if (constVal->value == 0) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X + 0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            } else if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 0.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X + 0.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // %rx = %ry - 0 -> %rx = %ry
        if (bin->opcode == Opcode::Sub && isConstant(bin->operands[1])) {
            if (auto constVal = dynamic_cast<ConstantInt*>(bin->operands[1])) {
                if (constVal->value == 0) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X - 0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            } else if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 0.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X - 0.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // %rx = %ry * 1 -> %rx = %ry
        if (bin->opcode == Opcode::Mul && isConstant(bin->operands[1])) {
            if (auto constVal = dynamic_cast<ConstantInt*>(bin->operands[1])) {
                if (constVal->value == 1) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X * 1 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            } else if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 1.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X * 1.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // %rx = %ry / 1 -> %rx = %ry
        if ((bin->opcode == Opcode::SDiv || bin->opcode == Opcode::UDiv) && isConstant(bin->operands[1])) {
            if (auto constVal = dynamic_cast<ConstantInt*>(bin->operands[1])) {
                if (constVal->value == 1) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X / 1 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // %rx = %ry / 1.0 -> %rx = %ry (浮点除法)
        if (bin->opcode == Opcode::FDiv && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 1.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] X / 1.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // =============== 浮点运算优化 ===============
        // 浮点乘法优化: fmul X, 0.0 -> 0.0
        if (bin->opcode == Opcode::FMul && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 0.0f) {
                    bin->replaceAllUsesWith(new ConstantFloat(0.0f));
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fmul X, 0.0 -> 0.0: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点乘法优化: fmul X, 1.0 -> X
        if (bin->opcode == Opcode::FMul && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 1.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fmul X, 1.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点乘法优化: fmul X, -1.0 -> fsub 0.0, X
        if (bin->opcode == Opcode::FMul && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == -1.0f) {
                    auto newFSub = std::make_unique<BinaryOperator>(Opcode::FSub, new ConstantFloat(0.0f), bin->operands[0], getInstCombineTempName());
                    BinaryOperator* newFSubPtr = newFSub.get();
                    newFSubPtr->parent = bb;
                    bb->instructions.insert(it, std::move(newFSub));
                    bin->replaceAllUsesWith(newFSubPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fmul X, -1.0 -> fsub 0.0, X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点乘法优化: fmul X, 2.0 -> fadd X, X
        if (bin->opcode == Opcode::FMul && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 2.0f) {
                    auto newFAdd = std::make_unique<BinaryOperator>(Opcode::FAdd, bin->operands[0], bin->operands[0], getInstCombineTempName());
                    BinaryOperator* newFAddPtr = newFAdd.get();
                    newFAddPtr->parent = bb;
                    bb->instructions.insert(it, std::move(newFAdd));
                    bin->replaceAllUsesWith(newFAddPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fmul X, 2.0 -> fadd X, X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点减法优化: fsub X, X -> 0.0
        if (bin->opcode == Opcode::FSub && bin->operands[0] == bin->operands[1]) {
            bin->replaceAllUsesWith(new ConstantFloat(0.0f));
            toDelete.insert(bin);
            std::cout << "[instcombine] fsub X, X -> 0.0: " << bin->toString() << std::endl;
            return true;
        }
        
        // 浮点加法优化: fadd X, 0.0 -> X (已存在，但确保完整性)
        if (bin->opcode == Opcode::FAdd && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 0.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fadd X, 0.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点减法优化: fsub X, 0.0 -> X (已存在，但确保完整性)
        if (bin->opcode == Opcode::FSub && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                if (constFloat->value == 0.0f) {
                    bin->replaceAllUsesWith(bin->operands[0]);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fsub X, 0.0 -> X: " << bin->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // =============== 浮点基础优化 ===============
        // 浮点常量右置 (FAdd, FMul 是可交换的)
        if ((bin->opcode == Opcode::FAdd || bin->opcode == Opcode::FMul) && isConstant(bin->operands[0]) && !isConstant(bin->operands[1])) {
            std::swap(bin->operands[0], bin->operands[1]);
            std::cout << "[instcombine] 浮点常量右置: " << bin->toString() << std::endl;
            return true;
        }
        
        // 浮点减法转加法: %r1 = fsub float %r0, 4.0 -> %r1 = fadd float %r0, -4.0
        if (bin->opcode == Opcode::FSub && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                float c = constFloat->value;
                if (c != 0.0f) {
                    auto newFAdd = std::make_unique<BinaryOperator>(Opcode::FAdd, bin->operands[0], new ConstantFloat(-c), bin->name);
                    BinaryOperator* newFAddPtr = newFAdd.get();
                    newFAddPtr->parent = bb;
                    bb->instructions.insert(it, std::move(newFAdd));
                    bin->replaceAllUsesWith(newFAddPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] fsub X, const -> fadd X, -const: " << newFAddPtr->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // =============== 浮点算术表达式化简规则 ===============
        // 浮点规则1: %r = {a - (a - b)} -> %r = {(a - a) + b} -> %r = {b + 0} -> %r = {b}
        if (bin->opcode == Opcode::FSub) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::FSub && bin->operands[0] == subOp->operands[0]) {
                    // a - (a - b) -> b
                    std::cout << "[instcombine] a - (a - b) -> b [float]: " << bin->toString() << " -> " << subOp->operands[1]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[1]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }
        
        // 浮点规则2: %r = {(a - b) + b} -> %r = {a - (b - b)} -> %r = {a + 0} -> %r = {a}
        if (bin->opcode == Opcode::FAdd) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                if (subOp->opcode == Opcode::FSub && subOp->operands[1] == bin->operands[1]) {
                    // (a - b) + b -> a
                    std::cout << "[instcombine] (a - b) + b -> a [float]: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::FSub && subOp->operands[1] == bin->operands[0]) {
                    // b + (a - b) -> a
                    std::cout << "[instcombine] b + (a - b) -> a [float]: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }
        
        // 浮点规则3: %r = {a - (a + b)} -> %r = {(a - a) - b} -> %r = {0 - b} -> %r = {-b}
        if (bin->opcode == Opcode::FSub) {
            if (auto addOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (addOp->opcode == Opcode::FAdd && bin->operands[0] == addOp->operands[0]) {
                    // a - (a + b) -> -b
                    auto negB = std::make_unique<BinaryOperator>(Opcode::FSub, new ConstantFloat(0.0f), addOp->operands[1], getInstCombineTempName());
                    BinaryOperator* negBPtr = negB.get();
                    negB->parent = bb;
                    bb->instructions.insert(it, std::move(negB));
                    bin->replaceAllUsesWith(negBPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] a - (a + b) -> -b [float]: " << bin->toString() << " -> " << negBPtr->toString() << std::endl;
                    return true;
                }
                if (addOp->opcode == Opcode::FAdd && bin->operands[0] == addOp->operands[1]) {
                    // a - (b + a) -> -b
                    auto negB = std::make_unique<BinaryOperator>(Opcode::FSub, new ConstantFloat(0.0f), addOp->operands[0], getInstCombineTempName());
                    BinaryOperator* negBPtr = negB.get();
                    negB->parent = bb;
                    bb->instructions.insert(it, std::move(negB));
                    bin->replaceAllUsesWith(negBPtr);
                    toDelete.insert(bin);
                    std::cout << "[instcombine] a - (b + a) -> -b [float]: " << bin->toString() << " -> " << negBPtr->toString() << std::endl;
                    return true;
                }
            }
        }
        
        // 浮点规则4: %r = {a + (b - a)} -> %r = {(a - a) + b} -> %r = {b + 0} -> %r = {b}
        if (bin->opcode == Opcode::FAdd) {
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::FSub && subOp->operands[1] == bin->operands[0]) {
                    // a + (b - a) -> b
                    std::cout << "[instcombine] a + (b - a) -> b [float]: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
            if (auto subOp = dynamic_cast<BinaryOperator*>(bin->operands[1])) {
                if (subOp->opcode == Opcode::FSub && subOp->operands[1] == bin->operands[0]) {
                    // (b - a) + a -> b
                    std::cout << "[instcombine] (b - a) + a -> b [float]: " << bin->toString() << " -> " << subOp->operands[0]->toString() << std::endl;
                    bin->replaceAllUsesWith(subOp->operands[0]);
                    toDelete.insert(bin);
                    return true;
                }
            }
        }
        
        // =============== 浮点除法常量合并 ===============
        // 浮点除法常量合并: a / c1 / c2 -> a / (c1*c2)
        if (bin->opcode == Opcode::FDiv && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                float c2 = constFloat->value;
                if (c2 != 0.0f) {  // 避免除零
                    if (auto divOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                        if (divOp->opcode == Opcode::FDiv && isConstant(divOp->operands[1])) {
                            if (auto constFloat2 = dynamic_cast<ConstantFloat*>(divOp->operands[1])) {
                                float c1 = constFloat2->value;
                                if (c1 != 0.0f) {  // 避免除零
                                    float combinedConst = c1 * c2;
                                    auto newFDiv = std::make_unique<BinaryOperator>(Opcode::FDiv, divOp->operands[0], new ConstantFloat(combinedConst), getInstCombineTempName());
                                    BinaryOperator* newFDivPtr = newFDiv.get();
                                    newFDivPtr->parent = bb;
                                    bb->instructions.insert(it, std::move(newFDiv));
                                    bin->replaceAllUsesWith(newFDivPtr);
                                    toDelete.insert(bin);
                                    std::cout << "[instcombine] a / c1 / c2 -> a / (c1*c2) [float]: " << bin->toString() << " -> " << newFDivPtr->toString() << std::endl;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // =============== 乘法与除法结合优化 ===============
        // 整数乘法与除法结合: (X * C1) / C2 -> X * (C1 / C2)
        if ((bin->opcode == Opcode::SDiv || bin->opcode == Opcode::UDiv) && isConstant(bin->operands[1])) {
            if (auto constDiv = dynamic_cast<ConstantInt*>(bin->operands[1])) {
                int32_t c2 = constDiv->value;
                if (c2 != 0) {  // 避免除零
                    if (auto mulOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                        if (mulOp->opcode == Opcode::Mul && isConstant(mulOp->operands[1])) {
                            if (auto constMul = dynamic_cast<ConstantInt*>(mulOp->operands[1])) {
                                int32_t c1 = constMul->value;
                                
                                // 检查是否可以整除
                                if (c1 % c2 == 0) {
                                    int32_t combinedConst = c1 / c2;
                                    auto newMul = std::make_unique<BinaryOperator>(Opcode::Mul, mulOp->operands[0], new ConstantInt(combinedConst), getInstCombineTempName());
                                    BinaryOperator* newMulPtr = newMul.get();
                                    newMulPtr->parent = bb;
                                    bb->instructions.insert(it, std::move(newMul));
                                    bin->replaceAllUsesWith(newMulPtr);
                                    toDelete.insert(bin);
                                    std::cout << "[instcombine] (X * " << c1 << ") / " << c2 << " -> X * " << combinedConst << " [int]: " << bin->toString() << " -> " << newMulPtr->toString() << std::endl;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 浮点乘法与除法结合: (X * C1) / C2 -> X * (C1 / C2)
        if (bin->opcode == Opcode::FDiv && isConstant(bin->operands[1])) {
            if (auto constFloat = dynamic_cast<ConstantFloat*>(bin->operands[1])) {
                float c2 = constFloat->value;
                if (c2 != 0.0f) {  // 避免除零
                    if (auto mulOp = dynamic_cast<BinaryOperator*>(bin->operands[0])) {
                        if (mulOp->opcode == Opcode::FMul && isConstant(mulOp->operands[1])) {
                            if (auto constMul = dynamic_cast<ConstantFloat*>(mulOp->operands[1])) {
                                float c1 = constMul->value;
                                float combinedConst = c1 / c2;
                                
                                // 创建新的浮点乘法指令
                                auto newFMul = std::make_unique<BinaryOperator>(Opcode::FMul, mulOp->operands[0], new ConstantFloat(combinedConst), getInstCombineTempName());
                                BinaryOperator* newFMulPtr = newFMul.get();
                                newFMulPtr->parent = bb;
                                bb->instructions.insert(it, std::move(newFMul));
                                bin->replaceAllUsesWith(newFMulPtr);
                                toDelete.insert(bin);
                                std::cout << "[instcombine] (X * " << c1 << ") / " << c2 << " -> X * " << combinedConst << " [float]: " << bin->toString() << " -> " << newFMulPtr->toString() << std::endl;
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        // sub X, X -> 0
        if (bin->opcode == Opcode::Sub && bin->operands[0] == bin->operands[1]) {
            if (bin->type == Type::Float) {
                bin->replaceAllUsesWith(new ConstantFloat(0.0f));
            } else {
                bin->replaceAllUsesWith(new ConstantInt(0, bin->type));
            }
            toDelete.insert(bin);
            std::cout << "[instcombine] X - X -> 0: " << bin->toString() << std::endl;
            return true;
        }
    }
    // 3. 比较指令转换
    if (auto icmp = dynamic_cast<ICmpInst*>(inst)) {
        // icmp slt i32 %X, 1  && %X is i1 -> icmp eq i32 %X, 0
        if (icmp->condition == ICmpCond::SLT && isConstant(icmp->operands[1]) && getConstantInt(icmp->operands[1]) == 1 && isBoolType(icmp->operands[0])) {
            auto newCmp = std::make_unique<ICmpInst>(ICmpCond::EQ, icmp->operands[0], new ConstantInt(0), getInstCombineTempName());
            ICmpInst* newCmpPtr = newCmp.get();
            newCmpPtr->parent = bb;
            bb->instructions.insert(it, std::move(newCmp));
            icmp->replaceAllUsesWith(newCmpPtr);
            toDelete.insert(icmp);
            std::cout << "[instcombine] 3. icmp slt i1, 1 -> icmp eq i1, 0: " << icmp->toString() << std::endl;
            return true;
        }
        // 4. 布尔比较消除
        if (isBoolType(icmp->operands[0]) && isConstant(icmp->operands[1])) {
            int32_t c = getConstantInt(icmp->operands[1]);
            if (icmp->condition == ICmpCond::EQ) {
                if (c == 1) {
                    icmp->replaceAllUsesWith(icmp->operands[0]);
                    toDelete.insert(icmp);
                    std::cout << "[instcombine] 4. icmp eq i1 %A, true -> %A: " << icmp->toString() << std::endl;
                    return true;
                } else if (c == 0) {
                    auto newXor = std::make_unique<BinaryOperator>(Opcode::Xor, icmp->operands[0], new ConstantInt(1, Type::I1), getInstCombineTempName());
                    BinaryOperator* newXorPtr = newXor.get();
                    newXorPtr->parent = bb;
                    bb->instructions.insert(it, std::move(newXor));
                    icmp->replaceAllUsesWith(newXorPtr);
                    toDelete.insert(icmp);
                    std::cout << "[instcombine] 4. icmp eq i1 %A, false -> xor i1 %A, true: " << icmp->toString() << std::endl;
                    return true;
                }
            }
            // icmp ne i1 %A, true/false 也可类似处理
            if (icmp->condition == ICmpCond::NE) {
                if (c == 0) {
                    icmp->replaceAllUsesWith(icmp->operands[0]);
                    toDelete.insert(icmp);
                    std::cout << "[instcombine] 4. icmp ne i1 %A, false -> %A: " << icmp->toString() << std::endl;
                    return true;
                } else if (c == 1) {
                    auto newXor = std::make_unique<BinaryOperator>(Opcode::Xor, icmp->operands[0], new ConstantInt(1, Type::I1), getInstCombineTempName());
                    BinaryOperator* newXorPtr = newXor.get();
                    newXorPtr->parent = bb;
                    bb->instructions.insert(it, std::move(newXor));
                    icmp->replaceAllUsesWith(newXorPtr);
                    toDelete.insert(icmp);
                    std::cout << "[instcombine] 4. icmp ne i1 %A, true -> xor i1 %A, true: " << icmp->toString() << std::endl;
                    return true;
                }
            }
        }
        // icmp eq X, X -> true
        if (icmp->condition == ICmpCond::EQ && icmp->operands[0] == icmp->operands[1]) {
            std::cout << "[instcombine] 6. icmp eq X, X -> true: " << icmp->toString() << " => 1" << std::endl;
            icmp->replaceAllUsesWith(new ConstantInt(1, Type::I1));
            toDelete.insert(icmp);
            return true;
        }
        // icmp ne X, X -> false
        if (icmp->condition == ICmpCond::NE && icmp->operands[0] == icmp->operands[1]) {
            std::cout << "[instcombine] 6. icmp ne X, X -> false: " << icmp->toString() << " => 0" << std::endl;
            icmp->replaceAllUsesWith(new ConstantInt(0, Type::I1));
            toDelete.insert(icmp);
            return true;
        }  
        // 模2运算优化: x % 2 == 0/1 -> (x & 1/-2147483647) == 0/1
        if ((icmp->condition == ICmpCond::EQ || icmp->condition == ICmpCond::NE) && isConstant(icmp->operands[1])) {
            if (auto binOp = dynamic_cast<BinaryOperator*>(icmp->operands[0])) {
                if (binOp->opcode == Opcode::SRem && isConstant(binOp->operands[1])) {
                    int32_t modulus = getConstantInt(binOp->operands[1]);
                    int32_t compareVal = getConstantInt(icmp->operands[1]);
                    
                    // 只处理模2的情况
                    if (modulus == 2 && (compareVal == 0 || compareVal == 1)) {
                        // 选择合适的掩码：对于 == 0 用 1，对于 == 1 用 INT32_MIN + 1
                        int32_t mask = (compareVal == 0) ? 1 : -2147483647;
                        
                        // 创建新的 and 指令
                        auto newAnd = std::make_unique<BinaryOperator>(Opcode::And, binOp->operands[0], new ConstantInt(mask), getInstCombineTempName());
                        BinaryOperator* newAndPtr = newAnd.get();
                        newAndPtr->parent = bb;
                        bb->instructions.insert(it, std::move(newAnd));
                        
                        // 用新的and指令替换原来的mod指令
                        binOp->replaceAllUsesWith(newAndPtr);
                        toDelete.insert(binOp);
                        
                        std::cout << "[instcombine][mod2_optimize] x % 2 == " << compareVal << " -> (x & " << mask << ") == " << compareVal << ": " << icmp->toString() << std::endl;
                        return true;
                    }
                }
            }
        }
        // =============== 除法比较优化 ===============
        // 检查是否为除法比较: if(%r / c1 > c2) => if(%r > (c2+1)*c1-1)
        if (auto divOp = dynamic_cast<BinaryOperator*>(icmp->operands[0])) {
            if (divOp->opcode == Opcode::SDiv && isConstant(divOp->operands[1]) && isConstant(icmp->operands[1])) {
                int32_t c1 = getConstantInt(divOp->operands[1]);
                int32_t c2 = getConstantInt(icmp->operands[1]);
                
                // 只处理正数情况
                if (c1 > 0 && c2 >= 0) {
                    bool changed = false;
                    ICmpCond newCond = icmp->condition;
                    int32_t newConst = 0;
                    
                    switch (icmp->condition) {
                        case ICmpCond::SGT: // if(%r / c1 > c2) => if(%r > (c2+1)*c1-1)
                            if (c2 < INT32_MAX) {
                                int32_t temp = c2 + 1;
                                if (temp <= INT32_MAX / c1) {
                                    newConst = temp * c1 - 1;
                                    newCond = ICmpCond::SGT;
                                    changed = true;
                                }
                            }
                            break;
                            
                        case ICmpCond::SGE: // if(%r / c1 >= c2) => if(%r >= c2*c1)
                            if (c2 <= INT32_MAX / c1) {
                                newConst = c2 * c1;
                                newCond = ICmpCond::SGE;
                                changed = true;
                            }
                            break;
                            
                        case ICmpCond::SLT: // if(%r / c1 < c2) => if(%r < c2*c1)
                            if (c2 <= INT32_MAX / c1) {
                                newConst = c2 * c1;
                                newCond = ICmpCond::SLT;
                                changed = true;
                            }
                            break;
                            
                        case ICmpCond::SLE: // if(%r / c1 <= c2) => if(%r <= (c2+1)*c1-1)
                            if (c2 < INT32_MAX) {
                                int32_t temp = c2 + 1;
                                if (temp <= INT32_MAX / c1) {
                                    newConst = temp * c1 - 1;
                                    newCond = ICmpCond::SLE;
                                    changed = true;
                                }
                            }
                            break;
                            
                        default:
                            break;
                    }
                    
                    if (changed) {
                        auto newCmp = std::make_unique<ICmpInst>(newCond, divOp->operands[0], new ConstantInt(newConst), getInstCombineTempName());
                        ICmpInst* newCmpPtr = newCmp.get();
                        newCmpPtr->parent = bb;
                        bb->instructions.insert(it, std::move(newCmp));
                        icmp->replaceAllUsesWith(newCmpPtr);
                        toDelete.insert(icmp);
                        std::cout << "[instcombine] 除法比较优化: " << icmp->toString() << " -> " << newCmpPtr->toString() << std::endl;
                        return true;
                    }
                }
            }
        }
        
        // =============== 比较指令操作数简化 ===============
        // 模式1: icmp slt (X + C1), C2 -> icmp slt X, (C2 - C1)
        // 模式2: icmp slt (X - C1), C2 -> icmp slt X, (C2 + C1)
        // 支持所有比较条件：SLT, SLE, SGT, SGE, ULT, ULE, UGT, UGE
        if (isConstant(icmp->operands[1])) {
            if (auto binOp = dynamic_cast<BinaryOperator*>(icmp->operands[0])) {
                if ((binOp->opcode == Opcode::Add || binOp->opcode == Opcode::Sub) && isConstant(binOp->operands[1])) {
                    int32_t c1 = getConstantInt(binOp->operands[1]);
                    int32_t c2 = getConstantInt(icmp->operands[1]);
                    
                    // 检查运算是否会溢出
                    bool canSimplify = false;
                    int32_t newConst = 0;
                    
                    if (binOp->opcode == Opcode::Add) {
                        // 加法模式: (X + C1) op C2 -> X op (C2 - C1)
                        // 对于有符号比较，需要考虑溢出
                        if (icmp->condition == ICmpCond::SLT || icmp->condition == ICmpCond::SLE || 
                            icmp->condition == ICmpCond::SGT || icmp->condition == ICmpCond::SGE) {
                            // 检查 c2 - c1 是否会溢出
                            if (c1 > 0) {
                                if (c2 > INT32_MIN + c1) {
                                    newConst = c2 - c1;
                                    canSimplify = true;
                                }
                            } else if (c1 < 0) {
                                if (c2 < INT32_MAX + c1) {
                                    newConst = c2 - c1;
                                    canSimplify = true;
                                }
                            } else {
                                // c1 == 0，直接简化
                                newConst = c2;
                                canSimplify = true;
                            }
                        } else {
                            // 无符号比较，直接计算
                            newConst = c2 - c1;
                            canSimplify = true;
                        }
                    } else if (binOp->opcode == Opcode::Sub) {
                        // 减法模式: (X - C1) op C2 -> X op (C2 + C1)
                        // 对于有符号比较，需要考虑溢出
                        if (icmp->condition == ICmpCond::SLT || icmp->condition == ICmpCond::SLE || 
                            icmp->condition == ICmpCond::SGT || icmp->condition == ICmpCond::SGE) {
                            // 检查 c2 + c1 是否会溢出
                            if (c1 > 0) {
                                if (c2 <= INT32_MAX - c1) {
                                    newConst = c2 + c1;
                                    canSimplify = true;
                                }
                            } else if (c1 < 0) {
                                if (c2 >= INT32_MIN - c1) {
                                    newConst = c2 + c1;
                                    canSimplify = true;
                                }
                            } else {
                                // c1 == 0，直接简化
                                newConst = c2;
                                canSimplify = true;
                            }
                        } else {
                            // 无符号比较，直接计算
                            newConst = c2 + c1;
                            canSimplify = true;
                        }
                    }
                    
                    if (canSimplify) {
                        auto newCmp = std::make_unique<ICmpInst>(icmp->condition, binOp->operands[0], new ConstantInt(newConst), getInstCombineTempName());
                        ICmpInst* newCmpPtr = newCmp.get();
                        newCmpPtr->parent = bb;
                        bb->instructions.insert(it, std::move(newCmp));
                        icmp->replaceAllUsesWith(newCmpPtr);
                        toDelete.insert(icmp);
                        std::cout << "[instcombine] 比较指令操作数简化: " << icmp->toString() << " -> " << newCmpPtr->toString() << std::endl;
                        return true;
                    }
                }
            }
        }
    }
    
    // =============== 浮点比较指令优化 ===============
    if (auto fcmp = dynamic_cast<FCmpInst*>(inst)) {
        // 浮点比较优化: fcmp eq X, X -> true
        if (fcmp->condition == FCmpCond::OEQ && fcmp->operands[0] == fcmp->operands[1]) {
            std::cout << "[instcombine] fcmp eq X, X -> true: " << fcmp->toString() << " => 1" << std::endl;
            fcmp->replaceAllUsesWith(new ConstantInt(1, Type::I1));
            toDelete.insert(fcmp);
            return true;
        }
        
        // 浮点比较优化: fcmp ne X, X -> false
        if (fcmp->condition == FCmpCond::ONE && fcmp->operands[0] == fcmp->operands[1]) {
            std::cout << "[instcombine] fcmp ne X, X -> false: " << fcmp->toString() << " => 0" << std::endl;
            fcmp->replaceAllUsesWith(new ConstantInt(0, Type::I1));
            toDelete.insert(fcmp);
            return true;
        }
        
        // 浮点比较操作数简化: fcmp slt (X + C1), C2 -> fcmp slt X, (C2 - C1)
        if (isConstant(fcmp->operands[1])) {
            if (auto binOp = dynamic_cast<BinaryOperator*>(fcmp->operands[0])) {
                if ((binOp->opcode == Opcode::FAdd || binOp->opcode == Opcode::FSub) && isConstant(binOp->operands[1])) {
                    float c1 = getConstantFloat(binOp->operands[1]);
                    float c2 = getConstantFloat(fcmp->operands[1]);
                    
                    bool canSimplify = false;
                    float newConst = 0.0f;
                    
                    if (binOp->opcode == Opcode::FAdd) {
                        // 浮点加法模式: (X + C1) op C2 -> X op (C2 - C1)
                        newConst = c2 - c1;
                        canSimplify = true;
                    } else if (binOp->opcode == Opcode::FSub) {
                        // 浮点减法模式: (X - C1) op C2 -> X op (C2 + C1)
                        newConst = c2 + c1;
                        canSimplify = true;
                    }
                    
                    if (canSimplify) {
                        auto newFCmp = std::make_unique<FCmpInst>(fcmp->condition, binOp->operands[0], new ConstantFloat(newConst), getInstCombineTempName());
                        FCmpInst* newFCmpPtr = newFCmp.get();
                        newFCmpPtr->parent = bb;
                        bb->instructions.insert(it, std::move(newFCmp));
                        fcmp->replaceAllUsesWith(newFCmpPtr);
                        toDelete.insert(fcmp);
                        std::cout << "[instcombine] 浮点比较指令操作数简化: " << fcmp->toString() << " -> " << newFCmpPtr->toString() << std::endl;
                        return true;
                    }
                }
            }
        }
    }
    
    // 5. 类型转换合并
    if (auto zext = dynamic_cast<ZExtInst*>(inst)) {
        if (zext->operands[0]->type == zext->type) {
            std::cout << "[instcombine] 5. zext(X) src==dst类型: " << zext->toString() << " => " << zext->operands[0]->toString() << std::endl;
            zext->replaceAllUsesWith(zext->operands[0]);
            toDelete.insert(zext);
            return true;
        }
        // ZExt(Trunc(X))，只有类型完全一致时才可消除
        if (auto inner = dynamic_cast<TruncInst*>(zext->operands[0])) {
            if (zext->type == inner->operands[0]->type) {
                std::cout << "[instcombine] 5. zext(trunc(X)) -> X: " << zext->toString() << " => " << inner->operands[0]->toString() << std::endl;
                zext->replaceAllUsesWith(inner->operands[0]);
                toDelete.insert(zext);
                toDelete.insert(inner);
                return true;
            }
        }
        // 修复 zext(zext(X)) 合并逻辑
        if (auto inner = dynamic_cast<ZExtInst*>(zext->operands[0])) {
            // 只有当类型不同才需要合并
            if (inner->type != zext->type) {
                auto newZext = std::make_unique<ZExtInst>(inner->operands[0], zext->type, getInstCombineTempName());
                ZExtInst* newZextPtr = newZext.get();
                newZextPtr->parent = bb;
                bb->instructions.insert(it, std::move(newZext));
                zext->replaceAllUsesWith(newZextPtr);
                toDelete.insert(zext);
                std::cout << "[instcombine] 5. zext(zext(X)) -> zext(X) (merged): " << zext->toString() << std::endl;
                return true;
            }
        }
    }
    if (auto sext = dynamic_cast<SExtInst*>(inst)) {
        if (sext->operands[0]->type == sext->type) {
            std::cout << "[instcombine] 5. sext(X) src==dst类型: " << sext->toString() << " => " << sext->operands[0]->toString() << std::endl;
            sext->replaceAllUsesWith(sext->operands[0]);
            toDelete.insert(sext);
            return true;
        }
    }
    if (auto trunc = dynamic_cast<TruncInst*>(inst)) {
        if (trunc->operands[0]->type == trunc->type) {
            std::cout << "[instcombine] 5. trunc(X) src==dst类型: " << trunc->toString() << " => " << trunc->operands[0]->toString() << std::endl;
            trunc->replaceAllUsesWith(trunc->operands[0]);
            toDelete.insert(trunc);
            return true;
        }
        // Trunc(ZExt(X))，只有类型完全一致时才可消除
        if (auto inner = dynamic_cast<ZExtInst*>(trunc->operands[0])) {
            if (trunc->type == inner->operands[0]->type) {
                std::cout << "[instcombine] 5. trunc(zext(X)) -> X: " << trunc->toString() << " => " << inner->operands[0]->toString() << std::endl;
                trunc->replaceAllUsesWith(inner->operands[0]);
                toDelete.insert(trunc);
                toDelete.insert(inner);
                return true;
            }
        }
    }
    if (auto bitcast = dynamic_cast<BitCastInst*>(inst)) {
        if (bitcast->operands[0]->type == bitcast->type) {
            std::cout << "[instcombine] 5. bitcast(X) src==dst类型: " << bitcast->toString() << " => " << bitcast->operands[0]->toString() << std::endl;
            bitcast->replaceAllUsesWith(bitcast->operands[0]);
            toDelete.insert(bitcast);
            return true;
        }
    }
    // 7. select 恒等化简
    if (auto sel = dynamic_cast<SelectInst*>(inst)) {
        // select true, X, Y -> X
        if (isConstant(sel->operands[0]) && getConstantInt(sel->operands[0]) == 1) {
            std::cout << "[instcombine] 7. select true, X, Y -> X: " << sel->toString() << " => " << sel->operands[1]->toString() << std::endl;
            sel->replaceAllUsesWith(sel->operands[1]);
            toDelete.insert(sel);
            return true;
        }
        // select false, X, Y -> Y
        if (isConstant(sel->operands[0]) && getConstantInt(sel->operands[0]) == 0) {
            std::cout << "[instcombine] 7. select false, X, Y -> Y: " << sel->toString() << " => " << sel->operands[2]->toString() << std::endl;
            sel->replaceAllUsesWith(sel->operands[2]);
            toDelete.insert(sel);
            return true;
        }
        // select C, X, X -> X
        if (sel->operands[1] == sel->operands[2]) {
            std::cout << "[instcombine] 7. select C, X, X -> X: " << sel->toString() << " => " << sel->operands[1]->toString() << std::endl;
            sel->replaceAllUsesWith(sel->operands[1]);
            toDelete.insert(sel);
            return true;
        }
    }
    // 8. PHI节点同值合并
    if (auto phi = dynamic_cast<PhiInst*>(inst)) {
        if (!phi->incoming_values.empty()) {
            Value* first = phi->incoming_values[0].first;
            bool allSame = true;
            for (auto& [val, _] : phi->incoming_values) {
                if (val != first) { allSame = false; break; }
            }
            if (allSame) {
                std::cout << "[instcombine] 8. PHI节点同值合并: " << phi->toString() << " => " << first->toString() << std::endl;
                phi->replaceAllUsesWith(first);
                toDelete.insert(phi);
                return true;
            }
        }
    }
    return false;
}

// =============== 全局函数相关优化 ===============
// 未写全局变量标记为常量
// 分析所有函数：若某全局变量从未作为 store 目标（或其 GEP 结果、位转换后的结果作为 store 目标），
// 且未以指针形式作为实参传入调用（保守认为可能写），则将其标记为 constant。
static bool markNoWriteGlobalsAsConst(Module& M) {
    std::set<GlobalVariable*> maybeWritten;
    std::unordered_map<Value*, GlobalVariable*> gepBaseGlobal; // GEP结果 -> 对应的全局变量

    auto record_geps_in_function = [&](Function& F) {
        for (auto& bb : F.basicBlocks) {
            for (auto& uptr : bb->instructions) {
                if (auto gep = dynamic_cast<GetElementPtrInst*>(uptr.get())) {
                    // 只处理直接以全局变量为基指针的 GEP（或名称以'@'开头的值）
                    if (auto g = dynamic_cast<GlobalVariable*>(gep->operands[0])) {
                        gepBaseGlobal[gep] = g;
                    }
                }
            }
        }
    };

    auto findGlobalByName = [&](const std::string& n) -> GlobalVariable* {
        std::string name = n;
        if (!name.empty() && name[0] == '@') name = name.substr(1);
        for (auto& gvarUptr : M.globalVariables) {
            if (gvarUptr->name == name) return gvarUptr.get();
        }
        return nullptr;
    };

    auto resolve_base_global = [&](Value* v) -> GlobalVariable* {
        if (!v) return nullptr;
        if (auto g = dynamic_cast<GlobalVariable*>(v)) return g;
        if (!v->name.empty() && v->name[0] == '@') {
            if (auto g = findGlobalByName(v->name)) return g;
        }
        // 直接来自 GEP 的结果
        if (auto it = gepBaseGlobal.find(v); it != gepBaseGlobal.end()) return it->second;
        if (auto inst = dynamic_cast<Instruction*>(v)) {
            if (auto gep = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (auto g = dynamic_cast<GlobalVariable*>(gep->operands[0])) return g;
                if (gep->operands[0] && !gep->operands[0]->name.empty() && gep->operands[0]->name[0] == '@') {
                    if (auto g = findGlobalByName(gep->operands[0]->name)) return g;
                }
            }
            if (inst->opcode == Opcode::BitCast || inst->opcode == Opcode::Move) {
                if (!inst->operands.empty()) {
                    if (auto g = dynamic_cast<GlobalVariable*>(inst->operands[0])) return g;
                    if (inst->operands[0] && !inst->operands[0]->name.empty() && inst->operands[0]->name[0] == '@') {
                        if (auto g = findGlobalByName(inst->operands[0]->name)) return g;
                    }
                }
            }
        }
        return nullptr;
    };

    // 第一次：收集所有 GEP 的 base global
    for (auto& funcPtr : M.functions) {
        if (!funcPtr->isDeclaration()) {
            record_geps_in_function(*funcPtr);
        }
    }

    // 第二次：扫描所有 store / call，保守标记可能被写的全局
    for (auto& funcPtr : M.functions) {
        if (funcPtr->isDeclaration()) continue;
        if (funcPtr->name == "global") continue;
        for (auto& bb : funcPtr->basicBlocks) {
            for (auto& uptr : bb->instructions) {
                Instruction* I = uptr.get();
                if (I->opcode == Opcode::Store) {
                    auto storeI = static_cast<StoreInst*>(I);
                    Value* ptr = storeI->getPointerOperand();
                    if (auto g = resolve_base_global(ptr)) {
                        maybeWritten.insert(g);
                    }
                } else if (I->opcode == Opcode::Call) {
                    auto callI = static_cast<CallInst*>(I);
                    for (auto* arg : callI->operands) {
                        // 去掉对指针类型的限制，直接解析到底层全局
                        if (auto g = resolve_base_global(arg)) {
                            maybeWritten.insert(g);
                        }
                    }
                }
            }
        }
    }

    bool changed = false;
    // 查找名为 "global" 的初始化函数（如果存在）
    Function* globalInitFunc = nullptr;
    for (auto& fptr : M.functions) {
        if (!fptr->isDeclaration() && fptr->name == "global") {
            globalInitFunc = fptr.get();
            break;
        }
    }

    for (auto& gvarUptr : M.globalVariables) {
        GlobalVariable* g = gvarUptr.get();
        // if (maybeWritten.find(g) == maybeWritten.end()) {
            // 仅处理标量（非数组）全局的初值提取
            if (g->arraySize == 0) {
                Value* foundInit = nullptr;
                if (globalInitFunc) {
                    for (auto& bb : globalInitFunc->basicBlocks) {
                        for (auto& uptr : bb->instructions) {
                            if (auto storeI = dynamic_cast<StoreInst*>(uptr.get())) {
                                Value* ptr = storeI->getPointerOperand();
                                if (resolve_base_global(ptr) == g) {
                                    Value* v = storeI->getValueOperand();
                                    if (auto ci = dynamic_cast<ConstantInt*>(v)) {
                                        // 统一为 i32 常量
                                        foundInit = new ConstantInt(ci->value, Type::I32);
                                    } else if (auto cf = dynamic_cast<ConstantFloat*>(v)) {
                                        foundInit = new ConstantFloat(cf->value, Type::Float);
                                    }
                                    // 不 break，保留最后一次 store 的值
                                }
                            }
                        }
                    }
                }
                // 如果未找到，设为按类型的 0
                if (!foundInit) {
                    if (g->type == Type::I32) foundInit = new ConstantInt(0, Type::I32);
                    else if (g->type == Type::Float) foundInit = new ConstantFloat(0.0f, Type::Float);
                }
                if (foundInit) g->initializer = foundInit;
            } else {
                // 数组全局：提取数组各元素初始化（仅处理全局函数里的常量 store）
                if (g->arraySize > 0) {
                    if (g->elementType == Type::I32) {
                        g->intInitVals.assign(static_cast<size_t>(g->arraySize), 0);
                    } else if (g->elementType == Type::Float) {
                        g->floatInitVals.assign(static_cast<size_t>(g->arraySize), 0.0f);
                    }
                    if (globalInitFunc) {
                        for (auto& bb : globalInitFunc->basicBlocks) {
                            for (auto& uptr : bb->instructions) {
                                if (auto storeI = dynamic_cast<StoreInst*>(uptr.get())) {
                                    Value* ptr = storeI->getPointerOperand();
                                    // 只接受 GEP 到该全局，且索引为常量
                                    if (auto gep = dynamic_cast<GetElementPtrInst*>(ptr)) {
                                        GlobalVariable* base = nullptr;
                                        if (auto gb = dynamic_cast<GlobalVariable*>(gep->operands[0])) base = gb;
                                        else if (!gep->operands[0]->name.empty() && gep->operands[0]->name[0] == '@') {
                                            // 按名查找
                                            std::string nm = gep->operands[0]->name.substr(1);
                                            for (auto& gvarUptr2 : M.globalVariables) {
                                                if (gvarUptr2->name == nm) { base = gvarUptr2.get(); break; }
                                            }
                                        }
                                        if (base != g) continue;
                                        if (!isConstant(gep->operands[1])) continue;
                                        int idx = getConstantInt(gep->operands[1]);
                                        if (idx < 0 || idx >= g->arraySize) continue;
                                        Value* v = storeI->getValueOperand();
                                        if (g->elementType == Type::I32) {
                                            if (auto ci = dynamic_cast<ConstantInt*>(v)) {
                                                g->intInitVals[static_cast<size_t>(idx)] = ci->value;
                                            } else {
                                                // 非常量，保留默认（0）
                                            }
                                        } else if (g->elementType == Type::Float) {
                                            if (auto cf = dynamic_cast<ConstantFloat*>(v)) {
                                                g->floatInitVals[static_cast<size_t>(idx)] = cf->value;
                                            } else {
                                                // 非常量，保留默认（0.0）
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        if (maybeWritten.find(g) == maybeWritten.end()) {
            if (!g->isConstant) {
                g->isConstant = true;
                changed = true;
                std::cout << "[instcombine][global] 标记未写全局为constant: @" << g->name << std::endl;
            }
        }
        // }
    }
    return changed;
}

// 移除未使用的全局变量
static bool eraseNoUseGlobals(Module& M) {
    std::set<GlobalVariable*> used;

    auto findGlobalByName = [&](const std::string& n) -> GlobalVariable* {
        std::string name = n;
        if (!name.empty() && name[0] == '@') name = name.substr(1);
        for (auto& gvarUptr : M.globalVariables) {
            if (gvarUptr->name == name) return gvarUptr.get();
        }
        return nullptr;
    };

    for (auto& funcPtr : M.functions) {
        if (funcPtr->isDeclaration()) continue;
        for (auto& bb : funcPtr->basicBlocks) {
            for (auto& uptr : bb->instructions) {
                Instruction* I = uptr.get();
                // 检查 GEP 的基全局是否被使用
                if (auto gep = dynamic_cast<GetElementPtrInst*>(I)) {
                    Value* base = gep->operands.size() > 0 ? gep->operands[0] : nullptr;
                    if (auto g = dynamic_cast<GlobalVariable*>(base)) {
                        used.insert(g);
                    } else if (base && !base->name.empty() && base->name[0] == '@') {
                        if (auto g = findGlobalByName(base->name)) used.insert(g);
                    }
                }
                // 检查所有操作数是否直接是全局或以@name形式出现
                for (auto* op : I->operands) {
                    if (auto g = dynamic_cast<GlobalVariable*>(op)) {
                        used.insert(g);
                    } else if (op && !op->name.empty() && op->name[0] == '@') {
                        if (auto g = findGlobalByName(op->name)) used.insert(g);
                    }
                }
            }
        }
    }

    bool changed = false;
    auto& globals = M.globalVariables;
    auto it = globals.begin();
    while (it != globals.end()) {
        GlobalVariable* g = it->get();
        if (used.find(g) == used.end()) {
            std::cout << "[instcombine][global] 移除未使用全局: @" << g->name << std::endl;
            it = globals.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    return changed;
}

// 常量数组GEP优化：将常量数组的GEP+Load组合优化为直接常量（当前仅支持对constant零初始化数组的消除）
static bool eliminateSimpleConstArrayValue(Function& F) {
    bool changed = false;
    std::unordered_map<Value*, Value*> gepConstElem; // GEP结果 -> 常量元素值

    // 第一遍：收集所有符合条件的 GEP
    for (auto& bb : F.basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (auto gep = dynamic_cast<GetElementPtrInst*>(inst.get())) {
                // 只考虑基指针为全局数组，且标记为 constant 的情况
                if (auto g = dynamic_cast<GlobalVariable*>(gep->operands[0])) {
                    if (g->arraySize > 0 && g->isConstant) {
                        // 要求索引为常量 i32
                        if (auto ci = dynamic_cast<ConstantInt*>(gep->operands[1])) {
                            // 由于当前未保存数组逐元素初始值，且全局常量数组默认 zeroinitializer，元素恒为 0
                            if (g->elementType == Type::I32) {
                                gepConstElem[gep] = new ConstantInt(0, Type::I32);
                            } else if (g->elementType == Type::Float) {
                                gepConstElem[gep] = new ConstantFloat(0.0f, Type::Float);
                            }
                        }
                    }
                }
            }
        }
    }

    // 第二遍：将对这些 GEP 指针的 load 用常量替换
    for (auto& bb : F.basicBlocks) {
        for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ) {
            Instruction* I = it->get();
            if (auto load = dynamic_cast<LoadInst*>(I)) {
                Value* ptr = load->getPointerOperand();
                auto itMap = gepConstElem.find(ptr);
                if (itMap != gepConstElem.end()) {
                    Value* c = itMap->second;
                    load->replaceAllUsesWith(c);
                    // 清理操作数上的 use 关系
                    for (auto* opnd : load->operands) {
                        if (opnd) opnd->removeUse(load);
                    }
                    it = bb->instructions.erase(it);
                    changed = true;
                    std::cout << "[instcombine] const array GEP+load -> const: replaced " << c->toString() << std::endl;
                    continue;
                }
            }
            ++it;
        }
    }

    return changed;
}

// 将对常量全局变量（标记 isConstant 且具有可用初始值或默认零）的 load 替换为常量
static bool replaceConstGlobalLoads(Function& F, Module& M) {
    bool changed = false;

    auto findGlobalByName = [&](const std::string& n) -> GlobalVariable* {
        std::string name = n;
        if (!name.empty() && name[0] == '@') name = name.substr(1);
        for (auto& gvarUptr : M.globalVariables) {
            if (gvarUptr->name == name) return gvarUptr.get();
        }
        return nullptr;
    };

    std::function<GlobalVariable*(Value*)> resolve_base_global = [&](Value* v) -> GlobalVariable* {
        if (!v) return nullptr;
        // if (!(v->name[0] == '@')) {
        //     std::cout << "v: " << v->toString() << std::endl;
        //     return nullptr;
        // }
        if (auto g = dynamic_cast<GlobalVariable*>(v)) {
            return g;
        }
        if (!v->name.empty() && v->name[0] == '@') {
            if (auto g = findGlobalByName(v->name)) return g;
        }
        if (auto inst = dynamic_cast<Instruction*>(v)) {
            if (auto gep = dynamic_cast<GetElementPtrInst*>(inst)) {
                if (auto g = dynamic_cast<GlobalVariable*>(gep->operands[0])) return g;
                if (gep->operands[0] && !gep->operands[0]->name.empty() && gep->operands[0]->name[0] == '@') {
                    if (auto g = findGlobalByName(gep->operands[0]->name)) return g;
                }
            }
            if (inst->opcode == Opcode::BitCast || inst->opcode == Opcode::Move) {
                if (!inst->operands.empty()) {
                    return resolve_base_global(inst->operands[0]);
                }
            }
        }
        return nullptr;
    };

    std::function<GetElementPtrInst*(Value*)> unwrap_to_gep = [&](Value* v) -> GetElementPtrInst* {
        if (!v) return nullptr;
        if (auto gep = dynamic_cast<GetElementPtrInst*>(v)) return gep;
        if (auto inst = dynamic_cast<Instruction*>(v)) {
            if ((inst->opcode == Opcode::BitCast || inst->opcode == Opcode::Move) && !inst->operands.empty()) {
                return unwrap_to_gep(inst->operands[0]);
            }
        }
        return nullptr;
    };

    for (auto& bb : F.basicBlocks) {

        for (auto it = bb->instructions.begin(); it != bb->instructions.end();) {
            Instruction* I = it->get();
            if (auto load = dynamic_cast<LoadInst*>(I)) {
                Value* ptr = load->getPointerOperand();

                // 解析底层全局
                GlobalVariable* base = resolve_base_global(ptr);
                // 如果不是常量全局，或无法解析到底层全局，则跳过
                if (!base || !base->isConstant) { ++it; continue; }

                // 如果是数组，尝试识别常量索引的GEP再替换
                if (base->arraySize > 0) {
                    if (auto gep = unwrap_to_gep(ptr)) {
                        if (isConstant(gep->operands[1])) {
                            int idx = getConstantInt(gep->operands[1]);
                            if (0 <= idx && idx < base->arraySize) {
                                Value* repl = nullptr;
                                if (base->elementType == Type::I32) {
                                    int32_t v = 0;
                                    if (base->intInitVals.size() == static_cast<size_t>(base->arraySize)) {
                                        v = base->intInitVals[static_cast<size_t>(idx)];
                                    }
                                    repl = new ConstantInt(v, Type::I32);
                                } else if (base->elementType == Type::Float) {
                                    float v = 0.0f;
                                    if (base->floatInitVals.size() == static_cast<size_t>(base->arraySize)) {
                                        v = base->floatInitVals[static_cast<size_t>(idx)];
                                    }
                                    repl = new ConstantFloat(v, Type::Float);
                                }
                                if (repl) {
                                    load->replaceAllUsesWith(repl);
                                    for (auto* opnd : load->operands) {
                                        if (opnd) opnd->removeUse(load);
                                    }
                                    it = bb->instructions.erase(it);
                                    changed = true;
                                    std::cout << "[instcombine][gloabl] const array gep+load -> const: @" << base->name << "[" << idx << "]" << std::endl;
                                    continue;
                                }
                            }
                        }
                    }
                    // 无法解析常量索引，不替换数组
                    ++it; continue;
                }

                // 标量常量全局：若有 initializer 直接替换；否则按类型置0
                Value* repl = nullptr;
                if (base->initializer) {
                    repl = base->initializer;
                } else {
                    if (load->type == Type::I32) repl = new ConstantInt(0, Type::I32);
                    else if (load->type == Type::Float) repl = new ConstantFloat(0.0f, Type::Float);
                }
                if (repl) {
                    load->replaceAllUsesWith(repl);
                    for (auto* opnd : load->operands) {
                        if (opnd) opnd->removeUse(load);
                    }
                    it = bb->instructions.erase(it);
                    changed = true;
                    std::cout << "[instcombine][global] const global load -> const: @" << base->name << std::endl;
                    continue;
                }
            }
            ++it;
        }
    }
    return changed;
}

// 消除索引为0的GEP
static bool EliminateEmptyIndexGEP(Function& F) {
    bool changed = false;
    std::set<Instruction*> toDelete;
    for (auto& bb : F.basicBlocks) {
        for (auto& inst : bb->instructions) {
            if (auto gep = dynamic_cast<GetElementPtrInst*>(inst.get())) {
                if (gep->operands.size() == 2 && isConstant(gep->operands[1])) {
                    int32_t index = getConstantInt(gep->operands[1]);
                    if (index == 0) {
                        gep->replaceAllUsesWith(gep->operands[0]);
                        toDelete.insert(gep);
                        changed = true;
                    }
                }
            }
        }
    }
    for (auto* inst : toDelete) {
        inst->removeFromParent();
    }
    toDelete.clear();
    return changed;
}

// 入口函数
// 对单个函数做 instcombine
static bool runOnFunction(Function& F, Module& M) {
    bool changed = false;
    IRBuilder builder;
    std::set<Instruction*> toDelete;

    cfg::DominatorTree DT;
    DT.runOnFunction(F, false);
    
    do {
        changed = false;
        for (auto& bb : F.basicBlocks) {
            builder.setInsertPoint(bb.get());
            for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ) {
                if (tryCombine(bb.get(), it, builder, toDelete)) {
                    changed |= true;
                }
                ++it;
            }
        }
        // 删除无用指令
        for (auto* inst : toDelete) {
            inst->removeFromParent();
        }
        toDelete.clear();

        changed |= domTreeAddCombine(F, DT); // 不用重复计算DT，因为这里不会更改CFG
        

    } while (changed);

    return changed;
}

// 对整个模块做 instcombine
bool runOnModule(Module& module) {
    bool hasChanged = false;
    for (auto& funcPtr : module.functions) {
        if (!funcPtr->isDeclaration()) {
            bool funcChanged = runOnFunction(*funcPtr, module);
            hasChanged = hasChanged || funcChanged;
        }
    }
    // 移除未使用的全局
    bool globalsErased = eraseNoUseGlobals(module);
    hasChanged = hasChanged || globalsErased;
    return hasChanged;
}

// 全局常量传播
bool runGlobalConstProp(Module& M) {
    std::cout << "[instcombine][global] 开始全局常量传播" << std::endl;
    // 先标记未写全局为常量，便于函数内进行替换
    bool globalsMarked = markNoWriteGlobalsAsConst(M);
    bool hasChanged = false;
    for (auto& funcPtr : M.functions) {
        if (!funcPtr->isDeclaration() && funcPtr->name != "global") {
            // 常量全局相关优化
            hasChanged |= replaceConstGlobalLoads(*funcPtr, M);
            hasChanged |= eliminateSimpleConstArrayValue(*funcPtr);
            // bool emptyIndexGEPChanged = EliminateEmptyIndexGEP(*funcPtr);
            hasChanged |= false; // TODO, 需要后端支持，测试点: 34 arr exper len
        }
    }

    return hasChanged;
}

} // namespace instcombine
} // namespace llvm_ir