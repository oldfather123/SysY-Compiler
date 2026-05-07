#include "SimplifyInstructions.h"

ValuePtr simplifyInstruction(Instruction *instr){
    if (instr->type == Binary || instr->type == Icmp || instr->type == Fcmp)
        return simplifyInstructionBinary(instr);

    else if (auto phi = dynamic_cast<PhiInstruction *>(instr))
        return simplifyInstructionPhi(phi);

    else if (auto fneg = dynamic_cast<FnegInstruction *>(instr))
        return simplifyInstructionFneg(fneg);

    return instr->reg;
}

bool isZero(Const* val, bool isInt, bool isFloat, bool isBool){
    return (isInt && val->intVal == 0) || (isFloat && val->floatVal == 0) || (isBool && val->boolVal == false);
}

bool isOne(Const* val, bool isInt, bool isFloat, bool isBool){
    return (isInt && val->intVal == 1) || (isFloat && val->floatVal == 1.0) || (isBool && val->boolVal == true);
}

ValuePtr simplifyInstructionBinary(Instruction *instr){
    switch(instr->type){
        case InsID::Binary :{
            auto bin = dynamic_cast<BinaryInstruction *>(instr);
            auto lhs = bin->a;
            auto rhs = bin->b;
            auto clhs = dynamic_cast<Const *>(lhs.get()); 
            auto crhs = dynamic_cast<Const *>(rhs.get());
            // maybe wrong
            BinaryInstruction* srhs = nullptr;
            BinaryInstruction* slhs = nullptr;
            if(lhs->I && rhs->I){
                srhs = dynamic_cast<BinaryInstruction *>(rhs->I);
                slhs = dynamic_cast<BinaryInstruction *>(lhs->I);
            }
            bool lhsIsConst = (clhs != nullptr);
            bool rhsIsConst = (crhs != nullptr);
            bool insIsInt = bin->a->type->isInt();
            bool insIsFloat = bin->a->type->isFloat();
            bool insIsBool = bin->a->type->isBool();
            if(lhsIsConst || rhsIsConst){
                switch (bin->op){
                    case '+':{
                        if (lhsIsConst){
                            bin->swapOperands();
                            lhs = bin->a;
                            rhs = bin->b;
                        }
                        // X + 0 = 0
                        if(crhs){
                            if(isZero(crhs, insIsInt, insIsFloat, insIsBool)){
                                return lhs;
                            }
                            else if(insIsBool && crhs->boolVal == true){
                                return  Const::createConstant(Type::getBool(), true);
                            }
                        }
                        break;
                    }
                    case '-':{                
                        // X - 0 = X
                        if (crhs && isZero(crhs, insIsInt, insIsFloat, insIsBool)){
                            return lhs;
                        }
                        // 0 - (0 - X) = X
                        else if (lhsIsConst){
                            if (clhs && isZero(clhs, insIsInt, insIsFloat, insIsBool)){
                                if (srhs && srhs->getOpcode() == Sub){      
                                    auto clhsofRhs = dynamic_cast<Const *>(srhs->a.get());                          
                                    if (clhsofRhs && isZero(clhsofRhs, insIsInt, insIsFloat, insIsBool))
                                        return srhs->b;
                                    }
                            }
                        }

                        break;
                    }
                    case '*':{
                        if (lhsIsConst){
                            bin->swapOperands();
                            lhs = bin->a;
                            rhs = bin->b;
                        }
                        // X * 0 = 0 X * 1 = 1
                        if (crhs){
                            if (isZero(crhs, insIsInt, insIsFloat, insIsBool)){
                                if(insIsFloat){
                                    return Const::createConstant(Type::getFloat(), float(0));
                                }
                                else if(insIsBool){
                                    return Const::createConstant(Type::getBool(), false);
                                }else{
                                    return Const::createConstant(Type::getInt(), int(0));
                                }
                            }
                            else if (isOne(crhs, insIsInt, insIsFloat, insIsBool))
                                return lhs;
                        }
                        break;
                    }
                    case '/':{
                        // X / 1 = X
                            if(rhsIsConst && isOne(crhs, insIsInt, insIsFloat, insIsBool)){
                                return lhs;
                            }
                            if(lhsIsConst && isZero(clhs, insIsInt, insIsFloat, insIsBool)){
                                if(insIsFloat){
                                    return Const::createConstant(Type::getFloat(), float(0));
                                }
                                else if(insIsBool){
                                    return Const::createConstant(Type::getBool(), false);
                                }else{
                                    return Const::createConstant(Type::getInt(), int(0));
                                }
                            }
                        break;
                    }
                    default:
                        break;
                }
            }else{
                switch(bin->op){
                    case '+':{
                        if(!slhs && !srhs)
                            return instr->reg;
                        if(slhs && srhs){
                            // (x - y) + (y - x) = 0
                            if(slhs->getOpcode() == Sub && srhs->getOpcode() == Sub) 
                                if (slhs->a == srhs->b && slhs->b == srhs->a){
                                    if(insIsFloat){
                                        return Const::createConstant(Type::getFloat(), float(0));
                                    }
                                    else if(insIsBool){
                                        return Const::createConstant(Type::getBool(), false);
                                    }else{
                                        return Const::createConstant(Type::getInt(), int(0));
                                    }
                                }
                        }else if(slhs){
                            // X + (0 - X) = 0, (0 - X) + X = 0
                            if (slhs->getOpcode() == Sub){
                                auto cSubslhs = dynamic_cast<Const *>(slhs->a.get());
                                if (cSubslhs != nullptr && isZero(cSubslhs, insIsInt, insIsFloat, insIsBool) && slhs->b == rhs){
                                    if(insIsFloat){
                                        return Const::createConstant(Type::getFloat(), float(0));
                                    }
                                    else if(insIsBool){
                                        return Const::createConstant(Type::getBool(), false);
                                    }else{
                                        return Const::createConstant(Type::getInt(), float(0));
                                    }
                                }
                            }
                        }else if(srhs){
                            // X + (0 - X) = 0, (0 - X) + X = 0
                            if (srhs->getOpcode() == Sub){
                                auto clhsofRhs = dynamic_cast<Const *>(srhs->a.get());
                                if (clhsofRhs != nullptr && isZero(clhsofRhs, insIsInt, insIsFloat, insIsBool) && srhs->b == lhs){
                                    if(insIsFloat){
                                        return Const::createConstant(Type::getFloat(), float(0));
                                    }
                                    else if(insIsBool){
                                        return Const::createConstant(Type::getBool(), false);
                                    }else{
                                        return Const::createConstant(Type::getInt(),int(0));
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case '-':{
                        // X - X = 0
                        if (lhs == rhs){
                            if(insIsFloat){
                                return Const::createConstant(Type::getFloat(), 0);
                            }
                            else if(insIsBool){
                                return Const::createConstant(Type::getBool(), false);
                            }else{
                                return Const::createConstant(Type::getInt(), 0);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        case InsID::Icmp:{
            auto bin = dynamic_cast<IcmpInstruction *>(instr);
            if (bin->op == "==" && bin->a == bin->b)
                return Const::createConstant(Type::getBool(), true);
            break;
        }
        case InsID::Fcmp:{
            auto bin = dynamic_cast<FcmpInstruction *>(instr);
            if (bin->op == "==" && bin->a == bin->b)
                return Const::createConstant(Type::getBool(), true);
            break;
        }
        default:
            break;
    }
    return instr->reg;
}

ValuePtr simplifyInstructionPhi(PhiInstruction *instr){
    auto first = instr->from[0].first;
    for (auto& phiVal : instr->from)
        if (phiVal.first != first)
            return instr->reg;
    return first;
}

ValuePtr simplifyInstructionFneg(FnegInstruction *instr)
{
    auto bin = new BinaryInstruction(Const::createConstant(Type::getFloat(), 0.0f), instr->reg, '-', instr->basicblock);
    return bin->reg;
}