use crate::ast::table::SymbolTable;
use crate::hir::hir::{
    HirModule, HirUnit, IRArrayDefine, IRArrayInit, IRBasicExpr, IRBasicType, IRBool, IRBoolExpr,
    IRBoolOpe, IRBoolOrVarDefine, IRBoolVar, IRBoolVarDefine, IRExprPrefix, IRExternalFun,
    IRFirstSuffix, IRFunDefine, IRInvoke, IRLValue, IRMathOpe, IRNamedArrayEle, IRNamedParam,
    IRRValue, IRRelSource, IRSecondSuffix, IRStatement, IRTypedParam, IRTypedVar,
    IRVarDefine, IRVarOrArrayDefine,
};
use crate::util::Kotlin;

pub trait ToCodeString {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String;
}

impl ToCodeString for HirModule {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        let mut codes = String::new();
        for single in &self.units {
            let code = single.to_code(symbol_table);
            codes.push_str(&code);
            codes.push_str("\n")
        }
        codes
    }
}

impl ToCodeString for HirUnit {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        match self {
            HirUnit::GLVarDefine(def) => def.to_code(table),
            HirUnit::GLArrayDefine(array_def) => array_def.to_code(table),
            HirUnit::GlDefines(gl_def) => {
                let mut codes = String::new();
                for single in gl_def {
                    let code = single.to_code(table);
                    codes.push_str(&code);
                    codes.push_str("\n")
                }
                codes
            }
            HirUnit::FunDefine(def) => def.to_code(table),
            HirUnit::ExternalFun(def) => def.to_code(table),
        }
    }
}

impl ToString for IRBasicType {
    fn to_string(&self) -> String {
        match self {
            IRBasicType::INT => "int".to_string(),
            IRBasicType::FLOAT => "float".to_string(),
            IRBasicType::VOID => "void".to_string(),
        }
    }
}

impl ToCodeString for IRTypedVar {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        symbol_table
            .get_symbol(&self.copy_name().get_sym_ref())
            .to_string()
            .apply(|mut str| {
                str.push_str(&format!("_{}", self.get_sub()));
                str
            })
    }
}

impl ToCodeString for IRNamedParam {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRNamedParam::IRRValue(_suffix, irr) => {
                let irr = irr.to_code(symbol_table);
                format!("{}", irr)
            }
            IRNamedParam::IRNamedArray(name_array) => symbol_table
                .get_symbol(&name_array.get_name().get_sym_ref())
                .to_string()
                .apply_once(|mut str| {
                    str.push_str(" /* collapsed */");
                    str
                }),
        }
    }
}

impl ToCodeString for IRInvoke {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        let mut invoke = symbol_table
            .get_symbol(&self.fun_name.get_sym_ref())
            .to_string();
        let vars = &self.variables;
        invoke.push_str("(");

        for (which, var) in vars.iter().enumerate() {
            invoke.push_str(&var.to_code(symbol_table));
            if which + 1 != vars.len() {
                invoke.push_str(",");
            }
        }
        invoke.push_str(")");
        invoke
    }
}

impl ToCodeString for IRNamedArrayEle {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        let dims = self.get_dims();
        let mut named_array = symbol_table
            .get_symbol(&self.get_name().get_sym_ref())
            .to_string();
        for single in dims {
            let single = single.to_code(symbol_table);
            named_array.push_str(&format!("[{}]", &single))
        }
        named_array
    }
}

impl ToCodeString for IRBasicExpr {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRBasicExpr::Literal(lit) => lit.to_string(),
            IRBasicExpr::Var(var) => var.to_code(symbol_table),
            IRBasicExpr::ArrayEleRef(array_ref) => array_ref.to_code(symbol_table),
        }
    }
}

impl ToString for IRMathOpe {
    fn to_string(&self) -> String {
        match self {
            IRMathOpe::Add => "+".to_string(),
            IRMathOpe::Sub => "-".to_string(),
            IRMathOpe::Mul => "*".to_string(),
            IRMathOpe::Div => "/".to_string(),
            IRMathOpe::Rem => "%".to_string(),
        }
    }
}

