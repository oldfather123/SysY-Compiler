//! AST Module
//! Sysy2022 syntax
//! 翻譯單元      CompUnit → [ CompUnit ] ( Decl | FuncDef )
//! 聲明          Decl → ConstDecl | VarDecl
//! 常量聲明      ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
//! 基本類型      BType → 'int' | 'float'
//! 常數定義      ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
//! 常數初值      ConstInitVal → ConstExp
//!                              | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
//! 变量声明      VarDecl → BType VarDef { ',' VarDef } ';'
//! 變量定義      VarDef → Ident { '[' ConstExp ']' }
//!                      | Ident { '[' ConstExp ']' } '=' InitVal
//! 变量初值      InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
//! 函数定义      FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
//! 函数类型      FuncType → 'void' | 'int' | 'float'
//! 函数形参表    FuncFParams → FuncFParam { ',' FuncFParam }
//! 函数形参      FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
//! 语句块        Block → '{' { BlockItem } '}'
//! 语句块项      BlockItem → Decl | Stmt
//! 语句          Stmt → LVal '=' Exp ';' | [Exp] ';' | Block
//!                       | 'if' '( Cond ')' Stmt [ 'else' Stmt ]
//!                       | 'while' '(' Cond ')' Stmt
//!                       | 'break' ';' | 'continue' ';'
//!                       | 'return' [Exp] ';'
//! 表达式        Exp → AddExp 注：SysY 表达式是 int/float 型
//! 条件表达式    Cond → LOrExp
//! 左值表达式    LVal → Ident {'[' Exp ']'}
//! 基本表达式    PrimaryExp → '(' Exp ')' | LVal | Number
//! 数值          Number → IntConst | floatConst
//! 一元表达式    UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')'
//!                       | UnaryOp UnaryExp
//! 单目运算符    UnaryOp → '+' | '−' | '!' 注：'!'仅出现在条件表达式中
//! 函数实参表    FuncRParams → Exp { ',' Exp }
//! 乘除模表达式  MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
//! 加减表达式    AddExp → MulExp | AddExp ('+' | '−') MulExp
//! 关系表达式    RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
//! 相等性表达式  EqExp → RelExp | EqExp ('==' | '!=') RelExp
//! 逻辑与表达式  LAndExp → EqExp | LAndExp '&&' EqExp
//! 逻辑或表达式  LOrExp → LAndExp | LOrExp '||' LAndExp
//! 常量表达式    ConstExp → AddExp 注：使用的 Ident 必须是常量
//!
use crate::prelude::*;
// Re-export refmap types for external use
pub use crate::utils::refmap::{CompUnitRef, DeclRef, ExprRef, StmtRef};
use strum::{Display, EnumIs, EnumIter};

// ============================================================================
// 类型系统
// ============================================================================

/// 基本类型
#[derive(Debug, Clone, Copy, PartialEq, Eq, Display, EnumIs, EnumIter)]
pub enum BaseType {
    #[strum(serialize = "int")]
    Int,
    #[strum(serialize = "float")]
    Float,
    #[strum(serialize = "void")]
    Void,
}

/// 完整类型 -- 数组
#[derive(Debug, Clone, PartialEq, Eq, derive_new::new)]
pub struct TypeNode {
    pub base: BaseType,
    /// 数组维度 -- 非数组Vec![] & Vec![None] 是未指定大小的维度
    pub dimensions: Vec<Option<i32>>,
}

// ============================================================================
// 编译单元
// ============================================================================

/// 编译单元 - AST根节点
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct CompUnit {
    pub items: Vec<DeclRef>,
}

// ============================================================================
// 声明系统
// ============================================================================

/// 声明种类
#[derive(Debug, Clone, PartialEq)]
pub enum DeclKind {
    Const(ConstDecl),   // 常量
    Var(VarDecl),       // 变量
    Function(FuncDecl), // 函数
}

/// 声明节点
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct Decl {
    pub kind: DeclKind,
}

/// 常量声明
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct ConstDecl {
    pub base_type: BaseType,
    pub items: Vec<ConstDefItem>,
}

/// 常量定义项
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct ConstDefItem {
    pub name: String,
    pub ty: TypeNode,
    pub init: ExprRef, // 常量初始化表达式
}

/// 变量声明  
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct VarDecl {
    pub base_type: BaseType,
    pub items: Vec<VarDefItem>,
}

/// 变量定义项
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct VarDefItem {
    pub name: String,
    pub ty: TypeNode,
    pub init: Option<ExprRef>, // 可选的初始化表达式
}

/// 函数声明
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct FuncDecl {
    pub return_type: BaseType,
    pub name: String,
    pub params: Vec<FuncParam>,
    pub body: Option<StmtRef>, // None表示函数声明，Some表示函数定义
}

/// 函数参数
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct FuncParam {
    pub name: String,
    pub ty: TypeNode,
}

