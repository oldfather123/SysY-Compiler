use super::{
    antlr_dep::sysyparser::*,
    antlr_dep::sysyvisitor::SysYVisitorCompat,
    return_content::{AstReturnContent, ValueMode},
    symbol_table::*,
};
use crate::common::{error::SemanticError, immediate::Immediate, r#type::Type};
use crate::frontend::{
    llvm::{
        function::{ArgumentList, Function},
        instr::*,
        llvm_module::*,
        ssa::*,
    },
    return_content::AstExp,
};
use antlr_rust::{
    parser_rule_context::ParserRuleContext,
    token::Token,
    tree::{ParseTree, ParseTreeVisitorCompat, Tree, Visitable},
};
use defaults::Defaults;
use derive_new::new;
use getset::{Getters, MutGetters};
use hexf_parse::parse_hexf32;
use std::str::FromStr;

#[derive(Defaults, Debug)]
pub struct AstNodeResult(#[def = "Ok(AstReturnContent::default())"] eyre::Result<AstReturnContent>);

impl AstNodeResult {
    pub fn is_not_empty(&self) -> bool {
        self.0.is_ok()
    }
}
/// use into to convert SysYAstReturn to SysYAstNodeResult
impl From<AstReturnContent> for AstNodeResult {
    fn from(ok: AstReturnContent) -> Self {
        Self(Ok(ok))
    }
}

impl From<SemanticError> for AstNodeResult {
    fn from(err: SemanticError) -> Self {
        Self(Err(eyre!(err)))
    }
}

#[derive(Debug, Getters, new, MutGetters)]
pub struct SysYAstVisitor<'a> {
    #[new(default)]
    node_result: AstNodeResult,
    #[new(default)]
    value_mode: ValueMode,
    #[new(value = "String::from(\"_init\")")]
    cur_func_name: String,
    module: &'a mut LLVMModule,
    #[new(value = "VariableTable::new(None)")]
    global_vtable: VariableTable,
    #[new(value = "Some(Box::new(VariableTable::new(None)))")]
    cur_vtable: Option<Box<VariableTable>>,
    #[new(value = "None")]
    cur_bb: Option<i32>,
    #[new(value = "Type::Void")]
    cur_type: Type,
    #[new(value = "Type::Int")]
    cur_num_type: Type,
    #[new(default)]
    ret_value_opt: Option<SSALeftValue>,
    #[new(default)]
    ret_bb_opt: Option<i32>,
    #[new(value = "false")]
    has_return: bool,
    #[new(value = "0")]
    depth: i32,
    #[new(default)]
    true_bb_stack: Vec<i32>,
    #[new(default)]
    false_bb_stack: Vec<i32>,
    #[new(default)]
    break_target_bb: Vec<i32>,
    #[new(default)]
    continue_target_bb: Vec<i32>,
}

impl SysYAstVisitor<'_> {
    pub fn return_content(&self) -> AstReturnContent {
        self.node_result.0.as_ref().expect("semantic error").clone()
    }

    fn dfs_const_init(
        &mut self,
        ctx: &ListConstInitValContext,
        shape: &mut Vec<i32>,
        ty: Type,
        result: &mut Vec<Immediate>,
    ) {
        if shape.len() == 0 {
            panic!("Invalid Init List");
        }
        let mut total_size = 1;
        let mut child_size = 1;
        for i in 0..shape.len() {
            total_size *= shape[i];
            if i > 0 {
                child_size *= shape[i];
            }
        }
        if total_size == 0 {
            return;
        }
        result.reserve(result.len() + total_size as usize);
        let mut child_shape = shape[1..].to_vec();
        let mut cnt = 0;
        for child in ctx.constInitVal_all() {
            match child.as_ref() {
                ConstInitValContextAll::ScalarConstInitValContext(scalar_child) => {
                    if cnt + 1 > total_size {
                        panic!("Invalid Init List");
                    }
                    scalar_child.constExp().unwrap().accept(self);
                    let return_content = self.return_content();
                    result.push(match return_content.into_exp().unwrap() {
                        AstExp::StaticValue(immediate) => {
                            Self::convert_imme2needed_type(immediate, ty)
                        }
                        _ => panic!("Invalid Init List"),
                    });
                    cnt += 1;
                }
                ConstInitValContextAll::ListConstInitValContext(list_child) => {
                    if cnt % child_size != 0 || cnt + child_size > total_size {
                        panic!("Invalid Init List");
                    }
                    self.dfs_const_init(list_child, &mut child_shape, ty, result);
                    cnt += child_size;
                }
                ConstInitValContextAll::Error(_) => {}
            }
        }
        while cnt < total_size {
            if ty == Type::Float {
                result.push(Immediate::Float(0.0));
            } else {
                result.push(Immediate::Int(0));
            }
            cnt += 1;
        }
    }

    #[inline(always)]
    fn convert_imme2needed_type(imme: Immediate, ty: Type) -> Immediate {
        assert!(imme.get_type() != Type::Void);
        assert!(ty != Type::Void);
        if imme.get_type() == ty {
            imme
        } else if imme.get_type() == Type::Int && ty == Type::Float {
            Immediate::Float(imme.into_int().unwrap() as f32)
        } else if imme.get_type() == Type::Float && ty == Type::Int {
            Immediate::Int(imme.into_float().unwrap() as i32)
        } else {
            panic!("Invalid Type Conversion")
        }
    }

    fn parse_const_init(
        &mut self,
        ctx: &ConstInitValContextAll,
        shape: &mut Vec<i32>,
        ty: Type,
    ) -> Vec<Immediate> {
        let mut result = Vec::new();
        match ctx {
            ConstInitValContextAll::ScalarConstInitValContext(ctx) => {
                if shape.len() != 0 {
                    panic!("Invalid Init List");
                }
                ctx.constExp().unwrap().accept(self);
                let return_content = self.return_content();
                result.push(match return_content.into_exp().unwrap() {
                    AstExp::StaticValue(immediate) => Self::convert_imme2needed_type(immediate, ty),
                    _ => panic!("Invalid Init List"),
                });
            }
            ConstInitValContextAll::ListConstInitValContext(ctx) => {
                self.dfs_const_init(ctx, shape, ty, &mut result);
            }
            ConstInitValContextAll::Error(_) => {
                panic!("Invalid Init List");
            }
        }

        result
    }

    fn dfs_var_init(
        &mut self,
        ctx: &ListInitvalContext,
        shape: &mut Vec<i32>,
        result: &mut Vec<SSARightValue>,
    ) {
        if shape.len() == 0 {
            panic!("Invalid Init List");
        }
        let total_size: i32 = shape.iter().product();
        let child_size: i32 = shape[1..].iter().product();
        if total_size == 0 {
            return;
        }
        result.reserve(result.len() + total_size as usize);
        let mut child_shape = shape[1..].to_vec();
        let mut cnt = 0;
        for child in ctx.initVal_all() {
            match child.as_ref() {
                InitValContextAll::ScalarInitValContext(scalar_child) => {
                    if cnt + 1 > total_size {
                        panic!("Invalid Init List");
                    }
                    scalar_child.exp().unwrap().accept(self);
                    let return_content = self.return_content();
                    if self.value_mode == ValueMode::Const {
                        let value = return_content
                            .into_exp()
                            .unwrap()
                            .into_static_value()
                            .unwrap();
                        result.push(SSARightValue::new_imme(value));
                    } else {
                        let ssa_res = return_content.into_exp().unwrap().into_ssa_value().unwrap();
                        result.push(ssa_res);
                    }
                    cnt += 1;
                }
                InitValContextAll::ListInitvalContext(list_child) => {
                    if cnt + child_size > total_size {
                        panic!("Invalid Init List");
                    }
                    let cur_ceil =
                        (result.len() / (child_size as usize) + 1) * (child_size as usize);
                    let need = cur_ceil - result.len();
                    let mut child_result = Vec::new();
                    self.dfs_var_init(list_child, &mut child_shape, &mut child_result);
                    child_result.truncate(need);
                    result.append(&mut child_result);
                    cnt += need as i32;
                }
                InitValContextAll::Error(_) => {}
            }
        }
        let zero_value = SSARightValue::new_imme(Immediate::Int(0));
        while cnt < total_size {
            result.push(zero_value.clone());
            cnt += 1;
        }
    }

    fn parse_var_init(
        &mut self,
        ctx: &InitValContextAll,
        shape: &mut Vec<i32>,
    ) -> Vec<SSARightValue> {
        let mut result = Vec::new();
        match ctx {
            InitValContextAll::ScalarInitValContext(ctx) => {
                if shape.len() != 0 {
                    panic!("Invalid Init List");
                }
                ctx.exp().unwrap().accept(self);
                let return_content = self.return_content();
                if self.value_mode == ValueMode::Const {
                    let value = match return_content.into_exp().unwrap() {
                        AstExp::StaticValue(immediate) => immediate,
                        _ => panic!("Invalid Scalar Type, need immediate"),
                    };
                    let ssa_res = SSARightValue::new_imme(value);
                    result.push(ssa_res);
                } else {
                    let ssa_res = match return_content {
                        AstReturnContent::Exp(AstExp::SSAValue(ssa_res)) => ssa_res,
                        _ => panic!("Invalid Scalar Type, need ssa value"),
                    };
                    result.push(ssa_res);
                }
            }
            InitValContextAll::ListInitvalContext(ctx) => {
                self.dfs_var_init(ctx, shape, &mut result);
            }
            InitValContextAll::Error(_) => {
                panic!("Invalid Init List");
            }
        }
        result
    }

    fn is_init_value_all_zero(init_values: &Vec<SSARightValue>) -> bool {
        for init_value in init_values {
            match init_value.inner() {
                SSARightValueInner::Immediate(imme) => {
                    if !imme.is_zero() {
                        return false;
                    }
                }
                _ => return false,
            }
        }
        true
    }

    fn generate_lvalue_init_ir(
        &mut self,
        bb_id: i32,
        lvalue: SSALeftValue,
        rvalue_vec: Vec<SSARightValue>,
        ir_vec: &mut Vec<Instruction>,
    ) {
        let shape = lvalue.get_shape();
        let cur_func = self
            .module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap();
        if shape.is_empty() {
            if rvalue_vec.len() == 1 {
                if lvalue.get_type() == Type::Int && rvalue_vec[0].get_type() == Type::Float {
                    let new_rvalue = SSARightValue::new_reg(cur_func.alloc_ssa_id(), Type::Int);
                    ir_vec.push(Instruction::new(
                        Box::new(Fptosi::new(new_rvalue.clone(), rvalue_vec[0].clone())),
                        bb_id,
                    ));
                    ir_vec.push(Instruction::new(
                        Box::new(Store::new(lvalue.to_address(), new_rvalue)),
                        bb_id,
                    ));
                } else if lvalue.get_type() == Type::Float && rvalue_vec[0].get_type() == Type::Int
                {
                    let new_rvalue = SSARightValue::new_reg(cur_func.alloc_ssa_id(), Type::Float);
                    ir_vec.push(Instruction::new(
                        Box::new(Sitofp::new(new_rvalue.clone(), rvalue_vec[0].clone())),
                        bb_id,
                    ));
                    ir_vec.push(Instruction::new(
                        Box::new(Store::new(lvalue.to_address(), new_rvalue)),
                        bb_id,
                    ));
                } else {
                    ir_vec.push(Instruction::new(
                        Box::new(Store::new(lvalue.to_address(), rvalue_vec[0].clone())),
                        bb_id,
                    ));
                }
            } else {
                panic!("Invalid Init List");
            }
        } else {
            let new_shape = shape[1..].to_vec();
            let mut child_size = 1;
            for i in 0..new_shape.len() {
                child_size *= new_shape[i];
            }
            for index in 0..shape[0] {
                let new_lvalue = SSALeftValue::new_addr(
                    self.cur_function().alloc_ssa_id(),
                    lvalue.get_type(),
                    new_shape.to_vec(),
                );
                ir_vec.push(Instruction::new(
                    Box::new(Gep::new(
                        lvalue.to_address(),
                        new_lvalue.to_address(),
                        SSARightValue::new_imme(Immediate::Int(index as i32)),
                    )),
                    bb_id,
                ));
                let start = (index * child_size) as usize;
                let end = ((index + 1) * child_size) as usize;

                let sub_rvalue_vec = rvalue_vec[start..end].to_vec();
                self.generate_lvalue_init_ir(bb_id, new_lvalue, sub_rvalue_vec, ir_vec);
            }
        }
    }

    fn generate_lvalue_init_ir_wrap(
        &mut self,
        bb_id: i32,
        lvalue: SSALeftValue,
        rvalue_vec: Vec<SSARightValue>,
        ir_vec: &mut Vec<Instruction>,
    ) {
        let shape = lvalue.get_shape();
        if shape.len() > 0 && Self::is_init_value_all_zero(&rvalue_vec) {
            let total_len: i32 = shape.iter().product();
            let start = SSARightValue::new_imme(Immediate::Int(0));
            let end = SSARightValue::new_imme(Immediate::Int(total_len));
            let addr = lvalue.to_address();
            ir_vec.push(Instruction::new(
                Box::new(Call::new(
                    None,
                    "__sysy_homemade_mem_zero_init".to_string(),
                    vec![addr, start, end],
                )),
                bb_id,
            ));
        } else {
            self.generate_lvalue_init_ir(bb_id, lvalue, rvalue_vec, ir_vec);
        }
    }

    fn generate_lvalue_zero_init_ir(
        &mut self,
        bb_id: i32,
        lvalue: SSALeftValue,
        ir_vec: &mut Vec<Instruction>,
    ) {
        let shape = lvalue.get_shape();
        if shape.is_empty() {
            // scalar
            if lvalue.get_type() == Type::Int {
                let new_rvalue = SSARightValue::new_imme(Immediate::Int(0));
                ir_vec.push(Instruction::new(
                    Box::new(Store::new(lvalue.to_address(), new_rvalue)),
                    bb_id,
                ));
            } else if lvalue.get_type() == Type::Float {
                let new_rvalue = SSARightValue::new_imme(Immediate::Float(0.0));
                ir_vec.push(Instruction::new(
                    Box::new(Store::new(lvalue.to_address(), new_rvalue)),
                    bb_id,
                ));
            } else {
                panic!("Invalid lvalue type");
            }
        } else {
            // array
            let new_shape = shape[1..].to_vec();
            for index in 0..shape[0] {
                let new_lvalue = SSALeftValue::new_addr(
                    self.cur_function().alloc_ssa_id(),
                    lvalue.get_type(),
                    new_shape.to_vec(),
                );
                ir_vec.push(Instruction::new(
                    Box::new(Gep::new(
                        lvalue.to_address(),
                        new_lvalue.to_address(),
                        SSARightValue::new_imme(Immediate::Int(index as i32)),
                    )),
                    bb_id,
                ));
                self.generate_lvalue_zero_init_ir(bb_id, new_lvalue, ir_vec);
            }
        }
    }

    fn parse_int_literal(&self, s: &str) -> i32 {
        let ret;
        if s.starts_with("0x") || s.starts_with("0X") {
            ret = i32::from_str_radix(s.trim_start_matches("0x").trim_start_matches("0X"), 16)
                .unwrap();
        } else if s.starts_with("0") {
            ret = i32::from_str_radix(s.trim_start_matches("0o").trim_start_matches("0O"), 8)
                .unwrap();
        } else {
            ret = i32::from_str(s).unwrap();
        }
        ret
    }

    fn parse_float_literal(&self, s: &str) -> f32 {
        // println!("float literal: {}", s);
        let result = if s.starts_with("0x") || s.starts_with("0X") {
            parse_hexf32(s, false).unwrap()
        } else {
            f32::from_str(s).unwrap()
        };
        // println!("float: {}", result);
        result
    }

    fn register_lib_func(&mut self) {
        // get from i/o
        self.module
            .register_lib_func("getint", Type::Int, ArgumentList::Normal(vec![]));
        self.module
            .register_lib_func("getch", Type::Int, ArgumentList::Normal(vec![]));
        self.module
            .register_lib_func("getfloat", Type::Float, ArgumentList::Normal(vec![]));
        self.module.register_lib_func(
            "getarray",
            Type::Int,
            ArgumentList::Normal(vec![SSALeftValue::new_arg_unknown_length_array(
                String::from("arg_0"),
                0,
                Type::Int,
            )]),
        );
        self.module.register_lib_func(
            "getfarray",
            Type::Int,
            ArgumentList::Normal(vec![SSALeftValue::new_arg_unknown_length_array(
                String::from("arg_0"),
                0,
                Type::Float,
            )]),
        );

        // put to i/o
        self.module.register_lib_func(
            "putint",
            Type::Void,
            ArgumentList::Normal(vec![SSALeftValue::new_arg_scalar(
                String::from("arg_0"),
                0,
                Type::Int,
            )]),
        );
        self.module.register_lib_func(
            "putch",
            Type::Void,
            ArgumentList::Normal(vec![SSALeftValue::new_arg_scalar(
                String::from("arg_0"),
                0,
                Type::Int,
            )]),
        );
        self.module.register_lib_func(
            "putfloat",
            Type::Void,
            ArgumentList::Normal(vec![SSALeftValue::new_arg_scalar(
                String::from("arg_0"),
                0,
                Type::Float,
            )]),
        );
        self.module.register_lib_func(
            "putarray",
            Type::Void,
            ArgumentList::Normal(vec![
                SSALeftValue::new_arg_scalar(String::from("arg_0"), 0, Type::Int),
                SSALeftValue::new_arg_unknown_length_array(String::from("arg_1"), 1, Type::Int),
            ]),
        );
        self.module.register_lib_func(
            "putfarray",
            Type::Void,
            ArgumentList::Normal(vec![
                SSALeftValue::new_arg_scalar(String::from("arg_0"), 0, Type::Int),
                SSALeftValue::new_arg_unknown_length_array(String::from("arg_1"), 1, Type::Float),
            ]),
        );
        self.module
            .register_lib_func("putf", Type::Void, ArgumentList::Variadic);

        // time
        self.module
            .register_lib_func("starttime", Type::Void, ArgumentList::Normal(vec![]));
        self.module
            .register_lib_func("stoptime", Type::Void, ArgumentList::Normal(vec![]));

        // our lib
        // void __sysy_homemade_mem_zero_init(int addr[], int start, int end) {
        //     for(int i = start; i < end; i++) {
        //         addr[i] = 0;
        //     }
        // }
        self.module.register_lib_func(
            "__sysy_homemade_mem_zero_init",
            Type::Void,
            ArgumentList::Normal(vec![
                SSALeftValue::new_arg_unknown_length_array(String::from("data"), 0, Type::Int),
                SSALeftValue::new_arg_scalar(String::from("start"), 1, Type::Int),
                SSALeftValue::new_arg_scalar(String::from("end"), 2, Type::Int),
            ]),
        )
    }

    #[inline(always)]
    fn cur_function(&mut self) -> &mut Function {
        self.module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap()
    }

    fn new_id(&mut self) -> i32 {
        self.cur_function().alloc_ssa_id()
    }

    fn convert_type(&mut self, rvalue: SSARightValue, target_type: Type) -> SSARightValue {
        let rvalue_type = rvalue.get_type();
        if rvalue_type == target_type {
            return rvalue;
        }
        let cur_bb_id = self.cur_bb.unwrap();
        let new_rvalue = self.cur_function().new_reg(target_type);
        let convert_ir = match (rvalue_type, target_type) {
            (Type::Int, Type::Float) => {
                Instruction::new(Box::new(Sitofp::new(new_rvalue.clone(), rvalue)), cur_bb_id)
            }
            (Type::Float, Type::Int) => {
                Instruction::new(Box::new(Fptosi::new(new_rvalue.clone(), rvalue)), cur_bb_id)
            }
            _ => {
                panic!("Invalid Type Conversion");
            }
        };
        self.cur_function().add_inst2bb(convert_ir);
        new_rvalue
    }
}

