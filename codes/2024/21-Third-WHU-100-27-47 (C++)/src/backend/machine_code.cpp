#ifndef BACKEND_MACHINECODE_CPP_
#define BACKEND_MACHINECODE_CPP_

#include "machine_code.h"
#include "instruction.h"
#include <iomanip>

namespace backend {

    void processLiteralValue(const ir::Literal &literal, std::vector<std::string> &words) {
        if (literal.isInt()) {
            // 处理整型
            int32_t intValue = literal.getInt();
            words.push_back(".word " + std::to_string(intValue));
        } else if (literal.isFloat()) {
            // 处理浮点型
            String str = literal.toString();
            int32_t floatBits = floatToSigned(str);
            words.push_back(".word " + std::to_string(floatBits));
        }
    }

    void processArrayValues(const ir::Value &val, std::vector<std::string> &words) {
        dbgassert(!val.dataType().isPointer(), "Value should not be a pointer");

        int byteSize = DataType{val.dataType().baseDataType()}.bytes();
        int lastPosition = -1;
        for (auto &[position, value] : val.getArrayInitMap()) {
            if (position > lastPosition + 1) {
                words.push_back(".zero " + std::to_string((position - (lastPosition + 1)) * byteSize));
            }
            processLiteralValue(value->getLiteral(), words);
            lastPosition = position;
        }
        if (val.dataType().size() > lastPosition + 1) {
            words.push_back(".zero " + std::to_string((val.dataType().size() - (lastPosition + 1)) * byteSize));
        }
    }

    std::string GlobalBlock::toString() const {
        std::string result = "\t.type\t" + _tag + ",@object\n";
        result += "\t.global\t" + _tag + "\n";
        result += "\t.p2align\t2\n";
        result += _tag + ":\n";
        for (const auto &word : _words) {
            result += "\t" + word + "\n";
        }

        result += "\t.size\t" + _tag + ", " + std::to_string(_size) + "\n";
        return result;
    }

    Ptr<Instruction> RiscBasicBlock::addInstruction(Ptr<Instruction> inst) {
        _insts.push_back(inst);
        return inst;
    }

    void RiscBasicBlock::removeBack() {
        _insts.pop_back();
    }

    void RiscBasicBlock::clearInstruction() {
        _insts.clear();
    }

    void RiscBasicBlock::initLive(std::unordered_set<ir::Variable> &liveIn, std::unordered_set<ir::Variable> &liveOut, std::unordered_map<ir::RegPtr, Ptr<VirtualRegister>> &regMap) {
        for (auto &var : liveIn) {
            if (!var.isReg()) {
                continue;
            }
            auto reg = var.getReg();
            if (regMap.find(reg) != regMap.end()) {
                liveInSet.insert(regMap[reg]);
            }
        }

        for (auto &var : liveOut) {
            if (!var.isReg()) {
                continue;
            }
            auto reg = var.getReg();
            if (regMap.find(reg) != regMap.end()) {
                liveOutSet.insert(regMap[reg]);
            }
        }
    }

    std::string RiscBasicBlock::toString() const {
        std::string result = "";
        if (!isEntry) {
            result += _tag + ":";
            result += "\t\t##" + std::to_string(this->entryLine);
            result += "\n";
        }
        for (const auto &inst : _insts) {
            result += "\t" + inst->toString() + "\n";
        }
        return result;
    }

    Ptr<RiscBasicBlock> RiscFunction::createBasicBlock(const ir::BBPtr &src, Ptr<RiscFunction> func) {
        std::string tag = ".LBB" + std::to_string(_parentModule->funcCounter()) + "_" + src->label(); // std::to_string(blockCounter);
        bool flag = (blockCounter == -1);
        blockCounter++;
        Ptr<RiscBasicBlock> bb = std::make_shared<RiscBasicBlock>(tag, flag, src, func);
        _bbs.push_back(bb);
        return bb;
    }

    Ptr<RiscBasicBlock> RiscFunction::getBasicBlock(const ir::BBPtr &irbb) {
        for (const auto &bb : _bbs) {
            if (bb->sourceBB() == irbb) {
                return bb;
            }
        }
        return nullptr;
    }

    std::shared_ptr<VirtualRegister> &RiscFunction::getOrCreateVirtualRegister(const ir::RegPtr &irReg) {
        auto it = irToVirtualRegMap.find(irReg);
        if (it != irToVirtualRegMap.end()) {
            return it->second;
        } else {
            dbgassert(!irReg->dataType().isArray(), "Virtual register cannot be created for array data type, instead use array pointer");
            Register::Type type = (!irReg->dataType().isPointer() && irReg->dataType().baseDataType() == PrimaryDataType::Float) ? Register::Type::FLOAT : Register::Type::GENERAL;
            auto newVirtualReg = createVirtualRegister(parentModule()->virtualRegCounter++, irReg, type);
            irToVirtualRegMap[irReg] = newVirtualReg;
            // dbgout << "add" << newVirtualReg->regString() << irReg->toString() << std::endl;
            return irToVirtualRegMap[irReg];
        }
    }

