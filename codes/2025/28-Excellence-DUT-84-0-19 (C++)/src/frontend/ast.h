#pragma once

#include <variant>
#include <ostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


//打印AST树
class PrintAST
{
public:
    virtual ~PrintAST() = default;

    [[nodiscard]] std::string to_string() const
    {
        std::ostringstream os;
        print(os, 0);
        return os.str();
    }

    virtual void print(std::ostream &out, unsigned indent) const = 0;

    friend std::ostream &operator<<(std::ostream &out, PrintAST const &ast)
    {
        ast.print(out, 0);
        return out;
    }
};

//单目运算符枚举
enum class UnaryOp
{
    Add = 0 , Sub = 1 , Not = 2
};

//双目运算符枚举
enum class BinaryOp
{
    Add = 0, Sub = 1, Mul = 2, Div = 3,
    Mod = 4, Eq = 5, Neq = 6, Lt = 7,
    Gt = 8, Leq = 9, Geq = 10, And = 11,
    Or = 12
};

//非数组常量/变量类型枚举
enum ScalarType
{
    Int = 0 , Float = 1 , String = 2
};

struct Type
{
    int base_type; //基础类型ScalarType
    bool is_const; //是否为常量
    std::vector<int> dims;  
    int nr_dims() const { return dims.size(); }

    int nr_elems() const
    {
        int count = 1;
        for (int n : dims)
            count *= n;
        return count;
    }

    int size() const { return nr_elems() * 4; } //返回类型大小

    bool operator==(const Type &b) const
    {
        if (base_type != b.base_type)
            return false;
        if (nr_dims() != b.nr_dims())
            return false;
        for (int i = 0; i < nr_dims(); i++)
        {
            if (dims[i] != b.dims[i])
                return false;
        }
        return true;
    }
    bool operator!=(const Type &b) const { return !this->operator==(b); }
    Type get_pointer_type() const
    {
        Type new_type = *this;
        new_type.dims.insert(new_type.dims.begin(), 0);
        return new_type;
    }
    bool is_pointer() const { return dims.size() > 0 && dims[0] == 0; } //是否是指针
    bool is_pointer_to_scalar() const { return dims.size() == 1 && dims[0] == 0; } //是否是指向基础类型的指针

    Type() {}
    Type(int btype) : base_type{btype}, is_const{false} {}
    Type(int btype, bool const_qualified)
        : base_type{btype}, is_const{const_qualified} {}
    Type(int btype, std::vector<int> dimensions)
        : base_type{btype}, is_const{false}, dims{std::move(dimensions)} {}
    Type(Type type, std::vector<int> dimensions)
        : base_type{type.base_type}, is_const{false}, dims{
                                                          std::move(dimensions)}
    {
        assert(!(dims.size() > 0));
    }
};

struct ConstValue //常量
{
    int type;
    union
    {
        int iv;
        float fv;
    };
    ConstValue() {}
    ConstValue(int v) : type{Int} { iv = v; }
    ConstValue(float v) : type{Float} { fv = v; }

    bool operator==(const ConstValue &b) const
    {
        if (type != b.type)
            if (type == Int)
                return iv == b.iv;
        if (type == Float)
            return fv == b.fv;
        return false;
    }
    bool operator!=(const ConstValue &b) const { return !this->operator==(b); }

    std::string to_string() const
    {
        if (type == Int)
            return std::to_string(iv);
        if (type == Float)
            return std::to_string(fv);
    }
};

namespace std
{
    template <>
    class hash<ConstValue>
    {
    public:
        size_t operator()(const ConstValue &r) const { return r.iv; }
    };
} 

namespace std
{
    template <class T>
    struct hash<vector<T>>
    {
        size_t operator()(const vector<T> &r) const
        {
            size_t res = 0;
            for (auto t : r)
            {
                res = res * 1221821 + hash<T>()(t);
            }
            return res;
        }
    };
    template <>
    class hash<Type>
    {
    public:
        size_t operator()(const Type &r) const
        {
            return hash<int>()(r.base_type) * 17 + hash<std::vector<int>>()(r.dims);
        }
    };
} 


