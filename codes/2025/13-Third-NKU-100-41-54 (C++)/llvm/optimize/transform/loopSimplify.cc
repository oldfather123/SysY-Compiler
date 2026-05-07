#include "loopSimplify.h"
#include "loop.h"

void LoopSimplifyPass::Execute() {
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		LoopInfo* loopInfo = cfg->getLoopInfo();
		if(!loopInfo->verifySimplifyForm(cfg)) {
			loopInfo->simplify(cfg);
		}
		#ifdef debug 
			auto definst = (FunctionDefineInstruction*)defI;
			std::string funcName = definst->GetFunctionName();
			std::cerr <<  funcName + "(): LoopSimplify result:" << std::endl;
			loopInfo->displayLoopInfo();
		#endif
		loopInfo->verifySimplifyForm(cfg);
        cfg->loopInfo = loopInfo;
    }
}