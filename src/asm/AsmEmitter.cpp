#include "asm/AsmEmitter.h"

#include "codegen/Ops.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

namespace codegen {
namespace {

constexpr int kSavedRegsSize = 16;
constexpr const char *kGlobalInitFunction = "__sysy_global_init";

// Convert a float to its IEEE 754 binary representation.
std::uint32_t floatToBits(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

// Parse an integer literal text (decimal, hex 0x, octal 0).
long long parseImm(const std::string &text) {
    if (text.empty()) return 0;
    try {
        return std::stoll(text, nullptr, 0);
    } catch (...) {
        return 0;
    }
}

// Parse a float literal text.
float parseFloat(const std::string &text) {
    if (text.empty()) return 0.0f;
    try {
        return std::stof(text);
    } catch (...) {
        return 0.0f;
    }
}

// Attribute helpers.
std::string getAttr(const Op *op, const std::string &key) {
    if (!op->attrs().has_value()) return {};
    for (const auto &attr : *op->attrs()) {
        if (attr.key() == key) return attr.value();
    }
    return {};
}

bool hasAttr(const Op *op, const std::string &key) {
    if (!op->attrs().has_value()) return false;
    for (const auto &attr : *op->attrs()) {
        if (attr.key() == key) return true;
    }
    return false;
}

bool isInt12(int value) {
    return value >= -2048 && value <= 2047;
}

int alignUp(int value, int alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

bool hasPointerTypeAttr(const Op *op) {
    const std::string type = getAttr(op, "type");
    if (!type.empty()) {
        return type.find('*') != std::string::npos;
    }

    const std::string ret = getAttr(op, "ret");
    if (!ret.empty()) {
        return ret.find('*') != std::string::npos;
    }

    return false;
}

} // namespace

AsmEmitter::AsmEmitter(std::ostream &out) : out_(out) {}

// ---------------------------------------------------------------------------
// Public entry
// ---------------------------------------------------------------------------

void AsmEmitter::emit(const Region &region) {
    labelCounter_ = 0;
    globalLabels_.clear();
    collectModule(region);
}

// ---------------------------------------------------------------------------
// First pass: collect globals and function block ranges
// ---------------------------------------------------------------------------

void AsmEmitter::collectModule(const Region &region) {
    // Collect all blocks.
    std::vector<BasicBlock *> allBlocks;
    for (const auto &block : region.blocks()) {
        allBlocks.push_back(block.get());
    }

    // Separate globals and function groups.
    // Globals are emitted as GlobalOp in the first block(s).
    // Functions start with FuncOp.
    std::vector<std::vector<BasicBlock *>> functions;
    std::vector<BasicBlock *> currentGroup;
    bool inFunction = false;
    globalInitBlocks_.clear();
    hasGlobalInitOps_ = false;

    for (BasicBlock *block : allBlocks) {
        if (block->empty()) {
            if (inFunction) {
                currentGroup.push_back(block);
            } else {
                globalInitBlocks_.push_back(block);
            }
            continue;
        }

        // Check if this block starts a new function.
        Op *firstOp = block->ops().front().get();
        if (firstOp->opcode() == Opcode::FuncOp) {
            // Save previous function if any.
            if (!currentGroup.empty() && inFunction) {
                functions.push_back(std::move(currentGroup));
                currentGroup.clear();
            }
            inFunction = true;
        }

        if (inFunction) {
            currentGroup.push_back(block);
        } else {
            globalInitBlocks_.push_back(block);
            // Global declarations.
            for (const auto &op : block->ops()) {
                if (op->opcode() == Opcode::GlobalOp) {
                    std::string name = getAttr(op.get(), "name");
                    std::string sizeStr = getAttr(op.get(), "size");
                    std::size_t size = 4;
                    try { size = std::stoul(sizeStr); } catch (...) {}
                    globalLabels_[name] = name;

                    out_ << "    .globl " << name << "\n";
                    out_ << "    .section .bss\n";
                    out_ << "    .align 3\n";
                    out_ << name << ":\n";
                    out_ << "    .zero " << size << "\n";
                } else {
                    hasGlobalInitOps_ = true;
                }
            }
        }
    }
    if (!currentGroup.empty() && inFunction) {
        functions.push_back(std::move(currentGroup));
    }

    // Emit .text section before functions.
    out_ << "    .text\n";

    if (hasGlobalInitOps_) {
        emitGlobalInitFunction(globalInitBlocks_);
    }

    // Emit each function.
    for (auto &fnBlocks : functions) {
        emitFunction(fnBlocks);
    }
}

// ---------------------------------------------------------------------------
// Emit a single function
// ---------------------------------------------------------------------------

void AsmEmitter::emitFunction(const std::vector<BasicBlock *> &blocks) {
    // Reset per-function state.
    opSpillSlots_.clear();
    allocaAddrOffsets_.clear();
    blockLabels_.clear();
    spillOffset_ = 0;
    allocaEnd_ = 0;
    frameSize_ = 0;
    outgoingArgsOffset_ = 0;
    maxCallArgs_ = 0;
    hasFloatCalleeSaved_ = false;

    // Assign labels to all blocks.
    for (BasicBlock *block : blocks) {
        std::string label = "L" + std::to_string(labelCounter_++);
        blockLabels_[block] = label;
    }

    // Pre-scan to compute frame layout.
    preScanFunction(blocks);

    // Emit function body.
    for (BasicBlock *block : blocks) {
        out_ << blockLabels_[block] << ":\n";
        for (const auto &op : block->ops()) {
            emitOp(op.get());
        }
    }
}

void AsmEmitter::emitGlobalInitFunction(const std::vector<BasicBlock *> &blocks) {
    opSpillSlots_.clear();
    allocaAddrOffsets_.clear();
    blockLabels_.clear();
    spillOffset_ = 0;
    allocaEnd_ = 0;
    frameSize_ = 0;
    outgoingArgsOffset_ = 0;
    maxCallArgs_ = 0;
    hasFloatCalleeSaved_ = false;

    for (BasicBlock *block : blocks) {
        blockLabels_[block] = "LG" + std::to_string(labelCounter_++);
    }

    preScanFunction(blocks);

    out_ << "    .globl " << kGlobalInitFunction << "\n";
    out_ << "    .type " << kGlobalInitFunction << ", @function\n";
    out_ << kGlobalInitFunction << ":\n";
    emitPrologue();

    for (BasicBlock *block : blocks) {
        if (!block->empty()) {
            out_ << blockLabels_[block] << ":\n";
        }
        for (const auto &op : block->ops()) {
            if (op->opcode() == Opcode::GlobalOp) {
                continue;
            }
            emitOp(op.get());
        }
    }

    emitEpilogue();
}

// ---------------------------------------------------------------------------
// Pre-scan: compute frame layout
// ---------------------------------------------------------------------------

void AsmEmitter::preScanFunction(const std::vector<BasicBlock *> &blocks) {
    int allocaSize = 0;
    int spillCount = 0;
    int maxStackArgs = 0;

    for (BasicBlock *block : blocks) {
        for (const auto &op : block->ops()) {
            Opcode opc = op->opcode();

            if (opc == Opcode::FuncOp) continue;
            if (opc == Opcode::BreakOp || opc == Opcode::ContinueOp) continue;

            // Count alloca space and record offsets.
            if (opc == Opcode::AllocaOp) {
                std::string sizeStr = getAttr(op.get(), "size");
                int sz = 8;
                try { sz = std::stoul(sizeStr); } catch (...) {}
                sz = std::max(8, alignUp(sz, 8));
                allocaSize += sz;
            }

            // Track max outgoing stack args for calls.
            if (opc == Opcode::CallOp) {
                int intArgIdx = 0;
                int floatArgIdx = 0;
                int stackArgIdx = 0;
                for (const Value &operand : op->operands()) {
                    Op *argOp = operand.defining();
                    if (argOp != nullptr && isFloatOp(argOp)) {
                        if (floatArgIdx >= 8) {
                            stackArgIdx++;
                        }
                        floatArgIdx++;
                    } else {
                        if (intArgIdx >= 8) {
                            stackArgIdx++;
                        }
                        intArgIdx++;
                    }
                }
                maxStackArgs = std::max(maxStackArgs, stackArgIdx);
            }

            // Count non-void ops for spill slots.
            if (op->returnType() != ValueType::Void) {
                spillCount++;
            }
        }
    }

    allocaEnd_ = allocaSize;

    // Frame layout (low addr to high):
    // sp+0:             outgoing stack args area (maxArgs * 8 bytes)
    // sp+outgoingSize:  ra (8 bytes)
    // sp+outgoingSize+8:s0 (8 bytes)
    // then:             alloca area (allocaEnd_ bytes)
    // then:             spill slots (spillCount * 8 bytes)
    // sp+frame:         <- old sp

    int outgoingSize = maxStackArgs * 8;
    int savedSize = kSavedRegsSize; // ra + s0
    int spillSize = spillCount * 8;
    frameSize_ = outgoingSize + savedSize + allocaEnd_ + spillSize;

    // Align to 16 bytes.
    frameSize_ = alignUp(frameSize_, 16);
    outgoingArgsOffset_ = 0;
    maxCallArgs_ = maxStackArgs;

    int allocaOffset = outgoingSize + savedSize;
    for (BasicBlock *block : blocks) {
        for (const auto &op : block->ops()) {
            if (op->opcode() != Opcode::AllocaOp) {
                continue;
            }
            std::string sizeStr = getAttr(op.get(), "size");
            int sz = 8;
            try { sz = std::stoul(sizeStr); } catch (...) {}
            sz = std::max(8, alignUp(sz, 8));
            allocaAddrOffsets_[op.get()] = allocaOffset;
            allocaOffset += sz;
        }
    }

    // Assign spill slot offsets (start after outgoing args + saved regs + alloca).
    int spillBase = outgoingSize + savedSize + allocaEnd_;
    int spillIdx = 0;
    for (BasicBlock *block : blocks) {
        for (const auto &op : block->ops()) {
            if (op->returnType() != ValueType::Void &&
                op->opcode() != Opcode::FuncOp) {
                opSpillSlots_[op.get()] = spillBase + spillIdx * 8;
                spillIdx++;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Stack helpers
// ---------------------------------------------------------------------------

int AsmEmitter::slotOffset(Op *op) const {
    auto it = opSpillSlots_.find(op);
    if (it == opSpillSlots_.end()) {
        throw std::runtime_error("AsmEmitter: no spill slot for op");
    }
    return it->second;
}

bool AsmEmitter::isFloatOp(Op *op) const {
    return op->returnType() == ValueType::Float;
}

bool AsmEmitter::isWideIntOp(Op *op) const {
    if (op == nullptr || isFloatOp(op) || op->returnType() == ValueType::Void) {
        return false;
    }

    switch (op->opcode()) {
    case Opcode::AllocaOp:
    case Opcode::AddLOp:
    case Opcode::SubLOp:
    case Opcode::MulLOp:
    case Opcode::DivLOp:
    case Opcode::ModLOp:
    case Opcode::LShiftLOp:
    case Opcode::RShiftLOp:
    case Opcode::MulshOp:
    case Opcode::MuluhOp:
    case Opcode::GetGlobalOp:
        return true;
    default:
        break;
    }

    return hasPointerTypeAttr(op);
}

std::string AsmEmitter::blockLabel(const BasicBlock *block) const {
    auto it = blockLabels_.find(block);
    if (it == blockLabels_.end()) {
        throw std::runtime_error("AsmEmitter: unknown block");
    }
    return it->second;
}

// ---------------------------------------------------------------------------
// Load/store helpers
// ---------------------------------------------------------------------------

void AsmEmitter::emitStackAddress(const std::string &destReg, int offset,
                                  const std::string &baseReg) {
    if (isInt12(offset)) {
        out_ << "    addi " << destReg << ", " << baseReg << ", " << offset << "\n";
        return;
    }

    out_ << "    li t6, " << offset << "\n";
    out_ << "    add " << destReg << ", " << baseReg << ", t6\n";
}

void AsmEmitter::emitStackLoad(const std::string &inst, const std::string &destReg,
                               int offset, const std::string &baseReg) {
    if (isInt12(offset)) {
        out_ << "    " << inst << " " << destReg << ", " << offset << "(" << baseReg << ")\n";
        return;
    }

    emitStackAddress("t6", offset, baseReg);
    out_ << "    " << inst << " " << destReg << ", 0(t6)\n";
}

void AsmEmitter::emitStackStore(const std::string &inst, const std::string &srcReg,
                                int offset, const std::string &baseReg) {
    if (isInt12(offset)) {
        out_ << "    " << inst << " " << srcReg << ", " << offset << "(" << baseReg << ")\n";
        return;
    }

    emitStackAddress("t6", offset, baseReg);
    out_ << "    " << inst << " " << srcReg << ", 0(t6)\n";
}

void AsmEmitter::emitAdjustStackPointer(int delta) {
    if (delta == 0) {
        return;
    }
    if (isInt12(delta)) {
        out_ << "    addi sp, sp, " << delta << "\n";
        return;
    }

    out_ << "    li t6, " << delta << "\n";
    out_ << "    add sp, sp, t6\n";
}

std::string AsmEmitter::loadIntTo(Op *operand, const std::string &reg) {
    if (operand->opcode() == Opcode::GlobalOp) {
        out_ << "    la " << reg << ", " << getAttr(operand, "name") << "\n";
        return reg;
    }

    emitStackLoad(isWideIntOp(operand) ? "ld" : "lw", reg, slotOffset(operand));
    return reg;
}

std::string AsmEmitter::loadFloatTo(Op *operand, const std::string &freg) {
    emitStackLoad("flw", freg, slotOffset(operand));
    return freg;
}

void AsmEmitter::storeIntFrom(const std::string &reg, Op *dest) {
    emitStackStore(isWideIntOp(dest) ? "sd" : "sw", reg, slotOffset(dest));
}

void AsmEmitter::storeFloatFrom(const std::string &freg, Op *dest) {
    emitStackStore("fsw", freg, slotOffset(dest));
}

void AsmEmitter::emitLoadFromAddr(const std::string &addrReg,
                                  const std::string &destReg, bool isFloat,
                                  bool isWideInt) {
    if (isFloat) {
        out_ << "    flw " << destReg << ", 0(" << addrReg << ")\n";
    } else if (isWideInt) {
        out_ << "    ld " << destReg << ", 0(" << addrReg << ")\n";
    } else {
        out_ << "    lw " << destReg << ", 0(" << addrReg << ")\n";
    }
}

void AsmEmitter::emitStoreToAddr(const std::string &srcReg,
                                 const std::string &addrReg, bool isFloat,
                                 bool isWideInt) {
    if (isFloat) {
        out_ << "    fsw " << srcReg << ", 0(" << addrReg << ")\n";
    } else if (isWideInt) {
        out_ << "    sd " << srcReg << ", 0(" << addrReg << ")\n";
    } else {
        out_ << "    sw " << srcReg << ", 0(" << addrReg << ")\n";
    }
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

void AsmEmitter::emitOp(Op *op) {
    switch (op->opcode()) {
    case Opcode::FuncOp:      emitFuncOp(op); break;
    case Opcode::IntOp:       emitIntOp(op); break;
    case Opcode::FloatOp:     emitFloatOp(op); break;
    case Opcode::AllocaOp:    emitAllocaOp(op); break;
    case Opcode::GlobalOp:    break; // handled in collectModule
    case Opcode::GetArgOp:    emitGetArgOp(op); break;
    case Opcode::LoadOp:      emitLoadOp(op); break;
    case Opcode::StoreOp:     emitStoreOp(op); break;
    case Opcode::ReturnOp:    emitReturnOp(op); break;
    case Opcode::GotoOp:      emitGotoOp(op); break;
    case Opcode::BranchOp:    emitBranchOp(op); break;
    case Opcode::CallOp:      emitCallOp(op); break;
    case Opcode::AddIOp:      emitBinaryOp(op, "addw"); break;
    case Opcode::SubIOp:      emitBinaryOp(op, "subw"); break;
    case Opcode::MulIOp:      emitBinaryOp(op, "mulw"); break;
    case Opcode::DivIOp:      emitBinaryOp(op, "divw"); break;
    case Opcode::ModIOp:      emitBinaryOp(op, "remw"); break;
    case Opcode::AndIOp:      emitBinaryOp(op, "and"); break;
    case Opcode::OrIOp:       emitBinaryOp(op, "or"); break;
    case Opcode::XorIOp:      emitBinaryOp(op, "xor"); break;
    case Opcode::AddFOp:      emitBinaryFpOp(op, "fadd.s"); break;
    case Opcode::SubFOp:      emitBinaryFpOp(op, "fsub.s"); break;
    case Opcode::MulFOp:      emitBinaryFpOp(op, "fmul.s"); break;
    case Opcode::DivFOp:      emitBinaryFpOp(op, "fdiv.s"); break;
    case Opcode::LtOp:        emitCompareOp(op, "slt"); break;
    case Opcode::LeOp:
        // a <= b  ≡  !(b < a)  ≡  slt t0, b, a; xori t0, t0, 1
        loadIntTo(op->operands()[1].defining(), "t1");
        loadIntTo(op->operands()[0].defining(), "t0");
        out_ << "    slt t0, t1, t0\n";
        out_ << "    xori t0, t0, 1\n";
        storeIntFrom("t0", op);
        break;
    case Opcode::EqOp:
        loadIntTo(op->operands()[0].defining(), "t0");
        loadIntTo(op->operands()[1].defining(), "t1");
        out_ << "    xor t0, t0, t1\n";
        out_ << "    seqz t0, t0\n";
        storeIntFrom("t0", op);
        break;
    case Opcode::NeOp:
        loadIntTo(op->operands()[0].defining(), "t0");
        loadIntTo(op->operands()[1].defining(), "t1");
        out_ << "    xor t0, t0, t1\n";
        out_ << "    snez t0, t0\n";
        storeIntFrom("t0", op);
        break;
    case Opcode::LtFOp:       emitCompareFpOp(op, "flt.s"); break;
    case Opcode::LeFOp:       emitCompareFpOp(op, "fle.s"); break;
    case Opcode::EqFOp:       emitCompareFpOp(op, "feq.s"); break;
    case Opcode::NeFOp:
        loadFloatTo(op->operands()[0].defining(), "ft0");
        loadFloatTo(op->operands()[1].defining(), "ft1");
        out_ << "    feq.s t0, ft0, ft1\n";
        out_ << "    xori t0, t0, 1\n";
        storeIntFrom("t0", op);
        break;
    case Opcode::MinusOp:     emitUnaryOp(op, "negw"); break;
    case Opcode::MinusFOp:    emitUnaryFpOp(op, "fneg.s"); break;
    case Opcode::NotOp:       emitUnaryOp(op, "seqz"); break;
    case Opcode::I2FOp:       emitConversionOp(op, true); break;
    case Opcode::F2IOp:       emitConversionOp(op, false); break;
    case Opcode::SetNotZeroOp: emitSetNotZeroOp(op); break;
    case Opcode::AddLOp:      emitBinaryOp(op, "add"); break;
    case Opcode::SubLOp:      emitBinaryOp(op, "sub"); break;
    case Opcode::MulLOp:      emitBinaryOp(op, "mul"); break;
    case Opcode::LShiftOp:    emitBinaryOp(op, "sllw"); break;
    case Opcode::RShiftOp:    emitBinaryOp(op, "srlw"); break;
    // Opcodes that are no-ops or markers in this backend.
    case Opcode::BreakOp:
    case Opcode::ContinueOp:
        break;
    default:
        // Unknown opcode: emit a comment.
        out_ << "    # unhandled opcode: " << static_cast<int>(op->opcode()) << "\n";
        break;
    }
}

// ---------------------------------------------------------------------------
// Individual op emitters
// ---------------------------------------------------------------------------

void AsmEmitter::emitFuncOp(Op *op) {
    std::string name = getAttr(op, "name");
    out_ << "    .globl " << name << "\n";
    out_ << "    .type " << name << ", @function\n";
    out_ << name << ":\n";
    emitPrologue();
    if (hasGlobalInitOps_ && name == "main") {
        out_ << "    call " << kGlobalInitFunction << "\n";
    }
}

void AsmEmitter::emitIntOp(Op *op) {
    std::string valStr = getAttr(op, "value");
    long long val = parseImm(valStr);
    if (val >= -2048 && val <= 2047) {
        out_ << "    li t0, " << val << "\n";
    } else {
        // Use li which expands to lui+addi for large values.
        out_ << "    li t0, " << val << "\n";
    }
    storeIntFrom("t0", op);
}

void AsmEmitter::emitFloatOp(Op *op) {
    std::string valStr = getAttr(op, "value");
    float val = parseFloat(valStr);
    std::uint32_t bits = floatToBits(val);
    out_ << "    li t0, " << bits << "\n";
    out_ << "    fmv.w.x ft0, t0\n";
    storeFloatFrom("ft0", op);
}

void AsmEmitter::emitAllocaOp(Op *op) {
    auto it = allocaAddrOffsets_.find(op);
    if (it == allocaAddrOffsets_.end()) {
        out_ << "    # alloca offset not found\n";
        return;
    }
    emitStackAddress("t0", it->second);
    storeIntFrom("t0", op);
}

void AsmEmitter::emitGetArgOp(Op *op) {
    std::string reg = getAttr(op, "reg");
    std::string stackStr = getAttr(op, "stack");

    if (!reg.empty()) {
        if (isFloatOp(op)) {
            storeFloatFrom(reg, op);
        } else {
            storeIntFrom(reg, op);
        }
        return;
    }

    int stackIdx = 0;
    try { stackIdx = std::stoi(stackStr); } catch (...) {}
    int stackArgOffset = frameSize_ + stackIdx * 8;

    if (isFloatOp(op)) {
        emitStackLoad("flw", "ft0", stackArgOffset);
        storeFloatFrom("ft0", op);
    } else {
        emitStackLoad(isWideIntOp(op) ? "ld" : "lw", "t0", stackArgOffset);
        storeIntFrom("t0", op);
    }
}

void AsmEmitter::emitLoadOp(Op *op) {
    // LoadOp(addr) -> load value from address in operand.
    Op *addrOp = op->operands()[0].defining();
    bool isFloat = isFloatOp(op);
    bool isWideInt = isWideIntOp(op);

    // Load address from addrOp's spill slot.
    loadIntTo(addrOp, "t0");
    if (isFloat) {
        emitLoadFromAddr("t0", "ft0", true, false);
        storeFloatFrom("ft0", op);
    } else {
        emitLoadFromAddr("t0", "t1", false, isWideInt);
        storeIntFrom("t1", op);
    }
}

void AsmEmitter::emitStoreOp(Op *op) {
    // StoreOp(val, addr) -> store val to address.
    Op *valOp = op->operands()[0].defining();
    Op *addrOp = op->operands()[1].defining();
    bool isFloat = isFloatOp(valOp);
    bool isWideInt = isWideIntOp(valOp);

    loadIntTo(addrOp, "t0");
    if (isFloat) {
        loadFloatTo(valOp, "ft0");
        emitStoreToAddr("ft0", "t0", true, false);
    } else {
        loadIntTo(valOp, "t1");
        emitStoreToAddr("t1", "t0", false, isWideInt);
    }
}

void AsmEmitter::emitReturnOp(Op *op) {
    if (!op->operands().empty()) {
        Op *valOp = op->operands()[0].defining();
        if (isFloatOp(valOp)) {
            loadFloatTo(valOp, "fa0");
        } else {
            loadIntTo(valOp, "a0");
        }
    }
    emitEpilogue();
}

void AsmEmitter::emitGotoOp(Op *op) {
    std::string targetName = getAttr(op, "target");
    // Find the block with this name.
    for (const auto &[block, label] : blockLabels_) {
        if (block->name() == targetName) {
            out_ << "    j " << label << "\n";
            return;
        }
    }
    out_ << "    # goto unknown target: " << targetName << "\n";
}

void AsmEmitter::emitBranchOp(Op *op) {
    Op *condOp = op->operands()[0].defining();
    std::string trueName = getAttr(op, "true");
    std::string falseName = getAttr(op, "false");

    std::string trueLabel, falseLabel;
    for (const auto &[block, label] : blockLabels_) {
        if (block->name() == trueName) trueLabel = label;
        if (block->name() == falseName) falseLabel = label;
    }

    loadIntTo(condOp, "t0");
    if (!trueLabel.empty()) {
        out_ << "    bnez t0, " << trueLabel << "\n";
    }
    if (!falseLabel.empty()) {
        out_ << "    j " << falseLabel << "\n";
    }
}

void AsmEmitter::emitCallOp(Op *op) {
    std::string callee = getAttr(op, "callee");

    // Load arguments into registers.
    // Float args go to fa0-fa7, int args to a0-a7.
    // We need to classify args by type. Since we don't know the callee's
    // signature, we'll use a simple heuristic: if the arg op produces float,
    // use fa register; otherwise use a register.

    int intArgIdx = 0;
    int floatArgIdx = 0;
    int stackArgIdx = 0;

    for (std::size_t i = 0; i < op->operands().size(); ++i) {
        Op *argOp = op->operands()[i].defining();
        if (isFloatOp(argOp)) {
            if (floatArgIdx < 8) {
                loadFloatTo(argOp, "fa" + std::to_string(floatArgIdx));
            } else {
                // Store on stack.
                loadFloatTo(argOp, "ft0");
                emitStackStore("fsw", "ft0", outgoingArgsOffset_ + stackArgIdx * 8);
                stackArgIdx++;
            }
            floatArgIdx++;
        } else {
            if (intArgIdx < 8) {
                loadIntTo(argOp, "a" + std::to_string(intArgIdx));
            } else {
                // Store on stack.
                loadIntTo(argOp, "t0");
                emitStackStore(isWideIntOp(argOp) ? "sd" : "sw", "t0",
                               outgoingArgsOffset_ + stackArgIdx * 8);
                stackArgIdx++;
            }
            intArgIdx++;
        }
    }

    out_ << "    call " << callee << "\n";

    // Store return value.
    if (op->returnType() != ValueType::Void) {
        if (isFloatOp(op)) {
            storeFloatFrom("fa0", op);
        } else {
            storeIntFrom("a0", op);
        }
    }
}

void AsmEmitter::emitBinaryOp(Op *op, const std::string &inst) {
    loadIntTo(op->operands()[0].defining(), "t0");
    loadIntTo(op->operands()[1].defining(), "t1");
    out_ << "    " << inst << " t0, t0, t1\n";
    storeIntFrom("t0", op);
}

void AsmEmitter::emitBinaryFpOp(Op *op, const std::string &inst) {
    loadFloatTo(op->operands()[0].defining(), "ft0");
    loadFloatTo(op->operands()[1].defining(), "ft1");
    out_ << "    " << inst << " ft0, ft0, ft1\n";
    storeFloatFrom("ft0", op);
}

void AsmEmitter::emitCompareOp(Op *op, const std::string &inst) {
    loadIntTo(op->operands()[0].defining(), "t0");
    loadIntTo(op->operands()[1].defining(), "t1");
    out_ << "    " << inst << " t0, t0, t1\n";
    storeIntFrom("t0", op);
}

void AsmEmitter::emitCompareFpOp(Op *op, const std::string &inst) {
    loadFloatTo(op->operands()[0].defining(), "ft0");
    loadFloatTo(op->operands()[1].defining(), "ft1");
    out_ << "    " << inst << " t0, ft0, ft1\n";
    storeIntFrom("t0", op);
}

void AsmEmitter::emitUnaryOp(Op *op, const std::string &inst) {
    loadIntTo(op->operands()[0].defining(), "t0");
    out_ << "    " << inst << " t0, t0\n";
    storeIntFrom("t0", op);
}

void AsmEmitter::emitUnaryFpOp(Op *op, const std::string &inst) {
    loadFloatTo(op->operands()[0].defining(), "ft0");
    out_ << "    " << inst << " ft0, ft0\n";
    storeFloatFrom("ft0", op);
}

void AsmEmitter::emitConversionOp(Op *op, bool toFloat) {
    if (toFloat) {
        // I2F: int -> float
        loadIntTo(op->operands()[0].defining(), "t0");
        out_ << "    fcvt.s.w ft0, t0\n";
        storeFloatFrom("ft0", op);
    } else {
        // F2I: float -> int
        loadFloatTo(op->operands()[0].defining(), "ft0");
        out_ << "    fcvt.w.s t0, ft0, rtz\n";
        storeIntFrom("t0", op);
    }
}

void AsmEmitter::emitSetNotZeroOp(Op *op) {
    loadIntTo(op->operands()[0].defining(), "t0");
    out_ << "    snez t0, t0\n";
    storeIntFrom("t0", op);
}

// ---------------------------------------------------------------------------
// Prologue / Epilogue
// ---------------------------------------------------------------------------

void AsmEmitter::emitPrologue() {
    emitAdjustStackPointer(-frameSize_);
    emitStackStore("sd", "ra", maxCallArgs_ * 8);
    emitStackStore("sd", "s0", maxCallArgs_ * 8 + 8);
    emitStackAddress("s0", frameSize_);
}

void AsmEmitter::emitEpilogue() {
    emitStackLoad("ld", "ra", maxCallArgs_ * 8);
    emitStackLoad("ld", "s0", maxCallArgs_ * 8 + 8);
    emitAdjustStackPointer(frameSize_);
    out_ << "    ret\n";
}

} // namespace codegen
