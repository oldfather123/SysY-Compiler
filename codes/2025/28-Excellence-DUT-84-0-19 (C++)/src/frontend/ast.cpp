#include <cassert>
#include <ostream>
#include <string>
#include <string_view>

#include "ast.h"

using namespace std;
using namespace frontend::ast;


//打印AST树

inline void print_indent(ostream &os, int indent)
{
    os << string(indent, ' ');
}

bool swappable(BinaryOp op);

string_view type_string(int type)
{
    if (type == Int) 
        return "int";
    if (type == Float) 
        return "float";

    assert("ScalarType ERROR");
}

string_view type_string(const frontend::ast::ScalarType *type)
{
    if (type == 0) 
        return "void";

    return type_string(type->type());
}

ostream &operator<<(ostream &os, const std::unique_ptr<frontend::ast::ScalarType> &type)
{
    os << type_string(type.get());
    return os;
}

ostream &operator<<(ostream &os, const Identifier &ident)
{
    os << ident.name();
    return os;
}

ostream &operator<<(ostream &os, const ArrayType &type)
{
    os << type_string(type.base_type());
    int n_dims = type.dimensions().size();
    if (type.first_dimension_omitted() == true)
        n_dims++;
    for (int i = 0; i < n_dims; ++i)
        os << "[]";
    return os;
}

ostream &operator<<(ostream &os, const std::unique_ptr<SysYType> &type)
{
    if (!type)
    {
        os << "void";
        return os;
    }
    if (auto scalar_type = dynamic_cast<frontend::ast::ScalarType *>(type.get()))
    {
        os << type_string(scalar_type);
    }
    else if (auto array_type = dynamic_cast<ArrayType *>(type.get()))
    {
        os << *array_type;
    }
    else
    {
        assert(false);
    }
    return os;
}

ostream &operator<<(ostream &os, const Parameter &param)
{
    os << param.type() << ' ' << param.ident();
    return os;
}

//打印抽象语法树需要的print函数
void Parameter::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << *this << '\n';
}

void Identifier::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << *this << '\n';
}

void LValue::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "LValue " << m_ident;
    for (auto const &index : this->m_indices)
    {
        out << "[\n";
        index->print(out, indent + 2);
        print_indent(out, indent);
        out << ']';
    }
    out << '\n';
}

void frontend::ast::ScalarType::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << type_string(this) << '\n';
}

void ArrayType::print(std::ostream &out, unsigned int indent) const {
    print_indent(out, indent);
    out << *this << '\n';
}

string_view Unaryop_string(UnaryOp op)  //单目运算符转字符串
{
    if (op == UnaryOp::Add) 
        return "+";
    if (op == UnaryOp::Sub) 
        return "-";
    if (op == UnaryOp::Not) 
        return "!";
    assert("UnaryOp ERROR");
}


void UnaryExpr::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "UnaryExpr " << Unaryop_string(m_op) << '\n';
    if(m_operand)
        m_operand->print(out, indent + 2);
}

string UnaryExpr::to_string() const
{
    auto op = string{Unaryop_string(m_op)};
    return op + m_operand->to_string();
}

string_view Binaryop_string(BinaryOp op) 
{
    unsigned i = static_cast<unsigned>(op);
    if (op > BinaryOp::Or)
        return "<unknown_binary_op>";
    static constexpr std::string_view op_strs[] = {
            "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&&", "||"};
    return op_strs[i];
}

void BinaryExpr::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "BinaryExpr " << Binaryop_string(m_op) << '\n';

    if(m_lhs)
        m_lhs->print(out, indent + 2);
    if(m_rhs)
        m_rhs->print(out, indent + 2);
}

string BinaryExpr::to_string() const
{
    auto op = m_op;
    auto lhs = m_lhs->to_string();
    auto rhs = m_rhs->to_string();
    if (op == BinaryOp::Gt)
    {
        op = BinaryOp::Lt;
        std::swap(lhs, rhs);
    }
    if (op == BinaryOp::Geq)
    {
        op = BinaryOp::Leq;
        std::swap(lhs, rhs);
    }

    if (swappable(op) && !(lhs < rhs))
        std::swap(lhs, rhs);
    auto ops = string{Binaryop_string(op)};
    return lhs + ops + rhs;
}


void IntLiteral::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "IntLiteral " << m_value << '\n';
}

void FloatLiteral::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "FloatLiteral " << m_value << '\n';
}

void StringLiteral::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "StringLiteral " << m_value << '\n';
}

