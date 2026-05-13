# RISC-V 目标代码生成方案

目标架构：RV64IF（riscv64 + 单精度浮点）

---

## 方案一：最短路径——直接翻译，能跑就行

### 核心思路

跳过 Machine IR，直接从当前 IR 翻译为 RISC-V 汇编。不做寄存器分配，所有值走栈。

### 流水线

```
IR (Region/BasicBlock/Op)
  │
  ▼
AsmEmitter  ← 单个类，遍历 IR 直接输出汇编文本
  │
  ▼
.s 文件
```

### 文件规划

```
src/asm/
├── AsmEmitter.h / AsmEmitter.cpp   ← IR→汇编直接翻译
└── AsmRegs.h                        ← 寄存器名常量（可选，也可直接写字符串）
```

### 栈帧设计

不做寄存器分配，每个 IR Op 的结果都 spill 到栈上：

```
┌──────────────────────┐ ← sp (函数入口时分配)
│  ra  (返回地址)       │   sp + framesize - 8
│  s0  (帧指针, 可选)   │   sp + framesize - 16
│  saved regs (如有)    │   ...
├──────────────────────┤
│  alloca 局部变量区     │   由 AllocaOp 分配
├──────────────────────┤
│  临时值栈槽           │   每个非 void Op 分配一个 8 字节槽
├──────────────────────┤
│  调用参数区 (≥8个时)  │   最靠栈底
└──────────────────────┘ ← sp = 0
```

### 翻译规则（逐 Opcode）

#### 常量/取参

| IR Op | RISC-V 输出 |
|-------|------------|
| `IntOp <value=42>` | `li t0, 42` → `sw t0, offset(sp)` |
| `FloatOp <value=3.14>` | `li t0, <ieee754_bits>` → `fmv.w.x ft0, t0` → `fsw ft0, offset(sp)` |
| `GetArgOp <idx=0>` | 前 8 个整数参数：`sw a0, offset(sp)`；浮点前 8 个：`fsw fa0, offset(sp)`；第 9+ 个：从调用者栈帧 load |

#### 内存操作

| IR Op | RISC-V 输出 |
|-------|------------|
| `AllocaOp <size=N>` | 在栈帧中预留 N 字节，记录该 alloca 的 sp 偏移 |
| `GlobalOp <name=x>` | 在 `.data` 段声明：`x: .word 0` 或 `.space N` |
| `LoadOp(addr)` | `lw t0, addr_offset(sp)` → 如果是全局：`la t1, x; lw t0, 0(t1)` |
| `StoreOp(val, addr)` | `lw t0, val_offset(sp)` → `sw t0, addr_offset(sp)` |
| `GetGlobalOp` | `la t0, x` → `sw t0, offset(sp)` |

#### 算术运算

| IR Op | RISC-V 输出 |
|-------|------------|
| `AddIOp(a, b)` | `lw t0, a(sp); lw t1, b(sp); add t0, t0, t1; sw t0, result(sp)` |
| `SubIOp` | `sub` |
| `MulIOp` | `mul` |
| `DivIOp` | `div` |
| `ModIOp` | `rem` |
| `AddFOp` | `flw ft0, a(sp); flw ft1, b(sp); fadd.s ft0, ft0, ft1; fsw ft0, result(sp)` |
| `SubFOp/MulFOp/DivFOp` | 同上，`fsub.s`/`fmul.s`/`fdiv.s` |
| `MinusOp(a)` | `lw t0, a(sp); neg t0, t0; sw t0, result(sp)` |
| `MinusFOp` | `flw ft0, a(sp); fneg.s ft0, ft0; fsw ft0, result(sp)` |
| `NotOp(a)` | `lw t0, a(sp); seqz t0, t0; sw t0, result(sp)` |
| `AndIOp(a,b)` | `and`（按位与，当前 IR 用它做逻辑与） |
| `OrIOp(a,b)` | `or` |
| `AddLOp/SubLOp` | 同 AddI/SubI（地址算术，64 位用 `add`/`sub`，但当前 IR 用 Int 类型，32 位够用） |

#### 比较运算

| IR Op | RISC-V 输出 |
|-------|------------|
| `LtOp(a, b)` | `lw t0, a(sp); lw t1, b(sp); slt t0, t0, t1; sw t0, result(sp)` |
| `LeOp(a, b)` | `lw t1, b(sp); lw t0, a(sp); slt t0, t1, t0; xori t0, t0, 1` |
| `EqOp(a, b)` | `xor t0, t0, t1; seqz t0, t0` |
| `NeOp(a, b)` | `xor t0, t0, t1; snez t0, t0` |
| `LtFOp(a, b)` | `flw ft0, a(sp); flw ft1, b(sp); flt.s t0, ft0, ft1` |
| `LeFOp` | `fle.s` |
| `EqFOp` | `feq.s` |
| `NeFOp` | `feq.s` + `xori` 取反 |

