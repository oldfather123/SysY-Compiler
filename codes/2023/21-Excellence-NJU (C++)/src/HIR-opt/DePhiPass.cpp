//
// Created by q1w2e3r4 on 23-8-8.
//

#include "HIR-opt/DePhiPass.h"
#include "IRInstruction.h"
#include <vector>
using namespace std;

void DePhiPass::run() {
    for(auto& [name,func]:gu->func_table){
        vector<BasicBlock*> to_push;
        map<pair<BasicBlock*,BasicBlock*>,BasicBlock*> mapper;
        for(auto now:func->block_list){
            vector<PhiInstruction*> to_del;
            for(auto instr:now->local_instr){
                if(instr->instType != PHI) break;
                auto phi = dynamic_cast<PhiInstruction*>(instr);
                to_del.push_back(phi);

                ValueRef* dst = phi->result;


                for(auto &[block,ref] : phi->mp){
                    auto jump = (*(block->local_instr.end() - 1));
                    if(jump->instType == BR) continue;
                    if(mapper.count({block,now})) continue;

                    string Bname = "PHI";
                    BasicBlock * newBlock = new BasicBlock(Bname,func);
                    jump->replace(now,newBlock);
                    // TODO:这样写是不对的，只修改跳转而不修改pred和succ;
                    IRInstruction* br1 = new BrInstruction(now);
                    newBlock->appendCode(br1);

                    newBlock->pred.push_back(block);
                    newBlock->succ.push_back(now);
                    auto &v1 = block->succ;
                    for(int i=0;i<v1.size();++i){
                        if(v1[i] == now)
                            v1[i] = newBlock;
                    }

                    auto &v2 = now->pred;
                    for(int i=0;i<v2.size();++i){
                        if(v2[i] == block)
                            v2[i] = newBlock;
                    }

                    // cerr << block->label << "<-" << newBlock->label << "<-" << now->label << endl;
                    mapper[{block,now}] = newBlock;
                    to_push.push_back(newBlock);
                }

                for(auto &[block,ref] : phi->mp){
                    if(ref->type == Undefined) continue; // TODO：这种情况在DCE之后能解决，现在先占位
                    IRInstruction* move = new MoveInstruction(dst,ref); //dst <- ref;
                    BasicBlock * prev;
                    if(mapper.count({block,now})){
                        prev = mapper[{block,now}];
                    }
                    else{
                        prev = block;
                    }
                    Insert_instr_beforeBr(move,prev);
                }
            }

            for(auto phi:to_del){
                DeleteInstruction(phi);
            }
        }
        for(auto block:to_push){
            func->block_list.push_back(block);
        }
    }
}


