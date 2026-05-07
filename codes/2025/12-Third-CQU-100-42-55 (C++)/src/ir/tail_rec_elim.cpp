#include "../../include/ir/tail_rec_elim.h"
#include <set>
#include <iostream>

namespace llvm_ir {
namespace tail_rec_elim {

static bool isBuiltin(const std::string& name) {
    static const std::set<std::string> builtins = {
        "getint","getch","getfloat","getarray","getfarray","putint","putch","putfloat","putarray","putfarray","starttime","stoptime","_sysy_starttime","_sysy_stoptime","memset","printf","scanf"};
    return builtins.count(name);
}

// Helpers to recognize trivial value forwarding instructions we can peel through
static bool isTrivialForwarding(Instruction* I) {
    if (!I) return false;
    switch (I->opcode) {
        case Opcode::Move:
        case Opcode::ZExt:
        case Opcode::SExt:
        case Opcode::Trunc:
        case Opcode::BitCast:
            return true;
        default: return false;
    }
}

static Instruction* stripForwarding(Instruction* I) {
    std::set<Instruction*> visited;
    while (I && isTrivialForwarding(I) && !I->operands.empty()) {
        if (visited.count(I)) break; // avoid cycles (should not happen)
        visited.insert(I);
        Value* op = I->operands[0];
        I = dynamic_cast<Instruction*>(op);
    }
    return I;
}

// Detect tail recursive call in a return block (more robust: follow def-use chain of ret value)
static CallInst* getTailRecursiveCall(BasicBlock* BB, Function* F) {
    Instruction* term = BB->getTerminator();
    if (!term || term->opcode != Opcode::Ret) return nullptr;
    auto retInst = static_cast<ReturnInst*>(term);
    if (F->returnType == Type::Void) {
        // For void, tail call must be immediately before ret and no side-effect insts after it
        if (BB->instructions.empty()) return nullptr;
        // Find last non-terminator instruction
        for (auto it = BB->instructions.rbegin(); it != BB->instructions.rend(); ++it) {
            if ((*it)->opcode == Opcode::Ret || (*it)->opcode == Opcode::Br) continue;
            if ((*it)->opcode == Opcode::Call) {
                auto *CI = static_cast<CallInst*>(it->get());
                if (CI->functionName == F->name) return CI;
            }
            break;
        }
        return nullptr;
    }
    if (retInst->operands.empty()) return nullptr;
    Value* retVal = retInst->operands[0];
    Instruction* retDef = dynamic_cast<Instruction*>(retVal);
    if (!retDef) return nullptr;
    retDef = stripForwarding(retDef);
    auto callDef = dynamic_cast<CallInst*>(retDef);
    if (!callDef) return nullptr;
    if (callDef->functionName != F->name) return nullptr;
    // Ensure no side-effecting instructions occur between the call and the ret in the same block
    // (Conservative: if the call not in this block, reject)
    if (callDef->parent != BB) return nullptr;
    bool between=false;
    for (auto &instUP : BB->instructions) {
        Instruction* I = instUP.get();
        if (I == callDef) between = true; // mark start
        else if (I == retInst) break; // reached ret
        else if (between) {
            // Allow only trivial forwarding chain instructions we've already stripped (e.g., Move)
            if (!isTrivialForwarding(I)) {
                return nullptr; // something else in between -> not a tail call
            }
        }
    }
    return callDef;
}

// Find the semantic original entry (skip previously inserted header if present)
static BasicBlock* findOriginalEntry(Function& F) {
    if (F.basicBlocks.empty()) return nullptr;
    if (F.basicBlocks[0]->label.find("tailrecurse_header") != std::string::npos) {
        if (F.basicBlocks.size() > 1) return F.basicBlocks[1].get();
    }
    return F.basicBlocks[0].get();
}

static BasicBlock* ensureLoopHeader(Function& F, BasicBlock* originalEntry) {
    // If header already exists at position 0, reuse it
    if (!F.basicBlocks.empty() && F.basicBlocks[0]->label.find("tailrecurse_header") != std::string::npos) {
        return F.basicBlocks[0].get();
    }
    auto header = std::make_unique<BasicBlock>("tailrecurse_header", &F);
    BasicBlock* headerPtr = header.get();
    F.insertBasicBlockAt(std::move(header), 0);
    IRBuilder builder; builder.setInsertPoint(headerPtr);
    builder.createBr(originalEntry->label);
    return headerPtr;
}

static void createOrFetchEntryPHIs(Function& F, BasicBlock* header, BasicBlock* entry, std::vector<PhiInst*>& paramPhis) {
    if (!entry->phi_instructions.empty()) {
        bool allMatch = entry->phi_instructions.size() >= F.parameters.size();
        if (allMatch) {
            for (size_t i=0;i<F.parameters.size();++i) paramPhis.push_back(entry->phi_instructions[i].get());
            return;
        }
    }
    for (auto *param : F.parameters) {
        auto phi = std::make_unique<PhiInst>(param->type, param->name + "_tr_phi");
        PhiInst* phiPtr = phi.get();
        phiPtr->addIncoming(param, header);
        entry->addPhi(std::move(phi));
        paramPhis.push_back(phiPtr);
        auto uses = param->getUses();
        for (auto* user : uses) {
            user->replaceOperand(param, phiPtr);
        }
    }
}

static void transformTailCall(Function& F, BasicBlock* BB, CallInst* callInst, ReturnInst* retInst, BasicBlock* header, BasicBlock* entry, const std::vector<PhiInst*>& paramPhis) {
    for (size_t i=0;i<paramPhis.size();++i) {
        Value* argVal = (i < callInst->operands.size()) ? callInst->operands[i] : callInst;
        paramPhis[i]->addIncoming(argVal, BB);
    }
    // Remove return first (it uses the call result)
    retInst->removeFromParent();
    callInst->removeFromParent();
    IRBuilder builder; builder.setInsertPoint(BB);
    builder.createBr(entry->label);
}

static bool runOnFunction(Function& F) {
    if (F.isDeclaration()) return false;
    if (isBuiltin(F.name) || F.name == "main") return false;
    // Quick scan: ensure at least one self call exists; allow stores/allocas (we only rewrite tail position).
    bool hasTailCandidate = false;
    for (auto &bbPtr : F.basicBlocks) {
        for (auto &instUP : bbPtr->instructions) {
            if (instUP->opcode == Opcode::Call) {
                auto *CI = static_cast<CallInst*>(instUP.get());
                if (CI->functionName == F.name) { hasTailCandidate = true; break; }
            }
        }
        if (hasTailCandidate) break;
    }
    if (!hasTailCandidate) return false;

    bool changed = false;
    // Preserve original semantic entry across all iterations
    BasicBlock* originalEntry = findOriginalEntry(F);
    BasicBlock* header = nullptr;
    BasicBlock* entry = originalEntry; // original entry block (after header branch target)
    std::vector<PhiInst*> paramPhis;
    while (true) {
        bool transformedOne = false;
        for (auto &bbPtr : F.basicBlocks) {
            BasicBlock* BB = bbPtr.get();
            CallInst* tailCall = getTailRecursiveCall(BB, &F);
            if (!tailCall) {
                // Debug if self-recursive value returned but not tail
                Instruction* term = BB->getTerminator();
                if (term && term->opcode == Opcode::Ret) {
                    auto *retInst = static_cast<ReturnInst*>(term);
                    if (!retInst->operands.empty()) {
                        if (auto *I = dynamic_cast<Instruction*>(retInst->operands[0])) {
                            Instruction* base = stripForwarding(I);
                            if (auto *CI = dynamic_cast<CallInst*>(base)) {
                                if (CI->functionName == F.name) {
                                    std::cout << "[TailRecElim][DEBUG] Block %" << BB->label << ": self call found but rejected as tail due to intervening instructions or placement." << std::endl;
                                }
                            }
                        }
                    }
                }
                continue;
            }
            std::cout << "[TailRecElim] Found tail call in function " << F.name << " block " << BB->label << " -> transforming" << std::endl;
            auto retInst = static_cast<ReturnInst*>(BB->getTerminator());
            if (!header) {
                header = ensureLoopHeader(F, originalEntry);
                createOrFetchEntryPHIs(F, header, entry, paramPhis);
                std::cout << "[TailRecElim] Inserted header block and PHIs for function " << F.name << std::endl;
            }
            transformTailCall(F, BB, tailCall, retInst, header, entry, paramPhis);
            transformedOne = true; changed = true; break; // restart scan; blocks vector may have changed order only at first insertion
        }
        if (!transformedOne) break;
    }
    if (changed) {
        std::cout << "[TailRecElim] Function " << F.name << " after transformation:" << std::endl;
        for (auto &bbPtr : F.basicBlocks) {
            std::cout << "  BasicBlock %" << bbPtr->label << ":" << std::endl;
            for (auto &phi : bbPtr->phi_instructions) {
                std::cout << "    " << phi->toString() << std::endl;
            }
            for (auto &inst : bbPtr->instructions) {
                std::cout << "    " << inst->toString() << std::endl;
            }
        }
    }
    return changed;
}

void runOnModule(Module& M) {
    std::cout << "[TailRecElim] Running tail recursion elimination..." << std::endl;
    bool any=false; for (auto &func : M.functions) any |= runOnFunction(*func);
    if (any) {
        cfg::rebuildUseDefChainsModule(M);
        for (auto &func : M.functions) { if(!func->isDeclaration()) cfg::buildCFG(*func); }
        std::cout << "[TailRecElim] Transformation applied." << std::endl;
    } else {
        std::cout << "[TailRecElim] No tail recursion found." << std::endl;
    }
}

} // namespace tail_rec_elim
} // namespace llvm_ir
