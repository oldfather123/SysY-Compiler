//! 寄存器表示
//!
//! 设计方案:
//! 1. 使用内部统一的Reg枚举, 并使用XReg/FReg作类型化包装, 能够在保持强类型的同时, 无需为相同的逻辑编写两套代码
//!     例如在装入容器的时候, 就能够统一存储
//! 2. 提供了 trait RegType 能够在转换为Reg后, 还能还原出原类型
//! 3. 提供AsRef/AsMut/From/TryFrom等trait实现
#![allow(non_upper_case_globals)]

use std::{
    collections::HashMap,
    fmt::{self, Debug},
};
use strum::{
    self, AsRefStr, EnumCount, EnumIs, EnumIter, EnumString, FromRepr, IntoStaticStr, VariantArray,
};

use crate::define_index;

// ===================================================================
// Reg types
// ===================================================================

// 定义虚拟寄存器的类型化索引
define_index!(VXId);
define_index!(VFId);

/// 共用寄存器枚举
/// 注意: 对于ISLE应该接近不透明类型 -- 尽量少用
///
/// 只表示寄存器位置, 溢出操作通过 MInst::Spill/Reload 伪指令处理
/// 这样分离了「值在哪」和「如何移动值」的关注点
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord, EnumIs, strum::Display)]
pub enum Reg {
    /// 物理整数寄存器
    #[strum(serialize = "{0}")]
    X(RvXReg),
    /// 虚拟整数寄存器
    #[strum(serialize = "%VX{0}")]
    VX(VXId),

    /// 物理浮点寄存器
    #[strum(serialize = "{0}")]
    F(RvFReg),
    /// 虚拟浮点寄存器
    #[strum(serialize = "%VF{0}")]
    VF(VFId),
}

impl Reg {
    /// 判断是否为物理寄存器
    pub fn is_physical(&self) -> bool {
        matches!(self, Reg::X(_) | Reg::F(_))
    }

    /// 判断是否为虚拟寄存器
    pub fn is_virtual(&self) -> bool {
        matches!(self, Reg::VX(_) | Reg::VF(_))
    }

    pub fn is_xreg(&self) -> bool {
        matches!(self, Reg::X(_) | Reg::VX(_))
    }

    pub fn is_freg(&self) -> bool {
        matches!(self, Reg::F(_) | Reg::VF(_))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct XReg(Reg);

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct FReg(Reg);

impl XReg {
    pub fn inner(&self) -> &Reg {
        &self.0
    }

    pub fn to_abi(&self) -> Option<&'static str> {
        match self.0 {
            Reg::X(rv) => Some(rv.into()),
            _ => None,
        }
    }

    pub fn phys(rv: RvXReg) -> Self {
        XReg(Reg::X(rv))
    }

    pub fn virt(id: u32) -> Self {
        XReg(Reg::VX(VXId::new(id as usize)))
    }

    pub fn virt_id(id: VXId) -> Self {
        XReg(Reg::VX(id))
    }

    /// 获取虚拟寄存器ID
    pub fn get_virt_id(&self) -> Option<VXId> {
        match self.0 {
            Reg::VX(id) => Some(id),
            _ => None,
        }
    }

    /// 判断是否为虚拟寄存器
    pub fn is_virtual(&self) -> bool {
        matches!(self.0, Reg::VX(_))
    }

    /// 判断是否为物理寄存器
    pub fn is_physical(&self) -> bool {
        matches!(self.0, Reg::X(_))
    }
}

impl FReg {
    pub fn inner(&self) -> &Reg {
        &self.0
    }

    pub fn to_abi(&self) -> Option<&'static str> {
        match self.0 {
            Reg::F(rv) => Some(rv.into()),
            _ => None,
        }
    }

    pub fn phys(rv: RvFReg) -> Self {
        FReg(Reg::F(rv))
    }

    pub fn virt(id: u32) -> Self {
        FReg(Reg::VF(VFId::new(id as usize)))
    }

