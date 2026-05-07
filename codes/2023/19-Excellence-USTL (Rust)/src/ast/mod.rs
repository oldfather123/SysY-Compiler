pub mod check_translate;
pub mod table;
pub mod to_hir;
pub mod to_string;

pub mod filter;

use crate::lexer::symbol::Symbol;
use crate::lexer::LiteralKind;
use crate::span::Span;


pub type MissOrContain<T> = Option<T>;


pub enum AstUnit {
    ExternalFn(ExternalFun),
    FnDefine(FnDefine),
    VarDefine(VarDefine),
    ArrayDefine(ArrayDefine),
    RowDefines(RowDefines),
}

pub struct AST {
    units: Vec<AstUnit>,
}

impl AST {
    pub(crate) fn push(&mut self, unit: AstUnit) {
        self.units.push(unit)
    }
    pub(crate) fn new() -> AST {
        AST { units: vec![] }
    }
    pub fn get_units(self) -> Vec<AstUnit> {
        self.units
    }
    pub fn combine(&mut self, mut ast: AST) {
        self.units.append(&mut ast.units)
    }
}

#[derive(Debug, Clone, Eq, PartialEq, Hash)]
pub enum BasicType {
    Void,
    Int,
    Float,
}

#[derive(Debug, Clone)]
pub struct ArrayType {
    pub basic_type: BasicType,
    pub dimensions: Vec<AddExpr>,
}

#[derive(Debug, Clone)]
pub enum Type {
    Basic(BasicType),
    Array(ArrayType),
}

#[derive(Debug, Clone)]
pub enum MathOpe {
    /// +
    Add,
    /// -
    Sub,
    /// *
    Mul,
    /// /
    Div,
    /// %
    Ram,
}

#[derive(Debug, Clone)]
pub enum RelOpe {
    /// "<"
    Less,
    /// "<="
    LessOrEqual,
    /// ">"
    Greater,
    /// ">="
    GreaterOrEqual,
    /// "=="
    IsEqual,
    /// "!="
    NotEqual,
}


pub type ValueAble = AddExpr;

#[derive(Debug, Clone, Eq, PartialEq, Hash)]
pub struct Ident {
    pub sym: Symbol,
    pub span: Span,
}

impl Ident {
    pub(crate) fn new(sym: Symbol, span: Span) -> Ident {
        Ident { sym, span }
    }
}


pub struct VarDefine {
    pub is_const: bool,
    pub define_type: BasicType,
    pub ident: Ident,
    pub with_value: Option<ValueAble>,
}


pub struct RowDefines {
    pub is_const: bool,
    pub define_type: BasicType,
    pub defs: Vec<IdentWithInit>,
}

pub enum IdentWithInit {
    Var {
        ident: Ident,
        init: Option<AddExpr>,
    },
    Array {
        ident: Ident,
        dimensions: Vec<AddExpr>,
        init: Option<ArrayInit>,
    },
}


pub struct ArrayDefine {
    pub is_const: bool,
    pub define_type: BasicType,
    pub ident: Ident,
    pub dimensions: Vec<AddExpr>,
    pub init: Option<ArrayInit>,
}

/// # 禁止直接使用 Array::Expr 作为ArrayDefine的 init

#[derive(Debug, Clone)]
pub enum ArrayInit {
    /// "{ 1, 2 , 3, {} }"
    Brace(Vec<ArrayInit>),
    /// 10
    Expr(AddExpr),
    /// just {}
    None,
}

impl ArrayInit {
    pub fn new(expr: AddExpr) -> ArrayInit {
        ArrayInit::Expr(expr)
    }
}


pub struct FnDefine {
    pub name: Ident,
    pub ret_type: BasicType,
    pub params: MissOrContain<FnParams>,
    pub stmts: Stmts,
}

impl FnDefine {
    pub fn new(ident: Ident) -> FnDefine {
        FnDefine {
            name: ident,
            ret_type: BasicType::Int,
            params: None,
            stmts: vec![],
        }
    }
}

pub struct ExternalFun {
    pub name: Ident,
    pub ret: BasicType,
    pub params: MissOrContain<FnParams>,
}


#[derive(Debug, Clone)]
pub struct FnParams {
    pub type_idents: Vec<(Type, Ident)>,
}

impl FnParams {
    pub fn new() -> Self {
        FnParams {
            type_idents: vec![],
        }
    }
    pub fn add(&mut self, par_type: Type, par_ident: Ident) {
        self.type_idents.push((par_type, par_ident))
    }
}

