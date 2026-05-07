#include "Ast.h"

using namespace ast;

#pragma region Data type casting

ir::Value tryCast(const Ptr<ir_builder::Builder> &builder, const ir::Value &val, const DataType &targetDataType) {
    if (val.isArrayInitializer()) {
        // Array initializer, already cast to target data type
        return val;
    }

    if (val.isLiteral()) {
        return val.getLiteral().cast(targetDataType);
    }

    bool cast;
    auto srcDataType = val.dataType();
    if (srcDataType == targetDataType) {
        cast = false;
    } else if (srcDataType == PrimaryDataType::Int && targetDataType == PrimaryDataType::Float ||
               srcDataType == PrimaryDataType::Float && targetDataType == PrimaryDataType::Int) {
        cast = true;
    } else if (srcDataType == PrimaryDataType::Int && targetDataType == PrimaryDataType::Bool ||
               srcDataType == PrimaryDataType::Bool && targetDataType == PrimaryDataType::Int) {
        cast = true;
    } else if (srcDataType == PrimaryDataType::Float && targetDataType == PrimaryDataType::Bool ||
               srcDataType == PrimaryDataType::Bool && targetDataType == PrimaryDataType::Float) {
        cast = true;
    } else {
        excassert(false, "Cannot cast from " + srcDataType.toString() + " to " + targetDataType.toString());
    }
    if (cast) {
        auto regIR = ir::Register::create(builder->currentFunc(), targetDataType);
        builder->appendInst(ir::CastInst::create(builder->currentBB(), regIR, val));
        return regIR;
    } else {
        return val;
    }
}

#pragma endregion

#pragma region Expr

// bool getMergeVectorValueIR(ir::Value &currentVecVal, DataType currentDataType, DataType mergeDataType) {
//     if (mergeDataType.elemType() == currentDataType.elemType()) {
//         return true;
//     }
//     excassert(!currentDataType.isPrimary(), "Data type mismatch");
//     dbgassert(currentVecVal.isVector(), "Current value is not a vector");

//     if (currentVecVal.getVector().empty()) {
//         currentVecVal.getVector().push_back(ir::Value{Vector<ir::Value>{}, currentDataType.elemType()});
//     }

//     for (auto &elem : currentVecVal.getVector()) {
//         excassert(elem.dataType() == currentDataType.elemType(), "Data type mismatch");
//         ir::Value &availableVecVal = elem;
//         if (getMergeVectorValueIR(availableVecVal, elem.dataType(), mergeDataType)) {
//             currentVecVal = availableVecVal;
//             return true;
//         }
//     }

//     if (currentVecVal.getVector().size() == currentDataType.arraySizes()[0]) {
//         return false; // Current dimension is full, merge vector not found
//     }

//     auto newElem = ir::Value{Vector<ir::Value>{}, currentDataType.elemType()};
//     currentVecVal.getVector().push_back(newElem);
//     ir::Value &availableVecVal = currentVecVal.getVector().back();
//     if (getMergeVectorValueIR(availableVecVal, newElem.dataType(), mergeDataType)) {
//         currentVecVal = availableVecVal;
//         return true;
//     }

//     return false; // Merge vector not found
// }

// bool getMergeVectorValueIR(ir::Value *&resultVecVal, ir::Value *currentVecVal, DataType currentDataType, DataType mergeDataType) {
//     if (mergeDataType.elemType() == currentDataType.elemType()) {
//         resultVecVal = currentVecVal;
//         return true;
//     }
//     excassert(!currentDataType.isPrimary(), "Data type mismatch");
//     dbgassert(currentVecVal->isVector(), "Current value is not a vector");

//     if (currentVecVal->getVector().empty()) {
//         currentVecVal->getVector().push_back(ir::Value{Vector<ir::Value>{}, currentDataType.elemType()});
//     }

//     for (auto &elem : currentVecVal->getVector()) {
//         excassert(elem.dataType() == currentDataType.elemType(), "Data type mismatch");
//         ir::Value *availableVecVal = &elem;
//         if (getMergeVectorValueIR(availableVecVal, availableVecVal, elem.dataType(), mergeDataType)) {
//             resultVecVal = availableVecVal;
//             return true;
//         }
//     }

//     if (currentVecVal->getVector().size() == currentDataType.arraySizes()[0]) {
//         return false; // Current dimension is full, merge vector not found
//     }

