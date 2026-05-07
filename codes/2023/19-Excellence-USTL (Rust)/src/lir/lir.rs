use crate::asm::RegisterKind;
use crate::ast::table::AnySymbolRef;
use crate::util::{ElementRef, HashInsertVec};

// 我们将额外寄存器扩展到10 15 这样的数量上而不遵照8 16 32 这样的标准，
// 即如果某种寄存器是7个那么经过额外数量，我们需要额外扩展一个，
// 虽然暂时并没有什么用，但是多留几个接口总是好的

/// HighLevel Register ,
/// ***NOT*** RISC-V Register , just named by RISC-V standard
/// 高级寄存器
#[allow(nonstandard_style)]
#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub enum HRegister {
    /// zero
    zero,
    /// return address register
    ra,
    /// stack pointer register
    sp,
    /// frame pointer register , may not supported by ISA
    fp,
    /// 不允许使用
    #[allow(unused)]
    gp,
    /// 不允许使用
    #[allow(unused)]
    tp,
    /// param normal purpose usable register
    /// because ARM and RISC-V all contain more than 32 register ,
    /// we defined four normal usable register
    /// for RISC-V , t0 - t7 , a0 - a7
    /// if there is any ISA miss some one , we could use load and store to ***create_new_one***
    /// but which can never be the first or the second and so on register.
    p0,
    p1,
    p2,
    p3,
    p4,
    p5,
    p6,
    p7,
    // 额外的扩展，一般用不到
    p8,
    p9,

    // general purpose registers
    t0,
    t1,
    t2,
    t3,
    t4,
    t5,
    t6,
    t7,
    // 同上
    t8,
    t9,
    // 保存寄存器
    s0,
    s1,
    s2,
    s3,
    s4,
    s5,
    s6,
    s7,
    s8,
    s9,
    s10,
    // 额外寄存器
    s11,
    s12,
    s13,
    s14,
    // 浮点寄存器
    ft0,
    ft1,
    ft2,
    ft3,
    ft4,
    ft5,
    ft6,
    ft7,
    ft8,
    ft9,
    ft10,
    ft11,
    ft12,
    ft13,
    ft14,
    // 浮点保存寄存器
    fs0,
    fs1,
    fs2,
    fs3,
    fs4,
    fs5,
    fs6,
    fs7,
    fs8,
    fs9,
    fs10,
    fs11,
    // 额外寄存器
    fs12,
    fs13,
    fs14,

    // 浮点参数寄存器
    fp0,
    fp1,
    fp2,
    fp3,
    fp4,
    fp5,
    fp6,
    fp7,
    // 额外寄存器
    fp8,
    fp9,
}

impl HRegister {
    /// 下面这个函数可以优化成上面的那样
    /// sp , ra 等寄存器 ，不支持reg_kind操作
    pub fn get_reg_kind(&self) -> RegisterKind {
        type REG = HRegister;
        match self {
            REG::p0
            | REG::p1
            | REG::p2
            | REG::p3
            | REG::p4
            | REG::p5
            | REG::p6
            | REG::p7
            | REG::p8
            | REG::p9
            | REG::t0
            | REG::t1
            | REG::t2
            | REG::t3
            | REG::t4
            | REG::t5
            | REG::t6
            | REG::t7
            | REG::t8
            | REG::t9 => RegisterKind::Normal,
            REG::s0
            | REG::s1
            | REG::s2
            | REG::s3
            | REG::s4
            | REG::s5
            | REG::s6
            | REG::s7
            | REG::s8
            | REG::s9
            | REG::s10
            | REG::s11
            | REG::s12
            | REG::s13
            | REG::s14 => RegisterKind::AnyNormal,
            REG::ft0
            | REG::ft1
            | REG::ft2
            | REG::ft3
            | REG::ft4
            | REG::ft5
            | REG::ft6
            | REG::ft7
            | REG::ft8
            | REG::ft9
            | REG::ft10
            | REG::ft11
            | REG::ft12
            | REG::ft13
            | REG::ft14 => RegisterKind::FNormal,
            REG::fs0
            | REG::fs1
            | REG::fs2
            | REG::fs3
            | REG::fs4
            | REG::fs5
            | REG::fs6
            | REG::fs7
            | REG::fs8
            | REG::fs9
            | REG::fs10
            | REG::fs11
            | REG::fs12
            | REG::fs13
            | REG::fs14 => RegisterKind::AnyFNormal,
            REG::fp0 | REG::fp1 | REG::fp2 | REG::fp3 | REG::fp4 | REG::fp5 | REG::fp6 | REG::fp7 | REG::fp8 | REG::fp9 => RegisterKind::FNormal,
            REG::fp | REG::ra | REG::sp | REG::zero => RegisterKind::Normal,
            HRegister::gp | HRegister::tp => {
                panic!("NEVER HERE")
            }
        }
    }

