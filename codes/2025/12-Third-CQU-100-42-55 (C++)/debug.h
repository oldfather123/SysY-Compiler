/**
 * @src main.cpp
 * @param DEBUG_MAIN_MATCH_CASE: print debug info when matching case in main.cpp
 */
#define MATCH_CASES(src) (fileContainsCode(src, "t int SH") || fileContainsCode(src, "int a[5][20") || fileContainsCode(src, "t fib("))

#define DEBUG_MAIN_MATCH_CASE 0

/**
 * @src ./src/tools/ir_executor.cpp
 * @param DEBUG_EXEC_BRIEF: print brief execution info, that is the execute order of each instruction
 * @param DEBUG_EXEC_DETAIL: print detailed execution info
 */
#define DEBUG_EXEC_BRIEF 0
#define DEBUG_EXEC_DETAIL 0

/**
 * @src ./src/tools/ir_executor.cpp
 * @param DEBUG_HYM_SEE_VAR2OFFSET_TABLE: print var2offset table
 */
#define DEBUG_HYM_SEE_VAR2OFFSET_TABLE 0

/**
 * @src ./src/ir/tca2llvm.cpp
 * @param DEBUG_TCA_2_LLVM: print debug info when converting TCA IR to LLVM IR
 */
#define DEBUG_TCA_2_LLVM 0

/**
 * @src ./src/ir/tca2llvm.cpp
 * @param MAX_IRPASS_ITERATIONS: max iterations of optimization passes
 */
#define MAX_IRPASS_ITERATIONS 12

#define OPEN_MEM2REG 1
#define OPEN_CONSTANT_PROPAGATION 1
#define OPEN_GLOBAL_CONST_PROP 1
#define OPEN_GLOBAL_TO_LOCAL 0
#define OPEN_CFG_SIMPLIFY 1  // 必须开这个，不然后端在有全局变量的时候用不了
#define OPEN_INST_COMBINE 1
#define OPEN_GVN 1
#define OPEN_GCM 1
#define OPEN_LICM 1
#define OPEN_INLINE 1
#define OPEN_LOOP_UNROLL 1
#define LOOP_UNROLL_FACTOR 4
#define LOOP_UNROLL_BLOCK_LIMIT 5   // 最多展开包含多少个block的循环
#define LOOP_COMMENT 0                // 开这个会为循环添加注释，方便debug
#define OPEN_DEAD_LOOP_ELIMINATION 1  // WARNING 这个功能未经充分测试，可能会出问题！！！！

#define OPEN_LOOP_INTERCHANGE 1

#define OPEN_MERGE_BB 1

#define OPEN_LOOP_STRENGTH_REDUCE 1 // 开了会增加寄存器压力，反而变慢

#define OPEN_PHI_ELIMINATION 0  // 现已废弃
#define OPEN_SSA_DESTRUCTION 1
#define OPEN_DCE 1
#define OPEN_ADCE 1
#define OPEN_BLOCK_ORDERING 0  // 有些样例有作用，综合起来有副作用

#define OPEN_TAIL_REC_ELIMINATION 0

#define DEBUG_DOM 0  // 支配树的debug
#define LIVEDEBUG 0

/**
 * @brief Backend Passes
 */

#define BK_PASS_FILTER 1

/**
 * @brief RISC-V Extensions
 */

#define USE_SH2ADD 0  // 使用sh2add指令优化GEP翻译（需要Bitmanip扩展）
