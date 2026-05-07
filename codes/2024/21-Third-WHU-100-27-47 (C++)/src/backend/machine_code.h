#ifndef BACKEND_MACHINECODE_H_
#define BACKEND_MACHINECODE_H_

#include "Common.h"
#include "IR.h"
#include "register.h"

namespace backend {
    class RiscScope {
    private:

    public:
        virtual ~RiscScope() { }
    };

    class Instruction;

    class GlobalBlock : public RiscScope {
        friend class RiscModule;

    private:
        std::string _tag;
        std::vector<std::string> _words;
        int _size;

    public:
        std::string tag() { return _tag; }
        std::vector<std::string> words() { return _words; }

        GlobalBlock(const std::string &tag, std::vector<std::string> words, int size)
            : _tag(tag), _words(words), _size(size) { }

        std::string toString() const;
    };

    void processLiteralValue(const ir::Literal &, std::vector<std::string> &);

    void processArrayValues(const ir::Value &, std::vector<std::string> &);

    class RiscBasicBlock : public RiscScope {
        friend class RiscFunction;

    private:
        Ptr<RiscFunction> _parentFunc;
        ir::BBPtr _sourceBB;
        std::string _tag;
        bool isEntry;
        std::vector<Ptr<Instruction>> _insts;

        std::unordered_set<Ptr<VirtualRegister>> liveInSet;
        std::unordered_set<Ptr<VirtualRegister>> liveOutSet;

    public:
        RiscBasicBlock() = default;

        explicit RiscBasicBlock(const std::string &tag, bool isEntry, const ir::BBPtr &src, Ptr<RiscFunction> func)
            : _tag(tag), isEntry(isEntry), _sourceBB(src), _parentFunc(func) { }

        Ptr<RiscFunction> parentFunc() { return _parentFunc; }
        ir::BBPtr sourceBB() { return _sourceBB; }
        std::vector<Ptr<Instruction>> &insts() { return _insts; }
        std::unordered_set<Ptr<VirtualRegister>> &getLiveInSet() { return liveInSet; }
        std::unordered_set<Ptr<VirtualRegister>> &getLiveOutSet() { return liveOutSet; }
        std::string tag() { return _tag; }

        void initLive(std::unordered_set<ir::Variable> &, std::unordered_set<ir::Variable> &, std::unordered_map<ir::RegPtr, Ptr<VirtualRegister>> &);
        Ptr<Instruction> addInstruction(Ptr<Instruction>);
        void removeBack();
        void clearInstruction();
        std::string toString() const;

        int entryLine = 0;
    };

    struct StackFrame {
        int calleeSavedRegSize = 0; // Callee-saved registers (s0, ra, etc.), decreases from s0
        int callerSavedRegSize = 0; // Caller-saved registers (a0~a7, etc.), decreases from callee-saved registers
        int spillSize = 0;          // Total size of spilled variables, decreases from caller-saved registers
        int memVarSize = 0;         // Total size of allocated memory variables, decreases from spilled variables
        int innerMemArgSize = 0;    // Max size of arguments passed to inner callees on the stack, increases from sp

        // Relative to s0
        int getOffsetForCallerSavedReg(int callerSavedRegOffset) const {
            return -calleeSavedRegSize - callerSavedRegSize + callerSavedRegOffset;
        }

        // Relative to s0
        int getOffsetForSpill(int spillOffset) const {
            return -calleeSavedRegSize - callerSavedRegSize - spillSize + spillOffset;
        }

        // Relative to s0
        int getOffsetForMemVar(int memVarOffset) const {
            return -calleeSavedRegSize - callerSavedRegSize - spillSize - memVarSize + memVarOffset;
        }

        static int align(int size, int alignment) {
            return (size + alignment - 1) / alignment * alignment;
        }

        void updateMaxInnerMemArgSizeIfLarger(int size) {
            innerMemArgSize = align(MAX(innerMemArgSize, size), 8);
        }

        // Bottom-up allocation for spilled variables
        int allocSpill(int size) {
            auto t = spillSize;
            spillSize = align(spillSize + size, 8);
            return t;
        }