    pub fn virt_id(id: VFId) -> Self {
        FReg(Reg::VF(id))
    }

    pub fn get_virt_id(&self) -> Option<VFId> {
        match self.0 {
            Reg::VF(id) => Some(id),
            _ => None,
        }
    }

    pub fn is_virtual(&self) -> bool {
        matches!(self.0, Reg::VF(_))
    }

    pub fn is_physical(&self) -> bool {
        matches!(self.0, Reg::F(_))
    }
}

// ===================================================================
// RegType trait
// ===================================================================

/// 用于在接受Reg参数, 并且XReg->Reg->XReg恢复出原类型的trait
pub trait RegType: Sized + Copy {
    /// 尝试恢复原类型
    fn try_from_reg(reg: Reg) -> Option<Self>;

    fn to_reg(self) -> Reg;
}

impl RegType for XReg {
    fn try_from_reg(reg: Reg) -> Option<Self> {
        match reg {
            Reg::X(_) | Reg::VX(_) => Some(XReg(reg)),
            _ => None,
        }
    }

    fn to_reg(self) -> Reg {
        self.0
    }
}

impl RegType for FReg {
    fn try_from_reg(reg: Reg) -> Option<Self> {
        match reg {
            Reg::F(_) | Reg::VF(_) => Some(FReg(reg)),
            _ => None,
        }
    }

    fn to_reg(self) -> Reg {
        self.0
    }
}

impl RegType for Reg {
    fn try_from_reg(reg: Reg) -> Option<Self> {
        Some(reg)
    }

    fn to_reg(self) -> Reg {
        self
    }
}

// ===================================================================
// vreg allocator
// ===================================================================

/// 虚拟寄存器分配器
///
/// 别名系统:
/// 由于指令选择分为两个独立阶段:
/// - 第一阶段：在 Lower::new 中预分配虚拟寄存器给每个 SSA value
/// - 第二阶段：ISLE 规则生成指令时可能使用不同的寄存器
/// 因此需要建立映射: 预分配寄存器 → ISLE 生成的寄存器
/// 通过别名机制，后续使用这个 SSA value 的指令会自动使用正确的寄存器
pub struct VRegAllocator {
    pub next_vxreg: VXId,
    pub next_vfreg: VFId,
    pub vreg_alias: HashMap<Reg, Reg>, // 默认为所有寄存器创建别名, 哪怕是一模一样的虚拟寄存器
}

impl Default for VRegAllocator {
    fn default() -> Self {
        Self::new()
    }
}

impl VRegAllocator {
    pub fn new() -> Self {
        VRegAllocator {
            next_vxreg: VXId::new(1), // 从1开始, 0视作错误
            next_vfreg: VFId::new(1),
            vreg_alias: HashMap::new(),
        }
    }

    pub fn alloc_x(&mut self) -> XReg {
        let id = self.next_vxreg;
        self.next_vxreg = id.next();
        XReg::virt_id(id)
    }

    pub fn alloc_f(&mut self) -> FReg {
        let id = self.next_vfreg;
        self.next_vfreg = id.next();
        FReg::virt_id(id)
    }

    /// 设置别名
    pub fn set_vreg_alias(&mut self, from: impl AsRef<Reg>, to: impl AsRef<Reg>) {
        self.vreg_alias.insert(*from.as_ref(), *to.as_ref());
    }

    /// 解析别名
    /// 使用RegType能够获得原类型 XReg->XReg, FReg->FReg
    pub fn resolve_vreg_alias<T: RegType>(&self, vreg: T) -> Option<T> {
        let mut current = vreg.to_reg();
        while let Some(&aliased) = self.vreg_alias.get(&current) {
            if aliased == current {
                break;
            }
            current = aliased;
        }
        T::try_from_reg(current)
    }
}

// ===================================================================
// physical reg
// ===================================================================

