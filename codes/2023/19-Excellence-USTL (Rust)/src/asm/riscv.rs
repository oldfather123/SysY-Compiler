use crate::asm::{ISABackend, RegisterKind};
use crate::ast::table::SymbolTable;
use crate::lir::lir::{ImmediateBits, LirStatement, RegOrImm, RegWithOffOrImm, WidthType, ADDINS, ANDINS, CVTINS, DIVINS, LOADINS, LOADUINS, MOVINS, MULINS, NEGINS, REMINS, SLLINS, SLTINS, STOREINS, SUBINS, I12};

pub struct RISCVBackend;

// 注意，必须严格控制转译过程的空格的数量

const BEF_MARGIN: &str = "\t";

impl ISABackend for RISCVBackend {
    fn translate_stmt(&self, lir: LirStatement, name_table: &SymbolTable) -> String {
        match lir {
            LirStatement::Load(load) => self.tran_load(load),
            LirStatement::Loadu(loadu) => self.tran_loadu(loadu),
            LirStatement::Laddr(laddr) => format!("{}la {},{}", BEF_MARGIN, self.get_reg_name(laddr.0), name_table.get_symbol(&laddr.1)),
            LirStatement::Store(store) => self.tran_store(store),
            LirStatement::Mov(move_) => self.tran_move(move_),
            LirStatement::Cvt(cvt) => self.tran_cvt(cvt),
            LirStatement::Add(add) => self.tran_add(add),
            LirStatement::Sub(sub) => self.tran_sub(sub),
            LirStatement::Mul(mul) => self.tran_mul(mul),
            LirStatement::Div(div) => self.tran_div(div),
            LirStatement::Rem(rem) => self.tran_rem(rem),
            LirStatement::Sll(sll) => self.tran_sll(sll),
            LirStatement::Raw(raw) => raw.0.clone(),
            LirStatement::Call(call) => {
                format!("{}call {}", BEF_MARGIN, name_table.get_symbol(&call))
            }
            LirStatement::Return => format!("{}ret", BEF_MARGIN),
            LirStatement::Bne(bne) => format!(
                "{}bne {},{},{}",
                BEF_MARGIN,
                self.get_reg_name(bne.0),
                self.get_reg_name(bne.1),
                name_table.get_symbol(&bne.2)
            ),
            LirStatement::Beq(beq) => format!(
                "{}beq {},{},{}",
                BEF_MARGIN,
                self.get_reg_name(beq.0),
                self.get_reg_name(beq.1),
                name_table.get_symbol(&beq.2)
            ),
            LirStatement::Bge(bge) => format!(
                "{}bge {},{},{}",
                BEF_MARGIN,
                self.get_reg_name(bge.0),
                self.get_reg_name(bge.1),
                name_table.get_symbol(&bge.2)
            ),
            LirStatement::Blt(blt) => format!(
                "{}blt {},{},{}",
                BEF_MARGIN,
                self.get_reg_name(blt.0),
                self.get_reg_name(blt.1),
                name_table.get_symbol(&blt.2)
            ),
            LirStatement::Jalp(_) => {
                todo!("not implemented")
            }
            LirStatement::Jump(tag) => format!("{}j {} ", BEF_MARGIN, name_table.get_symbol(&tag)),
            LirStatement::Label(tag) => format!("{}:", name_table.get_symbol(&tag)),
            LirStatement::Tail(tag) => format!("{}", name_table.get_symbol(&tag)),
            LirStatement::Slt(slt) => self.tran_slt(slt),
            LirStatement::Fseq(fseq) => {
                if !fseq.0.is_in(RegisterKind::AnyNormal) {
                    panic!("target register must be normal register")
                }
                if !fseq.1.is_in(RegisterKind::AnyFNormal) {
                    panic!("positive1 register must be float register")
                }
                if !fseq.2.is_in(RegisterKind::AnyFNormal) {
                    panic!("positive2 register must be float register")
                }
                format!(
                    "{}feq.s {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(fseq.0),
                    self.get_reg_name(fseq.1),
                    self.get_reg_name(fseq.2)
                )
            }
            LirStatement::Fsle(fsle) => {
                if !fsle.0.is_in(RegisterKind::AnyNormal) {
                    panic!("target register must be normal register")
                }
                if !fsle.1.is_in(RegisterKind::AnyFNormal) {
                    panic!("positive1 register must be float register")
                }
                if !fsle.2.is_in(RegisterKind::AnyFNormal) {
                    panic!("positive2 register must be float register")
                }
                format!(
                    "{}fle.s {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(fsle.0),
                    self.get_reg_name(fsle.1),
                    self.get_reg_name(fsle.2)
                )
            }
            LirStatement::Neg(neg) => self.tran_neg(neg),
            LirStatement::And(and) => self.tran_and(and),
            LirStatement::Or(_) => {
                panic!("Not Implemented")
            }
            LirStatement::Xor(_) => {
                panic!("Not Implemented")
            }
        }
    }
    // the riscv isa have been already implemented by trait defaults .
}

