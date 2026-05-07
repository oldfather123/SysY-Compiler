#include "Common.h"
#include "AstVisitor.h"
#include <memory>
#include <string>
#include <typeinfo>

using namespace ast;
template <typename T>
using Ptr = std::shared_ptr<T>;

#undef dbgout
#define dbgout null_out // 禁用调试输出

// Convert the string that getText() returned to 'primaryDataType'
PrimaryDataType convertToPrimaryDataType(const std::string &typeStr) {
    if (typeStr == "int") {
        return PrimaryDataType::Int;
    } else if (typeStr == "float") {
        return PrimaryDataType::Float;
    } else if (typeStr == "void") {
        return PrimaryDataType::Void;
    } else {
        throw std::invalid_argument("Unknown type: " + typeStr);
    }
}

DataType getDataType(SysyParser::BTypeContext *bTypeCtx, const std::vector<int> &arrayLengths) {
    PrimaryDataType primaryDataType = convertToPrimaryDataType(bTypeCtx->getText());
    return DataType(primaryDataType, arrayLengths);
}

Ptr<ast::CompUnit> AstVisitor::compileUnit() {
    return m_comp_unit;
}

// visitCompUnit:Entry Of AST Constructor
antlrcpp::Any AstVisitor::visitCompUnit(SysyParser::CompUnitContext *const ctx) {
    // 符号表
    std::vector<Ptr<ast::Node>> children;

    auto declCtx = ctx->decl();
    for (auto declCtx : ctx->decl()) {
        auto any = declCtx->accept(this);
        try {
            auto decls = AS(any, std::vector<Ptr<ast::DeclStmt>>);
            // dbgout << "Type of decls: " << typeid(decls).name() << std::endl; // 调试信息
            for (auto d : decls) {
                children.emplace_back(d);
            }
        } catch (const std::bad_cast &e) {
            std::cerr << "Bad cast: " << e.what() << std::endl;
            std::cerr << "Actual type: " << typeid(any).name() << std::endl;
        }
    }

    // funcDef-Done
    for (auto funcDefCtx : ctx->funcDef()) {
        // 这里进入的是func的作用域，在visitFuncDef中会进入新的作用域
        auto func = AS(funcDefCtx->accept(this), Ptr<ast::Function>);

        // 将函数添加到该作用域，函数的形参也添加进去
        handleFunctionDef(func);
        // 里面的block是新的作用域，main作用域的子作用域指向func作用域，func作用域的父作用域指向main作用域

        // dbgout << "Type of func: " << typeid(func).name() << std::endl; // 调试信息
        children.emplace_back(func);
    }

    // 构建 CompUnit 节点
    auto compUnitNode = std::make_shared<ast::CompUnit>(std::move(children));
    // dbgout << "Returning CompUnitNode of type: " << typeid(compUnitNode).name() << std::endl; // 调试信息
    return compUnitNode;
}

//------------符号表函数-------------
int AstVisitor::handleFunctionDef(Ptr<ast::Function> func) {
    String name = func->identifier;

    // 设置形参类型
    Vector<Ptr<ast::DeclStmt>> params = func->params; // 获取形参列表
    Vector<DataType *> paramTypes;                    // 形参类型列表

    // params转换为paramTypes
    for (auto p : params) {
        paramTypes.push_back(&p->type);
    }

    return 0;
}
//------------Declaration------------
antlrcpp::Any AstVisitor::visitDecl(SysyParser::DeclContext *ctx) {
    if (ctx->constDecl()) {
        // dbgout << "const declaration" << std::endl;
        return visit(ctx->constDecl());
    } else if (ctx->varDecl()) {
        // dbgout << "var declaration" << std::endl;
        return visit(ctx->varDecl());
    }
    return nullptr;
}
//------Declaration:Const------
antlrcpp::Any AstVisitor::visitConstDecl(SysyParser::ConstDeclContext *ctx) {
    std::vector<Ptr<ast::DeclStmt>> decls;

    for (auto defCtx : ctx->constDef()) {
        std::string identifier = defCtx->Ident()->getText();
        std::vector<Ptr<ast::Expr>> array_lengths;

        for (auto expCtx : defCtx->constExp()) {
            array_lengths.push_back(AS(visit(expCtx), Ptr<ast::Expr>));
        }
        DataType type = convertToPrimaryDataType(ctx->bType()->getText()); // arraySizes 在生成IR时再指定
        type = type.constType();
        // 解析初始化值，区分单个和多个初始化值
        auto initValResult = defCtx->constInitVal()->accept(this);

        Ptr<ast::InitExpr> initializer;

        if (initValResult.IS(std::vector<Ptr<ast::InitExpr>>)) {
            // 复合初始化值
            std::vector<Ptr<ast::InitExpr>> initVals = AS(initValResult, std::vector<Ptr<ast::InitExpr>>);
            initializer = std::make_shared<ast::InitExpr>(initVals);
        } else {
            // 单一初始化值
            Ptr<ast::InitExpr> singleInit = AS(initValResult, Ptr<ast::InitExpr>);
            initializer = std::make_shared<ast::InitExpr>(singleInit);
        }

        auto decl = std::make_shared<ast::DeclStmt>(type, identifier, initializer, true);
        decl->arrayLengths = array_lengths;
        decls.push_back(decl);
    }

    return decls;
}

