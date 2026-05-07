// #include "a.hpp"
// #include <unordered_set>
// #include <string>

// APass::APass(Module *m) : Pass(m), m_(m) {}

// // void APass::run()
// // {
// //     for (auto &func : m_->get_functions())
// //     {
// //         for (auto &bb : func.get_basic_blocks())
// //         {
// //             for (auto &instr : bb.get_instructions())
// //             {
// //                 std::cout<<instr.print() <<std::endl;
// //             }
// //         }
// //     }
// // }
// void APass::run()
// {
//     std::unordered_map<GEPKey, Value*, GEPKeyHash> seen;
//     for (auto &func : m_->get_functions())
//     {
//         for (auto &bb : func.get_basic_blocks())
//         {
//             std::vector<Instruction*> to_delete;

//             for (auto &instr : bb.get_instructions())
//             {
//                 if (instr.is_gep())
//                 {
//                     // 构造唯一 key
//                     GEPKey key;
//                     key.base = instr.get_operand(0);
//                     for (size_t i = 1; i < instr.get_num_operand(); ++i) {
//                         key.indices.push_back(instr.get_operand(i));
//                     }

//                     auto it = seen.find(key);
//                     if (it != seen.end())
//                     {
//                         // 替换所有使用这个重复 GEP 的地方
//                         instr.replace_all_use_with(it->second);

//                         // 记录待删除
//                         to_delete.push_back(&instr);
//                     }
//                     else
//                     {
//                         // 记录第一次出现的 GEP
//                         seen[key] = &instr;
//                         // std::cout << "[GEP] base=" << key.base->get_name()
//                         //           << " indices=" << key.indices.size() << "\n";
//                     }
//                 }
//             }
//         }
//     }
// }



// #include "arraypass.hpp"
// #include <unordered_map>
// #include <vector>
// #include <iostream>
// #include <queue>
// #include <algorithm>
// #include <unordered_set>
// #include <unordered_map>

// // using namespace array;

// ArrayPass::ArrayPass(Module *m) : Pass(m), m_(m) {}

// void ArrayPass::run()
// {
//     // 先跑支配树分析
//     // printf("666444\n");
//     dominators_ = std::make_unique<Dominators>(m_);
//     // printf("666444\n");
//     dominators_->run();
//     m_->set_print_name();
//     // 遍历函数
//     // printf("666\n");
//     for (auto &func : m_->get_functions())
//     {   
//         has_store store_blocks;
//         for (auto &bb : func.get_basic_blocks()) {
//             for (auto &instr : bb.get_instructions()) {
//                 if (instr.is_store()) {
//                     Value* store_ptr = instr.get_operand(1);
//                     store_blocks[store_ptr].insert(&bb);
//                     std::cout<<instr.print()<<std::endl;
//                 }
//             }
//         }

//         // print  f("666\n");
//         // 记录：某个 GEPKey 对应的 GEP 指令
//         std::unordered_map<GEPKey, std::vector<Instruction*>, GEPKeyHash> seen;
//         std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//         // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//         // 我们按支配树 DFS 顺序遍历，这样保证父节点支配子节点
//         std::vector<std::pair<Instruction*, Instruction*>> replace_list;
//         std::function<void(BasicBlock*)> process_block =[&](BasicBlock* bb)
//         {
//             // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//             //std::cout << "Processing BasicBlock: " << bb->get_name() << std::endl;
//             for (auto &instr : bb->get_instructions())
//             {
//                 // std::cout << "  Instruction: " << instr.print() << std::endl;
//                 if (instr.is_gep())
//                 {
//                     GEPKey key;
//                     key.base = instr.get_operand(0);
//                     for (size_t i = 1; i < instr.get_num_operand(); ++i)
//                         key.indices.push_back(instr.get_operand(i));
//                     auto &vec = seen[key]; 
//                     bool replaced = false;
//                     for (Instruction* existing : vec) {
//                         if (dominators_->get_idom(instr.get_parent()) != nullptr &&dom_dominates(existing, &instr, *dominators_)) {
//                             instr.replace_all_use_with(existing);
//                             replaced = true;
//                             break;
//                         }
//                     }
//                     if (!replaced) {
//                         vec.push_back(&instr);
//                     }
//                 }
//                 else if (instr.is_load()) {
//                     LoadKey key;
//                     key.ptr = instr.get_operand(0);
//                     auto &vec = seen_loads[key];
//                     bool replaced = false;
//                     for (Instruction* existing : vec) {
//                         if (dominators_->get_idom(instr.get_parent()) != nullptr &&
//                             dom_dominates(existing, &instr, *dominators_)) {
//                             if(!has_intervening_store_cross_blocks(existing, &instr, *dominators_, store_blocks)){
//                                 replace_list.emplace_back(&instr, existing);
//                                 replaced = true;
//                                 break;
//                             }
//                            // instr.replace_all_use_with(existing);
                            