#[derive(Debug, Clone)]
pub enum LVarRef {
    /// a var or val ref
    VarRef(Ident),
    // 这里不应该再是Token，丢失了已知信息导致后面的步骤复杂了
    /// reference a value from an array
    ArrayRef(ArrayRef),
}


#[derive(Debug, Clone)]
pub struct Assign {
    pub from: AddExpr,
    pub to: LVarRef,
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    /// "+"
    Positive,
    /// "-"
    Negative,
    /// "!"
    Bang,
    /// " "
    Space,
}


#[derive(Debug, Clone)]
pub struct ArrayRef {
    pub array_name: Ident,
    pub pos: Vec<AddExpr>, // sysy 只支持32位而且是有符号的
}

#[derive(Debug, Clone)]
pub struct Literal {
    pub sym: Symbol,
    pub kind: LiteralKind,
    pub span: Span,
}

#[derive(Debug, Clone)]
pub enum UnaryExp {
    /// a literal
    Lit(Vec<UnaryOp>, Literal),
    /// a var or val ref
    VarRef(Vec<UnaryOp>, Ident),
    /// reference a value from an array
    ArrayRef(Vec<UnaryOp>, ArrayRef),
    /// a function invoke
    Invoke(Vec<UnaryOp>, Invoke),
    /// (AddExpr)
    Paren(Vec<UnaryOp>, Box<MathExpr>),
    /// Nothing 专用于数组的省略第一维度
    None,
}

impl UnaryExp {
    pub fn into_left_val_ref(self) -> Result<LVarRef, ()> {
        match self {
            UnaryExp::VarRef(_, var) => Ok(LVarRef::VarRef(var)),
            UnaryExp::ArrayRef(_, array_ref) => Ok(LVarRef::ArrayRef(array_ref)),
            _ => Err(()),
        }
    }
}

// 注意sysy语言之中，不支持RelExp向AddExpr赋值
// 这意味着布尔表达式不支持给变量赋值

#[derive(Debug, Clone)]
pub enum AddExpr {
    Unary(UnaryExp),
    Expr(Box<AddExpr>, MathOpe, Box<AddExpr>),
}

pub type MathExpr = AddExpr;

impl AddExpr {
    fn into_left_val_ref(self) -> Result<LVarRef, ()> {
        match self {
            AddExpr::Unary(unary) => unary.into_left_val_ref(),
            _ => {
                return Err(());
            }
        }
    }
}

/// RelExp&&EqExp
/// 翻译的时候注意优先级，the priority of < , <= , > , >= is higher than == , !=
#[derive(Debug)]
pub enum RelExpr {
    Math(MathExpr),
    Rel(Box<RelExpr>, RelOpe, Box<RelExpr>),
}

/// LOrExp&&LAndExp
/// 翻译的时候注意优先级，the priority of AND is higher than OR
#[derive(Debug)]
pub enum BoolExprs {
    And(Box<BoolExprs>, Box<BoolExprs>),
    Or(Box<BoolExprs>, Box<BoolExprs>),
    RelExpr(RelExpr),
}

impl BoolExprs {
    pub fn into_left_val_ref(self) -> Result<LVarRef, ()> {
        match self {
            BoolExprs::RelExpr(expr) => expr.into_left_val_ref(),
            _ => {
                return Err(());
            }
        }
    }
}

impl RelExpr {
    fn into_left_val_ref(self) -> Result<LVarRef, ()> {
        match self {
            RelExpr::Math(expr) => expr.into_left_val_ref(),
            RelExpr::Rel(_, _, _) => {
                return Err(());
            }
        }
    }
}


pub type BoolCondition = BoolExprs;


#[derive(Debug, Clone)]
pub struct Invoke {
    pub fun_name: Ident,
    pub params: Vec<MathExpr>,
    pub line_at:usize
}


pub enum Stmt {
    IfElse(Branches),
    While(While),
    Continue,
    Break,
    Return(Return),
    Assign(Assign),
    VarDefine(VarDefine),
    ArrayDefine(ArrayDefine),
    RowDefines(RowDefines),
    Invoke(Invoke),
    Block(Stmts),
    Expr(BoolExprs),
    /// only ;
    None,
}
#[derive(Debug)]
pub struct Return {
    pub return_value: Option<MathExpr>,
}

/// 尽量不要用这种的语法，很让人头大

pub type Stmts = Vec<Stmt>;


pub struct Branches {
    pub ifs: Vec<Branch>,
}


pub struct Branch {
    /// 如果是None则是else分支，不然就是if或者else_if分支
    pub condition: Option<BoolCondition>,
    pub stats: Stmts,
}


pub struct While {
    pub condition: BoolCondition,
    pub stats: Stmts,
}