antlrcpp::Any AstVisitor::visitSingleConstInitVal(SysyParser::SingleConstInitValContext *const ctx) {
    // dbgout << "single init" << std::endl;

    auto ret = AS(ctx->constExp()->accept(this), Ptr<ast::Expr>);

    // dbgout << "single Done" << std::endl;
    return std::make_shared<ast::InitExpr>(ret);
}

antlrcpp::Any AstVisitor::visitMultiConstInitVal(SysyParser::MultiConstInitValContext *ctx) {
    std::vector<Ptr<ast::InitExpr>> initVals;
    for (auto initValCtx : ctx->constInitVal()) {
        auto initVal = visit(initValCtx); // 可能是 Ptr<ast::InitExpr> 或 std::vector<Ptr<ast::InitExpr>>
        if (initVal.IS(Ptr<ast::InitExpr>)) {
            initVals.push_back(AS(initVal, Ptr<ast::InitExpr>));
        } else if (initVal.IS(std::vector<Ptr<ast::InitExpr>>)) {
            // 对于多维数组嵌套，需要创建一个新的 InitExpr 来封装这个向量
            std::vector<Ptr<ast::InitExpr>> nestedVals = AS(initVal, std::vector<Ptr<ast::InitExpr>>);
            initVals.push_back(std::make_shared<ast::InitExpr>(nestedVals));
        }
    }
    return initVals; // 返回初始化表达式列表
}

//------Declaration:Var------
antlrcpp::Any AstVisitor::visitVarDecl(SysyParser::VarDeclContext *ctx) {

    // 获取基本类型
    PrimaryDataType primaryDataType = convertToPrimaryDataType(ctx->bType()->getText());
    // //dbgout << (primaryDataType == primaryDataType::Int ? "True" : "False") << std::endl;
    // 存储所有声明的变量
    auto decls = std::vector<Ptr<ast::DeclStmt>>();

    for (auto varDefCtx : ctx->varDef()) {
        std::string identifier;
        std::vector<Ptr<ast::Expr>> arrayLengths;
        Ptr<ast::InitExpr> initializer = nullptr;

        if (auto initVarDefCtx = dynamic_cast<SysyParser::InitVarDefContext *>(varDefCtx)) {
            identifier = initVarDefCtx->Ident()->getText();
            for (auto constExpCtx : initVarDefCtx->constExp()) {
                arrayLengths.push_back(AS(visit(constExpCtx), Ptr<ast::Expr>));
            }

            // 解析 initVal
            auto initValResult = initVarDefCtx->initVal()->accept(this);
            if (initValResult.IS(std::vector<Ptr<ast::InitExpr>>)) {
                // 对于复合初始化值，我们需要包装这些值在一个 InitExpr 对象中
                std::vector<Ptr<ast::InitExpr>> initVals = AS(initValResult, std::vector<Ptr<ast::InitExpr>>);
                initializer = std::make_shared<ast::InitExpr>(initVals);
            } else {
                // 对于单一初始化值，直接使用转换结果
                Ptr<ast::InitExpr> singleInit = AS(initValResult, Ptr<ast::InitExpr>);
                initializer = std::make_shared<ast::InitExpr>(singleInit);
            }
        } else if (auto noInitVarDefCtx = dynamic_cast<SysyParser::NoinitVarDefContext *>(varDefCtx)) {
            identifier = noInitVarDefCtx->Ident()->getText();
            for (auto constExpCtx : noInitVarDefCtx->constExp()) {
                arrayLengths.push_back(AS(visit(constExpCtx), Ptr<ast::Expr>));
            }
            // 无初始化值
            initializer = nullptr;
        }

        // 构造变量类型
        DataType varType(primaryDataType); // arraySizes 在生成IR时再指定

        // 创建 VarDecl 并加入声明列表
        auto decl = std::make_shared<ast::DeclStmt>(varType, identifier, initializer);
        decl->arrayLengths = arrayLengths;
        decls.push_back(decl);
    }

    return decls;
}