    std::string RiscFunction::toString() const {
        std::string result = "\t.global\t" + _tag + "\n";
        result += "\t.p2align\t1\n";
        result += "\t.type\t" + _tag + ",@function\n";
        result += _tag + ":\n";

        auto stackSize = stackFrame.getStackSize();
        auto liSize = stackSize > 2032 ? stackSize - 2032 : 0;
        auto addiSize = stackSize - liSize;

        // Allocate stack frame and save context
        if (addiSize > 0) {
            result += "\taddi\tsp, sp, " + std::to_string(-addiSize) + "\n";
        }
        int offset = 0;
        for (auto reg : modifiedCalleeSavedRegs) {
            offset += 8;
            result += (reg->getType() == Register::Type::FLOAT ? "\tfsd\t" : "\tsd\t") + reg->regString() + ", " + std::to_string(addiSize - offset) + "(sp)\n";
        }
        if (addiSize > 0) {
            result += "\taddi\ts0, sp, " + std::to_string(addiSize) + "\n";
        }
        if (liSize > 0) {
            result += "\tli\tt0, " + std::to_string(-liSize) + "\n";
            result += "\tadd\tsp, sp, t0\n";
        }

        Ptr<RiscBasicBlock> exitBB = nullptr;
        for (const auto &bb : _bbs) {
            result += bb->toString();
        }

        // Restore context and release stack frame
        if (liSize > 0) {
            result += "\tli\tt0, " + std::to_string(liSize) + "\n";
            result += "\tadd\tsp, sp, t0\n";
        }
        offset = 0;
        for (auto reg : modifiedCalleeSavedRegs) {
            offset += 8;
            result += (reg->getType() == Register::Type::FLOAT ? "\tfld\t" : "\tld\t") + reg->regString() + ", " + std::to_string(addiSize - offset) + "(sp)\n";
        }
        if (addiSize > 0) {
            result += "\taddi\tsp, sp, " + std::to_string(addiSize) + "\n";
        }
        result += "\tret\n";

        // End Function
        static int count = 0;
        result += ".Lfunc_end" + std::to_string(count) + ":\n\t.size\t" + _tag + ", .Lfunc_end" + std::to_string(count) + "-" + _tag;
        result += "\n";
        count++;
        return result;
    }

    Ptr<RiscFunction> RiscModule::createFunction(const ir::FuncPtr &src, Ptr<RiscModule> module, bool addToFuncs) {
        std::string tag = src->name();
        bool flag = (_funcCounter == -1);
        if (!src->isPrototype()) {
            _funcCounter++;
        }
        Ptr<RiscFunction> func = std::make_shared<RiscFunction>(tag, src, module);
        if (addToFuncs)
            _funcs.push_back(func);
        else
            _protos.push_back(func);
        return func;
    }

    Ptr<GlobalBlock> RiscModule::createGlobal(const Ptr<ir::GlobalInst> &globalInst, Ptr<RiscModule> &module) {
        std::string tag = globalInst->destReg()->name();
        std::vector<std::string> words;
        auto val = globalInst->initConstVal();
        int size = val.dataType().bytes();

        if (val.isLiteral()) {
            processLiteralValue(val.getLiteral(), words);
        } else if (val.isArrayInitializer()) {
            processArrayValues(val, words);
        } else {
            words.push_back(".word 0");
        }
        Ptr<GlobalBlock> gb = std::make_shared<GlobalBlock>(tag, words, size);
        _global.push_back(gb);

        for (auto func : module->funcs()) {
            func->getOrCreateVirtualRegister(globalInst->destReg());
        }
        return gb;
    }

    Ptr<RiscFunction> RiscModule::getFunction(const ir::FuncPtr &irfunc) {
        for (const auto &func : _funcs) {
            if (func->sourceFunc() == irfunc) {
                return func;
            }
        }
        for (const auto &proto : _protos) {
            if (proto->sourceFunc() == irfunc) {
                return proto;
            }
        }
        return nullptr;
    }

    std::string RiscModule::toString() const {
        std::string result = "\t.text\n";
        result += "\t.attribute	5, \"rv64i2p0_m2p0_a2p0_f2p0_d2p0_c2p0\"\n\n";

        for (const auto &func : _funcs) {
            result += func->toString() + "\n";
        }

        if (!_global.empty()) {
            result += ".data\n";
        }

        for (const auto &global : _global) {
            result += global->toString() + "\n";
        }

        result += "\n\t.section	\".note.GNU-stack\",\"\",@progbits";
        return result;
    }
}

#endif
