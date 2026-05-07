use crate::ast::table::SymbolTable;
use crate::mir::{MIRArrayEleRef, MIRArrayInit, MIRArrayRef, MIRBlock, MIRBool, MIRBoolCmpAssign, MIRBoolCopyAssign, MIRBoolLRef, MIRBoolOper, MIRCmpRight, MIRCmpExpr, MIRCondJump, MIRFun, MIRGlDef, MIRInvoke, MIRJump, MIRLit, MIRMathLRef, MIRMathOper, MIRMathPrefix, MIRMathRight, MIRModule, MIRName, MIRNamedArrayParam, MIRNamedParam, MIRRefPool, MIRRight, MIRStmt, MIRType, MIRTypedParam, MIRUnit, MIRVarRef, MIRVarActiveType};
use crate::util::Kotlin;
use std::ops::Add;
use crate::hir::to_string::ToCodeString;
use crate::mir::cfg::{CFG, CFGBasicBlock, CFGBasicBlockRef, CFGFun};

const ENABLE_MANGLE_OUTPUT: bool = false;

pub trait ToMIRString {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String;
}

impl ToMIRString for MIRModule {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut alls = String::new();
        for single_unit in &self.units {
            let ret = single_unit.to_code_string(ref_pool, symbol_table);
            alls.push_str(&ret);
            alls.push_str("\n");
        }
        alls
    }
}

impl ToMIRString for MIRUnit {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRUnit::Fun(fun) => fun.to_code_string(ref_pool, symbol_table),
            MIRUnit::Def(def) => def.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRArrayInit {
    fn to_code_string(&self, _ref_pool: &mut MIRRefPool, _symbol_table: &mut SymbolTable) -> String {
        let mut init = "{".to_string();
        for single in &self.inits {
            init.push_str(&format!("(${},{})", single.0, single.1.to_string()));
        }
        init.push_str("}");
        init
    }
}

impl ToMIRString for MIRGlDef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRGlDef::Var(a, b, c) => {
                let is_const = if *a { "const" } else { "no-const" };
                let var_ref = b.to_code_string(ref_pool, symbol_table);
                let init = c.to_string();
                format!("{} {} : {}", is_const, var_ref, init)
            }
            MIRGlDef::Array(a, b, c) => {
                let is_const = if *a { "const" } else { "no-const" };
                let var_ref = b.to_code_string(ref_pool, symbol_table);
                let init = c.to_code_string(ref_pool, symbol_table);
                format!("{} {} : {}", is_const, var_ref, init)
            }
        }
    }
}

impl ToMIRString for MIRName {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        if self.id == 0 {
            return "zero".to_string()
        }
        if ENABLE_MANGLE_OUTPUT {
            let sym = ref_pool.get_sym_ref(self);
            let sym = symbol_table.get_symbol(sym);
            sym.get_string()
        } else {
            self.id.to_string()
        }
    }
}

impl ToString for MIRType {
    fn to_string(&self) -> String {
        match self {
            MIRType::Int => "int".to_string(),
            MIRType::Float => "float".to_string(),
            MIRType::Bool => "bool".to_string(),
            MIRType::Void => "void".to_string(),
        }
    }
}

impl ToString for MIRVarActiveType {
    fn to_string(&self) -> String {
        match self {
            MIRVarActiveType::Unknown => {
                "u".to_string()
            }
            MIRVarActiveType::Normal => {
                "n".to_string()
            }
            MIRVarActiveType::Global => {
                "g".to_string()
            }
            MIRVarActiveType::Temp => {
                "t".to_string()
            }
        }
    }
}

impl ToMIRString for MIRVarRef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let var_type = self.var_type.to_string();
        let active_type = self.active_type.to_string();
        format!("@{}({},{})", self.name.to_code_string(ref_pool, symbol_table), var_type,active_type)
    }
}

impl ToMIRString for MIRArrayRef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let array_type = self.array_type.to_string();
        let mut dims = String::new();
        for (index, dim) in self.dims.iter().enumerate() {
            dims.push_str(&dim.to_string());
            if index != self.dims.len() - 1 {
                dims.push_str("x")
            }
        }
        format!("@{}({},{})", self.name.to_code_string(ref_pool, symbol_table), array_type, dims)
    }
}

impl ToString for MIRLit {
    fn to_string(&self) -> String {
        match self {
            MIRLit::I32(i32) => i32.to_string(),
            MIRLit::F32(f32) => f32.to_string(),
        }
    }
}

impl ToMIRString for MIRArrayEleRef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let name = self.name.to_code_string(ref_pool, symbol_table);
        let var_type = self.array_type.to_string();
        format!("@{}[{}]({})", name, var_type, self.offset.to_code_string(ref_pool, symbol_table))
    }
}

