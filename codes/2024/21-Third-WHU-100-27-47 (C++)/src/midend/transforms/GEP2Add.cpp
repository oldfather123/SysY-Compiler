#include "GEP2Add.h"

using namespace ir;

void ir::gep2add(FuncPtr func) {
    dbgout << std::endl
           << "GEP2Add pass started (" << func->name() << ")." << std::endl;

    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            if (auto gepInst = inst->as<GEPInst>()) {
                auto elemDataType = gepInst->destReg()->dataType().derefType();
                auto indexReg = Register::create(func, DataType{PrimaryDataType::Int}.longType());
                Instruction::insertBefore(gepInst, OperInst::createBinary(bb, indexReg, Op::Mul, gepInst->indexVal(), Literal{elemDataType.bytes()}));
                auto addInst = OperInst::createBinary(bb, gepInst->destReg(), Op::Add, gepInst->arrPtrReg(), indexReg);
                Instruction::replace(gepInst, addInst);
                dbgout << "├── Converted GEP instruction `" << gepInst->toString() << "` -> `" << addInst->toString() << "`" << std::endl;
            }
        }
    }

    dbgout << "└── GEP2Add pass done." << std::endl;
}
