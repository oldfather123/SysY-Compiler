#include "constReplaceBlock.h"
#include <unordered_map>

using std::unordered_map;

void ConstReplaceBlock(Module &ir)
{
    for (auto &var : ir.globalVariables){
        auto type = var->type;
        bool is_int = type->isInt();
        bool is_float = type->isFloat();
        bool is_bool = type->isBool();
        if(var->type->ID == TypeID::ArrID || var->type->ID == TypeID::PtrID)
            continue;
        

        for (auto &func : ir.globalFunctions){
            if (func->isLibraryFunction) continue;
            unordered_map<BasicBlockPtr, bool> hasStore;
            unordered_map<BasicBlockPtr, bool> isUnprogressable;
            for(auto& bb : func->basicBlocks){
                for (auto &ins : bb->instructions){
                    if(auto Si = dynamic_cast<StoreInstruction*>(ins.get())){
                        if(Si->des == var && !hasStore[bb]){
                            if(is_int){
                                auto int_valptr = dynamic_cast<Int *>(Si->value.get());
                                if(!int_valptr)
                                    continue;
                                bb->propagatedIntValue = (int)int_valptr->intVal;
                                hasStore[bb] = true;
                            }else if(is_float){
                                auto foalt_valptr = dynamic_cast<Float *>(Si->value.get());
                                if(!foalt_valptr)
                                    continue;
                                bb->propagatedFloatValue = (float)foalt_valptr->floatVal;
                                hasStore[bb] = true;
                            }
                        }else if(hasStore[bb]){
                            break;
                        }
                    }
                }
            }
            Use *p = var->useHead;
            while (p){
                auto user = p->user;
                auto bb = user->basicblock;

                if(user->type == InsID::Store || hasStore[bb]){
                    p = p->next;
                    continue;
                }
                bool isSame = true;

                for(auto& predBb : bb->predBasicBlocks){
                    if(is_int)
                        if(predBb->propagatedIntValue != bb->propagatedIntValue){
                            isSame = false;
                            break;
                        }
                    else if(is_float)
                        if(predBb->propagatedFloatValue != bb->propagatedFloatValue){
                            isSame = false;
                            break;
                        }
                }

                if(isSame){
                    cerr << "begin isSame" << endl;
                    if(is_int){
                        auto constVal = Const::createConstant(Type::getInt(), bb->propagatedIntValue);
                        if (user->type == InsID::Load){
                        }else{
                            cerr << "not load global replace" << endl;
                            user->replaceOperand(var, constVal);
                        }
                        p = p->next;
                    }else if(is_float){
                        auto constVal = Const::createConstant(Type::getFloat(), bb->propagatedFloatValue);
                        if (user->type == InsID::Load){
                        }else{
                            cerr << "not load global replace" << endl;
                            user->replaceOperand(var, constVal);
                        }
                        p = p->next;
                    }else 
                        p = p->next;
                }else   
                    p = p->next;
            }                        
        }
    }

}