use crate::allow_nothing;
use crate::asm::ISABackend;
use crate::ast::table::{AnySymbol, AnySymbolRef, SymbolTable};
use crate::lir::lir::ImmediateBits::{ImmI12, ImmWordI32};
use crate::lir::lir::{
    Annotation, HRegister, ImmediateBits, LabelTag, LirBlock, LirGlobal, LirPartCode, LirStatement, RegOrImm, RegWithOff,
    RegWithOffOrImm, WidthType, ADDINS, ANDINS, I12, IMM, LOADINS, RAWINS, ROI, RWOOI, STOREINS, SUBINS,
};
use crate::util::{ElementRef, HashInsertVec};
use std::collections::{HashMap, HashSet};

#[derive(Clone, Hash, Eq, PartialEq)]
pub struct SessionTag {
    name: AnySymbolRef,
}

impl SessionTag {
    fn new(name: AnySymbolRef) -> SessionTag {
        SessionTag {
            name,
        }
    }
    pub fn get_label_tag(&self) -> LabelTag {
        self.name.clone()
    }
}

/// LIR 符号集
/// 指令或者标记
#[derive(Clone, Eq, Hash, PartialEq)]
pub(crate) enum LirOrTag {
    Tag(SessionTag),
    Statement(LirStatement),
}

#[derive(Clone, Eq, Hash, PartialEq)]
struct LIRINS {
    lir: LirOrTag,
    annotation: Option<Annotation>,
}

pub(crate) enum SessionMode {
    Append,
    TagBefore(SessionTag),
}

pub(crate) struct Session {
    blocks: Vec<LirBlock>,
    globals: Vec<LirGlobal>,

    current_ins: HashInsertVec<LIRINS>,
    session_mode: SessionMode,
    tag_maps: HashMap<SessionTag, ElementRef<LIRINS>>,
    fun_start_tag: Option<SessionTag>,
    fun_ret_tags: HashSet<SessionTag>,
    // 因为涉及到添加全局变量 ， 进入函数，退出函数等语句 ，所以Backend 在这里是必要的
    // 但是目前采用的策略是仅仅实现RISC-V,而且是固定实现
    #[allow(unused)]
    isa_backend: &'static dyn ISABackend,
}

/// LIR 编排器
impl Session {
    pub fn new(isa: &'static dyn ISABackend) -> Self {
        Session {
            current_ins: HashInsertVec::new(),
            blocks: vec![],
            globals: vec![],
            session_mode: SessionMode::Append,
            tag_maps: Default::default(),
            fun_start_tag: None,
            fun_ret_tags: HashSet::new(),
            isa_backend: isa,
        }
    }

    /// 根据编排模式，填入LIR指令
    pub fn push(&mut self, lir: LirStatement) {
        let lir = LIRINS {
            lir: LirOrTag::Statement(lir),
            annotation: None,
        };
        match &self.session_mode {
            SessionMode::Append => {
                self.current_ins.push(lir);
            }
            SessionMode::TagBefore(tag) => {
                let ele_ref = &self.tag_maps[tag];
                self.current_ins.insert_before(lir, ele_ref);
            }
        }
    }

    #[allow(unused)]
    pub fn push_with_ann(&mut self, lir: LirStatement, annotation: Option<Annotation>) {
        let lir = LIRINS {
            lir: LirOrTag::Statement(lir),
            annotation,
        };
        match &self.session_mode {
            SessionMode::Append => {
                self.current_ins.push(lir);
            }
            SessionMode::TagBefore(tag) => {
                let ele_ref = &self.tag_maps[tag];
                self.current_ins.insert_before(lir, ele_ref);
            }
        }
    }