//     auto newElem = ir::Value{Vector<ir::Value>{}, currentDataType.elemType()};
//     currentVecVal->getVector().push_back(newElem);
//     ir::Value *availableVecVal = &currentVecVal->getVector().back();
//     if (getMergeVectorValueIR(availableVecVal, availableVecVal, newElem.dataType(), mergeDataType)) {
//         resultVecVal = availableVecVal;
//         return true;
//     }

//     return false; // Merge vector not found
// }

// ir::Value handleMultiInitExpr(ir::Value &resultValIR, InitExpr *initExpr, Ptr<ir_builder::Builder> builder) {
//     if (initExpr->singleInitVal) {
//         return initExpr->rValCodegen(builder);
//     }

//     for (auto elemExpr : initExpr->multiInitVal) {
//         auto elemInitExpr = castPtr<InitExpr>(elemExpr);
//         dbgassert(elemInitExpr, "Invalid initializer expression");

//         elemInitExpr->targetDataType = initExpr->targetDataType.elemType();
//         auto elemValIR = handleMultiInitExpr(resultValIR, initExpr, builder);
//         if (!elemValIR.isVector()) {
//             dbgassert(DataType{initExpr->targetDataType.baseDataType()}.isNumeric(), "Invalid data type");
//             elemValIR = tryCast(builder, elemValIR, initExpr->targetDataType.baseDataType());
//             // Wrap in vector for merging
//             elemValIR = ir::Value{Vector<ir::Value>{elemValIR}, elemValIR.dataType().arrayType()};
//         }
//         ir::Value *mergeVecValIR = nullptr;
//         if (getMergeVectorValueIR(mergeVecValIR, &resultValIR, initExpr->targetDataType, elemValIR.dataType())) {
//             // Merge vectors
//             auto &mergeVector = mergeVecValIR->getVector();
//             for (auto &mergeElem : elemValIR.getVector()) {
//                 if (mergeVector.size() == resultValIR.dataType().arraySizes()[0]) {
//                     // Discard excess elements
//                     break;
//                 }
//                 mergeVector.push_back(mergeElem);
//             }
//         }
//     }
//     return resultValIR;
// }

// bool handleMultiInitExpr(ir::Value &resultValIR, InitExpr *initExpr, Ptr<ir_builder::Builder> builder, Vector<int> &counters) {
//     if (initExpr->singleInitVal) {
//         auto &leafVecVal = resultValIR; // 4 3, counters 0 2
//         for (int i = 0; i < counters.size() - 1; ++i) {
//             leafVecVal = leafVecVal.getVector()[counters[i]]; // 3, counters 0 2
//         }
//         if (counters.back() == resultValIR.dataType().arraySizes().back()) {
//             // Cannot insert
//             return false;
//         }
//         leafVecVal.getVector().push_back(initExpr->rValCodegen(builder));
//         ++counters.back(); // counters 0 3
//         return;
//     } else {
//         for (auto elemExpr : initExpr->multiInitVal) {
//             auto elemInitExpr = castPtr<InitExpr>(elemExpr);
//             dbgassert(elemInitExpr, "Invalid initializer expression");

//             handleMultiInitExpr(resultValIR, elemInitExpr.get(), builder, counters);
//         }
//     }
// }

void handleMultiInitExpr(OrderedMap<int, Ptr<ir::Value>> &linearValueMap, InitExpr *initExpr, Ptr<ir_builder::Builder> builder, int &position) {
    for (auto elem : initExpr->multiInitVal) {
        if (auto elemInitExpr = castPtr<InitExpr>(elem)) {
            elemInitExpr->targetDataType = initExpr->targetDataType;
            if (elemInitExpr->singleInitVal) {
                auto elemValIR = elemInitExpr->targetDataType.isConst() ? elemInitExpr->singleInitVal->getConstVal(builder) : elemInitExpr->singleInitVal->rValCodegen(builder);
                linearValueMap[position++] = makePtr<ir::Value>(tryCast(builder, elemValIR, initExpr->targetDataType.baseDataType()));
            } else {
                int lastPosition = position;
                int tFillSize = 1, fillSize = 1;
                auto arraySizes = elemInitExpr->targetDataType.arraySizes();
                for (int i = arraySizes.size() - 1; i >= 0 && position % tFillSize == 0; tFillSize *= arraySizes[i--]) {
                    fillSize = tFillSize;
                }
                elemInitExpr->targetDataType = initExpr->targetDataType;
                handleMultiInitExpr(linearValueMap, elemInitExpr.get(), builder, position);
                position = lastPosition + fillSize;
            }
        }
    }
}

