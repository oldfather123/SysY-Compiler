#include "../../include/ir/loop_strength_reduce.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/scev.h"
#include "../../include/ir/cfg.h"
#include "../../debug.h"
#ifndef DEBUG_LSR
#define DEBUG_LSR 1
#endif
#include <set>
#include <functional>
#include <unordered_map>

namespace llvm_ir {
namespace lsr {

struct LSRStats { int reused = 0; int ptr = 0; int phi = 0; };
static LSRStats gStats;

// 帮助：判断一个Value是否是常量i32
static bool tryGetConstI32(Value* v, int32_t& out) {
    if (auto ci = dynamic_cast<ConstantInt*>(v)) { out = ci->value; return true; }
    return false;
}

// 在preheader末尾（terminator前）插入指令
static void insertBeforeTerminator(BasicBlock* bb, std::unique_ptr<Instruction> inst) {
    if (!bb || !inst) return;
    // 终止指令在最后一个；我们将新指令插在它前面
    auto term = bb->getTerminator();
    if (!term) { bb->addInstruction(std::move(inst)); return; }
    size_t pos = 0; size_t idx = 0;
    for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ++it, ++idx) {
        if (it->get() == term) { pos = idx; break; }
    }
    std::cout << "[LSR] insertBeforeTerminator: " << inst->toString() << std::endl;
    bb->insertInstructionAt(std::move(inst), pos);
}

// 仅支持形如：gep ptr, i32 idx 的一维等价形式；
// 以及数组GEP getelementptr inbounds [n x T], ptr %arr, i32 0, i32 idx
static bool decomposeGEP(Instruction* I, Value*& basePtr, Value*& index, Type& elemTy, int& arraySize) {
    if (!I || I->opcode != Opcode::GetElementPtr) return false;
    auto gep = dynamic_cast<GetElementPtrInst*>(I);
    if (!gep) return false;
    basePtr = gep->operands[0];
    index = gep->operands[1];
    elemTy = gep->elementType;
    arraySize = gep->arraySize;
    return true;
}

// 简易SCEV相加（仅常量折叠 + 构造SCEVAddExpr）
static std::unique_ptr<SCEV> scevAddSimple(std::unique_ptr<SCEV> a, std::unique_ptr<SCEV> b) {
    if (!a) return b;
    if (!b) return a;
    if (auto* ca = dynamic_cast<SCEVConstant*>(a.get())) {
        if (auto* cb = dynamic_cast<SCEVConstant*>(b.get())) {
            return std::make_unique<SCEVConstant>(ca->value + cb->value, ca->valueType);
        }
    }
    std::vector<std::unique_ptr<SCEV>> ops;
    ops.push_back(std::move(a));
    ops.push_back(std::move(b));
    // 取第一个操作数的类型作为值类型
    Type vt = ops.front()->valueType;
    return std::make_unique<SCEVAddExpr>(std::move(ops), vt);
}

// 展开链式GEP：累加所有索引，返回最根的base指针
static Value* flattenGEPChain(Instruction* leafGEP, std::vector<Value*>& indexParts, Type& elemTy, int& arraySize) {
    indexParts.clear();
    if (!leafGEP || leafGEP->opcode != Opcode::GetElementPtr) return nullptr;
    auto* cur = dynamic_cast<GetElementPtrInst*>(leafGEP);
    if (!cur) return nullptr;
    elemTy = cur->elementType;
    arraySize = cur->arraySize;

    Value* base = cur->operands[0];
    indexParts.push_back(cur->operands[1]);

    while (auto* baseInst = dynamic_cast<Instruction*>(base)) {
        if (baseInst->opcode != Opcode::GetElementPtr) break;
        auto* baseG = dynamic_cast<GetElementPtrInst*>(baseInst);
        if (!baseG) break;
        // 注意：沿链向上，叠加索引
        indexParts.push_back(baseG->operands[1]);
        base = baseG->operands[0];
    }
    return base;
}

// 从叶子GEP向上收集整条GEP链，并返回从根到叶的序列
static bool collectGEPChain(Instruction* leafGEP, std::vector<GetElementPtrInst*>& chainOut, std::vector<Value*>& indicesOut) {
    chainOut.clear();
    indicesOut.clear();
    if (!leafGEP || leafGEP->opcode != Opcode::GetElementPtr) return false;
    auto* cur = dynamic_cast<GetElementPtrInst*>(leafGEP);
    if (!cur) return false;
    std::vector<GetElementPtrInst*> tmpNodes;
    std::vector<Value*> tmpIdx;
    tmpNodes.push_back(cur);
    tmpIdx.push_back(cur->operands[1]);
    Value* base = cur->operands[0];
    while (auto* baseInst = dynamic_cast<Instruction*>(base)) {
        if (baseInst->opcode != Opcode::GetElementPtr) break;
        auto* baseG = dynamic_cast<GetElementPtrInst*>(baseInst);
        if (!baseG) break;
        tmpNodes.push_back(baseG);
        tmpIdx.push_back(baseG->operands[1]);
        base = baseG->operands[0];
    }
    // 逆转为根->叶
    for (int i = (int)tmpNodes.size() - 1; i >= 0; --i) {
        chainOut.push_back(tmpNodes[i]);
        indicesOut.push_back(tmpIdx[i]);
    }
    return true;
}

// 合成若干索引之和：在preheader里插入加法指令（折叠常量）
static Value* buildSumIndex(BasicBlock* preheader, const std::vector<Value*>& parts, const std::string& baseName) {
    int64_t constSum = 0;
    std::vector<Value*> nonConsts;
    for (auto* v : parts) {
        if (!v) continue;
        if (auto* ci = dynamic_cast<ConstantInt*>(v)) {
            constSum += ci->value;
        } else {
            nonConsts.push_back(v);
        }
    }
    Value* acc = nullptr;
    // 把常量部分先放进去
    if (constSum != 0) acc = new ConstantInt((int32_t)constSum);
    // 逐个相加
    int idx = 0;
    auto makeName = [&](int i){ return baseName + ".sum" + std::to_string(i); };
    for (auto* v : nonConsts) {
        if (!acc) { acc = v; continue; }
        auto add = std::make_unique<BinaryOperator>(Opcode::Add, acc, v, makeName(idx++));
        acc = add.get();
        insertBeforeTerminator(preheader, std::move(add));
    }
    if (!acc) acc = new ConstantInt(0);
    return acc;
}

// 在指定指令之前插入
static void insertBefore(Instruction* where, std::unique_ptr<Instruction> inst) {
    if (!where || !where->parent || !inst) return;
    BasicBlock* bb = where->parent;
    size_t idx = 0, pos = 0; bool found = false;
    for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ++it, ++idx) {
        if (it->get() == where) { pos = idx; found = true; break; }
    }
    if (!found) {
        // where 可能是PHI或不在 instructions 列表，安全起见插到指令列表开头（PHI单独存放，不会打乱PHI位置）
        bb->insertInstructionAt(std::move(inst), 0);
        return;
    }
    bb->insertInstructionAt(std::move(inst), pos);
}