#### 类型转换

| IR Op | RISC-V 输出 |
|-------|------------|
| `I2FOp(a)` | `lw t0, a(sp); fcvt.s.w ft0, t0; fsw ft0, result(sp)` |
| `F2IOp(a)` | `flw ft0, a(sp); fcvt.w.s t0, ft0; sw t0, result(sp)` |
| `SetNotZeroOp(a)` | `lw t0, a(sp); snez t0, t0; sw t0, result(sp)` |

#### 控制流

| IR Op | RISC-V 输出 |
|-------|------------|
| `GotoOp <target=B>` | `j .L_B` |
| `BranchOp(cond) <true=T, false=F>` | `lw t0, cond(sp); bnez t0, .L_T; j .L_F` |
| `ReturnOp()` | 恢复 ra/s0 → `ret` |
| `ReturnOp(val)` | `lw a0, val(sp)` (整数) 或 `flw fa0, val(sp)` (浮点) → 恢复 → `ret` |

#### 函数调用

| IR Op | RISC-V 输出 |
|-------|------------|
| `FuncOp <name=f>` | `.globl f` + `f:` + 分配栈帧 + 保存 ra/s0 |
| `CallOp <callee=f>(args...)` | 前 8 个整数→a0-a7，前 8 个浮点→fa0-fa7，多余的压栈 → `call f` → 从 a0/fa0 取返回值 |

#### 数组地址计算

当前 IR 中数组地址通过 `MulIOp(index, stride)` + `AddLOp(base, offset)` 计算，直接按上面的算术规则翻译即可。

### 浮点常量处理

`FloatOp` 的值存为 IEEE 754 位模式。汇编中两种策略：

```asm
# 策略 A：放入 .rodata 段
.section .rodata
.LC0: .word 0x4048f5c3    # 3.14 的 IEEE 754
.text
    la t0, .LC1
    flw ft0, 0(t0)

# 策略 B：整数寄存器中转
    li t0, 0x4048f5c3
    fmv.w.x ft0, t0
```

方案一用策略 B（更简单，不需要管理常量池）。

### 运行时库对接

SysY 运行时库（`sylib`）需要一份 RISC-V 汇编实现：

- `getint` / `getch` / `getfloat`：通过 `scanf` 实现
- `putint` / `putch` / `putfloat`：通过 `printf` 实现
- `getarray` / `getfarray` / `putarray` / `putfarray`：通过 `scanf`/`printf` 循环
- `starttime` / `stoptime`：通过 `gettimeofday` 实现

可以用比赛提供的 `sylib.c` 编译为 `.o`，链接时直接用。

### 构建与测试

```bash
# 编译器生成 .s
./compiler --dump-asm test.sy -o test.s

# 交叉编译 + 链接
riscv64-unknown-linux-gnu-gcc -march=rv64if -static -o test test.s sylib.c

# QEMU 运行
qemu-riscv64 test < test.in
```

### 预计工作量

| 模块 | 代码量 | 说明 |
|------|--------|------|
| AsmEmitter | ~600-800 行 | 遍历 IR，逐 Op 翻译 |
| 栈帧管理 | ~100 行 | 帧大小计算、alloca 偏移记录 |
| sylib.s | ~200 行 | 运行时库（或直接用 C 版编译） |
| main.cpp 改动 | ~30 行 | 加 `--dump-asm` 参数 |
| **合计** | **~1000 行** | **1-2 周可完成** |

---

## 方案二：比赛级——MIR + 寄存器分配

### 核心思路

在 IR 和汇编之间插入 Machine IR（MIR），使用虚拟寄存器，然后做寄存器分配，最后打印汇编。

### 流水线

```
IR (Region/BasicBlock/Op)
  │
  ▼
ISel (Instruction Selection)    ← IR Op → MIR 指令，使用虚拟寄存器
  │
  ▼
MIR (MachineFunction/MachineBlock/MachineInst)
  │
  ▼
Liveness Analysis               ← 计算每个虚拟寄存器的活跃区间
  │
  ▼
Register Allocation             ← 虚拟寄存器 → 物理寄存器 + spill
  │
  ▼
Frame Lowering                  ← 确定栈帧布局、溢出槽位置
  │
  ▼
AsmPrinter                      ← MIR → 汇编文本
  │
  ▼
.s 文件
```

### 文件规划