//                         }
//                     }
//                     if (!replaced) {
//                         vec.push_back(&instr);
//                     }
              
    
//                 }else if (instr.is_store()) {
//                     Value *store_ptr = instr.get_operand(1);
//                    // std::cout<<instr.print()<<std::endl;
                   
//                     bool is_use=false;
//                     // 逐个检查是否写到了已缓存的 load 地址
//                     for (auto it = seen_loads.begin(); it != seen_loads.end(); ) {
//                         if (it->first.ptr == store_ptr) {
                        
//                             it = seen_loads.erase(it); // 移除被破坏的缓存
                        
//                         } else {
//                             ++it;
//                         }
//                     }
                   
//                 }
//                 else if (instr.is_call()) {// 这里需要你定义 is_pure 来判断 call 是否无副作用
//                     auto function_names = std::vector<std::string>{
//                         "getint", "getch", "getarray", "getfloat", "getfarray",
//                         "putint", "putch", "putarray", "putfloat", "putfarray",
//                         "putf", "before_main", "after_main", "_sysy_starttime", "_sysy_stoptime","memset"
//                     };
//                     auto f=instr.get_operand(0);
//                     if(std::find(function_names.begin(), function_names.end(), f->get_name()) != function_names.end()){
//                         continue;
//                     }
//                     seen_loads.clear();
//                 }
//             }

//             // 递归支配树的子节点
//             for (auto succ : dominators_->get_dom_tree_succ_blocks(bb))
//                 process_block(succ);
//         };

//         BasicBlock *root = nullptr;
//         // 找到函数入口块（支配树根）
//         if (!func.get_basic_blocks().empty())
//             root = &*func.get_basic_blocks().begin();


//         if (root)
//             process_block(root);
//         int pp=0;
//         for (auto it = replace_list.rbegin(); it != replace_list.rend(); ++it) {
//             Instruction *use = it->first;
//             Instruction *def = it->second;
//             use->replace_all_use_with(def);
//             pp++;
//         }

//         //printf("%d\n",pp);
//     }
// }

// bool ArrayPass::dom_dominates(Instruction *def_instr, Instruction *use_instr, Dominators &dominators)
// {
//     BasicBlock *def_bb = def_instr->get_parent();
//     BasicBlock *use_bb = use_instr->get_parent();

//     if (def_bb == use_bb)
//     {
//         for (auto &inst : def_bb->get_instructions())
//         {
//             if (&inst == def_instr)
//                 return true;  // 先找到 def_instr，说明def在use之前，def支配use
//             if (&inst == use_instr)
//                 return false; // 先遇到use_instr，def在后面，不支配
//         }
//         return false; // 两指令都没找到，保守false
//     }
//     else
//     {
//         // 不在同一个基本块，用支配树判断
//         BasicBlock *bb = use_bb;
//         while (bb != nullptr)
//         {
//             if (bb == def_bb) return true;
//             BasicBlock* next_bb = dominators.get_idom(bb);
//             if (next_bb == bb) {  // 如果 idom 返回自己，跳出循环，避免死循环
//                 break;
//             }
//             bb = next_bb;
//         }
//         return false;
//     }
// }