    pub fn is_in(&self, supply_reg_type: RegisterKind) -> bool {
        let this_reg_kind = self.get_reg_kind();
        return match supply_reg_type {
            RegisterKind::Normal => {
                if this_reg_kind == RegisterKind::Normal {
                    true
                } else {
                    false
                }
            }
            RegisterKind::FNormal => {
                if this_reg_kind == RegisterKind::FNormal {
                    true
                } else {
                    false
                }
            }
            RegisterKind::AnyNormal => {
                if this_reg_kind == RegisterKind::Normal || this_reg_kind == RegisterKind::AnyNormal {
                    true
                } else {
                    false
                }
            }
            RegisterKind::AnyFNormal => {
                if this_reg_kind == RegisterKind::FNormal || this_reg_kind == RegisterKind::AnyFNormal {
                    true
                } else {
                    false
                }
            }
        };
    }

    pub fn is_same_anykind(&self, b_reg: &HRegister) -> bool {
        let a_kind = self.get_reg_kind();
        let b_kind = b_reg.get_reg_kind();
        return match a_kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => match b_kind {
                RegisterKind::Normal => true,
                RegisterKind::AnyNormal => true,
                RegisterKind::FNormal => false,
                RegisterKind::AnyFNormal => false,
            },
            RegisterKind::FNormal | RegisterKind::AnyFNormal => match b_kind {
                RegisterKind::Normal => false,
                RegisterKind::AnyNormal => false,
                RegisterKind::FNormal => true,
                RegisterKind::AnyFNormal => true,
            },
        };
    }
}

pub trait I12 {
    fn is_in_i12(&self) -> bool;
}

impl I12 for i16 {
    fn is_in_i12(&self) -> bool {
        !(*self < -2048 || *self > 2047)
    }
}

impl I12 for i32 {
    fn is_in_i12(&self) -> bool {
        !(*self < -2048 || *self > 2047)
    }
}

impl I12 for u32 {
    fn is_in_i12(&self) -> bool {
        *self <= 2047
    }
}

/// Immediate Number , NO SIGN NOT UNSIGNED
/// 注意就算是有符号数也采用无符号数储存，编译关键点 <- 错误实现[7月28日]
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub enum ImmediateBits {
    // assumption a large number of platform immediate with 12 bits.
    // 更加准确的新的实现
    /// 12位立即数
    ImmI12(i16),
    /// 立即数 32位 有符号
    ImmWordI32(i32),
    /// 立即数 32位 无符号,负数和浮点数，都使用这个接口
    ImmWordU32(u32),
    /// 立即数 64位 有符号
    ImmDoubleWordI64(i64),
}

#[derive(Clone, Copy, Hash, Eq, PartialEq, Debug)]
pub enum WidthType {
    #[allow(unused)]
    Byte,
    Word,
    #[allow(unused)]
    HalfWord,
    DoubleWord,
}

/// 反向寻址 ： 逆栈生长方向
/// 正向寻址 ： 正帧生长方向
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct RegWithOff {
    /// i32 is max length of jump distance;
    /// # 禁止直接构造
    pub base_addr_reg: HRegister,
    /// # 禁止直接构造
    pub offset: i32,
}

impl RegWithOff {
    /// 反向寻址建议使用，一般用于向上一个函数的栈空间寻址
    /// # 栈顶部在低地址，基于栈顶部寻址
    /// 以 reg 寄存器作为基地址, offset来完成寻址
    pub fn suggest_fp_top_based(reg: HRegister, offset: i32) -> Self {
        Self { base_addr_reg: reg, offset }
    }
    /// 正向寻址建议使用
    /// # 栈顶部在低地址，基于栈底部寻址
    /// 以 reg 寄存器作为基地址, 反转offset来完成寻址
    pub fn suggest_fp_bottom_based(reg: HRegister, offset: i32) -> Self {
        Self {
            base_addr_reg: reg,
            offset: -offset,
        }
    }
    /// # 反向寻址建议使用
    /// # 栈顶部在低地址，基于栈顶部寻址
    /// 以 reg 寄存器作为基地址, offset来完成寻址
    pub fn suggest_sp_top_based(reg: HRegister, offset: i32) -> Self {
        Self { base_addr_reg: reg, offset }
    }

