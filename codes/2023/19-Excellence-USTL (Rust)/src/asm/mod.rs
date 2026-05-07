// risc-v架构
mod riscv;
// arm架构
mod arm;
// 龙架构
mod loong;
mod tests;

use crate::ast::table::SymbolTable;
use crate::lir::lir::{Annotation, HRegister, LirPartCode, LirStatement};
use crate::util::Kotlin;
pub use riscv::RISCVBackend;
use std::fs::File;
use std::io::Write;
use std::ops::Add;

/// ASM 组织器
pub struct ASMOrganizer<'a> {
    text_region: Vec<Vec<(LirStatement, Option<Annotation>)>>,
    static_region: Vec<Vec<LirStatement>>,
    ro_region: Vec<Vec<LirStatement>>,
    asm_generator: ASMTranslate<'a>,
}

impl<'a> ASMOrganizer<'a> {
    pub(crate) fn new(asm: ASMTranslate) -> ASMOrganizer {
        ASMOrganizer {
            text_region: vec![],
            static_region: vec![],
            ro_region: vec![],
            asm_generator: asm,
        }
    }
    pub(crate) fn write_to(mut self, path: &std::path::Path) {
        let file = File::create(path);
        let mut io = file.unwrap();
        let ret = io.write(&"# YanDevelop 2023 5 22 \n".to_string().into_bytes());
        if ret.is_err() {
            panic!("Write Error")
        }
        let ret = io.write(&"# text region   \n".to_string().into_bytes());
        if ret.is_err() {
            panic!("Write Error")
        }
        let ret = io.write(&"	.text  \n".to_string().into_bytes());
        for text in self.text_region {
            if ret.is_err() {
                panic!("Write Error")
            }
            for ins in text {
                let ins = self.asm_generator.translate_lir(ins.0, ins.1);
                let ret = io.write(&ins.into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
                let ret = io.write(&"\n".to_string().into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
            }
        }
        let ret = io.write(&"   \n".to_string().into_bytes());
        if ret.is_err() {
            panic!("Write Error")
        }
        if self.static_region.len() > 0 {
            let ret = io.write(&"# static region \n".to_string().into_bytes());
            if ret.is_err() {
                panic!("Write Error")
            }
        }
        let ret = io.write(
            &"	.section	.sdata,\"aw\",@progbits \n"
                .to_string()
                .into_bytes(),
        );
        for text in self.static_region {
            if ret.is_err() {
                panic!("Write Error")
            }
            let ret = io.write(&"   \n".to_string().into_bytes());
            if ret.is_err() {
                panic!("Write Error")
            }
            for ins in text {
                let ins = self.asm_generator.translate_lir(ins, None);
                let ret = io.write(&ins.into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
                let ret = io.write(&"\n".to_string().into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
            }
        }

        let ret = io.write(&"# read only region \n".to_string().into_bytes());
        let mut ro_defined = false;
        for text in self.ro_region {
            if ret.is_err() {
                panic!("Write Error")
            }
            let ret = io.write(&"   \n".to_string().into_bytes());
            if ret.is_err() {
                panic!("Write Error")
            }
            for ins in text {
                if !ro_defined {
                    ro_defined = true;
                    let ret =
                        io.write(&"	.section	.rodata,\"a\",@progbits\n".to_string().into_bytes());
                    if ret.is_err() {
                        panic!("Write Error")
                    }
                    // todo 非致命的错误翻译
                }
                let ins = self.asm_generator.translate_lir(ins, None);
                let ret = io.write(&ins.into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
                let ret = io.write(&"\n".to_string().into_bytes());
                if ret.is_err() {
                    panic!("Write Error")
                }
            }
        }
    }
    pub(crate) fn organ(&mut self, lir: LirPartCode) {
        for global in lir.globals {
            if global.is_ro {
                self.ro_region.push(global.statements)
            } else {
                self.static_region.push(global.statements)
            }
        }
        for fun in lir.fun_blocks {
            let stmt = fun.statements.collect_to_vec();
            self.text_region.push(stmt)
        }
    }
}

/// 后端特性
/// the default implement is risc-v isa
pub trait ISABackend {
    fn translate_stmt(&self, lir: LirStatement, name_table: &SymbolTable) -> String;
    /// 是否支持zero寄存器
    fn support_zero(&self) -> bool {
        true
    }
    /// 是否支持栈帧指针寄存器 ， 这个fp还是很头大的，虽然 fp  = sp - stack_size ,
    /// 然后 如果定位位于上一个函数内部的变量，最好还是得有这个fp指针，如果没有则需要sp定位
    fn support_fp(&self) -> bool {
        true
    }
    /// 是否支持浮点寄存器
    fn support_freg(&self) -> bool {
        true
    }
    /// param register number
    fn preg_num(&self) -> u8 {
        // a0 - a7
        8
    }
    /// saved register number
    fn sreg_num(&self) -> u8 {
        // riscv s1-s11 , s0 have been used to as fp
        11
    }
    /// temp register number
    fn treg_num(&self) -> u8 {
        // t0 - t6
        7
    }
    /// float saved register number
    fn fsreg_num(&self) -> u8 {
        12
    }
    /// float temp register number
    fn ftreg_num(&self) -> u8 {
        12
    }
    /// float param register number
    fn fpreg_num(&self) -> u8 {
        8
    }

    /// 是否支持浮点立即数直接加载
    fn is_support_lfi(&self) -> bool {
        false
    }

    fn get_reg_name(&self, which: HRegister) -> String {
        match which {
            HRegister::zero => "x0".to_string(),
            HRegister::ra => "ra".to_string(),
            HRegister::sp => "sp".to_string(),
            HRegister::fp => "fp".to_string(),
            //
            HRegister::gp => "gp".to_string(),
            HRegister::tp => "tp".to_string(),
            //
            HRegister::p0 => "a0".to_string(),
            HRegister::p1 => "a1".to_string(),
            HRegister::p2 => "a2".to_string(),
            HRegister::p3 => "a3".to_string(),
            HRegister::p4 => "a4".to_string(),
            HRegister::p5 => "a5".to_string(),
            HRegister::p6 => "a6".to_string(),
            HRegister::p7 => "a7".to_string(),
            HRegister::p8 => {
                panic!("ERROR")
            }
            HRegister::p9 => {
                panic!("ERROR")
            }
            HRegister::t0 => "t0".to_string(),
            HRegister::t1 => "t1".to_string(),
            HRegister::t2 => "t2".to_string(),
            HRegister::t3 => "t3".to_string(),
            HRegister::t4 => "t4".to_string(),
            HRegister::t5 => "t5".to_string(),
            HRegister::t6 => "t6".to_string(),
            HRegister::t7 => "t7".to_string(),
            HRegister::t8 => "t8".to_string(),
            HRegister::t9 => "t9".to_string(),
            HRegister::s0 => "s1".to_string(), // in risc-v s0 -> fp
            HRegister::s1 => "s2".to_string(),
            HRegister::s2 => "s3".to_string(),
            HRegister::s3 => "s4".to_string(),
            HRegister::s4 => "s5".to_string(),
            HRegister::s5 => "s6".to_string(),
            HRegister::s6 => "s7".to_string(),
            HRegister::s7 => "s8".to_string(),
            HRegister::s8 => "s9".to_string(),
            HRegister::s9 => "s10".to_string(),
            HRegister::s10 => "s11".to_string(),
            HRegister::s11 => {
                panic!("ERROR")
            }
            HRegister::s12 => {
                panic!("ERROR")
            }
            HRegister::s13 => {
                panic!("ERROR")
            }
            HRegister::s14 => {
                panic!("ERROR")
            }
            HRegister::ft0 => "ft0".to_string(),
            HRegister::ft1 => "ft1".to_string(),
            HRegister::ft2 => "ft2".to_string(),
            HRegister::ft3 => "ft3".to_string(),
            HRegister::ft4 => "ft4".to_string(),
            HRegister::ft5 => "ft5".to_string(),
            HRegister::ft6 => "ft6".to_string(),
            HRegister::ft7 => "ft7".to_string(),
            HRegister::ft8 => "ft8".to_string(),
            HRegister::ft9 => "ft9".to_string(),
            HRegister::ft10 => "ft10".to_string(),
            HRegister::ft11 => "ft11".to_string(),
            HRegister::ft12 => {
                panic!("ERROR")
            }
            HRegister::ft13 => {
                panic!("ERROR")
            }
            HRegister::ft14 => {
                panic!("ERROR")
            }
            HRegister::fs0 => "fs0".to_string(),
            HRegister::fs1 => "fs1".to_string(),
            HRegister::fs2 => "fs2".to_string(),
            HRegister::fs3 => "fs3".to_string(),
            HRegister::fs4 => "fs4".to_string(),
            HRegister::fs5 => "fs5".to_string(),
            HRegister::fs6 => "fs6".to_string(),
            HRegister::fs7 => "fs7".to_string(),
            HRegister::fs8 => "fs8".to_string(),
            HRegister::fs9 => "fs9".to_string(),
            HRegister::fs10 => "fs10".to_string(),
            HRegister::fs11 => "fs11".to_string(),
            HRegister::fs12 => {
                panic!("ERROR")
            }
            HRegister::fs13 => {
                panic!("ERROR")
            }
            HRegister::fs14 => {
                panic!("ERROR")
            }
            //
            HRegister::fp0 => "fa0".to_string(),
            HRegister::fp1 => "fa1".to_string(),
            HRegister::fp2 => "fa2".to_string(),
            HRegister::fp3 => "fa3".to_string(),
            HRegister::fp4 => "fa4".to_string(),
            HRegister::fp5 => "fa5".to_string(),
            HRegister::fp6 => "fa6".to_string(),
            HRegister::fp7 => "fa7".to_string(),
            HRegister::fp8 => {
                panic!("ERROR")
            }
            HRegister::fp9 => {
                panic!("ERROR")
            }
        }
    }
}

// 获得特定种类的特定寄存器
pub fn get_reg<'a>(
    isa: &'a dyn ISABackend,
    reg_type: DetailedRegisterKind,
    which: u8,
) -> HRegister {
    match reg_type {
        DetailedRegisterKind::Temp => {
            if which > isa.treg_num() {
                panic!("ERROR")
            }
            match which {
                0 => HRegister::t0,
                1 => HRegister::t1,
                2 => HRegister::t2,
                3 => HRegister::t3,
                4 => HRegister::t4,
                5 => HRegister::t5,
                6 => HRegister::t6,
                7 => HRegister::t7,
                8 => HRegister::t8,
                9 => HRegister::t9,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
        DetailedRegisterKind::Saved => {
            if which > isa.sreg_num() {
                panic!("ERROR")
            }
            match which {
                0 => HRegister::s0,
                1 => HRegister::s1,
                2 => HRegister::s2,
                3 => HRegister::s3,
                4 => HRegister::s4,
                5 => HRegister::s5,
                6 => HRegister::s6,
                7 => HRegister::s7,
                8 => HRegister::s8,
                9 => HRegister::s9,
                10 => HRegister::s10,
                11 => HRegister::s11,
                12 => HRegister::s12,
                13 => HRegister::s13,
                14 => HRegister::s14,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
        DetailedRegisterKind::Param => {
            if which > isa.preg_num() {
                panic!("ERROR , THERE IS NO SUFFICIENT PARAM REG")
            }
            match which {
                0 => HRegister::p0,
                1 => HRegister::p1,
                2 => HRegister::p2,
                3 => HRegister::p3,
                4 => HRegister::p4,
                5 => HRegister::p5,
                6 => HRegister::p6,
                7 => HRegister::p7,
                8 => HRegister::p8,
                9 => HRegister::p9,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
        DetailedRegisterKind::FTemp => {
            if which > isa.ftreg_num() {
                panic!("ERROR")
            }
            match which {
                0 => HRegister::ft0,
                1 => HRegister::ft1,
                2 => HRegister::ft2,
                3 => HRegister::ft3,
                4 => HRegister::ft4,
                5 => HRegister::ft5,
                6 => HRegister::ft6,
                7 => HRegister::ft7,
                8 => HRegister::ft8,
                9 => HRegister::ft9,
                10 => HRegister::ft10,
                11 => HRegister::ft11,
                12 => HRegister::ft12,
                13 => HRegister::ft13,
                14 => HRegister::ft14,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
        DetailedRegisterKind::FSaved => {
            if which > isa.fsreg_num() {
                panic!("ERROR")
            }
            match which {
                0 => HRegister::fs0,
                1 => HRegister::fs1,
                2 => HRegister::fs2,
                3 => HRegister::fs3,
                4 => HRegister::fs4,
                5 => HRegister::fs5,
                6 => HRegister::fs6,
                7 => HRegister::fs7,
                8 => HRegister::fs8,
                9 => HRegister::fs9,
                10 => HRegister::fs10,
                11 => HRegister::fs11,
                12 => HRegister::fs12,
                13 => HRegister::fs13,
                14 => HRegister::fs14,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
        DetailedRegisterKind::FParam => {
            if which > isa.fpreg_num() {
                panic!("ERROR")
            }
            match which {
                0 => HRegister::fp0,
                1 => HRegister::fp1,
                2 => HRegister::fp2,
                3 => HRegister::fp3,
                4 => HRegister::fp4,
                5 => HRegister::fp5,
                6 => HRegister::fp6,
                7 => HRegister::fp7,
                8 => HRegister::fp8,
                9 => HRegister::fp9,
                _ => {
                    panic!("NEVER HERE")
                }
            }
        }
    }
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum RegisterKind {
    // 非Save,fp,sp的任意高级通用寄存器
    Normal,
    // 非FSave的任意高级浮点寄存器
    FNormal,
    // 任意高级通用寄存器但不包含 fp,sp
    AnyNormal,
    // 任意高级浮点寄存器
    AnyFNormal,
}

pub enum DetailedRegisterKind {
    Temp,
    Saved,
    Param,
    FTemp,
    FSaved,
    FParam,
}

/// ASM 局部翻译
pub struct ASMTranslate<'a> {
    translate_backend: &'a dyn ISABackend,
    name_table: &'a SymbolTable,
}

impl<'a> ASMTranslate<'a> {
    pub fn translate_lir(&mut self, lir_part: LirStatement, ann: Option<Annotation>) -> String {
        self.translate_backend
            .translate_stmt(lir_part, self.name_table)
            .apply_once(|it| {
                if ann.is_some() {
                    it.add(&format!("\t# {}", ann.unwrap())) // risc-v and loong arch support # prefix started annotation
                } else {
                    it
                }
            })
    }

    pub fn new(backend: &'a dyn ISABackend, name_table: &'a SymbolTable) -> Self {
        Self {
            translate_backend: backend,
            name_table,
        }
    }
}
