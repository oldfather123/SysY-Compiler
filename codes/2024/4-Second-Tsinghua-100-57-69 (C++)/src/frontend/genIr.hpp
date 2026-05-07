#include "frontend/ast.hpp"
#include "middleend/ir.hpp"
#include "frontend/funcEmitter.hpp"

namespace frontend{

using namespace middleend;

class GenIr{
private:
    std::unordered_map<Var *, DataMeta *> ast2ir_var;
    Module *module;

public:
    GenIr(){};
    virtual ~GenIr() {};

    Type getType(const ast::Function *func);
    Module *transform(const ast::CompileUnit &ast);
    void visitParameter(const ast::Parameter &param, FuncEmitter *funcEmitter);
    void visitStatement(const ast::Statement &stmt, FuncEmitter *funcEmitter);
    void visitDeclaration(const ast::Declaration &decl, FuncEmitter *funcEmitter);
    void visitIf(const ast::IfElse &if_stmt, FuncEmitter *funcEmitter);
    void visitWhile(const ast::While &while_stmt, FuncEmitter *funcEmitter);
    Temp *visitArithExpr(const ast::Expression *expr, FuncEmitter *funcEmitter, int literal_type = -1);
    Temp *visitLogicExpr(const ast::Expression *expr, BasicBlock *true_bb, BasicBlock *false_bb, FuncEmitter *funcEmitter);
    Temp *visitLValue(const ast::LValue *lvalue, FuncEmitter *funcEmitter, bool return_addr_when_scalar);
    int visitInitializer(Type var_type, DataMeta *data, ast::Initializer *initializer, int idx, FuncEmitter *funcEmitter, int max_size, bool is_zero_init);
    Temp *visitCast(Temp *temp, Type to_type, FuncEmitter *funcEmitter);
    ConstValue visitConstExpr(ast::Expression *expr);
    ConstValue visitConstLValue(ast::LValue *lvalue);
    std::vector<ConstValue> visitConstInitializer(ast::Initializer *initializer, Type var_type, int max_size);
};

} // namespace middleend