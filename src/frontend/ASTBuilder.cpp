#include "frontend/ASTBuilder.h"

#include "antlr4-runtime.h"

#include <utility>

std::unique_ptr<ast::TranslationUnit>
ASTBuilder::build(SysYParser::CompUnitContext *ctx) {
    auto unit = std::make_unique<ast::TranslationUnit>(locOf(ctx));
    for (antlr4::tree::ParseTree *child : ctx->children) {
        if (auto *decl = dynamic_cast<SysYParser::DeclContext *>(child)) {
            auto declarations = buildDecl(decl);
            for (auto &declaration : declarations) {
                unit->declarations.push_back(std::move(declaration));
            }
        } else if (auto *func = dynamic_cast<SysYParser::FuncDefContext *>(child)) {
            unit->declarations.push_back(buildFuncDef(func));
        }
    }
    return unit;
}

ast::SourceLocation ASTBuilder::locOf(antlr4::ParserRuleContext *ctx) const {
    if (ctx == nullptr || ctx->getStart() == nullptr) {
        return {};
    }
    return {static_cast<int>(ctx->getStart()->getLine()),
            static_cast<int>(ctx->getStart()->getCharPositionInLine())};
}

ast::Type ASTBuilder::bType(SysYParser::BTypeContext *ctx, bool isConst) const {
    ast::Type type;
    type.isConst = isConst;
    type.base = ctx->FLOAT() != nullptr ? ast::BuiltinType::Float : ast::BuiltinType::Int;
    return type;
}

ast::Type ASTBuilder::funcType(SysYParser::FuncTypeContext *ctx) const {
    ast::Type type;
    if (ctx->VOID() != nullptr) {
        type.base = ast::BuiltinType::Void;
    } else if (ctx->FLOAT() != nullptr) {
        type.base = ast::BuiltinType::Float;
    } else {
        type.base = ast::BuiltinType::Int;
    }
    return type;
}

std::vector<std::unique_ptr<ast::Decl>>
ASTBuilder::buildDecl(SysYParser::DeclContext *ctx) {
    std::vector<std::unique_ptr<ast::Decl>> declarations;
    if (ctx->constDecl() != nullptr) {
        auto vars = buildConstDecl(ctx->constDecl());
        for (auto &var : vars) {
            declarations.push_back(std::move(var));
        }
    } else {
        auto vars = buildVarDecl(ctx->varDecl());
        for (auto &var : vars) {
            declarations.push_back(std::move(var));
        }
    }
    return declarations;
}

std::vector<std::unique_ptr<ast::VarDecl>>
ASTBuilder::buildConstDecl(SysYParser::ConstDeclContext *ctx) {
    std::vector<std::unique_ptr<ast::VarDecl>> declarations;
    const ast::Type type = bType(ctx->bType(), true);
    for (SysYParser::ConstDefContext *def : ctx->constDef()) {
        auto declaration =
            std::make_unique<ast::VarDecl>(locOf(def), type, def->IDENT()->getText());
        for (SysYParser::ConstExpContext *dim : def->constExp()) {
            declaration->dimensions.push_back(buildConstExp(dim));
        }
        declaration->initializer = buildConstInitVal(def->constInitVal());
        declarations.push_back(std::move(declaration));
    }
    return declarations;
}

std::vector<std::unique_ptr<ast::VarDecl>>
ASTBuilder::buildVarDecl(SysYParser::VarDeclContext *ctx) {
    std::vector<std::unique_ptr<ast::VarDecl>> declarations;
    const ast::Type type = bType(ctx->bType(), false);
    for (SysYParser::VarDefContext *def : ctx->varDef()) {
        auto declaration =
            std::make_unique<ast::VarDecl>(locOf(def), type, def->IDENT()->getText());
        for (SysYParser::ConstExpContext *dim : def->constExp()) {
            declaration->dimensions.push_back(buildConstExp(dim));
        }
        if (def->initVal() != nullptr) {
            declaration->initializer = buildInitVal(def->initVal());
        }
        declarations.push_back(std::move(declaration));
    }
    return declarations;
}

std::unique_ptr<ast::FunctionDecl>
ASTBuilder::buildFuncDef(SysYParser::FuncDefContext *ctx) {
    auto declaration = std::make_unique<ast::FunctionDecl>(
        locOf(ctx), funcType(ctx->funcType()), ctx->IDENT()->getText());

    if (ctx->funcFParams() != nullptr) {
        for (SysYParser::FuncFParamContext *param : ctx->funcFParams()->funcFParam()) {
            declaration->params.push_back(buildFuncFParam(param));
        }
    }

    declaration->body = buildBlock(ctx->block());
    return declaration;
}

std::unique_ptr<ast::ParamDecl>
ASTBuilder::buildFuncFParam(SysYParser::FuncFParamContext *ctx) {
    auto parameter =
        std::make_unique<ast::ParamDecl>(locOf(ctx), bType(ctx->bType(), false),
                                         ctx->IDENT()->getText());
    parameter->isArray = !ctx->LBRACK().empty();
    for (SysYParser::ExpContext *dim : ctx->exp()) {
        parameter->dimensions.push_back(buildExp(dim));
    }
    return parameter;
}