    /// 根据当前的编排模式来添加标记
    /// 如果是TagBefore模式 则 添加tag到给定Tag(SessionMode中的成员)的前面
    /// 如果是Append模式 则 添加到最后
    /// 如果是TagAfter模式 则 FIXME 待定
    pub fn tag_now(&mut self, name: AnySymbolRef) -> SessionTag {
        let session_tag = SessionTag::new(name);
        let ins = LIRINS {
            lir: LirOrTag::Tag(session_tag.clone()),
            annotation: None,
        };

        let ele_ref = match &self.session_mode {
            SessionMode::Append => self.current_ins.push(ins),
            SessionMode::TagBefore(tag) => {
                let ele_ref = &self.tag_maps[tag];
                self.current_ins.insert_before(ins, ele_ref)
            }
        };
        self.tag_maps.insert(session_tag.clone(), ele_ref);
        session_tag
    }

    ///
    ///
    /// # Arguments
    ///
    /// * `session_tag`:
    /// * `name`: 如果该SessionTag要作为跳转的Label, name 必须遵循ABI规范
    ///
    /// returns: SessionTag
    ///
    #[allow(unused)]
    pub fn tag_before(&mut self, session_tag: &SessionTag, name: AnySymbolRef) -> SessionTag {
        let new_session_tag = SessionTag::new(name);
        let ele_rf = self.tag_maps[session_tag].clone();
        let lir_ins = LIRINS {
            lir: LirOrTag::Tag(new_session_tag.clone()),
            annotation: None,
        };
        let new_ele_rf = self.current_ins.insert_before(lir_ins, &ele_rf);
        self.tag_maps.insert(new_session_tag.clone(), new_ele_rf);
        new_session_tag
    }

    pub fn remove_tag(&mut self, tag: &SessionTag) {
        let session = self.tag_maps.remove(tag).unwrap();
        self.current_ins.remove(&session);
    }

    /// 切换为TagBefore，从tag处向前加入语句，先加入的语句被放到前面
    pub fn set_push_before(&mut self, tag: &SessionTag) {
        self.session_mode = SessionMode::TagBefore(tag.clone())
    }

    /// 切换为Append模式
    pub fn set_append(&mut self) {
        self.session_mode = SessionMode::Append;
    }

    /// 加入必要的LIR定位信息,添加Label
    pub fn enter_fun(&mut self, fun_name: String, tag_symbol: AnySymbolRef) {
        // 完全RISCV翻译
        let global = format!("	.globl	{}", fun_name);
        let p2align = format!("	.p2align	{}", 1);
        let code_type = format!("	.type	{},@function", fun_name);
        let fun_label = format!("{}:", fun_name);
        self.push(LirStatement::Raw(RAWINS(global)));
        self.push(LirStatement::Raw(RAWINS(p2align)));
        self.push(LirStatement::Raw(RAWINS(code_type)));
        self.push(LirStatement::Raw(RAWINS(fun_label)));
        let tag = self.tag_now(tag_symbol);
        self.fun_start_tag = Some(tag)
    }
    /// 注意这是错的,栈大小当前是未知的