impl ToString for IRExprPrefix {
    fn to_string(&self) -> String {
        match self.get_first() {
            IRFirstSuffix::None => match self.get_second() {
                IRSecondSuffix::None => "".to_string(),
                IRSecondSuffix::EvenBang => "!!".to_string(),
                IRSecondSuffix::OddBang => "!".to_string(),
            },
            IRFirstSuffix::Negative => match self.get_second() {
                IRSecondSuffix::None => "-".to_string(),
                IRSecondSuffix::EvenBang => "- !!".to_string(),
                IRSecondSuffix::OddBang => "- !".to_string(),
            },
        }
    }
}

impl ToString for IRBoolOpe {
    fn to_string(&self) -> String {
        match self {
            IRBoolOpe::Less => "<".to_string(),
            IRBoolOpe::Greater => ">".to_string(),
            IRBoolOpe::GreaterOrEqual => ">=".to_string(),
            IRBoolOpe::LessOrEqual => "<=".to_string(),
            IRBoolOpe::Equal => "==".to_string(),
            IRBoolOpe::NotEqual => "!=".to_string(),
            IRBoolOpe::And => "&&".to_string(),
            IRBoolOpe::Or => "&&".to_string(),
        }
    }
}

impl ToCodeString for IRBoolVar {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        symbol_table
            .get_symbol(&self.get_name().get_sym_ref())
            .to_string()
    }
}

impl ToCodeString for IRRelSource {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRRelSource::Literal(lit) => lit.to_string(),
            IRRelSource::Var(var) => var.to_code(symbol_table),
            IRRelSource::ArrayEleRef(array_ref) => array_ref.to_code(symbol_table),
            IRRelSource::BasicBool(basic) => basic.to_code(symbol_table),
        }
    }
}

impl ToCodeString for IRBool {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRBool::Basic(expr) => expr.to_code(symbol_table),
            IRBool::Rel(a, b, c) => {
                let a = a.to_code(symbol_table);
                let c = c.to_code(symbol_table);
                let b = b.to_string();
                format!("{} {} {}", a, b, c)
            }
        }
    }
}

impl ToCodeString for IRRValue {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRRValue::Invoke(suffix, invoke) => {
                let suffix = suffix.to_string();
                let invoke = invoke.to_code(symbol_table);
                format!("{}{}", suffix, invoke)
            }
            IRRValue::Basic(suffix, basic) => {
                let suffix = suffix.to_string();
                let basic = basic.to_code(symbol_table);
                format!("{}{}", suffix, basic)
            }
            IRRValue::Math(suffix, a, b, c) => {
                format!(
                    "{}({} {} {})",
                    suffix.to_string(),
                    a.to_code(symbol_table),
                    b.to_string(),
                    c.to_code(symbol_table)
                )
            }
            IRRValue::Bool(a) => a.to_code(symbol_table),
        }
    }
}

fn var_init_to_string(init: &Option<IRRValue>, symbol_table: &mut SymbolTable) -> String {
    match init {
        None => "0 /* auto init */".to_string(),
        Some(init) => init.to_code(symbol_table),
    }
}

fn array_init_to_string(_init: &IRArrayInit, _symbol_table: &mut SymbolTable) -> String {
    "/* not support*/".to_string()
}

impl ToCodeString for IRVarDefine {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        let is_const = self.is_const();
        let ir_type = self.get_type();
        let type_str = ir_type.to_string();
        let ir_name = self.get_name();
        let name_str = table
            .get_symbol(&ir_name.get_sym_ref())
            .to_string()
            .apply(|mut str| {
                str.push_str(&format!("_{}", ir_name.get_sub()));
                str
            });
        let init = self.get_init();
        if !is_const {
            format!(
                "{} {} = {};",
                type_str,
                name_str,
                var_init_to_string(init, table)
            )
        } else {
            format!(
                "const {} {} = {};",
                type_str,
                name_str,
                var_init_to_string(init, table)
            )
        }
    }
}

impl ToCodeString for IRArrayDefine {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        let is_const = self.is_const();
        let ir_type = self.get_type();
        let type_str = ir_type.to_string();
        let name_str = table.get_symbol(&self.get_name().get_sym_ref()).to_string();
        let init = self.get_init();
        if !is_const {
            format!(
                "{} {} = {};",
                type_str,
                name_str,
                array_init_to_string(init, table)
            )
        } else {
            format!(
                "const {} {} = {};",
                type_str,
                name_str,
                array_init_to_string(init, table)
            )
        }
    }
}

