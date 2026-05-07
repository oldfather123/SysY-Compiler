#ifndef AAAC_AST_UTILS_H
#define AAAC_AST_UTILS_H
#pragma once
namespace aaac {
namespace frontend {
class Expr;
bool isLiteral(Expr *expr);

bool isIntLiteral(Expr *expr);

bool isFloatLiteral(Expr *expr);

int getValueInt(Expr *expr);

float getValueFloat(Expr *expr);

// type of expr is const int or float, copy the constExpr of expr, which is a literal type
Expr *copyLiteral(Expr *expr);

} // namespace frontend
} // namespace aaac

#endif
