#include "optimize.h"

void optimize(Module& irModule)
{
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            domTree(func);
            mem2reg(func);
            globalValueNumbering(func);
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            // funcCache(func, irModule);
            // generateCFG(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            TailCallElimination(func);
            generateCFG(func);
        }
    }

    analyseUse(irModule);
    eliminateDeadCodeInModule(irModule);
    UnusedArgEliminate(irModule);
    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateDeadCode(func);
            generateCFG(func);
        }
    }

    analyseUse(irModule);
    deadGvarElimnation(irModule);

    analyseUse(irModule);

    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateDeadCode(func);
            generateCFG(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            reassociate(func);
        }
    }

    analyseUse(irModule);
    replaceGlobalConstants(irModule);

    analyseUse(irModule);
    bool stopFlag = false;
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction && func->basicBlocks.size()<50)
        {
            domTree(func);
            loopAnalysis(func);
            stopFlag |= EarlyExitLoopElimination(func);
            generateCFG(func);
            // constProgression(func);
            // optimizeBranches(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateDeadCode(func);
            generateCFG(func);
        }
    }

    if(stopFlag) return;

    analyseUse(irModule);
    inlineOpt(irModule);

    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            optimizeCFG(func);
        }
    }

    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            domTree(func);
            loopAnalysis(func);
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            domTree(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateDeadCode(func);
            cse(func);
        }
    }

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         generateCFG(func);
    //         optimizeCFG(func);
    //         branch2select(func); // 请注意，br2select需要在inline的后面
    //         generateCFG(func);
    //         optimizeCFG(func);
    //     }
    // }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            strengthReduction(func);
            constProgression(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            loopAnalysis(func);
            LICM(func, false);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            loopAnalysis(func);
            SmoothGlobalVarAdd(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            loopAnalysis(func);
            loopUnroll(func);
            generateCFG(func);
            loopAnalysis(func);
            partialUnrollLoop(func);
            generateCFG(func);
        }
    }

    // analyseUse(irModule); // 并行化那里有用到use信息
    // std::vector<FunctionPtr> funcToInsert;
    // unordered_set<FunctionPtr> hasParallelFunc;
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         // domTree(func);
    //         // loopAnalysis(func);
    //         // 这里中间就不重新做Analysis了，否则会清除掉我留下的信息（loop中的loopEnd等）
    //         // if(loopParallel(func, irModule, funcToInsert)){
    //         //     hasParallelFunc.insert(func);
    //         // }
    //     }
    // }
    // for(auto it:funcToInsert) irModule.addFunction(it); // 注意，必须在这里做，否则可能会导致前面的遍历迭代器混乱

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         generateCFG(func);
    //         if(hasParallelFunc.count(func)){
    //             domTree(func);
    //             loopAnalysis(func);
    //             storeHoist(func);
    //         }
            
    //     }
    // }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            // reassociate(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateDeadCode(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            eliminateLoopDeadCode(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            strengthReduction(func);
            constProgression(func);
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            domTree(func);
            globalValueNumbering(func);
        }
    }

    analyseUse(irModule);

    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            cse(func);
        }
    }

    analyseUse(irModule); // 在cse之后记得加analyseUse
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
        }
    }
    loadStoreMerge(irModule);

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            eliminateDeadCode(func);
        }
    }

    analyseUse(irModule);
    bool hasCombined4If = false;
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            if(mergeCondBranches(func))
                hasCombined4If = true;
            constProgression(func);
            generateCFG(func);
            optimizeCFG(func);
            foldConstantBranches(func);
            optimizeBranches(func);
            generateCFG(func);
        }
    }

    if(hasCombined4If) {
        analyseUse(irModule);
        for (auto &func : (irModule.globalFunctions))
        {
            if (!func->isLibraryFunction)
            {
                eliminateDeadCode(func);
                generateCFG(func);
                optimizeCFG(func);
                optimizeBranches(func);
                generateCFG(func);
            }
        }
    }

    analyseUse(irModule);
    for (auto &func : (irModule.globalFunctions))
    {
        if (!func->isLibraryFunction)
        {
            generateCFG(func);
            optimizeCFG(func);
            optimizeBranches(func);
            generateCFG(func);
            domTree(func);
        }
    }

    // bool need_reassociate = false;
    // need_reassociate =optimizeLocalToGlobal(irModule);

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         generateCFG(func);
    //         if(need_reassociate)
    //             reassociate(func);
    //     }
    // }

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         loopAnalysis(func);
    //         if(need_reassociate)
    //         {
    //             LICM(func, true);
    //             reconstructLoops(func);
    //         }
    //     }
    // }

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         generateCFG(func);
    //         optimizeCFG(func);
    //         optimizeBranches(func);
    //         generateCFG(func);
    //     }
    // }

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         eliminateDeadCode(func);
    //     }
    // }

    // analyseUse(irModule);
    // for (auto &func : (irModule.globalFunctions))
    // {
    //     if (!func->isLibraryFunction)
    //     {
    //         strengthReduction(func);
    //         constProgression(func);
    //         generateCFG(func);
    //         optimizeCFG(func);
    //         optimizeBranches(func);
    //         foldConstantBranches(func);
    //     }
    // }

    // for(auto &func : (irModule.globalFunctions)){
    //     if(!func->isLibraryFunction){
    //         parallelSpecialDeal(func, irModule);
    //     }
    // }
}