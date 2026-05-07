#include "IR.h"
#include <sstream>
#include <iomanip>

using namespace ir;

template <>
unsigned IPrintOrderable<BasicBlock>::_printOrderCounter = 0;
template <>
unsigned IPrintOrderable<Function>::_printOrderCounter = 0;
template <>
unsigned IPrintOrderable<Instruction>::_printOrderCounter = 0;

String Value::toString() const {
    switch (this->_valueType) {
        case Type::Literal: {
            return this->getLiteral().toString();
        }
        case Type::Register: {
            return this->getRegister()->toString();
        }
        case Type::ArrayInitializer: {
            String str = "[";
            auto &initMap = this->getArrayInitMap();
            int i = 0;
            for (auto &[position, value] : initMap) {
                str += "(" + std::to_string(position) + ": " + value->toString() + ")" + (i < initMap.size() - 1 ? ", " : "");
                ++i;
            }
            str += "]";
            return str;
        }
        default:
            break;
    }
    return "<unspecified_value>";
}

String Literal::toString() const {
    switch (this->_dataType.baseDataType()) {
        case PrimaryDataType::Void: {
            return "";
        }
        case PrimaryDataType::Int: {
            return std::to_string(this->getInt());
        }
        case PrimaryDataType::Float: {
            // std::ostringstream out;
            // out << std::setprecision(std::numeric_limits<float>::) << this->getFloat();
            // std::string floatStr = out.str();
            return this->_floatStr + "F";
        }
        case PrimaryDataType::Bool: {
            return this->getBool() ? "1" : "0";
        }
        case PrimaryDataType::String: {
            return this->getString();
        }
        default: {
            break;
        }
    }
    dbgassert(false, "Invalid literal type");
    return "<invalid_literal_type>";
}

String Register::toString() const {
    return this->_isDiscard
               ? "_"
               : (this->_isGlobal
                      ? "@"
                  : this->_isRet
                      ? "$"
                      : "%") +
                     this->_name;
}

String PhiTuple::toString() const {
    return "[" + this->_value.toString() + ", " + this->_bb->label() + "]";
}

String PseudoInst::toString(bool exprOnly) const {
    String exprStr = "";

    if (exprOnly) {
        return exprStr;
    }
    if (this->_instType == Instruction::Type::PseudoEntry) {
        return "__PSEUDO_ENTRY_INST__";
    } else if (this->_instType == Instruction::Type::PseudoPlaceholder) {
        return "__PSEUDO_PLACEHOLDER_INST__";
    } else {
        dbgassert(false, "Invalid pseudo instruction type");
        return "";
    }
}