struct Var
{
    Type type;
    std::optional<ConstValue> val;
    std::unique_ptr<std::map<int, ConstValue>>
        arr_val; 

    Var() {}
    Var(Type type_) : type{std::move(type_)} {}
    Var(Type type_, std::optional<ConstValue> value)
        : type{std::move(type_)}, val{std::move(value)} {}
};

namespace frontend
{
    namespace ast
    {

        using UnaryOp = ::UnaryOp;
        using BinaryOp = ::BinaryOp;

        class SysYType : public PrintAST
        {
        public:
            virtual ~SysYType() = default;
        };

        class ScalarType : public SysYType //基础类型
        {
        public:
            using Type = int;

            explicit ScalarType(Type type) : m_type{type} {}
            virtual ~ScalarType() = default;

            void print(std::ostream &out, unsigned indent) const override;

            Type type() const { return m_type; }

        private:
            Type m_type; //按照ScalarType枚举类
        };

        class Expression;

        class ArrayType : public SysYType //数组类型
        {
        public:
            using Dimension = std::unique_ptr<Expression>;

            ArrayType(ScalarType type, std::vector<Dimension> dimensions,bool omit_first_dimension)
                : m_type{type}, m_dimensions{std::move(dimensions)},
                  m_omit_first_dimension{omit_first_dimension} {}
            virtual ~ArrayType() = default;

            void print(std::ostream &out, unsigned indent) const override;

            ScalarType::Type base_type() const { return m_type.type(); }
            const std::vector<Dimension> &dimensions() const { return m_dimensions; }
            bool first_dimension_omitted() const { return m_omit_first_dimension; }

        private:
            ScalarType m_type; //数组的基础类型
            std::vector<Dimension> m_dimensions; //数组维数
            bool m_omit_first_dimension;
        };

        class Identifier : public PrintAST //变量名
        {
        public:
            Identifier(std::string name, bool const mangle = false)
                : m_name{mangle ? '$' + name : std::move(name)} {}
            virtual ~Identifier() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::string &name() const { return m_name; }

        private:
            std::string m_name; //字符串变量名
        };

        class Parameter : public PrintAST //函数参数
        {
        public:
            Parameter(std::unique_ptr<SysYType> type, Identifier ident)
                : m_type{std::move(type)}, m_ident{std::move(ident)} {}
            virtual ~Parameter() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<SysYType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            std::unique_ptr<SysYType> m_type; //参数类型
            Identifier m_ident; //参数名
        };

        class AstNode : public PrintAST //抽象语法树节点
        {
        public:
            virtual ~AstNode() = default;
        };


        class Expression : public AstNode //表达式
        {
        public:
            Expression() {}
            virtual ~Expression() = default;
            virtual std::string to_string() const = 0;

        public:
            // 求得的表达式类型
            mutable std::optional<Type> type;
        };

        class LValue : public Expression //左值 赋值操作
        {
        public:
            LValue(Identifier ident, std::vector<std::unique_ptr<Expression>> indices)
                : m_ident{std::move(ident)}, m_indices{std::move(indices)} {}
            virtual ~LValue() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            const Identifier &ident() const { return m_ident; }
            const std::vector<std::unique_ptr<Expression>> &indices() const
            {
                return m_indices;
            }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            Identifier m_ident; //左值变量名
            std::vector<std::unique_ptr<Expression>> m_indices;
        };

        class UnaryExpr : public Expression //单目运算符表达式
        {
        public:
            UnaryExpr(UnaryOp op, std::unique_ptr<Expression> operand)
                : m_op{op}, m_operand{std::move(operand)} {}
            virtual ~UnaryExpr() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            UnaryOp op() const { return m_op; }
            const std::unique_ptr<Expression> &operand() const { return m_operand; }

        private:
            UnaryOp m_op; //单目操作符
            std::unique_ptr<Expression> m_operand; //操作对象
        };

        class BinaryExpr : public Expression //双目运算符表达式
        {
        public:
            BinaryExpr(BinaryOp op, std::unique_ptr<Expression> lhs,
                       std::unique_ptr<Expression> rhs)
                : m_op{op}, m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
            virtual ~BinaryExpr() = default;

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

