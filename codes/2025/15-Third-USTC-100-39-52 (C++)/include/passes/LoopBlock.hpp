#pragma once

#include "PassManager.hpp"
#include "Looplook.hpp"
#include "Loopcase.hpp"



using LoopSequence = std::list<std::vector<BBset_t*>>;
using addwhileblock=std::tuple<BasicBlock*,BasicBlock*,BasicBlock*,BasicBlock*>;
using blocks=std::list<addwhileblock>;
class LoopBlock : public Pass {
    public:
        LoopBlock(Module *m) : Pass(m) {}

        void run() override;
        void get_block_info(const LoopTree &loop_tree,
            const LoopArrayAccessInfo &access_info,
            const LoopControl &loop_con,
            LoopSequence &sequence,
            LoopInfo loop_info);
        void block(LoopSequence &loopseq, LoopInfo &loop_info,LoopEnd &loopend);
        void printBBset(const BBset_t* bbset);
        void print_loop_sequence(const LoopSequence& seq);
        void addwhile(BBset_t* whileblock,addwhileblock* addblock);
        BasicBlock* find_loop_header(BBset_t *loop);
        void fix_addwhile(LoopInfo loopinfo, BBset_t* loop, addwhileblock* addblock,std::vector<BBset_t*> loopset,LoopEnd loopend,int size);
        void addwhile(addwhileblock *insertedblock, addwhileblock *addblock);
    };
void pre_succ(BasicBlock* bb);
Value *resolve_loop_start(Value *start, const std::vector<BBset_t *> &loopset, LoopInfo &loopinfo, BBset_t *self_loop);
bool matchnoloop(std::string str);