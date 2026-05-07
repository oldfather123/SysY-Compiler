/*
  此文件实现了ast节点的访问操作函数，将antlr4生成的ast对应到我们实现的ast节点上（即在ast.hpp中定义的节点）
  本文件中需注意：antlrcpp::Any类型的返回值必须用as函数转换为具体类型的变量才能使用！
*/

#include <cassert>
#include <memory>
#include <string>

#include "common/utils.hpp"
#include "frontend/AstVisitor.hpp"
#include "frontend/ast.hpp"

using namespace frontend;

CompileUnit const &AstVisitor::compileUnit() const {
  return *this->m_compile_unit;
}

antlrcpp::Any
AstVisitor::visitCompUnit(SysYParser::CompUnitContext *const ctx) {
  std::vector<CompileUnit::Child> children;
  for (auto item : ctx->compUnitItem()) {
    // if is declaration
    if (auto decl = item->decl()) {
      auto const decls =
          std::any_cast<std::shared_ptr<std::vector<Declaration *>>>(decl->accept(this));
      for (auto d : *decls) {
        children.emplace_back(std::unique_ptr<Declaration>(d));
      }
    }
    // if is func 
    else if (auto func_ = item->funcDef()) {
      auto const func = std::any_cast<Function *>(func_->accept(this));
      children.emplace_back(std::unique_ptr<Function>(func));
    }
    // ??? 
    else {
      assert(false), "Error in visitCompUnit";
    }
  }
  auto compile_unit = new CompileUnit(std::move(children));//因为children中有unique_ptr所以要move
  m_compile_unit.reset(compile_unit);//因为是unique_ptr所以要转移所有权
  return compile_unit;
}

antlrcpp::Any
AstVisitor::visitConstDecl(SysYParser::ConstDeclContext *const ctx) {
  auto const base_type_new = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
  std::unique_ptr<ScalarType> base_type(base_type_new);
  std::vector<Declaration *> ret;
  for (auto def : ctx->constDef()) {
    Identifier ident(def->Ident()->getText());
    auto dimensions = this->visitDimensions(def->exp());
    std::unique_ptr<SysYType> type;
    if (dimensions.empty()) {
      type = std::make_unique<ScalarType>(*base_type);
    } 
    // is array
    else {
      type =
          std::make_unique<ArrayType>(*base_type, std::move(dimensions), false);
    }
    auto const init_new = std::any_cast<Initializer *>(def->initVal()->accept(this));
    std::unique_ptr<Initializer> init(init_new);
    ret.push_back(new Declaration(std::move(type), std::move(ident), std::move(init), true));
  }
  return std::make_shared<std::vector<Declaration *>>(std::move(ret));
}

antlrcpp::Any AstVisitor::visitInt(SysYParser::IntContext *const ctx) {
  return new ScalarType(Int);
}

antlrcpp::Any AstVisitor::visitFloat(SysYParser::FloatContext *const ctx) {
  return new ScalarType(Float);
}

antlrcpp::Any AstVisitor::visitVarDecl(SysYParser::VarDeclContext *const ctx) {
  auto const base_type_new = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
  std::unique_ptr<ScalarType> base_type(base_type_new);
  std::vector<Declaration*> ret;
  for(auto def: ctx->varDef()) {
    Identifier ident(def->Ident()->getText());
    auto dimensions = this->visitDimensions(def->exp());
    std::unique_ptr<SysYType> type;
    if(dimensions.empty()){
      type = std::make_unique<ScalarType>(*base_type);
    }else{
      type = std::make_unique<ArrayType>(*base_type, std::move(dimensions), false);
    }
    std::unique_ptr<Initializer> init;
    if(auto init_val = def->initVal()){
      init.reset(std::any_cast<Initializer *>(init_val->accept(this)));
    }
    ret.push_back(new Declaration(std::move(type), std::move(ident), std::move(init), false));
  }
  return std::make_shared<std::vector<Declaration *>>(std::move(ret));
}

