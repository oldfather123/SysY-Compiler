#include "Ast.h"
#include <iostream>

namespace ast {
    // Helper function to print indent
    void print_indent(std::ostream &out, unsigned indent) {
        out << std::string(indent, ' ');
    }

    // CompUnit
    std::string CompUnit::toString() const {
        return "CompUnit";
    }

    void CompUnit::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        for (const auto &child : children) {
            child->print(out, indent + 2);
        }
    }

    // BlockStmt
    std::string BlockStmt::toString() const {
        return "BlockStmt";
    }

    void BlockStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        for (const auto &stmt : stmts) {
            stmt->print(out, indent + 2);
        }
    }

    // ExprStmt
    std::string ExprStmt::toString() const {
        return "ExprStmt";
    }

    void ExprStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (expr) {
            expr->print(out, indent + 2);
        }
    }

    // DeclStmt
    std::string DeclStmt::toString() const {
        return "DeclStmt: " + identifier;
    }

    void DeclStmt::print(std::ostream &out, unsigned indent) const {
        print_indent(out, indent);

        out << "DeclStmt: " << identifier << " : ";
        switch (type.baseDataType()) {
            case PrimaryDataType::Void:
                out << "void";
                break;
            case PrimaryDataType::Int:
                out << "int";
                break;
            case PrimaryDataType::Float:
                out << "float";
                break;
            default:
                out << "<unsupported_type>";
                break;
        }

        for (int i = 0; i < type.arrayDimensionCount(); ++i) {
            out << "[" << type.arraySizes()[i] << "]";
        }

        if (initializer) {
            out << " = ";
            initializer->print(out, 0);
        }

        out << "\n";
    }
    // AssignStmt
    std::string AssignStmt::toString() const {
        return "AssignStmt";
    }

    void AssignStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (lvalue) {
            lvalue->print(out, indent + 2);
        }
        if (expr) {
            expr->print(out, indent + 2);
        }
    }

    // IfElseStmt
    std::string IfElseStmt::toString() const {
        return "IfElseStmt";
    }

    void IfElseStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (condition) {
            condition->print(out, indent + 2);
        }
        if (then_body) {
            then_body->print(out, indent + 2);
        }
        if (else_body) {
            else_body->print(out, indent + 2);
        }
    }

    // WhileStmt
    std::string WhileStmt::toString() const {
        return "WhileStmt";
    }

    void WhileStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (condition) {
            condition->print(out, indent + 2);
        }
        if (body) {
            body->print(out, indent + 2);
        }
    }

    // BreakStmt
    std::string BreakStmt::toString() const {
        return "BreakStmt";
    }

    // ContinueStmt
    std::string ContinueStmt::toString() const {
        return "ContinueStmt";
    }

    // ReturnStmt
    std::string ReturnStmt::toString() const {
        return "ReturnStmt";
    }

    void ReturnStmt::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (ret_expr) {
            ret_expr->print(out, indent + 2);
        }
    }

    // LValueExpr
    std::string LValueExpr::toString() const {
        return "LValueExpr: " + ident;
    }

    void LValueExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        for (const auto &index : indices) {
            index->print(out, indent + 2);
        }
    }

    // BinaryExpr
    std::string BinaryExpr::toString() const {
        return "BinaryExpr";
    }

    void BinaryExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (lhs) {
            lhs->print(out, indent + 2);
        }
        if (rhs) {
            rhs->print(out, indent + 2);
        }
    }

    // UnaryExpr
    std::string UnaryExpr::toString() const {
        std::string op_str;
        if (op == UnaryOp::Add) {
            op_str = "+";
        } else if (op == UnaryOp::Sub) {
            op_str = "-";
        } else {
            op_str = "!";
        }
        return "UnaryExpr:" + op_str;
    }

    void UnaryExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        if (operand) {
            operand->print(out, indent + 2);
        }
    }

    // IntLiteralExpr
    std::string IntLiteralExpr::toString() const {
        return "IntLiteralExpr: " + std::to_string(value);
    }

    void IntLiteralExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
    }

    // FloatLiteralExpr
    std::string FloatLiteralExpr::toString() const {
        return "FloatLiteralExpr: " + text;
    }

    void FloatLiteralExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
    }
    std::string CallExpr::toString() const {
        std::string str;
        str += callee + "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0)
                str += ", ";
            str += args[i]->toString();
        }
        str += ")";
        return str;
    }
    void CallExpr::print(std::ostream &out, unsigned indent) const {
        out << std::string(indent, ' ') << toString() << std::endl;
    }

    // StringLiteralExpr
    std::string StringLiteralExpr::toString() const {
        return "StringLiteralExpr: " + value;
    }

    void StringLiteralExpr::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
    }

    // Function
    std::string Function::toString() const {
        return "Function";
    }

    void Function::print(std::ostream &out, unsigned indent) const {
        Node::print(out, indent);
        // returnType 打印逻辑
        switch (returnType.baseDataType()) {
            case PrimaryDataType::Void:
                out << "void";
                break;
            case PrimaryDataType::Int:
                out << "int";
                break;
            case PrimaryDataType::Float:
                out << "float";
                break;
            default:
                break;
        }

        for (int i = 0; i < returnType.arrayDimensionCount(); ++i) {
            out << "[" << returnType.arraySizes()[i] << "]";
        }

        out << " " << identifier << "(";

        print_indent(out, indent + 2);
        out << "Parameters:\n";
        for (const auto &param : params) {
            param->print(out, indent + 4);
        }
        out << ")";

        if (body) {
            out << " ";
            body->print(out, indent);
        } else {
            out << ";";
        }

        out << "\n";
    }

}
