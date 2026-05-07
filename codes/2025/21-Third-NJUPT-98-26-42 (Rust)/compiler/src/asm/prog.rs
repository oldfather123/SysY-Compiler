//! 程序 & 模块结构
//!
//! - RvProg: 标准RISCV汇编程序, 能够直接emit
//! - VirtProg: 虚拟汇编程序, 需要寄存器分配/伪指令展开等操作
//!
use super::{inst::*, reg::*, stack::*};
use crate::{
    asm::cfg::VirtControlFlowGraph,
    prelude::*,
    ssa::{
        ctx::*,
        globals::{GlobalData, GlobalInit},
        ir::*,
    },
};
use itertools::Itertools;
use std::{
    collections::HashMap,
    fmt::{self, Debug, Display},
};

// ===================================================================
// 程序段
// ===================================================================

#[derive(Debug, Clone)]
pub struct DataSection {
    pub label: Label,
    pub data: Vec<u8>,
}

#[derive(Debug, Clone)]
pub struct BssSection {
    pub label: Label,
    pub size: usize,
}

pub type FuncName = String;

// ===================================================================
// RISCV汇编程序
// ===================================================================

#[derive(Debug, Clone)]
pub struct RvBlock {
    pub label: Label,
    pub instructions: Vec<RvInst>,
}

#[derive(Debug, Clone)]
pub struct RvFunc {
    pub name: FuncName,
    pub exported: bool,
    pub blocks: Vec<RvBlock>,
}

#[derive(Debug, Clone, Default)]
pub struct RvProg {
    pub data: Vec<DataSection>,
    pub rodata: Vec<DataSection>,
    pub bss: Vec<BssSection>,
    pub text: Vec<RvFunc>,
}

// ===================================================================
// VirtASM 块级别数据
// ===================================================================

/// 块ID -- 直接用作Hash的键
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum VirtBlockId {
    /// 原始SSA基本块
    BasicBlock { block: Block },
    /// 关键边分割产生的虚拟块
    /// pred -> succ 之间的关键边被分割为 pred -> CriticalEdge -> succ
    CriticalEdge {
        succ_idx: u32, // 注意这个succ_idx是表示"第几条边"
        pred: Block,
        succ: Block,
    },
}

impl VirtBlockId {
    pub fn as_basic(&self) -> Option<Block> {
        match self {
            VirtBlockId::BasicBlock { block } => Some(*block),
            _ => None,
        }
    }

    pub fn is_basic(&self) -> bool {
        matches!(self, VirtBlockId::BasicBlock { .. })
    }

    pub fn is_edge(&self) -> bool {
        matches!(self, VirtBlockId::CriticalEdge { .. })
    }

    pub fn egde_pred(&self) -> Option<Block> {
        match self {
            VirtBlockId::CriticalEdge { pred, .. } => Some(*pred),
            _ => None,
        }
    }

    pub fn edge_succ(&self) -> Option<Block> {
        match self {
            VirtBlockId::CriticalEdge { succ, .. } => Some(*succ),
            _ => None,
        }
    }

    /// 转换为标签，需要传入 FuncRef 以生成唯一标签
    pub fn to_label(&self, func_ref: FuncRef) -> Label {
        // 使用 FuncRef 的 index 作为唯一标识
        let func_idx = func_ref.index();

        match self {
            VirtBlockId::BasicBlock { block } => {
                // 使用 block 的 index 作为标识
                format!(".LBB_f{}_{}", func_idx, block.index())
            }
            VirtBlockId::CriticalEdge {
                succ_idx,
                pred,
                succ,
            } => {
                // 使用各个 Block 的 index 作为标识
                format!(
                    ".LCE_f{}_{}_{}_{}",
                    func_idx,
                    succ_idx,
                    pred.index(),
                    succ.index()
                )
            }
        }
    }
}

// 为调试目的实现 Display
impl Display for VirtBlockId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            VirtBlockId::BasicBlock { block } => write!(f, "BB_{}", block.index()),
            VirtBlockId::CriticalEdge {
                succ_idx,
                pred,
                succ,
            } => {
                write!(f, "CE_{}_{}_{}", succ_idx, pred.index(), succ.index())
            }
        }
    }
}

/// 块参数传递信息
#[derive(Debug, Clone)]
pub struct EdgeParams {
    /// 参数传递列表
    pub transfers: Vec<ParamTransfer>,
    /// Move 插入位置
    pub move_loc: VirtBlockId,
}

/// 参数传递类型
#[derive(Debug, Clone)]
pub enum ParamTransfer {
    /// 寄存器间移动: (src, dst)
    Move(Reg, Reg),
    /// 常量加载: (const, dst)
    LoadConst(crate::ssa::ir::ConstValue, Reg),
}