// 确保不变量值在preheader可用：若其为当前loop内定义的简单算术，递归在preheader重建
static Value* materializeInvariantInPreheader(Value* v, BasicBlock* preheader, Loop& L) {
    if (!v) return nullptr;
    if (dynamic_cast<Constant*>(v)) return v;
    auto* inst = dynamic_cast<Instruction*>(v);
    if (!inst) return v;
    // 若不是当前loop内定义，视作已可用
    if (!L.contains(inst)) return v;

    // 支持重建BinaryOperator: Add/Sub/Mul
    auto* bin = dynamic_cast<BinaryOperator*>(inst);
    if (!bin) return nullptr;
    Value* a = materializeInvariantInPreheader(bin->operands[0], preheader, L);
    Value* b = materializeInvariantInPreheader(bin->operands[1], preheader, L);
    if (!a || !b) return nullptr;
    std::string name = inst->name + ".repl";
    auto rebuilt = std::make_unique<BinaryOperator>(bin->opcode, a, b, name);
    Value* out = rebuilt.get();
    insertBeforeTerminator(preheader, std::move(rebuilt));
    return out;
}

// 计算两个索引SCEV之间的常量差值（b - a），仅当二者均为同一循环上的AddRec且步长相同且起点为常量时返回常量
static std::optional<int64_t> getAddRecStartDiffConst(const SCEV* a, const SCEV* b) {
    if (!a || !b) return std::nullopt;
    if (a->type != SCEVType::AddRecExpr || b->type != SCEVType::AddRecExpr) return std::nullopt;
    auto* arA = static_cast<const SCEVAddRecExpr*>(a);
    auto* arB = static_cast<const SCEVAddRecExpr*>(b);
    if (arA->loop != arB->loop) return std::nullopt;
    // 步长必须等价
    if (!arA->step->equals(arB->step.get())) return std::nullopt;
    auto sA = arA->start->getConstantValue();
    auto sB = arB->start->getConstantValue();
    if (!sA.has_value() || !sB.has_value()) return std::nullopt;
    return sB.value() - sA.value();
}