// 物理寄存器常量
impl XReg {
    pub const Zero: XReg = XReg(Reg::X(RvXReg::Zero));
    pub const Ra: XReg = XReg(Reg::X(RvXReg::Ra));
    pub const Sp: XReg = XReg(Reg::X(RvXReg::Sp));
    pub const Gp: XReg = XReg(Reg::X(RvXReg::Gp));
    pub const Tp: XReg = XReg(Reg::X(RvXReg::Tp));
    pub const T0: XReg = XReg(Reg::X(RvXReg::T0));
    pub const T1: XReg = XReg(Reg::X(RvXReg::T1));
    pub const T2: XReg = XReg(Reg::X(RvXReg::T2));
    pub const S0: XReg = XReg(Reg::X(RvXReg::S0));
    pub const S1: XReg = XReg(Reg::X(RvXReg::S1));
    pub const A0: XReg = XReg(Reg::X(RvXReg::A0));
    pub const A1: XReg = XReg(Reg::X(RvXReg::A1));
    pub const A2: XReg = XReg(Reg::X(RvXReg::A2));
    pub const A3: XReg = XReg(Reg::X(RvXReg::A3));
    pub const A4: XReg = XReg(Reg::X(RvXReg::A4));
    pub const A5: XReg = XReg(Reg::X(RvXReg::A5));
    pub const A6: XReg = XReg(Reg::X(RvXReg::A6));
    pub const A7: XReg = XReg(Reg::X(RvXReg::A7));
    pub const S2: XReg = XReg(Reg::X(RvXReg::S2));
    pub const S3: XReg = XReg(Reg::X(RvXReg::S3));
    pub const S4: XReg = XReg(Reg::X(RvXReg::S4));
    pub const S5: XReg = XReg(Reg::X(RvXReg::S5));
    pub const S6: XReg = XReg(Reg::X(RvXReg::S6));
    pub const S7: XReg = XReg(Reg::X(RvXReg::S7));
    pub const S8: XReg = XReg(Reg::X(RvXReg::S8));
    pub const S9: XReg = XReg(Reg::X(RvXReg::S9));
    pub const S10: XReg = XReg(Reg::X(RvXReg::S10));
    pub const S11: XReg = XReg(Reg::X(RvXReg::S11));
    pub const T3: XReg = XReg(Reg::X(RvXReg::T3));
    pub const T4: XReg = XReg(Reg::X(RvXReg::T4));
    pub const T5: XReg = XReg(Reg::X(RvXReg::T5));
    pub const T6: XReg = XReg(Reg::X(RvXReg::T6));
}

impl FReg {
    pub const Ft0: FReg = FReg(Reg::F(RvFReg::Ft0));
    pub const Ft1: FReg = FReg(Reg::F(RvFReg::Ft1));
    pub const Ft2: FReg = FReg(Reg::F(RvFReg::Ft2));
    pub const Ft3: FReg = FReg(Reg::F(RvFReg::Ft3));
    pub const Ft4: FReg = FReg(Reg::F(RvFReg::Ft4));
    pub const Ft5: FReg = FReg(Reg::F(RvFReg::Ft5));
    pub const Ft6: FReg = FReg(Reg::F(RvFReg::Ft6));
    pub const Ft7: FReg = FReg(Reg::F(RvFReg::Ft7));
    pub const Fs0: FReg = FReg(Reg::F(RvFReg::Fs0));
    pub const Fs1: FReg = FReg(Reg::F(RvFReg::Fs1));
    pub const Fa0: FReg = FReg(Reg::F(RvFReg::Fa0));
    pub const Fa1: FReg = FReg(Reg::F(RvFReg::Fa1));
    pub const Fa2: FReg = FReg(Reg::F(RvFReg::Fa2));
    pub const Fa3: FReg = FReg(Reg::F(RvFReg::Fa3));
    pub const Fa4: FReg = FReg(Reg::F(RvFReg::Fa4));
    pub const Fa5: FReg = FReg(Reg::F(RvFReg::Fa5));
    pub const Fa6: FReg = FReg(Reg::F(RvFReg::Fa6));
    pub const Fa7: FReg = FReg(Reg::F(RvFReg::Fa7));
    pub const Fs2: FReg = FReg(Reg::F(RvFReg::Fs2));
    pub const Fs3: FReg = FReg(Reg::F(RvFReg::Fs3));
    pub const Fs4: FReg = FReg(Reg::F(RvFReg::Fs4));
    pub const Fs5: FReg = FReg(Reg::F(RvFReg::Fs5));
    pub const Fs6: FReg = FReg(Reg::F(RvFReg::Fs6));
    pub const Fs7: FReg = FReg(Reg::F(RvFReg::Fs7));
    pub const Fs8: FReg = FReg(Reg::F(RvFReg::Fs8));
    pub const Fs9: FReg = FReg(Reg::F(RvFReg::Fs9));
    pub const Fs10: FReg = FReg(Reg::F(RvFReg::Fs10));
    pub const Fs11: FReg = FReg(Reg::F(RvFReg::Fs11));
    pub const Ft8: FReg = FReg(Reg::F(RvFReg::Ft8));
    pub const Ft9: FReg = FReg(Reg::F(RvFReg::Ft9));
    pub const Ft10: FReg = FReg(Reg::F(RvFReg::Ft10));
    pub const Ft11: FReg = FReg(Reg::F(RvFReg::Ft11));
}

