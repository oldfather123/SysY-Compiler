#include "AST/expr.h"
#include "AST/type.h"
#include "AST/utils.h"
#include "common.h"
#include "frontend.h"
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

namespace aaac {
namespace frontend {

static void Indent(std::ostream &os, int d) {
    os << std::string(d,' ');
}

std::string getString(UnaryOp op) {
    switch (int(op)) {
        case Positive: return "+";
        case Negative: return "-";
        case Negate  : return "!";
        default: assert(false && "wrong uanry operator");
    }
}

std::string getString(BinaryOp op) {
    switch (int(op)) {
        case PLUS_OP: return "+";
        case MINUS_OP: return "-";
        case TIMES_OP: return "*";
        case DIVIDE_OP: return "/";
        case MODULE_OP: return "%";
        case EQ_OP: return "==";
        case NEQ_OP: return "!=";
        case LT_OP: return "<";
        case LE_OP: return "<=";
        case GT_OP: return ">";
        case GE_OP: return ">=";
        case AND_OP: return "&&";
        case OR_OP: return "||";
        default: assert(false && "wrong binary operator");
    }
}

bool isArithOp(BinaryOp op) {
    return PLUS_OP <= op && op <= MODULE_OP;
}

bool isCondOp(BinaryOp op) {
    return EQ_OP <= op && op <= OR_OP;
}

void UnaryOperator::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << " " << "/*" << (int)getExType() << "*/";
    os << getString(op);
    bool flag = false;
    if(typeid(*expr) != typeid(Var) &&
        typeid(*expr) != typeid(IntLiteral) &&
        typeid(*expr) != typeid(FloatLiteral)) {
        flag = true;
    }

    if(flag) os << "(";
    expr->PrintTo(os, 0);
    if(flag) os << ")";
}

void BinaryOperator::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    bool flag = true;
    if(typeid(*lhs) == typeid(IntLiteral) ||
    typeid(*lhs) == typeid(FloatLiteral) ||
    typeid(*lhs) == typeid(Var) || 
    typeid(*lhs) == typeid(SubscriptVar)) {
        flag = false;
    }
    if(flag) os << "(";
    lhs->PrintTo(os, 0);
    if(flag) os << ")";

    os << " " << "/*" << (getExType() == Expressioned? "expr": "cond") << "*/" << getString(op) << " ";

    flag = true;
    if(typeid(*rhs) == typeid(IntLiteral) ||
    typeid(*rhs) == typeid(FloatLiteral) ||
    typeid(*rhs) == typeid(Var) || 
    typeid(*rhs) == typeid(SubscriptVar)) {
        flag = false;
    }
    if(flag) os << "(";
    rhs->PrintTo(os, 0);
    if(flag) os << ")";
}

void IntLiteral::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << std::to_string(value);
}

void FloatLiteral::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << std::to_string(value);
}

void ImplicitCast::PrintTo(std::ostream &os, int d) const {
    switch (castTy) {
        case CastType::IntToFloat:
            os << "float"; break;
        case CastType::FloatToInt:
            os << "int"; break;
    }
    os << "(";
    expr->PrintTo(os, d);
    os << ")";
    return;
}

void InitList::PrintTo(std::ostream &os, int d) const {
    Indent(os, 0);
    os << "{";
    if(auto type = getType()) {
        os << "/*";
        type->PrintTo(os);
        os << "*/";
    }
    for(Expr *expr : init_list) {
        if(expr != init_list.front()) {
            os << ", ";
        }
        expr->PrintTo(os, 0);
    }
    os << "}";
}

void CallExpr::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << func_name <<"(";
    for(RParm *rparm : *rparms) {
        if(rparm != rparms->front()) os << ",";
        rparm->PrintTo(os, 0);
    }
    os << ")";
}

void Var::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << ident;
}

void SubscriptVar::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    var->PrintTo(os, 0);
    os << "[";
    subscript->PrintTo(os, 0);
    os << "]";
}

ImplicitCast::ImplicitCast(Expr *expr, CastType castTy): 
Expr(expr->getPos()), expr(expr),castTy(castTy) {
    setConst(expr->isConst());
    switch (castTy) {
        case CastType::IntToFloat:
        {
            Assert(expr->getType() == TypeFactory::IntTy, "src expr type is int");
            setType(TypeFactory::FloatTy);
            if(isConst()) {
                FloatLiteral *ptr = new FloatLiteral(expr->getPos(),getValueInt(expr->getConstExpr()));
                setConstExpr(ptr);
            }
            break;
        }
        case CastType::FloatToInt:
        {
            Assert(expr->getType() == TypeFactory::FloatTy, "src expr type is float");
            setType(TypeFactory::IntTy);
            if(isConst()) {
                IntLiteral *ptr = new IntLiteral(expr->getPos(),getValueFloat(expr->getConstExpr()));
                setConstExpr(ptr);
            }
            break;
        }
        default:
            Assert(false, "unreachable");
    }
}

InitList* InitList::copy_constlist(int subscript) {
    Assert(isConst(),"only const initlist can be copied");
    std::shared_ptr<ArrayType> arrayty = std::static_pointer_cast<ArrayType>(type);
    if(subscript >= 0) {
        if(subscript >= arrayty->getArrayLen()) {
            return nullptr;
        }
        if((size_t)subscript >= init_list.size()) {
            return new InitList(getPos());
        }
        return static_cast<InitList*>(init_list[subscript])->copy_constlist();
    }
    InitList *copy = new InitList(getPos());
    for(Expr *expr: init_list) {
        if(expr->getType()->isBuiltinType()) {
            copy->append(copyLiteral(expr->getConstExpr()));
        } else if(typeid(*expr) == typeid(InitList)) {
            copy->append(static_cast<InitList*>(expr)->copy_constlist());
        }
    }
    copy->setConst();
    copy->setType(type);
    return copy;
}

Expr* InitList::get_element(int subscript) {
    Assert(isConst(),"only const element can be get during compiling time");
    std::shared_ptr<ArrayType> arrayty = std::static_pointer_cast<ArrayType>(type);
    Assert(arrayty->getBaseType()->isBuiltinType(),"the element should be int or float");
    if(subscript >= arrayty->getArrayLen()) {
        return nullptr;
    }
    if((size_t)subscript >= init_list.size()) {
        if(arrayty->getBaseType() == TypeFactory::IntTy) {
            return new IntLiteral(getPos(),0);
        } else if(arrayty->getBaseType() == TypeFactory::FloatTy) {
            return new FloatLiteral(getPos(),0);
        } else {
            Assert(false, "unreachable");
        }
    }
    if(arrayty->getBaseType() == TypeFactory::IntTy) {
        return new IntLiteral(getPos(),getValueInt(init_list[subscript]));
    } else if(arrayty->getBaseType() == TypeFactory::FloatTy) {
        return new FloatLiteral(getPos(),getValueFloat(init_list[subscript]));
    } else {
            Assert(false, "unreachable");
    }
}

std::shared_ptr<Type> CallExpr::getType() const {
    std::shared_ptr<FunctionType> ptr = std::static_pointer_cast<FunctionType>(type);
    return ptr->getRetType();
}

} // namespace frontend
} // namespace aaac