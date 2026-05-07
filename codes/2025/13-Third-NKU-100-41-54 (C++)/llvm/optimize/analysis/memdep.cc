#include "memdep.h"
#include "../../../include/ir.h"
#include <assert.h>
#include <queue>
#include "dominator_tree.h"
#include <array> 
#include <algorithm>
std::unordered_map<std::string, CFG*> cfgTable;
//extern DomAnalysis *domtrees;

LLVMBlock GetRetBlock(CFG* C)
{
    int size=C->block_map->size();
    for(auto [id,bb]:*C->block_map)
    {
        for(auto ins:bb->Instruction_list)
        {
            if(ins->GetOpcode() == BasicInstruction::LLVMIROpcode::RET)
            {
                return bb;
            }
        }
    }
    return nullptr;
}

//获取LOAD指令的最近存储依赖->确定load读取的值由哪个store指令写入
std::set<Instruction> SimpleMemDepAnalyser::GetLoadClobbers(Instruction I, CFG *C) {
    std::set<Instruction> res;//大多数时候res中有且只有一条指令，有时候可能还有首块的函数定义指令
    //1.获取load指令的读取地址
    Operand ptr;
    assert(I->GetOpcode() == BasicInstruction::LOAD || I->GetOpcode() == BasicInstruction::STORE);
    if (I->GetOpcode() == BasicInstruction::LOAD) {
        ptr = ((LoadInstruction *)I)->GetPointer();
    } else if (I->GetOpcode() == BasicInstruction::STORE) {
        ptr = ((StoreInstruction *)I)->GetPointer();
    }
    //I->PrintIR(std::cerr);
    //2.在load指令所在基本块中反向遍历
    // first search all the Instructions before I in I's block
    auto IBB = (*C->block_map)[I->GetBlockID()];
    //std::cout<<"load_block_id= "<<I->GetBlockID()<<"\n";
    
    int Iindex = -1;
    //std::cerr<<"IBB_size()="<<IBB->Instruction_list.size()<<"\n";
    for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
        
        //3.找到load指令所在位置
        auto tmpI = IBB->Instruction_list[i];
        //tmpI->PrintIR(std::cerr);
        if (tmpI == I) {
            Iindex = i;
            break;
        }
    }
    if(Iindex==-1){return res;}
    assert(Iindex != -1);
    //4.从该load指令开始向前遍历
    for (int i = Iindex; i >= 0; --i) {
        auto tmpI = IBB->Instruction_list[i];
        //5.处理load指令前面的store指令与call指令（都可能把值存入该处内存）
        if (tmpI->GetOpcode() == BasicInstruction::STORE) {
            auto StoreI = (StoreInstruction *)tmpI;
            //6.如果store指令处理的内存地址一定与该load指令相同，则将store指令加入res集合
            if (alias_analyser->QueryAlias(ptr, StoreI->GetPointer(), C) == AliasStatus::MustAlias) {
                res.insert(StoreI);
                return res;
            }
        } else if (tmpI->GetOpcode() == BasicInstruction::CALL) {
            auto CallI = (CallInstruction *)tmpI;
            auto call_name = CallI->GetFunctionName();
            //7.如果是外部调用，则将call指令直接加入res,并返回res(不再遍历)（？）
            if (cfgTable.find(call_name) == cfgTable.end()) {    // external call
                res.insert(CallI);
                return res;
            }
            //8.如果是正常函数调用，且该函数调用中没有写内存，则将call指令直接加入res,并不再遍历（？）
            auto target_cfg = cfgTable[call_name];
            auto& rwmap = alias_analyser->GetRWMap();
            auto it = rwmap.find(target_cfg);
            assert(it != rwmap.end());
            assert(target_cfg!=nullptr);
            if(alias_analyser->HasSideEffect(target_cfg))
            {
                res.insert(CallI);
                return res;
            }
            // const RWInfo& rwinfo = it->second;
            // if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) 
            // {    // may def memory
            //     res.insert(CallI);
            //     return res;
            // }
        }
    }
    //9.获取当前块的所有前驱块
    std::queue<LLVMBlock> q;
    for (auto bb : C->GetPredecessor(IBB)) {
        q.push(bb);
    }
    //10.如果当前块为首块且无前驱块，将函数定义指令加入res,直接返回（？）
    if (IBB->block_id == 0 && q.empty()) {
        res.insert(C->function_def);
        return res;
    }
    //11.按BFS反向遍历前驱块
    // then BFS the CFG in reverse order
    std::map<LLVMBlock, bool> vis_map;
    while (!q.empty()) {
        auto x = q.front();
        q.pop();
        if (vis_map[x]) {
            continue;
        }
        vis_map[x] = true;

        bool is_find = false;
        //12.反向遍历块中的指令，处理store指令和call指令
        for (int i = x->Instruction_list.size() - 1; i >= 0; --i) {
            auto tmpI = x->Instruction_list[i];
            if (tmpI->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)tmpI;
                if (alias_analyser->QueryAlias(ptr, StoreI->GetPointer(), C) == AliasStatus::MustAlias) {
                    res.insert(StoreI);
                    is_find = true;
                    break;
                }
            } else if (tmpI->GetOpcode() == BasicInstruction::CALL) {
                auto CallI = (CallInstruction *)tmpI;
                auto call_name = CallI->GetFunctionName();

                if (cfgTable.find(call_name) == cfgTable.end()) {    // external call
                    res.insert(CallI);
                    is_find = true;
                    break;
                }

                auto target_cfg = cfgTable[call_name];
                auto& rwmap = alias_analyser->GetRWMap();
                auto it = rwmap.find(target_cfg);
                assert(it != rwmap.end());
                if(alias_analyser->HasSideEffect(target_cfg))
                {
                    res.insert(CallI);
                    is_find = true;
                    break;
                }
                // const RWInfo& rwinfo = it->second;
                // if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) 
                // {    // may def memory
                //     res.insert(CallI);
                //     is_find = true;
                //     break;
                // }
            }
        }
        //13.如果还没有找到最近的内存存值指令，继续压入当前块的前驱块
        if (!is_find) {
            for (auto bb : C->GetPredecessor(x)) {
                q.push(bb);
            }
        }
        //14.如果抵达第一个块，插入函数定义指令
        // if reach BB0,  insert functiondef
        if (x->block_id == 0 && !is_find) {
            res.insert(C->function_def);
        }
    }

    return res;
}

