use crate::asm::RegisterKind;
use crate::ast::table::{AnySymbol, AnySymbolRef, SymbolTable};
use crate::lir::lir::{
    HRegister, LirStatement, RegOrImm, RegWithOff, RegWithOffOrImm, WidthType, ADDINS, BNEINS, IMM, LADDRINS, LOADINS, MOVINS, ROI, RWOOI, SLLINS,
    STOREINS, SUBINS,
};
use crate::lir::session::Session;
use crate::mir::{
    MIRArrayEleRef, MIRArrayInit, MIRArrayRef, MIRLabel, MIRLit, MIRName, MIRNamedArrayParam, MIRRefPool, MIRStmt, MIRType,
    MIRVarActiveType, MIRVarRef,
};
use crate::util::MutableRef;
use std::collections::{HashMap, HashSet, VecDeque};

/// 消极的寄存器分配方案
pub struct RegisterHelper2 {
    // 通用寄存器
    normal_registers: VecDeque<HRegister>,
    // 浮点寄存器
    float_registers: VecDeque<HRegister>,
    // 临时寄存器
    temp_regs: HashSet<HRegister>,

    /// 新实现，现阶段作为局部变量名称到地址的绑定
    local_var_array: HashMap<MIRName, RegWithOff>,
    /// 记录已经定义但是没有初始化的变量
    local_def_wait_assign: HashSet<MIRName>,
    /// 新实现，全局变量变量到地址的绑定
    global_var: HashMap<MIRName, AnySymbolRef>,
    /// 外部数组的基地址，现在阶段作为传递进来的数组
    external_array: HashMap<MIRName, RegWithOff>,
    /// 全局数组向地址的绑定
    global_array: HashMap<MIRName, AnySymbolRef>,

    /// 间接寻址寄存器，记录一个寄存器，这个寄存器记录目前被加载进寄存器的数组元素地址
    bind_elereg2addr_reg: HashMap<HRegister, HRegister>,
    /// 新实现，根据变量寻找已经绑定的寄存器
    bind_name2reg_var: HashMap<MIRName, HRegister>,
    /// 新实现，记录已经绑定，但是没有同步的变量，即寄存器当中的值和变量的实际值不一致，目前用在变量的赋值上
    bind_name2reg_wait_save: HashMap<MIRName, HRegister>,

    /// 新实现，记录已经绑定的变量的绑定次数
    bind_times_var: HashMap<MIRName, usize>,
    /// 新实现，记录数组元素向某一个地址的绑定
    bind_ele2addr: HashMap<MIRArrayEleRef, RegWithOff>,
    /// 新实现，记录数组元素加载到的寄存器
    bind_ele2reg: HashMap<MIRArrayEleRef, HRegister>,

    /// 新实现，记录活跃型临时变量到寄存器绑定
    bind_temp_var2regs: HashMap<MIRName, HRegister>,
    /// 新实现，记录非活跃临时变量到地址的绑定
    bind_temp_spill2addr: HashMap<MIRName, RegWithOff>,
    /// 记录已经加载完成，等待Drop的临时变量
    temp_wait_drop: HashMap<MIRName,HRegister>,
    /// 记录当前的活跃临时变量，用于spill的时候选出最不可能被当前使用的通用寄存器
    active_ntemps: VecDeque<MIRName>,
    /// 记录当前的活跃临时变量，用于spill的时候选出最不可能被当前使用的浮点寄存器
    active_ftemps: VecDeque<MIRName>,

    /// 当前翻译会话
    session: MutableRef<Session>,
    /// 当前栈顶所在位置，最后的结果作为栈大小的一部分
    stack_at: usize,
    /// 额外stack空间,目前主要用于多参数传递
    extra_stack_size: usize,
}

