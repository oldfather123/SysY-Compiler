use crate::common::{immediate::Immediate, r#type::Type};
use crate::frontend::llvm::ssa::*;
use defaults::Defaults;
use derive_new::new;
use enum_as_inner::EnumAsInner;

#[derive(Debug, PartialEq, new, Clone, Default, EnumAsInner)]
pub enum ValueMode {
    #[default]
    Normal,
    Const,
}

#[derive(Debug, PartialEq, new, Clone, EnumAsInner)]
pub enum AstExp {
    StaticValue(Immediate),
    SSAValue(SSARightValue), // todo: change string to real SSA value
}

#[derive(Debug, new, Defaults, Clone, EnumAsInner)]
#[def = "Empty"]
pub enum AstReturnContent {
    Empty,
    VarType(Type),
    FuncType(Type),
    NumLiteral(Immediate),
    Exp(AstExp),
    // Lvalue(SSALeftValue),
    FuncRparams(Vec<SSARightValue>),
    FuncFParams(Vec<(String, SSALeftValue)>),
    FuncFParam((String, SSALeftValue)),
}