String GlobalInst::toString(bool exprOnly) const {
    String exprStr = (String) "global" + (this->_destReg->dataType().isConst() ? " const " : " ") + this->_destReg->dataType().derefType().toString() + " " + this->_initConstVal.toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String MoveInst::toString(bool exprOnly) const {
    String exprStr = this->_srcVal.dataType().toString() + " " + this->_srcVal.toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String OperInst::opToString() const {
    switch (_op) {
        case Operator::Shl:
            return "shl";
        case Operator::Shr:
            return "shr";
        case Operator::BitwiseAnd:
            return "and";
        case Operator::BitwiseOr:
            return "or";
        case Operator::Xor:
        case Operator::BitwiseXor:
            return "xor";
        case Operator::Add:
            return "add";
        case Operator::Sub:
            return "sub";
        case Operator::Mul:
            return "mul";
        case Operator::Div:
            return "div";
        case Operator::Mod:
            return "mod";
        case Operator::Eq:
            return "cmp eq";
        case Operator::Ne:
            return "cmp ne";
        case Operator::Lt:
            return "cmp lt";
        case Operator::Ge:
            return "cmp ge";
        case Operator::Not:
            return "not";
        case Operator::And:
            return "and";
        case Operator::Or:
            return "or";
        default:
            break;
    }
    dbgassert(false, "Unknown operator");
    return "<unknown_operator>";
}

String OperInst::toString(bool exprOnly) const {
    String exprStr = this->opToString() + " " + this->_destReg->dataType().toString() + ", ";
    if (this->isUnary()) {
        // Unary
        exprStr += this->_rhs.dataType().toString() + " " + this->_rhs.toString();
    } else {
        // Binary
        exprStr += this->_lhs.dataType().toString() + " " + this->_lhs.toString() + ", " + this->_rhs.dataType().toString() + " " + this->_rhs.toString();
    }

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String CallInst::toString(bool exprOnly) const {
    String exprStr = "call " + this->_function->retDataType().toString() + " @" + this->_function->name() + "(";
    int argCount = this->_argList.size();
    for (int i = 0; i < argCount; ++i) {
        auto &arg = this->_argList[i];
        exprStr += arg.dataType().toString() + " " + arg.toString() + (i < argCount - 1 ? ", " : "");
    }
    exprStr += ")";

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String LoadInst::toString(bool exprOnly) const {
    String exprStr = "load " + this->_destReg->dataType().toString() + ", " + this->_srcAddrReg->dataType().toString() + " " + this->_srcAddrReg->toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String StoreInst::toString(bool exprOnly) const {
    String exprStr = "";

    if (exprOnly) {
        return exprStr;
    }
    return "store " + this->_srcVal.dataType().toString() + " " + this->_srcVal.toString() + ", " + this->_destAddrReg->dataType().toString() + " " + this->_destAddrReg->toString();
}

String AllocInst::toString(bool exprOnly) const {
    String exprStr = "alloca " + this->_destReg->dataType().toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String CastInst::toString(bool exprOnly) const {
    String exprStr = "cast " + this->_destReg->dataType().toString() + ", " + this->_srcVal.dataType().toString() + " " + this->_srcVal.toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String GEPInst::toString(bool exprOnly) const {
    String exprStr = "getelementptr " + this->_destReg->dataType().toString() + ", " + this->_arrPtrReg->dataType().toString() + " " + this->_arrPtrReg->toString() + ", " + this->_indexVal.toString();

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String PhiInst::toString(bool exprOnly) const {
    String exprStr = "phi " + this->_destReg->dataType().toString() + " ";
    auto tuples = this->getTuples();
    int tupleCount = tuples.size();
    int i = 0;
    for (auto &tuple : tuples) {
        exprStr += tuple.toString() + (i < tupleCount - 1 ? ", " : "");
        ++i;
    }

    if (exprOnly) {
        return exprStr;
    }
    return this->_destReg->toString() + " = " + exprStr;
}

String RetInst::toString(bool exprOnly) const {
    String exprStr = "";

    if (exprOnly) {
        return exprStr;
    }
    return "ret " + this->_retVal.dataType().toString() + " " + this->_retVal.toString();
}

String ExitInst::toString(bool exprOnly) const {
#ifdef DEBUG
    if (_parentBB->outEdges().empty()) {
        dbgassert(_parentBB->exitInst() != nullptr, "Invalid basic block exit");
        dbgassert(_parentBB->exitInst().get() == this, "Invalid basic block exit");
    } else if (_parentBB->outEdges().size() == 1) {
        dbgassert(_parentBB->firstOutEdge()->tag() == CFGEdge::BrUncond, "Invalid basic block exit");
        dbgassert(_parentBB->firstOutEdge()->brCondition().isUnspecified(), "Invalid basic block exit");
    } else if (_parentBB->outEdges().size() == 2) {
        bool foundTrue = false, foundFalse = false;
        Value condition{};
        for (auto &outEdge : _parentBB->outEdges()) {
            dbgassert(outEdge->tag() == CFGEdge::BrTrue || outEdge->tag() == CFGEdge::BrFalse, "Invalid basic block exit");
            dbgassert(!outEdge->brCondition().isUnspecified(), "Invalid basic block exit");
            if (condition.isUnspecified()) {
                condition = outEdge->brCondition();
            } else {
                dbgassert(condition == outEdge->brCondition(), "Invalid basic block exit");
            }
            if (outEdge->tag() == CFGEdge::BrTrue) {
                foundTrue = true;
            } else if (outEdge->tag() == CFGEdge::BrFalse) {
                foundFalse = true;
            }
        }
    }
#endif

    String exprStr = "";

    if (exprOnly) {
        return exprStr;
    }
    if (this->isBranch()) {
        return "br " +
               (this->isUncondBr()
                    ? this->unconditionalTarget()->label()
                    : this->condition().dataType().toString() + " " + this->condition().toString() + ", " +
                          this->trueTarget()->label() + ", " + this->falseTarget()->label());
    } else if (this->isFuncExit()) {
        return "func_exit";
    }
    dbgassert(false, "Invalid exit instruction");
    return "<invalid_exit_instruction>";
}

String BasicBlock::toString() const {
    String str = this->_label + ":\n";
    for (auto inst : this->getInstTopoList()) {
        if (castPtr<PseudoInst>(inst)) {
            continue;
        }
        if (castPtr<RetInst>(inst) && this->_parentFunc->isGlobalInitFunc()) {
            continue;
        }
        if (auto exitInst = castPtr<ExitInst>(inst)) {
            if (exitInst->isFuncExit()) {
                continue;
            }
        }
        str += "    " + inst->toString() + "\n";
        // str += "        preds:";
        // for (auto p : inst->preds()) {
        //     str += " " + p->toString();
        // }
        // str += "\n";
        // str += "        succs:";
        // for (auto s : inst->succs()) {
        //     str += " " + s->toString();
        // }
        // str += "\n";
    }
    return str;
}

String Function::toString() const {
    String str = "define " + this->_retDataType.toString() + " @" + this->_name + "(";
    int paramCount = _paramList.size();
    for (int i = 0; i < paramCount; ++i) {
        auto param = _paramList[i];
        str += param->dataType().toString() + " " + param->toString() + (i < paramCount - 1 ? ", " : "");
    }
    str += ")\n";
    for (auto bb : BasicBlock::sortByPrintOrder(this->_bbSet)) {
        if ((bb == this->_entryBB || bb == this->_exitBB) && bb->isEmpty()) {
            // continue;
        }
        str += bb->toString();
    }
    return str;
}

String Module::toString() const {
    String str = "";
    str += this->_globalInitFunc->entryBB()->toString();
    for (auto func : Function::sortByPrintOrder(_funcSet)) {
        if (func->isPrototype()) {
            continue;
        }
        str += "\n" + func->toString();
    }
    return str;
}
