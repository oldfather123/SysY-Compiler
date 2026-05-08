#include "frontend/ASTDumper.h"

#include <ostream>
#include <sstream>

namespace {

std::string childPrefix(const std::string &prefix, bool isLast) {
    return prefix + (isLast ? "  " : "| ");
}

std::string location(ast::SourceLocation loc) {
    if (loc.line <= 0) {
        return "<invalid>";
    }
    std::ostringstream out;
    out << "<line:" << loc.line << ':' << loc.column << '>';
    return out.str();
}

std::string arraySuffix(std::size_t rank) {
    std::string suffix;
    for (std::size_t i = 0; i < rank; ++i) {
        suffix += "[]";
    }
    return suffix;
}

std::string varType(const ast::VarDecl &decl) {
    if (decl.type.isArray()) {
        return ast::typeToString(decl.type);
    }
    return ast::typeToString(decl.type) + arraySuffix(decl.dimensions.size());
}

std::string paramType(const ast::ParamDecl &decl) {
    if (decl.type.isArray()) {
        return ast::typeToString(decl.type);
    }
    const std::size_t fallbackRank = decl.isArray ? decl.dimensions.size() + 1 : 0;
    return ast::typeToString(decl.type) + arraySuffix(fallbackRank);
}

std::string functionType(const ast::FunctionDecl &decl) {
    std::string result = ast::typeToString(decl.returnType) + " (";
    for (std::size_t i = 0; i < decl.params.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += paramType(*decl.params[i]);
    }
    result += ')';
    return result;
}

std::string exprType(const ast::Expr &expr, const char *fallback = nullptr) {
    if (expr.hasType) {
        return ast::typeToString(expr.inferredType);
    }
    return fallback == nullptr ? "" : fallback;
}

void appendExprType(std::ostringstream &out, const ast::Expr &expr,
                    const char *fallback = nullptr) {
    const std::string type = exprType(expr, fallback);
    if (!type.empty()) {
        out << " '" << type << "'";
    }
}

} // namespace

ASTDumper::ASTDumper(std::ostream &out) : out_(out) {}

void ASTDumper::dump(const ast::TranslationUnit &unit) {
    out_ << "TranslationUnitDecl " << location(unit.location()) << '\n';
    for (std::size_t i = 0; i < unit.declarations.size(); ++i) {
        dumpDecl(*unit.declarations[i], "", i + 1 == unit.declarations.size());
    }
}

void ASTDumper::dumpDecl(const ast::Decl &decl, const std::string &prefix,
                         bool isLast) {
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(&decl)) {
        std::ostringstream text;
        text << "VarDecl " << location(var->location()) << " " << var->name
             << " '" << varType(*var) << "'";
        writeNode(prefix, isLast, text.str());
        dumpVarDeclChildren(*var, prefix, isLast);
        return;
    }

    if (const auto *func = dynamic_cast<const ast::FunctionDecl *>(&decl)) {
        std::ostringstream text;
        text << "FunctionDecl " << location(func->location()) << " " << func->name
             << " '" << functionType(*func) << "'";
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        const std::size_t childCount = func->params.size() + (func->body != nullptr ? 1 : 0);
        std::size_t childIndex = 0;
        for (const auto &param : func->params) {
            ++childIndex;
            dumpParamDecl(*param, nextPrefix, childIndex == childCount);
        }
        if (func->body != nullptr) {
            dumpStmt(*func->body, nextPrefix, true);
        }
        return;
    }

    writeNode(prefix, isLast, "Decl <unknown>");
}

void ASTDumper::dumpStmt(const ast::Stmt &stmt, const std::string &prefix,
                         bool isLast) {
    if (const auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        std::ostringstream text;
        text << "CompoundStmt " << location(compound->location());
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < compound->statements.size(); ++i) {
            dumpStmt(*compound->statements[i], nextPrefix,
                     i + 1 == compound->statements.size());
        }
        return;
    }

    if (const auto *declStmt = dynamic_cast<const ast::DeclStmt *>(&stmt)) {
        std::ostringstream text;
        text << "DeclStmt " << location(declStmt->location());
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < declStmt->declarations.size(); ++i) {
            dumpDecl(*declStmt->declarations[i], nextPrefix,
                     i + 1 == declStmt->declarations.size());
        }
        return;
    }

    if (const auto *exprStmt = dynamic_cast<const ast::ExprStmt *>(&stmt)) {
        std::ostringstream text;
        text << "ExprStmt " << location(exprStmt->location());
        writeNode(prefix, isLast, text.str());
        if (exprStmt->expression != nullptr) {
            dumpExpr(*exprStmt->expression, childPrefix(prefix, isLast), true);
        }
        return;
    }

    if (const auto *assign = dynamic_cast<const ast::AssignStmt *>(&stmt)) {
        std::ostringstream text;
        text << "AssignStmt " << location(assign->location());
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*assign->target, nextPrefix, false);
        dumpExpr(*assign->value, nextPrefix, true);
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        std::ostringstream text;
        text << "IfStmt " << location(ifStmt->location());
        writeNode(prefix, isLast, text.str());

        const bool hasElse = ifStmt->elseBranch != nullptr;
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*ifStmt->condition, nextPrefix, false);
        dumpStmt(*ifStmt->thenBranch, nextPrefix, !hasElse);
        if (hasElse) {
            dumpStmt(*ifStmt->elseBranch, nextPrefix, true);
        }
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        std::ostringstream text;
        text << "WhileStmt " << location(whileStmt->location());
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*whileStmt->condition, nextPrefix, false);
        dumpStmt(*whileStmt->body, nextPrefix, true);
        return;
    }

    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        std::ostringstream text;
        text << "BreakStmt " << location(stmt.location());
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        std::ostringstream text;
        text << "ContinueStmt " << location(stmt.location());
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt *>(&stmt)) {
        std::ostringstream text;
        text << "ReturnStmt " << location(returnStmt->location());
        writeNode(prefix, isLast, text.str());
        if (returnStmt->value != nullptr) {
            dumpExpr(*returnStmt->value, childPrefix(prefix, isLast), true);
        }
        return;
    }

    if (const auto *expr = dynamic_cast<const ast::Expr *>(&stmt)) {
        dumpExpr(*expr, prefix, isLast);
        return;
    }

    writeNode(prefix, isLast, "Stmt <unknown>");
}

