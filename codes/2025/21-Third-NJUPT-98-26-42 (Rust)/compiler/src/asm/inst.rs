//! 汇编指令
//!
#![allow(non_upper_case_globals)]

use super::{reg::*, stack::*};
use crate::prelude::*;
use std::fmt::Debug;

// ===================================================================
// instructions meta info
// ===================================================================

use crate::define_index;

// 定义InstColor为类型化索引
// 副作用颜色 -- 为了指令融合/指令重排的"状态版本"
// 颜色单调递增，相同颜色内的指令可以自由重排序
// 1. 通过side-effecting来对同一个块里的操作进行区分
//     所有具有相同"颜色"的指令，保证不会被任何具有副作用的操作隔开
//     因此只有在一个基本块中的操作才能拥有相同的颜色, 并且相同颜色意味着能够自由重排
// 2. "颜色"变化的边界是那些具有副作用的指令
//     比如load/store/call等都被视作副作用指令
// 3. 任意两条颜色相同的指令, 要么A支配B, 要么B后置支配A
define_index!(InstColor);

/// 指令输出类型 -- ISLE降级结果
#[derive(Debug, Clone, Copy)]
pub enum InstOutput {
    OutputNone,             // 副作用指令如store
    OutputReg { reg: Reg }, // 合并了Reg/FReg/XReg
}

/// 输入源指令
/// 回答了“我们能对这个源指令做什么？”这个问题, 用于表示一个值的来源, 决定是否可以指令融合
#[derive(Clone, Copy, Debug)]
pub enum InputSourceInst {
    /// 最佳情况 -- 处理的地方是该指令的唯一使用者
    /// 可以安全将该指令sink到此处, 并进行融合, 同时删除源指令
    UniqueUse(Inst, usize),
    /// 需要权衡的情况 -- 源指令的结果还有其他使用者
    /// 但是仍然可以sink, 通过duplicate源指令计算的方法
    /// 如果源指令廉价(例如加法指令可以sink到load/store的立即数偏移量), 则进行复制
    /// 如果源指令成本高昂, 例如udiv, 就不会进行复制
    Use(Inst, usize),
    /// 无法或不应融合的情况 -- 安全回退, 表示指令无法融合
    None,
}
impl InputSourceInst {
    pub fn as_inst(&self) -> Option<(Inst, usize)> {
        match self {
            &InputSourceInst::UniqueUse(inst, output_idx)
            | &InputSourceInst::Use(inst, output_idx) => Some((inst, output_idx)),
            &InputSourceInst::None => None,
        }
    }
}

/// 输入值的非寄存器表示
/// 描述了一个输入值可以来自的两种“非寄存器”来源:
/// 1. 已知的常量: 如果较小就无需先把它加载到寄存器里，再进行计算, 可以进行立刻的常量折叠
/// 2. 由另一条指令"就地产生": 如果输入是由前一条指令算出来的, 不一定非要把前一条指令放入寄存器, 可以"就地产生"--sink/fusion
#[derive(Clone, Copy, Debug)]
pub struct NonRegInput {
    pub inst: InputSourceInst,
    pub constant: Option<u64>,
}

/// 传递性唯一分析
/// 代表的不是“一个值被直接使用了多少次”
/// 而是“如果尽可能进行指令融合，这个值对应的操作最终在多少个地方被生成代码”
///
/// 这个状态分析了如下问题:
///
/// ```riscv
/// v1 = load.i32 [r_addr]  ; 指令A。v1 的直接使用者只有 v2，看起来很安全
/// v2 = iadd v1, 1         ; 指令B。v2 的直接使用者只有 v3，看起来很安全
///
/// v3 = imul v2, 10        ; 指令C, 但是 v3 被两个地方使用
/// v4 = foo v3             ; 指令D
/// v5 = bar v3             ; 指令E
/// ```
///
/// 在这种情况下, 如果仅仅因为分析v2的操作数均为Once, 就进行复制是十分危险的
/// 因此需要进行"传染性"地分析每一个值的使用次数, 以回答:
/// "如果我现在把值v进行融合, 会不会导致因为下游的值的多重使用, 导致无意中复制了一个昂贵且带有副作用的操作"
///
/// Root Instruction -- Multiple传染的防火墙
/// 根指令会阻断Multiple状态的向上传播, 因为根指令永远不会被融合, 只会被生成一次
/// ----但值得注意的是, 在RISCV下极少有有真正的根指令
///
/// 因此理论上单返回值 Call 可以被标记为融合 --- 但阻止Call的融合, 是依靠InstColor
/// Call被视作副作用指令
#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum ValueUseState {
    /// 完全未使用
    #[default]
    Unused,
    /// 在整条使用链上是唯一状态, 可以被安全融合
    Once,
    /// 在使用链上, 要么被直接使用了多次, 要么被间接使用了很多次
    /// 这个Multiple的传播, 实际上是"传染"
    /// 如果在确定了复制的成本很低, 那就可以放心地融合
    Multiple,
}

impl ValueUseState {
    /// 增加一次使用
    pub fn inc(&mut self) {
        let new = match self {
            Self::Unused => Self::Once,
            Self::Once | Self::Multiple => Self::Multiple,
        };
        *self = new;
    }
}

// ===================================================================
// ABI 参数传递结构
// ===================================================================

/// 函数参数/返回值的ABI约束表示
/// 描述一个参数/返回值的虚拟寄存器与其ABI物理位置的绑定关系
#[derive(Debug, Clone, Copy, PartialEq, Eq, strum::Display)]
pub enum ArgPair {
    /// 寄存器参数位置
    /// vreg: 虚拟寄存器（分配前）
    /// preg: ABI物理寄存器（如a0-a7, fa0-fa7）
    #[strum(serialize = "{vreg}({preg})")]
    RegArg { vreg: Reg, preg: Reg },

    /// 栈参数位置
    /// vreg: 虚拟寄存器（分配前）
    /// slot: 栈槽位置（incoming_args或outgoing_args）
    #[strum(serialize = "{vreg}(slot:{slot}-{kind})")]
    StackArg {
        vreg: Reg,
        slot: SlotId,
        kind: SlotKindDisc,
    },

    /// 无值（用于void返回类型）
    #[strum(serialize = "_")]
    None,
}

/// 参数列表类型
pub type ArgPairVec = Vec<ArgPair>;

// ===================================================================
// instructions enum
// ===================================================================

// 虚拟汇编&汇编共用一套指令集
pub type VirtInst = MInst;
pub type RvInst = MInst;

// 立即数
pub type Imm5 = u8; // 32位移位 0-31
pub type Imm6 = u8; // 64位移位 0-63
pub type Imm8 = u8;
pub type Imm12 = i16; // I -2048 to 2047
pub type Imm20 = i32; // J -524288 to 524287
pub type UImm20 = u32; // U 0 to 1048575
pub type Imm64 = i64; // li

pub const Imm12_Zero: Imm12 = 0;
pub const Imm12_MinusOne: Imm12 = -1;
pub const Imm64_Zero: Imm64 = 0;
pub const OptionReg_None: OptionReg = None;