// ============================================================================
// 语句系统
// ============================================================================

/// 语句种类
#[derive(Debug, Clone, PartialEq)]
pub enum StmtKind {
    /// 空语句: ;
    Empty,
    /// 表达式语句: expr;
    Expr(ExprRef),
    /// 赋值语句: lval = expr;
    Assign { lval: ExprRef, rval: ExprRef },
    /// 语句块: { stmt1; stmt2; ... }
    Block(BlockStmt),
    /// if语句: if (cond) then_stmt [else else_stmt]
    If {
        cond: ExprRef,
        then_stmt: StmtRef,
        else_stmt: Option<StmtRef>,
    },
    /// while循环: while (cond) body  
    While { cond: ExprRef, body: StmtRef },
    /// break语句
    Break,
    /// continue语句
    Continue,
    /// return语句: return [expr];
    Return(Option<ExprRef>),
    /// 声明语句: int x = 5;
    Decl(DeclRef),
}

/// 语句节点
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct Stmt {
    pub kind: StmtKind,
}

/// 语句块                             
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct BlockStmt {
    pub stmts: Vec<StmtRef>,
}

// ============================================================================
// 表达式系统
// ============================================================================

/// 二元运算符
#[derive(Debug, Clone, Copy, PartialEq, Eq, Display, EnumIs, EnumIter)]
pub enum BinOp {
    // 算术运算
    #[strum(serialize = "+")]
    Add,
    #[strum(serialize = "-")]
    Sub,
    #[strum(serialize = "*")]
    Mul,
    #[strum(serialize = "/")]
    Div,
    #[strum(serialize = "%")]
    Mod,

    // 比较运算
    #[strum(serialize = "<")]
    Lt,
    #[strum(serialize = "<=")]
    Le,
    #[strum(serialize = ">")]
    Gt,
    #[strum(serialize = ">=")]
    Ge,
    #[strum(serialize = "==")]
    Eq,
    #[strum(serialize = "!=")]
    Ne,

    // 逻辑运算
    #[strum(serialize = "&&")]
    And,
    #[strum(serialize = "||")]
    Or,
}

/// 一元运算符
#[derive(Debug, Clone, Copy, PartialEq, Eq, Display, EnumIs, EnumIter)]
pub enum UnaryOp {
    #[strum(serialize = "+")]
    Plus,
    #[strum(serialize = "-")]
    Minus,
    #[strum(serialize = "!")]
    Not,
}

/// 表达式种类
#[derive(Debug, Clone, PartialEq)]
pub enum ExprKind {
    /// 整数字面量
    Int(i32),
    /// 浮点字面量  
    Float(f32),
    /// 标识符引用
    Ident(String),
    /// 二元运算: lhs op rhs
    Binary {
        lhs: ExprRef,
        op: BinOp,
        rhs: ExprRef,
    },
    /// 一元运算: op expr
    Unary { op: UnaryOp, expr: ExprRef },
    /// 函数调用: func(args...)
    Call { func: String, args: Vec<ExprRef> },
    /// 数组访问: array[index]  
    Index { array: ExprRef, index: ExprRef },
    /// 初始化列表: {expr1, expr2, ...}
    InitList(Vec<ExprRef>),
    /// 括号表达式: (expr)
    Paren(ExprRef),
}

/// 表达式节点
#[derive(Debug, Clone, PartialEq, derive_new::new)]
pub struct Expr {
    pub kind: ExprKind,
}

// ============================================================================
// AST上下文
// ============================================================================

/// AST上下文
#[derive(Debug, Default)]
pub struct AstContext {
    pub comp_units: RefMap<CompUnitRef, CompUnit>,
    pub decls: RefMap<DeclRef, Decl>,
    pub stmts: RefMap<StmtRef, Stmt>,
    pub exprs: RefMap<ExprRef, Expr>,
}

impl AstContext {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn add_comp_unit(&mut self, comp_unit: CompUnit) -> CompUnitRef {
        self.comp_units.insert(comp_unit)
    }

    pub fn add_decl(&mut self, decl: Decl) -> DeclRef {
        self.decls.insert(decl)
    }

    pub fn add_stmt(&mut self, stmt: Stmt) -> StmtRef {
        self.stmts.insert(stmt)
    }

    pub fn add_expr(&mut self, expr: Expr) -> ExprRef {
        self.exprs.insert(expr)
    }

    pub fn get_comp_unit(&self, r: CompUnitRef) -> Option<&CompUnit> {
        self.comp_units.get(r)
    }

    pub fn get_decl(&self, r: DeclRef) -> Option<&Decl> {
        self.decls.get(r)
    }

    pub fn get_stmt(&self, r: StmtRef) -> Option<&Stmt> {
        self.stmts.get(r)
    }

    pub fn get_expr(&self, r: ExprRef) -> Option<&Expr> {
        self.exprs.get(r)
    }
}
