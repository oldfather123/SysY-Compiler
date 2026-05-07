#ifndef DEFINE_HPP
#define DEFINE_HPP
#undef VOID

enum class StmtType { SEMI, ASGN, EXP, CONTINUE, BREAK, RET, BLK, IF, WHILE };

enum class UnaryOp { ADD, MINUS, NOT };

enum class AddOp { ADD, MINUS };

enum class MulOp { MUL, DIV, MOD };

enum class RelOp { GTE, LTE, GT, LT };

enum class EqOp { EQ, NEQ };

enum class BType { VOID, INT, FLOAT };

// StmtType to int
constexpr int operator+(StmtType v) { return static_cast<int>(v); }

// UnaryOp to int
constexpr int operator+(UnaryOp v) { return static_cast<int>(v); }

// AddOp to int
constexpr int operator+(AddOp v) { return static_cast<int>(v); }

// MulOp to int
constexpr int operator+(MulOp v) { return static_cast<int>(v); }

// RelOp to int
constexpr int operator+(RelOp v) { return static_cast<int>(v); }

// EqOp to int
constexpr int operator+(EqOp v) { return static_cast<int>(v); }

// BType to int
constexpr int operator+(BType v) { return static_cast<int>(v); }

#endif