impl RegisterHelper2 {
    pub(crate) fn stack_16_init(regs: Vec<HRegister>, session: MutableRef<Session>) -> RegisterHelper2 {
        let mut helper = RegisterHelper2 {
            normal_registers: Default::default(),
            float_registers: Default::default(),
            local_def_wait_assign:Default::default(),
            local_var_array: Default::default(),
            global_var: Default::default(),
            bind_elereg2addr_reg: Default::default(),
            bind_name2reg_var: Default::default(),
            bind_name2reg_wait_save: Default::default(),
            bind_times_var: Default::default(),
            bind_ele2addr: Default::default(),
            bind_ele2reg: Default::default(),
            bind_temp_var2regs: Default::default(),
            active_ntemps: Default::default(),
            active_ftemps: Default::default(),
            bind_temp_spill2addr: Default::default(),
            session,
            stack_at: 16,
            extra_stack_size: 0,
            temp_regs: Default::default(),
            global_array: Default::default(),
            external_array: Default::default(),
            temp_wait_drop: Default::default(),
        };

        for reg in regs {
            match reg.get_reg_kind() {
                RegisterKind::Normal | RegisterKind::AnyNormal => {
                    helper.normal_registers.push_back(reg);
                }
                RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                    helper.float_registers.push_back(reg);
                }
            }
        }
        helper
    }

    pub fn print_normals(&self) {
        for nor in &self.normal_registers {
            print!("{:?} ", nor)
        }
        println!()
    }

    pub fn check_bind(&self, stmt: &MIRStmt) {
        if self.bind_name2reg_var.len() > 0 {
            panic!("局部变量的绑定没有清理干净 ,未清理数量 {} , AT {:?}", self.bind_name2reg_var.len(), stmt)
        }
        if self.bind_ele2reg.len() > 0 {
            panic!("数组元素清理不干净")
        };
        if self.temp_regs.len() > 0 {
            for temp_reg in &self.temp_regs {
                print!("{:?} ", temp_reg);
            }
            println!();
            panic!("临时变量的绑定没有清理干净 ,未清理数量 {} , AT {:?}", self.temp_regs.len(), stmt)
        }
    }

    pub fn reset(&mut self, regs: Vec<HRegister>) {
        self.stack_at = 16;
        self.normal_registers.clear();
        self.float_registers.clear();

        self.local_var_array.clear();
        self.local_def_wait_assign.clear();
        self.external_array.clear();
        self.extra_stack_size = 0;
        self.temp_regs.clear();

        self.bind_temp_spill2addr.clear();
        self.bind_temp_var2regs.clear();
        self.active_ntemps.clear();
        self.active_ftemps.clear();

        self.bind_times_var.clear();
        self.bind_ele2addr.clear();
        self.bind_ele2reg.clear();
        self.bind_elereg2addr_reg.clear();

        self.bind_name2reg_var.clear();
        self.bind_name2reg_wait_save.clear();

        self.check_bind(&MIRStmt::Label(MIRLabel::special()));
        for reg in regs {
            match reg.get_reg_kind() {
                RegisterKind::Normal | RegisterKind::AnyNormal => {
                    self.normal_registers.push_back(reg);
                }
                RegisterKind::FNormal | RegisterKind::AnyFNormal => {
                    self.float_registers.push_back(reg);
                }
            }
        }
    }

    pub fn request_reg(&mut self, need_type: &MIRType) -> HRegister {
        let reg = self.pop_reg(&need_type);
        let reg = match reg {
            None => {
                // 没有可用寄存器，spill temp
                let reg = self.spill_temp_to_get_reg(&need_type);
                reg
            }
            Some(var) => {
                // 找到了可用的寄存器
                var
            }
        };
        return reg;
    }

    // 以该方式加载的变量无法通过寄存器保存变量到内存
    pub fn load_var_new(&mut self, var: &MIRVarRef) -> HRegister {
        let var_name = var.get_name();
        if var.active_type == MIRVarActiveType::Temp {
            if self.bind_temp_var2regs.contains_key(var_name) {
                // 正在活跃，直接给出寄存器
                self.temp_wait_drop.insert(var_name.clone(),self.bind_temp_var2regs[var_name]);
                return self.bind_temp_var2regs[var_name];
            } else if self.bind_temp_spill2addr.contains_key(var_name) {
                // 不活跃,这里需要找个新寄存器，然后加载
                let addr = self.bind_temp_spill2addr[var_name].clone();
                let reg = self.request_reg(var.get_type());
                self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                    0: WidthType::Word,
                    1: reg,
                    2: RWOOI::RegWO(addr),
                }));
                self.temp_wait_drop.insert(var_name.clone(),reg);
                return reg;
            } else {
                panic!("无法找到临时变量，MIR可能存在错误的活跃变量类型 {:?}", var)
            }
        }
        let var_type = var.get_type();
        if self.bind_times_var.contains_key(var_name) {
            let time = self.bind_times_var[var_name];
            self.bind_times_var.insert(var_name.clone(), time + 1);
            return self.bind_name2reg_var[var_name];
        }

        let reg = self.request_reg(var_type);
        if self.local_var_array.contains_key(var.get_name()) {
            let address = self.local_var_array.get(var.get_name()).unwrap().clone();
            // 变量查寄存器
            self.bind_name2reg_var.insert(var.copy_name(), reg);
            // 变量查绑定次数
            self.bind_times_var.insert(var.copy_name(), 1);

            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                0: WidthType::Word,
                1: reg,
                2: RWOOI::RegWO(address),
            }));
            return reg;
        } else if self.global_var.contains_key(var.get_name()) {
            let addr_reg = self.alloc_reg(MIRType::Int);
            let global_name = self.global_var.get(var.get_name()).unwrap();
            // 变量查寄存器
            self.bind_name2reg_var.insert(var.copy_name(), reg);
            // 变量查绑定次数
            self.bind_times_var.insert(var.copy_name(), 1);

            self.session.borrow_mut().push(LirStatement::Laddr(LADDRINS {
                0: addr_reg,
                1: global_name.clone(),
            }));
            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                0: WidthType::Word,
                1: reg,
                2: RWOOI::RegWO(RegWithOff::suggest_sp_top_based(addr_reg, 0)),
            }));

            self.drop_reg(&addr_reg);
        } else {
            panic!("MIR错误,找不到变量 {:?}", var.get_name())
        }
        reg
    }

    pub fn load_var_for_save(&mut self, var: &MIRVarRef) -> HRegister {
        if self.local_def_wait_assign.contains(var.get_name()) {
            self.local_def_wait_assign.remove(var.get_name());
            let var_type = var.get_type();
            let reg = self.request_reg(var_type);
            self.bind_name2reg_wait_save.insert(var.copy_name(), reg);
            return reg
        }
        if var.active_type == MIRVarActiveType::Temp {
            panic!("TEMP VAR {:?} ASSIGN TWICE , MIR ERROR", var)
        }
        let var_type = var.get_type();
        let reg = self.request_reg(var_type);
        self.bind_name2reg_wait_save.insert(var.copy_name(), reg);
        reg
    }

    pub fn save_var_to_addr(&mut self, var: &MIRVarRef, is_def_save: bool) {
        if var.active_type == MIRVarActiveType::Temp && !is_def_save {
            panic!("TEMP VAR {:?} ASSIGN TWICE , MIR ERROR", var)
        }
        // 后期修改，如果是def_save 并且是临时变量，这里的逻辑需要改
        let var_name = var.get_name();

        if var.active_type == MIRVarActiveType::Temp {
            // 这里必然是,def_save,
            if self.bind_name2reg_wait_save.contains_key(var_name) {
                self.bind_name2reg_wait_save.remove(var_name).unwrap();
                // 这里什么也做,临时变量，等会加载完之后，等待消除即可
                return;
            } else {
                panic!("NOT FOUND TEMP WAIT SAVE VAR {:?} !", var);
            }
        }

        if self.bind_name2reg_wait_save.contains_key(var_name) {
            if self.bind_times_var.contains_key(var_name) {
                panic!("变量存在引用，无法保存")
            }
            // 等待赋值的变量
            let reg = self.bind_name2reg_wait_save.remove(var_name).unwrap();

            if self.local_var_array.contains_key(&var_name) {
                let addr = self.local_var_array[var_name].clone();
                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                    0: WidthType::Word,
                    1: reg.clone(),
                    2: addr,
                }));
            } else if self.global_var.contains_key(&var_name) {
                let addr_reg = self.alloc_reg(MIRType::Int);
                let global_name = self.global_var.get(var_name).unwrap();

                self.session.borrow_mut().push(LirStatement::Laddr(LADDRINS {
                    0: addr_reg,
                    1: global_name.clone(),
                }));

                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                    0: WidthType::Word,
                    1: reg.clone(),
                    2: RegWithOff::suggest_sp_top_based(addr_reg, 0),
                }));

                self.drop_reg(&addr_reg);
            } else {
                panic!("{:?} VAR EXIST BUT , CAN FIND ARR TO SAVE", var)
            }

            let kind = reg.get_reg_kind();
            match kind {
                RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(reg.clone()),
                RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(reg.clone()),
            }
        } else {
            panic!("NOT FOUND WAIT SAVE VAR {:?} !", var);
        }
    }

    pub fn save_array_ele_to_addr(&mut self, ele: &MIRArrayEleRef) {
        let addr = self.bind_ele2addr.remove(ele).unwrap();
        let ele_reg = self.bind_ele2reg.remove(ele).unwrap();
        let addr_reg = self.bind_elereg2addr_reg.remove(&ele_reg).unwrap();
        self.normal_registers.push_front(addr_reg);
        let kind = ele_reg.get_reg_kind();
        match kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(ele_reg.clone()),
            RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(ele_reg.clone()),
        }
        self.session.borrow_mut().push(LirStatement::Store(STOREINS {
            0: WidthType::Word,
            1: ele_reg.clone(),
            2: addr,
        }));
    }

    /// 新版本的drop_array_ele 由专门的函数来完成
    pub fn drop_var_one_time(&mut self, var: &MIRVarRef) {
        if var.active_type == MIRVarActiveType::Temp {
            let var_name = var.get_name();
            if self.temp_wait_drop.contains_key(var_name) {
                let removed_reg = self.temp_wait_drop.remove(var_name).unwrap();
                if self.bind_temp_var2regs.contains_key(var_name) {
                    // 活跃型消除
                    let _reg = self.bind_temp_var2regs.remove(var_name).unwrap();
                    let mut is_deal = false;
                    match var.get_type() {
                        MIRType::Int | MIRType::Bool => {
                            for (index, name) in self.active_ntemps.iter().enumerate() {
                                if name == var_name {
                                    self.active_ntemps.remove(index);
                                    is_deal = true;
                                    break;
                                }
                            }
                        }
                        MIRType::Float => {
                            for (index, name) in self.active_ftemps.iter().enumerate() {
                                if name == var_name {
                                    self.active_ftemps.remove(index);
                                    is_deal = true;
                                    break;
                                }
                            }
                        }
                        MIRType::Void => {
                            panic!("NEVER HERE")
                        }
                    }
                    if !is_deal {
                        panic!("NEVER HERE")
                    }
                } else if self.bind_temp_spill2addr.contains_key(var_name) {
                    //非活跃型
                    let _addr = self.bind_temp_spill2addr.remove(var_name).unwrap();
                }else {
                    panic!("NEVER HERE")
                }
                let kind = removed_reg.get_reg_kind();
                match kind {
                    RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(removed_reg.clone()),
                    RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(removed_reg.clone()),
                }
                return;
            } else {
                panic!("临时变量被意外Drop,或者临时变量被多次Drop {:?}", var);
            }
        }

        let var_name = var.get_name();

        // 清理引用次数
        if self.bind_times_var[var_name] != 1 {
            let num = self.bind_times_var[var_name];
            self.bind_times_var.insert(var_name.clone(), num - 1);
            return;
        } else {
            self.bind_times_var.remove(var_name);
        }
        // 清理变量到寄存器的绑定
        let removed_reg = self.bind_name2reg_var.remove(&var_name);
        let reg = removed_reg.unwrap();
        let kind = reg.get_reg_kind();
        match kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(reg.clone()),
            RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(reg.clone()),
        }
    }

    pub fn alloc_global_var(&mut self, var: MIRVarRef, lit: MIRLit, ref_table: &mut MIRRefPool, symbol_table: &mut SymbolTable) {
        let sym_ref = ref_table.get_sym_ref(var.get_name());
        let ret = self.global_var.insert(var.copy_name(), sym_ref.clone());

        if ret.is_some() {
            panic!("NEVER HERE")
        }
        self.session
            .borrow_mut()
            .add_word_var2(sym_ref, lit.into_ieee754_or_complement(), false, symbol_table);
        return;
    }

    /// 不对寄存器做任何处理
    pub fn alloc_var_with_specified_reg(&mut self, var: &MIRVarRef, reg: HRegister) {
        self.stack_at += var.get_width_bytes();
        let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
        self.local_var_array.insert(var.copy_name(), address.clone());
        self.session.borrow_mut().push(LirStatement::Store(STOREINS {
            0: WidthType::Word,
            1: reg,
            2: address,
        }))
    }

    /// 不对寄存器做处理
    pub fn add_var_with_specified_addr(&mut self, var: &MIRVarRef, address: RegWithOff) {
        self.local_var_array.insert(var.copy_name(), address.clone());
    }

    /// 标记变量已经声明，还没有定义
    pub fn add_var_wait_assign(&mut self,var:&MIRVarRef){
        self.local_def_wait_assign.insert(var.copy_name());
        self.stack_at += var.get_width_bytes();
        let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
        self.local_var_array.insert(var.copy_name(), address.clone());
    }

    /// 以该方式申请的变量无法通过寄存器drop或者save
    pub fn alloc_var_new(&mut self, var: &MIRVarRef) -> HRegister {
        if var.active_type == MIRVarActiveType::Temp {
            let reg = self.request_reg(var.get_type());
            match var.get_type() {
                MIRType::Int | MIRType::Bool => {
                    self.active_ntemps.push_back(var.copy_name());
                }
                MIRType::Float => {
                    self.active_ftemps.push_back(var.copy_name());
                }
                MIRType::Void => {
                    panic!("NEVER HERE")
                }
            }
            self.bind_name2reg_wait_save.insert(var.copy_name(), reg);
            self.bind_temp_var2regs.insert(var.copy_name(), reg);
            return reg;
        }

        let reg = self.request_reg(var.get_type());
        self.stack_at += var.get_width_bytes();
        let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
        self.local_var_array.insert(var.copy_name(), address.clone());
        self.bind_name2reg_wait_save.insert(var.copy_name(), reg);
        // self.bind_name2reg_var.insert(var.copy_name(), reg);
        // self.bind_times.insert(reg, 1);
        reg
    }

    /// need_load = true
    ///         need_bind = true array ele bind reg
    ///         need_bind = false          temp reg
    /// need_load = false
    ///         need_bind 无效,不允许是true ，返回  array ele bind reg
    pub fn load_array_ele_new(&mut self, ele: &MIRArrayEleRef, need_bind: bool, need_load: bool) -> HRegister {
        let all_offset = self.request_reg(&MIRType::Int);
        if self.local_var_array.contains_key(ele.get_name()) {
            let stack_offset_stack = self.local_var_array[ele.get_name()].clone();
            self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: stack_offset_stack.base_addr_reg,
                3: RegOrImm::Imm(IMM::ImmWordI32(stack_offset_stack.offset)), // 拓宽了局部数组寻址上限
            })); // 加载基址
        } else if self.global_array.contains_key(ele.get_name()) {
            self.session.borrow_mut().push(LirStatement::Laddr(LADDRINS {
                0: all_offset,
                1: self.global_array.get(ele.get_name()).unwrap().clone(),
            }));
        } else if self.external_array.contains_key(ele.get_name()) {
            let addr = self.external_array[ele.get_name()].clone();
            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: RegWithOffOrImm::RegWO(addr),
            })); // 加载基址
        } else {
            panic!("NEVER HERE")
        }

        let offset = self.load_var_new(ele.get_offset());
        self.session.borrow_mut().push(LirStatement::Sll(SLLINS {
            0: offset.clone(),
            1: offset.clone(),
            2: ROI::Imm(IMM::ImmI12(2)), // todo fix length
        })); // 获得偏移地址

        self.session.borrow_mut().push(LirStatement::Add(ADDINS {
            0: WidthType::DoubleWord,
            1: all_offset,
            2: all_offset,
            3: RegOrImm::Reg(offset),
        }));

        self.drop_var_one_time(&ele.get_offset());

        if need_load {
            // 注意all_offset 寄存器的身份
            let ret_reg = if !need_bind {
                self.alloc_reg(ele.get_type())
            } else {
                self.request_reg(&MIRType::Int)
            };
            let addr = RegWithOff::suggest_fp_bottom_based(all_offset, 0);
            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                0: WidthType::Word,
                1: ret_reg,
                2: RWOOI::RegWO(addr.clone()),
            })); // 加载变量

            if need_bind {
                self.bind_ele2addr.insert(ele.clone(), addr);
                self.bind_ele2reg.insert(ele.clone(), ret_reg);
                self.bind_elereg2addr_reg.insert(ret_reg, all_offset);
            } else {
                self.normal_registers.push_front(all_offset);
            }
            return ret_reg;
        } else {
            if !need_bind {
                panic!("NEVER HERE")
            }
            // 不需要加载,用于给数组元素更新
            // 无需加载时 bind 字段无效
            let reg = match ele.get_type() {
                MIRType::Int => self.normal_registers.pop_front(),
                MIRType::Float => self.float_registers.pop_front(),
                MIRType::Bool => self.normal_registers.pop_front(),
                MIRType::Void => {
                    panic!("NEVER HERE")
                }
            };
            if reg.is_none() {
                // 这里的逻辑，以后都是要改的，涉及到spill temp寄存器
                panic!("ERROR , NO AVAILABLE REG")
            }
            let ret_reg = reg.unwrap();
            let addr = RegWithOff::suggest_fp_bottom_based(all_offset, 0);
            self.bind_ele2addr.insert(ele.clone(), addr);
            self.bind_ele2reg.insert(ele.clone(), ret_reg);
            self.bind_elereg2addr_reg.insert(ret_reg, all_offset);
            return ret_reg;
        }
    }

    pub fn load_array_addr(&mut self, array_addr: MIRNamedArrayParam, reg: &HRegister) {
        let all_offset = self.request_reg(&MIRType::Int);
        if self.local_var_array.contains_key(array_addr.get_name()) {
            /*
            call	read_int
            mv	a1, a0
            ld	a0, -72(s0)
            slli	a2, a1, 2
            addi	a1, fp, -64
            add	    a1, a1, a2
            sw	a0, 0(a1)
            */

            let stack_offset_stack = self.local_var_array[array_addr.get_name()].clone();
            self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: stack_offset_stack.base_addr_reg,
                3: RegOrImm::Imm(IMM::ImmWordI32(stack_offset_stack.offset)),
            })); // 加载基址

            let offset = self.load_var_new(array_addr.get_offset());
            self.session.borrow_mut().push(LirStatement::Sll(SLLINS {
                0: offset.clone(),
                1: offset.clone(),
                2: ROI::Imm(IMM::ImmI12(2)), // todo fix length
            })); // 获得偏移地址

            self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: all_offset,
                3: RegOrImm::Reg(offset),
            }));
            self.drop_var_one_time(&array_addr.get_offset());
        } else if self.global_array.contains_key(array_addr.get_name()) {
            self.session.borrow_mut().push(LirStatement::Laddr(LADDRINS {
                0: all_offset,
                1: self.global_array.get(array_addr.get_name()).unwrap().clone(),
            }));
            let offset = self.load_var_new(array_addr.get_offset());
            self.session.borrow_mut().push(LirStatement::Sll(SLLINS {
                0: offset.clone(),
                1: offset.clone(),
                2: ROI::Imm(IMM::ImmI12(2)), // todo fix length
            })); // 获得偏移地址
            self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: all_offset,
                3: RegOrImm::Reg(offset),
            }));
            self.drop_var_one_time(&array_addr.get_offset());
        } else if self.external_array.contains_key(array_addr.get_name()) {
            let addr = self.external_array[array_addr.get_name()].clone();
            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                0: WidthType::DoubleWord,
                1: all_offset,
                2: RegWithOffOrImm::RegWO(addr),
            }));
        } else {
            panic!("NEVER HERE")
        }
        self.normal_registers.push_front(all_offset);
        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: *reg, 1: all_offset }))
    }

    /// 编译时已知的偏移量
    pub fn save_reg_to_indexed_local_array_ele(&mut self, array: &MIRArrayRef, index: usize, from_reg: HRegister) {
        let mut dest = self.local_var_array[array.get_name()].clone();
        dest.offset += index as i32 * array.get_type().get_width() as i32;
        let store_ins = LirStatement::Store(STOREINS {
            0: WidthType::Word, // fixme fixed type
            1: from_reg,
            2: dest,
        });
        self.session.borrow_mut().push(store_ins)
    }

    pub fn add_array_with_addr(&mut self, array: &MIRArrayRef, addr: RegWithOff) {
        self.external_array.insert(array.copy_name(), addr);
    }

    // 不对寄存器做任何处理
    pub fn add_array_with_addr_reg(&mut self, array: &MIRArrayRef, reg: HRegister) {
        self.stack_at += 8; // 预留储存地址
        self.session.borrow_mut().push(LirStatement::Store(STOREINS {
            0: WidthType::DoubleWord,
            1: reg,
            2: RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32),
        }));
        self.external_array.insert(
            array.copy_name(),
            RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32),
        );
    }

    pub fn alloc_array(&mut self, array: &MIRArrayRef, symbol_table: &mut SymbolTable) {
        let type_size = array.get_type().get_width();
        let all_size = type_size * array.ele_num;
        // let start_pos = self.stack_at + type_size; //  - 4 高地址
        let last_pos = self.stack_at + all_size; // - 4 * size 低地址

        let addr_reg = self.alloc_reg(MIRType::Int);
        let times_reg = self.alloc_reg(MIRType::Int);
        self.session.borrow_mut().push(LirStatement::Load(LOADINS {
            0: WidthType::Word,
            1: times_reg,
            2: RWOOI::Imm(IMM::ImmWordI32(array.ele_num as i32)),
        }));
        self.session.borrow_mut().push(LirStatement::Sub(SUBINS {
            0: WidthType::DoubleWord,
            1: addr_reg,
            2: HRegister::fp,
            3: RegOrImm::Imm(IMM::ImmWordU32(last_pos as u32)),
        }));

        let tag_symbol = symbol_table.add_symbol(AnySymbol::new_rand_underscore(".Larray_init_"));
        let tag = self.session.borrow_mut().tag_now(tag_symbol);
        self.session.borrow_mut().push(LirStatement::Store(STOREINS {
            0: WidthType::Word,
            1: HRegister::zero,
            2: RegWithOff::suggest_sp_top_based(addr_reg, 0),
        }));
        self.session.borrow_mut().push(LirStatement::Add(ADDINS {
            0: WidthType::DoubleWord,
            1: addr_reg,
            2: addr_reg,
            3: RegOrImm::Imm(IMM::ImmI12(4)),
        }));
        self.session.borrow_mut().push(LirStatement::Sub(SUBINS {
            0: WidthType::Word,
            1: times_reg,
            2: times_reg,
            3: RegOrImm::Imm(IMM::ImmI12(1)),
        }));
        self.session.borrow_mut().push(LirStatement::Bne(BNEINS {
            0: times_reg,
            1: HRegister::zero,
            2: tag.get_label_tag(),
        }));
        self.drop_reg(&times_reg);
        self.drop_reg(&addr_reg);
        self.local_var_array
            .insert(array.copy_name(), RegWithOff::suggest_fp_bottom_based(HRegister::fp, last_pos as i32));
        self.stack_at += all_size;
    }

    pub fn alloc_global_array(&mut self, array: &MIRArrayRef, init: MIRArrayInit, is_const: bool, ref_pool: &MIRRefPool, sym_table: &SymbolTable) {
        let sym_ref = ref_pool.get_sym_ref(array.get_name());
        self.global_array.insert(array.copy_name(), sym_ref.clone());
        let name = sym_table.get_symbol(sym_ref);
        let mut u32_init = vec![];
        for (index, init) in init.inits {
            let init = init.into_ieee754_or_complement();
            u32_init.push((index, init))
        }
        self.session
            .borrow_mut()
            .add_array_riscv(name.get_string(), array.ele_num, u32_init, is_const);
    }

    /// 申请临时寄存器
    pub fn alloc_reg(&mut self, reg_type: MIRType) -> HRegister {
        let reg = match reg_type {
            MIRType::Bool | MIRType::Int => self.normal_registers.pop_front().unwrap(),
            MIRType::Float => self.float_registers.pop_front().unwrap(),
            MIRType::Void => {
                panic!("NEVER HERE")
            }
        };
        self.temp_regs.insert(reg);
        reg
    }

    /// 回收临时寄存器
    pub fn drop_reg(&mut self, reg: &HRegister) {
        let ret = self.temp_regs.remove(reg);
        if ret == false {
            panic!("unknown HRegister was withdrew")
        }
        let kind = reg.get_reg_kind();
        match kind {
            RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(reg.clone()),
            RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(reg.clone()),
        }
    }

    pub fn deprived_free(&mut self, reg: HRegister) {
        if self.temp_regs.contains(&reg) {
            panic!("register {:?} is temp reg", reg);
        }

        if self.normal_registers.contains(&reg) {
            for (now_index, now_reg) in self.normal_registers.iter().enumerate() {
                if &reg == now_reg {
                    self.normal_registers.remove(now_index);
                    break;
                }
            }
            return;
        }
        if self.float_registers.contains(&reg) {
            if self.float_registers.contains(&reg) {
                for (now_index, now_reg) in self.float_registers.iter().enumerate() {
                    if &reg == now_reg {
                        self.float_registers.remove(now_index);
                        break;
                    }
                }
                return;
            }
        }
        self.print_normals();
        panic!("reg {:?} not found", reg)
    }

    // 归还deprived寄存器
    pub fn grant_free(&mut self, reg: HRegister) {
        match reg.get_reg_kind() {
            RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_back(reg),
            RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_back(reg),
        }
    }

    pub fn get_stack_size(&self) -> usize {
        self.stack_at + self.extra_stack_size
    }

    pub fn add_extra_stack_size(&mut self, len: usize) {
        self.extra_stack_size += len;
    }

    pub fn spill_all_active_temp(&mut self){
        loop {
            let ntemps = self.active_ntemps.pop_front();
            match ntemps {
                None => {
                    break;
                }
                Some(var) => {
                    self.stack_at += 4;
                    let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
                    let active_reg = self.bind_temp_var2regs.remove(&var).unwrap(); // 去掉活跃
                    self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                        0: WidthType::Word, // fixme 宽度被固定
                        1: active_reg,
                        2: address.clone(),
                    }));
                    self.bind_temp_spill2addr.insert(var, address); // 加入非活跃
                    let kind = active_reg.get_reg_kind();
                    match kind {
                        RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(active_reg.clone()),
                        RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(active_reg.clone()),
                    }
                }
            }
        }
        loop {
            let ntemps = self.active_ftemps.pop_front();
            match ntemps {
                None => {
                    break;
                }
                Some(var) => {
                    self.stack_at += 4;
                    let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
                    let active_reg = self.bind_temp_var2regs.remove(&var).unwrap(); // 去掉活跃
                    self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                        0: WidthType::Word, // fixme 宽度被固定
                        1: active_reg,
                        2: address.clone(),
                    }));
                    self.bind_temp_spill2addr.insert(var, address); // 加入非活跃
                    let kind = active_reg.get_reg_kind();
                    match kind {
                        RegisterKind::Normal | RegisterKind::AnyNormal => self.normal_registers.push_front(active_reg.clone()),
                        RegisterKind::FNormal | RegisterKind::AnyFNormal => self.float_registers.push_front(active_reg.clone()),
                    }
                }
            }
        }
    }

    fn pop_reg(&mut self, var_type: &MIRType) -> Option<HRegister> {
        let reg = match var_type {
            MIRType::Int => self.normal_registers.pop_front(),
            MIRType::Float => self.float_registers.pop_front(),
            MIRType::Bool => self.normal_registers.pop_front(),
            MIRType::Void => {
                panic!("NEVER HERE")
            }
        };
        return reg;
    }

    fn spill_temp_to_get_reg(&mut self, var_type: &MIRType) -> HRegister {
        self.stack_at += 4;
        let address = RegWithOff::suggest_fp_bottom_based(HRegister::fp, self.stack_at as i32);
        let var = match var_type {
            MIRType::Int | MIRType::Bool => self.active_ntemps.pop_back(),
            MIRType::Float => self.active_ftemps.pop_back(),
            MIRType::Void => {
                panic!("NEVER HERE")
            }
        };
        match var {
            None => {
                panic!("无法spill,找不到合适的寄存器")
            }
            Some(var) => {
                let active_reg = self.bind_temp_var2regs.remove(&var).unwrap(); // 去掉活跃
                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                    0: WidthType::Word, // fixme 宽度被固定
                    1: active_reg,
                    2: address.clone(),
                }));
                self.bind_temp_spill2addr.insert(var, address); // 加入非活跃
                return active_reg;
            }
        }
    }

}

// 目前存在问题，临时变量需要在调用前保存
// WHILE 的条件变量不能作为临时变量