impl ToMIRString for MIRMathLRef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRMathLRef::Var(var) => var.to_code_string(ref_pool, symbol_table),
            MIRMathLRef::ArrayEle(ele) => ele.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRRight {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match &self {
            MIRRight::Lit(lit) => lit.to_string(),
            MIRRight::VarRef(left) => left.to_code_string(ref_pool, symbol_table),
            MIRRight::CmpRef(cmp_ref) => cmp_ref.var.to_code_string(ref_pool, symbol_table),
            MIRRight::Invoke(invoke) => invoke.to_code_string(ref_pool, symbol_table),
            MIRRight::ArrEleRef(arr_ele) => arr_ele.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRNamedArrayParam {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let array_ref = self.name.to_code_string(ref_pool, symbol_table);
        let array_type = self.array_type.to_string();
        let dims = format!("@{}(array,{})", array_ref, array_type);
        dims
    }
}

impl ToMIRString for MIRNamedParam {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRNamedParam::Right(right) => right.to_code_string(ref_pool, symbol_table),
            MIRNamedParam::Array(array) => array.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRInvoke {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut codes = format!("call @{}(", self.name.to_code_string(ref_pool, symbol_table),);
        for (index, param) in self.params.iter().enumerate() {
            let part_param = param.to_code_string(ref_pool, symbol_table);
            codes.push_str(&part_param);
            if index != self.params.len() - 1 {
                codes.push_str(",")
            }
        }
        codes.push_str(")");
        return codes;
    }
}

impl ToMIRString for MIRMathPrefix {
    fn to_code_string(&self, _ref_pool: &mut MIRRefPool, _symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRMathPrefix::None => "".to_string(),
            MIRMathPrefix::Neg => "-".to_string(),
            MIRMathPrefix::Bang => "!".to_string(),
            MIRMathPrefix::BangBang => "!!".to_string(),
            MIRMathPrefix::NegBang => "-!".to_string(),
            MIRMathPrefix::NegBangBang => "-!!".to_string(),
        }
    }
}

impl ToMIRString for MIRMathRight {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let prefix = self.prefix.to_code_string(ref_pool, symbol_table);

        let init = match &self.math {
            MIRRight::Lit(lit) => lit.to_string(),
            MIRRight::VarRef(left) => left.to_code_string(ref_pool, symbol_table),
            MIRRight::CmpRef(cmp_ref) => cmp_ref.var.to_code_string(ref_pool, symbol_table),
            MIRRight::Invoke(invoke) => invoke.to_code_string(ref_pool, symbol_table),
            MIRRight::ArrEleRef(arr_ele) => arr_ele.to_code_string(ref_pool, symbol_table),
        };
        format!("{}{}", prefix, init)
    }
}

impl ToMIRString for MIRBoolLRef {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        self.var.to_code_string(ref_pool, symbol_table)
    }
}

impl ToMIRString for MIRCmpRight {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRCmpRight::MathL(mathl) => mathl.to_code_string(ref_pool, symbol_table),
            MIRCmpRight::Lit(lit) => lit.to_string(),
            MIRCmpRight::CmpL(cmp) => cmp.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRCmpExpr {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let first = self.first.to_code_string(ref_pool, symbol_table);
        let oper = self.oper.to_string();
        let right = self.second.to_code_string(ref_pool, symbol_table);
        format!("{} {} {}", first, oper, right)
    }
}

impl ToMIRString for MIRBoolCmpAssign {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let left = self.left.to_code_string(ref_pool, symbol_table);
        let right = self.right.to_code_string(ref_pool, symbol_table);
        if self.is_def {
            format!("{} : {}", left, right)
        } else {
            format!("{} = {}", left, right)
        }
    }
}

impl ToMIRString for MIRBoolCopyAssign {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let left = self.left.var.to_code_string(ref_pool, symbol_table);
        let right = self.right.to_code_string(ref_pool, symbol_table);
        if self.is_def {
            format!("{} : {}", left, right)
        } else {
            format!("{} = {}", left, right)
        }
    }
}

impl ToMIRString for MIRBool {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRBool::Cmp(cmp) => cmp.to_code_string(ref_pool, symbol_table),
            MIRBool::BoolRef(bool) => bool.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRCondJump {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let cmp = self.cond.to_code_string(ref_pool, symbol_table);
        if self.true_exit.is_some() {
            format!(
                "jump if {} -> {} or {} ",
                cmp,
                self.true_exit.as_ref().unwrap().to_code_string(ref_pool, symbol_table),
                self.false_exit.to_code_string(ref_pool, symbol_table)
            )
        } else {
            format!("jump not {} -> {}", cmp, self.false_exit.to_code_string(ref_pool, symbol_table))
        }
    }
}

impl ToMIRString for MIRJump {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        format!("jump -> {}", self.label.to_code_string(ref_pool, symbol_table))
    }
}

