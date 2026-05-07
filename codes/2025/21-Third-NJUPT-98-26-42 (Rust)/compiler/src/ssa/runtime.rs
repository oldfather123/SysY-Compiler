//! Runtime 运行时函数
//!
//! - tests/公开样例与运行时库/sylib.h
//! - tests/公开样例与运行时库/sylib.c

use crate::parser::ast::{self, BaseType, FuncDecl, FuncParam, TypeNode};

fn param(name: &str, base_type: BaseType) -> FuncParam {
    FuncParam {
        name: name.into(),
        ty: TypeNode::new(base_type, vec![]),
    }
}

fn array_param(name: &str, element_type: BaseType) -> FuncParam {
    FuncParam {
        name: name.into(),
        ty: TypeNode::new(element_type, vec![None]), // 未指定大小的数组参数
    }
}

/// 获取所有运行时函数的声明
pub fn make_runtime_functions() -> Vec<ast::FuncDecl> {
    vec![
        // 输入函数
        FuncDecl {
            return_type: BaseType::Int,
            name: "getint".into(),
            params: vec![],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Int,
            name: "getch".into(),
            params: vec![],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Float,
            name: "getfloat".into(),
            params: vec![],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Int,
            name: "getarray".into(),
            params: vec![array_param("a", BaseType::Int)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Int,
            name: "getfarray".into(),
            params: vec![array_param("a", BaseType::Float)],
            body: None,
        },
        // 输出函数
        FuncDecl {
            return_type: BaseType::Void,
            name: "putint".into(),
            params: vec![param("a", BaseType::Int)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "putch".into(),
            params: vec![param("a", BaseType::Int)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "putfloat".into(),
            params: vec![param("a", BaseType::Float)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "putarray".into(),
            params: vec![param("n", BaseType::Int), array_param("a", BaseType::Int)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "putfarray".into(),
            params: vec![param("n", BaseType::Int), array_param("a", BaseType::Float)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "putf".into(),
            params: vec![array_param("a", BaseType::Int)],
            body: None,
        }, // char[] 表示为 int[]
        // 计时函数
        FuncDecl {
            return_type: BaseType::Void,
            name: "starttime".into(),
            params: vec![],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "stoptime".into(),
            params: vec![],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "_sysy_starttime".into(),
            params: vec![param("lineno", BaseType::Int)],
            body: None,
        },
        FuncDecl {
            return_type: BaseType::Void,
            name: "_sysy_stoptime".into(),
            params: vec![param("lineno", BaseType::Int)],
            body: None,
        },
    ]
}