// Unit类型 - 空类型，用于表示没有返回值的操作
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Unit;

// ISLE 需要的类型别名
pub type Label = String;
pub type LabelVec = Vec<Label>; // TODO: 性能很差, 不过不用在意
pub type ValueSlice = Vec<Value>;
pub type RegsVec = Vec<Reg>; // 用于伪指令Call的参数
pub type OptionReg = Option<Reg>; // 用于可选返回值

/// 检查偏移量是否在 12 位立即数范围内
#[inline]
pub fn is_imm12_valid(offset: i64) -> bool {
    (-2048..=2047).contains(&offset)
}

/// 指令枚举
#[derive(Debug, Clone, strum::Display)]
pub enum MInst {
    // === 伪指令 -- 在寄存器分配后展开 ===
    /// 溢出寄存器到栈槽
    #[strum(serialize = "spill {reg} -> slot#{slot}-{kind}")]
    Spill {
        reg: Reg,
        slot: SlotId,
        kind: SlotKindDisc,
    },

    /// 从栈槽重载到寄存器
    #[strum(serialize = "reload slot#{slot}-{kind} -> {reg}")]
    Reload {
        reg: Reg,
        slot: SlotId,
        kind: SlotKindDisc,
    },

    /// 获取栈槽地址
    #[strum(serialize = "addr slot#{slot}-{kind} -> {dst}")]
    AddrSlot {
        dst: XReg,
        slot: SlotId,
        kind: SlotKindDisc,
    },

    /// 汇编注释
    #[strum(serialize = "# {label}")]
    Comment { label: Label },

    /// 调用伪指令 -- 用于处理ABI约束
    #[strum(serialize = "call_pseudo {label}, {ret} #, {args:?}")]
    CallPseudo {
        label: Label,
        ret: ArgPair,
        args: ArgPairVec,
    },

    /// 返回伪指令 -- 用于处理ABI约束
    #[strum(serialize = "ret_pseudo {ret}")]
    RetPseudo { ret: ArgPair },

    /// 尾调用伪指令 -- 用于处理ABI约束
    #[strum(serialize = "ret_call_pseudo {label} #, {args:?}")]
    RetCallPseudo { label: Label, args: ArgPairVec },

    /// 函数序言：调整栈指针
    #[strum(serialize = "prologue_sp_adjust")]
    PrologueSPAdjust,

    /// 函数序言：设置帧指针
    #[strum(serialize = "prologue_fp_setup")]
    PrologueFPSetup,

    /// 函数尾声：恢复栈指针
    #[strum(serialize = "epilogue_sp_restore")]
    EpilogueSPRestore,

    // === 真实指令 ===
    // 整数加载/存储
    #[strum(serialize = "li {rd}, {imm}")]
    Li { rd: XReg, imm: Imm64 }, // lui+addi
    #[strum(serialize = "la {rd}, {label}")]
    La { rd: XReg, label: Label }, // auipc+addi

    #[strum(serialize = "lw {rd}, {offset}({rs})")]
    Lw { rd: XReg, rs: XReg, offset: Imm12 }, // 符号扩展
    #[strum(serialize = "lwu {rd}, {offset}({rs})")]
    Lwu { rd: XReg, rs: XReg, offset: Imm12 }, // 零扩展
    #[strum(serialize = "ld {rd}, {offset}({rs})")]
    Ld { rd: XReg, rs: XReg, offset: Imm12 },

    #[strum(serialize = "sw {rs2}, {offset}({rs1})")]
    Sw { rs2: XReg, rs1: XReg, offset: Imm12 },
    #[strum(serialize = "sd {rs2}, {offset}({rs1})")]
    Sd { rs2: XReg, rs1: XReg, offset: Imm12 },
    #[strum(serialize = "mv {rd}, {rs}")]
    Mv { rd: XReg, rs: XReg }, // addi rd, rs, 0

    // 符号扩展
    #[strum(serialize = "sext.w {rd}, {rs}")]
    Sextw { rd: XReg, rs: XReg }, // addiw rd, rs, 0 (i32 -> i64)