#[repr(i8)]
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    Hash,
    PartialOrd,
    Ord,
    AsRefStr,       // xr.as_ref()
    IntoStaticStr,  // 'static str = xr.into()
    strum::Display, // fmt::Display
    EnumCount,      // RvReg::COUNT
    EnumIter,       // RvReg::iter(),
    EnumString,     // RvReg::from_str("t0")
    EnumIs,         // RvReg::T0.is_t0()
    FromRepr,       // RvReg::from_repr(5)
    VariantArray,   // RvReg::VARIANTS = &[RvReg::T0, RvReg::T1, ...]
)]
#[strum(serialize_all = "lowercase")]
pub enum RvXReg {
    Zero = 0,
    Ra = 1,
    Sp = 2,
    Gp = 3,
    Tp = 4,
    T0 = 5,
    T1 = 6,
    T2 = 7,
    S0 = 8,
    S1 = 9,
    A0 = 10,
    A1 = 11,
    A2 = 12,
    A3 = 13,
    A4 = 14,
    A5 = 15,
    A6 = 16,
    A7 = 17,
    S2 = 18,
    S3 = 19,
    S4 = 20,
    S5 = 21,
    S6 = 22,
    S7 = 23,
    S8 = 24,
    S9 = 25,
    S10 = 26,
    S11 = 27,
    T3 = 28,
    T4 = 29,
    T5 = 30,
    T6 = 31,
}

#[repr(i8)]
#[derive(
    Debug,
    Clone,
    Copy,
    PartialEq,
    Eq,
    Hash,
    PartialOrd,
    Ord,
    AsRefStr,
    IntoStaticStr,
    strum::Display,
    EnumCount,
    EnumIter,
    EnumString,
    EnumIs,
    FromRepr,
    VariantArray,
)]
#[strum(serialize_all = "lowercase")]
pub enum RvFReg {
    Ft0 = 0,
    Ft1 = 1,
    Ft2 = 2,
    Ft3 = 3,
    Ft4 = 4,
    Ft5 = 5,
    Ft6 = 6,
    Ft7 = 7,
    Fs0 = 8,
    Fs1 = 9,
    Fa0 = 10,
    Fa1 = 11,
    Fa2 = 12,
    Fa3 = 13,
    Fa4 = 14,
    Fa5 = 15,
    Fa6 = 16,
    Fa7 = 17,
    Fs2 = 18,
    Fs3 = 19,
    Fs4 = 20,
    Fs5 = 21,
    Fs6 = 22,
    Fs7 = 23,
    Fs8 = 24,
    Fs9 = 25,
    Fs10 = 26,
    Fs11 = 27,
    Ft8 = 28,
    Ft9 = 29,
    Ft10 = 30,
    Ft11 = 31,
}

