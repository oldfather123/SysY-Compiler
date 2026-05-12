# 计划实现的Passes

## 数组相关

- ArrayAccess
- ColumnMajor

## 常量相关

- EarlyConstFold
- RegularFold

## 函数相关

- EarlyInline
- TCO
- AtMostOnce
- CallGraph
- Inline
- LateInline
- Pureness

## 循环相关

- Fusion（多面体模型实现）
- LoopDCE（合并到DCE中）
- Lower
- RaiseToFor
- CanonicalizeLoop
- LICM
- LoopAnalysis
- LoopUnroll
- RemoveEmptyLoop（合并到DCE中）
- SCEV

## 变量相关

- Localize
- Globalize
- Range
- RangeAwareFold

## 内存相关

- Base
- MoveAlloca（考虑在IR阶段直接执行）
- TidyMemory
- Alias
- Mem2Reg

## CFG相关

- Remerge
- AggressiveDCE
- DAE
- DCE
- DLE
- DSE
- FlattenCFG
- GCM
- GVN
- SimplifyCFG

## 额外的

- Unswitch
- InlineStore
- InstSchedule
- LoopRotate
- SynthConstArray