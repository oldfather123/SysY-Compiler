extern crate defaults;
extern crate enum_as_inner;
extern crate thiserror;

use defaults::Defaults;
use enum_as_inner::EnumAsInner;
use thiserror::Error;

#[derive(Error, Debug, EnumAsInner)]
pub enum InvalidLiteral {
    #[error("invalid int literal {literal:?}")]
    InvalidI32Literal { literal: String },
    #[error("invalid float literal {literal:?}")]
    InvalidF32Literal { literal: String },
}

/// semantic check error
#[derive(Error, Debug, Defaults, EnumAsInner)]
#[def = "MainFuncNotFound"]
pub enum SemanticError {
    #[error("function {func_name} redefine")]
    FunctionRedifine { func_name: String },
    #[error("cannot cast from {from} to {to}")]
    InvalidTypeCast { from: String, to: String },
    #[error("main function not found")]
    MainFuncNotFound,
    #[error("cannot index {name} with offset {index}")]
    NonPositiveArraySize { index: i32, name: String },
    #[error("cannot initialize array {array_name} with size {expected_size:?} s on dimension {dimension} but got {actual_size} elementinstead")]
    InvalidInitList {
        array_name: String,
        expected_size: i32,
        dimension: usize,
        actual_size: i32,
    },
    #[error("duplicate local variable {name}")]
    DuplicateLocalName { name: String },
    #[error("duplicate global variable {name}")]
    DuplicateGlobalName { name: String },
    #[error("unrecognized variable name {name}")]
    UnrecognizedVarName { name: String },
    #[error("cannot index on non-array variable {name}")]
    InvalidIndexOperator { name: String },
    #[error("unrecognized function name {name}")]
    UnrecognizedFuncName { name: String },
    #[error("cannot assign {actual} to {expected}")]
    AssignmentTypeError { expected: String, actual: String },
    #[error("invalid literal {specific:?}")]
    InvalidLiteral { specific: InvalidLiteral },
    #[error("break statement not in loop")]
    InvalidBreak,
    #[error("continue statement not in loop")]
    InvalidContinue,
    #[error("return statement not in function")]
    OutFunctionReturn,
    #[error("return statement in void function")]
    VoidFuncReturn,
    #[error("function {caller} used void function {callee} return value ")]
    VoidFuncReturnValueUsed { caller: String, callee: String },
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_invalid_int_literal() {
        // too large
        let err = InvalidLiteral::InvalidI32Literal {
            literal: "2147483648".to_string(),
        };
        assert_eq!(format!("{}", err), "invalid int literal \"2147483648\"");

        // too small
        let err = InvalidLiteral::InvalidI32Literal {
            literal: "-2147483649".to_string(),
        };
        assert_eq!(format!("{}", err), "invalid int literal \"-2147483649\"");
    }

    #[test]
    fn test_invalid_float_literal() {
        let err = InvalidLiteral::InvalidF32Literal {
            literal: "123.abc".to_string(),
        };
        assert_eq!(format!("{}", err), "invalid float literal \"123.abc\"");
    }

    #[test]
    fn test_semantic_error() {
        let err = SemanticError::InvalidTypeCast {
            from: "&[f32; 5]".to_string(),
            to: "i32".to_string(),
        };
        assert_eq!(format!("{}", err), "cannot cast from &[f32; 5] to i32");
    }
}