antlrcpp::Any AstVisitor::visitSingleInitVal(SysyParser::SingleInitValContext *const ctx) {
    auto ret = AS(ctx->exp()->accept(this), Ptr<ast::Expr>);
    return std::make_shared<ast::InitExpr>(ret);
}

antlrcpp::Any AstVisitor::visitMultiInitVal(SysyParser::MultiInitValContext *ctx) {
    std::vector<Ptr<ast::InitExpr>> initVals;
    for (auto initValCtx : ctx->initVal()) {
        auto initVal = visit(initValCtx); // 可能是 Ptr<ast::InitExpr> 或 std::vector<Ptr<ast::InitExpr>>
        if (initVal.IS(Ptr<ast::InitExpr>)) {
            initVals.push_back(AS(initVal, Ptr<ast::InitExpr>));
        } else if (initVal.IS(std::vector<Ptr<ast::InitExpr>>)) {
            // 对于更复杂的嵌套，需要创建一个新的 InitExpr 来封装这个向量
            std::vector<Ptr<ast::InitExpr>> nestedVals = AS(initVal, std::vector<Ptr<ast::InitExpr>>);
            initVals.push_back(std::make_shared<ast::InitExpr>(nestedVals));
        }
    }
    return initVals; // 返回初始化表达式列表
}

//------------Function------------
antlrcpp::Any AstVisitor::visitFuncDef(SysyParser::FuncDefContext *ctx) {
    PrimaryDataType primaryDataType = convertToPrimaryDataType(ctx->funcType()->getText());
    DataType returnType(primaryDataType);
    std::string identifier = ctx->Ident()->getText();
    std::vector<Ptr<ast::DeclStmt>> params;

    if (ctx->funcFParams()) {
        for (auto paramCtx : ctx->funcFParams()->funcFParam()) {
            auto paramDecl = AS(visit(paramCtx), Ptr<ast::DeclStmt>);
            paramDecl->isFuncParam = true;
            params.push_back(paramDecl);
        }
    }

    Ptr<ast::BlockStmt> body = AS(visit(ctx->block()), Ptr<ast::BlockStmt>);

    return std::make_shared<ast::Function>(returnType, identifier, params, body);
}

antlrcpp::Any AstVisitor::visitFuncType(SysyParser::FuncTypeContext *ctx) {
    PrimaryDataType primaryDataType = convertToPrimaryDataType(ctx->getText());
    return primaryDataType;
}

antlrcpp::Any AstVisitor::visitFuncFParams(SysyParser::FuncFParamsContext *ctx) {
    std::vector<Ptr<ast::DeclStmt>> params;
    for (auto paramCtx : ctx->funcFParam()) {
        auto paramDecl = AS(visit(paramCtx), Ptr<ast::DeclStmt>);
        paramDecl->isFuncParam = true;
        params.push_back(paramDecl);
    }
    return params;
}

antlrcpp::Any AstVisitor::visitFuncRParams(SysyParser::FuncRParamsContext *ctx) {
    std::vector<Ptr<ast::Expr>> params;
    for (auto expCtx : ctx->exp()) {
        auto param = AS(visit(expCtx), Ptr<ast::Expr>);
        params.push_back(param);
    }
    return params;
}

antlrcpp::Any AstVisitor::visitFuncFParam(SysyParser::FuncFParamContext *ctx) {
    PrimaryDataType primaryDataType = convertToPrimaryDataType(ctx->bType()->getText());
    std::string identifier = ctx->Ident()->getText();
    //
    std::vector<Ptr<ast::Expr>> array_lengths;

    if (ctx->epsilon()) {
        auto minusone = std::make_shared<ast::IntLiteralExpr>(-1);
        array_lengths.push_back(minusone); // -1 表示省略数组该维的长度
    }
    if (ctx->exp().size() > 0) {
        for (auto expCtx : ctx->exp()) {
            dbgout << expCtx->getText() << std::endl;
            array_lengths.push_back(AS(visit(expCtx), Ptr<ast::Expr>));
        }
    }

    DataType type(primaryDataType); // arraySizes 在生成IR时再指定
    auto decl = std::make_shared<ast::DeclStmt>(type, identifier, nullptr, false);
    decl->arrayLengths = array_lengths;
    return decl;
}