bool SimpleMemDepAnalyser::IsExternalCallReadPtr(CallInstruction *I, Operand ptr, CFG *C) {
    auto n = I->GetFunctionName();
    
    // 所有直接返回false的函数名数组
    static constexpr std::array<const char*, 11> falseFuncs = {
        "getint", "getch", "getfloat", 
        "getarray", "getfarray", 
        "putint", "putch", "putfloat", 
        "_sysy_starttime", "_sysy_stoptime", 
        "llvm.memset.p0.i32"
    };
    
    // 检查是否在直接返回false的数组中
    if (std::find(falseFuncs.begin(), falseFuncs.end(), n) != falseFuncs.end()) {
        return false;
    }
    
    // 特殊处理需要检查别名的函数
    if (n == "putarray" || n == "putfarray") {
        assert(I->GetParameterList().size() == 2);
        auto arg2 = I->GetParameterList()[1].second;
        assert(arg2->GetOperandType() == BasicOperand::REG);
        auto res = alias_analyser->QueryAlias(ptr, arg2, C);
        return (res == AliasStatus::MustAlias);
    }
    
    return true;
}
//获取store指令的后续使用者
std::set<Instruction> SimpleMemDepAnalyser::GetStorePostClobbers(Instruction I, CFG *C) {
    //1.获取该store指令对应的内存指针、基本块位置
    std::set<Instruction> res;

    assert(I->GetOpcode() == BasicInstruction::STORE);
    Operand ptr = ((StoreInstruction *)I)->GetPointer();

    // first search all the Instructions before I in I's block
    auto IBB = (*C->block_map)[I->GetBlockID()];
    //2.逆序找到store指令所在位置
    int Iindex = -1;
    for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
        auto tmpI = IBB->Instruction_list[i];
        if (tmpI == I) {
            Iindex = i;
            break;
        }
    }
    assert(Iindex != -1);
    //3.从store指令开始往后遍历
    for (int i = Iindex; i < IBB->Instruction_list.size(); ++i) {
        auto tmpI = IBB->Instruction_list[i];
        //1)如果load指令与store指令涉及同一块内存，加入ret集合
        if (tmpI->GetOpcode() == BasicInstruction::LOAD) {
            auto LoadI = (LoadInstruction *)tmpI;
            if (alias_analyser->QueryAlias(ptr, LoadI->GetPointer(), C) == AliasStatus::MustAlias) {
                res.insert(LoadI);
                return res;
            }
        //2)如果为call指令，分外部调用与内部函数讨论
        } else if (tmpI->GetOpcode() == BasicInstruction::CALL) {
            auto CallI = (CallInstruction *)tmpI;
            auto call_name = CallI->GetFunctionName();
            //3)如果是外部函数调用中涉及指针读写的，插入res之中
            if (cfgTable.find(call_name) == cfgTable.end()) {    // external call
                if (!IsExternalCallReadPtr(CallI, ptr, C)) {
                    continue;
                }
                res.insert(CallI);
                return res;
            }
            //4)如果是内部函数，涉及读写内存的加入res
            auto target_cfg = cfgTable[call_name];
            auto& rwmap = alias_analyser->GetRWMap();
            auto it = rwmap.find(target_cfg);
            assert(it != rwmap.end());
            const RWInfo& rwinfo = it->second;
            if(alias_analyser->HasSideEffect(target_cfg)||!rwinfo.ReadRoots.empty())
            {
                res.insert(CallI);
                return res;
            }
            // if (!rwinfo.WriteRoots.empty() ||!rwinfo.ReadRoots.empty()|| rwinfo.has_lib_func_call) {    // may use memory
            //     res.insert(CallI);
            //     return res;
            // }
        }
    }
    //5.加入当前块的所有后继块
    std::queue<LLVMBlock> q;
    for (auto bb : C->GetSuccessor(IBB)) {
        q.push(bb);
    }
    auto ret_block=GetRetBlock(C);
    //6.如果当前块为返回块且无后继块，插入RET指令（？）
    if (IBB->block_id == ret_block->block_id && q.empty()) {
        res.insert(*(ret_block->Instruction_list.end() - 1));
        return res;
    }
    //7.BFS后继块
    // then BFS the CFG
    std::map<LLVMBlock, bool> vis_map;
    while (!q.empty()) {
        auto x = q.front();
        q.pop();
        if (vis_map[x]) {
            continue;
        }
        vis_map[x] = true;

        bool is_find = false;
        //1)逆序遍历块中的指令，加入涉及相同内存的load指令与涉及内存操作的call指令
        for (int i = x->Instruction_list.size() - 1; i >= 0; --i) {
            auto tmpI = x->Instruction_list[i];
            if (tmpI->GetOpcode() == BasicInstruction::LOAD) {
                auto LoadI = (LoadInstruction *)tmpI;
                if (alias_analyser->QueryAlias(ptr, LoadI->GetPointer(), C) == AliasStatus::MustAlias) {
                    res.insert(LoadI);
                    is_find = true;
                    break;
                }
            } else if (tmpI->GetOpcode() == BasicInstruction::CALL) {
                auto CallI = (CallInstruction *)tmpI;
                auto call_name = CallI->GetFunctionName();

                if (cfgTable.find(call_name) == cfgTable.end()) {    // external call
                    if (!IsExternalCallReadPtr(CallI, ptr, C)) {
                        continue;
                    }
                    res.insert(CallI);
                    is_find = true;
                    break;
                }

                auto target_cfg = cfgTable[call_name];
                auto& rwmap = alias_analyser->GetRWMap();
                auto it = rwmap.find(target_cfg);
                assert(it != rwmap.end());
                const RWInfo& rwinfo = it->second;
                if(alias_analyser->HasSideEffect(target_cfg)||!rwinfo.ReadRoots.empty())
                {
                    res.insert(CallI);
                    is_find = true;
                    break;
                }
                // if (!rwinfo.WriteRoots.empty() ||!rwinfo.ReadRoots.empty()|| rwinfo.has_lib_func_call) {    // may use memory
                //     res.insert(CallI);
                //     is_find = true;
                //     break;
                // }
            }
        }
        //2)如果没找到，还需要加入后继块的后继块
        if (!is_find) {
            for (auto bb : C->GetSuccessor(x)) {
                q.push(bb);
            }
        }
        //3)如果抵达返回块，加入RET指令
        // if reach ret_block, insert
        if (x->block_id == ret_block->block_id && !is_find) {
            res.insert(*(ret_block->Instruction_list.end() - 1));
        }
    }

    return res;
}


