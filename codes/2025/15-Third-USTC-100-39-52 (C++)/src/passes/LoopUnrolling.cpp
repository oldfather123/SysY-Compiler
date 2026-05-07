#include <unordered_map>
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "LoopUnrolling.hpp"
#include <iostream>
#include <stack>
#include <tuple>
#include "Instclone.hpp"
#define N 10

void LoopUnrolling::run() {
            Loop_unrolling(m_);
}

bool LoopUnrolling::Loop_unrolling(Module* m) {

    Looplook looplook(m);
    looplook.run();
    //获取所有循环的信息
    new_LoopInfo new_loop_info = looplook.new_get_loopinfo();
    //获取有孩子的父循环的信息,后续不展开父循环
    LoopTree loop_tree=looplook.get_looptree();
    auto builder = IRBuilder(nullptr, m);
    std::unordered_set<BBset_t *> visited;
    std::stack<BBset_t *> worklist;
    int x=0;
    for (const auto &pair : new_loop_info)
    {
        BBset_t *key_ptr = pair.first;
        if(!loop_tree.count(pair.first)){
            worklist.push(key_ptr);x++;
        }
    }
  //  printf("there is %d while\n",worklist.size());
    while(!worklist.empty())
    {
        BBset_t *loop = worklist.top();
        worklist.pop();

        if (!loop || visited.count(loop))
            continue;
        visited.insert(loop);
        auto it = new_loop_info.find(loop);
        auto [first_while_bb,after_end_bb,start_bb,back_bb,update_val,start, end, step, step_num, step_type, icmp_type] = it->second;
        //记录phi语句的信息
        std::unordered_set<Value *>phi_node;
        std::unordered_map<Value *, Value *> phi_map;
        std::unordered_map<Value *, Value *> staic_phi_map;//这个只记录最原始的phi的对应关系，用于后面构建mod_while的时候
        std::unordered_map<Value *, Value *> re_phi_map;

        //这里先只从start_bb里面获取phi的信息
        for (auto &inst : start_bb->get_instructions()) {
            //统计所有的phi指令，后续块复制的时候需要对phi进行特殊处理
            if(auto phi=dynamic_cast<PhiInst *>(&inst)){
                phi_node.insert(phi);
                for(int i=0;i<phi->get_num_operand();i++){
                    if(phi->get_operand(i)==back_bb){
                        phi_map[phi]=phi->get_operand(i-1);
                        staic_phi_map[phi]=phi->get_operand(i-1);
                        re_phi_map[phi->get_operand(i-1)]=phi;
                    }
                }
            }
        }
        std::unordered_map<Value *, int> array_map; //映射关系：数组->每次偏移量需要添加的大小
        //为了实现对于数组的优化，这里先遍历去找可以进行优化的数组
        for (auto *bb : *loop) {
            for (auto &inst : bb->get_instructions()) {
                if (auto *gep = dynamic_cast<GetElementPtrInst*>(&inst)) {
                    bool all_valid = true;
                    int offset=0;
                    for(int i=1;i<gep->get_num_operand();i++){
                        Value *op = gep->get_operand(i);
                        if (auto *c = dynamic_cast<ConstantInt*>(op)) {
                            continue;
                        }
                        else if (op == step_num) {
                            //先计算大小
                            Type *baseType = gep->get_operand(0)->get_type()->get_pointer_element_type();
                            Type *currType = baseType;
                            for (int j = 1; j < i; j++) {
                                    currType = currType->get_array_element_type();
                            }
                            size_t elemSize = currType->get_size();
                            //这里elemSize和步长作用
                            if(icmp_type=="add")
                                offset=offset+ dynamic_cast<ConstantInt*>(step)->get_value()*elemSize;
                            else
                                offset=offset- dynamic_cast<ConstantInt*>(step)->get_value()*elemSize;
                            continue;
                        }
                        else {
                            all_valid = false;
                            break;
                        }
                    }
                    //接下来如果满足要求，就插入array_map
                    if (all_valid) {
                        array_map[gep]=offset;
                    }
                }
            }        
        }
        //处理back_bb  
        auto back_br=back_bb->get_terminator();
        back_bb->erase_instr(back_br);
        start_bb->remove_pre_basic_block(back_bb);
        back_bb->remove_succ_basic_block(start_bb);
        //成堆复制N-1次
        //复制绝对map   最原op->最新op
        std::unordered_map<Value *, Value *> clone_map;
        std::unordered_map<Value *, Value *> old_clone_map;
        std::unordered_map<Value *, Value *> none_map;
        // //复制相对map   所有的:上一次op->最新的op，主要用于更新label(暂时是只用于)  由于普通块都是乱的，所以这个map也要用来更新op了
        // std::unordered_map<Value *, Value *> clone_map;
        Value* final_val;
        Value* final_label;
        //需要操作的块
        BasicBlock* final_bb=nullptr;
        BasicBlock* first_bb=nullptr;
        BasicBlock* operate_bb=back_bb;
        //所有新建的块 
        BBset_t all_new_loop;
        for(int i=0;i<N-1;i++)
        {   
            //进入一个大循环开始前，需要利用phi_map来更新clone_map
            //为了防止phi更新后互相影响，先把clone_map复制一份得到old_clone_map
            if(clone_map.size()>0){
                for(auto &pair:clone_map){
                    Value*key=pair.first;
                    old_clone_map[key]=clone_map[key];
                }
            }
            //此处用old_clone_map来更新，为了防止phi变量之间的影响
            for (auto &pair : phi_map) {
                Value *key = pair.first;
                //r如果出现op1<-op2的情况，就需要把上一轮op2的值给op1,这里需要用到staic_phi_map
                if(staic_phi_map.count(staic_phi_map[key])&&old_clone_map.count(staic_phi_map[key]))//即判断op1对应的op2是一个phi
                    clone_map[key]=old_clone_map[staic_phi_map[key]];//设置op1对应的是上一轮op2对应的op
                else//此为正确情况，op1对应上阶段保留的最新phi.op
                    clone_map[key]=phi_map[key];
            }
            //用来存新的循环块集合
            BBset_t new_loop;
            //最先处理first_while_bb块  但是整个循环体只有一个块的话，first_while_bb就是back_bb，这时不能重复写
            if(first_while_bb!=back_bb){
                auto first_new_bb =BasicBlock::create(m, "", start_bb->get_parent());
                new_loop.insert(first_new_bb);
                all_new_loop.insert(first_new_bb);
                clone_map[first_while_bb]=first_new_bb;
                //现在 operate_bb还是上一个块   operate_bb!=back_bb是后面我会为operate_bb加上br first_bb
                if(operate_bb!=nullptr&&!operate_bb->is_terminated()&&operate_bb!=back_bb){
                    operate_bb->add_succ_basic_block(first_new_bb);
                    first_new_bb->add_pre_basic_block(operate_bb);
                }
                for(auto &inst : first_while_bb->get_instructions()){
                    Instruction* insert_inst;
                    //如果是需要优化的gep,先插入我的mqt指令
                    if(array_map.count(&inst)){
                        auto offset=array_map[&inst]*(i+1);
                        insert_inst=MqtInst::create_mqt(&inst,ConstantInt::get(offset,m),first_new_bb);
                    }
                    else
                        insert_inst=clone_inst(&inst, first_new_bb, none_map);
                    //如果是phi指令，要多处理一步
                    if(inst.is_phi()){
                        first_new_bb->add_instr_begin(insert_inst);
                    }
                    clone_map[dynamic_cast<Value *>(&inst)]=dynamic_cast<Value *>(insert_inst);
                    //如果遇到phi语句有关的更新，要记录
                    if(re_phi_map.count(dynamic_cast<Value *>(&inst))){
                        auto phi=re_phi_map[dynamic_cast<Value *>(&inst)];
                        phi_map[phi]=insert_inst;
                    }    
                }
                operate_bb=first_new_bb; 
                if(i==0)
                    first_bb=first_new_bb;
            }
            
            //接着处理普通块
            for (auto *bb : *loop) {
                if(bb==start_bb||bb==first_while_bb||bb==back_bb)
                    continue;
                auto new_bb =BasicBlock::create(m, "", start_bb->get_parent());
                new_loop.insert(new_bb); 
                all_new_loop.insert(new_bb);
                clone_map[bb]=new_bb;
                //现在 operate_bb还是上一个块
                if(operate_bb!=nullptr&&!operate_bb->is_terminated()&&operate_bb!=back_bb){
                    operate_bb->add_succ_basic_block(new_bb);
                    new_bb->add_pre_basic_block(operate_bb);
                }
                for(auto &inst : bb->get_instructions()){
                    Instruction* insert_inst;
                    //如果是需要优化的gep,先插入我的mqt指令
                    if(array_map.count(&inst)){
                        auto offset=array_map[&inst]*(i+1);
                        insert_inst=MqtInst::create_mqt(&inst,ConstantInt::get(offset,m),new_bb);
                    }
                    else
                    insert_inst=clone_inst(&inst, new_bb, none_map);
                    //如果是phi指令，要多处理一步
                    if(inst.is_phi()){
                        new_bb->add_instr_begin(insert_inst);
                    }
                    clone_map[dynamic_cast<Value *>(&inst)]=dynamic_cast<Value *>(insert_inst);
                    //如果遇到phi语句有关的更新，要记录
                    if(re_phi_map.count(dynamic_cast<Value *>(&inst))){
                        auto phi=re_phi_map[dynamic_cast<Value *>(&inst)];
                        phi_map[phi]=insert_inst;
                    } 
                }
                operate_bb=new_bb; 
            }
            //最后处理最终块，即back_bb，记录最终的i和对应标签用来回写phi，同时建立back的br
            auto back_new_bb =BasicBlock::create(m, "", start_bb->get_parent());
            new_loop.insert(back_new_bb);
            all_new_loop.insert(back_new_bb);
            clone_map[back_bb]=back_new_bb; 
            //现在 operate_bb还是上一个块
            if(operate_bb!=nullptr&&!operate_bb->is_terminated()&&operate_bb!=back_bb){
                operate_bb->add_succ_basic_block(back_new_bb);
                back_new_bb->add_pre_basic_block(operate_bb);
            }
            for(auto &inst : back_bb->get_instructions()){
                Instruction* insert_inst;
                //如果是需要优化的gep,先插入我的mqt指令
                if(array_map.count(&inst)){
                    auto offset=array_map[&inst]*(i+1);
                    insert_inst=MqtInst::create_mqt(&inst,ConstantInt::get(offset,m),back_new_bb);
                }
                else
                    insert_inst=clone_inst(&inst, back_new_bb, none_map);
                //如果是phi指令，要多处理一步
                if(inst.is_phi()){
                    back_new_bb->add_instr_begin(insert_inst);
                }
                clone_map[dynamic_cast<Value *>(&inst)]=dynamic_cast<Value *>(insert_inst);
                //如果遇到phi语句有关的更新，要记录
                if(re_phi_map.count(dynamic_cast<Value *>(&inst))){
                    auto phi=re_phi_map[dynamic_cast<Value *>(&inst)];
                    phi_map[phi]=insert_inst;
                }
            }
            operate_bb=back_new_bb;
            //如果 first_while_bb==back_bb，那么到这里才能记录first_bb
            if(i==0&&first_while_bb==back_bb){
                first_bb=back_new_bb;
            }
            
            //等所有的map对应都建立起来之后，再去替换之前没有来得及替换的label以及op，因为克隆的时候所以的普通块都是乱的
            for (auto *bb : new_loop) {
                for(auto &inst : bb->get_instructions()){
                    //如果是mqt，就跳过
                    if(auto mqt=dynamic_cast<MqtInst*>(&inst))
                        continue;
                    //这是更新br里面的label,因为这个要改前驱和后继
                    if(auto br_inst=dynamic_cast<BranchInst*>(&inst)){
                        //跳转语句br分两种情况，后继跳1个或者2个
                        //同时改br的时候，要把之前br跳转块的前驱和自己的后继改一下
                        if(br_inst->get_num_operand()==3){
                            if(clone_map.count(br_inst->get_operand(0))){
                                br_inst->set_operand(0,clone_map[br_inst->get_operand(0)]);
                            }
                            if(clone_map.count(br_inst->get_operand(1))){
                                
                                //改对应的后继块的前驱，和自己的后继
                                BasicBlock* last_bb=nullptr;
                                for(auto *bb2:bb->get_succ_basic_blocks()){
                                    if(bb2==br_inst->get_operand(1))
                                    {
                                        bb2->remove_pre_basic_block(bb);
                                        last_bb=bb2;
                                        break;
                                    }
                                }
                                dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(1)])->add_pre_basic_block(bb);
                                bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(1)]));
                                bb->remove_succ_basic_block(last_bb);
                                br_inst->set_operand(1,clone_map[br_inst->get_operand(1)]);
                                
                            }
                            if(clone_map.count(br_inst->get_operand(2))){
                                
                                //改对应的后继块的前驱，和自己的后继
                                BasicBlock* last_bb=nullptr;
                                for(auto *bb2:bb->get_succ_basic_blocks()){
                                    if(bb2==br_inst->get_operand(2))
                                    {
                                        bb2->remove_pre_basic_block(bb);
                                        last_bb=bb2;
                                        break;
                                    }
                                }
                                dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(2)])->add_pre_basic_block(bb);
                                bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(2)]));
                                bb->remove_succ_basic_block(last_bb);
                                br_inst->set_operand(2,clone_map[br_inst->get_operand(2)]);
                                
                            }
                        }else{
                            if(clone_map.count(br_inst->get_operand(0))){
                                
                                //改对应的后继块的前驱，和自己的后继
                                BasicBlock* last_bb=nullptr;
                                for(auto *bb2:bb->get_succ_basic_blocks()){
                                    if(bb2==br_inst->get_operand(0))
                                    {
                                        bb2->remove_pre_basic_block(bb);
                                        last_bb=bb2;
                                        break;
                                    }
                                }
                                dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(0)])->add_pre_basic_block(bb);
                                bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(clone_map[br_inst->get_operand(0)]));
                                bb->remove_succ_basic_block(last_bb);
                                br_inst->set_operand(0,clone_map[br_inst->get_operand(0)]);
                                
                            }
                        }
                    }

                    //接下来替换其他指令
                    else{
                        for(int i=0;i<(&inst)->get_num_operand();i=i+1){
                            if(clone_map.count((&inst)->get_operand(i))){
                                (&inst)->set_operand(i,clone_map[(&inst)->get_operand(i)]);
                            }
                        }
                    }
                    
                }
            }
            if(i==N-2){
                //处理最终块的数据
                final_label=operate_bb;
                final_bb=operate_bb;
                builder.set_insert_point(operate_bb);
                builder.create_br(start_bb);
            }

        }
        //最后操作first块(back_bb不一定是最后一个块，这个代码不能去掉)
        builder.set_insert_point(back_bb);
        builder.create_br(first_bb);

        //回改start_bb里面的phi
        for(auto phi:phi_node){
            auto phi_inst=dynamic_cast<PhiInst *>(phi);
            for(int i=0;i<phi_inst->get_num_operand();i++){
                if(phi_inst->get_operand(i)==back_bb){
                    phi_inst->set_operand(i-1,clone_map[phi_inst->get_operand(i-1)]);
                    phi_inst->set_operand(i,final_label);
                }
            }
        }

        //去掉所有的空块以及将所有没有br的块和他的后继合并
        std::unordered_set<BasicBlock*>blocks_to_remove;
        //先收集所有的非终结块
        std::unordered_set<BasicBlock*>unfinal_blocks;
        for(auto *bb:*loop){
            if(!bb->is_terminated())
                unfinal_blocks.insert(bb);
        }
        for(auto *bb:all_new_loop){
            if(!bb->is_terminated())
                unfinal_blocks.insert(bb);
        }
        int x=0;
        while(!unfinal_blocks.empty()){
            auto it = unfinal_blocks.begin(); // 拿到第一个迭代器
            BasicBlock* bb = *it;
            //如果bb没有完结，那么它的后继只有可能是一个
            auto succs = bb->get_succ_basic_blocks(); // 拷贝一份，防迭代器失效
            for(auto bb2:succs){
                //移动指令
                std::vector<Instruction *> to_move;
                if(bb2->get_num_of_instr()==0){
                    break;
                }
                for (auto &inst : bb2->get_instructions()) {
                    if (inst.isTerminator()) break;
                    to_move.push_back(&inst);
                }
                //在把结束语句也加进去
                if(bb2->is_terminated()){
                    to_move.push_back(bb2->get_terminator());
                }
                move_instrsz(bb,bb2, to_move,nullptr);
                //吧bb2的后继都处理一下,同时要处理bb2后继中的phi指令
                replace_phi_predecessor(bb,bb2);
                for(auto bb3:bb2->get_succ_basic_blocks()){
                    bb3->remove_pre_basic_block(bb2);
                    bb3->add_pre_basic_block(bb);
                    bb->add_succ_basic_block(bb3);
                    
                }
                bb->remove_succ_basic_block(bb2);
                if(unfinal_blocks.count(bb2))
                    unfinal_blocks.erase(bb2);
                bb2->erase_from_parent();
            }
            if(bb->is_terminated())
                 unfinal_blocks.erase(bb);
        }

        //把这个park声明在这里，可以分清楚主while和余数while的分界线
        //整一个临时停车场,先用于生成语句并转移
        BasicBlock * park=BasicBlock ::create(m, "park",start_bb->get_parent());

        //接着处理余数mod的while
        //先把整个原循环复制一份，用于后面构造余数的while
        BasicBlock* mod_start_bb;
        BasicBlock* mod_back_bb;
        BBset_t mod_loop;   //存储mod循环中的所有块
        std::unordered_map<Value *, Value *> mod_clone_map;
        std::unordered_map<Value *, Value *> mod_phi_map;//这个用于之后重构mod_while的start_bb里面的phi指令,对应关系为  新phi->原phi 
        //先复制mod_start_bb
        mod_start_bb =BasicBlock::create(m, "", start_bb->get_parent());
        mod_loop.insert(mod_start_bb);
        all_new_loop.insert(mod_start_bb);
        mod_clone_map[start_bb]=mod_start_bb;
        //移植start_bb的时候，要保证phi指令顺序是一致的
        std::stack<Instruction*> phi_stack;
        for(auto &inst :start_bb->get_instructions()){
            //如果是phi指令，先存起来，后面再说
            if(inst.is_phi()){
                phi_stack.push(&inst);
                continue;
            }
            auto insert_inst=clone_inst(&inst,mod_start_bb, mod_clone_map);
            mod_clone_map[dynamic_cast<Value *>(&inst)]=dynamic_cast<Value *>(insert_inst);    
        }
        //再处理phi指令
        while (!phi_stack.empty()) {
            Instruction* phi_inst = phi_stack.top();
            phi_stack.pop();
            auto insert_inst = clone_inst(phi_inst, mod_start_bb, mod_clone_map);
            mod_start_bb->add_instr_begin(insert_inst); 
            mod_phi_map[insert_inst] = phi_inst;
            mod_clone_map[dynamic_cast<Value *>(phi_inst)] = dynamic_cast<Value *>(insert_inst);
        }
        //再复制剩余块
        for (auto *bb : *loop) {
            if(bb==start_bb)
                continue;
            auto new_bb =BasicBlock::create(m, "", start_bb->get_parent());
            mod_loop.insert(new_bb);
            all_new_loop.insert(mod_start_bb); 
            mod_clone_map[bb]=new_bb;
            for(auto &inst : bb->get_instructions()){
                auto insert_inst=clone_inst(&inst, new_bb, mod_clone_map);
                //如果是phi指令，要多处理一步
                if(inst.is_phi()){
                    new_bb->add_instr_begin(insert_inst);
                }
                //由于之前改过back_bb的最后一个跳转br，这里特殊单独处理一下
                if(bb==back_bb&& &inst==bb->get_terminator()){
                    mod_back_bb=new_bb;
                    new_bb->erase_instr(insert_inst);
                    builder.set_insert_point(new_bb);
                    builder.create_br(dynamic_cast<BasicBlock*>(mod_clone_map[start_bb]));
                    first_bb->remove_pre_basic_block(new_bb);
                    new_bb->remove_succ_basic_block(first_bb);
                }
                mod_clone_map[dynamic_cast<Value *>(&inst)]=dynamic_cast<Value *>(insert_inst);
            }
        }
        //等所有块都生成完之后，对所有的label进行替换
        for (auto *bb : mod_loop) {
            for(auto &inst : bb->get_instructions()){
                //先替换br的指令
                if(auto br_inst=dynamic_cast<BranchInst*>(&inst)){
                    //跳转语句br分两种情况，后继跳1个或者2个
                    //同时改br的时候，要把之前br跳转块的前驱和自己的后继改一下
                    if(br_inst->get_num_operand()==3){
                        if(mod_clone_map.count(br_inst->get_operand(0))){
                            br_inst->set_operand(0,br_inst->get_operand(0));
                        }
                        if(mod_clone_map.count(br_inst->get_operand(1))){
                            
                            //改对应的后继块的前驱，和自己的后继
                            BasicBlock* last_bb=nullptr;
                            for(auto *bb2:bb->get_succ_basic_blocks()){
                                if(bb2==br_inst->get_operand(1))
                                {
                                    bb2->remove_pre_basic_block(bb);
                                    last_bb=bb2;
                                    break;
                                }
                            }
                            dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(1)])->add_pre_basic_block(bb);
                            bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(1)]));
                            bb->remove_succ_basic_block(last_bb);
                            br_inst->set_operand(1,mod_clone_map[br_inst->get_operand(1)]);
                            
                        }
                        if(mod_clone_map.count(br_inst->get_operand(2))){
                            
                            //改对应的后继块的前驱，和自己的后继
                            BasicBlock* last_bb=nullptr;
                            for(auto *bb2:bb->get_succ_basic_blocks()){
                                if(bb2==br_inst->get_operand(2))
                                {
                                    bb2->remove_pre_basic_block(bb);
                                    last_bb=bb2;
                                    break;
                                }
                            }
                            dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(2)])->add_pre_basic_block(bb);
                            bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(2)]));
                            bb->remove_succ_basic_block(last_bb);
                            br_inst->set_operand(2,mod_clone_map[br_inst->get_operand(2)]);
                            
                        }
                    }else{
                        if(mod_clone_map.count(br_inst->get_operand(0))){
                            
                            //改对应的后继块的前驱，和自己的后继
                            BasicBlock* last_bb=nullptr;
                            for(auto *bb2:bb->get_succ_basic_blocks()){
                                if(bb2==br_inst->get_operand(0))
                                {
                                    bb2->remove_pre_basic_block(bb);
                                    last_bb=bb2;
                                    break;
                                }
                            }
                            dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(0)])->add_pre_basic_block(bb);
                            bb->add_succ_basic_block(dynamic_cast<BasicBlock*>(mod_clone_map[br_inst->get_operand(0)]));
                            bb->remove_succ_basic_block(last_bb);
                            br_inst->set_operand(0,mod_clone_map[br_inst->get_operand(0)]);
                            
                        }
                    }   
                }
                //再对其余指令进行替换
                else{
                    for(int i=0;i<(&inst)->get_num_operand();i=i+1){
                        if(mod_clone_map.count((&inst)->get_operand(i))){
                            (&inst)->set_operand(i,mod_clone_map[(&inst)->get_operand(i)]);
                        }
                    }
                }
            }
        }
        //再对mod_start_bb中的phi语句进行修改
        for(auto &inst:mod_start_bb->get_instructions()){
            if(inst.is_phi()){
                auto phi_inst=dynamic_cast<PhiInst*>(&inst);
                for(int i=0;i<phi_inst->get_num_operand();i=i+2){
                    //如果原来是final_bb，那应该改成来自自己的back_bb
                    auto ori_phi=mod_phi_map[phi_inst];
                    auto ori_upd_phi=staic_phi_map[ori_phi];
                    if(i==0){
                        phi_inst->set_operand(i,mod_clone_map[ori_upd_phi]);
                        phi_inst->set_operand(i+1,mod_back_bb);
                    }else{
                        phi_inst->set_operand(i,ori_phi);
                        phi_inst->set_operand(i+1,start_bb);
                    }
                }
            }
        }
        //最后，把start_bb往外跳的语句改成往mod_start_bb跳
        auto start_br=start_bb->get_terminator();
        for(int i=0;i<start_br->get_num_operand();i=i+1){
            if(start_br->get_operand(i)==after_end_bb){
                start_br->set_operand(i,mod_start_bb);
                after_end_bb->remove_pre_basic_block(start_bb);
                start_bb->remove_succ_basic_block(after_end_bb);
                start_bb->add_succ_basic_block(mod_start_bb);
                mod_start_bb->add_pre_basic_block(start_bb);
            }
        }
        
        //等mod的while处理完之后，再回去改start_bb的写法
        //修改start_bb  如果i+N<end 就先去跑N次，不行就直接去跑i<end次(这里假设是i++以及<，但是下面具体情况具体判断)
        auto &start_inst_list = start_bb->get_instructions();
        for (auto &inst : start_bb->get_instructions()) {
            if(auto icmp=dynamic_cast<ICmpInst *>(&inst)){
                //这里算出N*步长，再把end-N*步长去替换
                int intend=dynamic_cast<ConstantInt*>(end)->get_value();
                int intstep=dynamic_cast<ConstantInt*>(step)->get_value();
                int finalend;
                if(step_type=="add")
                    finalend=intend-intstep*N;
                else
                    finalend=intend+intstep*N;
                auto const_end = ConstantInt::get(finalend, m);
                //最后修改end
                if(icmp->get_operand(1)==end)
                {
                    icmp->set_operand(1,const_end);
                }else{
                    icmp->set_operand(0,const_end);
                }
            }
        }

        //最终去除掉park
        park->erase_from_parent();

        //这里把原来的loop也加到all_new_loop中，用来完成后面的工作
        for(auto*bb:*loop){
            all_new_loop.insert(bb);
        }
        //最后的最后，对于所有的phi指令，后续使用的时候，换成最新的使用
        for(auto &inst:mod_start_bb->get_instructions()){
            if(inst.is_phi()){
                auto phi=dynamic_cast<PhiInst*>(&inst);
                mod_phi_map[phi]->replace_use_with_if(phi, [&](Use use)
                {
                    auto user_inst = dynamic_cast<Instruction*>(use.val_);
                    if (!user_inst) return false;

                    BasicBlock* bb = user_inst->get_parent();
                    return !all_new_loop.count(bb);
                });

            }
        }


    }
    return 0;
}




