
#include "../../include/cfg.h"
#include "../analysis/AliasAnalysis.h"
#include "../analysis/memdep.h"
#include "basic_dse.h"
#include <functional>
//#define DSE_DEBUG

#ifdef DSE_DEBUG
#define DSE_DEBUG_PRINT(x) do { x; } while(0)
#else
#define DSE_DEBUG_PRINT(x)
#endif

// 注：需在此PASS前运行inv_dom.invExecute
extern std::unordered_map<std::string, CFG*> cfgTable;
extern LLVMBlock GetRetBlock(CFG* C);
/**
 * 判断指令I1执行后是否可能执行到I2（I1是否能到达I2）
 * @param I1 起始指令
 * @param I2 目标指令
 * @param C 控制流图指针
 * @return 若能到达则返回true，否则返回false
 */
static bool CanReach(Instruction I1, Instruction I2, CFG *C) {
    auto bb1_id = I1->GetBlockID();
    auto bb2_id = I2->GetBlockID();
    //1.若两指令在同一基本块，则返回false（彼此可达）
    if (bb1_id == bb2_id) {
        return false;
    }

    std::vector<int> vis;//标记基本块是否已访问
    vis.resize(C->max_label + 1);
    
    std::queue<int> q;// 用于BFS遍历的队列
    
    //2.将I1所在基本块ID加入队列，开始BFS
    q.push(bb1_id);

    //3.BFS遍历控制流图
    while (!q.empty()) {
        // 取出队首基本块ID
        auto x = q.front();
        q.pop();
        // 若到达I2所在基本块，返回true
        if (x == bb2_id) {
            return true;
        }
        // 若已访问过该基本块，跳过
        if (vis[x]) {
            continue;
        }
        // 标记为已访问
        vis[x] = true;

        // 遍历当前基本块的所有后继基本块，加入队列
        for (auto bb : C->GetSuccessor(x)) {
            q.push(bb->block_id);
        }
    }
    // 遍历结束仍未到达I2所在基本块，返回false
    return false;
}
//根据erase_set删除对应的store指令
void SimpleDSEPass::EliminateIns(CFG* C)
{
    for (auto [id, bb] : *C->block_map) {
        std::deque<Instruction> new_instructions; 
        for (auto I : bb->Instruction_list) {
            if (erase_set.find(I) == erase_set.end()) {
                new_instructions.push_back(I);
            } else {
                // 输出被删除的 store 指令信息
                if (I->GetOpcode() == BasicInstruction::STORE) {
                    auto StoreI = (StoreInstruction *)I;
                    DSE_DEBUG_PRINT(std::cout << "[DSE] 删除 store 指令: 基本块 " << id 
                                              << ", 指针: " << StoreI->GetPointer()->GetFullName()
                                              << ", 值: " << StoreI->GetValue()->GetFullName() << std::endl);
                }
            }
        }
        bb->Instruction_list = new_instructions;
    }
}
/**
 * 基本块内的死存储消除（仅处理基本块内的冗余存储）
 * @param C 控制流图指针
 */
void SimpleDSEPass::BasicBlockDSE(CFG *C) {
    erase_set.clear();
    //1.遍历所有基本块
    for (auto [id, bb] : *C->block_map) {
        storeptrs_map.clear();

        // 1)反向遍历指令列表（反向遍历便于检测冗余存储）
        for (int i = bb->Instruction_list.size() - 1; i >= 0; --i) {
            auto I = bb->Instruction_list[i];
            if(I->GetOpcode() != BasicInstruction::STORE){continue;}
            // 2)若为store指令
            auto StoreI = (StoreInstruction *)I;
            auto ptr = StoreI->GetPointer();
            //3)如果从未记录过该地址
            if(storeptrs_map.find(ptr) == storeptrs_map.end())
            {
                //A.记录当前存储指令
                storeptrs_map[ptr].push_back(StoreI);
                continue;
            }
            // 4)如果store的地址已有存储指令记录
            //A.遍历该地址已有的存储指令
            for (auto oldI : storeptrs_map[ptr]) {
                // B.若判断旧存储使当前存储指令冗余（同一地址且无中间使用）
                if (memdep_analyser->isStoreBeUsedSame(I, oldI, C)) {
                    // 额外的安全检查：确保两个 store 指令在同一个基本块内
                    if (I->GetBlockID() == oldI->GetBlockID()) {
                        //C.将当前存储指令加入待删除集合
                        erase_set.insert(I);
                        DSE_DEBUG_PRINT(std::cout << "[DSE-基本块内] 标记删除冗余 store 指令: 基本块 " << id 
                                                  << ", 指针: " << StoreI->GetPointer()->GetFullName() << std::endl);
                        //D.找到冗余后跳出循环（无需检查更早的存储）
                        break;
                    }
                }
            }
        }
    }

    //2.从所有基本块中移除待删除的指令
    EliminateIns(C);
}
void SimpleDSEPass::DomDfs(CFG*C,int bbid)
{
    std::map<Operand, int> tmp_map;//当前基本块下，每个地址添加的存储指令数量
    LLVMBlock now = (*C->block_map)[bbid];//当前基本块
    // 1.反向遍历基本块中的指令
    for (int i = now->Instruction_list.size() - 1; i >= 0; --i) {
        auto I = now->Instruction_list[i];
        if (I->GetOpcode() != BasicInstruction::STORE){continue;}
        // 1）若为store指令
        bool is_dse = false;// 标记当前存储是否可被消除
        auto StoreI = (StoreInstruction *)I;
        auto ptr = StoreI->GetPointer();
        // 2)若该地址已有存储指令记录
        if (storeptrs_map.find(ptr) != storeptrs_map.end()) {
            // 3）遍历该地址已有的存储指令
            for (auto oldI : storeptrs_map[ptr]) {
                // 4）若判断两存储冗余，且当前存储无法到达旧存储（？）
                if (memdep_analyser->isStoreBeUsedSame(I, oldI, C) && !CanReach(I, oldI, C)) {
                    //5）将当前存储加入待删除集合，并标记dse
                    erase_set.insert(I);
                    DSE_DEBUG_PRINT(std::cout << "[DSE-后支配树] 标记删除冗余 store 指令: 基本块 " << bbid 
                                              << ", 指针: " << StoreI->GetPointer()->GetFullName() << std::endl);
                    is_dse = true;
                    // 找到冗余后跳出循环
                    break;
                }
            }
        }
        if(!is_dse)
        {
            //7)记录当前地址新增的存储指令数量
            tmp_map[ptr] += 1;
            //8)将当前存储指令加入全局映射表
            storeptrs_map[ptr].push_back(StoreI);
        }
    }
    
    // 2.递归遍历后支配树的子节点（后支配树的后继节点）
    for (auto v : C->PostDomTree->dom_tree[bbid]) {
        DomDfs(C,v->block_id);
    }

    //3. 回溯：移除当前基本块添加的存储指令（恢复全局映射表状态）
    for (auto [op, num] : tmp_map) {
        for (int i = 0; i < num; ++i) {
            storeptrs_map[op].pop_back();
        }
    }
}
/**
 * 基于后支配树遍历的死存储消除
 * @param C 控制流图指针
 */
