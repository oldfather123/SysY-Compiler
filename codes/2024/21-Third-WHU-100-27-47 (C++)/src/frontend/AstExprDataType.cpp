
#include "Ast.h"
using namespace ast;

DataType InitExpr::getDataType() const {
    if (singleInitVal) {
        return singleInitVal->getDataType();
    } else {
        // TODO Array initialization
        dbgassert(false, "Not implemented yet");
        return PrimaryDataType::Unknown;
    }
}

DataType LValueExpr::getDataType() const {
    dbgassert(this->declVarIR != nullptr, "declVarIR not filled in, cannot get data type");
    DataType dataType = this->declVarIR->dataType();
    for (int i = 0; i < this->indices.size(); ++i) {
        dataType = dataType.elemType();
    }
    return dataType;
}

DataType BinaryExpr::getDataType() const {
    DataType dataType(PrimaryDataType::Unknown);
    switch (this->op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod: {
            auto lhsType = this->lhs->getDataType();
            auto rhsType = this->rhs->getDataType();
            dbgassert(lhsType.isNumeric() && rhsType.isNumeric(), "Invalid operand data type");
            if (lhsType == PrimaryDataType::Float || rhsType == PrimaryDataType::Float) {
                dataType = PrimaryDataType::Float;
            } else {
                dataType = PrimaryDataType::Int;
            }
            break;
        }
        case BinaryOp::Eq:
        case BinaryOp::Neq:
        case BinaryOp::Lt:
        case BinaryOp::Gt:
        case BinaryOp::Leq:
        case BinaryOp::Geq: {
            auto lhsType = this->lhs->getDataType();
            auto rhsType = this->rhs->getDataType();
            dbgassert(lhsType.isNumeric() && rhsType.isNumeric(), "Invalid operand data type");
            dataType = PrimaryDataType::Bool;
            break;
        }
        case BinaryOp::And:
        case BinaryOp::Or: {
            dbgassert((this->lhs->getDataType().isNumeric() && this->rhs->getDataType().isNumeric()), "Invalid operand data type");
            dataType = PrimaryDataType::Bool;
            break;
        }
        default: {
            break;
        }
    }
    return dataType;
}

DataType UnaryExpr::getDataType() const {
    DataType dataType(PrimaryDataType::Unknown);
    switch (this->op) {
        case UnaryOp::Add:
        case UnaryOp::Sub: {
            auto operandDataType = this->operand->getDataType();
            dataType = operandDataType == PrimaryDataType::Bool ? PrimaryDataType::Int : operandDataType;
            break;
        }
        case UnaryOp::Not: {
            auto operandDataType = this->operand->getDataType();
            dbgassert(operandDataType.isNumeric(), "Invalid operand data type");
            dataType = PrimaryDataType::Bool;
            break;
        }
        default: {
            break;
        }
    }
    return dataType;
}

DataType IntLiteralExpr::getDataType() const {
    return PrimaryDataType::Int;
}

DataType FloatLiteralExpr::getDataType() const {
    return PrimaryDataType::Float;
}

DataType StringLiteralExpr::getDataType() const {
    return PrimaryDataType::String;
}

DataType CallExpr::getDataType() const {
    dbgassert(this->functionIR != nullptr, "functionIR not filled in, cannot get data type");
    return this->functionIR->retDataType();
}