// ===================================================================
// ABI 约束寄存器
// ===================================================================

/// 预留的临时寄存器
/// 选择 T6 (x31) 作为Spill时的寄存器
pub const SPILL_TEMP_REG: XReg = XReg::T6;

/// 最大压力 -- 调试使用设置为6
pub const XREGS_MAX_PRESSURE: usize = PRECOLORED_XREGS.len(); // 6;
pub const FREGS_MAX_PRESSURE: usize = PRECOLORED_FREGS.len(); // 6;
pub const CALLEE_SAVED_XREGS_LEN: usize = CALLEE_SAVED_XREGS.len() - 2; // S1-S11 (SP, S0不可用, 因此-2)
pub const CALLEE_SAVED_FREGS_LEN: usize = CALLEE_SAVED_FREGS.len(); // Fs0-Fs11 (全部可用)
pub const CALL_XARGS_LEN: usize = CALL_XARGS.len();
pub const CALL_FARGS_LEN: usize = CALL_FARGS.len();

/// 整数参数 -- 调试使用为3
pub const CALL_RET_XREG: XReg = XReg::A0;
pub const CALL_XARGS: [XReg; 8] = [
    XReg::A0,
    XReg::A1,
    XReg::A2,
    XReg::A3,
    XReg::A4,
    XReg::A5,
    XReg::A6,
    XReg::A7,
];

/// 浮点参数
pub const CALL_RET_FREG: FReg = FReg::Fa0;
pub const CALL_FARGS: [FReg; 8] = [
    FReg::Fa0,
    FReg::Fa1,
    FReg::Fa2,
    FReg::Fa3,
    FReg::Fa4,
    FReg::Fa5,
    FReg::Fa6,
    FReg::Fa7,
];

pub const PRECOLORED_XREGS: [XReg; 25] = [
    // XReg::Zero, -- Zero不能用来分配
    // XReg::Ra, -- 返回地址
    // XReg::Sp, -- 栈顶
    // XReg::Gp, -- 全局指针
    // XReg::Tp, -- 线程指针
    XReg::T0,
    XReg::T1,
    XReg::T2,
    // XReg::S0, -- FP 栈底
    XReg::S1,
    XReg::A0,
    XReg::A1,
    XReg::A2,
    XReg::A3,
    XReg::A4,
    XReg::A5,
    XReg::A6,
    XReg::A7,
    XReg::S2,
    XReg::S3,
    XReg::S4,
    XReg::S5,
    XReg::S6,
    XReg::S7,
    XReg::S8,
    XReg::S9,
    XReg::S10,
    XReg::S11,
    XReg::T3,
    XReg::T4,
    XReg::T5,
    // XReg::T6, -- 不分配用作临时计算
];

pub const PRECOLORED_FREGS: [FReg; 32] = [
    FReg::Ft0,
    FReg::Ft1,
    FReg::Ft2,
    FReg::Ft3,
    FReg::Ft4,
    FReg::Ft5,
    FReg::Ft6,
    FReg::Ft7,
    FReg::Fs0,
    FReg::Fs1,
    FReg::Fa0,
    FReg::Fa1,
    FReg::Fa2,
    FReg::Fa3,
    FReg::Fa4,
    FReg::Fa5,
    FReg::Fa6,
    FReg::Fa7,
    FReg::Fs2,
    FReg::Fs3,
    FReg::Fs4,
    FReg::Fs5,
    FReg::Fs6,
    FReg::Fs7,
    FReg::Fs8,
    FReg::Fs9,
    FReg::Fs10,
    FReg::Fs11,
    FReg::Ft8,
    FReg::Ft9,
    FReg::Ft10,
    FReg::Ft11,
];