// bool ArrayPass::has_intervening_store_cross_blocks(Instruction* def, Instruction* use, Dominators& dom, has_store& store_blocks) {
//     Value* ptr = def->get_operand(0);
//     BasicBlock* def_bb = def->get_parent();
//     BasicBlock* use_bb = use->get_parent();
//     // std::cout<<def->print()<<"   "<<use->print()<<std::endl;
//     // 同一基本块内
//     if (def_bb == use_bb) {
//         bool in_range = false;
//         for (auto &instr : def_bb->get_instructions()) {

//             if (&instr == def) {
//                 in_range = true; // 从 def 之后开始
//                 continue;
//             }
//             if (&instr == use) {
//                 break; // 到 use 之前停止
//             }
//             if (in_range && instr.is_store() && instr.get_operand(1) == ptr) {
//                 return true; // 找到干扰的 store
//             }
//         }
//         return false;
//     }
//     std::cout<<"1958  "<<def->print()<<"   "<<use->print()<<std::endl;
//     // 跨块：只检查可能存储 ptr 的块
//     auto it = store_blocks.find(ptr);
//     if (it == store_blocks.end()) return false; // 没有 store 过
//     std::cout<<"1958  "<<def->print()<<"   "<<use->print()<<std::endl;
//     const auto& blocks_with_store = it->second;
//     for (auto* store_bb : blocks_with_store) {
//         // 如果 store_bb 在 def 和 use 之间的路径上（支配 def 到 use 的路径）
//         // 可以用支配树判定：
//         //printf("aaa\n");
//         if (dom.dominates(def_bb, store_bb) || dom.dominates(store_bb, use_bb)) {
//             std::cout<<def_bb->print()<<" "<<def->print()<<"   "<<use_bb->print()<<"  "<<use->print()<<" "<<store_bb->print()<<std::endl;
//             return true; // 中途有 store
//         }
//     }
//     //printf("bbb\n");
//     return false;
// }

#include "arraypass.hpp"
#include <unordered_map>
#include <vector>
#include <iostream>
#include <queue>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

// using namespace array;

ArrayPass::ArrayPass(Module *m) : Pass(m), m_(m) {}

void ArrayPass::run()
{
    // 先跑支配树分析
    // printf("666444\n");
    dominators_ = std::make_unique<Dominators>(m_);
    // printf("666444\n");
    dominators_->run();
  //  m_->set_print_name();
    // 遍历函数
    // printf("666\n");
    for (auto &func : m_->get_functions())
    {   
        
        // print  f("666\n");
        // 记录：某个 GEPKey 对应的 GEP 指令
        std::unordered_map<GEPKey, std::vector<Instruction*>, GEPKeyHash> seen;
        //std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
        // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
        // 我们按支配树 DFS 顺序遍历，这样保证父节点支配子节点
        std::vector<std::pair<Instruction*, Instruction*>> replace_list;
        std::function<void(BasicBlock*)> process_block =[&](BasicBlock* bb)
        {
            // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
            //std::cout << "Processing BasicBlock: " << bb->get_name() << std::endl;
            for (auto &instr : bb->get_instructions())
            {
                // std::cout << "  Instruction: " << instr.print() << std::endl;
                if (instr.is_gep())
                {
                    GEPKey key;
                    key.base = instr.get_operand(0);
                    for (size_t i = 1; i < instr.get_num_operand(); ++i)
                        key.indices.push_back(instr.get_operand(i));
                    auto &vec = seen[key]; 
                    bool replaced = false;
                    for (Instruction* existing : vec) {
                        if (dominators_->get_idom(instr.get_parent()) != nullptr &&dom_dominates(existing, &instr, *dominators_)) {
                            instr.replace_all_use_with(existing);
                            replaced = true;
                            break;
                        }
                    }
                    if (!replaced) {
                        vec.push_back(&instr);
                    }
                }
            }

            // 递归支配树的子节点
            for (auto succ : dominators_->get_dom_tree_succ_blocks(bb))
                process_block(succ);
        };

        BasicBlock *root = nullptr;
        // 找到函数入口块（支配树根）
        if (!func.get_basic_blocks().empty())
            root = &*func.get_basic_blocks().begin();


        if (root)
            process_block(root);
        int pp=0;
        for (auto it = replace_list.rbegin(); it != replace_list.rend(); ++it) {
            Instruction *use = it->first;
            Instruction *def = it->second;
            use->replace_all_use_with(def);
            pp++;
        }

        //printf("%d\n",pp);
    }
    run2();
}