/// 块数据
#[derive(Debug, Clone)]
pub struct VirtBlockData {
    /// 块参数 -- 关于入口块参数 & 栈上传入参数问题:
    /// 入口块参数等同于函数参数，因此入口块的块参数<= CALL_XARGS_LEN 个应当被视作立即活跃
    /// 而超过8个的部分则被视作Spilled的寄存器 (在caller的传入参数的栈上)
    pub params: Vec<Reg>,
    pub insts: Vec<VirtInst>,
}

impl Default for VirtBlockData {
    fn default() -> Self {
        Self::new()
    }
}

impl VirtBlockData {
    pub fn new() -> Self {
        Self {
            params: vec![],
            insts: vec![],
        }
    }
    pub fn push_inst(&mut self, inst: VirtInst) {
        self.insts.push(inst);
    }
    pub fn insert_inst(&mut self, pos: usize, inst: VirtInst) {
        self.insts.insert(pos, inst);
    }
    pub fn remove_inst(&mut self, pos: usize) -> Option<VirtInst> {
        if pos < self.insts.len() {
            Some(self.insts.remove(pos))
        } else {
            None
        }
    }
    pub fn replace_inst(&mut self, pos: usize, inst: VirtInst) -> Option<VirtInst> {
        if pos < self.insts.len() {
            Some(std::mem::replace(&mut self.insts[pos], inst))
        } else {
            None
        }
    }
    pub fn extend_insts(&mut self, insts: impl IntoIterator<Item = VirtInst>) {
        self.insts.extend(insts);
    }

    /// 反转指令顺序
    pub fn reverse_insts(&mut self) {
        self.insts.reverse();
    }

    /// 找到第一个终结指令的位置
    /// 如果没有终结指令，返回块的末尾位置
    pub fn find_terminator_pos(&self) -> usize {
        for (i, inst) in self.insts.iter().enumerate() {
            if inst.is_terminator() {
                return i;
            }
        }
        self.insts.len()
    }

    /// 在前面插入
    pub fn extend_pred_insts(&mut self, insts: impl IntoIterator<Item = VirtInst>) {
        self.insts.splice(0..0, insts);
    }
}

// ===================================================================
// VirtASM 函数级别数据
// ===================================================================

/// 寄存器分配时传递的方法
#[derive(Debug, Default)]
pub struct VirtFunc {
    pub func_ref: FuncRef,
    pub func_name: FuncName,
    pub exported: bool,
    pub block_datas: HashMap<VirtBlockId, VirtBlockData>, // 顺序应该由CFG来给出
    pub cfg: VirtControlFlowGraph,
    pub stack: StackLayout,
    /// 持久化从寄存器到栈槽的映射
    /// 必须是持久化的，不能包括outgoing_args这样会因Call而改变的内存
    /// 但包括 incoming_args 和 spill_slots
    pub pst_slot_map: HashMap<Reg, SlotId>,
    /// 函数参数的ABI约束
    /// 避免依赖于诡异的"入口块参数"
    pub args: ArgPairVec,
}
impl VirtFunc {
    pub fn new() -> Self {
        Self::default()
    }
    // 实现更多方法
}

// ===================================================================
// VirtASM 全局级数据
// ===================================================================

#[derive(Debug, Clone)]
pub struct VirtProg {
    pub data: Vec<DataSection>,
    pub rodata: Vec<DataSection>,
    pub bss: Vec<BssSection>,
    pub global_map: HashMap<Global, Label>, // 映射到标签
    // 数据段去重支持
    rodata_map: HashMap<Vec<u8>, Label>, // 从数据内容到标签的映射
    next_rodata_id: u32,                 // 用于生成唯一标签
}

impl Default for VirtProg {
    fn default() -> Self {
        Self::new()
    }
}

impl VirtProg {
    pub fn new() -> Self {
        Self {
            data: Vec::new(),
            rodata: Vec::new(),
            bss: Vec::new(),
            global_map: HashMap::new(),
            rodata_map: HashMap::new(),
            next_rodata_id: 0,
        }
    }

    /// 初始化全局数据段
    pub fn init_globals(&mut self, core_ctx: &CoreContext) {
        for (global_ref, global_data) in core_ctx.globals.iter() {
            // 生成标签名
            let label = global_data.name.clone();
            self.global_map.insert(global_ref, label.clone());

            match &global_data.init {
                GlobalInit::Zero => {
                    // 零初始化的变量放入 .bss 段
                    let size = self.compute_global_size(global_data, core_ctx);
                    self.bss.push(BssSection { label, size });
                }
                GlobalInit::Scalar(val) => {
                    // 标量常量放入 .data 或 .rodata 段
                    let data = val.to_abi_bytes();
                    if global_data.is_const {
                        self.rodata.push(DataSection { label, data });
                    } else {
                        self.data.push(DataSection { label, data });
                    }
                }
                GlobalInit::Array(values) => {
                    // 数组初始化放入 .data 或 .rodata 段
                    let mut data = Vec::new();
                    for val in values {
                        data.extend(val.to_abi_bytes());
                    }
                    if global_data.is_const {
                        self.rodata.push(DataSection { label, data });
                    } else {
                        self.data.push(DataSection { label, data });
                    }
                }
            }
        }
    }

