#include "frontend/ASTDumper.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <sstream>

namespace {

std::string childPrefix(const std::string &prefix, bool isLast) {
    return prefix + (isLast ? "  " : "| ");
}

std::string normalizedIntLiteralText(const std::string &text);
std::string normalizedFloatLiteralText(const std::string &text);

std::string varType(const ast::VarDecl &decl) {
    return ast::typeToString(decl.type);
}

std::string paramType(const ast::ParamDecl &decl) {
    return ast::typeToString(decl.type);
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

std::string normalizedIntLiteralText(const std::string &text) {
    try {
        return std::to_string(std::stoll(text, nullptr, 0));
    } catch (const std::exception &) {
        return text;
    }
}

std::string normalizedFloatLiteralText(const std::string &text) {
    errno = 0;
    char *end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (errno != 0 || end == text.c_str()) {
        return text;
    }

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6e", value);
    return buffer;
}

void appendExprType(std::ostringstream &out, const ast::Expr &expr,
                    const char *fallback = nullptr) {
    const std::string type = exprType(expr, fallback);
    if (!type.empty()) {
        out << " '" << type << "'";
    }
}

const char *declKind(const ast::Decl *decl) {
    if (decl == nullptr) {
        return nullptr;
    }
    if (dynamic_cast<const ast::VarDecl *>(decl) != nullptr) {
        return "Var";
    }
    if (dynamic_cast<const ast::ParamDecl *>(decl) != nullptr) {
        return "ParmVar";
    }
    if (dynamic_cast<const ast::FunctionDecl *>(decl) != nullptr) {
        return "Function";
    }
    return "Decl";
}

std::string declType(const ast::Decl *decl) {
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(decl)) {
        return varType(*var);
    }
    if (const auto *param = dynamic_cast<const ast::ParamDecl *>(decl)) {
        return paramType(*param);
    }
    if (const auto *func = dynamic_cast<const ast::FunctionDecl *>(decl)) {
        return functionType(*func);
    }
    return "";
}

} // namespace

ASTDumper::ASTDumper(std::ostream &out) : out_(out) {}

void ASTDumper::dump(const ast::TranslationUnit &unit) {
    out_ << "TranslationUnitDecl\n";
    for (std::size_t i = 0; i < unit.declarations.size(); ++i) {
        dumpDecl(*unit.declarations[i], "", i + 1 == unit.declarations.size());
    }
}

