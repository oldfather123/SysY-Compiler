#include "AST/stmt.h"
#include "AST/expr.h"
#include "AST/decl.h"
#include <ostream>
#include <sstream>

namespace aaac {
namespace frontend {

AssignStmt::~AssignStmt() { delete var; delete expr; }

DeclStmt::~DeclStmt() { delete decl; };

CompondStmt::~CompondStmt() { 
    for(Stmt *stmt : stmts) delete stmt;
}

IfStmt::~IfStmt() { delete cond; delete then; delete elsee; }

WhileStmt::~WhileStmt() { delete cond; delete body; }

ReturnStmt::~ReturnStmt() { delete ret; }

static void Indent(std::ostream &os, int d) {
    os << std::string(d,' ');
}

std::string Stmt::getAsString() const {
    std::ostringstream os;
    PrintTo(os, 0);
    return os.str();
}

void AssignStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    var->PrintTo(os, 0);
    os << " = ";
    expr->PrintTo(os, 0);
    // os << ";\n";
}

void DeclStmt::PrintTo(std::ostream &os, int d) const {
    decl->PrintTo(os, d);
}

void CompondStmt::PrintTo(std::ostream &os, int d) const {
    // Indent(os, d);
    // os << "{\n";
    for(Stmt *stmt : stmts) {
        bool isCompound = false;
        if(typeid(stmt) == typeid(CompondStmt)) {
            isCompound = true;
        }
        if(isCompound) {
            Indent(os, d);
            os << "{\n";
        }
        stmt->PrintTo(os, d + 4 * isCompound);
        if(isCompound) {
            Indent(os, d);
            os << "}";
        } else {
            if(typeid(*stmt) != typeid(WhileStmt) &&
                typeid(*stmt) != typeid(IfStmt)) {
                os << ";";
            }
        }
        os << "\n";
    }
    // os << "\n";
    // Indent(os,d);
    // os << "}\n";
} 

void IfStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << "if (";
    cond->PrintTo(os, 0);
    os << ") {\n";
    then->PrintTo(os, d + 4);
    if(typeid(*then) != typeid(CompondStmt)) {
        os << ";\n";
    }
    Indent(os, d);
    os << "}";
    if(elsee) {
        os << " else {\n";
        elsee->PrintTo(os, d + 4);
        if(typeid(*elsee) != typeid(CompondStmt)) {
            os << ";\n";
        }
        Indent(os, d);
        os << "}";
    }
    // os << "\n";
}

void WhileStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << "while (";
    cond->PrintTo(os, 0);
    os << ") {\n";
    body->PrintTo(os, d + 4);
    Indent(os, d);
    os << "}";
}

void ReturnStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << "return ";
    if(ret) {
        ret->PrintTo(os, 0);
    }
    // os << ";\n";
}

void BreakStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << "break";
}

void ContinueStmt::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << "continue";
}

void EmptyStmt::PrintTo(std::ostream &os, int d) const {
    return;
}

} // namespace frontend
} // namespace aaac