ir::Value constructArrayInitializer(const Ptr<ir_builder::Builder> &builder, const OrderedMap<int, Ptr<ir::Value>> &linearValueMap, const DataType &dataType) {
    return ir::Value{linearValueMap, dataType};
}

ir::Value InitExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    if (singleInitVal) {
        if (this->targetDataType.isConst()) {
            return singleInitVal->getConstVal(builder);
        } else {
            return singleInitVal->rValCodegen(builder);
        }
    } else {
        // e.g. int c[4][3] = {1, 2, 3, 4, 5, 6, 7} 相当于 {{1, 2, 3}, {4, 5, 6}, {7, 0, 0}}
        // e.g. int d[4][3] = {1, 2, {3, 4}, {5}, 6, 7} 相当于 {{1, 2, 3}, {5, 0, 0}, {6, 7, 0}} 溢出的情况，现在不管了
        // e.g. int g[3][2][2] = {0, 1, 2, 3, {{{4}}}, 5, 6, 7}; 相当于 {{{0, 1}, {2, 3}}, {{4, 0}, {0, 0}}, {{5, 6}, {7, 0}}}
        int position = 0;
        OrderedMap<int, Ptr<ir::Value>> linearValueIRMap{};
        handleMultiInitExpr(linearValueIRMap, this, builder, position);
        return constructArrayInitializer(builder, linearValueIRMap, this->targetDataType);
    }
}

ir::Value InitExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    this->targetDataType = this->targetDataType.constType();
    if (singleInitVal) {
        return singleInitVal->getConstVal(builder);
    } else {
        auto constVal = this->rValCodegen(builder);
        dbgassert(constVal.dataType().isConst(), "Constant value must be constant");
        return constVal;
    }
}

ir::RegPtr LValueExpr::lValCodegen(const Ptr<ir_builder::Builder> &builder) {
    auto declVarIR = builder->lookupVariable(ident);
    this->declVarIR = declVarIR;
    auto addrRegIR = declVarIR->reg();
    if (indices.size() != 0) {
        Vector<ir::Value> indicesIR;
        for (auto &index : this->indices) {
            indicesIR.push_back(tryCast(builder, index->rValCodegen(builder), PrimaryDataType::Int));
        }
        // Calculate offset using getelementptr
        auto &arraySizes = declVarIR->dataType().arraySizes();
        dbgassert(indicesIR.size() <= arraySizes.size(), "Array index out of range");
        auto addrDataType = addrRegIR->dataType();
        for (int i = 0; i < indicesIR.size(); ++i) {
            auto &indexIR = indicesIR[i];
            addrDataType = addrDataType.derefType().elemType().ptrType();
            auto newAddrRegIR = ir::Register::create(builder->currentFunc(), addrDataType);
            builder->appendInst(ir::GEPInst::create(builder->currentBB(), newAddrRegIR, addrRegIR, indexIR));
            addrRegIR = newAddrRegIR;
        }
    }
    dbgassert(addrRegIR->dataType().isPointer(), "Address value is not a pointer");
    dbgassert(addrRegIR->dataType().derefType() == this->getDataType(), "Data type mismatch");
    return addrRegIR;
}

ir::Value LValueExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    auto addrRegIR = this->lValCodegen(builder);
    auto dataType = this->getDataType();
    dbgassert(addrRegIR->dataType().derefType() == dataType, "Data type mismatch");
    auto regIR = ir::Register::create(builder->currentFunc(), dataType);
    if (dataType.isArray()) {
        regIR->dataType() = dataType.ptrType();
        builder->appendInst(ir::MoveInst::create(builder->currentBB(), regIR, addrRegIR));
    } else {
        builder->appendInst(ir::LoadInst::create(builder->currentBB(), regIR, addrRegIR));
    }
    return regIR;
}

ir::Value LValueExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    dbgassert(this->indices.size() == 0, "Cannot get constant value of array access");
    auto regIR = builder->lookupVariable(ident)->reg();
    dbgassert(regIR->dataType().isConst(), "Variable is not constant");
    return regIR->constVal();
}