// 解析形如 AddRec(L) 或 AddExpr(AddRec(L), Const) 的索引，返回对应的AddRec与常量偏移
static bool extractAddRecPlusConst(const SCEV* s, const Loop* L, const SCEVAddRecExpr*& outAR, int64_t& outK) {
    outAR = nullptr; outK = 0;
    if (!s) return false;
    if (s->type == SCEVType::AddRecExpr) {
        auto* ar = static_cast<const SCEVAddRecExpr*>(s);
        if (ar->loop != L) return false;
        outAR = ar; outK = 0; return true;
    }
    if (s->type == SCEVType::AddExpr) {
        auto* add = static_cast<const SCEVAddExpr*>(s);
        const SCEVAddRecExpr* found = nullptr;
        int64_t ksum = 0;
        for (const auto& op : add->operands) {
            if (op->type == SCEVType::Constant) {
                auto* c = static_cast<const SCEVConstant*>(op.get());
                ksum += c->value;
            } else if (op->type == SCEVType::AddRecExpr) {
                auto* ar = static_cast<const SCEVAddRecExpr*>(op.get());
                if (ar->loop != L) return false;
                if (found && !found->equals(ar)) return false;
                found = ar;
            } else {
                return false; // 暂不支持其他项
            }
        }
        if (!found) return false;
        outAR = found; outK = ksum; return true;
    }
    return false;
}

// 识别 AddRec(L) + sum(const) + sum(invariant Unknown) 形式，收集常量偏移与不变值偏移
static bool extractAddRecPlusAffine(SCEVAnalysis& SA, const SCEV* s, const Loop* L,
                                    const SCEVAddRecExpr*& outAR, int64_t& outK,
                                    std::vector<Value*>& invOffsets) {
    outAR = nullptr; outK = 0; invOffsets.clear();
    if (!s) return false;
    if (s->type == SCEVType::AddRecExpr) {
        auto* ar = static_cast<const SCEVAddRecExpr*>(s);
        if (ar->loop != L) return false;
        outAR = ar; outK = 0; return true;
    }
    if (s->type == SCEVType::AddExpr) {
        auto* add = static_cast<const SCEVAddExpr*>(s);
        const SCEVAddRecExpr* found = nullptr;
        int64_t ksum = 0;
        for (const auto& op : add->operands) {
            if (op->type == SCEVType::Constant) {
                ksum += static_cast<const SCEVConstant*>(op.get())->value;
            } else if (op->type == SCEVType::AddRecExpr) {
                auto* ar = static_cast<const SCEVAddRecExpr*>(op.get());
                if (ar->loop != L) return false;
                if (found && !found->equals(ar)) return false;
                found = ar;
            } else if (op->type == SCEVType::Unknown) {
                auto* unk = static_cast<const SCEVUnknown*>(op.get());
                if (SA.isLoopInvariant(unk->value)) invOffsets.push_back(unk->value);
                else return false;
            } else {
                return false;
            }
        }
        if (!found) return false;
        outAR = found; outK = ksum; return true;
    }
    if (s->type == SCEVType::Unknown) {
        // 允许纯 AddRec 通过 loop var 兜底处理
        return false;
    }
    return false;
}

// 兜底：若getSCEV未返回AddRec(+const)，尝试从当前循环变量表取出leaf本身的SCEV（仅支持AddRec）
static bool fallbackExtractFromLoopVar(SCEVAnalysis& SA, const Loop* L, Value* v, const SCEVAddRecExpr*& outAR, int64_t& outK) {
    outAR = nullptr; outK = 0;
    auto& vars = SA.getCurrentLoopVariables();
    auto it = vars.find(v);
    if (it == vars.end() || !it->second.scev) return false;
    const SCEV* s = it->second.scev.get();
    if (s->type != SCEVType::AddRecExpr) return false;
    auto* ar = static_cast<const SCEVAddRecExpr*>(s);
    if (ar->loop != L) return false;
    outAR = ar; outK = 0;
    return true;
}