void SimpleDSEPass::PostDomTreeWalkDSE(CFG *C) {
    erase_set.clear();
    storeptrs_map.clear();
    //1.获取返回块作为后支配树遍历的起点
    auto ret_block=GetRetBlock(C);
    //2.从返回块开始DFS遍历后支配树
    DomDfs(C,ret_block->block_id);
    //3/从所有基本块中移除待删除的指令
    EliminateIns(C);
}

/**
 * 消除未被使用的存储指令
 * @param C 控制流图指针
 */
void SimpleDSEPass::EliminateNotUsedStore(CFG *C) {
    erase_set.clear();
    //1.遍历所有基本块
    for (auto [id, bb] : *C->block_map) {
        //2.遍历基本块中的所有指令
        for (auto I : bb->Instruction_list) {
            //3.只处理存储指令
            if (I->GetOpcode() != BasicInstruction::STORE) {
                continue;
            }
            auto StoreI = (StoreInstruction *)I;
            auto ptr = StoreI->GetPointer();
            auto PtrRegMemMap=alias_analyser->GetPtrMap();
            int tflag=false;
            if (ptr->GetOperandType() == BasicOperand::REG && PtrRegMemMap[C][((RegOperand *)ptr)->GetRegNo()]->type==PtrInfo::Local) {
                tflag=true;
            }
            //4. 若地址是局部指针，或当前函数是main函数(排除main函数)
            if ( (tflag)&&memdep_analyser->isStoreNotUsed(I, C)) {
                // 5.若判断该存储未被使用,将该存储指令加入待删除集合
               erase_set.insert(I);
               DSE_DEBUG_PRINT(std::cout << "[DSE-未使用] 标记删除未使用的 store 指令: 基本块 " << id 
                                         << ", 指针: " << StoreI->GetPointer()->GetFullName() << std::endl);
            }
        }
    }

    // 从所有基本块中移除待删除的指令（与前面的消除逻辑相同）
    EliminateIns(C);
}

/**
 * 简单死存储消除的入口函数，依次调用三种DSE优化
 * @param C 控制流图指针
 */
void SimpleDSEPass::SimpleDSE(CFG *C) {

    // 1. 基本块内的死存储消除
    BasicBlockDSE(C);
    // 2. 基于后支配树的死存储消除
    PostDomTreeWalkDSE(C);
    // 3. 未被使用的存储消除
    EliminateNotUsedStore(C);
}
void SimpleDSEPass::Execute()
{
    memdep_analyser = new SimpleMemDepAnalyser(llvmIR,alias_analyser,domtrees);
    cfgTable.clear();
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    for (auto [defI, cfg] : llvmIR->llvm_cfg)
    {
        std::string funcName = defI->GetFunctionName();
        DSE_DEBUG_PRINT(std::cout << "\n=== 开始处理函数: " << funcName << " ===" << std::endl);
        
        for (auto [id, bb] : *cfg->block_map) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(id);
            }
        }
        SimpleDSE(cfg);
        
        DSE_DEBUG_PRINT(std::cout << "=== 函数 " << funcName << " 处理完成 ===" << std::endl);
    }
}