ir::Value BinaryExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    ir::OperInst::Operator opIR;
    bool exchange = false;
    switch (op) {
        case BinaryOp::Add: {
            opIR = ir::OperInst::Operator::Add;
            break;
        }
        case BinaryOp::Sub: {
            opIR = ir::OperInst::Operator::Sub;
            break;
        }
        case BinaryOp::Mul: {
            opIR = ir::OperInst::Operator::Mul;
            break;
        }
        case BinaryOp::Div: {
            opIR = ir::OperInst::Operator::Div;
            break;
        }
        case BinaryOp::Mod: {
            opIR = ir::OperInst::Operator::Mod;
            break;
        }
        case BinaryOp::Eq: {
            opIR = ir::OperInst::Operator::Eq;
            break;
        }
        case BinaryOp::Neq: {
            opIR = ir::OperInst::Operator::Ne;
            break;
        }
        case BinaryOp::Lt: {
            opIR = ir::OperInst::Operator::Lt;
            break;
        }
        case BinaryOp::Gt: {
            opIR = ir::OperInst::Operator::Lt;
            exchange = true;
            break;
        }
        case BinaryOp::Leq: {
            opIR = ir::OperInst::Operator::Ge;
            exchange = true;
            break;
        }
        case BinaryOp::Geq: {
            opIR = ir::OperInst::Operator::Ge;
            break;
        }
        case BinaryOp::And: {
            opIR = ir::OperInst::Operator::And;
            break;
        }
        case BinaryOp::Or: {
            opIR = ir::OperInst::Operator::Or;
            break;
        }
        default: {
            break;
        }
    }
    if (op == BinaryOp::And || op == BinaryOp::Or) {
        // Short circuit evaluation
        auto rhsEvalBB = builder->newBB(op == BinaryOp::And ? "and.rhs.eval" : "or.rhs.eval");
        auto exitBB = builder->newBB(op == BinaryOp::And ? "and.exit" : "or.exit");
        auto tmpRegIR = ir::Register::create(builder->currentFunc(), DataType{PrimaryDataType::Bool}.ptrType());
        builder->appendInst(ir::AllocInst::create(builder->currentBB(), tmpRegIR));
        auto lhsValIR = tryCast(builder, lhs->rValCodegen(builder), PrimaryDataType::Bool);
        builder->appendInst(ir::StoreInst::create(builder->currentBB(), lhsValIR, tmpRegIR));
        if (op == BinaryOp::And) {
            builder->condBr(lhsValIR, rhsEvalBB, exitBB);
        } else {
            builder->condBr(lhsValIR, exitBB, rhsEvalBB);
        }
        builder->enterBB(rhsEvalBB);
        auto rhsValIR = tryCast(builder, rhs->rValCodegen(builder), PrimaryDataType::Bool);
        builder->appendInst(ir::StoreInst::create(builder->currentBB(), rhsValIR, tmpRegIR));
        builder->uncondBr(exitBB);
        builder->enterBB(exitBB);
        auto regIR = ir::Register::create(builder->currentFunc(), DataType{PrimaryDataType::Bool});
        builder->appendInst(ir::LoadInst::create(builder->currentBB(), regIR, tmpRegIR));
        return regIR;
    } else {
        auto lhsValIR = lhs->rValCodegen(builder);
        auto rhsValIR = rhs->rValCodegen(builder);
        auto operandDataType = ir::getOperandCastTargetDataType(opIR, lhsValIR.dataType(), rhsValIR.dataType());
        lhsValIR = tryCast(builder, lhsValIR, operandDataType);
        rhsValIR = tryCast(builder, rhsValIR, operandDataType);
        auto regIR = ir::Register::create(builder->currentFunc(), this->getDataType());
        auto operInstIR = ir::OperInst::createBinary(builder->currentBB(), regIR, opIR, exchange ? rhsValIR : lhsValIR, exchange ? lhsValIR : rhsValIR);
        builder->appendInst(operInstIR);
        return regIR;
    }
}