bool ArrayPass::dom_dominates(Instruction *def_instr, Instruction *use_instr, Dominators &dominators)
{
    BasicBlock *def_bb = def_instr->get_parent();
    BasicBlock *use_bb = use_instr->get_parent();

    if (def_bb == use_bb)
    {
        for (auto &inst : def_bb->get_instructions())
        {
            if (&inst == def_instr)
                return true;  // 先找到 def_instr，说明def在use之前，def支配use
            if (&inst == use_instr)
                return false; // 先遇到use_instr，def在后面，不支配
        }
        return false; // 两指令都没找到，保守false
    }
    else
    {
        // 不在同一个基本块，用支配树判断
        BasicBlock *bb = use_bb;
        while (bb != nullptr)
        {
            if (bb == def_bb) return true;
            BasicBlock* next_bb = dominators.get_idom(bb);
            if (next_bb == bb) {  // 如果 idom 返回自己，跳出循环，避免死循环
                break;
            }
            bb = next_bb;
        }
        return false;
    }
}

bool ArrayPass::has_intervening_store_cross_blocks(Instruction* def, Instruction* use, Dominators& dom, has_store& store_blocks) {
    Value* ptr = def->get_operand(0);
    BasicBlock* def_bb = def->get_parent();
    BasicBlock* use_bb = use->get_parent();
    // std::cout<<def->print()<<"   "<<use->print()<<std::endl;
    // 同一基本块内
    if (def_bb == use_bb) {
        bool in_range = false;
        for (auto &instr : def_bb->get_instructions()) {

            if (&instr == def) {
                in_range = true; // 从 def 之后开始
                continue;
            }
            if (&instr == use) {
                break; // 到 use 之前停止
            }
            if (in_range && instr.is_store() && instr.get_operand(1) == ptr) {
                return true; // 找到干扰的 store
            }
        }
        return false;
    }
    // std::cout<<"1958  "<<def->print()<<"   "<<use->print()<<std::endl;
    // 跨块：只检查可能存储 ptr 的块
    auto it = store_blocks.find(ptr);
    if (it == store_blocks.end()) return false; // 没有 store 过
   // std::cout<<"1958  "<<def->print()<<"   "<<use->print()<<std::endl;
    const auto& blocks_with_store = it->second;
    for (auto* store_bb : blocks_with_store) {
        // 如果 store_bb 在 def 和 use 之间的路径上（支配 def 到 use 的路径）
        // 可以用支配树判定：
        //printf("aaa\n");
        if (dom.dominates(def_bb, store_bb) && dom.dominates(store_bb, use_bb)) {
            //std::cout<<def->print()<<"  "<<"  "<<use->print()<<" "<<store_bb->print()<<std::endl;
            // for (auto it = use_bb->get_pre_basic_blocks().begin();it != use_bb->get_pre_basic_blocks().end(); ++it) {
            //     BasicBlock *pred = *it;
            //     for (auto* store_bb : blocks_with_store) {
            //         if (dom.dominates(use_bb, store_bb) && dom.dominates(store_bb, pred)){
            //             return true;
            //         }
            //     }
            // }
            return true; // 中途有 store
        }else{
            std::cout<<def->print()<<"  "<<"  "<<use->print()<<" "<<store_bb->print()<<std::endl;
            for (auto it = use_bb->get_pre_basic_blocks().begin();it != use_bb->get_pre_basic_blocks().end(); ++it) {
                BasicBlock *pred = *it;
                for (auto* store_bb : blocks_with_store) {
                    if (dom.dominates(use_bb, store_bb) && dom.dominates(store_bb, pred)){
                        return true;
                    }
                }
            }
            //return true; // 中途有 store
        }
        // if (dom.dominates(def_bb, store_bb) && dom.dominates(store_bb, use_bb)) {
        //     std::cout<<def_bb->print()<<" "<<def->print()<<"   "<<use_bb->print()<<"  "<<use->print()<<" "<<store_bb->print()<<std::endl;
        //     return true; // 中途有 store
        // }
    }
    //printf("bbb\n");
    return false;
}
void ArrayPass::run2()
{
    // 先跑支配树分析
    // printf("666444\n");
    dominators_ = std::make_unique<Dominators>(m_);
    // printf("666444\n");
    dominators_->run();
   // m_->set_print_name();
    // 遍历函数
    // printf("666\n");
    for (auto &func : m_->get_functions())
    {   
        has_store store_blocks;
        for (auto &bb : func.get_basic_blocks()) {
            for (auto &instr : bb.get_instructions()) {
                if (instr.is_store()) {
                    Value* store_ptr = instr.get_operand(1);
                    store_blocks[store_ptr].insert(&bb);
                    std::cout<<instr.print()<<std::endl;
                }
            }
        }

        // print  f("666\n");
        // 记录：某个 GEPKey 对应的 GEP 指令
        //std::unordered_map<GEPKey, std::vector<Instruction*>, GEPKeyHash> seen;
        std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
        // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
        // 我们按支配树 DFS 顺序遍历，这样保证父节点支配子节点
        std::vector<std::pair<Instruction*, Instruction*>> replace_list;
        std::function<void(BasicBlock*)> process_block =[&](BasicBlock* bb)
        {
            // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
            //std::cout << "Processing BasicBlock: " << bb->get_name() << std::endl;
            for (auto &instr : bb->get_instructions())
            {
                // std::cout << "  Instruction: " << instr.print() << std::endl
                if (instr.is_load()) {
                    LoadKey key;
                    key.ptr = instr.get_operand(0);
                    auto &vec = seen_loads[key];
                    bool replaced = false;
                    for (Instruction* existing : vec) {
                        if (dominators_->get_idom(instr.get_parent()) != nullptr &&
                            dom_dominates(existing, &instr, *dominators_)) {
                            if(!has_intervening_store_cross_blocks(existing, &instr, *dominators_, store_blocks)){
                                replace_list.emplace_back(&instr, existing);
                                replaced = true;
                                break;
                            }
                           // instr.replace_all_use_with(existing);
                            
                        }
                    }
                    if (!replaced) {
                        vec.push_back(&instr);
                    }
              
    
                }else if (instr.is_store()) {
                    Value *store_ptr = instr.get_operand(1);
                   // std::cout<<instr.print()<<std::endl;
                   //Value* store_ptr = instr.get_operand(1);
                    //store_blocks[store_ptr].insert(bb);
                    bool is_use=false;
                    // 逐个检查是否写到了已缓存的 load 地址
                    for (auto it = seen_loads.begin(); it != seen_loads.end(); ) {
                        if (it->first.ptr == store_ptr) {
                        
                            it = seen_loads.erase(it); // 移除被破坏的缓存
                        
                        } else {
                            ++it;
                        }
                    }
                   
                }
                else if (instr.is_call()) {// 这里需要你定义 is_pure 来判断 call 是否无副作用
                    auto function_names = std::vector<std::string>{
                        "getint", "getch", "getarray", "getfloat", "getfarray",
                        "putint", "putch", "putarray", "putfloat", "putfarray",
                        "putf", "before_main", "after_main", "_sysy_starttime", "_sysy_stoptime","memset"
                    };
                    auto f=instr.get_operand(0);
                    if(std::find(function_names.begin(), function_names.end(), f->get_name()) != function_names.end()){
                        continue;
                    }
                    seen_loads.clear();
                }
            }

            // 递归支配树的子节点
            for (auto succ : dominators_->get_dom_tree_succ_blocks(bb))
                process_block(succ);
        };

        BasicBlock *root = nullptr;
        // 找到函数入口块（支配树根）
        if (!func.get_basic_blocks().empty())
            root = &*func.get_basic_blocks().begin();


        if (root)
            process_block(root);
        int pp=0;
        for (auto it = replace_list.rbegin(); it != replace_list.rend(); ++it) {
            Instruction *use = it->first;
            Instruction *def = it->second;
            use->replace_all_use_with(def);
            pp++;
        }

        //printf("%d\n",pp);
    }
}