void Call::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Call " << m_func << '\n';

    for (const auto &arg : m_args)
    {
        std::visit([&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Expression>>) {
                arg->print(out, indent + 2);
            } else if constexpr (std::is_same_v<T, StringLiteral>) {
                arg.print(out, indent + 2);
            }
        }, arg);
    }
}

void ExprStmt::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "ExprStmt\n";
    if (m_expr)
        m_expr->print(out, indent + 2);
}

void Assignment::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Assignment\n";
    if(m_lhs)
        m_lhs->print(out, indent + 2);
    if(m_rhs)
        m_rhs->print(out, indent + 2);
}

void Initializer::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Initializer\n";

    std::visit([&](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<Expression>>) {
            assert(arg);  
            arg->print(out, indent + 2);
        } else if constexpr (std::is_same_v<T, std::vector<std::unique_ptr<Initializer>>>) {
            for (const auto &initializer : arg) {
                assert(initializer);  
                initializer->print(out, indent + 2);
            }
        }
    }, m_value);
}

void Declaration::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Declaration ";
    if (m_const_qualified)
        out << "const ";
    out << m_type << ' ' << m_ident << '\n';
    if (m_init)
        m_init->print(out, indent + 2);
}

void Block::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Block\n";

    for (const auto &child : m_children)
    {
        std::visit([&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Declaration>>) {
                assert(arg);  
                arg->print(out, indent + 2);
            } else if constexpr (std::is_same_v<T, std::unique_ptr<Statement>>) {
                assert(arg);  
                arg->print(out, indent + 2);
            }
        }, child);
    }
}

void IfElse::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "IfElse\n";

    print_indent(out, indent + 2);
    out << "Cond\n";
    if(m_cond)
        m_cond->print(out, indent + 2 * 2);

    print_indent(out, indent + 2);
    out << "Then\n";
    if(m_then)
        m_then->print(out, indent + 2 * 2);

    if (m_else)
    {
        print_indent(out, indent + 2);
        out << "Else\n";
        m_else->print(out, indent + 2 * 2);
    }
}

void While::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "While\n";

    print_indent(out, indent + 2);
    out << "Cond\n";
    if (m_cond)
        m_cond->print(out, indent + 2 * 2);

    print_indent(out, indent + 2);
    out << "Body\n";
    if(m_body)
        m_body->print(out, indent + 2 * 2);
}
void Break::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Break\n";
}

void Continue::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Continue\n";
}

void Return::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Return\n";
    if (m_res)
        m_res->print(out, indent + 2);
}

void Function::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "Function " << m_type << ' ' << m_ident;
    out << '(';

    bool first = true;
    for (const auto &param : m_params)
    {
        if (!first)
            out << ", ";
        first = false;
        assert(param);  
        out << *param;
    }
    out << ")\n";
    if (m_body) {
        m_body->print(out, indent + 2);
    } 
}


void CompileUnit::print(ostream &out, unsigned int indent) const
{
    print_indent(out, indent);
    out << "CompileUnit\n";
    
    for (const auto &child : m_children)
    {
        std::visit([&](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Declaration>>) {
                arg->print(out, indent + 2);
            } else if constexpr (std::is_same_v<T, std::unique_ptr<Function>>) {
                arg->print(out, indent + 2);
            }
        }, child);
    }
}


bool swappable(BinaryOp op)
{
    static const std::unordered_set<BinaryOp> swappable_ops = {
        BinaryOp::Add,BinaryOp::Mul,BinaryOp::Eq,BinaryOp::Neq,BinaryOp::Lt,BinaryOp::Gt,BinaryOp::Leq,BinaryOp::Geq
    };

    return swappable_ops.find(op) != swappable_ops.end();
}


std::string IntLiteral::to_string() const {
    return std::to_string(m_value);
}

std::string FloatLiteral::to_string() const {
    return std::to_string(m_value);
}


string LValue::to_string() const
{
    string s = m_ident.name();
    for (auto &index : m_indices)
    {
        s += "[";
        s += index->to_string();
        s += "]";
    }
    return s;
}

string Call::to_string() const
{
    std::string s = m_func.name() + "(";
    auto arg_string = [](const Argument &arg)
    {
        if (arg.index() == 0)
            return std::get<0>(arg)->to_string();
        else
            return std::get<1>(arg).value();
    };
    if (!m_args.empty())
        s += arg_string(m_args[0]);
    for (size_t i = 1; i < m_args.size(); ++i)
    {
        s += ", ";
        s += arg_string(m_args[i]);
    }
    s += ")";
    return s;
}