antlrcpp::Any AstVisitor::visitInit(SysYParser::InitContext *const ctx) {
  auto expr_new = std::any_cast<Expression *>(ctx->exp()->accept(this));
  std::unique_ptr<Expression> expr(expr_new);
  return new Initializer(std::move(expr));
}

antlrcpp::Any
AstVisitor::visitInitList(SysYParser::InitListContext *const ctx) {
  std::vector<std::unique_ptr<Initializer>> values;
  auto init_list = ctx->initVal();
  for (auto init : init_list) {
    auto const value = std::any_cast<Initializer *>(init->accept(this));
    values.emplace_back(value);
  }
  return new Initializer(std::move(values));
}

antlrcpp::Any AstVisitor::visitFuncDef(SysYParser::FuncDefContext *const ctx) {
  auto const type_new = std::any_cast<ScalarType *>(ctx->funcType()->accept(this));
  std::unique_ptr<ScalarType> type(type_new);
  Identifier ident(ctx->Ident()->getText(), false);
  std::vector<std::unique_ptr<Parameter>> params;
  if (auto params_list = ctx->funcFParams()) {
    for (auto param_new : params_list->funcFParam()) {
      auto const param = std::any_cast<Parameter *>(param_new->accept(this));
      params.emplace_back(param);
    }
  }
  auto const body_new = std::any_cast<Block *>(ctx->block()->accept(this));
  std::unique_ptr<Block> body(body_new);
  return new Function(std::move(type), std::move(ident), std::move(params), std::move(body));
}

antlrcpp::Any AstVisitor::visitVoid(SysYParser::VoidContext *const ctx) {
  return static_cast<ScalarType *>(nullptr); // 将void指针也视为scalar指针方便处理
}

antlrcpp::Any
AstVisitor::visitScalarParam(SysYParser::ScalarParamContext *const ctx) {
  auto const type_new = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
  std::unique_ptr<SysYType> type(type_new);
  Identifier ident(ctx->Ident()->getText());
  return new Parameter(std::move(type), std::move(ident));
}

antlrcpp::Any AstVisitor::visitArrayParam(SysYParser::ArrayParamContext *ctx) {
  auto const basic_type_new = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
  std::unique_ptr<ScalarType> basic_type(basic_type_new);
  Identifier ident(ctx->Ident()->getText());
  auto dimensions = this->visitDimensions(ctx->exp());
  std::unique_ptr<SysYType> type(new ArrayType(*basic_type, std::move(dimensions), true));
  return new Parameter(std::move(type), std::move(ident));
}

antlrcpp::Any AstVisitor::visitBlock(SysYParser::BlockContext *const ctx) {
  std::vector<Block::Child> children;//每一个children中的元素代表一条语句
  for (auto item : ctx->blockItem()) {
    if (auto decl = item->decl()) {
      auto const decls = std::any_cast<std::shared_ptr<std::vector<Declaration *>>>(decl->accept(this));
      for (auto d : *decls) {
        children.emplace_back(std::unique_ptr<Declaration>(d));
      }
    } else if (auto stmt_new = item->stmt()) {
      auto const stmt = std::any_cast<Statement *>(stmt_new->accept(this));
      children.emplace_back(std::unique_ptr<Statement>(stmt));
    } else {
      assert(false), "Error in visitBlock";
    }
  }
  return new Block(std::move(children));
}