// 在循环内做“GEP差值重用”：若同一基本块内已有相同base/形状的GEP，再次出现仅索引常量偏移不同，则生成从旧GEP加偏移的GEP
static bool reduceRedundantGepsInLoop(Loop& L, SCEVAnalysis& SA) {
    bool changed = false;
    for (auto* bb : L.getBlocks()) {
        // if (DEBUG_LSR) std::cout << "[LSR] BasicBlock: " << bb->label << std::endl;
        // 记录本块内已见到的叶子GEP，按“同一基指针（叶子GEP的第一个操作数）+ 形状”归类
        struct Key { Value* leafBase; Type elemTy; int arrSize; };
        struct Seen { Instruction* inst; std::unique_ptr<SCEV> idxSCEV; };
        std::vector<std::pair<Key, Seen>> seen;
        std::set<Instruction*> toRemove;
        std::unordered_map<Instruction*, Value*> replacePtrMap;

        for (auto& instUP : bb->instructions) {
            if (!instUP) {std::cerr << "[LSR] instUP is null" << std::endl; continue; }
            // if (DEBUG_LSR) { std::cout << "[LSR] Instruction: "; std::cout << instUP->toString() << std::endl; }
            Instruction* I = instUP.get();
            if (I->opcode != Opcode::GetElementPtr) continue;
            auto* gep = dynamic_cast<GetElementPtrInst*>(I);
            if (!gep) continue;

            Value* leafBase = gep->operands[0];
            Value* idx = gep->operands[1];
            Type et = gep->elementType; int as = gep->arraySize;

            auto idxS = SA.getSCEV(idx);
            if (!idxS) continue;

            for (auto& [k, s] : seen) {
                if (k.leafBase != leafBase || k.elemTy != et || k.arrSize != as) continue;
                // 计算 b - a 是否常量
                std::optional<int64_t> diffConst;
                if (auto ca = s.idxSCEV->getConstantValue()) {
                    if (auto cb = idxS->getConstantValue()) diffConst = cb.value() - ca.value();
                }
                // 支持 AddRec(+const)
                if (!diffConst) {
                    const SCEVAddRecExpr *arA=nullptr, *arB=nullptr; int64_t kA=0, kB=0;
                    if (!extractAddRecPlusConst(s.idxSCEV.get(), &L, arA, kA)) {
                        // 兜底：s.idxSCEV 不是AddRec(+const)，尝试从loop var获取
                        fallbackExtractFromLoopVar(SA, &L, s.inst, arA, kA);
                    }
                    if (!extractAddRecPlusConst(idxS.get(), &L, arB, kB)) {
                        fallbackExtractFromLoopVar(SA, &L, idx, arB, kB);
                    }
                    if (arA && arB && arA->step->equals(arB->step.get())) {
                        auto sd = getAddRecStartDiffConst(arA, arB); // 仅起点差
                        int64_t kd = kB - kA;
                        if (sd) diffConst = sd.value() + kd;
                    }
                }
                if (diffConst && diffConst.value() != 0) {
                    std::string name = I->name;
                    auto newGep = std::make_unique<GetElementPtrInst>(s.inst, new ConstantInt((int32_t)diffConst.value()), name, et, as);
                    Instruction* newPtr = newGep.get();
                    insertBefore(I, std::move(newGep));
                    I->replaceAllUsesWith(newPtr);
                    replacePtrMap[I] = newPtr;
                    toRemove.insert(I);
                    ++gStats.reused;
                    changed = true;
                    break;
                }
            }

            Key k{leafBase, et, as};
            seen.push_back({k, Seen{I, std::move(idxS)}});
        }
        for (auto* I : toRemove) {
            auto it = replacePtrMap.find(I);
            Value* repl = (it != replacePtrMap.end()) ? it->second : nullptr;
            // 强制清空残留uses
            auto uses = I->getUses();
            std::vector<Instruction*> copy(uses.begin(), uses.end());
            for (auto* u : copy) {
                if (!repl) continue; // 理论上都应有repl
                u->replaceOperand(I, repl);
            }
            if (I->getUses().size() > 0) {
                std::cerr << "[Warning] Instruction::removeFromParent: instruction has uses" << std::endl;
                std::cerr << "[Warning] Instruction: " << I->toString() << std::endl;
            }
            I->removeFromParent();
        }
        toRemove.clear();
    }
    return changed;
}

