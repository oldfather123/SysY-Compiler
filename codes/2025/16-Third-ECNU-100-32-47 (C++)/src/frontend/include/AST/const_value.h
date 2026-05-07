#ifndef AAAC_CONST_VALUE_H
#define AAAC_CONST_VALUE_H

#include <memory>
namespace aaac {
namespace frontend {
class Expr;
class ConstValue {
public:
    ConstValue(): isConst_(false), expr(nullptr) {}
    ConstValue(bool isConst, Expr *expr): 
        isConst_(isConst), expr(expr) {}
    ~ConstValue();
    void setConst(bool is_const = true) {isConst_ = is_const; }
    bool isConst() const { return isConst_; }
    Expr *getConstExpr() { return expr; }
    void setConstExpr(Expr *expr) { this->expr = expr; }
private:
    bool isConst_;
    // own by ConstValue itself, i.e. a copy, so we should free this in deconstructor.
    // 'expr' of a const var/array decl is nullptr, their const value is stored in their
    // 'init' fields.
    // specially, 'expr' of InitList is always nullptr
    // ('expr' in the comments above means 'this->expr')
    Expr *expr; 
};

// template<class Derived>
// class T {
// public:
//     void foo() {
//         get_Derived()->isConst();
//     }
// private:
//     Derived *get_Derived() {
//         const int a[10][20] = {{0 + 1},{1}};
//         int b = a[1][0];
//         return std::static_pointer_cast<Derived>(this);
//     }
// };


} // namespace frontend
} // namespace aaac

#endif