// bool ArrayPass::has_intervening_store_cross_blocks(Instruction* def, Instruction* use,
//                                         Dominators& dom, Module& m) {
//     BasicBlock* def_bb = def->get_parent();
//     BasicBlock* use_bb = use->get_parent();

//     // 如果 def 和 use 在同一个基本块里，只需要检查 def 到 use 之间的指令
//     if (def_bb == use_bb) {
//         for (auto it = def_bb->get_instructions().begin();
//              it != def_bb->get_instructions().end(); ++it) {
//             if (&*it == def) continue;
//             if (&*it == use) break;
//             if (it->is_store()) {
//                 Value* store_ptr = it->get_operand(1);
//                 if (store_ptr == def->get_operand(0)) {
//                     return true; // 被写过
//                 }
//             }
//         }
//         return false;
//     }

//     // 跨基本块：检查 def_bb 到 use_bb 的所有路径上有没有 store
//     // 可以用 DFS 或 BFS 遍历 def_bb 到 use_bb 的路径
//     std::unordered_set<BasicBlock*> visited;
//     std::function<bool(BasicBlock*)> dfs = [&](BasicBlock* bb) -> bool {
//         if (bb == use_bb) return false;
//         if (!visited.insert(bb).second) return false;

//         for (auto &instr : bb->get_instructions()) {
//             if (instr.is_store()) {
//                 Value* store_ptr = instr.get_operand(1);
//                 if (store_ptr == def->get_operand(0)) {
//                     return true; // 找到中途 store
//                 }
//             }
//         }