impl ToCodeString for IRVarOrArrayDefine {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        match self {
            IRVarOrArrayDefine::Var(var) => var.to_code(table),
            IRVarOrArrayDefine::Array(array) => array.to_code(table),
        }
    }
}

impl ToCodeString for IRFunDefine {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        let mut fn_def = String::new();
        let ret_type = self.get_ret_type().to_string();
        let fn_name = table.get_symbol(&self.get_name().get_sym_ref()).to_string();
        fn_def.push_str(&ret_type);
        fn_def.push_str(" ");
        fn_def.push_str(&fn_name);
        fn_def.push_str("(");
        let params = self.copy_params();
        for (which, single_param) in params.iter().enumerate() {
            match single_param {
                IRTypedParam::Var(var) => {
                    let name = var.to_code(table);
                    let param_type = var.get_type().to_string();
                    fn_def.push_str(&format!("{} {}", param_type, name))
                }
                IRTypedParam::Array(array) => {
                    let param_type = array.get_type().to_string();
                    let name = table
                        .get_symbol(&array.get_irname().get_sym_ref())
                        .to_string();
                    fn_def.push_str(&format!("{} {}[]", param_type, name))
                }
            }
            if which + 1 != params.len() {
                fn_def.push_str(",");
            } else {
                fn_def.push_str(")")
            }
        }
        if params.len() == 0 {
            fn_def.push_str(")")
        }
        fn_def.push_str("{\n");
        let stmts = self.get_stmts();
        for stmt in stmts {
            fn_def.push_str("\t");
            fn_def.push_str(&stmt.to_code(table));
            fn_def.push_str("\n");
        }
        fn_def.push_str("}");
        fn_def
    }
}

impl ToCodeString for IRLValue {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRLValue::Var(var) => symbol_table
                .get_symbol(&var.copy_name().get_sym_ref())
                .to_string()
                .apply(|mut str| {
                    str.push_str(&format!("_{}", var.get_sub()));
                    str
                }),
            IRLValue::ArrayEle(array) => symbol_table
                .get_symbol(&array.get_name().get_sym_ref())
                .to_string()
                .apply_mut(|mut str| {
                    for single in array.get_dims() {
                        let single = single.to_code(symbol_table);
                        str.push_str(&format!("[{}]", &single))
                    }
                    str
                }),
        }
    }
}

impl ToCodeString for IRBoolVarDefine {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        format!(
            "bool {} = {}",
            symbol_table.get_symbol(&self.get_name().get_sym_ref()),
            self.get_init().to_code(symbol_table)
        )
    }
}

impl ToCodeString for IRBoolOrVarDefine {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRBoolOrVarDefine::VarDef(var_def) => var_def.to_code(symbol_table),
            IRBoolOrVarDefine::BoolDef(bool_def) => bool_def.to_code(symbol_table),
        }
    }
}

impl ToCodeString for IRBoolExpr {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRBoolExpr::Bool(vars, b) => {
                let mut codes = "/* start of bool enter */\n\t".to_string();
                for var in vars {
                    codes.push_str(&var.to_code(symbol_table));
                    codes.push_str("\n\t")
                }
                codes.push_str(&"/* end of bool enter */\n\t");
                codes.push_str(&b.to_code(symbol_table));
                codes
            }
            IRBoolExpr::And(ands) => {
                let mut codes = "".to_string();
                for (index, var) in ands.iter().enumerate() {
                    codes.push_str(&var.to_code(symbol_table));
                    if index != ands.len() - 1 {
                        codes.push_str(" && \n\t")
                    }
                }
                codes
            }
            IRBoolExpr::Or(ors) => {
                let mut codes = "".to_string();
                for (index, var) in ors.iter().enumerate() {
                    codes.push_str(&var.to_code(symbol_table));
                    if index != ors.len() - 1 {
                        codes.push_str(" || \n\t")
                    }
                }
                codes
            }
        }
    }
}