impl<'input> ParseTreeVisitorCompat<'input> for SysYAstVisitor<'_> {
    ///
    type Node = SysYParserContextType;
    ///
    type Return = AstNodeResult;
    fn temp_result(&mut self) -> &mut Self::Return {
        &mut self.node_result
    }
}

impl<'input> SysYVisitorCompat<'input> for SysYAstVisitor<'_> {
    /**
     * Visit a parse tree produced by {@link SysYParser#compUnit}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_compUnit(&mut self, ctx: &CompUnitContext<'input>) -> Self::Return {
        log::trace!("visit_compUnit");
        let mut func_entry = Function::new_normal_func(self.cur_func_name.clone(), Type::Void);
        self.cur_bb = Some(func_entry.alloc_bb_with_alias("_init".to_string()));
        self.module.add_function(func_entry);
        self.register_lib_func();
        let res = self.visit_children(ctx);
        log::trace!("visit_compUnit end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#decl}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_decl(&mut self, ctx: &DeclContext<'input>) -> Self::Return {
        log::trace!("visit_decl");
        let res = self.visit_children(ctx);
        log::trace!("visit_decl end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDecl}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_constDecl(&mut self, ctx: &ConstDeclContext<'input>) -> Self::Return {
        log::trace!("visit_constDecl");
        self.value_mode = ValueMode::Const;
        ctx.bType().unwrap().accept(self);
        let return_content = self.return_content();
        self.cur_type = return_content.into_var_type().unwrap();
        ctx.constDef_all().into_iter().for_each(|def| {
            def.accept(self);
        });
        self.value_mode = ValueMode::Normal;
        log::trace!("visit_constDecl end");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#bType}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_bType(&mut self, ctx: &BTypeContext<'input>) -> Self::Return {
        log::trace!("visit_bType: {}", ctx.get_text());
        let res =
            AstReturnContent::new_var_type(Type::from_str(ctx.get_text().as_str()).unwrap()).into();
        log::trace!("visit_bType end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constDef}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_constDef(&mut self, ctx: &ConstDefContext<'input>) -> Self::Return {
        let ident_name = if let Some(ident) = ctx.Identifier() {
            ident.get_text()
        } else {
            panic!("visit_constDef: identifier not found")
        };

        if self.cur_func_name == "_init" {
            if self.module.have_function(&ident_name) || self.global_vtable.is_exist(&ident_name) {
                panic!("visit_constDef: identifier already exist")
            }
        } else {
            if self.module.have_function(&ident_name)
                || match &self.cur_vtable {
                    Some(vtable) => vtable.is_exist(&ident_name),
                    None => false,
                }
            {
                panic!("visit_constDef: identifier already exist")
            }
        }

        log::trace!("visit_constDef: identifier: {}", ident_name);

        /* get shape */
        let mut shape = ctx
            .constExp_all()
            .into_iter()
            .map(|exp| {
                exp.accept(self);
                let return_content = self.return_content();
                match return_content.into_exp().unwrap() {
                    AstExp::StaticValue(Immediate::Int(i)) => i,
                    _ => panic!("visit_constDef: shape must be int"),
                }
            })
            .collect::<Vec<_>>();
        let init_value = self.parse_const_init(
            &*ctx.constInitVal().unwrap(),
            &mut shape,
            self.cur_type.clone(),
        );
        let rvalue_vec: Vec<SSARightValue> = init_value
            .iter()
            .map(|i| SSARightValue::new_imme(*i))
            .collect();
        let left_id = self.new_id();
        let mut entry = VariableEntry::new_const_array(
            left_id,
            self.cur_type.clone(),
            ident_name.clone(),
            shape,
            init_value,
        );

        if self.cur_func_name == "_init" {
            entry.set_global();
            let globaldecl_ir = Instruction::new(Box::new(GlobalDecl::new(entry.clone())), 0);
            let func = self.cur_function();
            func.add_inst2bb(globaldecl_ir);
            self.module.global_scope_mut().new_mem_object(entry.clone());
        } else {
            let entry_bb_id = self.cur_function().entry_bb_id();
            let cur_bb_id = self.cur_bb.unwrap();
            let mut instrs: Vec<Instruction> = vec![];
            if cur_bb_id != entry_bb_id {
                instrs.push(Instruction::new(
                    Box::new(Alloca::new(entry.to_address())),
                    entry_bb_id,
                ));
                self.generate_lvalue_zero_init_ir(entry_bb_id, entry.clone(), &mut instrs);
                self.cur_function().add_instrs2bb_at_front(instrs);
                let mut instrs: Vec<Instruction> = vec![];
                self.generate_lvalue_init_ir_wrap(
                    cur_bb_id,
                    entry.clone(),
                    rvalue_vec,
                    &mut instrs,
                );
                self.cur_function().add_insts2bb(instrs);
            } else {
                instrs.push(Instruction::new(
                    Box::new(Alloca::new(entry.to_address())),
                    entry_bb_id,
                ));
                self.generate_lvalue_init_ir_wrap(
                    entry_bb_id,
                    entry.clone(),
                    rvalue_vec,
                    &mut instrs,
                );
                self.cur_function().add_insts2bb(instrs);
            }
            let func = self.cur_function();
            func.mem_scope_mut().new_mem_object(entry.clone());
            log::trace!("func: {}, insts_len: {}", func.name(), func.inst_len());
        }

        self.cur_vtable
            .as_mut()
            .unwrap()
            .append(ident_name.clone(), entry);
        log::trace!("visit_constDef: identifier: {} done", ident_name);
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code scalarConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_scalarConstInitVal(
        &mut self,
        ctx: &ScalarConstInitValContext<'input>,
    ) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listConstInitVal}
     * labeled alternative in {@link SysYParser#constInitVal}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_listConstInitVal(&mut self, ctx: &ListConstInitValContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#varDecl}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_varDecl(&mut self, ctx: &VarDeclContext<'input>) -> Self::Return {
        log::trace!("visit_varDecl: {}", ctx.get_text());
        ctx.bType().unwrap().accept(self);
        let return_content = self.return_content();
        self.cur_type = return_content.into_var_type().unwrap();
        let _ = ctx
            .varDef_all()
            .into_iter()
            .map(|var_def| var_def.accept(self))
            .collect::<Vec<_>>();
        log::trace!("visit_varDecl: {} done", ctx.get_text());
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code uninitVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_uninitVarDef(&mut self, ctx: &UninitVarDefContext<'input>) -> Self::Return {
        log::trace!("visit_uninitVarDef: {}", ctx.get_text());
        let ident_name = ctx.Identifier().unwrap().get_text();
        if self.cur_func_name == "_init" {
            if self.module.have_function(&ident_name) || self.global_vtable.is_exist(&ident_name) {
                panic!("visit_uninitVarDef: identifier already exist")
            }
        } else {
            if self.module.have_function(&ident_name)
                || match &self.cur_vtable {
                    Some(vtable) => vtable.is_exist(&ident_name),
                    None => false,
                }
            {
                panic!("visit_uninitVarDef: identifier already exist")
            }
        }

        let mut shape = vec![];
        if !ctx.constExp_all().is_empty() {
            self.value_mode = ValueMode::Const;
            shape = ctx
                .constExp_all()
                .into_iter()
                .map(|exp| {
                    exp.accept(self);
                    let return_content = self.return_content();
                    match return_content.into_exp().unwrap() {
                        AstExp::StaticValue(Immediate::Int(i)) => i,
                        _ => panic!("shape must be int"),
                    }
                })
                .collect::<Vec<_>>();
            self.value_mode = ValueMode::Normal;
        }
        let mut entry =
            VariableEntry::new_normal(self.new_id(), self.cur_type, ident_name.clone(), shape);

        if self.cur_func_name != "_init" {
            let cur_bb_id = self.cur_bb.unwrap();
            let entry_bb_id = self.cur_function().entry_bb_id();
            let instr = Instruction::new(Box::new(Alloca::new(entry.to_address())), entry_bb_id);
            if cur_bb_id != entry_bb_id {
                self.cur_function().add_instr2bb_at_front(instr);
            } else {
                self.cur_function().add_inst2bb(instr);
            }
            self.cur_function()
                .mem_scope_mut()
                .new_mem_object(entry.clone());
        } else {
            entry.set_global();
            self.cur_function().add_inst2bb(Instruction::new(
                Box::new(GlobalDecl::new(entry.clone())),
                0,
            ));
            self.module.global_scope_mut().new_mem_object(entry.clone());
        };
        self.cur_vtable
            .as_mut()
            .unwrap()
            .append(ident_name.clone(), entry.clone());

        log::trace!("visit_uninitVarDef: identifier: {} done", ident_name);
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code initVarDef}
     * labeled alternative in {@link SysYParser#varDef}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_initVarDef(&mut self, ctx: &InitVarDefContext<'input>) -> Self::Return {
        log::trace!("visit_initVarDef: {}", ctx.get_text());
        let ident_name = ctx.Identifier().unwrap().get_text();
        if self.cur_func_name == "_init" {
            if self.module.have_function(&ident_name) || self.global_vtable.is_exist(&ident_name) {
                panic!("visit_initVarDef: identifier already exist")
            }
        } else {
            // allow duplicate local variable name with function name
            if match &self.cur_vtable {
                Some(vtable) => vtable.is_exist(&ident_name),
                None => false,
            } {
                panic!("visit_initVarDef: identifier already exist")
            }
        }
        let mut shape = vec![];
        if !ctx.constExp_all().is_empty() {
            self.value_mode = ValueMode::Const;
            shape = ctx
                .constExp_all()
                .into_iter()
                .map(|exp| {
                    exp.accept(self);
                    let return_content = self.return_content();
                    match return_content.into_exp().unwrap() {
                        AstExp::StaticValue(Immediate::Int(i)) => i,
                        other_content => {
                            panic!("get: {:?}, shape must be int literal", other_content)
                        }
                    }
                })
                .collect::<Vec<_>>();
            self.value_mode = ValueMode::Normal;
        }

        if self.cur_func_name == "_init" {
            self.value_mode = ValueMode::Const;
        }

        let init_vals = self.parse_var_init(ctx.initVal().unwrap().as_ref(), &mut shape);

        let mut entry: SSALeftValue =
            VariableEntry::new_normal(self.new_id(), self.cur_type, ident_name.clone(), shape);

        if self.cur_func_name == "_init" {
            entry.set_global();
            if !Self::is_init_value_all_zero(&init_vals) {
                let init_value = init_vals
                    .iter()
                    .map(|val| val.inner().clone().into_immediate().unwrap())
                    .collect::<Vec<_>>();
                entry.set_init_value(Some(init_value));
            }
            let global = Instruction::new(Box::new(GlobalDecl::new(entry.clone())), 0);
            self.cur_function().add_inst2bb(global);
            self.module.global_scope_mut().new_mem_object(entry.clone());
        } else {
            let entry_bb_id = self.cur_function().entry_bb_id();
            let cur_bb_id = self.cur_bb.unwrap();
            let mut instrs: Vec<Instruction> = vec![];
            instrs.push(Instruction::new(
                Box::new(Alloca::new(entry.to_address())),
                entry_bb_id,
            ));
            if cur_bb_id != entry_bb_id {
                self.cur_function().add_instrs2bb_at_front(instrs);
                let mut instrs: Vec<Instruction> = vec![];
                self.generate_lvalue_init_ir_wrap(cur_bb_id, entry.clone(), init_vals, &mut instrs);
                self.cur_function().add_insts2bb(instrs);
            } else {
                self.generate_lvalue_init_ir_wrap(cur_bb_id, entry.clone(), init_vals, &mut instrs);
                self.cur_function().add_insts2bb(instrs);
            }
            self.cur_function()
                .mem_scope_mut()
                .new_mem_object(entry.clone());
        }
        self.cur_vtable
            .as_mut()
            .unwrap()
            .append(ident_name.clone(), entry.clone());
        self.value_mode = ValueMode::Normal;
        log::trace!("visit_initVarDef: {} done", ctx.get_text());
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code scalarInitVal}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_scalarInitVal(&mut self, ctx: &ScalarInitValContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by the {@code listInitval}
     * labeled alternative in {@link SysYParser#initVal}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_listInitval(&mut self, ctx: &ListInitvalContext<'input>) -> Self::Return {
        self.visit_children(ctx)
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcDef}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcDef(&mut self, ctx: &FuncDefContext<'input>) -> Self::Return {
        log::trace!("visit_funcDef");
        let func_name = ctx.Identifier().expect("cannot be None").get_text();
        log::trace!("func_name: {:?}", func_name);
        ctx.funcType().expect("cannot be None").accept(self);
        let return_content = self.return_content();
        let func_type: Type = return_content.into_func_type().unwrap();
        log::trace!("func_type: {:?}", func_type);
        let func_entry = Function::new_normal_func(func_name.clone(), func_type.clone());
        self.module.add_function(func_entry);

        self.has_return = false;
        self.cur_func_name = func_name.clone();
        self.cur_vtable = Some(self.cur_vtable.clone().unwrap().new_ctable()); // todo: 检查self.cur_vtable.clone().unwrap().new_ctable()这么写会不会有问题
        self.cur_bb = Some(self.cur_function().alloc_bb_with_alias("entry".to_string()));

        // parse params
        let is_have_args = match ctx.funcFParams() {
            Some(v) => {
                v.accept(self);
                true
            }
            None => false,
        };
        if is_have_args {
            let ret = self.return_content();
            let args = ret.into_func_f_params().unwrap();
            let func_entry = self.module.functions_mut().get_mut(&func_name).unwrap();
            func_entry.set_arg_list(ArgumentList::Normal(
                args.iter().map(|(_, i)| i.clone()).collect::<Vec<_>>(),
            ));
            for (arg_idx, (arg_name, arg)) in args.into_iter().enumerate() {
                let id = func_entry.alloc_ssa_id();
                let lvalue = arg.gen_save_arg_lvalue(id);
                let alloca_ir = Instruction::new(
                    Box::new(Alloca::new(lvalue.to_address())),
                    self.cur_bb.unwrap(),
                );
                func_entry.add_inst2bb(alloca_ir);
                let store_ir = Instruction::new(
                    Box::new(Store::new(lvalue.to_address(), arg.to_arg_rvalue())),
                    self.cur_bb.unwrap(),
                );
                func_entry.add_inst2bb(store_ir);
                self.cur_vtable
                    .as_mut()
                    .unwrap()
                    .append(arg_name, lvalue.clone()); // arg is saved in lvalue after alloca and store
                func_entry.mem_scope_mut().new_mem_object(lvalue);
                func_entry.mem_scope_mut().new_argument(arg_idx, arg);
            }
        }

        if func_type != Type::Void {
            let id = self.cur_function().alloc_ssa_id();
            let lvalue = SSALeftValue::new(id, func_type);
            let alloca = Instruction::new(
                Box::new(Alloca::new(lvalue.to_address())),
                self.cur_bb.unwrap(),
            );
            self.cur_function().add_inst2bb(alloca);
            self.ret_value_opt = Some(lvalue.clone());
            self.cur_function()
                .mem_scope_mut()
                .new_mem_object(lvalue.clone());
            let store = if func_type == Type::Int {
                Instruction::new(
                    Box::new(Store::new(
                        lvalue.to_address(),
                        SSARightValue::new_imme(Immediate::Int(0)),
                    )),
                    self.cur_bb.unwrap(),
                )
            } else {
                Instruction::new(
                    Box::new(Store::new(
                        lvalue.to_address(),
                        SSARightValue::new_imme(Immediate::Float(0.0)),
                    )),
                    self.cur_bb.unwrap(),
                )
            };
            self.cur_function().add_inst2bb(store);
        }

        self.depth += 1;
        ctx.block().unwrap().accept(self);
        self.depth -= 1;

        if let Some(ret_bb) = self.ret_bb_opt.clone() {
            if func_type != Type::Void {
                let ret_rvalue =
                    SSARightValue::new_reg(self.cur_function().alloc_ssa_id(), func_type.clone());

                let load = Instruction::new(
                    Box::new(Load::new(
                        self.ret_value_opt.clone().unwrap().to_address(),
                        ret_rvalue.clone(),
                    )),
                    ret_bb,
                );
                self.cur_function().add_inst2bb(load);
                let real_ret_value = self.convert_type(ret_rvalue, func_type);
                let ret = Instruction::new(Box::new(Ret::new(Some(real_ret_value))), ret_bb);
                self.module
                    .functions_mut()
                    .get_mut(&func_name)
                    .unwrap()
                    .add_inst2bb(ret);
            } else {
                let ret = Instruction::new(Box::new(Ret::new(None)), ret_bb);
                self.cur_function().add_inst2bb(ret);
                let cur_bb_id = self.cur_bb.unwrap();
                let cur_bb = self.cur_function().bb(cur_bb_id).unwrap();
                if !cur_bb.have_exit() {
                    let br = Instruction::new(Box::new(Branch::new_label(ret_bb)), cur_bb_id);
                    self.cur_function().add_inst2bb(br);
                }
            }
        } else {
            if !self.has_return {
                log::trace!("add ret");
                let ret = Instruction::new(Box::new(Ret::new(None)), self.cur_bb.unwrap());
                self.cur_function().add_inst2bb(ret);
            }
        }

        self.ret_value_opt = None;
        self.ret_bb_opt = None;
        self.cur_func_name = "_init".to_string();
        self.cur_vtable = match self.cur_vtable.clone() {
            Some(vtable) => vtable.clone().ptable,
            None => None,
        };
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcType}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcType(&mut self, ctx: &FuncTypeContext<'input>) -> Self::Return {
        AstReturnContent::new_func_type(Type::from_str(ctx.get_text().as_str()).unwrap()).into()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParams}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcFParams(&mut self, ctx: &FuncFParamsContext<'input>) -> Self::Return {
        log::trace!("visit_funcFParams");
        let ret = ctx
            .funcFParam_all()
            .iter()
            .map(|func_f_param| {
                func_f_param.accept(self);
                let res = self.return_content();
                let param = res.into_func_f_param().unwrap();
                param
            })
            .collect::<Vec<_>>();
        log::trace!("leave_funcFParams");
        AstReturnContent::new_func_f_params(ret).into()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcFParam}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcFParam(&mut self, ctx: &FuncFParamContext<'input>) -> Self::Return {
        log::trace!("visit_funcFParam");
        let ty = Type::from_str(ctx.bType().unwrap().get_text().as_str()).unwrap();
        let name = ctx.Identifier().unwrap().get_text().to_string();
        let mut shape: Vec<i32> = vec![];
        if ctx.Lbrkt_all().len() > 0 {
            shape.push(-1);
        }
        self.value_mode = ValueMode::Const;
        self.cur_type = ty;
        ctx.constExp_all().iter().for_each(|x| {
            x.accept(self);
            let ret = self.return_content();
            let dim = match ret.into_exp().unwrap() {
                AstExp::StaticValue(Immediate::Int(i)) => i,
                _ => unreachable!(),
            };
            shape.push(dim);
        });
        self.value_mode = ValueMode::Normal;
        let entry = if shape.len() == 0 {
            SSALeftValue::new_arg_scalar(name.clone(), self.cur_function().alloc_ssa_id(), ty)
        } else {
            SSALeftValue::new_arg_array(name.clone(), self.cur_function().alloc_ssa_id(), ty, shape)
        };
        log::trace!("leave_funcFParam");
        AstReturnContent::FuncFParam((name, entry)).into()
    }
    /**
     * Visit a parse tree produced by {@link SysYParser#block}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_block(&mut self, ctx: &BlockContext<'input>) -> Self::Return {
        log::trace!("visit_block");
        let res = self.visit_children(ctx);
        log::trace!("leave_block");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#blockItem}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_blockItem(&mut self, ctx: &BlockItemContext<'input>) -> Self::Return {
        log::trace!("visit_blockItem");
        let res = self.visit_children(ctx);
        log::trace!("leave_blockItem");
        res
    }

    /**
     * Visit a parse tree produced by the {@code assignment}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_assignment(&mut self, ctx: &AssignmentContext<'input>) -> Self::Return {
        log::trace!("visit_assignment");
        ctx.lVal().unwrap().accept(self);
        let return_content = self.return_content();
        let lvalue_addr = return_content.into_exp().unwrap().into_ssa_value().unwrap();
        ctx.exp().unwrap().accept(self);
        let return_content = self.return_content();
        let rvalue = match return_content {
            AstReturnContent::Exp(AstExp::SSAValue(rvalue)) => rvalue,
            _ => panic!("rvalue is not SSARightValue"),
        };
        if self.cur_bb.is_none() {
            panic!("cur_bb is None");
        }
        let rvalue = self.convert_type(rvalue, lvalue_addr.get_type());
        let store = Instruction::new(
            Box::new(Store::new(lvalue_addr, rvalue)),
            self.cur_bb.unwrap(),
        );
        self.cur_function().add_inst2bb(store);
        log::trace!("leave_assignment");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code expStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_expStmt(&mut self, ctx: &ExpStmtContext<'input>) -> Self::Return {
        log::trace!("visit_expStmt");
        let res = self.visit_children(ctx);
        log::trace!("leave_expStmt");
        res
    }

    /**
     * Visit a parse tree produced by the {@code blockStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_blockStmt(&mut self, ctx: &BlockStmtContext<'input>) -> Self::Return {
        log::trace!("visit_blockStmt");
        self.cur_vtable = Some(self.cur_vtable.clone().unwrap().new_ctable());
        ctx.block().unwrap().accept(self);
        self.cur_vtable = self.cur_vtable.clone().unwrap().ptable;
        log::trace!("leave_blockStmt");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt1}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_ifStmt1(&mut self, ctx: &IfStmt1Context<'input>) -> Self::Return {
        log::trace!("visit_ifStmt1");
        ctx.cond().unwrap().accept(self);
        self.cur_bb = self.true_bb_stack.pop();
        self.module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap()
            .bb_mut(self.cur_bb.unwrap())
            .unwrap()
            .set_alias("if.then".to_string());
        self.depth += 1;
        ctx.stmt().unwrap().accept(self);
        self.depth -= 1;
        let cur_func = self
            .module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap();
        let bb = cur_func.bb_mut(self.cur_bb.unwrap()).unwrap();
        let false_branch_bb = self.false_bb_stack.pop();
        if !bb.have_exit() {
            let br_ir = Instruction::new(
                Box::new(Branch::new_label(false_branch_bb.unwrap())),
                self.cur_bb.unwrap(),
            );
            cur_func.add_inst2bb(br_ir);
            let false_bb = cur_func.bb_mut(false_branch_bb.unwrap()).unwrap();
            false_bb.add_prev_bb(self.cur_bb.unwrap());
            let cur_bb = cur_func.bb_mut(self.cur_bb.unwrap()).unwrap();
            cur_bb.add_succ_bb(false_branch_bb.unwrap());
        } else {
            // may have return in true branch
        }
        self.cur_bb = false_branch_bb;
        let bb = cur_func.bb_mut(self.cur_bb.unwrap()).unwrap();
        bb.set_alias("if.after".to_string());
        log::trace!("leave_ifStmt1");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code ifStmt2}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_ifStmt2(&mut self, ctx: &IfStmt2Context<'input>) -> Self::Return {
        log::trace!("visit_ifStmt2");
        ctx.cond().unwrap().accept(self);

        self.cur_bb = Some(*self.true_bb_stack.last().unwrap());
        self.module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap()
            .bb_mut(self.cur_bb.unwrap())
            .unwrap()
            .set_alias("if.then".to_string());
        self.depth += 1;
        ctx.stmt(0).unwrap().accept(self);
        self.depth -= 1;
        let true_branch_last_bb = self.cur_bb;
        let false_branch_bb = Some(*self.false_bb_stack.last().unwrap());
        self.cur_bb = false_branch_bb;
        self.module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap()
            .bb_mut(self.cur_bb.unwrap())
            .unwrap()
            .set_alias("if.else".to_string());
        self.depth += 1;
        ctx.stmt(1).unwrap().accept(self);
        self.depth -= 1;
        let false_branch_last_bb = self.cur_bb;
        self.true_bb_stack.pop();
        self.false_bb_stack.pop();

        self.cur_bb = Some(
            self.cur_function()
                .alloc_bb_with_alias("after if".to_string()),
        );
        if !self
            .cur_function()
            .bb_mut(true_branch_last_bb.unwrap())
            .unwrap()
            .have_exit()
        {
            let br_ir = Instruction::new(
                Box::new(Branch::new_label(self.cur_bb.unwrap())),
                true_branch_last_bb.unwrap(),
            );
            let cur_bb_id = self.cur_bb.unwrap();
            let cur_func = self.cur_function();
            cur_func.add_inst2bb(br_ir);
            cur_func
                .bb_mut(cur_bb_id)
                .unwrap()
                .add_prev_bb(true_branch_last_bb.unwrap());
            cur_func
                .bb_mut(true_branch_last_bb.unwrap())
                .unwrap()
                .add_succ_bb(cur_bb_id);
        } else {
            // may have return in true branch
        }

        if !self
            .cur_function()
            .bb_mut(false_branch_last_bb.unwrap())
            .unwrap()
            .have_exit()
        {
            let br_ir = Instruction::new(
                Box::new(Branch::new_label(self.cur_bb.unwrap())),
                false_branch_last_bb.unwrap(),
            );
            let cur_bb_id = self.cur_bb.unwrap();
            let cur_func = self.cur_function();
            cur_func.add_inst2bb(br_ir);
            cur_func
                .bb_mut(cur_bb_id)
                .unwrap()
                .add_prev_bb(false_branch_last_bb.unwrap());
            cur_func
                .bb_mut(false_branch_last_bb.unwrap())
                .unwrap()
                .add_succ_bb(cur_bb_id);
        } else {
            // may have return in true branch
        }
        log::trace!("leave_ifStmt2");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code whileStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_whileStmt(&mut self, ctx: &WhileStmtContext<'input>) -> Self::Return {
        log::trace!("visit_whileStmt");
        let cur_func = self
            .module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap();
        let cond_bb = cur_func.alloc_bb_with_alias("while.cond".to_string());
        self.continue_target_bb.push(cond_bb);
        let branch_ir =
            Instruction::new(Box::new(Branch::new_label(cond_bb)), self.cur_bb.unwrap());
        cur_func.add_inst2bb(branch_ir);
        cur_func
            .bb_mut(cond_bb)
            .unwrap()
            .add_prev_bb(self.cur_bb.unwrap());
        cur_func
            .bb_mut(self.cur_bb.unwrap())
            .unwrap()
            .add_succ_bb(cond_bb);
        self.cur_bb = Some(cond_bb);
        ctx.cond().unwrap().accept(self);
        let cur_func = self
            .module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap();
        self.break_target_bb
            .push(*self.false_bb_stack.last().unwrap());
        self.cur_bb = Some(*self.true_bb_stack.last().unwrap());
        let cur_block = cur_func.bb_mut(self.cur_bb.unwrap()).unwrap();
        cur_block.set_alias("while.true".to_string());
        self.depth += 1;
        ctx.stmt().unwrap().accept(self);
        self.depth -= 1;
        let cur_func = self
            .module
            .functions_mut()
            .get_mut(&self.cur_func_name)
            .unwrap();
        if !cur_func.bb_mut(self.cur_bb.unwrap()).unwrap().have_exit() {
            let br_ir = Instruction::new(
                Box::new(Branch::new_label(*self.continue_target_bb.last().unwrap())),
                self.cur_bb.unwrap(),
            );
            cur_func.add_inst2bb(br_ir);
            let continue_target_block = cur_func
                .bb_mut(*self.continue_target_bb.last().unwrap())
                .unwrap();
            continue_target_block.add_prev_bb(self.cur_bb.unwrap());
            cur_func
                .bb_mut(self.cur_bb.unwrap())
                .unwrap()
                .add_succ_bb(*self.continue_target_bb.last().unwrap());
        } else {
            // while last true basic block may return early
        }

        self.cur_bb = Some(*self.false_bb_stack.last().unwrap());
        let cur_block = cur_func.bb_mut(self.cur_bb.unwrap()).unwrap();
        cur_block.set_alias("while.after".to_string());
        self.true_bb_stack.pop();
        self.false_bb_stack.pop();
        self.continue_target_bb.pop();
        self.break_target_bb.pop();
        log::trace!("leave_whileStmt");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by the {@code breakStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_breakStmt(&mut self, ctx: &BreakStmtContext<'input>) -> Self::Return {
        log::trace!("visit_breakStmt");
        let res = self.visit_children(ctx);
        if self.break_target_bb.len() == 0 {
            return SemanticError::InvalidBreak.into();
        }
        let cur_bb_id = self.cur_bb.unwrap();
        let break_target_bb_id = *self.break_target_bb.last().unwrap();
        let br_ir = Instruction::new(Box::new(Branch::new_label(break_target_bb_id)), cur_bb_id);
        let cur_func = self.cur_function();
        cur_func.add_inst2bb(br_ir);
        let break_target_bb = cur_func.bb_mut(break_target_bb_id).unwrap();
        break_target_bb.add_prev_bb(cur_bb_id);
        cur_func
            .bb_mut(cur_bb_id)
            .unwrap()
            .add_succ_bb(break_target_bb_id);
        self.cur_bb = Some(cur_func.alloc_bb());
        log::trace!("leave_breakStmt");
        res
    }

    /**
     * Visit a parse tree produced by the {@code continueStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_continueStmt(&mut self, _ctx: &ContinueStmtContext<'input>) -> Self::Return {
        log::trace!("visit_continueStmt");
        let res = self.visit_children(_ctx);
        if self.continue_target_bb.len() == 0 {
            return SemanticError::InvalidContinue.into();
        }
        let cur_bb_id = self.cur_bb.unwrap();
        let continue_target_bb_id = *self.continue_target_bb.last().unwrap();
        let br_ir = Instruction::new(
            Box::new(Branch::new_label(continue_target_bb_id)),
            cur_bb_id,
        );
        let cur_func = self.cur_function();
        cur_func.add_inst2bb(br_ir);
        let continue_target_bb = cur_func.bb_mut(continue_target_bb_id).unwrap();
        continue_target_bb.add_prev_bb(cur_bb_id);
        cur_func
            .bb_mut(cur_bb_id)
            .unwrap()
            .add_succ_bb(continue_target_bb_id);
        self.cur_bb = Some(cur_func.alloc_bb());
        log::trace!("leave_continueStmt");
        res
    }

    /**
     * Visit a parse tree produced by the {@code returnStmt}
     * labeled alternative in {@link SysYParser#stmt}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_returnStmt(&mut self, ctx: &ReturnStmtContext<'input>) -> Self::Return {
        log::trace!("visit_returnStmt");
        self.has_return = true;
        if ctx.exp().is_some() {
            if self.cur_function().ret_type() == &Type::Void {
                panic!("return statement must be in a function");
            } else {
                if self.depth == 0 {
                    panic!("return statement must be in a function");
                } else if self.depth == 1 {
                    ctx.exp().unwrap().accept(self);
                    let return_content = self.return_content();
                    let rvalue = match return_content.into_exp().unwrap() {
                        AstExp::SSAValue(v) => v,
                        AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
                    };
                    if self.ret_bb_opt.is_none() {
                        // single-return, just return in current bb
                        let func_type = *self.cur_function().ret_type();
                        let real_ret_value = self.convert_type(rvalue, func_type);
                        let ret = Instruction::new(
                            Box::new(Ret::new(Some(real_ret_value))),
                            self.cur_bb.unwrap(),
                        );
                        self.cur_function().add_inst2bb(ret);
                    } else {
                        // multi-return, need a unified return bb
                        let rvalue = self
                            .convert_type(rvalue, self.ret_value_opt.as_ref().unwrap().get_type());
                        let store = Instruction::new(
                            Box::new(Store::new(
                                self.ret_value_opt.clone().unwrap().to_address(),
                                rvalue,
                            )),
                            self.cur_bb.unwrap(),
                        );
                        self.cur_function().add_inst2bb(store);
                        let branch = Instruction::new(
                            Box::new(Branch::new_label(self.ret_bb_opt.unwrap())),
                            self.cur_bb.unwrap(),
                        );
                        self.cur_function().add_inst2bb(branch);
                        let cur_bb = self.cur_bb.unwrap();
                        let ret_bb_id = self.ret_bb_opt.unwrap();
                        self.cur_function()
                            .bb_mut(ret_bb_id)
                            .unwrap()
                            .add_prev_bb(cur_bb);
                        self.cur_function()
                            .bb_mut(cur_bb)
                            .unwrap()
                            .add_succ_bb(ret_bb_id);
                    }
                } else if self.depth >= 2 {
                    if self.ret_bb_opt.is_none() {
                        let ret_bb = self
                            .cur_function()
                            .alloc_bb_with_alias("return".to_string());
                        self.ret_bb_opt = Some(ret_bb);
                    }
                    ctx.exp().unwrap().accept(self);
                    let return_content = self.return_content();
                    let rvalue = match return_content.into_exp().unwrap() {
                        AstExp::SSAValue(v) => v,
                        AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
                    };
                    let rvalue =
                        self.convert_type(rvalue, self.ret_value_opt.as_ref().unwrap().get_type());
                    let store_ir = Instruction::new(
                        Box::new(Store::new(
                            self.ret_value_opt.clone().unwrap().to_address(),
                            rvalue,
                        )),
                        self.cur_bb.unwrap(),
                    );
                    self.cur_function().add_inst2bb(store_ir);
                    let branch = Instruction::new(
                        Box::new(Branch::new_label(self.ret_bb_opt.unwrap())),
                        self.cur_bb.unwrap(),
                    );
                    self.cur_function().add_inst2bb(branch);
                    let cur_bb = self.cur_bb.unwrap();
                    let ret_bb_id = self.ret_bb_opt.unwrap();
                    self.cur_function()
                        .bb_mut(ret_bb_id)
                        .unwrap()
                        .add_prev_bb(cur_bb);
                    self.cur_function()
                        .bb_mut(cur_bb)
                        .unwrap()
                        .add_succ_bb(ret_bb_id);
                }
            }
        } else {
            if self.cur_function().ret_type() == &Type::Void {
                if self.depth == 0 {
                    panic!("return statement must be in a function");
                } else if self.depth == 1 {
                    if self.ret_bb_opt.is_none() {
                        // single-return, just return in current bb
                        let ret = Instruction::new(Box::new(Ret::new(None)), self.cur_bb.unwrap());
                        self.cur_function().add_inst2bb(ret);
                    } else {
                        // multi-return, need a unified return bb
                        let branch = Instruction::new(
                            Box::new(Branch::new_label(self.ret_bb_opt.unwrap())),
                            self.cur_bb.unwrap(),
                        );
                        self.cur_function().add_inst2bb(branch);
                        let cur_bb = self.cur_bb.unwrap();
                        let ret_bb_id = self.ret_bb_opt.unwrap();
                        self.cur_function()
                            .bb_mut(ret_bb_id)
                            .unwrap()
                            .add_prev_bb(cur_bb);
                        self.cur_function()
                            .bb_mut(cur_bb)
                            .unwrap()
                            .add_succ_bb(ret_bb_id);
                    }
                } else if self.depth >= 2 {
                    if self.ret_bb_opt.is_none() {
                        let ret_bb = self
                            .cur_function()
                            .alloc_bb_with_alias("return".to_string());
                        self.ret_bb_opt = Some(ret_bb.clone());
                    }
                    let branch_ir = Instruction::new(
                        Box::new(Branch::new_label(self.ret_bb_opt.unwrap())),
                        self.cur_bb.unwrap(),
                    );
                    let cur_bb = self.cur_bb.unwrap();
                    let ret_bb_id = self.ret_bb_opt.unwrap();
                    self.cur_function().add_inst2bb(branch_ir);
                    self.cur_function()
                        .bb_mut(ret_bb_id)
                        .unwrap()
                        .add_prev_bb(cur_bb);
                    self.cur_function()
                        .bb_mut(cur_bb)
                        .unwrap()
                        .add_succ_bb(ret_bb_id);
                }
            } else {
                panic!("return value not found in a non-void function");
            }
        }
        log::trace!("visit_returnStmt end");
        Self::Return::default()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#exp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_exp(&mut self, ctx: &ExpContext<'input>) -> Self::Return {
        log::trace!("visit_exp start");
        let res = self.visit_children(ctx);
        log::trace!("visit_exp end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#cond}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_cond(&mut self, ctx: &CondContext<'input>) -> Self::Return {
        log::trace!("visit_cond start");
        let true_branch_bb = self
            .cur_function()
            .alloc_bb_with_alias("cond.true".to_string());
        self.true_bb_stack.push(true_branch_bb);
        let false_branch_bb = self
            .cur_function()
            .alloc_bb_with_alias("cond.false".to_string());
        self.false_bb_stack.push(false_branch_bb);
        let res = self.visit_children(ctx);
        if res.is_not_empty() {
            let &true_branch_bb = self.true_bb_stack.last().unwrap();
            let &false_branch_bb = self.false_bb_stack.last().unwrap();
            let cur_bb = self.cur_bb.unwrap();
            let cur_func = self.cur_function();
            let branch = Instruction::new(
                Box::new(Branch::new(
                    true_branch_bb,
                    Some(false_branch_bb),
                    match res
                        .0
                        .as_ref()
                        .expect("semantic error")
                        .clone()
                        .into_exp()
                        .unwrap()
                    {
                        AstExp::SSAValue(v) => Some(v),
                        AstExp::StaticValue(immediate) => Some(SSARightValue::new_imme(immediate)),
                    },
                )),
                cur_bb,
            );
            cur_func.add_inst2bb(branch);
            cur_func.bb_mut(true_branch_bb).unwrap().add_prev_bb(cur_bb);
            cur_func.bb_mut(cur_bb).unwrap().add_succ_bb(true_branch_bb);
            cur_func
                .bb_mut(false_branch_bb)
                .unwrap()
                .add_prev_bb(cur_bb);
            cur_func
                .bb_mut(cur_bb)
                .unwrap()
                .add_succ_bb(false_branch_bb);
        } else {
            panic!("should have a condition expression")
        }
        log::trace!("visit_cond end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#lVal}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_lVal(&mut self, ctx: &LValContext<'input>) -> Self::Return {
        log::trace!("visit_lVal start");
        let res = ctx.Identifier().unwrap().get_text().to_string();
        log::trace!("LVal: {}", res);
        let indexs: Vec<SSARightValue> = ctx
            .exp_all()
            .iter()
            .map(|exp| {
                exp.accept(self);
                let ret = self.return_content();
                match ret.into_exp().unwrap() {
                    AstExp::SSAValue(v) => v,
                    _ => panic!("not rvalue"),
                }
            })
            .collect::<Vec<_>>();
        if self.value_mode == ValueMode::Const {
            // TODO: Array Value
            if let Some(var) = self.cur_vtable.as_mut().unwrap().get_variable(res.as_str()) {
                if let Some(value) = var.init_value() {
                    if value.len() == 1 {
                        log::trace!("leaveLVal_0");
                        return AstReturnContent::Exp(AstExp::StaticValue(value[0])).into();
                    } else {
                        panic!("No Const Init Scalar Found");
                    }
                } else {
                    panic!("No Const Init Variable Found");
                }
            } else {
                panic!("variable not found");
            }
        } else {
            let cur_bb = self.cur_bb.unwrap();
            let mut lvalue = self
                .cur_vtable
                .as_mut()
                .unwrap()
                .get_variable_mut(res.as_str())
                .expect("variable not found")
                .clone();

            if *lvalue.is_omit_first_dim() {
                // arg is a pointer, make a fake lvalue(lvalue is in another function stack frame)
                let new_lvalue = SSALeftValue::new_addr(
                    self.cur_function().alloc_ssa_id(),
                    lvalue.get_type(),
                    lvalue.get_shape(),
                );
                let load_address_from_mem = Instruction::new(
                    Box::new(Load::new(lvalue.to_address(), new_lvalue.to_address())),
                    cur_bb,
                );
                self.cur_function().add_inst2bb(load_address_from_mem);
                lvalue = new_lvalue;
            } else {
                // arg is a value
            };

            let ty = lvalue.get_type();
            let cur_func = self.cur_function();
            for index in indexs.into_iter() {
                let new_shape = lvalue.get_shape()[1..].to_vec();
                let new_lvalue =
                    SSALeftValue::new_addr(cur_func.alloc_ssa_id(), ty.clone(), new_shape.clone());
                let gepir = Instruction::new(
                    Box::new(Gep::new(
                        lvalue.to_address(),
                        new_lvalue.to_address(),
                        index,
                    )),
                    cur_bb,
                );
                cur_func.add_inst2bb(gepir);
                lvalue = new_lvalue;
            }
            log::trace!("leaveLVal_1");
            return AstReturnContent::Exp(AstExp::SSAValue(lvalue.to_address())).into();
            // return lvalue address
        }
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp1}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_primaryExp1(&mut self, ctx: &PrimaryExp1Context<'input>) -> Self::Return {
        log::trace!("visit_primaryExp1 start");
        ctx.exp().unwrap().accept(self);
        let ret = self.return_content();
        let res = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        log::trace!("visit_primaryExp1 end");
        AstReturnContent::Exp(AstExp::SSAValue(res)).into()
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp2}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_primaryExp2(&mut self, ctx: &PrimaryExp2Context<'input>) -> Self::Return {
        log::trace!("visit_primaryExp2 start");
        if self.value_mode == ValueMode::Const {
            let res = self.visit_children(ctx);
            log::trace!("visit_primaryExp2_1 end");
            res
        } else {
            // let res = self.visit_children(ctx);
            // log::trace!("visit_primaryExp2_2 end");
            // res
            ctx.lVal().unwrap().accept(self);
            let return_content = self.return_content();
            let addr = return_content.into_exp().unwrap().into_ssa_value().unwrap();
            // println!("addr: {:?}", addr);
            if addr.is_array_addr() {
                if addr.is_global() {
                    let reg = SSARightValue::new_addr(
                        self.cur_function().alloc_ssa_id(),
                        addr.get_type(),
                        addr.addr_shape().unwrap(),
                    );
                    let gep_global_array = Instruction::new(
                        Box::new(Gep::new(
                            addr,
                            reg.clone(),
                            SSARightValue::new_imme(Immediate::Int(0)),
                        )),
                        self.cur_bb.unwrap(),
                    );
                    self.cur_function().add_inst2bb(gep_global_array);
                    log::trace!("visit_primaryExp2_2 end");
                    return AstReturnContent::Exp(AstExp::SSAValue(reg)).into();
                } else {
                    log::trace!("visit_primaryExp2_3 end");
                    return AstReturnContent::Exp(AstExp::SSAValue(addr)).into();
                }
            } else {
                let reg =
                    SSARightValue::new_reg(self.cur_function().alloc_ssa_id(), addr.get_type());
                let load_value =
                    Instruction::new(Box::new(Load::new(addr, reg.clone())), self.cur_bb.unwrap());
                self.cur_function().add_inst2bb(load_value);
                log::trace!("visit_primaryExp2_4 end");
                return AstReturnContent::Exp(AstExp::SSAValue(reg)).into();
            }
        }
    }

    /**
     * Visit a parse tree produced by the {@code primaryExp3}
     * labeled alternative in {@link SysYParser#primaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_primaryExp3(&mut self, ctx: &PrimaryExp3Context<'input>) -> Self::Return {
        log::trace!("visit_primaryExp3 start");
        ctx.number().unwrap().accept(self);
        let return_content = self.return_content();
        let value = match return_content {
            AstReturnContent::NumLiteral(immediate) => immediate,
            _ => panic!("primaryExp3: not a number"),
        };
        if self.value_mode != ValueMode::Const {
            log::trace!("leavePrimaryExp3_0");
            AstReturnContent::Exp(AstExp::SSAValue(SSARightValue::new_imme(value))).into()
        } else {
            log::trace!("leavePrimaryExp3_1");
            AstReturnContent::Exp(AstExp::StaticValue(value)).into()
        }
    }

    /**
     * Visit a parse tree produced by the {@code intnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_intnum(&mut self, ctx: &IntnumContext<'input>) -> Self::Return {
        log::trace!("intnum: {}", ctx.get_text());
        let int_num = self.parse_int_literal(&ctx.get_text());
        self.cur_num_type = Type::Int;
        AstReturnContent::new_num_literal(Immediate::new_int(int_num)).into()
    }

    /**
     * Visit a parse tree produced by the {@code floatnum}
     * labeled alternative in {@link SysYParser#number}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_floatnum(&mut self, ctx: &FloatnumContext<'input>) -> Self::Return {
        log::trace!("floatnum: {}", ctx.get_text());
        let float_num = self.parse_float_literal(&ctx.get_text());
        self.cur_num_type = Type::Float;
        AstReturnContent::new_num_literal(Immediate::new_float(float_num)).into()
    }

    /**
     * Visit a parse tree produced by the {@code unary1}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_unary1(&mut self, ctx: &Unary1Context<'input>) -> Self::Return {
        log::trace!("visit_unary1 start");
        let res = self.visit_children(ctx);
        log::trace!("visit_unary1 end");
        res
    }

    /**
     * Visit a parse tree produced by the {@code unary2}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_unary2(&mut self, ctx: &Unary2Context<'input>) -> Self::Return {
        log::trace!("visit_unary2 start");
        if self.value_mode == ValueMode::Const {
            panic!("function call occurs in compile-time constant expression");
        } else {
            let func_name = ctx.Identifier().unwrap().get_text();
            let mut args: Vec<SSARightValue> = match ctx.funcRParams() {
                Some(v) => {
                    v.accept(self);
                    self.return_content().into_func_rparams().unwrap()
                }
                None => {
                    vec![]
                }
            };
            let cur_bb = self.cur_bb.unwrap();
            let callee_entry_type = *self.module.function(&func_name).ret_type();
            let callee_arg_list = self.module.function(&func_name).arg_list().clone();
            // cvt between float and int
            if callee_arg_list.is_normal() {
                let callee_arg_list = callee_arg_list.as_normal().unwrap();
                assert!(callee_arg_list.len() == args.len());
                for (i, arg) in args.iter_mut().enumerate() {
                    let arg_ty = arg.get_type();
                    let callee_arg_ty = callee_arg_list[i].get_type();
                    if arg_ty == Type::Int && callee_arg_ty == Type::Float {
                        let reg =
                            SSARightValue::new_reg(self.cur_function().alloc_ssa_id(), Type::Float);
                        let sitofp = Instruction::new(
                            Box::new(Sitofp::new(reg.clone(), arg.clone())),
                            cur_bb,
                        );
                        self.cur_function().add_inst2bb(sitofp);
                        *arg = reg;
                    } else if arg_ty == Type::Float && callee_arg_ty == Type::Int {
                        let reg =
                            SSARightValue::new_reg(self.cur_function().alloc_ssa_id(), Type::Int);
                        let fptosi = Instruction::new(
                            Box::new(Fptosi::new(reg.clone(), arg.clone())),
                            cur_bb,
                        );
                        self.cur_function().add_inst2bb(fptosi);
                        *arg = reg;
                    }
                }
            }

            if func_name == "starttime" || func_name == "stoptime" {
                let line_no = ctx.start().get_line();
                let line_no_reg =
                    SSARightValue::new_reg(self.cur_function().alloc_ssa_id(), Type::Int);
                let mov_instr = Instruction::new(
                    Box::new(Mov::new(
                        line_no_reg.clone(),
                        SSARightValue::new_imme(Immediate::new_int(line_no as i32)),
                    )),
                    cur_bb,
                );
                self.cur_function().add_inst2bb(mov_instr);
                args.push(line_no_reg);
            }

            let caller_entry = self.cur_function();
            if callee_entry_type != Type::Void {
                let ret_value = caller_entry.new_reg(callee_entry_type);
                let callir = Instruction::new(
                    Box::new(Call {
                        func_name: func_name.clone(),
                        args,
                        ret: Some(ret_value.clone()),
                    }),
                    cur_bb,
                );
                caller_entry.add_inst2bb(callir);
                log::trace!("leaveUnary2_0");
                return AstReturnContent::Exp(AstExp::SSAValue(ret_value)).into();
            } else {
                let callir = Instruction::new(
                    Box::new(Call {
                        func_name: func_name.clone(),
                        args,
                        ret: None,
                    }),
                    cur_bb,
                );
                caller_entry.add_inst2bb(callir);
                log::trace!("leaveUnary2_1");
                return Self::Return::default();
            }
        }
    }

    /**
     * Visit a parse tree produced by the {@code unary3}
     * labeled alternative in {@link SysYParser#unaryExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_unary3(&mut self, ctx: &Unary3Context<'input>) -> Self::Return {
        log::trace!("visit_unary3 start");
        assert!(ctx.get_child(0).unwrap().get_text().len() == 1);
        let opcode = ctx.get_child(0).unwrap().get_text().chars().next().unwrap();
        if self.value_mode == ValueMode::Const {
            ctx.unaryExp().unwrap().accept(self);
            let ret = self.return_content();
            let lhs = match ret {
                AstReturnContent::Exp(AstExp::StaticValue(immediate)) => immediate,
                _ => panic!("unary3: not a number"),
            };
            let compile_value = match opcode {
                '+' => lhs,
                '-' => -lhs,
                '!' => !lhs,
                _ => panic!("unary3: unknown opcode"),
            };
            log::trace!("leaveUnary3_0");
            return AstReturnContent::Exp(AstExp::StaticValue(compile_value)).into();
        } else {
            ctx.unaryExp().unwrap().accept(self);
            let ret = self.return_content();
            let ssa = match ret {
                AstReturnContent::Exp(AstExp::SSAValue(ssa)) => ssa,
                _ => panic!("unary3: not a ssa"),
            };
            let cur_bb = self.cur_bb.unwrap();
            if opcode == '+' {
                log::trace!("leaveUnary3_1");
                return AstReturnContent::Exp(AstExp::SSAValue(ssa)).into();
            } else if opcode == '-' {
                let ret_ssa = self.cur_function().new_reg(ssa.get_type());
                if ssa.get_type() == Type::Int {
                    let neg_ir = Instruction::new(Box::new(Neg::new(ret_ssa.clone(), ssa)), cur_bb);
                    self.cur_function().add_inst2bb(neg_ir);
                } else {
                    let fsub_ir: Instruction = Instruction::new(
                        Box::new(FSub::new(
                            ret_ssa.clone(),
                            SSARightValue::new_imme(Immediate::new_float(0.0)),
                            ssa,
                        )),
                        cur_bb,
                    );
                    self.cur_function().add_inst2bb(fsub_ir);
                }
                log::trace!("leaveUnary3_2");
                return AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into();
            } else if opcode == '!' {
                let ret_ssa = self.cur_function().new_reg(Type::Int);
                let ssa = self.convert_type(ssa, Type::Int);
                let lnot_ir = Instruction::new(Box::new(Not::new(ret_ssa.clone(), ssa)), cur_bb);
                self.cur_function().add_inst2bb(lnot_ir);
                log::trace!("leaveUnary3_3");
                return AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into();
            } else {
                panic!("unary3: unknown opcode");
            }
        }
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#unaryOp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_unaryOp(&mut self, ctx: &UnaryOpContext<'input>) -> Self::Return {
        log::trace!("visit_unaryOp start");
        let res = self.visit_children(ctx);
        log::trace!("visit_unaryOp end");
        res
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParams}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcRParams(&mut self, ctx: &FuncRParamsContext<'input>) -> Self::Return {
        log::trace!("visit_funcRParams start");
        let res: Vec<SSARightValue> = ctx
            .funcRParam_all()
            .iter()
            .map(|x| {
                x.accept(self);
                let ret = self.return_content();
                match ret {
                    AstReturnContent::Exp(AstExp::SSAValue(ssa)) => ssa,
                    _ => panic!("funcRParams: not a ssa"),
                }
            })
            .collect();
        log::trace!("visit_funcRParams end");
        AstReturnContent::FuncRparams(res).into()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#funcRParam}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_funcRParam(&mut self, ctx: &FuncRParamContext<'input>) -> Self::Return {
        log::trace!("visit_funcRParam start");
        assert!(self.value_mode != ValueMode::Const);
        ctx.exp().unwrap().accept(self);
        let ret = self.return_content();
        log::trace!("visit_funcRParam end");
        ret.into()
    }

    /**
     * Visit a parse tree produced by the {@code mul2}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_mul2(&mut self, ctx: &Mul2Context<'input>) -> Self::Return {
        log::trace!("mul2: {}", ctx.get_text());
        assert!(ctx.get_child(1).unwrap().get_text().len() == 1);
        let opcode = ctx.get_child(1).unwrap().get_text().chars().next().unwrap();
        if self.value_mode == ValueMode::Const {
            ctx.mulExp().unwrap().accept(self);
            let ret = self.return_content();
            let lhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(ssa) => ssa.get_value().unwrap(),
                AstExp::StaticValue(immediate) => immediate,
            };
            ctx.unaryExp().unwrap().accept(self);
            let ret = self.return_content();
            let rhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(ssa) => ssa.get_value().unwrap(),
                AstExp::StaticValue(immediate) => immediate,
            };
            let compile_value = match opcode {
                '*' => lhs * rhs,
                '/' => lhs / rhs,
                '%' => lhs % rhs,
                _ => panic!("mul2: unknown opcode"),
            };
            log::trace!("leave_Mul2_0");
            return AstReturnContent::Exp(AstExp::StaticValue(compile_value)).into();
        } else {
            if opcode != '*' && opcode != '/' && opcode != '%' {
                panic!("mul2: unknown opcode");
            }
            ctx.mulExp().unwrap().accept(self);
            let ret = self.return_content();
            let lhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(v) => v,
                AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
            };
            ctx.unaryExp().unwrap().accept(self);
            let ret = self.return_content();
            let rhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(v) => v,
                AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
            };
            let cur_bb = self.cur_bb.unwrap();
            let res_type = Type::new_type(lhs.get_type(), rhs.get_type());
            let lhs = self.convert_type(lhs, res_type);
            let rhs = self.convert_type(rhs, res_type);
            let cur_func = self.cur_function();
            let ret_ssa = cur_func.new_reg(res_type);
            let mul2_ir = if opcode == '*' {
                if res_type == Type::Int {
                    Instruction::new(
                        Box::new(Mul {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                } else {
                    Instruction::new(
                        Box::new(FMul {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                }
            } else if opcode == '/' {
                if res_type == Type::Int {
                    Instruction::new(
                        Box::new(Div {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                } else {
                    Instruction::new(
                        Box::new(FDiv {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                }
            } else {
                if res_type == Type::Int {
                    Instruction::new(
                        Box::new(Mod {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                } else {
                    unreachable!()
                }
            };
            cur_func.add_inst2bb(mul2_ir);
            log::trace!("leave_Mul2_1");
            return AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into();
        }
    }

    /**
     * Visit a parse tree produced by the {@code mul1}
     * labeled alternative in {@link SysYParser#mulExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_mul1(&mut self, ctx: &Mul1Context<'input>) -> Self::Return {
        log::trace!("mul1: {}", ctx.get_text());
        ctx.unaryExp().unwrap().accept(self);
        let res = self.return_content();
        log::trace!("leave_Mul1");
        res.into()
    }

    /**
     * Visit a parse tree produced by the {@code add2}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_add2(&mut self, ctx: &Add2Context<'input>) -> Self::Return {
        log::trace!("add2: {}", ctx.get_text());
        assert!(ctx.get_child(1).unwrap().get_text().len() == 1);
        let opcode = ctx.get_child(1).unwrap().get_text().chars().next().unwrap();
        if self.value_mode == ValueMode::Const {
            ctx.addExp().unwrap().accept(self);
            let ret = self.return_content();
            let lhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(ssa) => ssa.get_value().unwrap(),
                AstExp::StaticValue(immediate) => immediate,
            };
            ctx.mulExp().unwrap().accept(self);
            let ret = self.return_content();
            let rhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(ssa) => ssa.get_value().unwrap(),
                AstExp::StaticValue(immediate) => immediate,
            };
            let compile_value = match opcode {
                '+' => lhs + rhs,
                '-' => lhs - rhs,
                _ => panic!("add2: unknown opcode"),
            };
            log::trace!("leave_Add2_0");
            return AstReturnContent::Exp(AstExp::StaticValue(compile_value)).into();
        } else {
            if opcode != '+' && opcode != '-' {
                panic!("add2: unknown opcode");
            }
            ctx.addExp().unwrap().accept(self);
            let ret = self.return_content();
            let lhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(v) => v,
                AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
            };
            ctx.mulExp().unwrap().accept(self);
            let ret = self.return_content();
            let rhs = match ret.into_exp().unwrap() {
                AstExp::SSAValue(v) => v,
                AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
            };
            let cur_bb = self.cur_bb.unwrap();
            let res_type = Type::new_type(lhs.get_type(), rhs.get_type());
            let lhs = self.convert_type(lhs, res_type);
            let rhs = self.convert_type(rhs, res_type);
            let cur_func = self.cur_function();
            let ret_ssa = cur_func.new_reg(res_type);
            let add2_ir = if opcode == '+' {
                if res_type == Type::Int {
                    Instruction::new(
                        Box::new(Add {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                } else {
                    Instruction::new(
                        Box::new(FAdd {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                }
            } else {
                if res_type == Type::Int {
                    Instruction::new(
                        Box::new(Sub {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                } else {
                    Instruction::new(
                        Box::new(FSub {
                            d1: ret_ssa.clone(),
                            s1: lhs,
                            s2: rhs,
                        }),
                        cur_bb,
                    )
                }
            };
            cur_func.add_inst2bb(add2_ir);
            log::trace!("leave_Add2_1");
            AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into()
        }
    }

    /**
     * Visit a parse tree produced by the {@code add1}
     * labeled alternative in {@link SysYParser#addExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_add1(&mut self, ctx: &Add1Context<'input>) -> Self::Return {
        log::trace!("add1: {}", ctx.get_text());
        ctx.mulExp().unwrap().accept(self);
        let res = self.return_content();
        log::trace!("leave_Add1");
        res.into()
    }

    /**
     * Visit a parse tree produced by the {@code rel2}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_rel2(&mut self, ctx: &Rel2Context<'input>) -> Self::Return {
        log::trace!("visitrel2: {}", ctx.get_text());
        let opcode = ctx.get_child(1).unwrap().get_text();
        let cmp_type = CmpType::from_str(opcode.as_str()).unwrap();
        ctx.relExp().unwrap().accept(self);
        let ret = self.return_content();
        let lhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(ssa) => ssa,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        ctx.addExp().unwrap().accept(self);
        let ret = self.return_content();
        let rhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(ssa) => ssa,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let res_type = Type::new_type(lhs.get_type(), rhs.get_type());
        let lhs = self.convert_type(lhs, res_type);
        let rhs = self.convert_type(rhs, res_type);
        let ret_ssa = self.cur_function().new_reg(Type::Int);
        let cmp_ir = if res_type.is_int() {
            Instruction::new(
                Box::new(Icmp {
                    d1: ret_ssa.clone(),
                    s1: lhs,
                    s2: rhs,
                    cmp_type,
                }),
                self.cur_bb.unwrap(),
            )
        } else {
            Instruction::new(
                Box::new(Fcmp {
                    d1: ret_ssa.clone(),
                    s1: lhs,
                    s2: rhs,
                    cmp_type,
                }),
                self.cur_bb.unwrap(),
            )
        };
        self.cur_function().add_inst2bb(cmp_ir);
        log::trace!("leave_rel2");
        AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into()
    }

    /**
     * Visit a parse tree produced by the {@code rel1}
     * labeled alternative in {@link SysYParser#relExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_rel1(&mut self, ctx: &Rel1Context<'input>) -> Self::Return {
        log::trace!("rel1: {}", ctx.get_text());
        ctx.addExp().unwrap().accept(self);
        let ret = self.return_content();
        log::trace!("leave_rel1");
        return ret.into();
    }

    /**
     * Visit a parse tree produced by the {@code eq1}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_eq1(&mut self, ctx: &Eq1Context<'input>) -> Self::Return {
        log::trace!("eq1: {}", ctx.get_text());
        ctx.relExp().unwrap().accept(self);
        let ret = self.return_content();
        log::trace!("leave_eq1");
        return ret.into();
    }

    /**
     * Visit a parse tree produced by the {@code eq2}
     * labeled alternative in {@link SysYParser#eqExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_eq2(&mut self, ctx: &Eq2Context<'input>) -> Self::Return {
        log::trace!("eq2: {}", ctx.get_text());
        let opcode = ctx.get_child(1).unwrap().get_text();
        log::trace!("opcode: {}", opcode);
        let cmp_type = CmpType::from_str(opcode.as_str()).unwrap();
        ctx.eqExp().unwrap().accept(self);
        let ret = self.return_content();
        let lhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        ctx.relExp().unwrap().accept(self);
        let ret = self.return_content();
        let rhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let res_type = Type::new_type(lhs.get_type(), rhs.get_type());
        let lhs = self.convert_type(lhs, res_type);
        let rhs = self.convert_type(rhs, res_type);
        let ret_ssa = self.cur_function().new_reg(Type::Int);
        let cmp_ir = if res_type.is_int() {
            Instruction::new(
                Box::new(Icmp {
                    d1: ret_ssa.clone(),
                    s1: lhs,
                    s2: rhs,
                    cmp_type,
                }),
                self.cur_bb.unwrap(),
            )
        } else {
            Instruction::new(
                Box::new(Fcmp {
                    d1: ret_ssa.clone(),
                    s1: lhs,
                    s2: rhs,
                    cmp_type,
                }),
                self.cur_bb.unwrap(),
            )
        };
        self.cur_function().add_inst2bb(cmp_ir);
        log::trace!("leave_eq2");
        AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into()
    }

    /**
     * Visit a parse tree produced by the {@code lAnd2}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_lAnd2(&mut self, ctx: &LAnd2Context<'input>) -> Self::Return {
        log::trace!("visit_lAnd2: {}", ctx.get_text());
        let new_cond_bb = self
            .cur_function()
            .alloc_bb_with_alias("&& cond".to_string());
        self.true_bb_stack.push(new_cond_bb);
        ctx.lAndExp().unwrap().accept(self);
        let ret = self.return_content();
        let lhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let true_branch_bb = self.true_bb_stack.pop().unwrap();
        let &false_branch_bb = self.false_bb_stack.last().unwrap();
        let cur_bb = self.cur_bb.unwrap();
        let cur_func = self.cur_function();
        let branch_ir = Instruction::new(
            Box::new(Branch {
                cond: Some(lhs),
                label1: true_branch_bb,
                label2: Some(false_branch_bb),
            }),
            cur_bb,
        );
        cur_func.add_inst2bb(branch_ir);
        cur_func.bb_mut(true_branch_bb).unwrap().add_prev_bb(cur_bb);
        cur_func.bb_mut(cur_bb).unwrap().add_succ_bb(true_branch_bb);
        cur_func
            .bb_mut(false_branch_bb)
            .unwrap()
            .add_prev_bb(cur_bb);
        cur_func
            .bb_mut(cur_bb)
            .unwrap()
            .add_succ_bb(false_branch_bb);
        self.cur_bb = Some(true_branch_bb);
        ctx.eqExp().unwrap().accept(self);
        let ret = self.return_content();
        let res = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let cur_bb = self.cur_bb.unwrap();
        let cur_func = self.cur_function();
        let last_instr_id = cur_func.layout().block_node(cur_bb).last_inst();
        if last_instr_id.is_none()
            || !cur_func
                .instructions()
                .get(&last_instr_id.unwrap())
                .unwrap()
                .is_cmp()
        {
            let ret_ssa = cur_func.new_reg(Type::Int);
            let cmp_ir = if res.get_type().is_float() {
                Instruction::new(
                    Box::new(Fcmp {
                        d1: ret_ssa.clone(),
                        s1: res,
                        s2: SSARightValue::new_imme(Immediate::Float(0.0)),
                        cmp_type: CmpType::NEQ,
                    }),
                    cur_bb,
                )
            } else {
                Instruction::new(
                    Box::new(Icmp {
                        d1: ret_ssa.clone(),
                        s1: res,
                        s2: SSARightValue::new_imme(Immediate::Int(0)),
                        cmp_type: CmpType::NEQ,
                    }),
                    cur_bb,
                )
            };
            cur_func.add_inst2bb(cmp_ir);
            log::trace!("leave_lAnd2_0");
            return AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into();
        } else {
            log::trace!("leave lAnd2_1");
            return AstReturnContent::Exp(AstExp::SSAValue(res)).into();
        }
    }

    /**
     * Visit a parse tree produced by the {@code lAnd1}
     * labeled alternative in {@link SysYParser#lAndExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_lAnd1(&mut self, ctx: &LAnd1Context<'input>) -> Self::Return {
        log::trace!("lAnd1: {}", ctx.get_text());
        ctx.eqExp().unwrap().accept(self);
        let ret = self.return_content();
        let res = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let cur_bb = self.cur_bb.unwrap();
        let cur_func = self.cur_function();
        let last_instr_id = cur_func.layout().block_node(cur_bb).last_inst();
        if last_instr_id.is_none()
            || !cur_func
                .instructions()
                .get(&last_instr_id.unwrap())
                .unwrap()
                .is_cmp()
        {
            let ret_ssa = cur_func.new_reg(Type::Int);
            let cmp_ir = if res.get_type().is_float() {
                Instruction::new(
                    Box::new(Fcmp {
                        d1: ret_ssa.clone(),
                        s1: res,
                        s2: SSARightValue::new_imme(Immediate::Float(0.0)),
                        cmp_type: CmpType::NEQ,
                    }),
                    cur_bb,
                )
            } else {
                Instruction::new(
                    Box::new(Icmp {
                        d1: ret_ssa.clone(),
                        s1: res,
                        s2: SSARightValue::new_imme(Immediate::Int(0)),
                        cmp_type: CmpType::NEQ,
                    }),
                    cur_bb,
                )
            };
            cur_func.add_inst2bb(cmp_ir);
            log::trace!("leave_lAnd1_0");
            return AstReturnContent::Exp(AstExp::SSAValue(ret_ssa)).into();
        } else {
            log::trace!("leave_lAnd1_1");
            return AstReturnContent::Exp(AstExp::SSAValue(res)).into();
        }
    }

    /**
     * Visit a parse tree produced by the {@code lOr1}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_lOr1(&mut self, ctx: &LOr1Context<'input>) -> Self::Return {
        log::trace!("visit_lOr1: {}", ctx.get_text());
        ctx.lAndExp().unwrap().accept(self);
        let ret = self.return_content();
        log::trace!("leave_lOr1");
        ret.into()
    }

    /**
     * Visit a parse tree produced by the {@code lOr2}
     * labeled alternative in {@link SysYParser#lOrExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_lOr2(&mut self, ctx: &LOr2Context<'input>) -> Self::Return {
        log::trace!("lOr2: {}", ctx.get_text());
        let new_cond_bb = self
            .cur_function()
            .alloc_bb_with_alias("|| cond".to_string());
        self.false_bb_stack.push(new_cond_bb);
        ctx.lOrExp().unwrap().accept(self);
        let ret = self.return_content();
        let lhs = match ret.into_exp().unwrap() {
            AstExp::SSAValue(v) => v,
            AstExp::StaticValue(immediate) => SSARightValue::new_imme(immediate),
        };
        let &true_branch_bb = self.true_bb_stack.last().unwrap();
        let false_branch_bb = self.false_bb_stack.pop().unwrap();
        let cur_bb = self.cur_bb.unwrap();
        let branch_ir = Instruction::new(
            Box::new(Branch {
                cond: Some(lhs),
                label1: true_branch_bb,
                label2: Some(false_branch_bb),
            }),
            cur_bb,
        );
        let cur_func = self.cur_function();
        cur_func.add_inst2bb(branch_ir);
        cur_func.bb_mut(true_branch_bb).unwrap().add_prev_bb(cur_bb);
        cur_func.bb_mut(cur_bb).unwrap().add_succ_bb(true_branch_bb);
        cur_func
            .bb_mut(false_branch_bb)
            .unwrap()
            .add_prev_bb(cur_bb);
        cur_func
            .bb_mut(cur_bb)
            .unwrap()
            .add_succ_bb(false_branch_bb);
        self.cur_bb = Some(false_branch_bb);
        ctx.lAndExp().unwrap().accept(self);
        let ret = self.return_content();
        log::trace!("leave_lOr2");
        ret.into()
    }

    /**
     * Visit a parse tree produced by {@link SysYParser#constExp}.
     * @param ctx the parse tree
     */
    #[allow(non_snake_case)]
    fn visit_constExp(&mut self, ctx: &ConstExpContext<'input>) -> Self::Return {
        log::trace!("visit_constExp");
        ctx.addExp().unwrap().accept(self);
        let return_content = self.return_content();
        let compile_value = return_content.into_exp().unwrap();
        log::trace!("leave_constExp");
        AstReturnContent::Exp(compile_value).into()
    }
}