ir::Value BinaryExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    auto lhsConstVal = lhs->getConstVal(builder).getLiteral();
    auto rhsConstVal = rhs->getConstVal(builder).getLiteral();
    bool hasFloat = lhsConstVal.dataType() == PrimaryDataType::Float || rhsConstVal.dataType() == PrimaryDataType::Float;
    switch (op) {
        case BinaryOp::Add: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal + (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal + (int)rhsConstVal};
            }
        }
        case BinaryOp::Sub: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal - (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal - (int)rhsConstVal};
            }
        }
        case BinaryOp::Mul: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal * (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal * (int)rhsConstVal};
            }
        }
        case BinaryOp::Div: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal / (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal / (int)rhsConstVal};
            }
        }
        case BinaryOp::Mod: {
            if (hasFloat) {
                dbgassert(false, "Not implemented yet");
                return ir::Literal{0};
            } else {
                return ir::Literal{(int)lhsConstVal % (int)rhsConstVal};
            }
        }
        case BinaryOp::Eq: {
            return ir::Literal{lhsConstVal == rhsConstVal};
        }
        case BinaryOp::Neq: {
            return ir::Literal{lhsConstVal != rhsConstVal};
        }
        case BinaryOp::Lt: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal < (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal < (int)rhsConstVal};
            }
        }
        case BinaryOp::Gt: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal > (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal > (int)rhsConstVal};
            }
        }
        case BinaryOp::Leq: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal <= (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal <= (int)rhsConstVal};
            }
        }
        case BinaryOp::Geq: {
            if (hasFloat) {
                return ir::Literal{(float)lhsConstVal >= (float)rhsConstVal};
            } else {
                return ir::Literal{(int)lhsConstVal >= (int)rhsConstVal};
            }
        }
        case BinaryOp::And: {
            return ir::Literal{(bool)lhsConstVal && (bool)rhsConstVal};
        }
        case BinaryOp::Or: {
            return ir::Literal{(bool)lhsConstVal || (bool)rhsConstVal};
        }
        default: {
            break;
        }
    }
    dbgassert(false, "Invalid binary operation");
    return ir::Literal{};
}

ir::Value UnaryExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    ir::OperInst::Operator opIR;
    bool toBinary = false;
    ir::Literal lhs{}, rhs{};
    auto rhsValIR = operand->rValCodegen(builder);
    switch (op) {
        case UnaryOp::Add: {
            opIR = ir::OperInst::Operator::Add;
            toBinary = true;
            rhs = ir::Literal{rhsValIR.dataType()};
            break;
        }
        case UnaryOp::Sub: {
            opIR = ir::OperInst::Operator::Sub;
            lhs = ir::Literal{rhsValIR.dataType()};
            toBinary = true;
            break;
        }
        case UnaryOp::Not: {
            opIR = ir::OperInst::Operator::Xor;
            toBinary = true;
            rhs = ir::Literal{true};
            break;
        }
    }
    auto dataType = this->getDataType();
    auto operandDataType = ir::getOperandCastTargetDataType(opIR, {}, rhsValIR.dataType());
    rhsValIR = tryCast(builder, rhsValIR, operandDataType);
    auto regIR = ir::Register::create(builder->currentFunc(), dataType);
    auto operInstIR = toBinary
                          ? ir::OperInst::createBinary(builder->currentBB(), regIR, opIR, lhs.hasValue() ? lhs : rhsValIR, rhs.hasValue() ? rhs : rhsValIR)
                          : ir::OperInst::createUnary(builder->currentBB(), regIR, opIR, rhsValIR);
    builder->appendInst(operInstIR);
    return regIR;
}

ir::Value UnaryExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    auto operandConstVal = operand->getConstVal(builder).getLiteral();
    bool hasFloat = operandConstVal.dataType() == PrimaryDataType::Float;
    switch (op) {
        case UnaryOp::Add: {
            return operandConstVal;
        }
        case UnaryOp::Sub: {
            if (hasFloat) {
                return ir::Literal{-(float)operandConstVal};
            } else {
                return ir::Literal{-(int)operandConstVal};
            }
        }
        case UnaryOp::Not: {
            return ir::Literal{!(bool)operandConstVal};
        }
        default: {
            break;
        }
    }
    dbgassert(false, "Invalid unary operation");
    return ir::Literal{};
}

ir::Value IntLiteralExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal{value};
}

ir::Value IntLiteralExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal{value};
}

ir::Value FloatLiteralExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal::ofFloat(text);
}

ir::Value FloatLiteralExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal::ofFloat(text);
}

ir::Value StringLiteralExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal{value};
}

ir::Value StringLiteralExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    return ir::Literal{value};
}

