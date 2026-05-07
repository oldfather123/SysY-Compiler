#include "cse.h"


void cse(BlockPtr block){
    
    for(auto& bb : (block->basicBlocks)){
        unordered_map<InstructionPtr, bool > del;
        for(int i=0;i<bb->instructions.size();i++){
            //此时是ssa，所以不需要考虑值会不会改变
            for(int j=i+1;j<bb->instructions.size();j++){
                if(del.find(bb->instructions[j])== del.end()&& bb->instructions[i]->type == bb->instructions[j]->type){
                    if(bb->instructions[i]->type == Binary){
                        auto Ii = (BinaryInstruction*)(bb->instructions[i].get());
                        auto Ij = (BinaryInstruction*)(bb->instructions[j].get());
                        if(Ii->op!=Ij->op){
                            continue;
                        }
                        // if(Ii->a==Ij->a){
                        //     cerr<<Ii->reg->name<<endl;
                        //     cerr<<Ii->a->isConst<<endl;
                        //     cerr<<Ii->b->isConst<<endl;
                        //     cout<<"error"<<endl;
                        //     Ii->print();
                        // }
                        if(Ii->a==Ij->a&&Ii->b==Ij->b){
                            replaceVarByVar(Ij->reg, Ii->reg);
                            del[bb->instructions[j]] = true;
                        }
                        else if(Ii->a->isConst&&Ij->a->isConst&&Ii->b==Ij->b){
                            auto newA1 = dynamic_cast<Const*>(Ii->a.get());
                            auto newA2 = dynamic_cast<Const*>(Ii->a.get());
                            if(newA1->type->isInt()&&newA2->type->isInt()){
                                if(newA1->intVal ==  newA2->intVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                            else if(newA1->type->isFloat()&&newA2->type->isFloat()){
                                if(newA1->floatVal ==  newA2->floatVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                            else if(newA1->type->isBool()&&newA2->type->isBool()){
                                if(newA1->boolVal ==  newA2->boolVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                        }
                        else if(Ii->b->isConst&&Ij->b->isConst&&Ii->a==Ij->a){
                            auto newB1 = dynamic_cast<Const*>(Ii->b.get());
                            auto newB2 = dynamic_cast<Const*>(Ij->b.get());
                            if(newB1->type->isInt()&&newB2->type->isInt()){
                                if(newB1->intVal ==  newB2->intVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                            else if(newB1->type->isFloat()&&newB2->type->isFloat()){
                                if(newB1->floatVal ==  newB2->floatVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                            else if(newB1->type->isBool()&&newB2->type->isBool()){
                                if(newB1->boolVal ==  newB2->boolVal){
                                    replaceVarByVar(Ij->reg, Ii->reg);
                                    del[bb->instructions[j]] = true;
                                }
                            }
                        }
                        //两个都是常量
                        else{
                        }
                    }
                }
            }
            
            //queue<BasicBlockPtr> pred;
        }
        vector<InstructionPtr > newIns;
        for(int i=0;i<bb->instructions.size();i++){
            if(del.find(bb->instructions[i]) == del.end()){
                newIns.push_back(bb->instructions[i]);
            }
        }
        bb->instructions = newIns;
    }
}