std::unique_ptr<ast::CompoundStmt>
ASTBuilder::buildBlock(SysYParser::BlockContext *ctx) {
    auto block = std::make_unique<ast::CompoundStmt>(locOf(ctx));
    for (SysYParser::BlockItemContext *item : ctx->blockItem()) {
        block->statements.push_back(buildBlockItem(item));
    }
    return block;
}

std::unique_ptr<ast::Stmt>
ASTBuilder::buildBlockItem(SysYParser::BlockItemContext *ctx) {
    if (ctx->stmt() != nullptr) {
        return buildStmt(ctx->stmt());
    }

    auto statement = std::make_unique<ast::DeclStmt>(locOf(ctx->decl()));
    if (ctx->decl()->constDecl() != nullptr) {
        statement->declarations = buildConstDecl(ctx->decl()->constDecl());
    } else {
        statement->declarations = buildVarDecl(ctx->decl()->varDecl());
    }
    return statement;
}

std::unique_ptr<ast::Stmt> ASTBuilder::buildStmt(SysYParser::StmtContext *ctx) {
    if (ctx->block() != nullptr) {
        return buildBlock(ctx->block());
    }

    if (ctx->IF() != nullptr) {
        auto statement = std::make_unique<ast::IfStmt>(locOf(ctx));
        statement->condition = buildCond(ctx->cond());
        statement->thenBranch = buildStmt(ctx->stmt(0));
        if (ctx->ELSE() != nullptr) {
            statement->elseBranch = buildStmt(ctx->stmt(1));
        }
        return statement;
    }

    if (ctx->WHILE() != nullptr) {
        auto statement = std::make_unique<ast::WhileStmt>(locOf(ctx));
        statement->condition = buildCond(ctx->cond());
        statement->body = buildStmt(ctx->stmt(0));
        return statement;
    }

    if (ctx->BREAK() != nullptr) {
        return std::make_unique<ast::BreakStmt>(locOf(ctx));
    }

    if (ctx->CONTINUE() != nullptr) {
        return std::make_unique<ast::ContinueStmt>(locOf(ctx));
    }

    if (ctx->RETURN() != nullptr) {
        return std::make_unique<ast::ReturnStmt>(
            locOf(ctx), ctx->exp() == nullptr ? nullptr : buildExp(ctx->exp()));
    }

    if (ctx->lVal() != nullptr && ctx->ASSIGN() != nullptr) {
        return std::make_unique<ast::AssignStmt>(locOf(ctx), buildLVal(ctx->lVal()),
                                                 buildExp(ctx->exp()));
    }

    return std::make_unique<ast::ExprStmt>(
        locOf(ctx), ctx->exp() == nullptr ? nullptr : buildExp(ctx->exp()));
}