ir::Value CallExpr::rValCodegen(const Ptr<ir_builder::Builder> &builder) {
    auto &functionIR = builder->lookupFunction(callee);
    this->functionIR = functionIR;

    ir::BBPtr callerBB;
    ir::BBPtr callRetBB;

    bool separate = true;
    if (functionIR->isPrototype()) {
        separate = false;
    }

    Vector<ir::Value> argListIR;
    if (functionIR->isPrototype() && (functionIR->name() == "starttime" || functionIR->name() == "stoptime")) {
        argListIR.push_back(ir::Literal{(int)this->line});
    } else {
        for (int i = 0; i < args.size(); ++i) {
            auto &arg = args[i];
            auto argIR = arg->rValCodegen(builder);
            if (!functionIR->isPrototype() || functionIR->paramList().size() != 0 /* Prototype function verifies arguments if parameter data types are assigned */) {
                excassert(i < functionIR->paramList().size(), "Too many arguments for function call");
                auto paramType = functionIR->paramList()[i]->dataType();
                // 如果是基本类型的指针，则解引用
                if (paramType.isPointer() && paramType.derefType().isPrimary()) {
                    paramType = paramType.derefType();
                }
                if (argIR.dataType().isPrimary()) {
                    argIR = tryCast(builder, argIR, paramType);
                }
            }
            argListIR.push_back(argIR);
        }
    }

    auto dataType = this->getDataType();
    auto regIR = ir::Register::create(builder->currentFunc(), dataType);
    auto callInstIR = ir::CallInst::create(builder->currentBB(), regIR, functionIR, argListIR);
    builder->appendInst(callInstIR);

    // if (separate) {
    //     callRetBB = builder->newBB("call.ret");
    //     builder->uncondBr(callRetBB);

    //     builder->enterBB(callRetBB);
    // }

    return regIR;
}

ir::Value CallExpr::getConstVal(const Ptr<ir_builder::Builder> &builder) {
    dbgassert(false, "This should never happen");
    return ir::Literal{};
}

#pragma endregion

#pragma region Stmt

void BlockStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    builder->enterScope(); // Block
    for (auto &stmt : stmts) {
        stmt->codegen(builder);
    }
    builder->exitScope(); // Block
}

void ExprStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    if (!expr) {
        return;
    }
    expr->line = this->line;
    expr->rValCodegen(builder);
}

void arrayInitializer2Store(const Ptr<ir_builder::Builder> &builder, const ir::RegPtr &ptrRegIR, const ir::Value &arrInitValIR) {
    auto &initMap = arrInitValIR.getArrayInitMap();
    for (auto &[position, value] : initMap) {
        Vector<int> indices{};
        int t = position;
        auto arraySizes = arrInitValIR.dataType().arraySizes();
        for (int i = arraySizes.size() - 1; i >= 0; --i) {
            auto dimensionSize = arraySizes[i];
            indices.push_back(t % dimensionSize);
            t /= dimensionSize;
        }
        std::reverse(indices.begin(), indices.end());
        Ptr<ir::Register> elemPtrRegIR = nullptr, tPtrRegIR = ptrRegIR;
        DataType dt = arrInitValIR.dataType();
        for (auto index : indices) {
            dt = dt.elemType();
            elemPtrRegIR = ir::Register::create(builder->currentFunc(), dt.ptrType());
            auto gepInstIR = ir::GEPInst::create(builder->currentBB(), elemPtrRegIR, tPtrRegIR, ir::Literal{index});
            builder->appendInst(gepInstIR);
            tPtrRegIR = elemPtrRegIR;
        }
        builder->appendInst(ir::StoreInst::create(builder->currentBB(), *value, elemPtrRegIR));
    }
}