//检查两指令间路径是否存在存储冲突
bool SimpleMemDepAnalyser::IsNoStore(Instruction I1, Instruction I2, CFG *C) {
    //1.获取指令1的内存指向
    Operand ptr;
    if (I1->GetOpcode() == BasicInstruction::LOAD) {
        ptr = ((LoadInstruction *)I1)->GetPointer();
    } else if (I1->GetOpcode() == BasicInstruction::STORE) {
        ptr = ((StoreInstruction *)I1)->GetPointer();
    }
    //2.获取I1正向遍历，I2反向遍历的基本块交集（？）
    int id1=I1->GetBlockID();
    int id2=I2->GetBlockID();
    std::set<int> blocks;
    assert(id1 != id2);
    std::set<int> id1_tos, to_id2s;

    std::queue<int> q;
    std::vector<int> vis;
    vis.resize(C->max_label + 1);
    q.push(id1);
    //1.BFS编号为id1的基本块的所有后继块，所有可达块记录在id1_tos中
    while (!q.empty()) {
        int x = q.front();
        q.pop();
        if (vis[x]) {
            continue;
        }
        id1_tos.insert(x);
        vis[x] = 1;
        for (auto v : C->GetSuccessor(x)) {
            q.push(v->block_id);
        }
    }

    vis.clear();
    vis.resize(C->max_label + 1);
    q.push(id2);
    //2.BFS编号为id2的基本块的所有前驱块，所有可达块记录在id2_tos中（不记录id1的前驱块）
    while (!q.empty()) {
        int x = q.front();
        q.pop();
        if (vis[x]) {
            continue;
        }
        to_id2s.insert(x);
        vis[x] = 1;
        if (x == id1) {
            continue;
        }
        for (auto v : C->GetPredecessor(x)) {
            q.push(v->block_id);
        }
    }
    //3.记录两组路径的基本块交集
    for (int i = 0; i <= C->max_label; ++i) {
        if (id1_tos.find(i) != id1_tos.end() && to_id2s.find(i) != to_id2s.end()) {
            blocks.insert(i);
        }
    }

    if (blocks.size() == 0) {
        return false;
    }
    //3.遍历交集基本块
    for (auto id : blocks) {
        auto bb = (*C->block_map)[id];
        int st = 0, ed = bb->Instruction_list.size();
        //1)找到I1所在基本块及I1所在位置
        if (id == I1->GetBlockID()) {
            int Iindex = -1;
            auto IBB = (*C->block_map)[I1->GetBlockID()];
            for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
                auto tmpI = IBB->Instruction_list[i];
                if (tmpI == I1) {
                    Iindex = i;
                    break;
                }
            }
            assert(Iindex != -1);
            st = Iindex + 1;
        //2)找到I2所在基本块及指令所在位置
        } else if (id == I2->GetBlockID()) {
            int Iindex = -1;
            auto IBB = (*C->block_map)[I2->GetBlockID()];
            for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
                auto tmpI = IBB->Instruction_list[i];
                if (tmpI == I2) {
                    Iindex = i;
                    break;
                }
            }
            assert(Iindex != -1);
            ed = Iindex;
        }
        //3)遍历I1之后的指令，I2之前的指令，其他块中的指令
        for (int i = st; i < ed; ++i) {
            auto I = bb->Instruction_list[i];
            //4)如果I1之后还有store指令，处理了I1所在内存，则返回假
            if (I->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)I;
                if (alias_analyser->QueryAlias(ptr, StoreI->GetPointer(), C) == AliasStatus::MustAlias) {
                    return false;
                }
            //5)如果I1之后还有call 外部函数指令，则返回假；或者call函数涉及写内存，返回假
            } else if (I->GetOpcode() == BasicInstruction::CALL) {
                auto CallI = (CallInstruction *)I;
                auto call_name = CallI->GetFunctionName();

                if (cfgTable.find(call_name) == cfgTable.end()) {    // external call
                    return false;
                }

                auto target_cfg = cfgTable[call_name];
                auto& rwmap = alias_analyser->GetRWMap();
                auto it = rwmap.find(target_cfg);
                assert(it != rwmap.end());
                //const RWInfo& rwinfo = it->second;
                if(alias_analyser->HasSideEffect(target_cfg))
                {
                    return false;
                }
                // if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) 
                // {    // may def memory
                //     return false;
                // }
            }
        }
    }
    // std::cerr<<"NoStore\n";
    return true;
}
//判断两条load指令加载的是否是同一片内存
bool SimpleMemDepAnalyser::isLoadSameMemory(Instruction a, Instruction b, CFG *C) {
    //1)分别获取这两条load指令依赖的存储指令
    auto mem1 = GetLoadClobbers(a, C);
    auto mem2 = GetLoadClobbers(b, C);
    //2)如果依赖数不等，直接返回假
    if (mem1.size() != mem2.size()) {
        return false;
    }
    //3)如果依赖的指令不同，直接返回假
    for (auto I : mem1) {
        if (mem2.find(I) == mem2.end()) {
            return false;
        }
    }
    Operand ptr1,ptr2;
    if (a->GetOpcode() == BasicInstruction::LOAD) {
        ptr1 = ((LoadInstruction *)a)->GetPointer();
    } 
    else if(a->GetOpcode() == BasicInstruction::STORE) {
        ptr1 = ((StoreInstruction *)a)->GetPointer();
    }
    else{assert(false);}
    if (b->GetOpcode() == BasicInstruction::LOAD) {
        ptr2 = ((LoadInstruction *)b)->GetPointer();
    } 
    else if (b->GetOpcode() == BasicInstruction::STORE) {
        ptr2 = ((StoreInstruction *)b)->GetPointer();
    }
    else{assert(false);}
    //if(alias_analyser->QueryAlias(ptr1,ptr2,C)==NoAlias){return false;}//false
    if(ptr1->GetOperandType()==BasicOperand::REG&&ptr2->GetOperandType()==BasicOperand::REG)
    {//true
       return false;
    } 

    int id1 = a->GetBlockID();
    int id2 = b->GetBlockID();
    auto bb1=(*C->block_map)[id1];
    auto bb2=(*C->block_map)[id2];
    //4)如果依赖指令不只一条，且两条load指令在不同基本块中
    //如果两条指令之间没有其他修改该处内存的指令，则指向同一处内存
    if (mem1.size() > 1 && id1 != id2) {
        //5)如果a块支配b块，且从指令1到指令2没有其他可能修改此处内存的指令
        if( domtrees->GetDomTree(C)->dominates(bb1, bb2)) {
            if (IsNoStore(a, b, C)) {
                return true;
            }
        //6)如果b块支配a块，且从指令2到指令1没有其他可能修改此处内存的指令
        } else if (domtrees->GetDomTree(C)->dominates(bb2, bb1)) {
            if (IsNoStore(b, a, C)) {
                return true;
            }
        }
        return false;
    }

    return true;
}