// 将循环内的GEP强度削弱为指针携带形式（preheader init + header phi + latch step）
static bool runOnLoop_PtrInduction(Function& F, Module& M, Loop& L, SCEVAnalysis& SA) {
    bool changed = false;
    if (DEBUG_LSR) {
        std::cout << "[LSR]   Preheader: " << (L.getPreheader() ? L.getPreheader()->label : "<null>") << std::endl;
        std::cout << "[LSR]   Latches: " << L.getLatches().size() << std::endl;
        for (auto* lt : L.getLatches()) {
            std::cout << "[LSR]     Latch: " << lt->label << std::endl;
        }
    }
    if (L.getLatches().size() != 1 || L.getPreheader() == nullptr) {
        if (DEBUG_LSR) std::cout << "[LSR]   Skip PtrInduction due to latch/preheader guard" << std::endl;
        return false;
    }
    BasicBlock* header = L.getHeader();
    BasicBlock* preheader = L.getPreheader();
    BasicBlock* latch = *L.getLatches().begin();

    std::vector<Instruction*> geps;
    for (auto bb : L.getBlocks()) {
        for (auto& inst : bb->instructions) {
            if (inst->opcode == Opcode::GetElementPtr) geps.push_back(inst.get());
        }
    }
    if (DEBUG_LSR) std::cout << "[LSR]   GEP candidates: " << geps.size() << std::endl;

    for (Instruction* I : geps) {
        // 收集链（根->叶）
        std::vector<GetElementPtrInst*> chain;
        std::vector<Value*> chainIdx;
        if (!collectGEPChain(I, chain, chainIdx)) {
            if (DEBUG_LSR) std::cout << "[LSR]   collectGEPChain failed" << std::endl;
            continue;
        }
        if (chain.empty()) {
            if (DEBUG_LSR) std::cout << "[LSR]   Empty chain" << std::endl;
            continue;
        }
        if (DEBUG_LSR) {
            std::cout << "[LSR] Candidate GEP: " << I->toString() << std::endl;
            std::cout << "[LSR]   Chain length: " << chain.size() << std::endl;
        }

        // 叶子索引必须为 AddRec {Start,+,Step}
        Value* leafIdx = chainIdx.back();
        auto idxSCEV = SA.getSCEV(leafIdx);
        const SCEVAddRecExpr* ar = nullptr; int64_t koff = 0;
        std::unique_ptr<SCEV> arOwned; // 统一owned副本容器
        std::vector<Value*> invOffsets; // 新增：不变量偏移
        if (DEBUG_LSR) {
            std::cout << "[LSR]   LeafIdx: " << leafIdx->toString() << std::endl;
            std::cout << "[LSR]   LeafIdx SCEV: " << (idxSCEV ? idxSCEV->toString() : std::string("<null>")) << std::endl;
        }
        bool ok = idxSCEV && extractAddRecPlusAffine(SA, idxSCEV.get(), &L, ar, koff, invOffsets);
        if (!ok) {
            if (!idxSCEV || !extractAddRecPlusConst(idxSCEV.get(), &L, ar, koff)) {
                // 兜底：从loop var表取AddRec
                if (!fallbackExtractFromLoopVar(SA, &L, leafIdx, ar, koff)) {
                    // 再兜底：直接匹配 leafIdx 的值形态（BinaryOperator Add），识别 AddRec(j) + Invariant(V)
                    if (auto* addInst = dynamic_cast<BinaryOperator*>(leafIdx)) {
                        if (addInst->opcode == Opcode::Add || addInst->opcode == Opcode::Sub) {
                            Value* a = addInst->operands[0];
                            Value* b = addInst->operands[1];
                            auto trySide = [&](Value* x, Value* y) -> bool {
                                // x 作为内层IV（PHI in header），y 为不变量
                                if (auto* phi = dynamic_cast<PhiInst*>(x)) {
                                    if (phi->parent == L.getHeader()) {
                                        // 获取该phi在当前循环的AddRec
                                        const SCEVAddRecExpr* arTmp = nullptr; int64_t kTmp = 0; std::vector<Value*> dummy;
                                        auto sPhi = SA.getSCEV(phi);
                                        if (sPhi && (extractAddRecPlusAffine(SA, sPhi.get(), &L, arTmp, kTmp, dummy) || extractAddRecPlusConst(sPhi.get(), &L, arTmp, kTmp))) {
                                            if (SA.isLoopInvariant(y)) {
                                                // clone立即保存，避免悬垂
                                                arOwned = arTmp->clone();
                                                ar = dynamic_cast<const SCEVAddRecExpr*>(arOwned.get());
                                                koff = 0; invOffsets.push_back(y); return true;
                                            }
                                        }
                                    }
                                }
                                return false;
                            };
                            if (!(trySide(a, b) || trySide(b, a))) {
                                if (DEBUG_LSR) std::cout << "[LSR]   Skip: leafIdx is not AddRec(+affine) in current loop" << std::endl;
                                continue;
                            }
                        } else {
                            if (DEBUG_LSR) std::cout << "[LSR]   Skip: leafIdx is not AddRec(+affine) in current loop" << std::endl;
                            continue;
                        }
                    } else {
                        if (DEBUG_LSR) std::cout << "[LSR]   Skip: leafIdx is not AddRec(+affine) in current loop" << std::endl;
                        continue;
                    }
                }
            }
        }
        // 重要：确保有owned副本；若前面未设置，则此处clone一次
        if (ar && !arOwned) { arOwned = ar->clone(); ar = dynamic_cast<const SCEVAddRecExpr*>(arOwned.get()); }
        if (!ar) {
            if (DEBUG_LSR) std::cout << "[LSR]   ar is null after clone, skip" << std::endl;
            continue;
        }
        // 调试输出需保护
        if (DEBUG_LSR) {
            std::cout << "[LSR]   ar: " << ar->toString() << std::endl;
        }
        auto stepC = ar->step->getConstantValue();
        if (DEBUG_LSR) {
            std::cout << "[LSR]   Step: " << (stepC ? std::to_string(stepC.value()) : std::string("<non-const>"))
                      << ", Koff: " << koff << ", invOffsets: " << invOffsets.size() << std::endl;
        }
        if (!stepC.has_value()) continue;

        // 其余各层索引均需循环不变
        bool invariantOk = true;
        for (size_t i = 0; i + 1 < chainIdx.size(); ++i) {
            bool inv = SA.isLoopInvariant(chainIdx[i]);
            if (DEBUG_LSR) {
                std::cout << "[LSR]   Invariant idx[" << i << "]: " << chainIdx[i]->toString()
                          << " -> " << (inv ? "true" : "false") << std::endl;
            }
            if (!inv) { invariantOk = false; break; }
        }
        if (!invariantOk) continue;

        // 起点允许常量或不变值，合成 Start + koff + sum(invOffsets)
        Value* startVal = nullptr;
        if (auto c = ar->start->getConstantValue()) {
            int64_t sv = c.value() + koff;
            startVal = new ConstantInt((int32_t)sv);
            if (DEBUG_LSR) std::cout << "[LSR]   Start const: " << sv << std::endl;
        } else if (auto* unk = dynamic_cast<SCEVUnknown*>(ar->start.get())) {
            if (!SA.isLoopInvariant(unk->value)) { if (DEBUG_LSR) std::cout << "[LSR]   Start not invariant, skip" << std::endl; continue; }
            // startVal = start + koff
            if (koff == 0) startVal = unk->value;
            else {
                std::string name = I->name + ".start.off";
                auto add = std::make_unique<BinaryOperator>(Opcode::Add, unk->value, new ConstantInt((int32_t)koff), name);
                startVal = add.get();
                insertBeforeTerminator(preheader, std::move(add));
            }
        } else {
            if (DEBUG_LSR) std::cout << "[LSR]   Unsupported start form, skip" << std::endl; continue;
        }
        // 加上不变偏移们
        {
            bool abortCandidate = false;
            for (size_t k = 0; k < invOffsets.size(); ++k) {
                auto* off = invOffsets[k];
                // 确保偏移值在preheader可用，必要时重建
                Value* avail = materializeInvariantInPreheader(off, preheader, L);
                if (!avail) { if (DEBUG_LSR) std::cout << "[LSR]   Cannot materialize invariant offset, skip" << std::endl; abortCandidate = true; break; }
                std::string name = I->name + ".start.inv." + std::to_string((int)k);
                auto add = std::make_unique<BinaryOperator>(Opcode::Add, startVal, avail, name);
                startVal = add.get();
                insertBeforeTerminator(preheader, std::move(add));
            }
            if (abortCandidate) continue;
        }

        // 重建 preheader 链：从根开始，依次应用每层不变索引；最后一层使用 startVal
        Value* currPtrBase = nullptr;
        // 根base
        Value* rootBase = chain.front()->operands[0];
        currPtrBase = rootBase;
        {
            bool abortCandidate = false;
            for (size_t i = 0; i + 1 < chain.size(); ++i) {
                auto* node = chain[i];
                auto* idx = chainIdx[i];
                // 确保每层索引也在preheader可用（虽然我们之前已检查不变量，但也做重建防御）
                Value* idxAvail = materializeInvariantInPreheader(idx, preheader, L);
                if (!idxAvail) { if (DEBUG_LSR) std::cout << "[LSR]   Cannot materialize invariant idx layer, skip" << std::endl; abortCandidate = true; break; }
                std::string name = I->name + ".init.l" + std::to_string((int)i);
                auto gep = std::make_unique<GetElementPtrInst>(currPtrBase, idxAvail, name, node->elementType, node->arraySize);
                if (DEBUG_LSR) std::cout << "[LSR]   Preheader build: " << gep->toString() << std::endl;
                currPtrBase = gep.get();
                insertBeforeTerminator(preheader, std::move(gep));
            }
            if (abortCandidate) continue;
        }
        // 最后一层（叶层）用 startVal
        auto* leafNode = chain.back();
        std::string initName = I->name + ".init";
        auto initPtr = std::make_unique<GetElementPtrInst>(currPtrBase, startVal, initName, leafNode->elementType, leafNode->arraySize);
        Value* initPtrVal = initPtr.get();
        insertBeforeTerminator(preheader, std::move(initPtr));

        // header: PHI
        std::string phiName = I->name;
        auto phi = std::make_unique<PhiInst>(Type::Ptr, phiName);
        PhiInst* phiPtr = phi.get();
        phiPtr->addIncoming(initPtrVal, preheader);
        header->insertPhiAt(std::move(phi), 0);

        // latch: 仅在叶层步进 step
        std::string nxtName = I->name + ".next";
        auto nextPtr = std::make_unique<GetElementPtrInst>(phiPtr, new ConstantInt((int32_t)stepC.value()), nxtName, leafNode->elementType, leafNode->arraySize);
        Value* nextPtrVal = nextPtr.get();
        insertBeforeTerminator(latch, std::move(nextPtr));
        phiPtr->addIncoming(nextPtrVal, latch);

        // 修复支配问题：重写 latch 外对 nextPtr 的使用
        {
            auto uses = nextPtrVal->getUses();
            std::vector<Instruction*> useCopy(uses.begin(), uses.end());
            for (auto* user : useCopy) {
                if (!user || user->parent == latch) continue;
                // 情况1：nextPtr 作为GEP的base
                if (auto* gepUser = dynamic_cast<GetElementPtrInst*>(user)) {
                    // 合并索引：step + 现有索引
                    Value* idx = gepUser->operands[1];
                    Value* sumIdx = nullptr;
                    if (auto* k = dynamic_cast<ConstantInt*>(idx)) {
                        sumIdx = new ConstantInt((int32_t)(stepC.value() + k->value));
                    } else {
                        auto addName = gepUser->name + ".step.sum";
                        auto addInst = std::make_unique<BinaryOperator>(Opcode::Add, new ConstantInt((int32_t)stepC.value()), idx, addName);
                        sumIdx = addInst.get();
                        insertBefore(gepUser, std::move(addInst));
                    }
                    auto newName = gepUser->name + ".repl";
                    auto newG = std::make_unique<GetElementPtrInst>(phiPtr, sumIdx, newName, gepUser->elementType, gepUser->arraySize);
                    Value* newBase = newG.get();
                    insertBefore(gepUser, std::move(newG));
                    // 替换整条GEP的所有用途
                    gepUser->replaceAllUsesWith(newBase);
                    gepUser->removeFromParent();
                    continue;
                }
                // 情况2：nextPtr 直接作为指针使用（load/store/其他）
                {
                    auto newName = nextPtrVal->name + ".as.use";
                    auto newG = std::make_unique<GetElementPtrInst>(phiPtr, new ConstantInt((int32_t)stepC.value()), newName, leafNode->elementType, leafNode->arraySize);
                    Value* newPtr = newG.get();
                    insertBefore(user, std::move(newG));
                    user->replaceOperand(nextPtrVal, newPtr);
                }
            }
        }

        if (DEBUG_LSR) {
            std::cout << "[LSR] PtrInduction: init " << initPtrVal->toString() << ", step " << stepC.value() << std::endl;
        }

        I->replaceAllUsesWith(phiPtr);
        I->removeFromParent();
        ++gStats.ptr;
        changed = true;
        continue;
    }

    if (changed) {
        cfg::rebuildUseDefChainsOnFunction(F, false);
        cfg::buildCFG(F);
    }

    return changed;
}