BasicBlock *insert_before_blockz(BasicBlock *orig_bb, Module *m){
    auto func = orig_bb->get_parent();
    auto builder = IRBuilder(nullptr, m);
    auto new_bb1 = BasicBlock::create(m, "tailrecur.loop", func);
    for (auto pre_bb : orig_bb->get_pre_basic_blocks()) {
        auto term = pre_bb->get_terminator();
        for (int i = 0; i < term->get_num_operand(); ++i) {
            if (term->get_operand(i) == orig_bb) {
                term->set_operand(i, new_bb1);
            }
        }
        pre_bb->remove_succ_basic_block(orig_bb);
        pre_bb->add_succ_basic_block(new_bb1);
        new_bb1->add_pre_basic_block(pre_bb);
    }
    builder.set_insert_point(new_bb1);
    builder.create_br(orig_bb);
    new_bb1->add_succ_basic_block(orig_bb);
    return new_bb1;
}

BasicBlock *insert_after_blockz(BasicBlock *orig_bb, Module *m) {
    auto func = orig_bb->get_parent();
    auto builder = IRBuilder(nullptr, m);
    auto new_bb = BasicBlock::create(m, "loopunrolling.loop.after", func);
    
    auto term = orig_bb->get_terminator();
    assert(term && "orig_bb must have a terminator");
    if(term->get_num_operand()==3)
    {
        Value *cond = term->get_operand(0);             
        auto true_bb = static_cast<BasicBlock *>(term->get_operand(1));
        auto false_bb = static_cast<BasicBlock *>(term->get_operand(2));
        builder.set_insert_point(new_bb);
        builder.create_cond_br(cond,true_bb,false_bb); 
    }else{
        auto true_bb = static_cast<BasicBlock *>(term->get_operand(0));
        builder.set_insert_point(new_bb);
        builder.create_br(true_bb); 
    }

    orig_bb->erase_instr(term);
    // 遍历原块的所有后继基本块
    auto orig_succs = orig_bb->get_succ_basic_blocks(); 
    for (auto succ_bb : orig_succs) {

        // 更新 CFG 中的后继和前驱关系
        orig_bb->remove_succ_basic_block(succ_bb);
        succ_bb->remove_pre_basic_block(orig_bb);
        // succ_bb->add_pre_basic_block(new_bb);
        
        new_bb->add_succ_basic_block(succ_bb);
    }
    orig_bb->add_succ_basic_block(new_bb);
    // new_bb->add_pre_basic_block(orig_bb);

    builder.set_insert_point(orig_bb);
    builder.create_br(new_bb);


    return new_bb;
}