impl ToMIRString for MIRStmt {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRStmt::MathVarCopyAssign(assign) => {
                return if assign.is_def {
                    let right = match &assign.right {
                        None => {
                            "uninit".to_string()
                        }
                        Some(right) => {
                            right.to_code_string(ref_pool,symbol_table)
                        }
                    };
                    format!(
                        "{} : {} ",
                        assign.left.to_code_string(ref_pool, symbol_table),
                        right
                    )
                } else {
                    format!(
                        "{} = {} ",
                        assign.left.to_code_string(ref_pool, symbol_table),
                        assign.right.as_ref().unwrap().to_code_string(ref_pool, symbol_table)
                    )
                };
            }
            MIRStmt::ArrayEleAssign(assign) => {
                format!(
                    "{} = {} ",
                    assign.left.to_code_string(ref_pool, symbol_table),
                    assign.right.to_code_string(ref_pool, symbol_table)
                )
            }
            MIRStmt::MathVarComputeAssign(assign) => {
                return if assign.is_def {
                    format!(
                        "{} : {} {} {}",
                        assign.left.to_code_string(ref_pool, symbol_table),
                        assign.first.to_code_string(ref_pool, symbol_table),
                        assign.oper.to_string(),
                        assign.second.to_code_string(ref_pool, symbol_table)
                    )
                } else {
                    format!(
                        "{} = {} {} {}",
                        assign.left.to_code_string(ref_pool, symbol_table),
                        assign.first.to_code_string(ref_pool, symbol_table),
                        assign.oper.to_string(),
                        assign.second.to_code_string(ref_pool, symbol_table)
                    )
                };
            }
            MIRStmt::BoolCmpAssign(cmp) => {
                return cmp.to_code_string(ref_pool, symbol_table);
            }
            MIRStmt::ArrayDef(def) => {
                let mut init = "{".to_string();
                for single in &def.init {
                    init.push_str(&format!("(${},{})", single.0, single.1.to_code_string(ref_pool, symbol_table)));
                }
                init.push_str("}");
                let def = def.array_ref.to_code_string(ref_pool, symbol_table);
                return format!("{} : {}", def, init);
            }
            MIRStmt::MathCvt(cvt) => {
                return if cvt.is_def {
                    format!(
                        "{} cvt: {} ",
                        cvt.left.to_code_string(ref_pool, symbol_table),
                        cvt.right.to_code_string(ref_pool, symbol_table)
                    )
                } else {
                    format!(
                        "{} cvt= {} ",
                        cvt.left.to_code_string(ref_pool, symbol_table),
                        cvt.right.to_code_string(ref_pool, symbol_table)
                    )
                };
            }
            MIRStmt::CondJump(jump) => {
                return jump.to_code_string(ref_pool, symbol_table);
            }
            MIRStmt::Jump(jump) => {
                return jump.to_code_string(ref_pool, symbol_table);
            }
            MIRStmt::Invoke(invoke) => {
                return invoke.to_code_string(ref_pool, symbol_table);
            }
            MIRStmt::Label(label) => {
                return label.to_code_string(ref_pool, symbol_table).apply(|str| format!("{}:", str));
            }
            MIRStmt::Return(ret) => {
                return if ret.value.is_some() {
                    format!("return {}", ret.value.as_ref().unwrap().to_code_string(ref_pool, symbol_table))
                } else {
                    format!("return")
                };
            }
            MIRStmt::Sync(syncs) => {
                let mut str = "sync(".to_string();
                for (index, sync) in syncs.iter().enumerate() {
                    if index != syncs.len() - 1 {
                        str.push_str(&sync.to_code_string(ref_pool, symbol_table));
                        str.push(',')
                    } else {
                        str.push_str(&sync.to_code_string(ref_pool, symbol_table));
                    }
                }
                str.push(')');
                return str;
            }
            MIRStmt::Clear(clears) => {
                let mut str = "clear(".to_string();
                for (index, sync) in clears.iter().enumerate() {
                    if index != clears.len() - 1 {
                        str.push_str(&sync.to_code_string(ref_pool, symbol_table));
                        str.push(',')
                    } else {
                        str.push_str(&sync.to_code_string(ref_pool, symbol_table));
                    }
                }
                str.push(')');
                return str;
            }
            MIRStmt::Drop(drop) => {
                let name = drop.to_code_string(ref_pool, symbol_table);
                return format!("drop {}", name);
            }
        }
    }
}

impl ToMIRString for MIRBlock {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let label_name = self.name_label.to_code_string(ref_pool, symbol_table);
        let mut codes = label_name.add(":\n");
        let iter = self.stmts.node_iter();
        for stmt in  iter{
            codes.push_str("\t");
            let stmt = &stmt.borrow().value;
            codes.push_str(&stmt.to_code_string(ref_pool, symbol_table));
            codes.push_str("\n");
        }
        codes
    }
}