pub const CALLEE_SAVED_XREGS: [XReg; 13] = [
    XReg::Sp, // 栈顶
    XReg::S0, // 栈底
    XReg::S1,
    XReg::S2,
    XReg::S3,
    XReg::S4,
    XReg::S5,
    XReg::S6,
    XReg::S7,
    XReg::S8,
    XReg::S9,
    XReg::S10,
    XReg::S11,
];

pub const CALLEE_SAVED_FREGS: [FReg; 12] = [
    FReg::Fs0,
    FReg::Fs1,
    FReg::Fs2,
    FReg::Fs3,
    FReg::Fs4,
    FReg::Fs5,
    FReg::Fs6,
    FReg::Fs7,
    FReg::Fs8,
    FReg::Fs9,
    FReg::Fs10,
    FReg::Fs11,
];

pub const CALLER_SAVED_XREGS: [XReg; 16] = [
    XReg::Ra, // 返回地址
    XReg::T0,
    XReg::T1,
    XReg::T2,
    XReg::T3,
    XReg::T4,
    XReg::T5,
    XReg::T6,
    XReg::A0,
    XReg::A1,
    XReg::A2,
    XReg::A3,
    XReg::A4,
    XReg::A5,
    XReg::A6,
    XReg::A7,
];

pub const CALLER_SAVED_FREGS: [FReg; 20] = [
    FReg::Ft0,
    FReg::Ft1,
    FReg::Ft2,
    FReg::Ft3,
    FReg::Ft4,
    FReg::Ft5,
    FReg::Ft6,
    FReg::Ft7,
    FReg::Ft8,
    FReg::Ft9,
    FReg::Ft10,
    FReg::Ft11,
    FReg::Fa0,
    FReg::Fa1,
    FReg::Fa2,
    FReg::Fa3,
    FReg::Fa4,
    FReg::Fa5,
    FReg::Fa6,
    FReg::Fa7,
];

// ===================================================================
// Display traits
// ===================================================================

impl fmt::Display for XReg {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.inner())
    }
}
impl fmt::Display for FReg {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.inner())
    }
}

// ===================================================================
// Core trait implementations for XReg/FReg/Reg interoperability
// ===================================================================

// AsRef implementations - standard borrowing pattern
impl AsRef<Reg> for Reg {
    fn as_ref(&self) -> &Reg {
        self
    }
}

impl AsRef<Reg> for XReg {
    fn as_ref(&self) -> &Reg {
        &self.0
    }
}

impl AsRef<Reg> for FReg {
    fn as_ref(&self) -> &Reg {
        &self.0
    }
}

// AsMut implementations - mutable borrowing pattern
impl AsMut<Reg> for Reg {
    fn as_mut(&mut self) -> &mut Reg {
        self
    }
}

impl AsMut<Reg> for XReg {
    fn as_mut(&mut self) -> &mut Reg {
        &mut self.0
    }
}

impl AsMut<Reg> for FReg {
    fn as_mut(&mut self) -> &mut Reg {
        &mut self.0
    }
}

// From implementations - safe value conversions to Reg
impl From<XReg> for Reg {
    fn from(xreg: XReg) -> Self {
        xreg.0
    }
}

impl From<FReg> for Reg {
    fn from(freg: FReg) -> Self {
        freg.0
    }
}

// TryFrom implementations - fallible conversions from Reg
impl TryFrom<Reg> for XReg {
    type Error = &'static str;

    fn try_from(reg: Reg) -> Result<Self, Self::Error> {
        match reg {
            Reg::X(_) | Reg::VX(_) => Ok(XReg(reg)),
            _ => Err("Not an integer register"),
        }
    }
}

impl TryFrom<Reg> for FReg {
    type Error = &'static str;

    fn try_from(reg: Reg) -> Result<Self, Self::Error> {
        match reg {
            Reg::F(_) | Reg::VF(_) => Ok(FReg(reg)),
            _ => Err("Not a floating-point register"),
        }
    }
}