std::unique_ptr<ast::Expr> ASTBuilder::buildInitVal(SysYParser::InitValContext *ctx) {
    if (ctx->exp() != nullptr) {
        return buildExp(ctx->exp());
    }

    auto list = std::make_unique<ast::InitListExpr>(locOf(ctx));
    for (SysYParser::InitValContext *child : ctx->initVal()) {
        list->values.push_back(buildInitVal(child));
    }
    return list;
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildConstInitVal(SysYParser::ConstInitValContext *ctx) {
    if (ctx->constExp() != nullptr) {
        return buildConstExp(ctx->constExp());
    }

    auto list = std::make_unique<ast::InitListExpr>(locOf(ctx));
    for (SysYParser::ConstInitValContext *child : ctx->constInitVal()) {
        list->values.push_back(buildConstInitVal(child));
    }
    return list;
}

std::unique_ptr<ast::Expr> ASTBuilder::buildExp(SysYParser::ExpContext *ctx) {
    return buildAddExp(ctx->addExp());
}

std::unique_ptr<ast::Expr> ASTBuilder::buildCond(SysYParser::CondContext *ctx) {
    return buildLOrExp(ctx->lOrExp());
}

std::unique_ptr<ast::Expr> ASTBuilder::buildLVal(SysYParser::LValContext *ctx) {
    std::unique_ptr<ast::Expr> expression =
        std::make_unique<ast::DeclRefExpr>(locOf(ctx), ctx->IDENT()->getText());
    for (SysYParser::ExpContext *index : ctx->exp()) {
        expression = std::make_unique<ast::ArraySubscriptExpr>(
            locOf(index), std::move(expression), buildExp(index));
    }
    return expression;
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildPrimaryExp(SysYParser::PrimaryExpContext *ctx) {
    if (ctx->exp() != nullptr) {
        return buildExp(ctx->exp());
    }
    if (ctx->lVal() != nullptr) {
        return buildLVal(ctx->lVal());
    }
    return buildNumber(ctx->number());
}

std::unique_ptr<ast::Expr> ASTBuilder::buildNumber(SysYParser::NumberContext *ctx) {
    if (ctx->INT_CONST() != nullptr) {
        return std::make_unique<ast::IntegerLiteral>(locOf(ctx),
                                                     ctx->INT_CONST()->getText());
    }
    return std::make_unique<ast::FloatLiteral>(locOf(ctx), ctx->FLOAT_CONST()->getText());
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildUnaryExp(SysYParser::UnaryExpContext *ctx) {
    if (ctx->primaryExp() != nullptr) {
        return buildPrimaryExp(ctx->primaryExp());
    }

    if (ctx->IDENT() != nullptr) {
        auto call = std::make_unique<ast::CallExpr>(locOf(ctx), ctx->IDENT()->getText());
        if (ctx->funcRParams() != nullptr) {
            for (SysYParser::FuncRParamContext *param : ctx->funcRParams()->funcRParam()) {
                call->arguments.push_back(buildFuncRParam(param));
            }
        }
        return call;
    }

    ast::UnaryOpcode opcode = ast::UnaryOpcode::Plus;
    if (ctx->unaryOp()->MINUS() != nullptr) {
        opcode = ast::UnaryOpcode::Minus;
    } else if (ctx->unaryOp()->NOT() != nullptr) {
        opcode = ast::UnaryOpcode::LogicalNot;
    }
    return std::make_unique<ast::UnaryOperator>(locOf(ctx), opcode,
                                                buildUnaryExp(ctx->unaryExp()));
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildFuncRParam(SysYParser::FuncRParamContext *ctx) {
    if (ctx->STRING_CONST() != nullptr) {
        return std::make_unique<ast::StringLiteral>(locOf(ctx),
                                                    ctx->STRING_CONST()->getText());
    }
    return buildExp(ctx->exp());
}

std::unique_ptr<ast::Expr> ASTBuilder::buildMulExp(SysYParser::MulExpContext *ctx) {
    if (ctx->mulExp() == nullptr) {
        return buildUnaryExp(ctx->unaryExp());
    }

    ast::BinaryOpcode opcode = ast::BinaryOpcode::Mul;
    if (ctx->DIV() != nullptr) {
        opcode = ast::BinaryOpcode::Div;
    } else if (ctx->MOD() != nullptr) {
        opcode = ast::BinaryOpcode::Mod;
    }
    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), opcode, buildMulExp(ctx->mulExp()), buildUnaryExp(ctx->unaryExp()));
}

std::unique_ptr<ast::Expr> ASTBuilder::buildAddExp(SysYParser::AddExpContext *ctx) {
    if (ctx->addExp() == nullptr) {
        return buildMulExp(ctx->mulExp());
    }

    ast::BinaryOpcode opcode =
        ctx->MINUS() != nullptr ? ast::BinaryOpcode::Sub : ast::BinaryOpcode::Add;
    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), opcode, buildAddExp(ctx->addExp()), buildMulExp(ctx->mulExp()));
}

std::unique_ptr<ast::Expr> ASTBuilder::buildRelExp(SysYParser::RelExpContext *ctx) {
    if (ctx->relExp() == nullptr) {
        return buildAddExp(ctx->addExp());
    }

    ast::BinaryOpcode opcode = ast::BinaryOpcode::Less;
    if (ctx->GT() != nullptr) {
        opcode = ast::BinaryOpcode::Greater;
    } else if (ctx->LE() != nullptr) {
        opcode = ast::BinaryOpcode::LessEqual;
    } else if (ctx->GE() != nullptr) {
        opcode = ast::BinaryOpcode::GreaterEqual;
    }
    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), opcode, buildRelExp(ctx->relExp()), buildAddExp(ctx->addExp()));
}

std::unique_ptr<ast::Expr> ASTBuilder::buildEqExp(SysYParser::EqExpContext *ctx) {
    if (ctx->eqExp() == nullptr) {
        return buildRelExp(ctx->relExp());
    }

    ast::BinaryOpcode opcode =
        ctx->NEQ() != nullptr ? ast::BinaryOpcode::NotEqual : ast::BinaryOpcode::Equal;
    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), opcode, buildEqExp(ctx->eqExp()), buildRelExp(ctx->relExp()));
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildLAndExp(SysYParser::LAndExpContext *ctx) {
    if (ctx->lAndExp() == nullptr) {
        return buildEqExp(ctx->eqExp());
    }

    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), ast::BinaryOpcode::LogicalAnd, buildLAndExp(ctx->lAndExp()),
        buildEqExp(ctx->eqExp()));
}

std::unique_ptr<ast::Expr> ASTBuilder::buildLOrExp(SysYParser::LOrExpContext *ctx) {
    if (ctx->lOrExp() == nullptr) {
        return buildLAndExp(ctx->lAndExp());
    }

    return std::make_unique<ast::BinaryOperator>(
        locOf(ctx), ast::BinaryOpcode::LogicalOr, buildLOrExp(ctx->lOrExp()),
        buildLAndExp(ctx->lAndExp()));
}

std::unique_ptr<ast::Expr>
ASTBuilder::buildConstExp(SysYParser::ConstExpContext *ctx) {
    return buildAddExp(ctx->addExp());
}
