#include "reg2mem.h"


void reg2mem(BlockPtr block){
    vector<VariablePtr> newAlloca;
    vector<BasicBlockPtr> addBB;
    unordered_map<string,bool> used;

    for(auto &bb:(block->basicBlocks)){
        for(auto &ins:(bb->instructions)){
            if(ins->type == Phi){
                //在entry分配空间
                auto t = (PhiInstruction*)(ins.get());
                VariablePtr alloc;
                if(t->reg->type->isInt()){
                    alloc = VariablePtr(new Int(t->reg->name+".addr", false, false));
                }
                else if(t->reg->type->isFloat()){
                    alloc = VariablePtr(new Float(t->reg->name+".addr", false, false));
                }
                else{
                    alloc = VariablePtr(new Ptr(t->reg->name+".addr", false, false, t->reg->type));
                }
                newAlloca.emplace_back(alloc);
                for(auto &coming:t->from){
                    // if(coming.second->instructions.back()->type==Br){
                    //     auto tempBr = (BrInstruction*)(coming.second->instructions.back().get());
                    //     if(tempBr->label2!=nullptr){
                    //         string newName = coming.second->label->name+"."+bb->label->name+"_crit_edge";
                    //         if(used.find(newName)==used.end()){
                    //             if(tempBr->label2->name == bb->label->name){  
                    //                 used[newName] = true;
                    //                 auto newBB = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(newName))));
                                    
                    //                 coming.second->succBasicBlocks.erase(bb);
                    //                 bb->predBasicBlocks.erase(coming.second);

                    //                 coming.second->succBasicBlocks[newBB] = true;
                    //                 bb->predBasicBlocks[newBB] = true;

                    //                 newBB->pushInstruction(InstructionPtr(new BrInstruction(bb->label, newBB)));
                    //                 newBB->predBasicBlocks[coming.second] = true;
                    //                 newBB->succBasicBlocks[bb] = true;

                    //                 coming.second = newBB;
                    //                 tempBr->label2 = newBB->label;
                    //                 addBB.push_back(newBB);
                    //             }
                    //             else if(tempBr->label1->name == bb->label->name){
                    //                 used[newName] = true;
                    //                 auto newBB = BasicBlockPtr(new BasicBlock(LabelPtr(new Label(newName))));
                                    
                    //                 coming.second->succBasicBlocks.erase(bb);
                    //                 bb->predBasicBlocks.erase(coming.second);

                    //                 coming.second->succBasicBlocks[newBB] = true;
                    //                 bb->predBasicBlocks[newBB] = true;

                    //                 newBB->pushInstruction(InstructionPtr(new BrInstruction(bb->label, newBB)));
                    //                 newBB->predBasicBlocks[coming.second] = true;
                    //                 newBB->succBasicBlocks[bb] = true;

                    //                 coming.second = newBB;
                    //                 tempBr->label1 = newBB->label;
                    //                 addBB.push_back(newBB);
                    //             }
                    //             else{
                    //                 cerr<<bb->label->name<<endl;
                    //                 cerr<<coming.second->label->name<<endl;
                    //                 cerr<<"error\n";
                    //             }
                    //         }
                    //     }
                    // }
                    auto newStore = InstructionPtr(new StoreInstruction(alloc, coming.first, coming.second));
                    coming.second->instructions.emplace(coming.second->instructions.end()-1, newStore);

                    rmInstructionUse(ins, coming.first);
                }
                auto newLoad  = shared_ptr<LoadInstruction>(new LoadInstruction(alloc, t->reg, bb));
                ins = newLoad;
            }
        }
    }
    //在entry分配空间
    auto tmp = block->basicBlocks[0]->instructions;
    block->basicBlocks[0]->instructions.clear();
    for (auto &var : newAlloca) block->basicBlocks[0]->instructions.emplace_back(InstructionPtr(new AllocaInstruction(var, block->basicBlocks[0])));
    for(auto &ins: tmp) block->basicBlocks[0]->instructions.emplace_back(ins);

    //cerr<<addBB.size()<<endl;
    // for(auto &newBB:addBB){
    //     for(auto it = block->basicBlocks.begin();it!=block->basicBlocks.end();it++){
    //         if(newBB->predBasicBlocks.find(*it)!=newBB->predBasicBlocks.end()){
    //             block->basicBlocks.emplace(it+1,newBB);
    //             break;
    //         }
    //     }
    // }
}