    /// # t0寻址建议使用
    /// # 栈顶部在低地址，基于栈顶部寻址
    /// 以 reg 寄存器作为基地址, offset来完成寻址
    pub fn suggest_t0_based(reg: HRegister, offset: i32) -> Self {
        Self { base_addr_reg: reg, offset }
    }
}

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub enum RegOrImm {
    Imm(ImmediateBits),
    Reg(HRegister),
}

pub type RWOOI = RegWithOffOrImm;
pub type ROI = RegOrImm;
pub type IMM = ImmediateBits;

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub enum RegWithOffOrImm {
    Imm(ImmediateBits),
    RegWO(RegWithOff),
}

/// LOAD 指令需要查看寄存器类型，然后再选择加载指令
/// 如果寄存器类型是浮点，而提供的的宽度类型是 u8 或者 u16 等类似的不匹配类型，则需要报错
/// 但是对于浮点的立即数加载，RISCV 和 AArch64 有区别
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct LOADINS(pub WidthType, pub HRegister, pub RegWithOffOrImm);

/// 无符号加载指令只能用于整数加载，浮点数用LOAD
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct LOADUINS(pub WidthType, pub HRegister, pub RegWithOffOrImm);

/// 根据具体的寄存器类型，选择浮点或者通用储存指令
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct STOREINS(pub WidthType, pub HRegister, pub RegWithOff);

/// 根据具体的寄存器类型，选择浮点或者通用储存指令
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct LADDRINS(pub HRegister, pub LabelTag);

/// 除了加法和减法支持立即数外，其他数学运算指令都不支持立即数
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct ADDINS(pub WidthType, pub HRegister, pub HRegister, pub RegOrImm);

/// 除了加法和减法支持立即数外，其他数学运算指令都不支持立即数
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct SUBINS(pub WidthType, pub HRegister, pub HRegister, pub RegOrImm);

// 不打算加入立即数支持，如果ISA不支持的话，这里加入立即数只会造成性能大幅下降
/**
对与mul,div,rem 等指令的探讨
如果操作寄存器全部是同一种类的，那么翻译十分方便，
但是如果不是同一类型，那么就需要数据搬运，和额外的寄存器 ，而且这里隐含了类型转换
这种情况要么就保留两个专用搬运寄存器，要么就寄存器重申请(这里代价是十分大的)
所以对于寄存器不同类型这样的任务，交给上层而不是LIR去处理更好
所以最好就不要让他支持不同类型的计算
 */

// 这些寄存器的LIR设计全部存在问题，都默认支持了64位，而不是32位
// 而在Sysy中，类型大小统一为Word，
// 问题说大也大，说小也小，
// 保守方案 重新设计 LIR 改元组为结构体,加入宽度类型
// 通常方案 加入宽度类型
// 凑合事方案 backend翻译的时候翻译成32位
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct MULINS(pub HRegister, pub HRegister, pub HRegister);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct DIVINS(pub HRegister, pub HRegister, pub HRegister);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct REMINS(pub HRegister, pub HRegister, pub HRegister);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct ANDINS(pub HRegister, pub HRegister, pub RegOrImm);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct ORINS(pub HRegister, pub HRegister, pub RegOrImm);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct XORINS(pub HRegister, pub HRegister, pub RegOrImm);

/// BRANCH EQUAL
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct BEQINS(pub HRegister, pub HRegister, pub LabelTag);

/// BRANCH NOT EQUAL
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct BNEINS(pub HRegister, pub HRegister, pub LabelTag);

/// BRANCH LESS THAN
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct BLTINS(pub HRegister, pub HRegister, pub LabelTag);

/// BRANCH GREATER EQUAL
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct BGEINS(pub HRegister, pub HRegister, pub LabelTag);

/// SHIFT LEFT LOGICAL
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct SLLINS(pub HRegister, pub HRegister, pub RegOrImm);

/// SET LESS THAN , 需要支持不同类型 ，target 寄存器必须是通用寄存器 , positive 寄存器类型是一致的 , 如果架构不支持直接加载浮点数则需要检查
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct SLTINS(pub HRegister, pub HRegister, pub RegOrImm);

