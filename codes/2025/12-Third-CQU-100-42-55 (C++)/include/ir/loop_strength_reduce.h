#ifndef LLVM_IR_LOOP_STRENGTH_REDUCE_H
#define LLVM_IR_LOOP_STRENGTH_REDUCE_H

#include "llvm_ir.h"

namespace llvm_ir {
namespace lsr {

// 循环强度削弱（Loop Strength Reduction）
// 针对形如：
//   %p = getelementptr ..., i32 %iv
// 其中 %iv 在循环中为 {Start,+,Step} 的加法递归（且 Start/Step 为常量），
// 转化为：
//   preheader: %init_ptr = gep base, Start
//   header:    %p.phi = phi [ %init_ptr, preheader ], [ %next_ptr, latch ]
//   latch:     %next_ptr = gep %p.phi, Step
// 用以消除循环体中的昂贵索引计算。
bool runOnModule(Module& M);

} // namespace lsr
} // namespace llvm_ir

#endif // LLVM_IR_LOOP_STRENGTH_REDUCE_H 