            BinaryOp op() const { return m_op; }
            const std::unique_ptr<Expression> &lhs() const { return m_lhs; }
            const std::unique_ptr<Expression> &rhs() const { return m_rhs; }

        private:
            BinaryOp m_op; //双目操作符
            std::unique_ptr<Expression> m_lhs, m_rhs; //两个操作对象
        };

        class IntLiteral : public Expression //整型
        {
        public:
            using Value = std::int32_t;
            static_assert(sizeof(Value) == 4);

            IntLiteral(Value value) : m_value{value} {}
            virtual ~IntLiteral() = default;

            Value value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

        private:
            Value m_value; //整型value
        };

        class FloatLiteral : public Expression //浮点型
        {
        public:
            using Value = float;
            static_assert(sizeof(Value) == 4);

            FloatLiteral(Value value) : m_value{value} {}
            virtual ~FloatLiteral() = default;

            Value value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

        private:
            Value m_value; //浮点型value
        };

        class StringLiteral : AstNode //字符串型
        {
        public:
            using Value = std::string;

            StringLiteral(Value value) : m_value{std::move(value)} {}
            virtual ~StringLiteral() = default;
            const std::string &value() const { return m_value; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            Value m_value; //字符串型value
        };

        class Call : public Expression //调用函数
        {
        public:
            using Argument = std::variant<std::unique_ptr<Expression>, StringLiteral>;

            Call(Identifier func, std::vector<Argument> args, unsigned line)
                : m_func{std::move(func)}, m_args{std::move(args)}, m_line(line) {}
            virtual ~Call() = default;

            const Identifier &func() const { return m_func; }
            const std::vector<Argument> &args() const { return m_args; }
            unsigned line() const { return this->m_line; }

            void print(std::ostream &out, unsigned indent) const override;
            std::string to_string() const override;

        private:
            Identifier m_func; //调用的函数名
            std::vector<Argument> m_args; //函数参数
            unsigned m_line; //行数
        };

        class Statement : public AstNode
        {
        public:
            virtual ~Statement() = default;
        };

        class ExprStmt : public Statement //表达式语句
        {
        public:
            explicit ExprStmt(std::unique_ptr<Expression> expr)
                : m_expr{std::move(expr)} {}
            virtual ~ExprStmt() = default;

            const std::unique_ptr<Expression> &expr() const { return m_expr; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_expr; //表达式
        };

        class Assignment : public Statement //赋值语句
        {
        public:
            Assignment(std::unique_ptr<LValue> lhs, std::unique_ptr<Expression> rhs)
                : m_lhs{std::move(lhs)}, m_rhs{std::move(rhs)} {}
            virtual ~Assignment() = default;

            const std::unique_ptr<LValue> &lhs() const { return m_lhs; }
            const std::unique_ptr<Expression> &rhs() const { return m_rhs; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<LValue> m_lhs; //左值
            std::unique_ptr<Expression> m_rhs; //表达式
        };

        class Initializer : public AstNode //初始化器
        {
        public:
            using Value = std::variant<std::unique_ptr<Expression>,
                                       std::vector<std::unique_ptr<Initializer>>>;

            explicit Initializer(std::unique_ptr<Expression> value)
                : m_value{std::move(value)} {}
            explicit Initializer(std::vector<std::unique_ptr<Initializer>> values)
                : m_value{std::move(values)} {}
            virtual ~Initializer() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const Value &value() const { return m_value; }

        private:
            Value m_value; //表达式或初始化器
        };

        class Declaration : public AstNode //函数声明
        {
        public:
            Declaration(std::unique_ptr<SysYType> type, Identifier ident,
                        std::unique_ptr<Initializer> init, bool const_qualified)
                : m_type{std::move(type)}, m_ident{std::move(ident)},
                  m_init{std::move(init)}, m_const_qualified{const_qualified} {}
            virtual ~Declaration() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<SysYType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }
            const std::unique_ptr<Initializer> &init() const { return m_init; }
            bool is_const() const { return m_const_qualified; }

