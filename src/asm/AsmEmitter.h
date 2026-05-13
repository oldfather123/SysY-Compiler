#pragma once

#include "codegen/Defs.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace codegen {

class AsmEmitter {
public:
    explicit AsmEmitter(std::ostream &out);

    void emit(const Region &region);

private:
    // First pass: collect function block ranges and globals.
    void collectModule(const Region &region);

    // Emit a single function (FuncOp + all its body blocks).
    void emitFunction(const std::vector<BasicBlock *> &blocks);
    void emitGlobalInitFunction(const std::vector<BasicBlock *> &blocks);

    // Pre-scan a function's blocks to compute frame layout.
    void preScanFunction(const std::vector<BasicBlock *> &blocks);

    // Emit prologue/epilogue.
    void emitPrologue();
    void emitEpilogue();
    void emitAdjustStackPointer(int delta);

    // Emit a single IR op.
    void emitOp(Op *op);

    // Emit individual opcodes.
    void emitFuncOp(Op *op);
    void emitIntOp(Op *op);
    void emitFloatOp(Op *op);
    void emitAllocaOp(Op *op);
    void emitGlobalOp(Op *op);
    void emitGetArgOp(Op *op);
    void emitLoadOp(Op *op);
    void emitStoreOp(Op *op);
    void emitReturnOp(Op *op);
    void emitGotoOp(Op *op);
    void emitBranchOp(Op *op);
    void emitCallOp(Op *op);
    void emitBinaryOp(Op *op, const std::string &inst);
    void emitBinaryFpOp(Op *op, const std::string &inst);
    void emitCompareOp(Op *op, const std::string &inst);
    void emitCompareFpOp(Op *op, const std::string &inst);
    void emitUnaryOp(Op *op, const std::string &inst);
    void emitUnaryFpOp(Op *op, const std::string &inst);
    void emitConversionOp(Op *op, bool toFloat);
    void emitSetNotZeroOp(Op *op);

    // Load operand value into a register. Returns the register name.
    std::string loadIntTo(Op *operand, const std::string &reg);
    std::string loadFloatTo(Op *operand, const std::string &freg);

    // Store register value to an op's spill slot.
    void storeIntFrom(const std::string &reg, Op *dest);
    void storeFloatFrom(const std::string &freg, Op *dest);

    // Load/store using an address (held in an int register).
    void emitLoadFromAddr(const std::string &addrReg, const std::string &destReg,
                          bool isFloat, bool isWideInt);
    void emitStoreToAddr(const std::string &srcReg, const std::string &addrReg,
                         bool isFloat, bool isWideInt);

    // Stack-addressing helpers that tolerate large frame offsets.
    void emitStackAddress(const std::string &destReg, int offset,
                          const std::string &baseReg = "sp");
    void emitStackLoad(const std::string &inst, const std::string &destReg,
                       int offset, const std::string &baseReg = "sp");
    void emitStackStore(const std::string &inst, const std::string &srcReg,
                        int offset, const std::string &baseReg = "sp");

    // Stack offset for an op's spill slot.
    int slotOffset(Op *op) const;

    // Check if an op produces a float value.
    bool isFloatOp(Op *op) const;
    bool isWideIntOp(Op *op) const;

    // Assign spill slots for non-void ops in a function.
    void assignSpillSlots(const std::vector<BasicBlock *> &blocks);

    // Get the block label for a block.
    std::string blockLabel(const BasicBlock *block) const;

    std::ostream &out_;
    std::size_t labelCounter_ = 0;

    // Global variables: name -> label in .data section.
    std::unordered_map<std::string, std::string> globalLabels_;

    // Per-function state (reset for each function).
    int spillOffset_;                   // next available spill slot offset
    std::unordered_map<Op *, int> opSpillSlots_;  // op -> stack offset
    std::unordered_map<Op *, int> allocaAddrOffsets_; // alloca op -> addr offset from sp
    std::unordered_map<const BasicBlock *, std::string> blockLabels_;
    int allocaEnd_;                     // end of alloca region
    int frameSize_;                     // total frame size
    int outgoingArgsOffset_;            // base offset of outgoing stack args area
    int maxCallArgs_;                   // max stack args in any call in this function
    bool hasFloatCalleeSaved_;
    std::vector<BasicBlock *> globalInitBlocks_;
    bool hasGlobalInitOps_ = false;
};

} // namespace codegen
