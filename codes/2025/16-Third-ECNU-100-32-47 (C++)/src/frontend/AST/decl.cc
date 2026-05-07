#include "AST/decl.h"
#include "AST/expr.h"
#include "AST/stmt.h"
#include <ostream>
#include <sstream>
#include <string>

namespace aaac {
namespace frontend {

static void Indent(std::ostream &os, int d) {
    os << std::string(d,' ');
}

std::string Decl::getAsString() const {
    std::ostringstream os;
    PrintTo(os, 0);
    return os.str();
}

void TUDecl::PrintTo(std::ostream &os, int d) const  {
    for(Decl *decl : decls) {
        decl->PrintTo(os, d);
        if(typeid(*decl) != typeid(FuncDecl)) {
            os << ";";
        }
        os << "\n";
    }
}

void VarDecl::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    if(isConst()) {
        os << "const ";
    }
    os << BType << " " << getIdent();
    if(init) {
        os << " = ";
        init->PrintTo(os, 0);
    }
    // os <<";\n";
}

void ArrayDecl::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    if(isConst()) {
        os << "const ";
    }
    os << BType << " " << getIdent();
    for(Expr *subscript : subscripts) {
        os << "[";
        subscript->PrintTo(os, 0);
        os << "]";
    }
    if(init) {
        os << " = ";
        init->PrintTo(os, 0);
    }
    // os <<";\n";
}

void FuncDecl::PrintTo(std::ostream &os, int d) const {
    Indent(os, d);
    os << retType << " " << getIdent() << "(";
    for(FParmVarDecl *parm : *fparms) {
        if(parm != *fparms->begin()) {
            os << ", ";
        }
        parm->PrintTo(os, 0);
    }
    os << ") {\n";
    body->PrintTo(os, d + 4);
    os << "}\n";
}

DeclList* DeclList::patchTo(TUDecl *tu) {
    for(auto decl : patch_list) 
        tu->append(decl);
    if(func_decl) tu->append(func_decl);
    return this;
}

DeclList* DeclList::patchTo(CompondStmt *compond) {
    for(auto decl : patch_list) 
        compond->appendStmt(new DeclStmt(decl->getPos(),decl));
    return this;
}

VarDecl::~VarDecl() { delete init; }

ArrayDecl::~ArrayDecl() {
    for(Expr *expr : subscripts) delete expr; 
}

FuncDecl::~FuncDecl() {
    for(FParmVarDecl *parm : *fparms) delete parm;
    delete fparms;
    delete body; 
}

} // namespace frontend
} // namespace aaac