impl ToCodeString for IRStatement {
    fn to_code(&self, symbol_table: &mut SymbolTable) -> String {
        match self {
            IRStatement::VarDef(def) => def.to_code(symbol_table),
            IRStatement::ArrDef(array) => array.to_code(symbol_table),
            IRStatement::Assign(assign) => {
                let left = &assign.left.to_code(symbol_table);
                let right = &assign.right.to_code(symbol_table);
                format!("{} = {};", left, right)
            }
            IRStatement::Branches(branches) => {
                let first_if = &branches.first_if;
                let cond = &first_if.condition;
                let cond = cond.to_code(symbol_table);
                let mut branches_str = format!("if ({}) {{\n\t", cond);
                let stmts = &first_if.statements;
                for stmt in stmts {
                    branches_str.push_str(&stmt.to_code(symbol_table));
                    branches_str.push_str("\n\t");
                }
                branches_str.push_str("}");
                let other_branches = &branches.elif;
                for single_then in other_branches {
                    let cond = single_then.condition.to_code(symbol_table);
                    let mut then_branches_str = format!("else if ({}) {{\n\t", cond);
                    let stmts = &single_then.statements;
                    for stmt in stmts {
                        then_branches_str.push_str(&stmt.to_code(symbol_table));
                        then_branches_str.push_str("\n\t");
                    }
                    then_branches_str.push_str("}");
                    branches_str.push_str(&then_branches_str);
                }
                let else_branches = &branches.last_else;
                if else_branches.is_some() {
                    let stmts = else_branches.as_ref().unwrap();
                    let mut else_branch = format!("else {{\n\t");
                    for stmt in stmts {
                        else_branch.push_str(&stmt.to_code(symbol_table));
                        else_branch.push_str("\n\t");
                    }
                    else_branch.push_str("}");
                    branches_str.push_str(&else_branch);
                }
                branches_str
            }
            IRStatement::FunInvoke(invoke) => invoke
                .to_code(symbol_table)
                .apply_once(|str| format!("{};", str)),
            IRStatement::EmptyExpr(_) => {
                format!("empty expr")
            }
            IRStatement::Block(block) => {
                let mut fn_def = "".to_string();
                fn_def.push_str("{\n");
                for stmt in block {
                    fn_def.push_str("\t");
                    fn_def.push_str(&stmt.to_code(symbol_table));
                    fn_def.push_str("\n");
                }
                fn_def.push_str("\t}");
                fn_def
            }
            IRStatement::While(while_loop) => {
                let cond = &while_loop.condition;
                let cond = cond.to_code(symbol_table);
                let mut while_loop_str = format!("while({})\n\t{{\n\t", cond);
                for single in &while_loop.statements {
                    while_loop_str.push_str(&single.to_code(symbol_table));
                    while_loop_str.push_str("\n\t");
                }
                while_loop_str.push_str("}");
                while_loop_str
            }
            IRStatement::Continue => {
                return "continue;".to_string();
            }
            IRStatement::Break => {
                return "break;".to_string();
            }
            IRStatement::Return(ret) => match ret {
                None => "return".to_string(),
                Some(ir_expr) => {
                    let expr = ir_expr.to_code(symbol_table);
                    format!("return {};", expr)
                }
            },
            IRStatement::BoolDef(def) => def.to_code(symbol_table),
            IRStatement::BoolAssign(_) => {
                // 未经过优化的HIR不可能产生BoolAssign
                todo!("NEVER HERE")
            }
        }
    }
}

impl ToCodeString for IRExternalFun {
    fn to_code(&self, table: &mut SymbolTable) -> String {
        "extern ".to_string().apply_once(|mut str| {
            str.push_str(&self.return_type.to_string());
            str.push_str(" ");
            str.push_str(&table.get_symbol(&self.name.get_sym_ref()).to_string());
            let params = &self.params;
            str.push_str("(");
            for (which, single_param) in params.iter().enumerate() {
                match single_param {
                    IRTypedParam::Var(var) => {
                        let name = table
                            .get_symbol(&var.copy_name().get_sym_ref())
                            .to_string();
                        let param_type = var.get_type().to_string();
                        str.push_str(&format!("{} {}", param_type, name))
                    }
                    IRTypedParam::Array(array) => {
                        let param_type = array.get_type().to_string();
                        let name = table
                            .get_symbol(&array.get_irname().get_sym_ref())
                            .to_string();
                        str.push_str(&format!("{} {}", param_type, name))
                    }
                }
                if which + 1 != params.len() {
                    str.push_str(",");
                } else {
                    str.push_str(");")
                }
            }
            if params.len() == 0 {
                str.push_str(");");
            }
            str
        })
    }
}