```
src/asm/
├── RISCVDefs.h                    ← 寄存器枚举、指令枚举、操作数类型
├── MIR.h / MIR.cpp                ← MachineFunction / MachineBlock / MachineInst
├── ISel.h / ISel.cpp              ← IR → MIR 指令选择
├── LiveInterval.h / LiveInterval.cpp  ← 活跃区间计算
├── RegAlloc.h / RegAlloc.cpp      ← 寄存器分配（Linear Scan 或 Graph Coloring）
├── FrameContext.h / FrameContext.cpp  ← 栈帧布局
├── AsmPrinter.h / AsmPrinter.cpp  ← MIR → 汇编文本输出
└── RISCVRuntime.h / RISCVRuntime.cpp ← 运行时库包装（可选）
```

### 阶段详解

#### 阶段 1：RISCVDefs.h — 指令与寄存器定义

```cpp
// 物理寄存器
enum class PhysReg {
    // 整数
    ZERO = 0, RA = 1, SP = 2, GP = 3, TP = 4,
    T0 = 5, T1 = 6, T2 = 7,
    S0 = 8, S1 = 9,                                      // callee-saved
    A0 = 10, A1 = 11, A2 = 12, ..., A7 = 17,            // 参数/返回值
    S2 = 18, ..., S11 = 27,                              // callee-saved
    T3 = 28, T4 = 29, T5 = 30, T6 = 31,
    // 浮点
    FT0 = 32, FT1 = 33, ..., FT7 = 39,                  // caller-saved
    FS0 = 40, FS1 = 41,                                  // callee-saved
    FA0 = 42, FA1 = 43, ..., FA7 = 49,                   // 浮点参数/返回值
    FS2 = 50, ..., FS11 = 59,                            // callee-saved
    FT8 = 60, ..., FT11 = 63,                            // caller-saved
};

// 指令
enum class RISCVInst {
    ADD, ADDI, SUB, MUL, DIV, REM,
    ADDW, SUBW, MULW, DIVW, REMW,    // 32 位整数（RV64）
    AND, OR, XOR, SLL, SRL, SRA,
    SLT, SLTU, SEQZ, SNEZ,
    LW, SW, LH, SH, LB, SB,          // 整数 load/store
    LUI, AUIPC,
    J, JR, JAL, JALR, RET,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // 浮点
    FADD_S, FSUB_S, FMUL_S, FDIV_S,
    FMV_W_X, FMV_X_W,                // 整数寄存器 ↔ 浮点寄存器
    FCVT_S_W, FCVT_W_S,              // int ↔ float 转换
    FNEG_S, FABS_S,
    FLW, FSW,                         // 浮点 load/store
    FEQ_S, FLT_S, FLE_S,             // 浮点比较，结果写入整数寄存器
    // 伪指令
    LI, LA, MV, CALL, NOP,
    // 标记
    LABEL,
};

// 操作数
struct MachineOperand {
    enum Kind { VReg, PhysReg, ImmI, ImmF, Label, GlobalAddr };
    // ...
};
```

#### 阶段 2：ISel — 指令选择

逐 IR Op 映射为 MIR 指令序列，结果用虚拟寄存器（`v%0`, `v%1`, ...）：

```
IR:  %5 = addi (%3, %4)
  ↓
MIR:  v%3 = lw   (stack_slot[%3])
      v%4 = lw   (stack_slot[%4])
      v%5 = add  (v%3, v%4)
      sw  (v%5, stack_slot[%5])
```

实际上 ISel 阶段先全部用虚拟寄存器，不关心栈槽：

```
IR:  %5 = addi (%3, %4)
  ↓
MIR:  v%5 = add (v%3, v%4)
```

Load/Store/Alloca 单独处理为内存操作。

#### 阶段 3：Liveness Analysis

对每个基本块反向扫描，计算每个虚拟寄存器的活跃区间 `[def, last_use]`：

```
Block entry:
  v%3 = ...           ← def at 0
  v%4 = ...           ← def at 1
  v%5 = add v%3, v%4  ← use at 2, v%3 live [0,2], v%4 live [1,2]
  ...
```

跨基本块的活跃性通过 CFG 前驱/后继传播。

#### 阶段 4：寄存器分配

**方案 A：Linear Scan（推荐，实现较简单）**

```
1. 按活跃区间起始点排序
2. 遍历每个区间：
   - 尝试分配一个空闲物理寄存器
   - 无空闲时，溢出（spill）最晚结束的区间到栈
   - 如果当前区间比被溢出的更早结束，交换
3. 在 use 前插入 reload，在 def 后插入 spill（如有需要）
```

**方案 B：Graph Coloring（性能更好，实现复杂）**