ir::RegPtr DeclStmt::registerCodegen(const Ptr<ir_builder::Builder> &builder) {
    Vector<int> arraySizes;
    for (auto &size : this->arrayLengths) {
        auto sizeVal = size->getConstVal(builder).getLiteral();
        dbgassert(sizeVal.dataType().isNumeric(), "Array size must be an integer");
        arraySizes.push_back((int)sizeVal);
    }
    this->type = DataType(this->type.baseDataType(), arraySizes, this->type.isConst(), this->type.isPointer(), this->type.is64Bit());
    auto declVarIR = ir_builder::DeclVar::create(this->type, this->identifier, this->isFuncParam);
    builder->declareVariable(declVarIR);
    Ptr<ir::Register> regIR;
    if (declVarIR->isFuncArg() && declVarIR->dataType().isPrimary()) {
        regIR = ir::Register::create(builder->currentFunc(), this->type, this->identifier);
        auto argAddrRegIR = ir::Register::create(builder->currentFunc(), declVarIR->dataType().ptrType(), regIR->name() + ".addr");
        declVarIR->reg() = argAddrRegIR;
        builder->appendInst(ir::AllocInst::create(builder->currentBB(), argAddrRegIR));
        builder->appendInst(ir::StoreInst::create(builder->currentBB(), regIR, argAddrRegIR));
    } else {
        regIR = ir::Register::create(builder->currentFunc(), this->type.ptrType(), this->identifier);
        declVarIR->reg() = regIR;
        if (!declVarIR->isGlobal() && !declVarIR->isFuncArg()) {
            builder->appendInst(ir::AllocInst::create(builder->currentBB(), regIR));
        }
    }
    regIR->isGlobal() = declVarIR->isGlobal();
    excassert(!this->type.isConst() || (this->type.isConst() && initializer != nullptr), "Initializer is required for constant declaration");
    auto variableDataType = declVarIR->dataType();
    if (!declVarIR->isGlobal() && declVarIR->dataType().isArray() && !declVarIR->isFuncArg()) {
        builder->appendInst(ir::CallInst::create(builder->currentBB(), ir::Register::create(builder->currentFunc(), PrimaryDataType::Void), builder->lookupFunction("memset"), Vector<ir::Value>{regIR, ir::Literal{0}, ir::Literal{regIR->dataType().derefType().bytes()}}));
    }
    if (initializer) {
        initializer->targetDataType = variableDataType;
        ir::Value constVal{};
        if (this->type.isConst() || declVarIR->isGlobal()) {
            initializer->targetDataType = initializer->targetDataType.constType();
            constVal = tryCast(builder, initializer->getConstVal(builder), variableDataType);
            if (this->type.isConst()) {
                dbgassert(constVal.dataType().isConst(), "Constant value must be constant");
                regIR->constVal() = constVal;
            }
        }
        if (declVarIR->isGlobal()) {
            dbgassert(constVal.hasValue(), "Constant value is required for global variable initialization");
            builder->appendInst(ir::GlobalInst::create(builder->currentBB(), regIR, constVal));
        } else {
            auto initValIR = tryCast(builder, initializer->rValCodegen(builder), variableDataType);
            dbgassert(regIR->dataType().derefType() == variableDataType, "Data type mismatch");
            if (initValIR.dataType().isArray()) {
                dbgassert(initValIR.isArrayInitializer(), "Array initializer value type mismatch");
                arrayInitializer2Store(builder, regIR, initValIR);
            } else {
                builder->appendInst(ir::StoreInst::create(builder->currentBB(), initValIR, regIR));
            }
        }
    } else if (declVarIR->isGlobal()) {
        ir::Value initValIR{};
        if (variableDataType.isPrimary()) {
            initValIR = ir::Literal{variableDataType};
        } else {
            initValIR = ir::Value{{}, variableDataType.constType()};
        }
        builder->appendInst(ir::GlobalInst::create(builder->currentBB(), regIR, initValIR));
    }
    return regIR;
}

void DeclStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    this->registerCodegen(builder);
}

void AssignStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto destAddrRegIR = this->lvalue->lValCodegen(builder);
    auto destDataType = this->lvalue->getDataType();
    auto srcValIR = tryCast(builder, this->expr->rValCodegen(builder), destDataType);
    dbgassert(destAddrRegIR->dataType().derefType() == destDataType, "Data type mismatch");
    builder->appendInst(ir::StoreInst::create(builder->currentBB(), srcValIR, destAddrRegIR));
}

void IfElseStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto thenBB = builder->newBB("if.then");
    ir::BBPtr elseBB = else_body ? builder->newBB("if.else") : nullptr;
    auto contBB = builder->newBB("if.end");

    auto condValIR = tryCast(builder, condition->rValCodegen(builder), PrimaryDataType::Bool);
    builder->condBr(condValIR, thenBB, else_body ? elseBB : contBB);
    builder->enterBB(thenBB);
    builder->enterScope(); // If-Then
    then_body->codegen(builder);
    builder->uncondBr(contBB);
    builder->exitScope(); // If-Then
    if (else_body) {
        builder->enterBB(elseBB);
        builder->enterScope(); // If-Else
        else_body->codegen(builder);
        builder->uncondBr(contBB);
        builder->exitScope(); // If-Else
    }
    builder->enterBB(contBB);
}

void WhileStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto condBB = builder->newBB("while.header");
    auto loopBB = builder->newBB("while.body");
    auto afterLoopBB = builder->newBB("while.end");

    builder->uncondBr(condBB); // Branch from original BB to condition BB
    builder->enterBB(condBB);
    auto condValIR = tryCast(builder, condition->rValCodegen(builder), PrimaryDataType::Bool);
    builder->condBr(condValIR, loopBB, afterLoopBB);
    builder->enterBB(loopBB);
    builder->enterLoop(condBB, afterLoopBB);
    builder->enterScope(); // While
    body->codegen(builder);
    builder->uncondBr(condBB);
    builder->exitScope(); // While
    builder->exitLoop();
    builder->enterBB(afterLoopBB);
}

void BreakStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto breakTarget = builder->currentBreakTarget();
    dbgassert(breakTarget != nullptr, "Unable to `break` without a loop");
    builder->uncondBr(breakTarget);
    builder->dropBB();
}

void ContinueStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto continueTarget = builder->currentContinueTarget();
    dbgassert(continueTarget != nullptr, "Unable to `continue` without a loop");
    builder->uncondBr(continueTarget);
    builder->dropBB();
}

void ReturnStmt::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto retDataType = builder->currentFunc()->retDataType();
    builder->ret(this->ret_expr ? tryCast(builder, this->ret_expr->rValCodegen(builder), retDataType) : ir::Value(ir::Literal::ofVoid()));
    builder->dropBB();
}

#pragma endregion

#pragma region Function

ir::FuncPtr Function::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto functionIR = ir::Function::create(builder->currentModule(), returnType, identifier);
    builder->declareFunction(functionIR);
    builder->enterFunction(functionIR);

    auto pseudoExitBB = builder->newBB("exit");
    auto pseudoEntryBB = builder->newBB("entry");
    auto entryBB = builder->newBB("start");

    builder->enterScope(); // Function
    functionIR->exitBB() = pseudoExitBB;
    builder->enterBB(pseudoEntryBB);
    Vector<ir::RegPtr> paramListIR;
    int argIndex = 0;
    for (auto &param : params) {
        auto paramIR = param->registerCodegen(builder);
        paramIR->argIndex() = argIndex;
        paramListIR.push_back(paramIR);
        ++argIndex;
    }
    functionIR->paramList() = paramListIR;
    builder->uncondBr(entryBB);
    builder->enterBB(entryBB);
    body->codegen(builder);
    if (builder->currentBB()) {
        builder->ret(ir::Value(ir::Literal{functionIR->retDataType()}));
    }
    builder->enterBB(pseudoExitBB);
    builder->exitScope(); // Function

    builder->exitFunction();

    functionIR->entryBB() = pseudoEntryBB;
    return functionIR;
}

#pragma endregion

#pragma region CompUnit

Ptr<ir::Module> CompUnit::codegen(const Ptr<ir_builder::Builder> &builder) {
    auto moduleIR = ir::Module::create();
    builder->enterModule(moduleIR);
    builder->enterScope(); // Module

    // Declare runtime library functions
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Int, "getint", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Int, "getch", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Float, "getfloat", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Int, "getarray", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Int, "getfarray", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putint", {PrimaryDataType::Int}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putch", {PrimaryDataType::Int}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putfloat", {PrimaryDataType::Float}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putarray", {}));  //, {PrimaryDataType::Int, DataType{PrimaryDataType::Int}.arrayType()}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putfarray", {})); //, {PrimaryDataType::Int, DataType{PrimaryDataType::Float}.arrayType()}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "putf", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "starttime", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "stoptime", {}));
    builder->declareFunction(ir::Function::createPrototype(moduleIR, PrimaryDataType::Void, "memset", {}));

    // Generate global init pseudo function IR
    auto globalInitFuncIR = ir::Function::create(nullptr, PrimaryDataType::Void, "__init__");
    builder->setGlobalInitFunc(globalInitFuncIR);
    builder->enterFunction(globalInitFuncIR);
    auto globalBB = builder->newBB("global");
    builder->enterBB(globalBB);
    for (auto &child : this->children) {
        if (auto stmt = castPtr<Stmt>(child)) {
            stmt->codegen(builder);
        }
    }
    builder->exitFunction();
    globalInitFuncIR->entryBB() = globalBB;

    // Generate functions IR
    for (auto &child : this->children) {
        if (auto func = castPtr<Function>(child)) {
            func->codegen(builder);
        }
    }

    builder->exitScope(); // Module
    return moduleIR;
}

#pragma endregion