/// FLOAT COMPARE SET LESS EQUAL ,target 寄存器必须是通用寄存器 , positive 必须是浮点寄存器
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct FSLEINS(pub HRegister, pub HRegister, pub HRegister);

/// SET IF EQUAL ,target 寄存器必须是通用寄存器 , positive 必须是浮点寄存器
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct FSEQINS(pub HRegister, pub HRegister, pub HRegister);

/// NEG
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct NEGINS(pub HRegister, pub HRegister);

/// 复制寄存器的每一位到另一个寄存器,支持不同类型寄存器之间的搬运
/// fixme 不支持宽度类型
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct MOVINS(pub HRegister, pub HRegister);

/// 进行浮点和整数之间的各种转换
/// fixme 不支持宽度,不支持精度，不支持有符号还是无符号,不支持浮点到浮点，整数到整数。
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct CVTINS(pub HRegister, pub HRegister);

/// 原生指令，能不用就不用，为的只是优化
/// 但是优化是很远很远的事情
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct RAWINS(pub String);

#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub struct JALPINS(pub HRegister, pub RegWithOff);

pub type LabelTag = AnySymbolRef;

/// FIXME LIR 指令，类似RISCV指令和伪指令的区别，尽量包含伪造指令好还是不包含伪指令好？
#[derive(Clone, Hash, Eq, PartialEq, Debug)]
pub enum LirStatement {
    Load(LOADINS),
    #[allow(unused)]
    Loadu(LOADUINS),
    Laddr(LADDRINS),
    Store(STOREINS),
    Mov(MOVINS),
    /// convert
    Cvt(CVTINS),
    Add(ADDINS),
    Sub(SUBINS),
    Mul(MULINS),
    Div(DIVINS),
    Rem(REMINS),
    // AND B,C -> A
    And(ANDINS),
    // OR B,C -> A
    #[allow(unused)]
    Or(ORINS),
    // XOR B,C -> A
    #[allow(unused)]
    Xor(XORINS),
    // shift left logical
    Sll(SLLINS),
    // 一般用在Debug
    Raw(RAWINS),
    // Session最后Finish时把SessionTag替换成LabelTag
    Label(LabelTag),
    /// branch not equal
    Bne(BNEINS),
    /// branch equal
    Beq(BEQINS),
    /// branch greater than or equal
    Bge(BGEINS),
    /// branch less than
    Blt(BLTINS),
    /// set less than
    Slt(SLTINS),
    /// set if float equal
    Fseq(FSEQINS),
    /// set if float less equal
    Fsle(FSLEINS),
    /// Negative
    Neg(NEGINS),
    Call(LabelTag),
    /// Jump And Link PC to register
    #[allow(unused)]
    Jalp(JALPINS),
    Jump(LabelTag),
    #[allow(unused)]
    Tail(LabelTag),
    Return,
}

// 可以给基本块添加影响寄存器来减少 LOAD 和 STORE 的占用
// 或者说更进一步的定义协议本身 ， 加速传递
/// 基本块，可理解为函数，但是不是函数，是函数的抽象 ！ 但该抽象不仅仅为函数
// #[derive(Clone)]
pub struct LirBlock {
    pub name: String,
    /// 执行体和传入变量，传出变量是强关联的
    pub statements: HashInsertVec<(LirStatement, Option<Annotation>)>,
}

impl LirBlock {
    pub fn new(name: String) -> Self {
        LirBlock {
            name,
            statements: HashInsertVec::new(),
        }
    }

    pub fn add_stmt(&mut self, stat: LirStatement) -> ElementRef<(LirStatement, Option<Annotation>)> {
        self.statements.push((stat, None))
    }

    pub fn add_stmt_with_ann(&mut self, stmt: LirStatement, ann: Option<Annotation>){
      self.statements.push((stmt, ann));
    }
}


/// 全局量，包括变量和数组

pub struct LirGlobal {
    pub name: String,
    pub is_ro: bool,
    pub statements: Vec<LirStatement>,
}

pub struct LirPartCode {
    pub fun_blocks: Vec<LirBlock>,
    pub globals: Vec<LirGlobal>,
}

pub type Annotation = String;

pub trait AnnAux {
    fn make_warped_ann(str: String) -> Option<Annotation>;
}

impl AnnAux for Annotation {
    /// 请不要自带#前缀
    fn make_warped_ann(str: String) -> Option<Annotation> {
        Some(str)
    }
}