impl RISCVBackend {
    fn tran_and(&self, and: ANDINS) -> String {
        match and.2 {
            RegOrImm::Imm(imm) => {
                format!(
                    "{}andi {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(and.0),
                    self.get_reg_name(and.1),
                    imm.translate_to_i64_uncheck()
                )
            }
            RegOrImm::Reg(reg) => {
                format!(
                    "{}and {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(and.0),
                    self.get_reg_name(and.1),
                    self.get_reg_name(reg)
                )
            }
        }
    }

    fn tran_sub(&self, sub: SUBINS) -> String {
        match &sub.3 {
            RegOrImm::Imm(imm) => {
                format!("    addi {},{},-{}", self.get_reg_name(sub.1), self.get_reg_name(sub.2), imm.translate_to_i64_uncheck())
            }
            RegOrImm::Reg(reg) => {
                if !sub.1.is_same_anykind(&reg) {
                    panic!("ERROR NOT SAME KIND REG  WERE ADDED {:?}", reg)
                }
                let kind = reg.get_reg_kind();
                match kind {
                    RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                        if sub.0 == WidthType::DoubleWord {
                            panic!("NOT SUPPORT DOUBLE SUB")
                        }
                        format!(
                            "    fsub.s {},{},{}",
                            self.get_reg_name(sub.1),
                            self.get_reg_name(sub.2),
                            self.get_reg_name(reg.clone())
                        )
                    }
                    RegisterKind::Normal | RegisterKind::AnyNormal => {
                        match sub.0 {
                            WidthType::Word => {
                                format!(
                                    "    subw {},{},{}",
                                    self.get_reg_name(sub.1),
                                    self.get_reg_name(sub.2),
                                    self.get_reg_name(reg.clone())
                                )
                            }
                            WidthType::DoubleWord => {
                                format!(
                                    "    sub {},{},{}",
                                    self.get_reg_name(sub.1),
                                    self.get_reg_name(sub.2),
                                    self.get_reg_name(reg.clone())
                                )
                            }
                            _ => {
                                panic!("NOT SUPPORT TYPE SUB");
                            }
                        }
                    }
                }
            }
        }
    }

    fn tran_move(&self, mov: MOVINS) -> String {
        let to_type = mov.0.get_reg_kind();
        let from_type = mov.1.get_reg_kind();
        return match to_type {
            RegisterKind::Normal | RegisterKind::AnyNormal => match from_type {
                RegisterKind::Normal | RegisterKind::AnyNormal => {
                    format!("{}mv\t{},{}", BEF_MARGIN, self.get_reg_name(mov.0), self.get_reg_name(mov.1))
                }
                RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                    format!("{}fmv.x.w\t{},{}", BEF_MARGIN, self.get_reg_name(mov.0), self.get_reg_name(mov.1))
                }
            },
            RegisterKind::FNormal | RegisterKind::AnyFNormal => match from_type {
                RegisterKind::Normal | RegisterKind::AnyNormal => {
                    format!("{}fmv.w.x\t{},{}", BEF_MARGIN, self.get_reg_name(mov.0), self.get_reg_name(mov.1))
                }
                RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                    format!("{}fmv.s\t{},{}", BEF_MARGIN, self.get_reg_name(mov.0), self.get_reg_name(mov.1))
                }
            },
        };
    }

    fn tran_cvt(&self, cvt: CVTINS) -> String {
        if cvt.0.is_in(RegisterKind::AnyNormal) {
            if cvt.1.is_in(RegisterKind::AnyNormal) {
                panic!("ERROR CVT FROM NORMAL TO NORMAL")
            } else {
                // float -> int
                return format!("{}fcvt.w.s {},{},rtz", BEF_MARGIN, self.get_reg_name(cvt.0), self.get_reg_name(cvt.1));
            }
        } else {
            // 只能是 F寄存器
            if cvt.1.is_in(RegisterKind::AnyFNormal) {
                panic!("CVT FROM FLOAT TO FLOAT")
            } else {
                // int -> float
                return format!("{}fcvt.s.w {},{}", BEF_MARGIN, self.get_reg_name(cvt.0), self.get_reg_name(cvt.1));
            }
        }
    }

    fn tran_mul(&self, mul: MULINS) -> String {
        if !mul.0.is_same_anykind(&mul.1) {
            panic!("ERROR NOT SAME KIND REG  WERE ADDED {:?}", mul)
        }
        let kind = mul.0.get_reg_kind();
        return match kind {
            RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                format!(
                    "{}fmul.s {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(mul.0),
                    self.get_reg_name(mul.1),
                    self.get_reg_name(mul.2)
                )
            }
            RegisterKind::Normal | RegisterKind::AnyNormal => {
                format!(
                    "{}mulw {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(mul.0),
                    self.get_reg_name(mul.1),
                    self.get_reg_name(mul.2)
                )
            }
        };
    }

    fn tran_div(&self, div: DIVINS) -> String {
        if !div.0.is_same_anykind(&div.1) {
            panic!("ERROR NOT SAME KIND REGS WERE DIVED")
        }
        let kind = div.0.get_reg_kind();
        return match kind {
            RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                format!(
                    "{}fdiv.s {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(div.0),
                    self.get_reg_name(div.1),
                    self.get_reg_name(div.2)
                )
            }
            RegisterKind::Normal | RegisterKind::AnyNormal => {
                format!(
                    "{}divw {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(div.0),
                    self.get_reg_name(div.1),
                    self.get_reg_name(div.2)
                )
            }
        };
    }

    fn tran_rem(&self, rem: REMINS) -> String {
        if !rem.0.is_same_anykind(&rem.1) {
            panic!("ERROR NOT SAME KIND REGS WERE REMAINDER")
        }
        let kind = rem.0.get_reg_kind();
        return match kind {
            RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                panic!("ERROR FLOAT NOT SUPPORT REM")
            }
            RegisterKind::Normal | RegisterKind::AnyNormal => {
                format!(
                    "{}remw {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(rem.0),
                    self.get_reg_name(rem.1),
                    self.get_reg_name(rem.2)
                )
            }
        };
    }

    fn tran_add(&self, add: ADDINS) -> String {
        match &add.3 {
            RegOrImm::Imm(imm) => {
                format!(
                    "{}addi {},{},{}",
                    BEF_MARGIN,
                    self.get_reg_name(add.1),
                    self.get_reg_name(add.2),
                    imm.translate_to_i64_uncheck()
                )
            }
            RegOrImm::Reg(reg) => {
                if !add.1.is_same_anykind(&reg) {
                    panic!("ERROR NOT SAME KIND REG  WERE ADDED {:?}", reg)
                }
                let kind = reg.get_reg_kind();
                match kind {
                    RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                        if add.0 == WidthType::DoubleWord {
                            panic!("NOT SUPPORT DOUBLE ADD")
                        }
                        format!(
                            "    fadd.s {},{},{}",
                            self.get_reg_name(add.1),
                            self.get_reg_name(add.2),
                            self.get_reg_name(reg.clone())
                        )
                    }
                    RegisterKind::Normal | RegisterKind::AnyNormal => {
                        match add.0 {
                            WidthType::Word => {
                                format!(
                                    "    addw {},{},{}",
                                    self.get_reg_name(add.1),
                                    self.get_reg_name(add.2),
                                    self.get_reg_name(reg.clone())
                                )
                            }
                            WidthType::DoubleWord => {
                                format!(
                                    "    add {},{},{}",
                                    self.get_reg_name(add.1),
                                    self.get_reg_name(add.2),
                                    self.get_reg_name(reg.clone())
                                )
                            }
                            _ => {
                                panic!("不支持的宽度相加")
                            }
                        }
                    }
                }
            }
        }
    }

    fn tran_slt(&self, slt: SLTINS) -> String {
        return match slt.2 {
            RegOrImm::Imm(imm) => {
                if slt.1.is_in(RegisterKind::AnyFNormal) {
                    panic!("riscv not support direct load float to compare")
                }
                let ret = imm.translate_to_i64_uncheck();
                format!("{}slti {},{},{}", BEF_MARGIN, self.get_reg_name(slt.0), self.get_reg_name(slt.1), ret)
            }
            RegOrImm::Reg(reg) => {
                if !slt.0.is_in(RegisterKind::AnyNormal) {
                    panic!("target register must be normal register")
                }
                return if slt.1.is_in(RegisterKind::AnyNormal) && reg.is_in(RegisterKind::AnyNormal) {
                    let reg = self.get_reg_name(reg);
                    format!("{}slt {},{},{}", BEF_MARGIN, self.get_reg_name(slt.0), self.get_reg_name(slt.1), reg)
                } else if slt.1.is_in(RegisterKind::AnyFNormal) && reg.is_in(RegisterKind::AnyFNormal) {
                    let reg = self.get_reg_name(reg);
                    format!("{}flt.s {},{},{}", BEF_MARGIN, self.get_reg_name(slt.0), self.get_reg_name(slt.1), reg)
                } else {
                    panic!("the type of positive1 {:?} and positive2 {:?} are not same", slt.1, reg)
                }
            }
        };
    }

    fn tran_store(&self, store: STOREINS) -> String {
        let kind = store.1.get_reg_kind();
        match kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => {
                let sx = match store.0 {
                    WidthType::Byte => "sb",
                    WidthType::Word => "sw",
                    WidthType::HalfWord => "sh",
                    WidthType::DoubleWord => "sd",
                };
                format!(
                    "    {} {},{}({})",
                    sx,
                    self.get_reg_name(store.1),
                    store.2.offset,
                    self.get_reg_name(store.2.base_addr_reg)
                )
            }
            RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                let fsx = match store.0 {
                    WidthType::Byte => {
                        panic!("ERROR BYTE NOT SUPPORT FLOAT STORE")
                    }
                    WidthType::Word => "fsw",
                    WidthType::HalfWord => {
                        panic!("ERROR HALF WORD NOT SUPPORT FLOAT STORE")
                    }
                    WidthType::DoubleWord => "fsd",
                };
                format!(
                    "{}{} {},{}({})",
                    BEF_MARGIN,
                    fsx,
                    self.get_reg_name(store.1),
                    store.2.offset,
                    self.get_reg_name(store.2.base_addr_reg)
                )
            }
        }
    }

    fn tran_loadu(&self, loadu: LOADUINS) -> String {
        match &loadu.2 {
            RegWithOffOrImm::Imm(immediate) => {
                format!("    li {},{}", self.get_reg_name(loadu.1), immediate.translate_to_i64_uncheck())
            }
            RegWithOffOrImm::RegWO(reg) => {
                let lx = match loadu.0 {
                    WidthType::Byte => "lbu",
                    WidthType::Word => "lwu",
                    WidthType::HalfWord => "lhu",
                    WidthType::DoubleWord => {
                        panic!("NEVER HERE")
                    }
                };
                format!(
                    "    {} {},{}({})",
                    lx,
                    self.get_reg_name(loadu.1),
                    reg.offset,
                    self.get_reg_name(reg.base_addr_reg)
                )
            }
        }
    }

    fn tran_load(&self, load: LOADINS) -> String {
        match &load.2 {
            RegWithOffOrImm::Imm(immediate) => {
                if load.1.is_in(RegisterKind::AnyFNormal) {
                    panic!("ERROR RISCV NOT SUPPORT DIRECT LOAD FLOAT")
                }
                format!("    li {},{}", self.get_reg_name(load.1), immediate.translate_to_imm())
            }
            RegWithOffOrImm::RegWO(reg) => {
                let kind = load.1.get_reg_kind();
                match kind {
                    RegisterKind::Normal | RegisterKind::AnyNormal => {
                        let lx = match load.0 {
                            WidthType::Byte => "lb",
                            WidthType::Word => "lw",
                            WidthType::HalfWord => "lh",
                            WidthType::DoubleWord => "ld",
                        };
                        format!(
                            "    {} {},{}({})",
                            lx,
                            self.get_reg_name(load.1),
                            reg.offset,
                            self.get_reg_name(reg.base_addr_reg)
                        )
                    }
                    RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                        let fsx = match load.0 {
                            WidthType::Byte => {
                                panic!("ERROR BYTE NOT SUPPORT FLOAT LOAD")
                            }
                            WidthType::Word => "flw",
                            WidthType::HalfWord => {
                                panic!("ERROR HALF WORD NOT SUPPORT FLOAT LOAD")
                            }
                            WidthType::DoubleWord => "fld",
                        };
                        format!(
                            "{}{} {},{}({})",
                            BEF_MARGIN,
                            fsx,
                            self.get_reg_name(load.1),
                            reg.offset,
                            self.get_reg_name(reg.base_addr_reg)
                        )
                    }
                }
            }
        }
    }

    fn tran_sll(&self, sll: SLLINS) -> String {
        if sll.0.is_in(RegisterKind::AnyFNormal) || sll.0.is_in(RegisterKind::AnyFNormal) {
            panic!("REG KIND ERROR");
        };
        let target = self.get_reg_name(sll.0);
        let source = self.get_reg_name(sll.1);
        let end = match sll.2 {
            RegOrImm::Imm(imm) => {
                let imm = imm.translate_to_i64_uncheck().to_string();
                format!("{}slli {},{},{}", BEF_MARGIN, target, source, imm)
            }
            RegOrImm::Reg(reg) => {
                if reg.is_in(RegisterKind::AnyFNormal) {
                    panic!("REG KIND ERROR");
                }
                let reg = self.get_reg_name(reg);
                format!("{}sll {},{},{}", BEF_MARGIN, target, source, reg)
            }
        };
        end
    }

    fn tran_neg(&self, neg: NEGINS) -> String {
        if neg.0.get_reg_kind() != neg.1.get_reg_kind() {
            panic!("ERROR ERG KIND UNMATCH")
        }
        let kind = neg.0.get_reg_kind();
        match kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => {
                format!("{}neg {},{}", BEF_MARGIN, self.get_reg_name(neg.0), self.get_reg_name(neg.1))
            }
            RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                format!("{}fneg.s {},{}", BEF_MARGIN, self.get_reg_name(neg.0), self.get_reg_name(neg.1))
            }
        }
    }
}

