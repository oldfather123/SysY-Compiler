#ifndef ELIMINATE_FRAME_INDICES_H
#define ELIMINATE_FRAME_INDICES_H

#include "RISCv64LLIR.h"

namespace sysy {

class EliminateFrameIndicesPass {
public:
    // Pass 的主入口函数
    void runOnMachineFunction(MachineFunction* mfunc);

private:
    // 帮助计算类型大小的辅助函数，从原RegAlloc中移出
    unsigned getTypeSizeInBytes(Type* type);
};

} // namespace sysy

#endif // ELIMINATE_FRAME_INDICES_H