    /// 计算全局变量字节
    fn compute_global_size(&self, global_data: &GlobalData, core_ctx: &CoreContext) -> usize {
        let elem_size = global_data.ty.abi_size();

        // 计算数组总元素数
        let dims = core_ctx.get_type_dims(global_data.dims);
        let total_elements: usize = dims.iter().filter_map(|d| d.map(|v| v as usize)).product();

        elem_size * total_elements.max(1)
    }

    /// 将初始化数据放入 rodata 段，并实现去重
    /// 如果相同的数据已经存在，返回已有的标签；否则创建新的数据段
    pub fn ensure_rodata(&mut self, data: Vec<u8>) -> Label {
        // 检查是否已存在相同数据
        if let Some(label) = self.rodata_map.get(&data) {
            return label.clone();
        }

        // 创建新的标签
        let label = format!(".Lconst_{}", self.next_rodata_id);
        self.next_rodata_id += 1;

        // 添加到 rodata 段
        self.rodata.push(DataSection {
            label: label.clone(),
            data: data.clone(),
        });

        // 记录映射关系
        self.rodata_map.insert(data, label.clone());

        label
    }
}

// ===================================================================
// Display traits
// ===================================================================

impl fmt::Display for RvProg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> std::fmt::Result {
        if !self.data.is_empty() {
            writeln!(f, ".data")?;
        }
        for data_sym in &self.data {
            write!(f, "{}", data_sym)?;
        }

        if !self.rodata.is_empty() {
            writeln!(f, ".data")?;
        }
        for rodata_sym in &self.rodata {
            write!(f, "{}", rodata_sym)?;
        }

        if !self.bss.is_empty() {
            writeln!(f, ".bss")?;
        }
        for bss_sym in &self.bss {
            write!(f, "{}", bss_sym)?;
        }

        if !self.text.is_empty() {
            writeln!(f, ".text")?;
        }
        for func in &self.text {
            write!(f, "{}", func)?;
        }

        Ok(())
    }
}

impl fmt::Display for DataSection {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, ".globl {}", self.label)?;
        writeln!(f, ".p2align 3")?;
        writeln!(f, "{}:", self.label)?;
        for chunk in &self.data.iter().chunks(8) {
            writeln!(
                f,
                "        .byte {}",
                chunk.map(|v| format!("{v:#04x}")).join(", ")
            )?;
        }
        Ok(())
    }
}

impl fmt::Display for BssSection {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, ".comm {}, {}, 8", self.label, self.size)
    }
}

impl fmt::Display for RvFunc {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> std::fmt::Result {
        if self.exported {
            writeln!(f, ".globl {}", self.name)?;
        }
        writeln!(f, ".p2align 2")?;
        writeln!(f, ".type {}, @function", self.name)?;
        writeln!(f, "{}:", self.name)?;
        for block in &self.blocks {
            write!(f, "{}", block)?;
        }
        writeln!(f, "END_{}:", self.name)?;
        writeln!(f, ".size {0}, END_{0}-{0}", self.name)?;
        Ok(())
    }
}

impl fmt::Display for RvBlock {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "{}:", self.label)?;
        for inst in &self.instructions {
            writeln!(f, "        {}", inst)?;
        }
        Ok(())
    }
}

#[cfg(test)]
mod test {
    use super::*;

    use indoc::indoc;

    #[test]
    fn test_asm_writer() {
        assert_eq!(
            RvProg {
                data: vec![],
                rodata: vec![DataSection {
                    label: "var1".to_owned(),
                    data: "Hello, world!".to_owned().into_bytes()
                }],
                bss: vec![BssSection {
                    label: "var2".to_owned(),
                    size: 1024,
                }],
                text: vec![RvFunc {
                    name: "main".to_owned(),
                    exported: true,
                    blocks: vec![RvBlock {
                        label: "L0".to_owned(),
                        instructions: vec![
                            RvInst::Li {
                                rd: XReg::phys(RvXReg::A0),
                                imm: 0
                            },
                            RvInst::Ret
                        ]
                    }]
                }]
            }
            .to_string(),
            indoc! {"
                .rodata
                .globl var1
                .p2align 3
                var1:
                        .byte 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77
                        .byte 0x6f, 0x72, 0x6c, 0x64, 0x21
                .bss
                .comm var2, 1024, 8
                .text
                .globl main
                .p2align 2
                .type main, @function:
                main:
                L0:
                        li a0, 0
                        ret
                END_main:
                .size main, .END_main-main
            "}
        );
    }
}
