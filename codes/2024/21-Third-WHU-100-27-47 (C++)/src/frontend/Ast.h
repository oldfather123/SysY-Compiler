#pragma once
#include "Common.h"
#include "IR.h"
#include "Builder.h"

namespace ast {

#pragma region 运算符

    enum class BinaryOp {
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        Eq,
        Neq,
        Lt,
        Gt,
        Leq,
        Geq,
        And,
        Or,
        NR_OPS,
    };

    enum class UnaryOp {
        Add,
        Sub,
        Not,
    };

#pragma endregion

#pragma region 前向声明

    class Node;
    class Stmt;
    class BlockStmt;
    class ExprStmt;
    class DeclStmt;
    class ConstDecl;
    class VarDecl;
    class AssignStmt;
    class IfElseStmt;
    class WhileStmt;
    class BreakStmt;
    class ContinueStmt;
    class ReturnStmt;
    class Expr;
    class InitExpr;
    class LValueExpr;
    class BinaryExpr;
    class UnaryExpr;
    class LiteralExpr;
    class NumberLiteralExpr;
    class IntLiteralExpr;
    class FloatLiteralExpr;
    class StringLiteralExpr;
    class CallExpr;
    class Function;
    class CompUnit;
    class TypeConverter;

#pragma endregion

#pragma region Node

    class Node {
    public:
        virtual ~Node() = default; // 虚析构函数，确保子类析构函数正确调用

        // 纯虚函数，用于返回节点的字符串表示
        virtual std::string toString() const = 0;

        // 虚函数，用于打印节点
        virtual void print(std::ostream &out, unsigned indent = 0) const {
            out << std::string(indent, ' ') << toString() << "\n";
        }

        virtual void baseCodegen(const Ptr<ir_builder::Builder> &builder) = 0;
    };

#pragma endregion

#pragma region Expr

    class Expr : public Node {
    public:
        unsigned line = 0;
        bool isConst = false; // 是否是常量表达式

        Expr() { }

        std::string toString() const override = 0;

        virtual DataType getDataType() const = 0;