impl ImmediateBits {
    // FIXME
    /// 危险函数，未稳定，负责对字面量的合适转译，注意这里的字面量全是无符号，
    /// 除了Bits12 ,Bits12用作寻址，虽然其他地方也能用，但是不建议
    fn translate_to_i64_uncheck(&self) -> i64 {
        match self {
            ImmediateBits::ImmI12(i12) => {
                if !i12.is_in_i12() {
                    panic!("i12 超出12位 {:?}",i12)
                }else {
                    *i12 as i64
                }
            }
            ImmediateBits::ImmWordI32(i32) => {
                *i32 as i64
            }
            ImmediateBits::ImmWordU32(u32) => {
                *u32 as i64
            }
            ImmediateBits::ImmDoubleWordI64(i64) => {
                *i64
            }
        }
    }

    fn translate_to_imm(&self) -> String {
        match self {
            ImmediateBits::ImmI12(i12) => {
                println!("检测到i12以长立即数的方式加载");
                i12.to_string()
            }
            ImmediateBits::ImmWordI32(i32) => {
                i32.to_string()
            }
            ImmediateBits::ImmWordU32(u32) => {
                u32.to_string()
            }
            ImmediateBits::ImmDoubleWordI64(i64) => {
                i64.to_string()
            }
        }
    }
}
