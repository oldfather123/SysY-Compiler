#include <unordered_map>
#include "Function.hpp"
#include "BasicBlock.hpp"
#include "Instruction.hpp"
#include "TailRecursionElimination.hpp"
#include <iostream>

void TailRecursionElimination::run() {
    for (auto &func : m_->get_functions()) {
        if (!func.is_declaration()) {
            eliminate_tail_recursion(&func);
        }
    }
}

bool TailRecursionElimination::eliminate_tail_recursion(Function *f) {
    if (f->get_basic_blocks().size() < 2) {
        return false;
    }
    if(f->get_num_of_args()>30){
        return false;
    }
    auto builder = IRBuilder(nullptr, f->get_parent());
    auto entry_bb = f->get_entry_block();
    BasicBlock *tail_call_bb = nullptr;
    BasicBlock *final_ret_bb = nullptr;
    CallInst *tail_call = nullptr;
    ReturnInst *ret_inst = nullptr;
    //如果有多个尾递归块
    struct TailCallInfo {
        CallInst *call;
        BasicBlock *call_bb;
        std::vector<Value *> args;
    };
    std::vector<TailCallInfo> tail_calls;
    bool is_void=f->get_function_type()->get_return_type()->is_void_type();
    int kk;
    int gg=0;//用来判断void函数是不是尾递归
    bool has_found=0; //void_ret_bb
    // 遍历每个基本块，找出哪个是尾递归调用块，哪个是最终返回块
    for (auto &bb1 : f->get_basic_blocks()) {
        gg=0;
        auto bb = &bb1;
        auto &insts = bb->get_instructions();
        if (insts.empty()) continue;
        TailCallInfo info;
        bool is_tail_call=0;
        // 倒着找 return 前的 call
        for (auto it = insts.rbegin(); it != insts.rend(); ++it) {

            if(gg==1){
                if (auto *call = dynamic_cast<CallInst *>(&*it))
                {
                    if (call->get_operand(0) == f) {
                        tail_call=call;
                        tail_call_bb=bb;

                        
                        info.call = call;
                        info.call_bb = bb;
                        // 保存参数
                        for (int i = 1; i < call->get_num_operand(); ++i) {
                            info.args.push_back(call->get_operand(i));
                        }
                        is_tail_call=1;
                    }
                }else{
                        final_ret_bb=bb;
                        has_found=1;
                }
                break;
            }
            if (auto *ret = dynamic_cast<ReturnInst *>(&*it)) {
                if (ret->get_num_operand() == 1) {
                    auto *retval = ret->get_operand(0);
                    if (auto *call = dynamic_cast<CallInst *>(retval)) {
                        if (call->get_operand(0) == f) {
                            //  尾递归调用块
                            tail_call_bb = bb;
                            tail_call = call;
                            ret_inst = ret;
                            
                            info.call = call;
                            info.call_bb = bb;
                       
                            // 保存参数
                            for (int i = 1; i < call->get_num_operand(); ++i) {
                                info.args.push_back(call->get_operand(i));
                            }
                            is_tail_call=1;
                            break;
                        }
                    } else {
                        //  最终返回块
                        final_ret_bb = bb;
                    }
                }else{
                    gg=1;
                }
                    
            }
        }
        //有一个块只有一个ret的情况
        if(gg==1&&!has_found){
            final_ret_bb=bb;
            has_found=1;
        }
        if(is_tail_call)
        {
            tail_calls.push_back(info);
        }
        

    }
    
    if (is_void)
    {
        if (!tail_call || !final_ret_bb||!tail_call_bb)
        {
           // std::cout<<f->get_name()+"fail"<<std::endl;
            return false;
        }
    }
    else{
        if (!tail_call_bb || !tail_call || !ret_inst || !final_ret_bb)
        {
          //  std::cout<<f->get_name()+"fail"<<std::endl;
            return false;
        }
    }

        
    std::unordered_map<Value *, Value *> store_map; // 记录地址变量和函数参数的对应关系 地址->参数
    std::unordered_map<Value *, Value *> re_store_map; // 记录地址变量和函数参数的对应关系 参数->地址
    std::vector<Instruction *> target_loads;
    std::unordered_map<Value *, Value *> rel_map;   //变量->参数
    int i=0;
    size_t arg_count = f->get_args().size();  
    for (auto &inst : entry_bb->get_instructions()) {
        if(i<arg_count*2){
            if (auto *store = dynamic_cast<StoreInst *>(&inst)) {
            auto *val = store->get_operand(0); // value to store
            auto *ptr = store->get_operand(1); // where to store
            store_map[ptr] = val;
            re_store_map[val] =ptr;
            }
        }
    }
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        for (auto &inst : bb->get_instructions()) {
            if (auto *load = dynamic_cast<LoadInst *>(&inst)) {
                auto *ptr = load->get_operand(0); // ptr from which we load
                if (store_map.count(ptr)) {
                    target_loads.push_back(load);
                    rel_map[load]=store_map[ptr];
                }
            }
        }
    }

    // 创建一个新的 BasicBlock，作为循环跳转目标
    auto loop_bb = insert_after_blockx(entry_bb, f->get_parent());

    //移动指令
    std::vector<Instruction *> to_move;
    i=0;
    for (auto &inst : entry_bb->get_instructions()) {
        if(i < arg_count*2){
            i+=1;
            continue;
        }
        if (inst.isTerminator()) break;
        to_move.push_back(&inst);
    }
    move_instrsx(loop_bb, entry_bb, to_move, loop_bb->get_terminator());


    // 处理尾递归块
    // auto term1 = tail_call_bb->get_terminator();
    // tail_call_bb->erase_instr(term1);
    // tail_call_bb->erase_instr(tail_call);
    // builder.set_insert_point(tail_call_bb);
    // i=0;
    // for (auto &arg : f->get_args()) {
    // Value *val = tail_call_args[i];      // 要存储的值
    // Value *addr = re_store_map[&arg];       // 对应的内存地址（alloca）
    // // 插入 store 指令： store val, addr
    // builder.create_store(val, addr);
    // i++;
    // }
    // builder.create_br(loop_bb);
    for (auto &tc : tail_calls) {
        auto term = tc.call_bb->get_terminator(); // 原结尾指令，如 ret
        tc.call_bb->erase_instr(term);
        tc.call_bb->erase_instr(tc.call);         // 删除尾递归 call 指令
        builder.set_insert_point(tc.call_bb);

        int i = 0;
        for (auto &arg : f->get_args()) {
            Value *val = tc.args[i];           // 要存储的值
            Value *addr = re_store_map[&arg];  // 对应的 alloca 地址
            builder.create_store(val, addr);   // store val -> addr
            i++;
        }

        builder.create_br(loop_bb);            // 跳转回循环基本块
    }

    return true;
}





BasicBlock *insert_before_blockx(BasicBlock *orig_bb, Module *m){
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

BasicBlock *insert_after_blockx(BasicBlock *orig_bb, Module *m) {
    auto func = orig_bb->get_parent();
    auto builder = IRBuilder(nullptr, m);
    auto new_bb = BasicBlock::create(m, "tailrecur.loop.after", func);
    
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
void move_instrsx(BasicBlock *insert_block,BasicBlock *inst_block,
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