bool SimpleMemDepAnalyser::isStoreBeUsedSame(Instruction a, Instruction b, CFG *C) {
    auto mem1 = GetStorePostClobbers(a, C);
    auto mem2 = GetStorePostClobbers(b, C);

    if (mem1.size() != mem2.size()) {
        return false;
    }

    for (auto I : mem1) {
        if (mem2.find(I) == mem2.end()) {
            return false;
        }
    }

    return true;
}

bool SimpleMemDepAnalyser::isStoreNotUsed(Instruction a, CFG *C) {

    auto mem1 = GetStorePostClobbers(a, C);
    if (mem1.size() != 1) {
        return false;
    }
    auto ClobberI = *mem1.begin();
    if (ClobberI->GetOpcode() == BasicInstruction::RET) {
        return true;
    }
    return false;
}

void SimpleMemDepAnalyser::MemDepTest() {
    //1.初始化blockID
    for (auto [defI, cfg] : IR->llvm_cfg) {
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(bb->block_id);
            }
        }
    }
    //2.测试指令
    std::set<Instruction> LoadSet;
    for (auto [defI, cfg] : IR->llvm_cfg) {
        defI->PrintIR(std::cerr);
        std::cerr<<"------------\n";
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                //3.测试load指令
                if (I->GetOpcode() == BasicInstruction::LOAD) {
                    //1)插入load指令到LoadSet，并打印
                    LoadSet.insert(I);
                    I->PrintIR(std::cerr);
                    // std::cerr<<"test loadclobeers:\n";
                    // auto IBB=(*cfg->block_map)[I->GetBlockID()];
                    // for (int i = IBB->Instruction_list.size() - 1; i >= 0; --i) {
        
                    //     //3.找到load指令所在位置
                    //     auto tmpI = IBB->Instruction_list[i];
                    //     tmpI->PrintIR(std::cerr);
                    // }
                    //2)获取当前load指令依赖的store指令/call指令，并打印
                    auto res = GetLoadClobbers(I, cfg);
                    for (auto resI : res) {
                        resI->PrintIR(std::cerr);
                    }
                    std::cerr << "-----------------------------------\n";
                //4.测试store指令
                } else if (I->GetOpcode() == BasicInstruction::STORE) {
                    I->PrintIR(std::cerr);
                    //1)获取store指令依赖的store指令/call指令
                    auto res = GetLoadClobbers(I, cfg);
                    for (auto resI : res) {
                        resI->PrintIR(std::cerr);
                    }
                    std::cerr << "-----------------------------------\n";
                }
            }
        }
        //5.测试两条load指令是否指向同一片内存
        std::cerr<<"test if load inst loads the same memory, LoadSet="<<LoadSet.size()<<"\n";
        for (auto I1 : LoadSet) {
            for (auto I2 : LoadSet) {
                I1->PrintIR(std::cerr);
                I2->PrintIR(std::cerr);
                std::cerr << isLoadSameMemory(I1, I2, cfg) << "\n";
            }
        }
        LoadSet.clear();
    }
    //6.测试store指令
    std::set<Instruction> StoreSet;
    for (auto [defI, cfg] : IR->llvm_cfg) {
        defI->PrintIR(std::cerr);
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                if (I->GetOpcode() == BasicInstruction::STORE) {
                    //1)加入store指令到storeSet并打印
                    StoreSet.insert(I);
                    I->PrintIR(std::cerr);
                    //2)获取store指令的后续使用者
                    auto res = GetStorePostClobbers(I, cfg);
                    for (auto resI : res) {
                        resI->PrintIR(std::cerr);
                    }
                    std::cerr << "-----------------------------------\n";
                }
            }
        }
        //3)判断两条store指令的后续用法是否相同
        for (auto I1 : StoreSet) {
            for (auto I2 : StoreSet) {
                I1->PrintIR(std::cerr);
                I2->PrintIR(std::cerr);
                std::cerr << isStoreBeUsedSame(I1, I2, cfg) << "\n";
            }
        }
        StoreSet.clear();
    }
}
void MemDepAnalysisPass::Execute()
{
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    SimpleMemDepAnalyser* memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);

    //memdep_analyser->MemDepTest();
}