impl ToMIRString for MIRTypedParam {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        match self {
            MIRTypedParam::Var(var) => var.to_code_string(ref_pool, symbol_table),
            MIRTypedParam::Array(array) => array.to_code_string(ref_pool, symbol_table),
        }
    }
}

impl ToMIRString for MIRFun {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut params = "".to_string();
        for (index, param) in self.params.iter().enumerate() {
            params.push_str(&param.to_code_string(ref_pool, symbol_table));
            if index != self.params.len() - 1 {
                params.push_str(",")
            }
        }
        let mut all_blocks = format!("fn @{}({})", self.name.to_code_string(ref_pool, symbol_table), params);
        if self.ret_type.is_some() {
            all_blocks.push_str(" -> ");
            all_blocks.push_str(&self.ret_type.as_ref().unwrap().to_string());
            all_blocks.push_str("\n");
        } else {
            all_blocks.push_str("\n");
        }
        for block in &self.blocks {
            let codes = block.to_code_string(ref_pool, symbol_table);
            all_blocks.push_str(&codes);
        }
        all_blocks
    }
}

impl ToMIRString for CFG{
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut alls = String::new();
        for def in &self.global_defs{
            let ret = def.to_code_string(ref_pool,symbol_table);
            alls.push_str(&ret);
            alls.push_str("\n");
        }
        for fun in &self.runnable{
            alls.push_str(&fun.to_code_string(ref_pool,symbol_table));
            alls.push_str("\n");
        }
        alls
    }
}

impl ToMIRString for CFGFun{
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut params = "".to_string();
        for (index, param) in self.params.iter().enumerate() {
            params.push_str(&param.to_code_string(ref_pool, symbol_table));
            if index != self.params.len() - 1 {
                params.push_str(",")
            }
        }
        let mut all_blocks = format!("fn @{}({})", self.get_name().to_code_string(ref_pool, symbol_table), params);
        if self.ret_type != MIRType::Void {
            all_blocks.push_str(" -> ");
            all_blocks.push_str(&self.ret_type.to_string());
            all_blocks.push_str("\n");
        } else {
            all_blocks.push_str("\n");
        }
        for block in self.linear_blocks.node_iter() {
            let block_node_ref = block.borrow();
            let block_ref = &(*block_node_ref).value;
            let codes = block_ref.to_code_string(ref_pool,symbol_table);
            all_blocks.push_str(&codes);
            all_blocks.push_str("\n")
        }
        all_blocks
    }
}

impl ToMIRString for CFGBasicBlock {
    fn to_code_string(&self, ref_pool: &mut MIRRefPool, symbol_table: &mut SymbolTable) -> String {
        let mut label_name = self.block_label.to_code_string(ref_pool, symbol_table);
        let node_id = match &self.self_ref {
            None => {
                panic!("NEVER HERE")
            }
            Some(ele) => {
                ele.clone()
            }
        };
        label_name.push_str(&format!(" #{},[",node_id));
        for (index,from_block) in self.from_blocks.iter().enumerate() {
            label_name.push_str(&format!("#{}",from_block));
            if index != self.from_blocks.len() -1 {
                label_name.push_str(",")
            }
        }
        label_name.push_str("],[");
        for (index,from_block) in self.to_blocks.iter().enumerate() {
            label_name.push_str(&format!("#{}",from_block));
            if index != self.to_blocks.len() -1 {
                label_name.push_str(",")
            }
        }
        label_name.push_str("]");
        let mut codes = label_name.add(":\n");
        let iter = self.stmts.node_iter();
        for stmt in  iter{
            codes.push_str("\t");
            let stmt = &stmt.borrow().value;
            codes.push_str(&stmt.to_code_string(ref_pool, symbol_table));
            codes.push_str("\n");
        }
        codes
    }
}

impl ToString for MIRBoolOper {
    fn to_string(&self) -> String {
        match self {
            MIRBoolOper::Equal => "==",
            MIRBoolOper::NotEqual => "!=",
            MIRBoolOper::Less => "<",
            MIRBoolOper::Greater => ">",
            MIRBoolOper::LessEqual => "<=",
            MIRBoolOper::GreaterEqual => ">=",
            MIRBoolOper::And => "&&",
            MIRBoolOper::Or => "||",
        }
        .to_string()
    }
}

impl ToString for MIRMathOper {
    fn to_string(&self) -> String {
        match self {
            MIRMathOper::Add => "+".to_string(),
            MIRMathOper::Sub => "-".to_string(),
            MIRMathOper::Mul => "*".to_string(),
            MIRMathOper::Div => "/".to_string(),
            MIRMathOper::Rem => "%".to_string(),
        }
    }
}