antlrcpp::Any AstVisitor::visitFuncCallUnaryExp(SysyParser::FuncCallUnaryExpContext *ctx) {
    std::string callee = ctx->Ident()->getText();
    auto funcRParams = ctx->funcRParams();
    std::vector<Ptr<ast::Expr>> args{};
    if (funcRParams) {
        args = AS(visit(funcRParams), std::vector<Ptr<ast::Expr>>);
    }
    auto callexpr = std::make_shared<ast::CallExpr>(callee, args);
    return static_cast<Ptr<ast::Expr>>(callexpr);
}

//------------Block------------
antlrcpp::Any AstVisitor::visitBlock(SysyParser::BlockContext *ctx) {
    // 进入新的作用域：因为可能ifStmt会在这里进入，
    // 所以如果在这里进入新的作用域，ifStmt的作用域就会是这个作用域的子作用域
    // m_symbol_table->EnterScope();
    std::vector<Ptr<ast::Stmt>> stmts;

    for (auto blockItemCtx : ctx->blockItem()) {
        // 将SysyParser中的DeclContext和StmtContext转换为ast::Stmt
        auto result = AS(visit(blockItemCtx), std::vector<Ptr<ast::Stmt>>);
        // 这里针对每一个stmt都需要注册符号表
        // 先打印符号表的当前作用域，此时的作用域应该是main
        stmts.insert(stmts.end(), result.begin(), result.end());
    }
    // std::cout << "stmts accept" << std::endl;
    // 退出作用域
    // m_symbol_table->ExitScope();
    return std::make_shared<ast::BlockStmt>(stmts);
}

antlrcpp::Any AstVisitor::visitBlockItem(SysyParser::BlockItemContext *ctx) {
    if (ctx->decl()) {
        // std::cout << "start block decl" << std::endl;

        auto ret = AS(ctx->decl()->accept(this), std::vector<Ptr<ast::DeclStmt>>);
        // std::cout << "decl accept" << std::endl;

        // 将 std::vector<Ptr<ast::DeclStmt>> 转换为 std::vector<Ptr<ast::Stmt>>
        std::vector<Ptr<ast::Stmt>> stmtVec;
        for (const auto &declStmt : ret) {
            stmtVec.push_back(static_cast<Ptr<ast::Stmt>>(declStmt));
        }
        // std::cout << "decl to stmt" << std::endl;
        return stmtVec;
    } else if (ctx->stmt()) {
        // 进入item符号表
        auto stmt = AS(ctx->stmt()->accept(this), Ptr<ast::Stmt>);
        std::vector<Ptr<ast::Stmt>> stmts;
        stmts.push_back(stmt);
        return stmts;
    }
    return std::vector<Ptr<ast::Stmt>>();
}