    // I64
    #[strum(serialize = "add {rd}, {rs1}, {rs2}")]
    Add { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "addi {rd}, {rs}, {imm}")]
    Addi { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "sub {rd}, {rs1}, {rs2}")]
    Sub { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "mul {rd}, {rs1}, {rs2}")]
    Mul { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "div {rd}, {rs1}, {rs2}")]
    Div { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "rem {rd}, {rs1}, {rs2}")]
    Rem { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "divu {rd}, {rs1}, {rs2}")]
    Divu { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "remu {rd}, {rs1}, {rs2}")]
    Remu { rd: XReg, rs1: XReg, rs2: XReg },

    // I32
    #[strum(serialize = "addw {rd}, {rs1}, {rs2}")]
    Addw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "addiw {rd}, {rs}, {imm}")]
    Addiw { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "subw {rd}, {rs1}, {rs2}")]
    Subw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "mulw {rd}, {rs1}, {rs2}")]
    Mulw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "divw {rd}, {rs1}, {rs2}")]
    Divw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "remw {rd}, {rs1}, {rs2}")]
    Remw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "divuw {rd}, {rs1}, {rs2}")]
    Divuw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "remuw {rd}, {rs1}, {rs2}")]
    Remuw { rd: XReg, rs1: XReg, rs2: XReg },

    // 逻辑运算
    #[strum(serialize = "xor {rd}, {rs1}, {rs2}")]
    Xor { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "xori {rd}, {rs}, {imm}")]
    Xori { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "or {rd}, {rs1}, {rs2}")]
    Or { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "ori {rd}, {rs}, {imm}")]
    Ori { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "and {rd}, {rs1}, {rs2}")]
    And { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "andi {rd}, {rs}, {imm}")]
    Andi { rd: XReg, rs: XReg, imm: Imm12 },

    // I64移位
    #[strum(serialize = "sll {rd}, {rs1}, {rs2}")]
    Sll { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "slli {rd}, {rs}, {shamt}")]
    Slli { rd: XReg, rs: XReg, shamt: Imm6 }, // RV64需要6位立即数
    #[strum(serialize = "srl {rd}, {rs1}, {rs2}")]
    Srl { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "srli {rd}, {rs}, {shamt}")]
    Srli { rd: XReg, rs: XReg, shamt: Imm6 }, // RV64需要6位立即数
    #[strum(serialize = "sra {rd}, {rs1}, {rs2}")]
    Sra { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "srai {rd}, {rs}, {shamt}")]
    Srai { rd: XReg, rs: XReg, shamt: Imm6 }, // RV64需要6位立即数

    // I32移位
    #[strum(serialize = "sllw {rd}, {rs1}, {rs2}")]
    Sllw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "slliw {rd}, {rs}, {shamt}")]
    Slliw { rd: XReg, rs: XReg, shamt: Imm5 },
    #[strum(serialize = "srlw {rd}, {rs1}, {rs2}")]
    Srlw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "srliw {rd}, {rs}, {shamt}")]
    Srliw { rd: XReg, rs: XReg, shamt: Imm5 },
    #[strum(serialize = "sraw {rd}, {rs1}, {rs2}")]
    Sraw { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "sraiw {rd}, {rs}, {shamt}")]
    Sraiw { rd: XReg, rs: XReg, shamt: Imm5 },

    // I64比较
    #[strum(serialize = "slt {rd}, {rs1}, {rs2}")]
    Slt { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "slti {rd}, {rs}, {imm}")]
    Slti { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "sltu {rd}, {rs1}, {rs2}")]
    Sltu { rd: XReg, rs1: XReg, rs2: XReg },
    #[strum(serialize = "sltiu {rd}, {rs}, {imm}")]
    Sltiu { rd: XReg, rs: XReg, imm: Imm12 },
    #[strum(serialize = "seqz {rd}, {rs}")]
    Seqz { rd: XReg, rs: XReg }, // sltiu rd, rs, 1
    #[strum(serialize = "snez {rd}, {rs}")]
    Snez { rd: XReg, rs: XReg }, // sltu rd, zero, rs

    // 浮点加载/存储
    #[strum(serialize = "flw {fd}, {offset}({rs})")]
    Flw { fd: FReg, rs: XReg, offset: Imm12 },
    #[strum(serialize = "fld {fd}, {offset}({rs})")]
    Fld { fd: FReg, rs: XReg, offset: Imm12 },
    #[strum(serialize = "fsw {fs}, {offset}({rs})")]
    Fsw { fs: FReg, rs: XReg, offset: Imm12 },
    #[strum(serialize = "fsd {fs}, {offset}({rs})")]
    Fsd { fs: FReg, rs: XReg, offset: Imm12 },
    #[strum(serialize = "fmv.s {fd}, {fs}")]
    FmvS { fd: FReg, fs: FReg }, // fsgnj.s fd, fs, fs (f32复制)
    #[strum(serialize = "fmv.d {fd}, {fs}")]
    FmvD { fd: FReg, fs: FReg }, // fsgnj.d fd, fs, fs (f64复制)

    // 整数寄存器到浮点寄存器的位移动
    #[strum(serialize = "fmv.w.x {fd}, {rs}")]
    FmvWX { fd: FReg, rs: XReg }, // 将整数寄存器的低32位移到单精度浮点寄存器
    #[strum(serialize = "fmv.d.x {fd}, {rs}")]
    FmvDX { fd: FReg, rs: XReg }, // 将整数寄存器的64位移到双精度浮点寄存器

    // 整数/浮点转换
    #[strum(serialize = "fcvt.s.w {fd}, {rs}")]
    FcvtSW { fd: FReg, rs: XReg }, // i32 -> f32
    #[strum(serialize = "fcvt.s.wu {fd}, {rs}")]
    FcvtSWu { fd: FReg, rs: XReg }, // u32 -> f32
    #[strum(serialize = "fcvt.s.l {fd}, {rs}")]
    FcvtSL { fd: FReg, rs: XReg }, // i64 -> f32
    #[strum(serialize = "fcvt.s.lu {fd}, {rs}")]
    FcvtSLu { fd: FReg, rs: XReg }, // u64 -> f32
    #[strum(serialize = "fcvt.d.w {fd}, {rs}")]
    FcvtDW { fd: FReg, rs: XReg }, // i32 -> f64
    #[strum(serialize = "fcvt.d.wu {fd}, {rs}")]
    FcvtDWu { fd: FReg, rs: XReg }, // u32 -> f64
    #[strum(serialize = "fcvt.d.l {fd}, {rs}")]
    FcvtDL { fd: FReg, rs: XReg }, // i64 -> f64
    #[strum(serialize = "fcvt.d.lu {fd}, {rs}")]
    FcvtDLu { fd: FReg, rs: XReg }, // u64 -> f64

    #[strum(serialize = "fcvt.w.s {rd}, {fs}, rtz")]
    FcvtWS { rd: XReg, fs: FReg }, // f32 -> i32
    #[strum(serialize = "fcvt.wu.s {rd}, {fs}, rtz")]
    FcvtWuS { rd: XReg, fs: FReg }, // f32 -> u32
    #[strum(serialize = "fcvt.l.s {rd}, {fs}, rtz")]
    FcvtLS { rd: XReg, fs: FReg }, // f32 -> i64
    #[strum(serialize = "fcvt.lu.s {rd}, {fs}, rtz")]
    FcvtLuS { rd: XReg, fs: FReg }, // f32 -> u64
    #[strum(serialize = "fcvt.w.d {rd}, {fs}, rtz")]
    FcvtWD { rd: XReg, fs: FReg }, // f64 -> i32
    #[strum(serialize = "fcvt.wu.d {rd}, {fs}, rtz")]
    FcvtWuD { rd: XReg, fs: FReg }, // f64 -> u32
    #[strum(serialize = "fcvt.l.d {rd}, {fs}, rtz")]
    FcvtLD { rd: XReg, fs: FReg }, // f64 -> i64
    #[strum(serialize = "fcvt.lu.d {rd}, {fs}, rtz")]
    FcvtLuD { rd: XReg, fs: FReg }, // f64 -> u64

    // 浮点转换
    #[strum(serialize = "fcvt.s.d {fd}, {fs}")]
    FcvtSD { fd: FReg, fs: FReg }, // f32 -> f64
    #[strum(serialize = "fcvt.d.s {fd}, {fs}")]
    FcvtDS { fd: FReg, fs: FReg }, // f64 -> f32

    // F32
    #[strum(serialize = "fadd.s {fd}, {fs1}, {fs2}")]
    FaddS { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fsub.s {fd}, {fs1}, {fs2}")]
    FsubS { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fmul.s {fd}, {fs1}, {fs2}")]
    FmulS { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fdiv.s {fd}, {fs1}, {fs2}")]
    FdivS { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "feq.s {rd}, {fs1}, {fs2}")]
    FeqS { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "flt.s {rd}, {fs1}, {fs2}")]
    FltS { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fle.s {rd}, {fs1}, {fs2}")]
    FleS { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fneg.s {fd}, {fs}")]
    FnegS { fd: FReg, fs: FReg }, // fsgnjn.s fd, fs, fs

    // F64
    #[strum(serialize = "fadd.d {fd}, {fs1}, {fs2}")]
    FaddD { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fsub.d {fd}, {fs1}, {fs2}")]
    FsubD { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fmul.d {fd}, {fs1}, {fs2}")]
    FmulD { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fdiv.d {fd}, {fs1}, {fs2}")]
    FdivD { fd: FReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "feq.d {rd}, {fs1}, {fs2}")]
    FeqD { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "flt.d {rd}, {fs1}, {fs2}")]
    FltD { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fle.d {rd}, {fs1}, {fs2}")]
    FleD { rd: XReg, fs1: FReg, fs2: FReg },
    #[strum(serialize = "fneg.d {fd}, {fs}")]
    FnegD { fd: FReg, fs: FReg }, // fsgnjn.d fd, fs, fs

    // === Fused Multiply-Add ===
    // FMA for Single-Precision (f32)
    #[strum(serialize = "fmadd.s {fd}, {fs1}, {fs2}, {fs3}")]
    FmaddS {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fmsub.s {fd}, {fs1}, {fs2}, {fs3}")]
    FmsubS {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fnmsub.s {fd}, {fs1}, {fs2}, {fs3}")]
    FnmsubS {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fnmadd.s {fd}, {fs1}, {fs2}, {fs3}")]
    FnmaddS {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },

    // FMA for Double-Precision (f64)
    #[strum(serialize = "fmadd.d {fd}, {fs1}, {fs2}, {fs3}")]
    FmaddD {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fmsub.d {fd}, {fs1}, {fs2}, {fs3}")]
    FmsubD {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fnmsub.d {fd}, {fs1}, {fs2}, {fs3}")]
    FnmsubD {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },
    #[strum(serialize = "fnmadd.d {fd}, {fs1}, {fs2}, {fs3}")]
    FnmaddD {
        fd: FReg,
        fs1: FReg,
        fs2: FReg,
        fs3: FReg,
    },

    // 控制流
    #[strum(serialize = "j {label}")]
    J { label: Label }, // jal zero, label
    #[strum(serialize = "call {label}")]
    Call { label: Label }, // jal ra, label
    #[strum(serialize = "ret")]
    Ret, // jalr zero, ra, 0
    #[strum(serialize = "beq {rs1}, {rs2}, {label}")]
    Beq { rs1: XReg, rs2: XReg, label: Label },
    #[strum(serialize = "bne {rs1}, {rs2}, {label}")]
    Bne { rs1: XReg, rs2: XReg, label: Label },
    #[strum(serialize = "blt {rs1}, {rs2}, {label}")]
    Blt { rs1: XReg, rs2: XReg, label: Label },
    #[strum(serialize = "bge {rs1}, {rs2}, {label}")]
    Bge { rs1: XReg, rs2: XReg, label: Label },
    #[strum(serialize = "bltu {rs1}, {rs2}, {label}")]
    Bltu { rs1: XReg, rs2: XReg, label: Label },
    #[strum(serialize = "bgeu {rs1}, {rs2}, {label}")]
    Bgeu { rs1: XReg, rs2: XReg, label: Label },

    // 其他
    #[strum(serialize = "nop")]
    Nop, // addi zero, zero, 0
    #[strum(serialize = "fence iorw, iorw")]
    Fence, // TODO: 内存屏障(用于循环并行化) fence iorw, iorw
}

// ===================================================================
// instructions impl
// ===================================================================

/// 指令访问器 trait - 用于遍历指令中的寄存器
pub trait InstVisitor {
    /// 访问定义的寄存器
    fn visit_def(&mut self, reg: Reg);

    /// 访问使用的寄存器  
    fn visit_use(&mut self, reg: Reg);
}

/// 寄存器映射器 trait - 用于替换指令中的寄存器
pub trait RegMapper {
    fn map_reg(&mut self, reg: Reg) -> Reg;

    fn map_xreg(&mut self, xreg: XReg) -> XReg {
        // 默认实现：通过 Reg 转换
        let reg = xreg.to_reg();
        let mapped = self.map_reg(reg);
        XReg::try_from(mapped).expect("RegMapper returned non-XReg for XReg input")
    }

    fn map_freg(&mut self, freg: FReg) -> FReg {
        // 默认实现：通过 Reg 转换
        let reg = freg.to_reg();
        let mapped = self.map_reg(reg);
        FReg::try_from(mapped).expect("RegMapper returned non-FReg for FReg input")
    }
}

/// 收集寄存器的访问器实现
pub struct RegCollector {
    pub defs: Vec<Reg>,
    pub uses: Vec<Reg>,
}

impl Default for RegCollector {
    fn default() -> Self {
        Self::new()
    }
}

impl RegCollector {
    pub fn new() -> Self {
        Self {
            defs: Vec::new(),
            uses: Vec::new(),
        }
    }
}

impl InstVisitor for RegCollector {
    fn visit_def(&mut self, reg: Reg) {
        // 过滤掉zero寄存器 - 它永远不会被定义
        if !matches!(reg, Reg::X(RvXReg::Zero)) {
            self.defs.push(reg);
        }
    }

    fn visit_use(&mut self, reg: Reg) {
        // 过滤掉zero寄存器 - 它是硬编码的常量0，不参与寄存器分配
        if !matches!(reg, Reg::X(RvXReg::Zero)) {
            self.uses.push(reg);
        }
    }
}

impl MInst {
    /// 判断是否为伪指令
    pub fn is_pseudo(&self) -> bool {
        matches!(
            self,
            MInst::Spill { .. }
                | MInst::Reload { .. }
                | MInst::AddrSlot { .. }
                | MInst::Comment { .. }
        )
    }

    pub fn is_call(&self) -> bool {
        matches!(
            self,
            MInst::Call { .. } | MInst::CallPseudo { .. } | MInst::RetCallPseudo { .. }
        )
    }

    pub fn is_pseudo_call_like(&self) -> bool {
        matches!(self, MInst::CallPseudo { .. } | MInst::RetCallPseudo { .. })
    }

    pub fn extractor_pseudo_call_like(&self) -> Option<(&ArgPairVec, Option<&ArgPair>)> {
        match self {
            MInst::CallPseudo { ret, args, .. } => Some((args, Some(ret))),
            MInst::RetCallPseudo { args, .. } => Some((args, None)),
            _ => None,
        }
    }

    pub fn is_comment(&self) -> bool {
        matches!(self, MInst::Comment { .. })
    }

    /// 判断是否为终结指令（控制流指令）
    pub fn is_terminator(&self) -> bool {
        matches!(
            self,
            MInst::J { .. }
                | MInst::Ret
                | MInst::Beq { .. }
                | MInst::Bne { .. }
                | MInst::Blt { .. }
                | MInst::Bge { .. }
                | MInst::Bltu { .. }
                | MInst::Bgeu { .. }
                | MInst::RetPseudo { .. }
                | MInst::RetCallPseudo { .. }
        )
    }

    /// 展开伪指令为真实指令序列
    /// sp_offset: 栈槽地址计算函数
    ///   - 输入: (slot, &mut Vec<MInst>)
    ///   - 输出: (基址寄存器, 偏移量)
    ///   - 对于小偏移量：返回 (sp, offset) 或者 (fp, offset)
    ///   - 对于大偏移量：可以插入额外指令到Vec并返回 (temp_reg, small_offset)
    pub fn expand_pseudo<F>(&self, mut sp_offset: F) -> Option<Vec<MInst>>
    where
        F: FnMut(SlotId, &mut Vec<MInst>) -> (XReg, Imm12),
    {
        let mut insts = Vec::new();

        match self {
            MInst::Spill { reg, slot, .. } => {
                let (base, offset) = sp_offset(*slot, &mut insts);
                match reg {
                    Reg::X(xreg) => {
                        // sd reg, offset(base)
                        insts.push(MInst::Sd {
                            rs2: XReg::phys(*xreg),
                            rs1: base,
                            offset,
                        });
                        Some(insts)
                    }
                    Reg::F(freg) => {
                        // fsd reg, offset(base)
                        insts.push(MInst::Fsd {
                            fs: FReg::phys(*freg),
                            rs: base,
                            offset,
                        });
                        Some(insts)
                    }
                    _ => panic!("虚拟寄存器需要先分配: {:?}", reg),
                }
            }
            MInst::Reload { slot, reg, .. } => {
                let (base, offset) = sp_offset(*slot, &mut insts);
                match reg {
                    Reg::X(xreg) => {
                        // ld reg, offset(base)
                        insts.push(MInst::Ld {
                            rd: XReg::phys(*xreg),
                            rs: base,
                            offset,
                        });
                        Some(insts)
                    }
                    Reg::F(freg) => {
                        // fld reg, offset(base)
                        insts.push(MInst::Fld {
                            fd: FReg::phys(*freg),
                            rs: base,
                            offset,
                        });
                        Some(insts)
                    }
                    _ => panic!("虚拟寄存器需要先分配: {:?}", reg),
                }
            }
            MInst::AddrSlot { slot, dst, .. } => {
                let (base, offset) = sp_offset(*slot, &mut insts);
                // 根据不同情况生成地址计算指令
                if offset == 0 && base != XReg::phys(RvXReg::Sp) {
                    // base已经是完整地址，直接移动
                    insts.push(MInst::Mv { rd: *dst, rs: base });
                } else {
                    // 需要加上偏移量: addi dst, base, offset
                    insts.push(MInst::Addi {
                        rd: *dst,
                        rs: base,
                        imm: offset,
                    });
                }
                Some(insts)
            }
            // === 新增伪指令展开 ===
            MInst::CallPseudo { label, .. } => {
                // 注意: 在伪指令展开阶段，ABI约束已经在寄存器分配阶段处理完毕
                // 这里所有的参数寄存器和返回值寄存器都已经被分配到正确的物理寄存器
                // 参数应该已经在 a0-a7/fa0-fa7，返回值应该在 a0/fa0
                // 因此直接展开为 Call 指令
                insts.push(MInst::Call {
                    label: label.clone(),
                });
                Some(insts)
            }
            MInst::RetPseudo { .. } => {
                // 注意: 返回值寄存器已经被分配到 a0/fa0
                // 直接展开为 Ret 指令
                insts.push(MInst::Ret);
                Some(insts)
            }
            MInst::RetCallPseudo { label, .. } => {
                // 注意: 尾调用的参数已经被分配到正确的寄存器
                // 直接展开为 J 指令（尾调用优化）
                insts.push(MInst::J {
                    label: label.clone(),
                });
                Some(insts)
            }
            // 新的序言/尾声伪指令暂时不在这里展开
            // 它们将在专门的expand_all_pseudo_instructions中处理
            MInst::PrologueSPAdjust | MInst::PrologueFPSetup | MInst::EpilogueSPRestore => {
                Some(vec![self.clone()])
            }
            _ => {
                // 非伪指令，返回自身
                Some(vec![self.clone()])
            }
        }
    }

    /// visit方法
    pub fn visit_regs(&self, visitor: &mut dyn InstVisitor) {
        match self {
            // === 伪指令 ===
            MInst::Spill { reg, .. } => {
                visitor.visit_use(*reg);
            }
            MInst::Reload { reg, .. } => {
                visitor.visit_def(*reg);
            }
            MInst::AddrSlot { dst, .. } => {
                visitor.visit_def(dst.to_reg());
            }
            MInst::Comment { .. } => {
                // Comment指令不使用也不定义任何寄存器
            }
            // === 新增伪指令 ===
            MInst::CallPseudo { ret, args, .. } => {
                // 使用所有参数的虚拟寄存器
                // !!!重要!!!: StackArg 的 vreg 不应该被标记为 use!!!
                // 原因：
                // 1. StackArg 只是一个信息标记，表示参数通过栈传递
                // 2. 在 CallPseudo 之前，Lower 阶段已经插入了 Store 指令
                //    将 vreg 存储到 outgoing_args 区域（非持久化栈）
                // 3. outgoing_args 与 spill_slots 是完全不同的栈区域
                // 4. vreg 在 Store 之后就不再需要活跃
                // 5. 将 StackArg.vreg 标记为 use 会导致错误的 reload/spill 决策
                for arg in args {
                    match arg {
                        ArgPair::RegArg { vreg, .. } => {
                            visitor.visit_use(*vreg);
                        }
                        ArgPair::StackArg { .. } => {
                            // !!!不要处理 StackArg 的 vreg!!!
                            // vreg 已经在之前的 Store 指令中被使用
                            // 这里不需要也不应该将它标记为 use
                        }
                        ArgPair::None => {
                            // 无操作
                        }
                    }
                }
                // 处理返回值
                match ret {
                    ArgPair::RegArg { vreg, .. } => {
                        visitor.visit_def(*vreg);
                    }
                    ArgPair::StackArg { .. } | ArgPair::None => {
                        // 函数不应该返回StackArg，None表示无返回值
                    }
                }
            }
            MInst::RetPseudo { ret } => {
                match ret {
                    ArgPair::RegArg { vreg, .. } => {
                        // RetPseudo 使用返回值寄存器（准备将其移动到a0/fa0）
                        visitor.visit_use(*vreg);
                    }
                    ArgPair::StackArg { .. } | ArgPair::None => {
                        // 函数不应该返回StackArg，None表示无返回值
                    }
                }
            }
            MInst::RetCallPseudo { args, .. } => {
                // 使用所有参数的虚拟寄存器
                // !!!重要!!!: 与 CallPseudo 相同，StackArg 的 vreg 不应该被标记为 use!!!
                for arg in args {
                    match arg {
                        ArgPair::RegArg { vreg, .. } => {
                            visitor.visit_use(*vreg);
                        }
                        ArgPair::StackArg { .. } => {
                            // !!!不要处理 StackArg 的 vreg!!!
                            // vreg 已经在之前的 Store 指令中被使用
                            // 这里不需要也不应该将它标记为 use
                        }
                        ArgPair::None => {
                            // 无操作
                        }
                    }
                }
            }
            // 新的序言/尾声伪指令不涉及寄存器
            MInst::PrologueSPAdjust | MInst::PrologueFPSetup | MInst::EpilogueSPRestore => {
                // 这些伪指令不使用也不定义任何虚拟寄存器
            }
            // === 整数加载/存储 ===
            MInst::Li { rd, .. } | MInst::La { rd, .. } => {
                visitor.visit_def(rd.to_reg());
            }
            MInst::Lw { rd, rs, .. } | MInst::Lwu { rd, rs, .. } | MInst::Ld { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Sw { rs1, rs2, .. } | MInst::Sd { rs1, rs2, .. } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
            }
            MInst::Mv { rd, rs } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Sextw { rd, rs } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 算术运算 (I64) ===
            MInst::Add { rd, rs1, rs2 }
            | MInst::Sub { rd, rs1, rs2 }
            | MInst::Mul { rd, rs1, rs2 }
            | MInst::Div { rd, rs1, rs2 }
            | MInst::Rem { rd, rs1, rs2 }
            | MInst::Divu { rd, rs1, rs2 }
            | MInst::Remu { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Addi { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 算术运算 (I32) ===
            MInst::Addw { rd, rs1, rs2 }
            | MInst::Subw { rd, rs1, rs2 }
            | MInst::Mulw { rd, rs1, rs2 }
            | MInst::Divw { rd, rs1, rs2 }
            | MInst::Remw { rd, rs1, rs2 }
            | MInst::Divuw { rd, rs1, rs2 }
            | MInst::Remuw { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Addiw { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 逻辑运算 ===
            MInst::Xor { rd, rs1, rs2 }
            | MInst::Or { rd, rs1, rs2 }
            | MInst::And { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Xori { rd, rs, .. } | MInst::Ori { rd, rs, .. } | MInst::Andi { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 移位运算 (I64) ===
            MInst::Sll { rd, rs1, rs2 }
            | MInst::Srl { rd, rs1, rs2 }
            | MInst::Sra { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Slli { rd, rs, .. }
            | MInst::Srli { rd, rs, .. }
            | MInst::Srai { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 移位运算 (I32) ===
            MInst::Sllw { rd, rs1, rs2 }
            | MInst::Srlw { rd, rs1, rs2 }
            | MInst::Sraw { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Slliw { rd, rs, .. }
            | MInst::Srliw { rd, rs, .. }
            | MInst::Sraiw { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 比较运算 ===
            MInst::Slt { rd, rs1, rs2 } | MInst::Sltu { rd, rs1, rs2 } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Slti { rd, rs, .. } | MInst::Sltiu { rd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::Seqz { rd, rs } | MInst::Snez { rd, rs } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            // === 浮点加载/存储 ===
            MInst::Flw { fd, rs, .. } | MInst::Fld { fd, rs, .. } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            MInst::Fsw { fs, rs, .. } | MInst::Fsd { fs, rs, .. } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_use(rs.to_reg());
            }
            MInst::FmvS { fd, fs } | MInst::FmvD { fd, fs } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            MInst::FmvWX { fd, rs } | MInst::FmvDX { fd, rs } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === 整数/浮点转换 ===
            MInst::FcvtSW { fd, rs }
            | MInst::FcvtSWu { fd, rs }
            | MInst::FcvtSL { fd, rs }
            | MInst::FcvtSLu { fd, rs }
            | MInst::FcvtDW { fd, rs }
            | MInst::FcvtDWu { fd, rs }
            | MInst::FcvtDL { fd, rs }
            | MInst::FcvtDLu { fd, rs } => {
                visitor.visit_use(rs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            MInst::FcvtWS { rd, fs }
            | MInst::FcvtWuS { rd, fs }
            | MInst::FcvtLS { rd, fs }
            | MInst::FcvtLuS { rd, fs }
            | MInst::FcvtWD { rd, fs }
            | MInst::FcvtWuD { rd, fs }
            | MInst::FcvtLD { rd, fs }
            | MInst::FcvtLuD { rd, fs } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::FcvtSD { fd, fs } | MInst::FcvtDS { fd, fs } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === 浮点算术 (F32) ===
            MInst::FaddS { fd, fs1, fs2 }
            | MInst::FsubS { fd, fs1, fs2 }
            | MInst::FmulS { fd, fs1, fs2 }
            | MInst::FdivS { fd, fs1, fs2 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            MInst::FeqS { rd, fs1, fs2 }
            | MInst::FltS { rd, fs1, fs2 }
            | MInst::FleS { rd, fs1, fs2 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::FnegS { fd, fs } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === 浮点算术 (F64) ===
            MInst::FaddD { fd, fs1, fs2 }
            | MInst::FsubD { fd, fs1, fs2 }
            | MInst::FmulD { fd, fs1, fs2 }
            | MInst::FdivD { fd, fs1, fs2 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            MInst::FeqD { rd, fs1, fs2 }
            | MInst::FltD { rd, fs1, fs2 }
            | MInst::FleD { rd, fs1, fs2 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_def(rd.to_reg());
            }
            MInst::FnegD { fd, fs } => {
                visitor.visit_use(fs.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === FMA 指令 (F32) ===
            MInst::FmaddS { fd, fs1, fs2, fs3 }
            | MInst::FmsubS { fd, fs1, fs2, fs3 }
            | MInst::FnmsubS { fd, fs1, fs2, fs3 }
            | MInst::FnmaddS { fd, fs1, fs2, fs3 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_use(fs3.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === FMA 指令 (F64) ===
            MInst::FmaddD { fd, fs1, fs2, fs3 }
            | MInst::FmsubD { fd, fs1, fs2, fs3 }
            | MInst::FnmsubD { fd, fs1, fs2, fs3 }
            | MInst::FnmaddD { fd, fs1, fs2, fs3 } => {
                visitor.visit_use(fs1.to_reg());
                visitor.visit_use(fs2.to_reg());
                visitor.visit_use(fs3.to_reg());
                visitor.visit_def(fd.to_reg());
            }
            // === 控制流 ===
            MInst::Beq { rs1, rs2, .. }
            | MInst::Bne { rs1, rs2, .. }
            | MInst::Blt { rs1, rs2, .. }
            | MInst::Bge { rs1, rs2, .. }
            | MInst::Bltu { rs1, rs2, .. }
            | MInst::Bgeu { rs1, rs2, .. } => {
                visitor.visit_use(rs1.to_reg());
                visitor.visit_use(rs2.to_reg());
            }
            // === 其他指令不访问寄存器 ===
            // TODO: 考虑在Call的def_regs中加入寄存器约束:
            // CALLER_SAVED_XREGS + CALLER_SAVED_FREGS
            MInst::J { .. } | MInst::Call { .. } | MInst::Ret | MInst::Nop | MInst::Fence => {}
        }
    }

    /// 使用 RegMapper 映射指令中的所有寄存器
    /// 注意：这个方法会映射所有寄存器，包括StackArg中的vreg，
    /// 用于别名解析等全面的寄存器替换场景
    pub fn map_regs(&mut self, mapper: &mut impl RegMapper) {
        self.map_regs_internal(mapper, true);
    }

    /// 内部方法：根据参数决定是否映射StackArg的vreg
    fn map_regs_internal(&mut self, mapper: &mut impl RegMapper, map_stack_args: bool) {
        use MInst::*;

        match self {
            // === 伪指令 ===
            Spill { reg, .. } | Reload { reg, .. } => {
                *reg = mapper.map_reg(*reg);
            }
            AddrSlot { dst, .. } => {
                *dst = mapper.map_xreg(*dst);
            }
            Comment { .. } => {}

            // === 算术运算指令 ===
            Add { rd, rs1, rs2 }
            | Sub { rd, rs1, rs2 }
            | And { rd, rs1, rs2 }
            | Or { rd, rs1, rs2 }
            | Xor { rd, rs1, rs2 }
            | Sll { rd, rs1, rs2 }
            | Srl { rd, rs1, rs2 }
            | Sra { rd, rs1, rs2 }
            | Mul { rd, rs1, rs2 }
            | Div { rd, rs1, rs2 }
            | Divu { rd, rs1, rs2 }
            | Rem { rd, rs1, rs2 }
            | Remu { rd, rs1, rs2 }
            | Addw { rd, rs1, rs2 }
            | Subw { rd, rs1, rs2 }
            | Sllw { rd, rs1, rs2 }
            | Srlw { rd, rs1, rs2 }
            | Sraw { rd, rs1, rs2 }
            | Mulw { rd, rs1, rs2 }
            | Divw { rd, rs1, rs2 }
            | Divuw { rd, rs1, rs2 }
            | Remw { rd, rs1, rs2 }
            | Remuw { rd, rs1, rs2 } => {
                *rd = mapper.map_xreg(*rd);
                *rs1 = mapper.map_xreg(*rs1);
                *rs2 = mapper.map_xreg(*rs2);
            }

            // === 立即数算术指令 ===
            Addi { rd, rs, .. }
            | Addiw { rd, rs, .. }
            | Andi { rd, rs, .. }
            | Ori { rd, rs, .. }
            | Xori { rd, rs, .. }
            | Slli { rd, rs, .. }
            | Srli { rd, rs, .. }
            | Srai { rd, rs, .. }
            | Slliw { rd, rs, .. }
            | Srliw { rd, rs, .. }
            | Sraiw { rd, rs, .. } => {
                *rd = mapper.map_xreg(*rd);
                *rs = mapper.map_xreg(*rs);
            }

            // === 比较指令 ===
            Slt { rd, rs1, rs2 } | Sltu { rd, rs1, rs2 } => {
                *rd = mapper.map_xreg(*rd);
                *rs1 = mapper.map_xreg(*rs1);
                *rs2 = mapper.map_xreg(*rs2);
            }
            Slti { rd, rs, .. } | Sltiu { rd, rs, .. } | Seqz { rd, rs } | Snez { rd, rs } => {
                *rd = mapper.map_xreg(*rd);
                *rs = mapper.map_xreg(*rs);
            }

            // === 移动指令 ===
            Mv { rd, rs } | Sextw { rd, rs } => {
                *rd = mapper.map_xreg(*rd);
                *rs = mapper.map_xreg(*rs);
            }

            // === 内存访问指令 ===
            Lw { rd, rs, .. } | Ld { rd, rs, .. } | Lwu { rd, rs, .. } => {
                *rd = mapper.map_xreg(*rd);
                *rs = mapper.map_xreg(*rs);
            }
            Sw { rs2, rs1, .. } | Sd { rs2, rs1, .. } => {
                *rs1 = mapper.map_xreg(*rs1);
                *rs2 = mapper.map_xreg(*rs2);
            }

            // === 立即数加载指令 ===
            Li { rd, .. } | La { rd, .. } => {
                *rd = mapper.map_xreg(*rd);
            }

            // === 分支指令 ===
            Beq { rs1, rs2, .. }
            | Bne { rs1, rs2, .. }
            | Blt { rs1, rs2, .. }
            | Bge { rs1, rs2, .. }
            | Bltu { rs1, rs2, .. }
            | Bgeu { rs1, rs2, .. } => {
                *rs1 = mapper.map_xreg(*rs1);
                *rs2 = mapper.map_xreg(*rs2);
            }

            // === 浮点算术指令 ===
            FaddS { fd, fs1, fs2 }
            | FsubS { fd, fs1, fs2 }
            | FmulS { fd, fs1, fs2 }
            | FdivS { fd, fs1, fs2 }
            | FaddD { fd, fs1, fs2 }
            | FsubD { fd, fs1, fs2 }
            | FmulD { fd, fs1, fs2 }
            | FdivD { fd, fs1, fs2 } => {
                *fd = mapper.map_freg(*fd);
                *fs1 = mapper.map_freg(*fs1);
                *fs2 = mapper.map_freg(*fs2);
            }

            // === 浮点融合乘加指令 ===
            FmaddS { fd, fs1, fs2, fs3 }
            | FmsubS { fd, fs1, fs2, fs3 }
            | FnmsubS { fd, fs1, fs2, fs3 }
            | FnmaddS { fd, fs1, fs2, fs3 }
            | FmaddD { fd, fs1, fs2, fs3 }
            | FmsubD { fd, fs1, fs2, fs3 }
            | FnmsubD { fd, fs1, fs2, fs3 }
            | FnmaddD { fd, fs1, fs2, fs3 } => {
                *fd = mapper.map_freg(*fd);
                *fs1 = mapper.map_freg(*fs1);
                *fs2 = mapper.map_freg(*fs2);
                *fs3 = mapper.map_freg(*fs3);
            }

            // === 浮点移动指令 ===
            FmvS { fd, fs } | FmvD { fd, fs } | FnegS { fd, fs } | FnegD { fd, fs } => {
                *fd = mapper.map_freg(*fd);
                *fs = mapper.map_freg(*fs);
            }
            FmvWX { fd, rs } | FmvDX { fd, rs } => {
                *fd = mapper.map_freg(*fd);
                *rs = mapper.map_xreg(*rs);
            }

            // === 浮点内存访问 ===
            Flw { fd, rs, .. } | Fld { fd, rs, .. } => {
                *fd = mapper.map_freg(*fd);
                *rs = mapper.map_xreg(*rs);
            }
            Fsw { fs, rs, .. } | Fsd { fs, rs, .. } => {
                *fs = mapper.map_freg(*fs);
                *rs = mapper.map_xreg(*rs);
            }

            // === 浮点转换指令 ===
            FcvtWS { rd, fs, .. }
            | FcvtWuS { rd, fs, .. }
            | FcvtLS { rd, fs, .. }
            | FcvtLuS { rd, fs, .. }
            | FcvtWD { rd, fs, .. }
            | FcvtWuD { rd, fs, .. }
            | FcvtLD { rd, fs, .. }
            | FcvtLuD { rd, fs, .. } => {
                *rd = mapper.map_xreg(*rd);
                *fs = mapper.map_freg(*fs);
            }
            FcvtSW { fd, rs, .. }
            | FcvtSWu { fd, rs, .. }
            | FcvtSL { fd, rs, .. }
            | FcvtSLu { fd, rs, .. }
            | FcvtDW { fd, rs, .. }
            | FcvtDWu { fd, rs, .. }
            | FcvtDL { fd, rs, .. }
            | FcvtDLu { fd, rs, .. } => {
                *fd = mapper.map_freg(*fd);
                *rs = mapper.map_xreg(*rs);
            }
            FcvtSD { fd, fs, .. } | FcvtDS { fd, fs, .. } => {
                *fd = mapper.map_freg(*fd);
                *fs = mapper.map_freg(*fs);
            }

            // === 浮点比较指令 ===
            FeqS { rd, fs1, fs2 }
            | FltS { rd, fs1, fs2 }
            | FleS { rd, fs1, fs2 }
            | FeqD { rd, fs1, fs2 }
            | FltD { rd, fs1, fs2 }
            | FleD { rd, fs1, fs2 } => {
                *rd = mapper.map_xreg(*rd);
                *fs1 = mapper.map_freg(*fs1);
                *fs2 = mapper.map_freg(*fs2);
            }

            // === 伪指令 ===
            CallPseudo { ret, args, .. } => {
                for arg in args {
                    match arg {
                        ArgPair::RegArg { vreg, .. } => {
                            *vreg = mapper.map_reg(*vreg);
                        }
                        ArgPair::StackArg { vreg, .. } => {
                            // 根据参数决定是否映射StackArg的vreg
                            // - 别名解析时(map_stack_args=true)：需要映射
                            // - 其他场景时(map_stack_args=false)：跳过
                            if map_stack_args {
                                *vreg = mapper.map_reg(*vreg);
                            }
                        }
                        ArgPair::None => {
                            // 无操作
                        }
                    }
                }
                // 处理返回值
                match ret {
                    ArgPair::RegArg { vreg, .. } => {
                        *vreg = mapper.map_reg(*vreg);
                    }
                    ArgPair::StackArg { .. } | ArgPair::None => {
                        // 函数不应该返回StackArg，None表示无返回值
                    }
                }
            }
            RetPseudo { ret } => {
                match ret {
                    ArgPair::RegArg { vreg, .. } => {
                        *vreg = mapper.map_reg(*vreg);
                    }
                    ArgPair::StackArg { .. } | ArgPair::None => {
                        // 函数不应该返回StackArg，None表示无返回值
                    }
                }
            }
            RetCallPseudo { args, .. } => {
                for arg in args {
                    match arg {
                        ArgPair::RegArg { vreg, .. } => {
                            *vreg = mapper.map_reg(*vreg);
                        }
                        ArgPair::StackArg { vreg, .. } => {
                            // 根据参数决定是否映射StackArg的vreg
                            if map_stack_args {
                                *vreg = mapper.map_reg(*vreg);
                            }
                        }
                        ArgPair::None => {
                            // 无操作
                        }
                    }
                }
            }

            // 新的序言/尾声伪指令不涉及寄存器映射
            PrologueSPAdjust | PrologueFPSetup | EpilogueSPRestore => {
                // 这些伪指令不使用也不定义任何虚拟寄存器
            }

            // === 无寄存器操作的指令 ===
            J { .. } | Call { .. } | Ret | Nop | Fence => {}
        }
    }

    /// 获取指令定义的寄存器
    pub fn get_defs(&self) -> Vec<Reg> {
        let mut collector = RegCollector::new();
        self.visit_regs(&mut collector);
        collector.defs
    }

    /// 获取指令使用的寄存器
    pub fn get_uses(&self) -> Vec<Reg> {
        let mut collector = RegCollector::new();
        self.visit_regs(&mut collector);
        collector.uses
    }

    /// 如果这条指令是一个简单的寄存器间移动，返回目标和源寄存器
    /// 用于IRC合并阶段
    pub fn as_move(&self) -> Option<(Reg, Reg)> {
        match self {
            // 整数寄存器间的移动
            MInst::Mv { rd, rs } => Some((rd.to_reg(), rs.to_reg())),
            // 单精度浮点寄存器间的移动
            MInst::FmvS { fd, fs } => Some((fd.to_reg(), fs.to_reg())),
            // 双精度浮点寄存器间的移动
            MInst::FmvD { fd, fs } => Some((fd.to_reg(), fs.to_reg())),
            // 其他指令不是可合并的移动
            _ => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_visitor_pattern() {
        // 测试访问器模式
        let inst = MInst::Add {
            rd: XReg::virt(1),
            rs1: XReg::virt(2),
            rs2: XReg::virt(3),
        };

        let mut collector = RegCollector::new();
        inst.visit_regs(&mut collector);

        assert_eq!(collector.defs.len(), 1);
        assert_eq!(collector.uses.len(), 2);
    }

    #[test]
    fn test_expand_pseudo_simple() {
        let spill = MInst::Spill {
            reg: Reg::X(RvXReg::A0),
            slot: SlotId::new(5),
            kind: SlotKindDisc::Spill,
        };

        // 简单情况：小偏移量
        let result = spill.expand_pseudo(|slot, _insts| {
            let offset = (slot.raw_u32() * 8) as i16; // 假设每个槽8字节
            (XReg::phys(RvXReg::Sp), offset)
        });

        assert!(result.is_some());
        let insts = result.unwrap();
        assert_eq!(insts.len(), 1);

        match &insts[0] {
            MInst::Sd { rs1, offset, .. } => {
                assert_eq!(*rs1, XReg::phys(RvXReg::Sp));
                assert_eq!(*offset, 40); // slot 5 * 8
            }
            _ => panic!("Expected Sd instruction"),
        }
    }

    #[test]
    fn test_expand_pseudo_large_offset() {
        let reload = MInst::Reload {
            slot: SlotId::new(1000),
            reg: Reg::X(RvXReg::A0),
            kind: SlotKindDisc::Spill,
        };

        // 复杂情况：大偏移量
        let result = reload.expand_pseudo(|slot, insts| {
            let offset = (slot.raw_u32() * 8) as i64;
            if (-2048..=2047).contains(&offset) {
                (XReg::phys(RvXReg::Sp), offset as i16)
            } else {
                // 需要额外指令
                let temp = XReg::phys(RvXReg::T0);
                insts.push(MInst::Li {
                    rd: temp,
                    imm: offset,
                });
                insts.push(MInst::Add {
                    rd: temp,
                    rs1: XReg::phys(RvXReg::Sp),
                    rs2: temp,
                });
                (temp, 0)
            }
        });

        assert!(result.is_some());
        let insts = result.unwrap();
        assert_eq!(insts.len(), 3); // Li + Add + Ld

        // 检查前两条是准备指令
        match &insts[0] {
            MInst::Li { rd, imm } => {
                assert_eq!(*rd, XReg::phys(RvXReg::T0));
                assert_eq!(*imm, 8000);
            }
            _ => panic!("Expected Li instruction"),
        }

        // 最后一条是实际的 load
        match &insts[2] {
            MInst::Ld { rs, offset, .. } => {
                assert_eq!(*rs, XReg::phys(RvXReg::T0));
                assert_eq!(*offset, 0);
            }
            _ => panic!("Expected Ld instruction"),
        }
    }
}
