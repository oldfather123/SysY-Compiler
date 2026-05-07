#include "CalleeSavedHandler.h"
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>

namespace sysy {

char CalleeSavedHandler::ID = 0;

bool CalleeSavedHandler::runOnFunction(Function *F, AnalysisManager& AM) {
    // This pass works on MachineFunction level, not IR level
    return false;
}

void CalleeSavedHandler::runOnMachineFunction(MachineFunction* mfunc) {
    StackFrameInfo& frame_info = mfunc->getFrameInfo();
    const std::set<PhysicalReg>& used_callee_saved = frame_info.used_callee_saved_regs;

    if (used_callee_saved.empty()) {
        frame_info.callee_saved_size = 0;
        frame_info.callee_saved_regs_to_store.clear();
        return;
    }

    // 1. 计算被调用者保存寄存器所需的总空间大小
    // s0 总是由 PEI Pass 单独处理，这里不计入大小，但要确保它在列表中
    int size = 0;
    std::set<PhysicalReg> regs_to_save = used_callee_saved;
    if (regs_to_save.count(PhysicalReg::S0)) {
        regs_to_save.erase(PhysicalReg::S0);
    }
    size = regs_to_save.size() * 8; // 每个寄存器占8字节 (64-bit)
    frame_info.callee_saved_size = size;

    // 2. 创建一个有序的、需要保存的寄存器列表，以便后续 Pass 确定地生成代码
    // s0 不应包含在此列表中，因为它由 PEI Pass 特殊处理
    std::vector<PhysicalReg> sorted_regs(regs_to_save.begin(), regs_to_save.end());
    std::sort(sorted_regs.begin(), sorted_regs.end(), [](PhysicalReg a, PhysicalReg b){ 
        return static_cast<int>(a) < static_cast<int>(b); 
    });
    frame_info.callee_saved_regs_to_store = sorted_regs;

    // 3. 更新栈帧总大小。
    // 这是初步计算，PEI Pass 会进行最终的对齐。
    frame_info.total_size = frame_info.locals_size + 
                            frame_info.spill_size + 
                            frame_info.callee_saved_size;
}

} // namespace sysy