// 归并等价的归纳变量PHI：相同的起点与步长（基于SCEV等价）
static bool coalesceInductionPhis(Function& F, Loop& L, SCEVAnalysis& SA) {
    bool changed = false;
    BasicBlock* header = L.getHeader();
    BasicBlock* preheader = L.getPreheader();
    if (!header || !preheader) return false;

    // 键：startSCEV.toString() + "|" + stepSCEV.toString()
    auto scevToKey = [](const SCEV* s) -> std::string { return s ? s->toString() : std::string("<null>"); };

    std::unordered_map<std::string, PhiInst*> leaderMap;
    // 复制一份当前PHI指针列表，避免遍历中删除导致崩溃
    std::vector<PhiInst*> phiList;
    for (auto& phiUP : header->phi_instructions) {
        phiList.push_back(phiUP.get());
    }

    for (auto* phi : phiList) {
        if (!phi || phi->parent != header) continue; // 可能已被删除
        // 要求来自preheader的incoming作为初值
        Value* initFromPre = nullptr; bool hasPre = false;
        for (auto& inc : phi->incoming_values) {
            if (inc.second == preheader) { initFromPre = inc.first; hasPre = true; break; }
        }
        if (!hasPre || initFromPre == nullptr) continue;

        auto phiSCEV = SA.getSCEV(phi);
        if (!phiSCEV || phiSCEV->type != SCEVType::AddRecExpr) continue;
        auto* ar = static_cast<SCEVAddRecExpr*>(phiSCEV.get());
        if (ar->loop != &L) continue;
        auto key = scevToKey(ar->start.get()) + "|" + scevToKey(ar->step.get());

        auto it = leaderMap.find(key);
        if (it == leaderMap.end()) {
            leaderMap.emplace(key, phi);
            continue;
        }
        // 替换为leader
        PhiInst* leader = it->second;
        if (leader == phi) continue;
        if (DEBUG_LSR) std::cout << "[LSR] Coalesce PHI: " << phi->name << " -> " << leader->name << std::endl;
        phi->replaceAllUsesWith(leader);
        phi->removeFromParent();
        ++gStats.phi;
        changed = true;
    }

    if (changed) {
        cfg::rebuildUseDefChainsOnFunction(F, false);
        cfg::buildCFG(F);
    }

    return changed;
}

