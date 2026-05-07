use crate::ast::{AddExpr, BoolExprs, MathOpe, RelExpr, RelOpe, UnaryExp, UnaryOp};

impl ToString for UnaryOp {
    fn to_string(&self) -> String {
        match self {
            UnaryOp::Positive => "+".to_string(),
            UnaryOp::Negative => "-".to_string(),
            UnaryOp::Bang => "!".to_string(),
            UnaryOp::Space => " ".to_string(),
        }
    }
}

impl ToString for BoolExprs {
    fn to_string(&self) -> String {
        match self {
            BoolExprs::And(left, right) => {
                let mut ret = left.to_string();
                ret.push_str("&&");
                ret.push_str(&right.to_string());
                ret
            }
            BoolExprs::Or(left, right) => {
                let mut ret = left.to_string();
                ret.push_str("||");
                ret.push_str(&right.to_string());
                ret
            }
            BoolExprs::RelExpr(x) => x.to_string(),
        }
    }
}

impl ToString for RelOpe {
    fn to_string(&self) -> String {
        match self {
            RelOpe::Less => "<".to_string(),
            RelOpe::LessOrEqual => "<=".to_string(),
            RelOpe::Greater => ">".to_string(),
            RelOpe::GreaterOrEqual => ">=".to_string(),
            RelOpe::IsEqual => "==".to_string(),
            RelOpe::NotEqual => "!=".to_string(),
        }
    }
}

impl ToString for MathOpe {
    fn to_string(&self) -> String {
        match self {
            MathOpe::Add => "+".to_string(),
            MathOpe::Sub => "-".to_string(),
            MathOpe::Mul => "*".to_string(),
            MathOpe::Div => "/".to_string(),
            MathOpe::Ram => "%".to_string(),
        }
    }
}

impl ToString for RelExpr {
    fn to_string(&self) -> String {
        match self {
            RelExpr::Math(expr) => {
                return expr.to_string();
            }
            RelExpr::Rel(a, ope, c) => {
                let mut ret = a.to_string();
                ret.push_str(&ope.to_string());
                ret.push_str(&c.to_string());
                ret
            }
        }
    }
}

impl ToString for AddExpr {
    fn to_string(&self) -> String {
        return match self {
            AddExpr::Unary(unary) => unary.to_string(),
            AddExpr::Expr(left, ope, right) => {
                let left = left.to_string();
                let ope = ope.to_string();
                let right = right.to_string();
                format!("{} {} {}", left, ope, right)
            }
        };
    }
}

impl ToString for UnaryExp {
    fn to_string(&self) -> String {
        return match self {
            UnaryExp::Lit(op, tok) => {
                let mut ret = String::new();
                for oper in op {
                    let ope = oper.to_string();
                    ret.push_str(&ope);
                    ret.push_str(" ");
                }
                let sym = tok.sym.string.to_string();
                ret.push_str(&sym);
                ret
            }
            UnaryExp::VarRef(op, tok) => {
                let mut ret = String::new();
                for oper in op {
                    let ope = oper.to_string();
                    ret.push_str(&ope);
                    ret.push_str(" ");
                }
                let sym = tok.sym.string.to_string();
                ret.push_str(&sym);
                ret
            }
            UnaryExp::ArrayRef(op, array_ref) => {
                let mut ret = String::new();
                for oper in op {
                    let ope = oper.to_string();
                    ret.push_str(&ope);
                    ret.push_str(" ");
                }
                let sym = &array_ref.array_name.sym.string.to_string();
                ret.push_str(&sym);
                for single in &array_ref.pos {
                    ret.push_str(&format!("[{}]", single.to_string()));
                }
                ret
            }
            UnaryExp::Invoke(op, invoke) => {
                let mut ret = String::new();
                for oper in op {
                    let ope = oper.to_string();
                    ret.push_str(&ope);
                    ret.push_str(" ");
                }
                let sym = invoke.fun_name.sym.string.to_string();
                ret.push_str(&sym);
                ret.push_str("(");
                for (index, expr) in invoke.params.iter().enumerate() {
                    ret.push_str(&expr.to_string());
                    if index != invoke.params.len() - 1 {
                        ret.push_str(",")
                    }
                }
                ret.push_str(")");
                ret
            }
            UnaryExp::Paren(op, tk) => {
                let mut ret = "(".to_string();
                for oper in op {
                    let ope = oper.to_string();
                    ret.push_str(&ope);
                    ret.push_str(" ");
                }
                let str = tk.to_string();
                ret.push_str(&str);
                ret.push_str(")");
                ret
            }
            UnaryExp::None => "".to_string(),
        };
    }
}