```
1. 构建干涉图（活跃区间重叠的虚拟寄存器之间连边）
2. 简化：移除度 < K 的节点
3. 选择：逆序恢复节点，分配颜色（物理寄存器）
4. 溢出：无法着色的节点 spill 到栈
5. 合并：合并 move 相关的节点
```

寄存器分配策略选择：

| 策略 | 优点 | 缺点 |
|------|------|------|
| Linear Scan | 实现简单，编译快 | 寄存器利用率略低 |
| Graph Coloring | 寄存器利用率高 | 实现复杂，编译慢 |
| **推荐** | 先用 LSRA 跑通，有余力再上 GC | - |

#### 阶段 5：Frame Context — 栈帧布局

```
┌─────────────────────────┐ ← 高地址 (调用者栈帧)
│  调用者传的第 9+ 个参数   │
├─────────────────────────┤ ← callee 的 fp
│  ra                     │
│  s0-s11 (callee-saved)  │   按需保存
│  fs0-fs11 (callee-saved)│   按需保存
├─────────────────────────┤
│  alloca 局部变量          │
├─────────────────────────┤
│  spill 溢出槽            │   寄存器分配器报告的溢出数
├─────────────────────────┤
│  outgoing args (≥8 时)  │   为被调函数准备的参数区
└─────────────────────────┘ ← sp
```

帧大小 = 所有区域之和，对齐到 16 字节。

#### 阶段 6：AsmPrinter — 输出汇编

遍历 MIR，逐指令翻译为汇编文本。处理：

- 虚拟寄存器替换为物理寄存器名
- 溢出槽替换为 `sp + offset`
- 标签输出为 `.L_xxx:`
- 函数序言/尾声插入

### 调用约定

遵循标准 RISC-V 调用约定（LP64D 的子集，因为只用 F 单精度）：

| 类型 | 参数传递 | 返回值 |
|------|---------|--------|
| 整数 | a0-a7（前 8 个），多余的从右到左压栈 | a0 |
| 浮点 | fa0-fa7（前 8 个），多余的从右到左压栈 | fa0 |
| 数组/指针 | 当作整数，走 a0-a7 | a0 |

caller-saved：ra, t0-t6, a0-a7, ft0-ft11, fa0-fa7
callee-saved：s0-s11, fs0-fs11（需要在序言保存、尾声恢复）

### 预计工作量

| 模块 | 代码量 | 说明 |
|------|--------|------|
| RISCVDefs.h | ~150 行 | 寄存器/指令/操作数定义 |
| MIR.h/cpp | ~200 行 | MachineFunction/Block/Inst 数据结构 |
| ISel | ~400 行 | IR→MIR 翻译 |
| Liveness | ~150 行 | 活跃区间分析 |
| RegAlloc (LSRA) | ~300 行 | Linear Scan 寄存器分配 |
| FrameContext | ~150 行 | 栈帧计算 |
| AsmPrinter | ~400 行 | MIR→汇编输出 |
| sylib.s | ~200 行 | 运行时库 |
| main.cpp 改动 | ~30 行 | 新增 --dump-asm 参数 |
| **合计** | **~2000 行** | **3-4 周** |

### 扩展路线

方案二的 MIR 架构天然支持后续后端优化：

```
ISel → [后端优化] → RegAlloc → FrameLower → AsmPrinter
              │
              ├─ Peephole（窥孔优化）
              ├─ 窥孔强度削减（乘除→移位）
              ├─ 冗余 load/store 消除
              ├─ 指令调度
              ├─ 分支预测提示
              └─ 常量传播
```

这些是 `opt.md` 之外的后端专属优化，可以按比赛节奏逐步加入。

---

## 两方案对比

| 维度 | 方案一 | 方案二 |
|------|--------|--------|
| 代码量 | ~1000 行 | ~2000 行 |
| 时间 | 1-2 周 | 3-4 周 |
| 生成代码质量 | 差（全走栈，大量 lw/sw） | 好（物理寄存器，少量 spill） |
| 运行速度 | 慢 5-10 倍 | 接近手写 |
| 架构扩展性 | 差（改动大） | 好（MIR 层可插拔优化） |
| 调试难度 | 低（直译，好定位） | 中（多了一层抽象） |
| 比赛可用性 | 能跑功能测试 | 能跑性能测试 |
| 后端优化 | 无法加 | 天然支持 |

### 建议路径

先用方案一快速跑通功能测试（验证 IR 正确性 + 汇编输出正确性），然后在方案一的基础上重构为方案二。两者的 AsmPrinter 部分可以复用，主要增量是插入 MIR 层和寄存器分配。
