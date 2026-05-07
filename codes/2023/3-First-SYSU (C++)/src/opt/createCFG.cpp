#include "createCFG.h"
//对BasicBlock连边
void addEdgeInCFG(shared_ptr<BasicBlock> pred,shared_ptr<BasicBlock> succ){
    if (pred->succBasicBlocks.find(succ) != pred->succBasicBlocks.end()) {
        cerr<<"error: Adding Exist Edge to CFG\n";
        return;
    }
    if (pred == succ) {
        cerr<<"error: Connect to Itself\n";
        return;
    }
    succ->predBasicBlocks[pred] = true;
    pred->succBasicBlocks[succ] = true;
}



//重新计算CFG，即可能会删掉某些块，需要重新计算CFG
void computeCFG(BlockPtr block) {
    block->getLabelBBMap();
    for(int i=0;i<block->basicBlocks.size();i++){
        if(block->basicBlocks[i]->instructions.back()->type==Br){
            addEdgeInCFG(block->basicBlocks[i],
                        block->LabelBBMap[(dynamic_cast<BrInstruction*>(block->basicBlocks[i]->instructions.back().get()))->label1]);
            if((dynamic_cast<BrInstruction*>(block->basicBlocks[i]->instructions.back().get()))->label2!=nullptr){
                addEdgeInCFG(block->basicBlocks[i],
                        block->LabelBBMap[(dynamic_cast<BrInstruction*>(block->basicBlocks[i]->instructions.back().get()))->label2]);
            }
        }
    }
    // clear previous CFG edges
    // IR->edges.clear();
    // for(auto bb : IR->blocks) {
    //     bb->preds.release();
    //     bb->succs.release();
    // }

    // // handle phi edges
    // for(auto bb : IR->blocks) {
    //     if (auto phi = has_phi(bb)) {
    //         for(auto src : phi->sources) {
    //             CONNECT(src, bb);
    //         }
    //     }
    // }

    // // handle other edges
    // for(auto bb : IR->blocks) {
    //     if (bb->insts.len != 0) {
    //         auto last_inst = bb->insts[bb->insts.len-1];
    //         if (last_inst->type == INST_BRANCH) {
    //             auto br = (Instruction_Branch *)last_inst;
    //             if (!has_phi(br->true_target)) CONNECT(bb, br->true_target);
    //             if (!has_phi(br->false_target)) CONNECT(bb, br->false_target);
    //         } else if (last_inst->type == INST_DIRECT_BRANCH) {
    //             auto br = (Instruction_Direct_Branch *)last_inst;
    //             if (!has_phi(br->target)) CONNECT(bb, br->target);
    //         } else if (last_inst->type == INST_RETURN) {
    //             if (!has_phi(IR->exit_block)) CONNECT(bb, IR->exit_block);
    //         }
    //     }
    // }
}