antlrcpp::Any AstVisitor::visitAssign(SysYParser::AssignContext *const ctx) {
  auto const lhs_new = std::any_cast<LValue *>(ctx->lVal()->accept(this));
  std::unique_ptr<LValue> lhs(lhs_new);
  auto const rhs_new = std::any_cast<Expression *>(ctx->exp()->accept(this));
  std::unique_ptr<Expression> rhs(rhs_new);
  auto const ret = new Assignment(std::move(lhs), std::move(rhs));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any
AstVisitor::visitExprStmt(SysYParser::ExprStmtContext *const ctx) {
  std::unique_ptr<Expression> expr;
  if (auto expr_new = ctx->exp()) {
    expr.reset(std::any_cast<Expression *>(expr_new->accept(this)));
  }
  auto const ret = new ExprStmt(std::move(expr));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any
AstVisitor::visitBlockStmt(SysYParser::BlockStmtContext *const ctx) {
  auto const ret = std::any_cast<Block *>(ctx->block()->accept(this));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any AstVisitor::visitIfElse(SysYParser::IfElseContext *const ctx) {
  auto const cond_new = std::any_cast<Expression *>(ctx->cond()->accept(this));
  std::unique_ptr<Expression> cond(cond_new);
  auto const then_new = std::any_cast<Statement *>(ctx->stmt(0)->accept(this));
  std::unique_ptr<Statement> then(then_new);
  std::unique_ptr<Statement> else_new;
  if (ctx->Else() != nullptr) {
    else_new.reset(std::any_cast<Statement *>(ctx->stmt(1)->accept(this)));
  }
  auto const ret = new IfElse(std::move(cond), std::move(then), std::move(else_new));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any AstVisitor::visitWhile(SysYParser::WhileContext *const ctx) {
  auto const cond_new = std::any_cast<Expression *>(ctx->cond()->accept(this));
  std::unique_ptr<Expression> cond(cond_new);
  auto const body_new = std::any_cast<Statement *>(ctx->stmt()->accept(this));
  std::unique_ptr<Statement> body(body_new);
  auto const ret = new While(std::move(cond), std::move(body));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any AstVisitor::visitBreak(SysYParser::BreakContext *const ctx) {
  auto const ret = new Break;
  return static_cast<Statement *>(ret);
}

antlrcpp::Any
AstVisitor::visitContinue(SysYParser::ContinueContext *const ctx) {
  auto const ret = new Continue;
  return static_cast<Statement *>(ret);
}

antlrcpp::Any AstVisitor::visitReturn(SysYParser::ReturnContext *const ctx) {
  std::unique_ptr<Expression> res;
  // if has return value
  if (auto exp = ctx->exp()) {
    res.reset(std::any_cast<Expression *>(exp->accept(this)));
  }
  auto const ret = new Return(std::move(res));
  return static_cast<Statement *>(ret);
}

antlrcpp::Any AstVisitor::visitLVal(SysYParser::LValContext *const ctx) {
  Identifier ident(ctx->Ident()->getText());
  std::vector<std::unique_ptr<Expression>> indices;
  // if is array
  for (auto exp : ctx->exp()) {
    auto const index = std::any_cast<Expression *>(exp->accept(this));
    indices.emplace_back(index);
  }
  return new LValue(std::move(ident), std::move(indices));
}

antlrcpp::Any
AstVisitor::visitPrimaryExp_(SysYParser::PrimaryExp_Context *const ctx) {
  // primaryexp_ 分为两种,Lparen exp Rparen和number
  if (ctx->number()) {
    return ctx->number()->accept(this);
  } else {
    assert(ctx->exp()), "Error in visit PrimaryExp_";
    return ctx->exp()->accept(this);
  }
}

antlrcpp::Any
AstVisitor::visitLValExpr(SysYParser::LValExprContext *const ctx) {
  auto const lval = std::any_cast<LValue *>(ctx->lVal()->accept(this));
  return static_cast<Expression *>(lval);
}

antlrcpp::Any
AstVisitor::visitDecIntConst(SysYParser::DecIntConstContext *const ctx) {
  return int(std::stoll(ctx->getText(), nullptr, 10));
}

antlrcpp::Any
AstVisitor::visitOctIntConst(SysYParser::OctIntConstContext *const ctx) {
  return int(std::stoll(ctx->getText(), nullptr, 8));
}

antlrcpp::Any
AstVisitor::visitHexIntConst(SysYParser::HexIntConstContext *const ctx) {
  return int(std::stoll(ctx->getText(), nullptr, 16));
}

antlrcpp::Any
AstVisitor::visitDecFloatConst(SysYParser::DecFloatConstContext *const ctx) {
  return std::stof(ctx->getText());//stof函数支持八进制和十六进制
}

antlrcpp::Any
AstVisitor::visitHexFloatConst(SysYParser::HexFloatConstContext *const ctx) {
  return std::stof(ctx->getText());
}

antlrcpp::Any AstVisitor::visitCall(SysYParser::CallContext *const ctx) {
  Identifier ident(ctx->Ident()->getText(), false);//函数不允许mangle
  std::vector<Call::Argument> args;
  auto args_ctx = ctx->funcRParams();
  if (args_ctx) {
    for (auto arg_new : args_ctx->funcRParam()) {
      // funcRParam : exp | stringConst
      if (auto exp = arg_new->exp()) {
        auto const arg = std::any_cast<Expression *>(exp->accept(this));
        args.emplace_back(std::unique_ptr<Expression>(arg));
      } else if (auto str = arg_new->stringConst()) {
        auto arg = std::any_cast<std::shared_ptr<std::string>>(str->accept(this));
        args.emplace_back(std::move(*arg));
      } else {
        assert(false), "Error in visitCall";
      }
    }
  }
  auto const ret =
      new Call(std::move(ident), std::move(args), ctx->getStart()->getLine());
  return static_cast<Expression *>(ret);
}

antlrcpp::Any
AstVisitor::visitUnaryAdd(SysYParser::UnaryAddContext *const ctx) {
  auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new UnaryExpr(UnaryOp::Add, std::unique_ptr<Expression>(operand));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any
AstVisitor::visitUnarySub(SysYParser::UnarySubContext *const ctx) {
  auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new UnaryExpr(UnaryOp::Sub, std::unique_ptr<Expression>(operand));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitNot(SysYParser::NotContext *const ctx) {
  auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new UnaryExpr(UnaryOp::Not, std::unique_ptr<Expression>(operand));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any
AstVisitor::visitStringConst(SysYParser::StringConstContext *const ctx) {
  return std::make_shared<std::string>(ctx->getText());
}

antlrcpp::Any AstVisitor::visitMul(SysYParser::MulContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Mul, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitDiv(SysYParser::DivContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Div, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitMod(SysYParser::ModContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Mod, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitAdd(SysYParser::AddContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Add, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitSub(SysYParser::SubContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Sub, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitLt(SysYParser::LtContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Lt, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitGt(SysYParser::GtContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Gt, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitLeq(SysYParser::LeqContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Leq, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitGeq(SysYParser::GeqContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Geq, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitEq(SysYParser::EqContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Eq, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitNeq(SysYParser::NeqContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Neq, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitAnd(SysYParser::AndContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->lAndExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::And, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitOr(SysYParser::OrContext *const ctx) {
  auto const lhs = std::any_cast<Expression *>(ctx->lOrExp()->accept(this));
  auto const rhs = std::any_cast<Expression *>(ctx->lAndExp()->accept(this));
  auto const ret =
      new BinaryExpr(BinaryOp::Or, std::unique_ptr<Expression>(lhs),
                     std::unique_ptr<Expression>(rhs));
  return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitNumber(SysYParser::NumberContext *const ctx) {
  if (ctx->intConst()) {
    auto val = std::any_cast<IntLiteral::Value>(ctx->intConst()->accept(this));//int32_t类型，32位有符号整数
    auto literal = new IntLiteral{val};
    return static_cast<Expression *>(literal);
  }
  if (ctx->floatConst()) {
    auto val = std::any_cast<FloatLiteral::Value>(ctx->floatConst()->accept(this));
    auto literal = new FloatLiteral{val};
    return static_cast<Expression *>(literal);
  }
  assert(false), "Error in visitNumber";
  return static_cast<Expression *>(nullptr);
}

std::vector<std::unique_ptr<Expression>>
AstVisitor::visitDimensions(const std::vector<SysYParser::ExpContext *> &ctxs) {
  std::vector<std::unique_ptr<Expression>> ret;
  for (auto dim : ctxs) {
    auto expr_new = std::any_cast<Expression *>(dim->accept(this));
    ret.emplace_back(expr_new);
  }
  return ret;
}
