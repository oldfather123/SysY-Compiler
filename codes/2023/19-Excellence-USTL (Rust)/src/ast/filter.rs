#![allow(unused)]

use crate::ast::{
    AddExpr, ArrayDefine, AstUnit, FnDefine, IdentWithInit, Stmt, Stmts, Type, UnaryExp, VarDefine,
    AST,
};

pub enum FeatureDetector<'a> {
    /// 只检测全局变量
    GlVar(&'a dyn FilterFeature<VarDefine>),
    /// 只检测全局数组
    GlArray(&'a dyn FilterFeature<ArrayDefine>),
    /// 所有数组
    Array(&'a dyn FilterFeature<ArrayDefine>),
    /// 所有变量
    Var(&'a dyn FilterFeature<VarDefine>),
    /// 函数检测，这个检测不建议用于去检测表达式
    FnDefine(&'a dyn FilterFeature<FnDefine>),
}

pub struct Filter<'a> {
    filters: Vec<FeatureDetector<'a>>,
}

pub trait FilterFeature<F> {
    fn filter(&self, ast_unit: &F) -> bool;
    fn name(&self) -> String;
}

impl<'a> Filter<'a> {
    pub fn filter_stmts(&self, stmt: &Stmts) -> bool {
        let mut is_detect = false;
        for stat in stmt {
            match stat {
                Stmt::IfElse(branches) => {
                    for branch in &branches.ifs {
                        is_detect = self.filter_stmts(&branch.stats);
                    }
                }
                Stmt::While(while_loop) => {
                    is_detect = self.filter_stmts(&while_loop.stats);
                }
                Stmt::Return(expr) => {
                    continue;
                }
                Stmt::Assign(assign) => {
                    continue;
                }
                Stmt::VarDefine(define) => {
                    for feature in &self.filters {
                        match feature {
                            FeatureDetector::Var(filter) => {
                                let result = filter.filter(define);
                                if result {
                                    is_detect = true;
                                    let name = filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            _ => {
                                continue;
                            }
                        }
                    }
                }
                Stmt::RowDefines(row) => {
                    for def in &row.defs {
                        match def {
                            IdentWithInit::Var { ident, init } => {
                                for feature in &self.filters {
                                    let var_def = &VarDefine {
                                        is_const: row.is_const,
                                        define_type: row.define_type.clone(),
                                        ident: ident.clone(),
                                        with_value: init.clone(),
                                    };
                                    match feature {
                                        FeatureDetector::Var(filter) => {
                                            let result = filter.filter(var_def);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        _ => {
                                            continue;
                                        }
                                    }
                                }
                            }
                            IdentWithInit::Array {
                                ident,
                                dimensions,
                                init,
                            } => {
                                let array = ArrayDefine {
                                    is_const: row.is_const,
                                    define_type: row.define_type.clone(),
                                    ident: ident.clone(),
                                    dimensions: dimensions.clone(),
                                    init: init.clone(),
                                };
                                for feature in &self.filters {
                                    match feature {
                                        FeatureDetector::Array(filter) => {
                                            let result = filter.filter(&array);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        _ => {
                                            continue;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                Stmt::Invoke(invoke) => {
                    continue;
                }
                Stmt::Block(stats) => is_detect = self.filter_stmts(stats),
                Stmt::Expr(_) => {
                    continue;
                }
                _ => {
                    continue;
                }
            }
        }
        is_detect
    }

    pub fn filter(&self, ast: &AST) -> bool {
        let mut is_detect = false;
        for ast_unit in &ast.units {
            match ast_unit {
                AstUnit::FnDefine(fn_def) => {
                    for filter in &self.filters {
                        match filter {
                            FeatureDetector::FnDefine(fn_define_filter) => {
                                let result = fn_define_filter.filter(&fn_def);
                                if result {
                                    is_detect = true;
                                    let name = fn_define_filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            _ => {
                                continue;
                            }
                        }
                    }
                    let is_detect_new = self.filter_stmts(&fn_def.stmts);
                    if is_detect_new {
                        is_detect = is_detect_new;
                    }
                }
                AstUnit::VarDefine(var) => {
                    for feature in &self.filters {
                        match feature {
                            FeatureDetector::Var(filter) => {
                                let result = filter.filter(&var);
                                if result {
                                    is_detect = true;
                                    let name = filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            FeatureDetector::GlVar(filter) => {
                                let result = filter.filter(&var);
                                if result {
                                    is_detect = true;
                                    let name = filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            _ => {
                                continue;
                            }
                        }
                    }
                }
                AstUnit::ArrayDefine(array) => {
                    for feature in &self.filters {
                        match feature {
                            FeatureDetector::GlArray(filter) => {
                                let result = filter.filter(&array);
                                if result {
                                    is_detect = true;
                                    let name = filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            FeatureDetector::Array(filter) => {
                                let result = filter.filter(&array);
                                if result {
                                    is_detect = true;
                                    let name = filter.name();
                                    println!("{} -> {}", "检测到特征", name)
                                }
                            }
                            _ => {
                                continue;
                            }
                        }
                    }
                }
                AstUnit::RowDefines(row) => {
                    for def in &row.defs {
                        match def {
                            IdentWithInit::Var { ident, init } => {
                                for feature in &self.filters {
                                    let var_def = &VarDefine {
                                        is_const: row.is_const,
                                        define_type: row.define_type.clone(),
                                        ident: ident.clone(),
                                        with_value: init.clone(),
                                    };
                                    match feature {
                                        FeatureDetector::Var(filter) => {
                                            let result = filter.filter(var_def);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        FeatureDetector::GlVar(filter) => {
                                            let result = filter.filter(var_def);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        _ => {
                                            continue;
                                        }
                                    }
                                }
                            }
                            IdentWithInit::Array {
                                ident,
                                dimensions,
                                init,
                            } => {
                                let array = ArrayDefine {
                                    is_const: row.is_const,
                                    define_type: row.define_type.clone(),
                                    ident: ident.clone(),
                                    dimensions: dimensions.clone(),
                                    init: init.clone(),
                                };
                                for detector in &self.filters {
                                    match detector {
                                        FeatureDetector::Array(filter) => {
                                            let result = filter.filter(&array);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        FeatureDetector::GlArray(filter) => {
                                            let result = filter.filter(&array);
                                            if result {
                                                is_detect = true;
                                                let name = filter.name();
                                                println!("{} -> {}", "检测到特征", name)
                                            }
                                        }
                                        _ => {
                                            continue;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                AstUnit::ExternalFn(_) => {}
            }
        }
        return is_detect;
    }
    pub fn new() -> Self {
        Self { filters: vec![] }
    }

    pub fn register_filter(&mut self, filter: FeatureDetector<'a>) {
        self.filters.push(filter)
    }
}

pub struct FirstDimensionUnknownDetect;

pub struct FirstDimensionLit;

pub struct ArrayPass;

pub struct ConstArrayDetect;

impl FilterFeature<ArrayDefine> for FirstDimensionLit {
    fn filter(&self, ast_unit: &ArrayDefine) -> bool {
        let dimension = &ast_unit.dimensions;
        let first = dimension.first();
        match first {
            None => {
                panic!("NEVER HERE");
            }
            Some(expr) => {
                return match expr {
                    AddExpr::Unary(unary) => {
                        return match unary {
                            UnaryExp::Lit(_, ..) => true,
                            _ => false,
                        };
                    }
                    AddExpr::Expr(_, _, _) => false,
                };
            }
        }
    }

    fn name(&self) -> String {
        "第一维度数字".to_string()
    }
}

impl FilterFeature<ArrayDefine> for FirstDimensionUnknownDetect {
    fn filter(&self, ast_unit: &ArrayDefine) -> bool {
        let dimension = &ast_unit.dimensions;
        let first = dimension.first();
        match first {
            None => {
                panic!("NEVER HERE");
            }
            Some(expr) => {
                return match expr {
                    AddExpr::Unary(unary) => {
                        return match unary {
                            UnaryExp::None => true,
                            _ => false,
                        };
                    }
                    AddExpr::Expr(_, _, _) => false,
                };
            }
        }
    }

    fn name(&self) -> String {
        "数组第一维度缺失".to_string()
    }
}

impl FilterFeature<FnDefine> for ArrayPass {
    fn filter(&self, ast_unit: &FnDefine) -> bool {
        let params = &ast_unit.params;
        match params {
            None => {
                return false;
            }
            Some(params) => {
                for param in &params.type_idents {
                    let first = &param.0;
                    match first {
                        Type::Basic(_) => {}
                        Type::Array(array) => {
                            return true;
                        }
                    }
                }
            }
        }
        false
    }

    fn name(&self) -> String {
        "数组传递检测".to_string()
    }
}

impl FilterFeature<ArrayDefine> for ConstArrayDetect {
    fn filter(&self, ast_unit: &ArrayDefine) -> bool {
        ast_unit.is_const
    }

    fn name(&self) -> String {
        "Const Array Define Detect".to_string()
    }
}