        public:
            mutable std::shared_ptr<Var> var;

        private:
            std::unique_ptr<SysYType> m_type; //声明的类型
            Identifier m_ident; //声明的名称
            std::unique_ptr<Initializer> m_init; //初始化器
            bool m_const_qualified; //是否是常量
        };

        class Block : public Statement //基本块
        {
        public:
            using Child =
                std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Statement>>;

            explicit Block(std::vector<Child> children)
                : m_children{std::move(children)} {}
            virtual ~Block() = default;

            const std::vector<Child> &children() const { return m_children; }

            void print(std::ostream &out, unsigned indent) const override;

        public:
            std::vector<Child> m_children;
        };

        class IfElse : public Statement //选择语句
        {
        public:
            IfElse(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then,
                   std::unique_ptr<Statement> else_)
                : m_cond{std::move(cond)}, m_then{std::move(then)}, m_else{std::move(
                                                                        else_)} {}
            virtual ~IfElse() = default;

            const std::unique_ptr<Expression> &cond() const { return m_cond; }
            const std::unique_ptr<Statement> &then() const { return m_then; }
            const std::unique_ptr<Statement> &otherwise() const { return m_else; }

            void print(std::ostream &out, unsigned indent) const override;

        public:
            std::unique_ptr<Expression> m_cond; //条件语句
            std::unique_ptr<Statement> m_then, m_else; //跳转到的语句
        };
        class While : public Statement //循环语句
        {
        public:
            While(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> body)
                : m_cond{std::move(cond)}, m_body{std::move(body)} {}
            virtual ~While() = default;

            const std::unique_ptr<Expression> &cond() const { return m_cond; }
            const std::unique_ptr<Statement> &body() const { return m_body; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_cond; //条件语句
            std::unique_ptr<Statement> m_body;
        };

        class Break : public Statement
        {
        public:
            virtual ~Break() = default;

            void print(std::ostream &out, unsigned indent) const override;
        };

        class Continue : public Statement
        {
        public:
            virtual ~Continue() = default;

            void print(std::ostream &out, unsigned indent) const override;
        };

        class Return : public Statement
        {
        public:
            explicit Return(std::unique_ptr<Expression> res) : m_res{std::move(res)} {}
            virtual ~Return() = default;

            const std::unique_ptr<Expression> &res() const { return m_res; }

            void print(std::ostream &out, unsigned indent) const override;

        private:
            std::unique_ptr<Expression> m_res;
        };

        class Function : public AstNode //函数
        {
        public:
            Function(std::unique_ptr<ScalarType> type, Identifier ident,
                     std::vector<std::unique_ptr<Parameter>> params,
                     std::unique_ptr<Block> body)
                : m_type{std::move(type)}, m_ident{std::move(ident)},
                  m_params{std::move(params)}, m_body{std::move(body)} {}
            virtual ~Function() = default;

            void print(std::ostream &out, unsigned indent) const override;

            const std::unique_ptr<ScalarType> &type() const { return m_type; }
            const Identifier &ident() const { return m_ident; }
            const std::vector<std::unique_ptr<Parameter>> &params() const
            {
                return m_params;
            }
            const std::unique_ptr<Block> &body() const { return m_body; }

        private:
            std::unique_ptr<ScalarType> m_type; //函数返回值类型
            Identifier m_ident; //函数名
            std::vector<std::unique_ptr<Parameter>> m_params; //函数参数
            std::unique_ptr<Block> m_body; //函数体
        };

        class CompileUnit : public AstNode //编译入口
        {
        public:
            using Child = std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>;
            explicit CompileUnit(std::vector<std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>> children)
                : m_children{std::move(children)} {}
            virtual ~CompileUnit() = default;

            const std::vector<std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>> &getChild() const { return m_children; }

            void print(std::ostream &out, unsigned indent) const override;

            const std::vector<std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>> &children() const { return m_children; }

        private:
            std::vector<std::variant<std::unique_ptr<Declaration>, std::unique_ptr<Function>>> m_children; //Declaration或Function语句
        };

    }; 
} 