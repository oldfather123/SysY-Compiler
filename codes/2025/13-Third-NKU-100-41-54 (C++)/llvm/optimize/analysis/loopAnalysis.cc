#include "loopAnalysis.h"
#include "loopAnalysis.h"
#include "loop.h"

void LoopAnalysisPass::Execute() {
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		LoopInfo* loopInfo = (cfg->getLoopInfo() == nullptr) ? new LoopInfo() : cfg->getLoopInfo();
		loopInfo->analyze(cfg);
		#ifdef debug
			auto definst = (FunctionDefineInstruction*)defI;
			std::string funcName = definst->GetFunctionName();
			std::cerr <<  funcName + "(): LoopAnalysis result:" << std::endl;
			loopInfo->displayLoopInfo();
		#endif
        cfg->loopInfo = loopInfo;
    }
}	