// 在循环内做“GEP差值重用”与“指针携带式LSR”
static bool runOnLoop(Function& F, Module& M, Loop& L, SCEVAnalysis& SA) {
    bool changed = false;

    // 确保当前loop上下文
    SA.setLoop(&L);

    if (DEBUG_LSR) std::cout << "[LSR] run reduceRedundantGepsInLoop on Loop: " << L.getHeader()->label << std::endl;
    changed |= reduceRedundantGepsInLoop(L, SA);
    // if (DEBUG_LSR) std::cout << "[LSR] run runOnLoop_PtrInduction on Loop: " << L.getHeader()->label << std::endl;
    // changed |= runOnLoop_PtrInduction(F, M, L, SA);

    return changed;
}

// 在模块级对每个函数/循环尝试
bool runOnModule(Module& M) {
    bool changed = false;

    for (auto& fptr : M.functions) {
        Function& F = *fptr;
        if (F.isDeclaration()) continue;
        if (DEBUG_LSR) std::cout << "[LSR] run on Function: " << F.name << std::endl;

        // 重置统计
        gStats = LSRStats{};

        cfg::rebuildUseDefChainsOnFunction(F, false);
        cfg::buildCFG(F);

        cfg::DominatorTree DT;
        DT.runOnFunction(F);
        LoopAnalysis LA;
        LA.runOnFunction(F, DT);
        std::cout << "[LSR] loops size: " << LA.loops.size() << std::endl;
        LA.normalizeLoops(F);
        std::cout << "[LSR] after normalize loops size: " << LA.loops.size() << std::endl;

        auto SA = analyzeSCEV(F, M, LA);
        std::cout << "[LSR] after scev loops size: " << LA.loops.size() << std::endl;

        // 后序遍历loops以保证子循环先处理
        std::vector<Loop*> worklist;
        std::function<void(Loop*)> collect = [&](Loop* L) {
            for (auto* ch : L->getChildren()) collect(ch);
            worklist.push_back(L);
        };
        for (auto& Lp : LA.loops) if (Lp->getParent() == nullptr) collect(Lp.get());
        std::cout << "[LSR] worklist size: " << worklist.size() << std::endl;

        bool funcChanged = false;
        for (auto* L : worklist) {
            if (DEBUG_LSR) std::cout << "[LSR] run on Loop: " << L->getHeader()->label << std::endl;
            L->printLoopInfo();
            funcChanged |= runOnLoop(F, M, *L, SA);
        }

        // 归并等价的归纳变量（函数级处理一次）
        // if (!LA.loops.empty()) {
        //     for (auto& Lp : LA.loops) {
        //         funcChanged |= coalesceInductionPhis(F, *Lp.get(), SA);
        //     }
        // }

        if (funcChanged) {
            cfg::rebuildUseDefChainsOnFunction(F, false);
            cfg::buildCFG(F);
        }

        if (DEBUG_LSR) {
            std::cout << "[LSR][Summary] Function " << F.name
                      << ": ReusedGEPs=" << gStats.reused
                      << ", PtrInductions=" << gStats.ptr
                      << ", PhiCoalesced=" << gStats.phi << std::endl;
        }

        changed |= funcChanged;
    }

    return changed;
}

} // namespace lsr
} // namespace llvm_ir 