//         for (auto succ : bb->get_succ_basic_blocks()) {
//             if (dfs(succ)) return true;
//         }
//         return false;
//     };

//     return dfs(def_bb);
// }





// #include "arraypass.hpp"
// #include <unordered_map>
// #include <vector>
// #include <iostream>
// #include <queue>
// #include <algorithm>
// ArrayPass::ArrayPass(Module *m) : Pass(m), m_(m) {}

// void ArrayPass::run()
// {
//     // 先跑支配树分析
//     // printf("666444\n");
//     dominators_ = std::make_unique<Dominators>(m_);
//     // printf("666444\n");
//     dominators_->run();
//     m_->set_print_name();
//     // 遍历函数
//     // printf("666\n");
//     for (auto &func : m_->get_functions())
//     {   
//         // print  f("666\n");
//         // 记录：某个 GEPKey 对应的 GEP 指令
//         std::unordered_map<GEPKey, std::vector<Instruction*>, GEPKeyHash> seen;
//         // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//         // std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//         // 我们按支配树 DFS 顺序遍历，这样保证父节点支配子节点
//         // std::vector<std::pair<Instruction*, Instruction*>> replace_list;
//         std::function<void(BasicBlock*)> process_block =[&](BasicBlock* bb)
//         {
//             std::unordered_map<LoadKey, std::vector<Instruction*>, LoadKeyHash> seen_loads;
//             // std::cout << "Processing BasicBlock: " << bb->get_name() << std::endl;
//             for (auto &instr : bb->get_instructions())
//             {
//                 // std::cout << "  Instruction: " << instr.print() << std::endl;
//                 if (instr.is_gep())
//                 {
//                     GEPKey key;
//                     key.base = instr.get_operand(0);
//                     for (size_t i = 1; i < instr.get_num_operand(); ++i)
//                         key.indices.push_back(instr.get_operand(i));
//                     auto &vec = seen[key]; 
//                     bool replaced = false;
//                     for (Instruction* existing : vec) {
//                         if (dominators_->get_idom(instr.get_parent()) != nullptr &&dom_dominates(existing, &instr, *dominators_)) {
//                             instr.replace_all_use_with(existing);
//                             replaced = true;
//                             break;
//                         }
//                     }
//                     if (!replaced) {
//                         vec.push_back(&instr);
//                     }
//                 }
//                 else if (instr.is_load()) {
//                     LoadKey key;
//                     key.ptr = instr.get_operand(0);
//                     auto &vec = seen_loads[key];
//                     bool replaced = false;
//                     for (Instruction* existing : vec) {
//                         if (dominators_->get_idom(instr.get_parent()) != nullptr &&
//                             dom_dominates(existing, &instr, *dominators_)) {
//                             // if (!has_intervening_store_cross_blocks(existing, &instr, *dominators_, *m_)){
//                             //     instr.replace_all_use_with(existing);
//                             //     replaced = true;
//                             //     break;
//                             // }
//                             instr.replace_all_use_with(existing);
//                             // replace_list.emplace_back(&instr, existing);
//                             replaced = true;
//                             break;
//                         }
//                     }
//                     if (!replaced) {
//                         vec.push_back(&instr);
//                     }
//                 // else if (instr.is_load()) {
//                 //     LoadKey key;
//                 //     key.ptr = instr.get_operand(0);