        // Bottom-up allocation for memory variables (arrays are allocated first)
        int allocMemVar(int size) {
            auto t = memVarSize;
            memVarSize = align(memVarSize + size, 8);
            return t;
        }

        int allocCalleeSavedReg(int size) {
            calleeSavedRegSize = align(calleeSavedRegSize + size, 8);
            return calleeSavedRegSize;
        }

        void updateCallerSavedRegSizeIfLarger(int size) {
            callerSavedRegSize = align(MAX(callerSavedRegSize, size), 8);
        }

        int getStackSize() const {
            auto stackSize = calleeSavedRegSize + callerSavedRegSize + spillSize + memVarSize + innerMemArgSize;
            if (stackSize % 16 != 0) {
                stackSize += 16 - (stackSize % 16);
            }
            return stackSize;
        }
    };

    class RiscFunction : public RiscScope {
        friend class RiscModule;

    private:
        Ptr<RiscModule> _parentModule;
        ir::FuncPtr _sourceFunc;
        std::string _tag;
        std::vector<Ptr<RiscBasicBlock>> _bbs;

        std::unordered_map<ir::RegPtr, std::shared_ptr<VirtualRegister>> irToVirtualRegMap;

        std::unordered_map<Ptr<RiscBasicBlock>, std::unordered_set<ir::RegPtr>> liveInMap;
        std::unordered_map<Ptr<RiscBasicBlock>, std::unordered_set<ir::RegPtr>> liveOutMap;

        signed int blockCounter = -1;

    public:
        StackFrame stackFrame{};
        std::set<Ptr<Register>, Register::RegisterPtrHash> regsRead;
        std::set<Ptr<Register>, Register::RegisterPtrHash> regsWritten;
        Vector<Ptr<Register>> modifiedCalleeSavedRegs{}; // Callee-saved registers to save on the stack

        int lineCounter = 0;
        RiscFunction(const std::string &tag, const ir::FuncPtr &src, Ptr<RiscModule> module)
            : _tag(tag), _sourceFunc(src), _parentModule(module) { }

        Ptr<RiscModule> parentModule() { return _parentModule; }
        std::vector<Ptr<RiscBasicBlock>> bbs() { return _bbs; }
        ir::FuncPtr sourceFunc() { return _sourceFunc; }
        std::string tag() { return _tag; }

        std::unordered_map<ir::RegPtr, std::shared_ptr<VirtualRegister>> getIrToVirtualRegMap() { return irToVirtualRegMap; }
        std::unordered_map<Ptr<RiscBasicBlock>, std::unordered_set<ir::RegPtr>> getLiveInMap() { return liveInMap; }
        std::unordered_map<Ptr<RiscBasicBlock>, std::unordered_set<ir::RegPtr>> getLiveOutInMap() { return liveOutMap; }

        Ptr<RiscBasicBlock> createBasicBlock(const ir::BBPtr &, Ptr<RiscFunction>);
        Ptr<RiscBasicBlock> getBasicBlock(const ir::BBPtr &);
        std::shared_ptr<VirtualRegister> &getOrCreateVirtualRegister(const ir::RegPtr &);

        std::string toString() const;
    };

    class RiscModule : public RiscScope {
    private:
        int _funcCounter = -1;
        std::vector<Ptr<RiscFunction>> _funcs;
        std::vector<Ptr<RiscFunction>> _protos;
        std::vector<Ptr<GlobalBlock>> _global;

    public:
        int virtualRegCounter = 0;
        std::vector<Ptr<RiscFunction>> funcs() { return _funcs; }
        int funcCounter() { return _funcCounter; }
        std::vector<Ptr<GlobalBlock>> global() { return _global; }

        Ptr<RiscFunction> createFunction(const ir::FuncPtr &, Ptr<RiscModule>, bool addToFuncs = true);
        Ptr<GlobalBlock> createGlobal(const Ptr<ir::GlobalInst> &, Ptr<RiscModule> &);
        Ptr<RiscFunction> getFunction(const ir::FuncPtr &);

        std::string toString() const;
    };

} // namespace backend

#endif