void ASTDumper::dumpDecl(const ast::Decl &decl, const std::string &prefix,
                         bool isLast) {
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(&decl)) {
        std::ostringstream text;
        text << "VarDecl ";
        text << var->name << " '" << varType(*var) << "'";
        writeNode(prefix, isLast, text.str());
        dumpVarDeclChildren(*var, prefix, isLast);
        return;
    }

    if (const auto *func = dynamic_cast<const ast::FunctionDecl *>(&decl)) {
        std::ostringstream text;
        text << "FunctionDecl ";
        text << func->name
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
        text << "CompoundStmt";
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
        text << "DeclStmt";
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < declStmt->declarations.size(); ++i) {
            dumpDecl(*declStmt->declarations[i], nextPrefix,
                     i + 1 == declStmt->declarations.size());
        }
        return;
    }

    if (dynamic_cast<const ast::NullStmt *>(&stmt) != nullptr) {
        std::ostringstream text;
        text << "NullStmt";
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        std::ostringstream text;
        text << "IfStmt";
        if (ifStmt->elseBranch != nullptr) {
            text << " has_else";
        }
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
        text << "WhileStmt";
        writeNode(prefix, isLast, text.str());

        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*whileStmt->condition, nextPrefix, false);
        dumpStmt(*whileStmt->body, nextPrefix, true);
        return;
    }

    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        std::ostringstream text;
        text << "BreakStmt";
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        std::ostringstream text;
        text << "ContinueStmt";
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt *>(&stmt)) {
        std::ostringstream text;
        text << "ReturnStmt";
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
        text << "IntegerLiteral";
        appendExprType(text, *literal, "int");
        text << ' ' << normalizedIntLiteralText(literal->text);
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *literal = dynamic_cast<const ast::FloatLiteral *>(&expr)) {
        std::ostringstream text;
        text << "FloatingLiteral";
        appendExprType(text, *literal, "float");
        text << ' ' << normalizedFloatLiteralText(literal->text);
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *literal = dynamic_cast<const ast::StringLiteral *>(&expr)) {
        std::ostringstream text;
        text << "StringLiteral";
        appendExprType(text, *literal, "string");
        text << ' ' << literal->text;
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(&expr)) {
        std::ostringstream text;
        text << "ImplicitCastExpr";
        appendExprType(text, *cast, ast::toString(cast->targetType.base));
        text << " <" << ast::toString(cast->kind) << ">";
        writeNode(prefix, isLast, text.str());
        dumpExpr(*cast->subExpr, childPrefix(prefix, isLast), true);
        return;
    }

    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        std::ostringstream text;
        text << "ParenExpr";
        appendExprType(text, *paren);
        if (dynamic_cast<const ast::DeclRefExpr *>(paren->subExpr.get()) != nullptr ||
            dynamic_cast<const ast::ArraySubscriptExpr *>(paren->subExpr.get()) != nullptr ||
            dynamic_cast<const ast::ParenExpr *>(paren->subExpr.get()) != nullptr) {
            text << " lvalue";
        }
        writeNode(prefix, isLast, text.str());
        dumpExpr(*paren->subExpr, childPrefix(prefix, isLast), true);
        return;
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        std::ostringstream text;
        text << "DeclRefExpr";
        if (const char *kind = declKind(ref->referencedDecl)) {
            const bool refersToFunction =
                dynamic_cast<const ast::FunctionDecl *>(ref->referencedDecl) != nullptr;
            const std::string referencedType = declType(ref->referencedDecl);
            if (!referencedType.empty()) {
                text << " '" << referencedType << "'";
            } else {
                appendExprType(text, *ref);
            }
            if (!refersToFunction) {
                text << " lvalue";
            }
            text << ' ' << kind << " '" << ref->name << "'";
            if (!referencedType.empty()) {
                text << " '" << referencedType << "'";
            }
        } else {
            appendExprType(text, *ref);
            text << " '" << ref->name << "'";
        }
        writeNode(prefix, isLast, text.str());
        return;
    }

    if (const auto *subscript = dynamic_cast<const ast::ArraySubscriptExpr *>(&expr)) {
        std::ostringstream text;
        text << "ArraySubscriptExpr";
        appendExprType(text, *subscript);
        if (subscript->hasType) {
            text << " lvalue";
        }
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*subscript->base, nextPrefix, false);
        dumpExpr(*subscript->index, nextPrefix, true);
        return;
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        std::ostringstream text;
        text << "UnaryOperator";
        if (unary->hasType) {
            text << " '" << ast::typeToString(unary->inferredType) << "'";
        }
        text << " prefix '" << ast::toString(unary->opcode) << "'";
        if (unary->opcode != ast::UnaryOpcode::Minus) {
            text << " cannot overflow";
        }
        writeNode(prefix, isLast, text.str());
        dumpExpr(*unary->operand, childPrefix(prefix, isLast), true);
        return;
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        std::ostringstream text;
        text << "BinaryOperator";
        appendExprType(text, *binary);
        text << " '" << ast::toString(binary->opcode) << "'";
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        dumpExpr(*binary->lhs, nextPrefix, false);
        dumpExpr(*binary->rhs, nextPrefix, true);
        return;
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        std::ostringstream text;
        text << "CallExpr";
        appendExprType(text, *call);
        text << " '" << call->callee << "'";
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        for (std::size_t i = 0; i < call->arguments.size(); ++i) {
            dumpExpr(*call->arguments[i], nextPrefix, i + 1 == call->arguments.size());
        }
        return;
    }

    if (const auto *list = dynamic_cast<const ast::InitListExpr *>(&expr)) {
        std::ostringstream text;
        text << "InitListExpr";
        appendExprType(text, *list);
        writeNode(prefix, isLast, text.str());
        const std::string nextPrefix = childPrefix(prefix, isLast);
        const std::size_t totalNodes =
            list->values.size() + (list->arrayFiller != nullptr ? 1U : 0U);
        std::size_t nodeIndex = 0;
        if (list->arrayFiller != nullptr) {
            ++nodeIndex;
            std::ostringstream filler;
            filler << "array_filler: ImplicitValueInitExpr '"
                   << ast::typeToString(list->arrayFiller->inferredType) << "'";
            writeNode(nextPrefix, nodeIndex == totalNodes, filler.str());
        }
        for (std::size_t i = 0; i < list->values.size(); ++i) {
            ++nodeIndex;
            dumpExpr(*list->values[i], nextPrefix, nodeIndex == totalNodes);
        }
        return;
    }

    if (const auto *implicitInit = dynamic_cast<const ast::ImplicitValueInitExpr *>(&expr)) {
        std::ostringstream text;
        text << "ImplicitValueInitExpr";
        appendExprType(text, *implicitInit, ast::toString(implicitInit->targetType.base));
        writeNode(prefix, isLast, text.str());
        return;
    }

    writeNode(prefix, isLast, "Expr <unknown>");
}

void ASTDumper::dumpVarDeclChildren(const ast::VarDecl &decl,
                                    const std::string &prefix,
                                    bool isLastParent) {
    const std::string nextPrefix = childPrefix(prefix, isLastParent);
    if (decl.initializer != nullptr) {
        dumpExpr(*decl.initializer, nextPrefix, true);
    }
}

void ASTDumper::dumpParamDeclChildren(const ast::ParamDecl &decl,
                                      const std::string &prefix,
                                      bool isLastParent) {
    (void)decl;
    (void)prefix;
    (void)isLastParent;
}

void ASTDumper::dumpParamDecl(const ast::ParamDecl &decl, const std::string &prefix,
                              bool isLast) {
    std::ostringstream text;
    text << "ParmVarDecl ";
    text << decl.name
         << " '" << paramType(decl) << "'";
    writeNode(prefix, isLast, text.str());
    dumpParamDeclChildren(decl, prefix, isLast);
}

void ASTDumper::writeNode(const std::string &prefix, bool isLast,
                          const std::string &text) {
    out_ << prefix << (isLast ? "`-" : "|-") << text << '\n';
}