//                 //     auto it = seen_loads.find(key);
//                 //     if (it != seen_loads.end()) {
//                 //         Instruction* existing = it->second;
//                 //         instr.replace_all_use_with(existing);
//                 //     } else {
//                 //         seen_loads[key] = &instr;
//                 //     }
    
//                 }else if (instr.is_store()) {
//                     Value *store_ptr = instr.get_operand(1);
//                     // 逐个检查是否写到了已缓存的 load 地址
//                     for (auto it = seen_loads.begin(); it != seen_loads.end(); ) {
//                         if (it->first.ptr == store_ptr) {
//                             it = seen_loads.erase(it); // 移除被破坏的缓存
//                             // for (auto it = replace_list.begin(); it != replace_list.end(); ) {
//                             //     Instruction* load_instr = it->first;
//                             //     if (load_instr->get_operand(0) == store_ptr) { // 或者根据你的 Value* 判断地址是否相同
//                             //         it = replace_list.erase(it); // 取消替换
//                             //     } else {
//                             //         ++it;
//                             //     }
//                             // }
//                             break;
//                         } else {
//                             ++it;
//                         }
//                     }
//                 }
//                 else if (instr.is_call()) {// 这里需要你定义 is_pure 来判断 call 是否无副作用
//                     auto function_names = std::vector<std::string>{
//                         "getint", "getch", "getarray", "getfloat", "getfarray",
//                         "putint", "putch", "putarray", "putfloat", "putfarray",
//                         "putf", "before_main", "after_main", "_sysy_starttime", "_sysy_stoptime","memset"
//                     };
//                     auto f=instr.get_operand(0);
//                     if(std::find(function_names.begin(), function_names.end(), f->get_name()) != function_names.end()){
//                         continue;
//                     }
//                     seen_loads.clear();
//                 }
//             }

//             // 递归支配树的子节点
//             for (auto succ : dominators_->get_dom_tree_succ_blocks(bb))
//                 process_block(succ);
//         };

//         BasicBlock *root = nullptr;
//         // 找到函数入口块（支配树根）
//         if (!func.get_basic_blocks().empty())
//             root = &*func.get_basic_blocks().begin();


//         if (root)
//             process_block(root);
//         // for (auto &p : replace_list) {
//         //     Instruction *use = p.first;
//         //     Instruction *def = p.second;
//         //     use->replace_all_use_with(def);
//         // }

//     }
// }

// bool ArrayPass::dom_dominates(Instruction *def_instr, Instruction *use_instr, Dominators &dominators)
// {
//     BasicBlock *def_bb = def_instr->get_parent();
//     BasicBlock *use_bb = use_instr->get_parent();

//     if (def_bb == use_bb)
//     {
//         for (auto &inst : def_bb->get_instructions())
//         {
//             if (&inst == def_instr)
//                 return true;  // 先找到 def_instr，说明def在use之前，def支配use
//             if (&inst == use_instr)
//                 return false; // 先遇到use_instr，def在后面，不支配
//         }
//         return false; // 两指令都没找到，保守false
//     }
//     else
//     {
//         // 不在同一个基本块，用支配树判断
//         BasicBlock *bb = use_bb;
//         while (bb != nullptr)
//         {
//             if (bb == def_bb) return true;
//             BasicBlock* next_bb = dominators.get_idom(bb);
//             if (next_bb == bb) {  // 如果 idom 返回自己，跳出循环，避免死循环
//                 break;
//             }
//             bb = next_bb;
//         }
//         return false;
//     }
// }