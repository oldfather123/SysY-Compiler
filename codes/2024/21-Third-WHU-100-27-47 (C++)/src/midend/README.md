# IR Passes

| 编号 | Pass Name       | 完成情况  | 测试情况          | TODO                             |                              |
| ---- | --------------- | --------- | ----------------- | -------------------------------- | ---- |
| 1    | ConstantFolding | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 2    | CopyProgagation | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 3    | DCE             | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 4    | Mem2Reg&SSA     | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 5    | CSE             | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 6    | SimplifyCFG     | ✔ 完成    | ✔ 测试通过        | Merge unconditional branched bbs |  |
| 7    | StrengthReduce  | ✔ 完成    | ✔ 测试通过        |                                  |                                  |
| 8    | FuncInline      | ✔ 完成 | ✔ 测试通过   |                                  |                                  |
| 9    | TailCallElim | 🚫 搁置 |                   | Eliminate tail call, avoiding stack overflow |  |
| 10   | DSE          | ✔ 完成 | ✔ 测试通过 | Redundant Load&Store Elimination |  |
|      | LoopDetection   | ✔ 完成    | ✔ 测试通过 |                                  |                                  |
| 11   | @LICM           | ❌ 未完成 | ✔ 测试通过   | [Link](https://llvm.org/docs/Passes.html#licm-loop-invariant-code-motion) @DDL:2024/8/18 | @ZZH |
| 12 | @InductionVariable | ❌ 未完成 |  | @DDL:2024/8/18 | @ZZY |
| 13 | @LoopRotate | ❌ 未完成 |  | 方便后续循环优化 @Maybe:2024/8/18 | @ZZH |
| 14 | LoopUnrolling   | ❌ 未完成 |                   | [Link](https://llvm.org/docs/Passes.html#loop-unroll-unroll-loops) | @FCH |
| 15 | LoopFlatten     | ❌ 未完成 |                   | [Link](https://www.cs.cornell.edu/courses/cs6120/2020fa/blog/loop-flatten/) | @FYY |
| 16 | @LoopGepCombine | ❌ 未完成 |                   | 针对`a[i][j]`中i和j不断递增的情况                            | @FCH |
| 17 | @PureFuncDetection | ❌ 未完成 | | @DDL:2024/8/18 | @ZZH |
| 18 | @OperationRewrite | ❌ 未完成 |                   | [Link](https://llvm.org/docs/Passes.html#reassociate-reassociate-expressions) @DDL:2024/8/18 | @ZZH |

# BackEnd Opt

| 编号 | Opt Name | 完成情况 | 测试情况   | 备注     |
| ---- | -------- | -------- | ---------- | -------- |
| 1    | 窥孔     | ✔ 完成   | ✔ 测试通过 | 多加规则 |
| 2    | 指令调度 | 🚫 搁置   |            | 提升 30% |

# Performance Analysis

| 测试点                     | 是否通过 | 运行时间       | 最佳时间     | 最佳时间队伍                            | 倍数        | 优化方法 |
|----------------------------|----------|----------------|--------------|-----------------------------------------|-------------|-------------|
| call_1                      | 通过     | 36666928.00    | 119.00       | 三进制冒险家/ 国防科技大学              | 308962.76   | Reassociate |
| call_2                      | 通过     | 9170339.00     | 108.00       | 三进制冒险家/ 国防科技大学              | 84910.55    | Reassociate |
| call_3                      | 通过     | 18342684.00    | 118.00       | 三进制冒险家/ 国防科技大学              | 155445.63   | Reassociate |
| fft0                        | 通过     | 28761486.00    | 5257773.00   | 睿睿也想打编译队/ 北京航空航天大学      | 5.47        |         |
| fft1                        | 通过     | 60921926.00    | 11287726.00  | 睿睿也想打编译队/ 北京航空航天大学      | 5.40        |         |
| fft2                        | 通过     | 59153069.00    | 10843319.00  | 睿睿也想打编译队/ 北京航空航天大学      | 5.45        |         |
| gameoflife-gosper           | 通过     | 51591089.00    | 14776686.00  | NNVM/ 南京大学                          | 3.49        |         |
| gameoflife-oscillator       | 通过     | 44720049.00    | 11776457.00  | NNVM/ 南京大学                          | 3.80        |         |
| gameoflife-p61glidergun     | 通过     | 44318861.00    | 12653008.00  | NNVM/ 南京大学                          | 3.50        |         |
| h-1-01                      | 通过     | 237964153.00   | 17365702.00  | 素履“译”往队/ 北京航空航天大学         | 13.71       |        |
| h-1-02                      | 通过     | 19662359.00    | 1730324.00   | 素履“译”往队/ 北京航空航天大学         | 11.36       |        |
| h-1-03                      | 通过     | 112047625.00   | 8690055.00   | 素履“译”往队/ 北京航空航天大学         | 12.89       |        |
| h-2-01                      | 通过     | 46676125.00    | 3920.00      | 素履“译”往队/ 北京航空航天大学         | 11907.88    |     |
| h-2-02                      | 通过     | 45442690.00    | 4026.00      | 素履“译”往队/ 北京航空航天大学         | 11284.24    |     |
| h-2-03                      | 通过     | 56381645.00    | 3968.00      | 素履“译”往队/ 北京航空航天大学         | 14208.12    |     |
| h-3-01                      | 通过     | 11321804.00    | 8565445.00   | NNVM/ 南京大学                          | 1.32        |         |
| h-3-02                      | 通过     | 11298483.00    | 8557324.00   | NNVM/ 南京大学                          | 1.32        |         |
| h-3-03                      | 通过     | 11303215.00    | 8588221.00   | NNVM/ 南京大学                          | 1.32        |         |
| h-4-01                      | 通过     | 10055845.00    | 3522049.00   | 世界第一可爱Fuyuki/ 清华大学            | 2.85        |         |
| h-4-02                      | 通过     | 36969301.00    | 11659280.00  | 世界第一可爱Fuyuki/ 清华大学            | 3.17        |         |
| h-4-03                      | 通过     | 67962397.00    | 21327117.00  | 世界第一可爱Fuyuki/ 清华大学            | 3.19        |         |
| h-5-01                      | 通过     | 30942577.00    | 22363674.00  | 三进制冒险家/ 国防科技大学              | 1.38        |         |
| h-5-02                      | 通过     | 30936518.00    | 22297606.00  | 三进制冒险家/ 国防科技大学              | 1.39        |         |
| h-5-03                      | 通过     | 30927562.00    | 22276852.00  | 三进制冒险家/ 国防科技大学              | 1.39        |         |
| h-6-01                      | 通过     | 11368963.00    | 3873336.00   | pinyinggaoshou/ 杭州电子科技大学        | 2.93        |         |
| h-6-02                      | 通过     | 16838581.00    | 5807323.00   | pinyinggaoshou/ 杭州电子科技大学        | 2.90        |         |
| h-6-03                      | 通过     | 22697179.00    | 7793688.00   | pinyinggaoshou/ 杭州电子科技大学        | 2.91        |         |
| h-7-01                      | 通过     | 1619968.00     | 1358324.00   | 世界第一可爱Fuyuki/ 清华大学            | 1.19        |         |
| h-8-01                      | 通过     | 20494121.00    | 10962649.00  | return_0;/ 清华大学                     | 1.87        |         |
| h-8-02                      | 通过     | 20463487.00    | 10931162.00  | return_0;/ 清华大学                     | 1.87        |         |
| h-8-03                      | 通过     | 20541550.00    | 10920232.00  | return_0;/ 清华大学                     | 1.88        |         |
| if-combine1                 | 通过     | 34389502.00    | 56.00        | 素履“译”往队/ 北京航空航天大学         | 614098.25   |    |
| if-combine2                 | 通过     | 51592745.00    | 57.00        | 素履“译”往队/ 北京航空航天大学         | 905117.46   |    |
| if-combine3                 | 通过     | 86967066.00    | 57.00        | 素履“译”往队/ 北京航空航天大学         | 1525685.37  |   |
| loop_array_1                | 通过     | 16428728.00    | 3759.00      | 素履“译”往队/ 北京航空航天大学         | 4371.86     |      |
| loop_array_2                | 通过     | 32968774.00    | 3898.00      | 素履“译”往队/ 北京航空航天大学         | 8458.95     |      |
| loop_array_3                | 通过     | 14313780.00    | 1130.00      | 素履“译”往队/ 北京航空航天大学         | 12664.41    |     |
| matmul1                     | 通过     | 39481675.00    | 10417238.00  | return_0;/ 清华大学                     | 3.79        |         |
| matmul2                     | 通过     | 39336347.00    | 10450737.00  | return_0;/ 清华大学                     | 3.76        |         |
| matmul3                     | 通过     | 39473717.00    | 10509087.00  | return_0;/ 清华大学                     | 3.76        |         |
| mm1                         | 通过     | 17921173.00    | 8166267.00   | NNVM/ 南京大学                          | 2.19        |         |
| mm2                         | 通过     | 15929616.00    | 7260793.00   | NNVM/ 南京大学                          | 2.19        |         |
| mm3                         | 通过     | 11969211.00    | 5475187.00   | NNVM/ 南京大学                          | 2.19        |         |

---

| 用例名                  | 功能点                             | 优化难点                                  | 备注                                             |
| ----------------------- | ---------------------------------- | ----------------------------------------- | ------------------------------------------------ |
| 01_mm1                  | 矩阵乘法、矩阵初始化、矩阵元素累加 | LoopUnrolling, RLELocal, LICM, FuncInline |                                                  |
| 01_mm2                  | 矩阵乘法、矩阵初始化、矩阵元素累加 | LoopUnrolling, RLELocal, LICM, FuncInline |                                                  |
| 01_mm3                  | 矩阵乘法、矩阵初始化、矩阵元素累加 | LoopUnrolling, RLELocal, LICM, FuncInline |                                                  |
| 03_sort1                | 基数排序、数组操作                 | LoopUnrolling, LICM, StrengthReduce       | 复杂的数组操作和递归处理，内存访问模式优化重要   |
| 03_sort2                | 基数排序、数组操作                 | LoopUnrolling, LICM, StrengthReduce       | 复杂的数组操作和递归处理，内存访问模式优化重要   |
| 03_sort3                | 基数排序、数组操作                 | LoopUnrolling, LICM, StrengthReduce       | 复杂的数组操作和递归处理，内存访问模式优化重要   |
| 04_spmv1                | 稀疏矩阵-向量乘法                  | RLELocal, RLEGlobal, StrengthReduce       | 内存访问稀疏性，计算与内存访问的平衡             |
| 04_spmv2                | 稀疏矩阵-向量乘法                  | RLELocal, RLEGlobal, StrengthReduce       | 内存访问稀疏性，计算与内存访问的平衡             |
| 04_spmv3                | 稀疏矩阵-向量乘法                  | RLELocal, RLEGlobal, StrengthReduce       | 内存访问稀疏性，计算与内存访问的平衡             |
| fft0                    | 快速傅里叶变换                     | LoopUnrolling, FuncInline, StrengthReduce | 递归处理深度，优化乘法和内存访问的模式           |
| fft1                    | 快速傅里叶变换                     | LoopUnrolling, FuncInline, StrengthReduce | 递归处理深度，优化乘法和内存访问的模式           |
| fft2                    | 快速傅里叶变换                     | LoopUnrolling, FuncInline, StrengthReduce | 递归处理深度，优化乘法和内存访问的模式           |
| gameoflife-gosper       | Conway's Game of Life              | LoopUnrolling, RLELocal, LICM             | 涉及到多次循环嵌套和数组操作，处理规则复杂性高   |
| gameoflife-oscillator   | Conway's Game of Life              | LoopUnrolling, RLELocal, LICM             | 涉及到多次循环嵌套和数组操作，处理规则复杂性高   |
| gameoflife-p61glidergun | Conway's Game of Life              | LoopUnrolling, RLELocal, LICM             | 涉及到多次循环嵌套和数组操作，处理规则复杂性高   |
| if-combine1             | 分支处理、多重 if-else 结构        | FuncInline, SimplifyCFG, RLEGlobal        | 多重分支的合并和优化                             |
| if-combine2             | 分支处理、多重 if-else 结构        | FuncInline, SimplifyCFG, RLEGlobal        | 多重分支的合并和优化                             |
| if-combine3             | 分支处理、多重 if-else 结构        | FuncInline, SimplifyCFG, RLEGlobal        | 多重分支的合并和优化                             |
| large_loop_array_1      | 大规模数组操作与循环               | LoopUnrolling, StrengthReduce, RLELocal   | 涉及大量数组的循环操作，优化内存访问和计算复杂性 |
| large_loop_array_2      | 大规模数组操作与循环               | LoopUnrolling, StrengthReduce, RLELocal   | 涉及大量数组的循环操作，优化内存访问和计算复杂性 |
| large_loop_array_3      | 大规模数组操作与循环               | LoopUnrolling, StrengthReduce, RLELocal   | 涉及大量数组的循环操作，优化内存访问和计算复杂性 |
| matmul1                 | 矩阵乘法、多重循环                 | LoopUnrolling, StrengthReduce, RLELocal   | 包含多次矩阵操作和条件判断，内存访问频繁         |
| matmul2                 | 矩阵乘法、多重循环                 | LoopUnrolling, StrengthReduce, RLELocal   | 包含多次矩阵操作和条件判断，内存访问频繁         |
| matmul3                 | 矩阵乘法、多重循环                 | LoopUnrolling, StrengthReduce, RLELocal   | 包含多次矩阵操作和条件判断，内存访问频繁         |
| recursive_call_1        | 递归调用处理                       | FuncInline, LICM, StrengthReduce          | 递归深度和栈帧管理是主要挑战                     |
| recursive_call_2        | 递归调用处理                       | FuncInline, LICM, StrengthReduce          | 递归深度和栈帧管理是主要挑战                     |
| recursive_call_3        | 递归调用处理                       | FuncInline, LICM, StrengthReduce          | 递归深度和栈帧管理是主要挑战                     |
| shuffle0                | 哈希表处理与查询                   | RLELocal, RLEGlobal, StrengthReduce       | 大量哈希表操作与查询，内存访问和条件分支优化重要 |
| shuffle1                | 哈希表处理与查询                   | RLELocal, RLEGlobal, StrengthReduce       | 大量哈希表操作与查询，内存访问和条件分支优化重要 |
| shuffle2                | 哈希表处理与查询                   | RLELocal, RLEGlobal, StrengthReduce       | 大量哈希表操作与查询，内存访问和条件分支优化重要 |
| sl1                     | 多维数组操作、大规模循环           | LoopUnrolling, StrengthReduce, RLELocal   | 复杂的三重嵌套循环和多维数组操作                 |
| sl2                     | 多维数组操作、大规模循环           | LoopUnrolling, StrengthReduce, RLELocal   | 复杂的三重嵌套循环和多维数组操作                 |
| sl3                     | 多维数组操作、大规模循环           | LoopUnrolling, StrengthReduce, RLELocal   | 复杂的三重嵌套循环和多维数组操作                 |
| transpose0              | 矩阵转置、数组操作                 | LoopUnrolling, RLELocal, StrengthReduce   | 涉及矩阵的转置和大量的内存访问操作               |
| transpose1              | 矩阵转置、数组操作                 | LoopUnrolling, RLELocal, StrengthReduce   | 涉及矩阵的转置和大量的内存访问操作               |
| transpose2              | 矩阵转置、数组操作                 | LoopUnrolling, RLELocal, StrengthReduce   | 涉及矩阵的转置和大量的内存访问操作               |
