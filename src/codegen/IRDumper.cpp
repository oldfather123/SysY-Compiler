#include "codegen/IRDumper.h"

#include <cstddef>
#include <ostream>
#include <unordered_map>

namespace codegen {
namespace {

const char *opcodeName(Opcode opcode) {
    switch (opcode) {
    case Opcode::ModuleOp: return "module";
    case Opcode::AddIOp: return "addi";
    case Opcode::SubIOp: return "subi";
    case Opcode::MulIOp: return "muli";
    case Opcode::DivIOp: return "divi";
    case Opcode::ModIOp: return "modi";
    case Opcode::AndIOp: return "andi";
    case Opcode::OrIOp: return "ori";
    case Opcode::XorIOp: return "xori";
    case Opcode::AddFOp: return "addf";
    case Opcode::SubFOp: return "subf";
    case Opcode::MulFOp: return "mulf";
    case Opcode::DivFOp: return "divf";
    case Opcode::ModFOp: return "modf";
    case Opcode::AddLOp: return "addl";
    case Opcode::SubLOp: return "subl";
    case Opcode::MulLOp: return "mull";
    case Opcode::DivLOp: return "divl";
    case Opcode::ModLOp: return "modl";
    case Opcode::EqOp: return "eq";
    case Opcode::NeOp: return "ne";
    case Opcode::LtOp: return "lt";
    case Opcode::LeOp: return "le";
    case Opcode::EqFOp: return "eqf";
    case Opcode::NeFOp: return "nef";
    case Opcode::LtFOp: return "ltf";
    case Opcode::LeFOp: return "lef";
    case Opcode::FuncOp: return "func";
    case Opcode::IntOp: return "int";
    case Opcode::FloatOp: return "float";
    case Opcode::AllocaOp: return "alloca";
    case Opcode::GetArgOp: return "getarg";
    case Opcode::StoreOp: return "store";
    case Opcode::LoadOp: return "load";
    case Opcode::ReturnOp: return "ret";
    case Opcode::IfOp: return "if";
    case Opcode::WhileOp: return "while";
    case Opcode::ForOp: return "for";
    case Opcode::ProceedOp: return "proceed";
    case Opcode::GotoOp: return "goto";
    case Opcode::BranchOp: return "branch";
    case Opcode::GlobalOp: return "global";
    case Opcode::GetGlobalOp: return "getglobal";
    case Opcode::CallOp: return "call";
    case Opcode::PhiOp: return "phi";
    case Opcode::F2IOp: return "f2i";
    case Opcode::I2FOp: return "i2f";
    case Opcode::MinusOp: return "minus";
    case Opcode::MinusFOp: return "minusf";
    case Opcode::NotOp: return "not";
    case Opcode::LShiftOp: return "lshift";
    case Opcode::LShiftLOp: return "lshiftl";
    case Opcode::RShiftOp: return "rshift";
    case Opcode::RShiftLOp: return "rshiftl";
    case Opcode::MulshOp: return "mulsh";
    case Opcode::MuluhOp: return "muluh";
    case Opcode::SetNotZeroOp: return "setnotzero";
    case Opcode::BreakOp: return "break";
    case Opcode::ContinueOp: return "continue";
    case Opcode::SelectOp: return "select";
    }
    return "unknown";
}

} // namespace

IRDumper::IRDumper(std::ostream &out) : out_(out) {}

void IRDumper::dump(const Region &region) {
    std::unordered_map<const Op *, std::size_t> opIds;
    std::size_t nextId = 0;
    for (const auto &blockPtr : region.blocks()) {
        if (blockPtr == nullptr) {
            continue;
        }
        for (const auto &opPtr : blockPtr->ops()) {
            opIds[opPtr.get()] = nextId++;
        }
    }

    for (std::size_t bi = 0; bi < region.blocks().size(); ++bi) {
        const BasicBlock *block = region.blockAt(bi);
        out_ << "block " << bi;
        if (block != nullptr && !block->name().empty()) {
            out_ << " (" << block->name() << ")";
        }
        out_ << ":\n";
        if (block == nullptr) {
            continue;
        }

        bool inFunctionBody = false;
        for (const auto &opPtr : block->ops()) {
            if (opPtr->opcode() == Opcode::FuncOp) {
                inFunctionBody = true;
            }

            out_ << "  ";
            if (inFunctionBody && opPtr->opcode() != Opcode::FuncOp) {
                out_ << "  ";
            }
            out_ << "%" << opIds[opPtr.get()] << " = " << opcodeName(opPtr->opcode());
            out_ << " (";
            for (std::size_t oi = 0; oi < opPtr->operands().size(); ++oi) {
                if (oi != 0) {
                    out_ << ", ";
                }
                const Op *def = opPtr->operands()[oi].defining();
                if (def == nullptr) {
                    out_ << "null";
                    continue;
                }
                auto it = opIds.find(def);
                if (it == opIds.end()) {
                    out_ << "<external>";
                } else {
                    out_ << "%" << it->second;
                }
            }
            out_ << ")";
            if (opPtr->attrs().has_value()) {
                out_ << " <";
                bool first = true;
                for (const auto &attr : *opPtr->attrs()) {
                    if (!first) {
                        out_ << ", ";
                    }
                    first = false;
                    out_ << attr.key() << "=" << attr.value();
                }
                out_ << ">";
            }
            out_ << '\n';
        }
    }
}

} // namespace codegen
