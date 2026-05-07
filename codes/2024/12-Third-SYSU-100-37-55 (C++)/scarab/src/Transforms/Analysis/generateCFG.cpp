#include "generateCFG.h"

// 在控制流图（CFG）中添加边
void addEdgeToCFG(shared_ptr<BasicBlock> pred, shared_ptr<BasicBlock> succ) {
    // 检查前驱和后继基本块是否为空
    if (!pred || !succ) {
        cerr << "error: pred or succ is null\n";
        return;
    }

    // 检查是否尝试连接自身
    if (pred == succ) {
        cerr << "error: Cannot connect a BasicBlock to itself\n";
        return;
    }

    // 检查边是否已存在
    if (pred->succBasicBlocks.count(succ) > 0) {
        cerr << "error: Edge already exists in CFG\n";
        return;
    }

    // 添加后继和前驱关系
    pred->succBasicBlocks.insert(succ);
    succ->predBasicBlocks.insert(pred);
}

void generateCFG(FunctionPtr func) {
    // 清理所有基本块的前驱和后继关系
    for (const auto& bb : func->basicBlocks) {
        bb->predBasicBlocks.clear();
        bb->succBasicBlocks.clear();
    }

    // 清理并重新生成 LabelToBBMap 和设置基本块所属函数
    func->LabelToBBMap.clear();
    func->initializeLabelToBlockMap();
    func->assignBlocksToFunction();

    // 遍历所有基本块，为每个基本块添加控制流边
    for (const auto& bb : func->basicBlocks) {
        // 检查基本块的最后一条指令是否为 Br 类型
        if (bb->instructions.back()->type == Br) {
            auto brInst = dynamic_cast<BrInstruction*>(bb->instructions.back().get());
            if (brInst) {
                addEdgeToCFG(bb, func->LabelToBBMap[brInst->label1]);

                // 如果存在 label2，则添加相应的控制流边
                if (brInst->label2 != nullptr) {
                    addEdgeToCFG(bb, func->LabelToBBMap[brInst->label2]);
                }
            }
        }
    }

    // 去除死代码块
    for (auto it = func->basicBlocks.begin(); it != func->basicBlocks.end(); ) {
        auto bb = *it;
        if (bb != func->basicBlocks.front() && bb->predBasicBlocks.size() == 0) {
            for (const auto& succ : bb->succBasicBlocks) {
                succ->predBasicBlocks.erase(bb);
            }
            it = func->basicBlocks.erase(it);
        }
        else {
            ++it;
        }
    }
}