    pub fn exit_fun(&mut self, fun_name: String, stack_size: usize, need_append_ret: bool, table: &mut SymbolTable) {
        let stack_rem = stack_size % 16;
        let need_append = 16 - stack_rem;
        if self.fun_start_tag.is_none() {
            panic!("exit fun was invoked in error place")
        }
        // 栈处理中间层

        // 设置栈帧
        let stack_size = (stack_size + need_append) as i32;
        println!("{} stack {} ",fun_name, stack_size);
        // 注意高地址是底部 , 这里只支持了递减方式的的堆栈
        let sp_base_stack_bottom = stack_size;

        let fun_start_tag = self.fun_start_tag.clone().unwrap();
        self.set_push_before(&fun_start_tag);

        self.push(LirStatement::Add(ADDINS(
            WidthType::DoubleWord,
            HRegister::sp,
            HRegister::sp,
            RegOrImm::Imm(ImmWordI32(-stack_size)),
        )));
        self.push(LirStatement::Store(STOREINS(
            WidthType::DoubleWord,
            HRegister::ra,
            RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 8),
        )));
        self.push(LirStatement::Store(STOREINS(
            WidthType::DoubleWord,
            HRegister::fp,
            RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 16),
        )));
        self.push(LirStatement::Add(ADDINS(
            WidthType::DoubleWord,
            HRegister::fp,
            HRegister::sp,
            ROI::Imm(ImmWordI32(stack_size)),
        )));

        self.remove_tag(&fun_start_tag);
        self.set_append();

        // let end_sym__ = AnySymbol::new(&format!(".SYSYC_{}_fun_end", fun_name));
        // let sym_ref__ = table.add_symbol(end_sym__);
        // let fun_end_tag = self.tag_now(sym_ref__); // 标记为函数体结束

        if need_append_ret {
            // 目前已经知道了stack_size 没有必要延后处理
            self.push(LirStatement::Load(LOADINS(
                WidthType::DoubleWord,
                HRegister::ra,
                RWOOI::RegWO(RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 8)),
            )));
            self.push(LirStatement::Load(LOADINS(
                WidthType::DoubleWord,
                HRegister::fp,
                RegWithOffOrImm::RegWO(RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 16)),
            )));
            self.push(LirStatement::Add(ADDINS(
                WidthType::DoubleWord,
                HRegister::sp,
                HRegister::sp,
                RegOrImm::Imm(ImmWordI32(stack_size)),
            )));
            self.push(LirStatement::Return);
        }

        // 插入结束标记
        let exit_fun_label = format!(".L{}_end:", fun_name);
        let size = format!("	.size	{0}, .L{0}_end - {0}", fun_name);
        self.push(LirStatement::Raw(RAWINS(exit_fun_label)));
        self.push(LirStatement::Raw(RAWINS(size)));
        self.push(LirStatement::Raw(RAWINS(" ".to_string()))); // 先填充指令，后面再转译

        // 处理所有return
        for single in self.fun_ret_tags.clone() {
            self.set_push_before(&single);
            self.push(LirStatement::Load(LOADINS(
                WidthType::DoubleWord,
                HRegister::ra,
                RWOOI::RegWO(RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 8)),
            )));
            self.push(LirStatement::Load(LOADINS(
                WidthType::DoubleWord,
                HRegister::fp,
                RegWithOffOrImm::RegWO(RegWithOff::suggest_sp_top_based(HRegister::sp, sp_base_stack_bottom - 16)),
            )));
            self.push(LirStatement::Add(ADDINS(
                WidthType::DoubleWord,
                HRegister::sp,
                HRegister::sp,
                RegOrImm::Imm(IMM::ImmWordU32(stack_size as u32)),
            )));
            self.push(LirStatement::Return);
            self.remove_tag(&single);
        }

        self.finish_fun(fun_name, table);
        self.current_ins = HashInsertVec::new();
        self.fun_ret_tags.clear();
        self.fun_start_tag = None;
        self.session_mode = SessionMode::Append;
        self.tag_maps.clear();
    }

    fn finish_fun(&mut self, fun_name: String, table: &mut SymbolTable) {
        let mut lir_block = LirBlock::new(fun_name);

        // 这个信息本应该从ISABackend里面获取
        let short_jump_max = 2047;
        let short_jump_min = -2048;

        let mut wait_add_jump = None;

        let iter = self.current_ins.iter();
        for single_ins in iter {
            match single_ins.lir {
                LirOrTag::Tag(tag) => {
                    let current_label = tag.get_label_tag();

                    match wait_add_jump {
                        None => {
                            allow_nothing!();
                        }
                        Some(jump_target) => {
                            if  jump_target != current_label {
                                lir_block.add_stmt(LirStatement::Jump(jump_target));
                                wait_add_jump = None;
                            }
                        }
                    }

                    // 通用标记
                    lir_block.add_stmt_with_ann(
                        LirStatement::Raw(RAWINS(format!("{}:", table.get_symbol(&tag.name).get_string()))),
                        single_ins.annotation,
                    );
                } // Tag 的翻译
                LirOrTag::Statement(stat) => {
                    match wait_add_jump {
                        None => {
                            allow_nothing!();
                        }
                        Some(jump_target) => {
                            lir_block.add_stmt(LirStatement::Jump(jump_target));
                            wait_add_jump = None;
                        }
                    }
                    match &stat {
                        // add 调整
                        LirStatement::Add(add) => {
                            match &add.3 {
                                RegOrImm::Imm(imm) => match imm {
                                    ImmediateBits::ImmI12(i12) => {
                                        if !i12.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordI32(*i12 as i32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Add(ADDINS(add.0, add.1, add.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        }
                                    }
                                    ImmediateBits::ImmWordI32(i32) => {
                                        if !i32.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordI32(*i32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Add(ADDINS(add.0, add.1, add.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        } else {
                                            lir_block.add_stmt(LirStatement::Add(ADDINS(add.0, add.1, add.2, RegOrImm::Imm(ImmI12(*i32 as i16)))));
                                            continue;
                                        }
                                    }
                                    IMM::ImmWordU32(u32) => {
                                        if !u32.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordU32(*u32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Add(ADDINS(add.0, add.1, add.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        } else {
                                            lir_block.add_stmt(LirStatement::Add(ADDINS(add.0, add.1, add.2, RegOrImm::Imm(ImmI12(*u32 as i16)))));
                                            continue;
                                        }
                                    }
                                    _ => {
                                        panic!("NEVER HERE")
                                    }
                                },
                                RegOrImm::Reg(_) => {
                                    allow_nothing!();
                                }
                            }
                            lir_block.add_stmt_with_ann(stat, single_ins.annotation);
                        }
                        // sub
                        LirStatement::Sub(sub) => {
                            match &sub.3 {
                                RegOrImm::Imm(imm) => match imm {
                                    ImmediateBits::ImmI12(i12) => {
                                        if !i12.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordI32(*i12 as i32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Sub(SUBINS(sub.0, sub.1, sub.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        }
                                    }
                                    ImmediateBits::ImmWordI32(i32) => {
                                        if !i32.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordI32(*i32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Sub(SUBINS(sub.0, sub.1, sub.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        } else {
                                            lir_block.add_stmt(LirStatement::Sub(SUBINS(sub.0, sub.1, sub.2, RegOrImm::Imm(ImmI12(*i32 as i16)))));
                                            continue;
                                        }
                                    }
                                    IMM::ImmWordU32(u32) => {
                                        if !u32.is_in_i12() {
                                            lir_block.add_stmt(LirStatement::Load(LOADINS(
                                                WidthType::Word,
                                                HRegister::t0,
                                                RWOOI::Imm(IMM::ImmWordU32(*u32)),
                                            )));
                                            lir_block.add_stmt(LirStatement::Sub(SUBINS(sub.0, sub.1, sub.2, RegOrImm::Reg(HRegister::t0))));
                                            continue;
                                        } else {
                                            lir_block.add_stmt(LirStatement::Sub(SUBINS(sub.0, sub.1, sub.2, RegOrImm::Imm(ImmI12(*u32 as i16)))));
                                            continue;
                                        }
                                    }
                                    _ => {
                                        panic!("NEVER HERE")
                                    }
                                },
                                RegOrImm::Reg(_) => {
                                    allow_nothing!();
                                }
                            }
                            lir_block.add_stmt_with_ann(stat, single_ins.annotation);
                        }
                        // load 调整
                        LirStatement::Load(load) => {
                            match &load.2 {
                                RegWithOffOrImm::Imm(_) => {
                                    allow_nothing!();
                                }
                                RegWithOffOrImm::RegWO(reg_with_off) => {
                                    if !(reg_with_off.offset < short_jump_min || reg_with_off.offset > short_jump_max) {
                                        allow_nothing!();
                                    } else {
                                        let load_dist = LirStatement::Load(LOADINS {
                                            0: WidthType::DoubleWord,
                                            1: HRegister::t0,
                                            2: RegWithOffOrImm::Imm(ImmediateBits::ImmWordI32(reg_with_off.offset)),
                                        });
                                        let move_base_pos = LirStatement::Add(ADDINS {
                                            0: WidthType::DoubleWord,
                                            1: HRegister::t0,
                                            2: HRegister::t0,
                                            3: RegOrImm::Reg(reg_with_off.base_addr_reg),
                                        });
                                        let new_load = LirStatement::Load(LOADINS {
                                            0: load.0,
                                            1: load.1,
                                            2: RegWithOffOrImm::RegWO(RegWithOff::suggest_t0_based(HRegister::t0, 0)),
                                        });
                                        lir_block.add_stmt(load_dist);
                                        lir_block.add_stmt(move_base_pos);
                                        lir_block.add_stmt(new_load);
                                        lir_block
                                            .add_stmt_with_ann(LirStatement::Raw(RAWINS("      #load adjust".to_string())), single_ins.annotation);
                                        continue;
                                    }
                                }
                            };
                            lir_block.add_stmt_with_ann(stat, single_ins.annotation);
                        }
                        // store 调整
                        LirStatement::Store(store) => {
                            if store.2.offset < short_jump_min || store.2.offset > short_jump_max {
                                let load_dist = LirStatement::Load(LOADINS {
                                    0: WidthType::DoubleWord,
                                    1: HRegister::t0,
                                    2: RegWithOffOrImm::Imm(ImmediateBits::ImmWordI32(store.2.offset)),
                                });
                                let move_base_pos = LirStatement::Add(ADDINS {
                                    0: WidthType::DoubleWord,
                                    1: HRegister::t0,
                                    2: HRegister::t0,
                                    3: RegOrImm::Reg(store.2.base_addr_reg),
                                });
                                let new_load = LirStatement::Store(STOREINS {
                                    0: store.0,
                                    1: store.1,
                                    2: RegWithOff::suggest_t0_based(HRegister::t0, 0),
                                });
                                lir_block.add_stmt(load_dist);
                                lir_block.add_stmt(move_base_pos);
                                lir_block.add_stmt(new_load);
                                lir_block.add_stmt_with_ann(LirStatement::Raw(RAWINS("      #store adjust".to_string())), single_ins.annotation);
                                continue;
                            } else {
                                lir_block.add_stmt_with_ann(stat, single_ins.annotation);
                            }
                        }
                        // jump 调整
                        LirStatement::Jump(jump) => {
                            wait_add_jump = Some(jump.clone());
                        }
                        _ => {
                            lir_block.add_stmt_with_ann(stat, single_ins.annotation);
                        }
                    }
                } // Statement 的翻译
            }
        }
        self.blocks.push(lir_block);
    }

    pub fn now_return(&mut self, fun_name: String, is_from_main: bool, table: &mut SymbolTable) {
        if is_from_main {
            // 返回值取低位
            self.push(LirStatement::Load(LOADINS {
                0: WidthType::Word,
                1: HRegister::p1,
                2: RegWithOffOrImm::Imm(IMM::ImmWordI32(255)),
            }));

            self.push(LirStatement::And(ANDINS {
                0: HRegister::p0,
                1: HRegister::p0,
                2: ROI::Reg(HRegister::p1),
            }));
        }
        let end_sym = AnySymbol::new_rand_underscore(&format!(".SYSYC_{}_fun_ret", fun_name));
        let sym_ref = table.add_symbol(end_sym);
        let tag = self.tag_now(sym_ref);
        self.fun_ret_tags.insert(tag);
    }

    pub fn add_word_var2(&mut self, name: &AnySymbolRef, init: u32, is_const: bool, symbol_table: &mut SymbolTable) {
        /*
            .type	a,@object
            .globl	a
            .p2align	2
        a:
            .word	10
            .size	a, 4
         */
        let name = symbol_table.get_symbol(name).get_string();
        self.push(LirStatement::Raw(RAWINS(format!("    .type	{},@object", name))));
        self.push(LirStatement::Raw(RAWINS(format!("    .globl	{}", name))));
        self.push(LirStatement::Raw(RAWINS(format!("    .p2align	2"))));
        self.push(LirStatement::Raw(RAWINS(format!("{}:", name))));
        self.push(LirStatement::Raw(RAWINS(format!("    .word  {}", init))));
        self.push(LirStatement::Raw(RAWINS(format!("    .size  {},4", name))));
        let mut lir_global = LirGlobal {
            name: name.clone(),
            statements: vec![],
            is_ro: is_const,
        };
        for single_ins in self.current_ins.iter() {
            match single_ins.lir {
                LirOrTag::Tag(_) => {
                    panic!("NEVER HERE");
                    // lir_global.statements.push(LirStatement::Raw(RAWINS(format!(".L{}_{}:", name, generate_random_str(5)))))
                }
                LirOrTag::Statement(stat) => {
                    lir_global.statements.push(stat);
                }
            }
        }
        self.globals.push(lir_global);
        self.current_ins = HashInsertVec::new();
    }

    /// 同样的这里的init是补码
    pub fn add_array_riscv(&mut self, name: String, size: usize, init: Vec<(usize, u32)>, is_const: bool) {
        /*
            .type	array,@object
            .globl	array
            .p2align	2, 0x0
        array:
            .word	1
            .word	2
            .word	3
            .word	4
            # init
            .zero	364 # 9 * 4 = 36;
            .size	array, 400
                 */
        self.push(LirStatement::Raw(RAWINS(format!("    .type	{},@object", name))));
        self.push(LirStatement::Raw(RAWINS(format!("    .globl	{}", name))));
        self.push(LirStatement::Raw(RAWINS(format!("    .p2align	2, 0x0")))); // 这里的值不明确，需注意
        self.push(LirStatement::Raw(RAWINS(format!("{}:", name))));
        let mut last_pos = 0;
        for (pos, init) in &init {
            let margin = pos - last_pos;
            if margin == 0 || margin == 1 {
                self.push(LirStatement::Raw(RAWINS(format!("    .word  {}", init))));
            } else {
                self.push(LirStatement::Raw(RAWINS(format!("    .zero  {}", (margin - 1) * 4))));
                self.push(LirStatement::Raw(RAWINS(format!("    .word  {}", init))));
            }
            last_pos = *pos;
        }
        if init.len() > 0 {
            last_pos += 1;
        }
        let zero_num = size - last_pos;
        if zero_num > 0 {
            self.push(LirStatement::Raw(RAWINS(format!("    .zero  {}", zero_num * 4))));
        }
        self.push(LirStatement::Raw(RAWINS(format!("    .size  {}, {}", name, size * 4))));

        let mut lir_global = LirGlobal {
            name: name.clone(),
            statements: vec![],
            is_ro: is_const,
        };
        for single_ins in self.current_ins.iter() {
            match single_ins.lir {
                LirOrTag::Tag(_) => {
                    panic!("NEVER HERE");
                }
                LirOrTag::Statement(stat) => {
                    lir_global.statements.push(stat);
                }
            }
        }
        self.globals.push(lir_global);
        self.current_ins = HashInsertVec::new();
    }

    pub fn finish(&mut self) -> LirPartCode {
        let block_len = self.blocks.len();
        let global_len = self.globals.len();
        let globals = std::mem::replace(&mut self.globals, vec![]);
        let blocks = std::mem::replace(&mut self.blocks, vec![]);
        println!("block 数量{}", block_len);
        println!("global 数量{}", global_len);
        LirPartCode { fun_blocks: blocks, globals }
    }
}