        virtual ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) = 0;
        virtual ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) = 0;
        void baseCodegen(const Ptr<ir_builder::Builder> &builder) override {
            rValCodegen(builder);
        }
    };

    class InitExpr : public Expr {
    public:
        Ptr<Expr> singleInitVal;                 // 用于单一初始值
        std::vector<Ptr<InitExpr>> multiInitVal; // 用于复合初始值
        DataType targetDataType;                 // 因为要赋值给一个变量

        InitExpr(Ptr<Expr> expr) : singleInitVal(expr) { }
        InitExpr(const std::vector<Ptr<InitExpr>> &initVals) : multiInitVal(initVals) { }

        std::string toString() const override {
            if (singleInitVal) {
                return singleInitVal->toString();
            } else {
                std::string result = "{";
                for (size_t i = 0; i < multiInitVal.size(); ++i) {
                    result += multiInitVal[i]->toString();
                    if (i < multiInitVal.size() - 1) {
                        result += ", ";
                    }
                }
                result += "}";
                return result;
            }
        }

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class LValueExpr : public Expr {
    public:
        String ident;              // 标识符
        Vector<Ptr<Expr>> indices; // 下标表达式，可能为空，也可能为标识符 a[i]

        Ptr<ir_builder::DeclVar> declVarIR = nullptr; // 标识符对应的变量，IR 生成时填入，类型推断用

        LValueExpr(String ident, Vector<Ptr<Expr>> indices)
            : ident{std::move(ident)}, indices{std::move(indices)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::RegPtr lValCodegen(const Ptr<ir_builder::Builder> &builder);

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class BinaryExpr : public Expr {
    public:
        Ptr<Expr> lhs;
        Ptr<Expr> rhs;
        BinaryOp op; // 在Common.h中定义

        BinaryExpr(BinaryOp op, Ptr<Expr> lhs, Ptr<Expr> rhs)
            : op{op}, lhs{std::move(lhs)}, rhs{std::move(rhs)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class UnaryExpr : public Expr {
    public:
        Ptr<Expr> operand;
        UnaryOp op; // 在Common.h中定义

        UnaryExpr(UnaryOp op, Ptr<Expr> operand)
            : op{op}, operand{std::move(operand)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class LiteralExpr : public Expr {
    public:
        std::string toString() const override = 0;
    };

    class NumberLiteralExpr : public LiteralExpr {
    public:
        virtual ~NumberLiteralExpr() = default;
    };

    class IntLiteralExpr : public NumberLiteralExpr {
        using Value = std::int32_t;

    public:
        Value value;

        static_assert(sizeof(Value) == 4);

        IntLiteralExpr(Value value) : value{value} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class FloatLiteralExpr : public NumberLiteralExpr {
        using Value = float;

    public:
        String text;

        static_assert(sizeof(Value) == 4);

        FloatLiteralExpr(String text) : text{text} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class StringLiteralExpr : public Expr {
        using Value = std::string;

    public:
        Value value;

        StringLiteralExpr(Value value) : value{std::move(value)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

    class CallExpr : public Expr {
    public:
        String callee;
        Vector<Ptr<Expr>> args;

        ir::FuncPtr functionIR = nullptr; // 对应的函数，IR 生成时填入，类型推断用

        CallExpr(String callee, Vector<Ptr<Expr>> args)
            : callee{std::move(callee)}, args{std::move(args)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::Value rValCodegen(const Ptr<ir_builder::Builder> &builder) override;
        ir::Value getConstVal(const Ptr<ir_builder::Builder> &builder) override;

        DataType getDataType() const override;
    };

#pragma endregion

#pragma region Stmt

    class Stmt : public Node {
    public:
        unsigned line = 0;

        std::string toString() const override = 0;

        virtual void codegen(const Ptr<ir_builder::Builder> &builder) = 0;
        void baseCodegen(const Ptr<ir_builder::Builder> &builder) override {
            codegen(builder);
        }
    };

    class BlockStmt : public Stmt {
    public:
        Vector<Ptr<Stmt>> stmts;

        BlockStmt(Vector<Ptr<Stmt>> stmts) : stmts{std::move(stmts)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class ExprStmt : public Stmt {
    public:
        Ptr<Expr> expr;

        ExprStmt(Ptr<Expr> expr) : expr{std::move(expr)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class DeclStmt : public Stmt {
    public:
        bool isConst;
        bool isFuncParam;
        DataType type;
        String identifier;
        Ptr<InitExpr> initializer; // nullptr if no initializer
        Vector<Ptr<Expr>> arrayLengths{};

        // 构造函数，使用默认参数实现isConst的自动设置
        DeclStmt(DataType type, String identifier, Ptr<InitExpr> initializer = nullptr, bool isConst = false, bool isFuncParam = false)
            : isConst{isConst}, isFuncParam(isFuncParam), type{std::move(type)}, identifier{std::move(identifier)}, initializer{std::move(initializer)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::RegPtr registerCodegen(const Ptr<ir_builder::Builder> &builder);

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class AssignStmt : public Stmt {
    public:
        Ptr<LValueExpr> lvalue;
        Ptr<Expr> expr;

        AssignStmt(Ptr<LValueExpr> lvalue, Ptr<Expr> expr) : lvalue{std::move(lvalue)}, expr{std::move(expr)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class IfElseStmt : public Stmt {
    public:
        Ptr<Expr> condition;
        Ptr<Stmt> then_body;
        Ptr<Stmt> else_body; // nullptr if no else

        IfElseStmt(Ptr<Expr> condition, Ptr<Stmt> then_body, Ptr<Stmt> else_body)
            : condition{std::move(condition)}, then_body{std::move(then_body)}, else_body{std::move(else_body)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class WhileStmt : public Stmt {
    public:
        Ptr<Expr> condition;
        Ptr<Stmt> body;

        WhileStmt(Ptr<Expr> condition, Ptr<Stmt> body) : condition{std::move(condition)}, body{std::move(body)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class BreakStmt : public Stmt {
    public:
        std::string toString() const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class ContinueStmt : public Stmt {
    public:
        std::string toString() const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

    class ReturnStmt : public Stmt {
    public:
        Ptr<Expr> ret_expr;

        ReturnStmt(Ptr<Expr> ret_expr) : ret_expr{std::move(ret_expr)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        void codegen(const Ptr<ir_builder::Builder> &builder) override;
    };

#pragma endregion

#pragma region Function

    class Function : public Node {
    public:
        DataType returnType;
        String identifier;
        Vector<Ptr<DeclStmt>> params;
        Ptr<BlockStmt> body;

        Function(DataType returnType, String identifier, Vector<Ptr<DeclStmt>> params, Ptr<BlockStmt> body)
            : returnType{std::move(returnType)}, identifier{std::move(identifier)}, params{std::move(params)}, body{std::move(body)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        ir::FuncPtr codegen(const Ptr<ir_builder::Builder> &builder);

        void baseCodegen(const Ptr<ir_builder::Builder> &builder) override {
            codegen(builder);
        }
    };

#pragma endregion

#pragma region CompUnit

    class CompUnit : public Node {
    public:
        std::vector<Ptr<Node>> children;

        CompUnit(std::vector<Ptr<Node>> children) : children{std::move(children)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;

        Ptr<ir::Module> codegen(const Ptr<ir_builder::Builder> &builder);

        void baseCodegen(const Ptr<ir_builder::Builder> &builder) override {
            codegen(builder);
        }
    };

#pragma endregion

    class TypeConverter {
    public:
        enum class Type {
            Int,
            Float,
            Unknown
        };

        Type type;

        explicit TypeConverter(Type type) : type{type} { }
        virtual ~TypeConverter() = default;

        void print(std::ostream &out, unsigned indent) const {
            // 打印类型信息
            std::string typeStr;
            switch (type) {
                case Type::Int:
                    typeStr = "Int";
                    break;
                case Type::Float:
                    typeStr = "Float";
                    break;
                case Type::Unknown:
                default:
                    typeStr = "Unknown";
                    break;
            }
            out << std::string(indent, ' ') << "Type: " << typeStr << std::endl;
        }
    };

} // namespace ast

namespace test {
    class ReturnStmt : public ast::Stmt {
    public:
        Ptr<ast::Expr> ret_expr;

        ReturnStmt(Ptr<ast::Expr> ret_expr) : ret_expr{std::move(ret_expr)} { }

        std::string toString() const override;
        void print(std::ostream &out, unsigned indent = 0) const override;
    };

    class test_fun {
    public:
        virtual ~test_fun() = default;
        Ptr<int> condition;
        Ptr<float> body;

        test_fun(Ptr<int> condition, Ptr<float> body) : condition{std::move(condition)}, body{std::move(body)} { }
    };
}
