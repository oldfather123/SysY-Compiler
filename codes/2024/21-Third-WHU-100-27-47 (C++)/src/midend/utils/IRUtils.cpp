#include "IRUtils.h"

using namespace ir;

DataType ir::getOperandCastTargetDataType(const ir::OperInst::Operator &op, const DataType &lhsDataType, const DataType &rhsDataType) {
    bool isLogicalOp = false;
    if (op == Op::And || op == Op::Or || op == Op::Not || op == Op::Xor) {
        isLogicalOp = true;
    }

    DataType operandDataType{};
    if (isLogicalOp) {
        operandDataType = PrimaryDataType::Bool;
    } else if (OperInst::isBinary(op)) {
        if (lhsDataType == PrimaryDataType::Float || rhsDataType == PrimaryDataType::Float) {
            operandDataType = PrimaryDataType::Float;
        } else {
            operandDataType = PrimaryDataType::Int;
        }
    } else {
        if (rhsDataType == PrimaryDataType::Float) {
            operandDataType = PrimaryDataType::Float;
        } else {
            operandDataType = PrimaryDataType::Int;
        }
    }
    return operandDataType;
}

ir::Literal ir::foldOperation(const ir::OperInst::Operator &op, ir::Literal lhsLiteral, ir::Literal rhsLiteral) {
    Literal resultLiteralVal{};

    auto toType = getOperandCastTargetDataType(op, lhsLiteral.dataType(), rhsLiteral.dataType());
    lhsLiteral = lhsLiteral.cast(toType);
    rhsLiteral = rhsLiteral.cast(toType);

    bool isIntOperation = false;
    bool isFloatOperation = false;
    bool isBoolOperation = false;
    if (OperInst::isBinary(op)) {
        isIntOperation = lhsLiteral.isInt() && rhsLiteral.isInt();
        isFloatOperation = lhsLiteral.isFloat() && rhsLiteral.isFloat();
        isBoolOperation = lhsLiteral.isBool() && rhsLiteral.isBool();
    } else {
        isIntOperation = rhsLiteral.isInt();
        isFloatOperation = rhsLiteral.isFloat();
        isBoolOperation = rhsLiteral.isBool();
    }
    int lhsInt, rhsInt;
    float lhsFloat, rhsFloat;
    bool lhsBool, rhsBool;
    if (isIntOperation) {
        if (lhsLiteral.hasValue()) {
            lhsInt = lhsLiteral.getInt();
        }
        rhsInt = rhsLiteral.getInt();
    } else if (isFloatOperation) {
        if (lhsLiteral.hasValue()) {
            lhsFloat = lhsLiteral.getFloat();
        }
        rhsFloat = rhsLiteral.getFloat();
    } else if (isBoolOperation) {
        if (lhsLiteral.hasValue()) {
            lhsBool = lhsLiteral.getBool();
        }
        rhsBool = rhsLiteral.getBool();
    }
    switch (op) {
        case OperInst::Operator::BitwiseAnd: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt & rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool & rhsBool;
            }
            break;
        }
        case OperInst::Operator::BitwiseOr: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt | rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool | rhsBool;
            }
            break;
        }
        case OperInst::Operator::BitwiseXor: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt ^ rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool ^ rhsBool;
            }
            break;
        }
        case OperInst::Operator::Shl: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt << rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool << rhsBool;
            }
            break;
        }
        case OperInst::Operator::Shr: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt >> rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool >> rhsBool;
            }
            break;
        }
        case OperInst::Operator::Xor: {
            if (isBoolOperation) {
                resultLiteralVal = (bool)(lhsBool ^ rhsBool);
            }
            break;
        }
        case OperInst::Operator::Add: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt + rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat + rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool + rhsBool;
            }
            break;
        }
        case OperInst::Operator::Sub: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt - rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat - rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool - rhsBool;
            }
            break;
        }
        case OperInst::Operator::Mul: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt * rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat * rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool * rhsBool;
            }
            break;
        }
        case OperInst::Operator::Div: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt / rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat / rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool / rhsBool;
            }
            break;
        }
        case OperInst::Operator::Mod: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt % rhsInt;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool % rhsBool;
            }
            break;
        }
        case OperInst::Operator::Eq: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt == rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat == rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool == rhsBool;
            }
            break;
        }
        case OperInst::Operator::Ne: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt != rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat != rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool != rhsBool;
            }
            break;
        }
        case OperInst::Operator::Lt: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt < rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat < rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool < rhsBool;
            }
            break;
        }
        case OperInst::Operator::Ge: {
            if (isIntOperation) {
                resultLiteralVal = lhsInt >= rhsInt;
            } else if (isFloatOperation) {
                resultLiteralVal = lhsFloat >= rhsFloat;
            } else if (isBoolOperation) {
                resultLiteralVal = lhsBool >= rhsBool;
            }
            break;
        }
        case OperInst::Operator::Not: {
            if (isBoolOperation) {
                resultLiteralVal = !rhsBool;
            }
            break;
        }
        case OperInst::Operator::And: {
            if (isBoolOperation) {
                resultLiteralVal = lhsBool && rhsBool;
            }
            break;
        }
        case OperInst::Operator::Or: {
            if (isBoolOperation) {
                resultLiteralVal = lhsBool || rhsBool;
            }
            break;
        }
        default: {
            break;
        }
    }
    return resultLiteralVal;
}