//------------Statement------------
antlrcpp::Any AstVisitor::visitAssignStmt(SysyParser::AssignStmtContext *ctx) {
    auto lvalue = AS(visit(ctx->lVal()), Ptr<ast::LValueExpr>);
    auto expr = AS(visit(ctx->exp()), Ptr<ast::Expr>);
    auto ret = std::make_shared<ast::AssignStmt>(lvalue, expr);

    dbgout << "Start assign: " << lvalue->ident << "=" << expr->toString() << std::endl;
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitExpStmt(SysyParser::ExpStmtContext *ctx) {
    if (ctx->exp()) {
        auto expr = AS(visit(ctx->exp()), Ptr<ast::Expr>);
        Ptr<ast::ExprStmt> ret = std::make_shared<ast::ExprStmt>(expr);
        ret->line = ctx->getStart()->getLine();
        return static_cast<Ptr<ast::Stmt>>(ret);
    } else {
        Ptr<ast::ExprStmt> ret = std::make_shared<ast::ExprStmt>(nullptr);
        ret->line = ctx->getStart()->getLine();
        return static_cast<Ptr<ast::Stmt>>(ret);
    }
}

antlrcpp::Any AstVisitor::visitBlockStmt(SysyParser::BlockStmtContext *ctx) {
    // visitBlock()
    auto ret = AS(ctx->block()->accept(this), Ptr<ast::BlockStmt>);
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitIfStmt(SysyParser::IfStmtContext *ctx) {
    // std::cout << "start if" << std::endl;
    auto condition = AS(visit(ctx->exp()), Ptr<ast::Expr>);
    // std::cout << "cond accept" << std::endl;

    auto thenBody = AS(visit(ctx->stmt()), Ptr<ast::Stmt>);
    // std::cout << "block stmt accept" << std::endl;
    auto ret = std::make_shared<ast::IfElseStmt>(condition, thenBody, nullptr);
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitIfElseStmt(SysyParser::IfElseStmtContext *ctx) {
    auto condition = AS(visit(ctx->exp()), Ptr<ast::Expr>);
    // 进入新的作用域
    auto thenBody = AS(visit(ctx->stmt(0)), Ptr<ast::Stmt>);

    // 进入新的作用域
    auto elseBody = AS(visit(ctx->stmt(1)), Ptr<ast::Stmt>);

    auto ret = std::make_shared<ast::IfElseStmt>(condition, thenBody, elseBody);
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitWhileStmt(SysyParser::WhileStmtContext *ctx) {
    auto condition = AS(visit(ctx->exp()), Ptr<ast::Expr>);
    // 进入新的作用域
    auto body = AS(visit(ctx->stmt()), Ptr<ast::Stmt>);

    auto ret = std::make_shared<ast::WhileStmt>(std::move(condition), std::move(body));
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitBreakStmt(SysyParser::BreakStmtContext *ctx) {
    auto ret = std::make_shared<ast::BreakStmt>();
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitContinueStmt(SysyParser::ContinueStmtContext *ctx) {
    auto ret = std::make_shared<ast::ContinueStmt>();
    return static_cast<Ptr<ast::Stmt>>(ret);
}

antlrcpp::Any AstVisitor::visitReturnStmt(SysyParser::ReturnStmtContext *ctx) {
    if (ctx->exp()) {
        auto expr = AS(visit(ctx->exp()), Ptr<ast::Expr>);
        auto ret = std::make_shared<ast::ReturnStmt>(expr);
        return static_cast<Ptr<ast::Stmt>>(ret);
    } else {
        auto ret = std::make_shared<ast::ReturnStmt>(nullptr);
        return static_cast<Ptr<ast::Stmt>>(ret);
    }
}

//------------Basic Functions------------
antlrcpp::Any AstVisitor::visitBType(SysyParser::BTypeContext *ctx) {
    PrimaryDataType primaryDataType = convertToPrimaryDataType(ctx->getText());
    return primaryDataType;
}

antlrcpp::Any AstVisitor::visitExp(SysyParser::ExpContext *ctx) {
    return visit(ctx->cond());
}

antlrcpp::Any AstVisitor::visitConstExp(SysyParser::ConstExpContext *ctx) {
    // std::cout << "start constexp" << std::endl;
    auto ret = AS(ctx->addExp()->accept(this), Ptr<ast::Expr>);
    ret->isConst = true;
    // std::cout << "constexp done" << std::endl;
    return ret;
}

//------------Value Function------------
antlrcpp::Any AstVisitor::visitLVal(SysyParser::LValContext *ctx) {
    // dbgout << "start lVal" << std::endl;
    std::string identifier = ctx->Ident()->getText();
    // dbgout << identifier << std::endl;
    std::vector<Ptr<ast::Expr>> indices;

    if (!ctx->exp().empty()) {
        // dbgout << "indices not empty" << std::endl;
        for (auto expCtx : ctx->exp()) {
            // dbgout << "start exp" << std::endl;
            auto index = AS(visit(expCtx), Ptr<ast::Expr>);
            indices.push_back(index);
        }
    } else {
        // dbgout << "empty indices" << std::endl;
    }
    auto ret = std::make_shared<ast::LValueExpr>(identifier, indices);
    // dbgout << "ret Done" << std::endl;
    return ret;
}

antlrcpp::Any AstVisitor::visitParensPrimaryExp(SysyParser::ParensPrimaryExpContext *ctx) {
    return visit(ctx->exp());
}

antlrcpp::Any AstVisitor::visitLValPrimaryExp(SysyParser::LValPrimaryExpContext *ctx) {
    auto ret = AS(ctx->lVal()->accept(this), Ptr<LValueExpr>);
    // dbgout << "primary to lVal done" << std::endl;
    //  return ret;错误的根源
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitNumberPrimaryExp(SysyParser::NumberPrimaryExpContext *ctx) {
    auto ret = AS(ctx->number()->accept(this), Ptr<NumberLiteralExpr>);
    // dbgout << "number accept" << std::endl;
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitNumber(SysyParser::NumberContext *ctx) {
    if (ctx->IntConst()) {
        // 根据前缀判断进制

        auto text = ctx->IntConst()->getText();
        int value = 0;
        if (text.find("0x") == 0 || text.find("0X") == 0) {
            value = std::stoi(text, nullptr, 16);
        } else if (text.find("0") == 0) {
            value = std::stoi(text, nullptr, 8);
        } else {
            value = std::stoi(text, nullptr, 10);
        }
        auto ret = std::make_shared<ast::IntLiteralExpr>(value);
        // dbgout << "int done" << std::endl;
        return static_cast<Ptr<NumberLiteralExpr>>(ret);
    }
    if (ctx->FloatConst()) {
        auto text = ctx->FloatConst()->getText();
        float value = std::stof(text);
        auto ret = std::make_shared<ast::FloatLiteralExpr>(text);
        // dbgout << "float done" << std::endl;
        return static_cast<Ptr<NumberLiteralExpr>>(ret);
    }
    return nullptr;
}

//------------Value Expression Handler------------
antlrcpp::Any AstVisitor::visitPrimaryUnaryExp(SysyParser::PrimaryUnaryExpContext *ctx) {
    // dbgout << "start primaryunary" << std::endl;
    auto ret = visit(ctx->primaryExp());
    // dbgout << "primaryunary done" << std::endl;
    return ret;
}

antlrcpp::Any AstVisitor::visitUnaryOpUnaryExp(SysyParser::UnaryOpUnaryExpContext *ctx) {
    auto sym = AS(visit(ctx->unaryOp()), ast::UnaryOp);
    std::string op = ctx->unaryOp()->getText();
    auto exp = AS(visit(ctx->unaryExp()), Ptr<ast::Expr>);
    auto ret = std::make_shared<ast::UnaryExpr>(sym, exp);
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitUnaryOp(SysyParser::UnaryOpContext *ctx) {
    std::string op = ctx->getText();
    if (op == "+") {
        return UnaryOp::Add;
    } else if (op == "-") {
        return UnaryOp::Sub;
    } else if (op == "!") {
        return UnaryOp::Not;
    } else {
        throw std::invalid_argument("Unknown unary operator: " + op);
    }
}

antlrcpp::Any AstVisitor::visitMulMulExp(SysyParser::MulMulExpContext *ctx) {
    auto lhs = AS(visit(ctx->mulExp()), Ptr<ast::Expr>);
    auto rhs = AS(visit(ctx->unaryExp()), Ptr<ast::Expr>);
    std::string op = ctx->children[1]->getText();
    BinaryOp binaryOp;
    if (op == "*") {
        binaryOp = BinaryOp::Mul;
    } else if (op == "/") {
        binaryOp = BinaryOp::Div;
    } else if (op == "%") {
        binaryOp = BinaryOp::Mod;
    } else {
        throw std::invalid_argument("Unknown binary operator: " + op);
    }
    auto ret = std::make_shared<ast::BinaryExpr>(binaryOp, lhs, rhs);
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitUnaryMulExp(SysyParser::UnaryMulExpContext *ctx) {
    // dbgout << "start unary" << std::endl;
    auto ret = visit(ctx->unaryExp());
    // dbgout << "unary done" << std::endl;
    return ret;
}

antlrcpp::Any AstVisitor::visitAddAddExp(SysyParser::AddAddExpContext *ctx) {
    // dbgout << "Start Add Add" << std::endl;
    auto lhs = AS(visit(ctx->addExp()), Ptr<ast::Expr>);
    // auto lhs = std::dynamic_pointer_cast<ast::Expr>(visit(ctx->addExp()));

    // dbgout << "step1" << std::endl;
    auto rhs = AS(visit(ctx->mulExp()), Ptr<ast::Expr>);
    // dbgout << "step2" << std::endl;

    std::string op = ctx->children[1]->getText();
    BinaryOp binaryOp;
    if (op == "+") {
        binaryOp = BinaryOp::Add;
        // dbgout << "+" << std::endl;
    } else if (op == "-") {
        binaryOp = BinaryOp::Sub;
    } else {
        throw std::invalid_argument("Unknown binary operator: " + op);
    }
    auto ret = std::make_shared<ast::BinaryExpr>(binaryOp, lhs, rhs);
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitMulAddExp(SysyParser::MulAddExpContext *ctx) {
    return visit(ctx->mulExp());
}

//------------Logical Handler------------
antlrcpp::Any AstVisitor::visitCond(SysyParser::CondContext *ctx) {
    return visit(ctx->lOrExp());
}

antlrcpp::Any AstVisitor::visitRelRelExp(SysyParser::RelRelExpContext *ctx) {
    auto lhs = AS(visit(ctx->relExp()), Ptr<ast::Expr>);
    auto rhs = AS(visit(ctx->addExp()), Ptr<ast::Expr>);
    std::string op = ctx->children[1]->getText(); // 获取操作符
    BinaryOp binaryOp;
    if (op == "<") {
        binaryOp = BinaryOp::Lt;
    } else if (op == ">") {
        binaryOp = BinaryOp::Gt;
    } else if (op == "<=") {
        binaryOp = BinaryOp::Leq;
    } else if (op == ">=") {
        binaryOp = BinaryOp::Geq;
    } else {
        throw std::invalid_argument("Unknown relational operator: " + op);
    }
    auto ret = std::make_shared<ast::BinaryExpr>(binaryOp, lhs, rhs);
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitAddRelExp(SysyParser::AddRelExpContext *ctx) {
    return visit(ctx->addExp());
}

antlrcpp::Any AstVisitor::visitEqEqExp(SysyParser::EqEqExpContext *ctx) {
    // std::cout << "start eq rel" << std::endl;
    auto lhs = AS(visit(ctx->eqExp()), Ptr<ast::Expr>);
    auto rhs = AS(visit(ctx->relExp()), Ptr<ast::Expr>);
    std::string op = ctx->children[1]->getText(); // 获取操作符
    BinaryOp binaryOp;
    if (op == "==") {
        binaryOp = BinaryOp::Eq;
    } else if (op == "!=") {
        binaryOp = BinaryOp::Neq;
    } else {
        throw std::invalid_argument("Unknown equality operator: " + op);
    }
    auto ret = std::make_shared<ast::BinaryExpr>(binaryOp, lhs, rhs);
    // std::cout << "rel eq accept" << std::endl;
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitRelEqExp(SysyParser::RelEqExpContext *ctx) {
    return visit(ctx->relExp());
}

antlrcpp::Any AstVisitor::visitEqLAndExp(SysyParser::EqLAndExpContext *ctx) {
    return visit(ctx->eqExp());
}

antlrcpp::Any AstVisitor::visitLAndLAndExp(SysyParser::LAndLAndExpContext *ctx) {
    auto lhs = AS(visit(ctx->lAndExp()), Ptr<ast::Expr>);
    auto rhs = AS(visit(ctx->eqExp()), Ptr<ast::Expr>);
    auto ret = std::make_shared<ast::BinaryExpr>(BinaryOp::And, lhs, rhs);
    return static_cast<Ptr<ast::Expr>>(ret);
}

antlrcpp::Any AstVisitor::visitLAndLOrExp(SysyParser::LAndLOrExpContext *ctx) {
    return visit(ctx->lAndExp());
}

antlrcpp::Any AstVisitor::visitLOrLOrExp(SysyParser::LOrLOrExpContext *ctx) {
    // std::cout << "start logical or" << std::endl;
    auto lhs = AS(visit(ctx->lOrExp()), Ptr<ast::Expr>);
    auto rhs = AS(visit(ctx->lAndExp()), Ptr<ast::Expr>);
    std::string op = ctx->children[1]->getText();

    BinaryOp binaryOp;
    if (op == "||") {
        binaryOp = BinaryOp::Or;
    } else {
        throw std::invalid_argument("Unknown logical operator: " + op);
    }

    auto ret = std::make_shared<ast::BinaryExpr>(binaryOp, lhs, rhs);
    return static_cast<Ptr<ast::Expr>>(ret);
}
