#include "AST/utils.h"
#include "AST/expr.h"
#include "common.h"
#include <memory>
namespace aaac {
namespace frontend {

// utils about expr

bool isLiteral(Expr *expr) {
    return typeid(*expr) == typeid(IntLiteral) ||
            typeid(*expr) == typeid(FloatLiteral);
}

bool isIntLiteral(Expr *expr) {
    return typeid(*expr) == typeid(IntLiteral);
}

bool isFloatLiteral(Expr *expr) {
    return typeid(*expr) == typeid(FloatLiteral);
}

int getValueInt(Expr *expr) {
    Assert(typeid(*expr) == typeid(IntLiteral), "expr must be IntLiteral");
    return dynamic_cast<IntLiteral*>(expr)->getValue();
}

float getValueFloat(Expr *expr) {
    Assert(typeid(*expr) == typeid(FloatLiteral), "expr must be FloatLiteral");
    return dynamic_cast<FloatLiteral*>(expr)->getValue();
}


Expr *copyLiteral(Expr *expr) {
    Assert(expr->isConst() && isLiteral(expr), "must be const literal expr");
    Expr* const_expr = expr;
    if(isIntLiteral(const_expr)) {
        return new IntLiteral(expr->getPos(),getValueInt(const_expr));
    } else if(isFloatLiteral(const_expr)) {
        return new FloatLiteral(expr->getPos(),getValueFloat(const_expr));
    } else {
        Assert(false, "unreachable");
    }
}

} // namespace frontend
} // namespace aaac