/**
 *   std::vector<Instruction *> to_move;
 *   for (auto &inst : newblock->get_instructions()) {
 *       if (inst.isTerminator()) break;
 *       to_move.push_back(&inst);
 *   }
 *   move_instrs(target_bb, newblock, to_move, target_bb->get_terminator());
 */
void move_instrsz(BasicBlock *insert_block,BasicBlock *inst_block,
    std::vector<Instruction *> to_move,Instruction *insert_before_inst){
        auto &inst_list = insert_block->get_instructions();
        auto termIt = inst_list.begin();
        for (; termIt != inst_list.end(); ++termIt)
        {
            if (&*termIt == insert_before_inst)
                break;
        }
        
        for (auto *inst : to_move)
        {
            inst_block->get_instructions().remove(inst);
            inst->set_parent(insert_block);
            inst_list.insert_before(termIt, inst);
        }
}

// 处理 bb2 的所有后继块中的 phi 节点，把来源块 bb2 换成 bb1
void replace_phi_predecessor(BasicBlock* bb1, BasicBlock* bb2) {
    // 遍历 bb2 的所有后继
    for (auto succ : bb2->get_succ_basic_blocks()) {
        // 遍历这个后继块中的所有指令
        for (auto &inst : succ->get_instructions()) {
            // 只处理 PHI 节点
            if (inst.is_phi()) {
                PhiInst* phi = static_cast<PhiInst*>(&inst);
                // 遍历 PHI 节点的所有 incoming
                for (unsigned i = 0; i < phi->get_num_operand(); ++i) {
                    if (phi->get_operand(i) == bb2) {
                        phi->replace_phi_operand(phi->get_operand(i-1), phi->get_operand(i-1),bb1);
                        phi->set_operand(i,bb1);
                    }
                }
            } else {
                // PHI 节点必须在基本块开头，遇到非 PHI 就可以退出循环
                break;
            }
        }
    }
}