void ASTDumper::dumpExpr(const ast::Expr &expr, const std::string &prefix,
                         bool isLast) {
    if (const auto *literal = dynamic_cast<const ast::IntegerLiteral *>(&expr)) {
        std::ostringstream text;
        text << "IntegerLiteral " << location(literal->location());
        appendExprType(text, *literal, "int");
        text << ' ' << literal->text;
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *literal = dynamic_cast<const ast::FloatLiteral *>(&expr)) {
        std::ostringstream text;
        text << "FloatingLiteral " << location(literal->location());
        appendExprType(text, *literal, "float");
        text << ' ' << literal->text;
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *literal = dynamic_cast<const ast::StringLiteral *>(&expr)) {
        std::ostringstream text;
        text << "StringLiteral " << location(literal->location());
        appendExprType(text, *literal, "string");
        text << ' ' << literal->text;
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        std::ostringstream text;
        text << "DeclRefExpr " << location(ref->location());
        appendExprType(text, *ref);
        text << ' ' << ref->name;
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(&expr)) {
        std::ostringstream text;
        text << "ArraySubscriptExpr " << location(subscript->location());
        appendExprType(text, *subscript);
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*subscript->base, nextPrefix, false);
        dumpExpr(*subscript->index, nextPrefix, true);
        return;
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        std::ostringstream text;
        text << "UnaryOperator " << location(unary->location()) << " '"
             << ast::toString(unary->opcode) << "'";
        appendExprType(text, *unary);
        writeNode(prefix, isLast, text.str());
        dumpExpr(*unary->operand, childPrefix(prefix, isLast), true);
        return;
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        std::ostringstream text;
        text << "BinaryOperator " << location(binary->location()) << " '"
             << ast::toString(binary->opcode) << "'";
        appendExprType(text, *binary);
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*binary->lhs, nextPrefix, false);
        dumpExpr(*binary->rhs, nextPrefix, true);
        return;
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        std::ostringstream text;
        text << "CallExpr " << location(call->location());
        appendExprType(text, *call);
        text << ' ' << call->callee;
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < call->arguments.size(); ++i) {
            dumpExpr(*call->arguments[i], nextPrefix, i + 1 == call->arguments.size());
        }
        return;
    }

    if (const auto *list = dynamic_cast<const ast::InitListExpr *>(&expr)) {
        std::ostringstream text;
        text << "InitListExpr " << location(list->location());
        appendExprType(text, *list);
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < list->values.size(); ++i) {
            dumpExpr(*list->values[i], nextPrefix, i + 1 == list->values.size());
        }
        return;
    }

    writeNode(prefix, isLast, "Expr <unknown>");
}

void ASTDumper::dumpVarDeclChildren(const ast::VarDecl &decl,
                                    const std::string &prefix,
                                    bool isLastParent) {
    const std::string nextPrefix = childPrefix(prefix, isLastParent);
    const std::size_t childCount =
        decl.dimensions.size() + (decl.initializer != nullptr ? 1 : 0);
    std::size_t childIndex = 0;

    for (const auto &dimension : decl.dimensions) {
        ++childIndex;
        dumpExpr(*dimension, nextPrefix, childIndex == childCount);
    }

    if (decl.initializer != nullptr) {
        dumpExpr(*decl.initializer, nextPrefix, true);
    }
}

void ASTDumper::dumpParamDecl(const ast::ParamDecl &decl, const std::string &prefix,
                              bool isLast) {
    std::ostringstream text;
    text << "ParmVarDecl " << location(decl.location()) << " " << decl.name
         << " '" << paramType(decl) << "'";
    writeNode(prefix, isLast, text.str());

    const std::string nextPrefix = childPrefix(prefix, isLast);
    for (std::size_t i = 0; i < decl.dimensions.size(); ++i) {
        dumpExpr(*decl.dimensions[i], nextPrefix, i + 1 == decl.dimensions.size());
    }
}

void ASTDumper::writeNode(const std::string &prefix, bool isLast,
                          const std::string &text) {
    out_ << prefix << (isLast ? "`-" : "|-") << text << '\n';
}
