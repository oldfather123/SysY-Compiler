#include <cassert>
#include <memory>
#include <string>
#include <any>

#include "AstVisitor.h"
#include "ast.h"

using namespace frontend;

std::unique_ptr<CompileUnit> AstVisitor::compileUnit()
{
    return std::move(m_compile_unit);
}

// 访问语法树中的 CompUnit 节点，返回一个 CompileUnit 指针封装在 antlrcpp::Any 中
antlrcpp::Any AstVisitor::visitCompUnit(SysyParser::CompUnitContext *const ctx)
{
    // 创建一个 CompileUnit::Child 类型的 vector 用于存放编译单元的所有子元素（函数或变量声明）

    std::vector<CompileUnit::Child> children;

    for (auto item : ctx->compUnitItem())
    {
        if (auto decl = item->decl())
        {
            // 调用 visitor 模式访问 decl 节点，并将返回值（std::vector<Declaration *>）转换为 shared_ptr
            auto const decls =
                std::any_cast<std::shared_ptr<std::vector<Declaration *>>>(decl->accept(this));

            // 遍历所有声明，并将每个裸指针转为 unique_ptr 后，加入 children 向量中
            for (auto d : *decls)
            {
                children.emplace_back(std::unique_ptr<Declaration>(d));
            }
        }
        else if (auto func_ = item->funcDef())
        {
            // 调用 visitor 模式访问 funcDef 节点，并将返回值转换为 Function*
            auto const func = std::any_cast<Function *>(func_->accept(this));

            // 将 Function 指针包装为 unique_ptr 并加入 children 向量
            children.emplace_back(std::unique_ptr<Function>(func));
        }
        // 如果既不是声明也不是函数定义，说明语法树结构异常
        else
        {
            assert(false); // 触发断言错误
        }
    }

    // 所有子节点处理完毕，构造 CompileUnit 对象，并将 children 移交给它
    auto compile_unit = new CompileUnit(std::move(children));

    // 将结果保存到成员变量 m_compile_unit 中（通常用于后续访问语义树根节点）
    m_compile_unit.reset(compile_unit);

    // 将 compile_unit 返回给 ANTLR 的 visitor 调用者
    return compile_unit;
}

// 访问语法树中的 ConstDecl 节点，返回多个声明（Declaration*）组成的 vector，封装在 shared_ptr 中
antlrcpp::Any AstVisitor::visitConstDecl(SysyParser::ConstDeclContext *const ctx)
{
    // 获取基本类型（int 或 float），通过访问 bType 子节点，并进行类型转换
    auto const base_type_ = std::any_cast<ScalarType *>(ctx->bType()->accept(this));

    // 将原始指针封装成 unique_ptr 以确保资源自动管理
    std::unique_ptr<ScalarType> base_type(base_type_);

    // 用于存储每个 const 定义对应的语义声明对象
    std::vector<Declaration *> ret;

    // 遍历所有 constDef（可能有多个 const a = 1, b = 2 的形式）
    for (auto def : ctx->constDef())
    {
        // 从 constDef 中提取标识符文本构造 Identifier 对象
        Identifier ident(def->Ident()->getText());

        // 获取维度信息（如果是数组则会返回多个维度，否则为空）
        auto dimensions = this->visitDimensions(def->exp());

        std::unique_ptr<SysYType> type;
        if (dimensions.empty())
        {
            // 如果没有维度信息，说明是一个普通的标量类型
            type = std::make_unique<ScalarType>(*base_type);
        }
        else
        {
            // 否则构造一个 ArrayType 对象（false 表示不是变量数组，是 const）
            type = std::make_unique<ArrayType>(*base_type, std::move(dimensions), false);
        }

        // 获取初值（Initializer 指针）
        auto const init_ = std::any_cast<Initializer *>(def->initVal()->accept(this));
        std::unique_ptr<Initializer> init(init_);

        // 构造 Declaration 对象（true 表示这是 const 声明），并加入结果向量
        ret.push_back(new Declaration(std::move(type), std::move(ident),
                                      std::move(init), true));
    }

    // 返回所有 const 声明组成的 vector，封装在 shared_ptr 中以便后续统一管理
    return std::make_shared<std::vector<Declaration *>>(std::move(ret));
}

// 访问 bType 中的 int 类型节点，返回一个 ScalarType 对象指针（值为 Int）
antlrcpp::Any AstVisitor::visitInt(SysyParser::IntContext *const ctx)
{
    return new ScalarType(Int);
}
// 访问 bType 中的 float 类型节点，返回一个 ScalarType 对象指针（值为 Float）
antlrcpp::Any AstVisitor::visitFloat(SysyParser::FloatContext *const ctx)
{
    return new ScalarType(Float);
}

// 访问语法树中的 VarDecl 节点，返回一个变量声明列表（vector<Declaration*>），用 shared_ptr 封装管理
antlrcpp::Any AstVisitor::visitVarDecl(SysyParser::VarDeclContext *const ctx)
{
    // 访问 bType 节点，得到变量的基础类型（如 int 或 float）
    auto const base_type_ = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
    // 使用 unique_ptr 管理 base_type 的生命周期
    std::unique_ptr<ScalarType> base_type(base_type_);

    // 用于存储多个变量定义对应的语义层声明对象
    std::vector<Declaration *> ret;

    // 遍历每一个变量定义（例如 int a, b[2], c = 1; 就有多个 varDef）
    for (auto def : ctx->varDef())
    {
        // 获取变量名并构造 Identifier 对象
        Identifier ident(def->Ident()->getText());

        // 获取维度信息（如果是数组，返回多个维度；否则为空）
        auto dimensions = this->visitDimensions(def->exp());

        std::unique_ptr<SysYType> type;
        if (dimensions.empty())
        {
            // 若没有维度信息，则是标量变量，构造 ScalarType 类型对象
            type = std::make_unique<ScalarType>(*base_type);
        }
        else
        {
            // 否则构造 ArrayType 数组类型（注意：false 表示不是 const）
            type = std::make_unique<ArrayType>(*base_type, std::move(dimensions), false);
        }

        std::unique_ptr<Initializer> init;
        if (auto init_val = def->initVal())
        {
            // 若 varDef 有初始化部分，访问 initVal 节点并转换为 Initializer*
            init.reset(std::any_cast<Initializer *>(init_val->accept(this)));
        }

        // 构造变量声明对象，并加入结果列表中（false 表示不是 const）
        ret.push_back(new Declaration(std::move(type), std::move(ident),
                                      std::move(init), false));
    }

    // 将所有 Declaration* 封装成 vector 并用 shared_ptr 管理后返回
    return std::make_shared<std::vector<Declaration *>>(std::move(ret));
}

// 处理初始化表达式，如 int a = 3;
antlrcpp::Any AstVisitor::visitInit(SysyParser::InitContext *const ctx)
{
    // 访问表达式子节点，并转换为 Expression* 类型
    auto expr_ = std::any_cast<Expression *>(ctx->exp()->accept(this));
    // 将裸指针转为智能指针，管理内存
    std::unique_ptr<Expression> expr(expr_);
    // 创建并返回一个 Initializer 节点
    return new Initializer(std::move(expr));
}

// 处理初始化列表，如 int a[3] = {1, 2, 3};
antlrcpp::Any AstVisitor::visitInitList(SysyParser::InitListContext *const ctx)
{
    std::vector<std::unique_ptr<Initializer>> values;
    // 遍历每个 initVal（即列表中的每个初始化项）
    for (auto init : ctx->initVal())
    {
        // 访问子节点并转换为 Initializer* 类型
        auto const value = std::any_cast<Initializer *>(init->accept(this));
        // 将其包装为智能指针并加入到 vector 中
        values.emplace_back(value);
    }
    // 创建并返回一个列表类型的 Initializer 节点
    return new Initializer(std::move(values));
}

// 处理函数定义，如 int f(int x) { return x + 1; }
antlrcpp::Any AstVisitor::visitFuncDef(SysyParser::FuncDefContext *const ctx)
{
    // 访问函数返回类型，转换为 ScalarType*
    auto const type_ = std::any_cast<ScalarType *>(ctx->funcType()->accept(this));
    std::unique_ptr<ScalarType> type(type_); // 使用智能指针管理类型对象

    // 提取函数名（Identifier），并指定其不是临时变量
    Identifier ident(ctx->Ident()->getText(), false);

    std::vector<std::unique_ptr<Parameter>> params;
    // 如果存在参数列表，访问每个参数
    if (auto params_ = ctx->funcFParams())
    {
        for (auto param_ : params_->funcFParam())
        {
            // 访问参数，转换为 Parameter*
            auto const param = std::any_cast<Parameter *>(param_->accept(this));
            params.emplace_back(param); // 添加到参数列表中
        }
    }

    // 访问函数体 Block 节点
    auto const body_ = std::any_cast<Block *>(ctx->block()->accept(this));
    std::unique_ptr<Block> body(body_);

    // 构造并返回 Function AST 节点
    return new Function(std::move(type), std::move(ident), std::move(params),
                        std::move(body));
}

// 处理 void 类型（作为返回类型）
antlrcpp::Any AstVisitor::visitVoid(SysyParser::VoidContext *const ctx)
{
    return static_cast<ScalarType *>(nullptr);
}

// 处理标量（非数组）函数参数，如 int x
antlrcpp::Any AstVisitor::visitScalarParam(SysyParser::ScalarParamContext *const ctx)
{
    // 获取基础类型（如 int、float），转换为 ScalarType*
    auto const type_ = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
    std::unique_ptr<SysYType> type(type_);     // 包装为 SysYType 智能指针
    Identifier ident(ctx->Ident()->getText()); // 获取标识符名
    // 构造 Parameter 节点并返回
    return new Parameter(std::move(type), std::move(ident));
}

// 处理数组类型的函数参数，如 int a[10][20]
antlrcpp::Any AstVisitor::visitArrayParam(SysyParser::ArrayParamContext *ctx)
{
    // 获取基础类型（如 int）
    auto const basic_type_ = std::any_cast<ScalarType *>(ctx->bType()->accept(this));
    std::unique_ptr<ScalarType> basic_type(basic_type_);

    // 获取数组名
    Identifier ident(ctx->Ident()->getText());

    // 获取数组维度列表（通过访问所有 exp() 表达式）
    auto dimensions = this->visitDimensions(ctx->exp());

    // 构造完整的数组类型（带维度和是否为参数信息）
    std::unique_ptr<SysYType> type(
        new ArrayType(*basic_type, std::move(dimensions), true));

    // 构造 Parameter 节点并返回
    return new Parameter(std::move(type), std::move(ident));
}

// 处理语法规则 block（复合语句块），即 {...}
antlrcpp::Any AstVisitor::visitBlock(SysyParser::BlockContext *const ctx)
{
    std::vector<Block::Child> children; // 存储 Block 中的语句或声明节点（Block::Child 是变体类型）

    // 遍历所有语句块项 blockItem
    for (auto item : ctx->blockItem())
    {
        // 如果是声明语句（如 int a = 1;）
        if (auto decl = item->decl())
        {
            // 接收声明，结果是一个指向 Declaration* 列表的 shared_ptr
            auto const decls =
                std::any_cast<std::shared_ptr<std::vector<Declaration *>>>(decl->accept(this));
            // 将每个裸指针封装成 unique_ptr 并添加进 children
            for (auto d : *decls)
            {
                children.emplace_back(std::unique_ptr<Declaration>(d));
            }
        }
        // 如果是一般语句（如表达式语句、控制流语句等）
        else if (auto stmt_ = item->stmt())
        {
            auto const stmt = std::any_cast<Statement *>(stmt_->accept(this));
            children.emplace_back(std::unique_ptr<Statement>(stmt));
        }
        // 理论上不应该到达这里
        else
        {
            assert(false); // 出现非法 blockItem 类型，直接断言失败
        }
    }

    // 构造 Block 节点并返回
    return new Block(std::move(children));
}

// 处理赋值语句，如 a = b + c;
antlrcpp::Any AstVisitor::visitAssign(SysyParser::AssignContext *const ctx)
{
    // 访问左值（LValue 节点），如变量名或数组元素
    auto const lhs_ = std::any_cast<LValue *>(ctx->lVal()->accept(this));
    std::unique_ptr<LValue> lhs(lhs_);

    // 访问右值表达式
    auto const rhs_ = std::any_cast<Expression *>(ctx->exp()->accept(this));
    std::unique_ptr<Expression> rhs(rhs_);

    // 构造 Assignment 节点
    auto const ret = new Assignment(std::move(lhs), std::move(rhs));

    // 作为语句返回
    return static_cast<Statement *>(ret);
}

// 处理表达式语句，如 a + 1;
// 注意表达式语句可能为空，如单独的分号 ;
antlrcpp::Any AstVisitor::visitExprStmt(SysyParser::ExprStmtContext *const ctx)
{
    std::unique_ptr<Expression> expr;

    // 如果有表达式，访问并包装为智能指针
    if (auto expr_ = ctx->exp())
    {
        expr.reset(std::any_cast<Expression *>(expr_->accept(this)));
    }

    // 构造表达式语句节点
    auto const ret = new ExprStmt(std::move(expr));
    return static_cast<Statement *>(ret);
}

// 处理语句是一个代码块的情况（嵌套 Block）
antlrcpp::Any AstVisitor::visitBlockStmt(SysyParser::BlockStmtContext *const ctx)
{
    // 直接访问 block 语法树节点并返回（Block 本身就是 Statement 的子类）
    auto const ret = std::any_cast<Block *>(ctx->block()->accept(this));
    return static_cast<Statement *>(ret);
}

// 处理 if-else 语句
// 支持 if (cond) stmt1 和 if (cond) stmt1 else stmt2 两种形式
antlrcpp::Any AstVisitor::visitIfElse(SysyParser::IfElseContext *const ctx)
{
    // 访问条件表达式
    auto const cond_ = std::any_cast<Expression *>(ctx->cond()->accept(this));
    std::unique_ptr<Expression> cond(cond_);

    // 访问 then 分支（第一个 stmt）
    auto const then_ = std::any_cast<Statement *>(ctx->stmt(0)->accept(this));
    std::unique_ptr<Statement> then(then_);

    std::unique_ptr<Statement> else_;
    // 如果存在 else 分支，访问第二个 stmt
    if (ctx->Else() != nullptr)
    {
        else_.reset(std::any_cast<Statement *>(ctx->stmt(1)->accept(this)));
    }

    // 构造 IfElse 节点
    auto const ret =
        new IfElse(std::move(cond), std::move(then), std::move(else_));
    return static_cast<Statement *>(ret);
}

// 处理 while 循环语句：while (cond) stmt;
antlrcpp::Any AstVisitor::visitWhile(SysyParser::WhileContext *const ctx)
{
    // 访问条件表达式 cond
    auto const cond_ = std::any_cast<Expression *>(ctx->cond()->accept(this));
    std::unique_ptr<Expression> cond(cond_);

    // 访问循环体语句 stmt
    auto const stmt_ = std::any_cast<Statement *>(ctx->stmt()->accept(this));
    std::unique_ptr<Statement> stmt(stmt_);

    // 构造 While 节点
    auto const ret = new While(std::move(cond), std::move(stmt));

    // 作为 Statement 类型返回
    return static_cast<Statement *>(ret);
}

// 处理 break 语句
antlrcpp::Any AstVisitor::visitBreak(SysyParser::BreakContext *const ctx)
{
    // 构造 Break 节点（无参数）
    auto const ret = new Break;
    return static_cast<Statement *>(ret);
}

// 处理 continue 语句
antlrcpp::Any AstVisitor::visitContinue(SysyParser::ContinueContext *const ctx)
{
    // 构造 Continue 节点（无参数）
    auto const ret = new Continue;
    return static_cast<Statement *>(ret);
}

// 处理 return 语句，支持 return 和 return expr;
antlrcpp::Any AstVisitor::visitReturn(SysyParser::ReturnContext *const ctx)
{
    std::unique_ptr<Expression> res;

    // 如果 return 后面有表达式，访问它
    if (auto exp = ctx->exp())
    {
        res.reset(std::any_cast<Expression *>(exp->accept(this)));
    }

    // 构造 Return 节点，参数为可选的返回值表达式
    auto const ret = new Return(std::move(res));
    return static_cast<Statement *>(ret);
}

// 处理左值（变量或数组下标）访问
// 例如：a 或 a[2][i]
antlrcpp::Any AstVisitor::visitLVal(SysyParser::LValContext *const ctx)
{
    // 提取变量名标识符
    Identifier ident(ctx->Ident()->getText());

    // 收集数组访问时的下标表达式（可能为多个维度）
    std::vector<std::unique_ptr<Expression>> indices;
    for (auto exp : ctx->exp())
    {
        auto const index = std::any_cast<Expression *>(exp->accept(this));
        indices.emplace_back(index);
    }

    // 构造 LValue 节点（可能是标量变量，也可能是数组元素）
    return new LValue(std::move(ident), std::move(indices));
}

// 处理 primaryExp_：基础表达式
// primaryExp: '(' exp ')' | number;
antlrcpp::Any AstVisitor::visitPrimaryExp_(SysyParser::PrimaryExp_Context *const ctx)
{
    if (ctx->number())
    {
        // 如果是字面值常数，直接交给 number 节点处理
        return ctx->number()->accept(this);
    }
    else
    {
        // 否则是括号中的表达式，如 (a + b)，递归访问 exp
        assert(ctx->exp());
        return ctx->exp()->accept(this);
    }
}

// 将 LVal 转换为表达式（用于表达式场景中的左值）
antlrcpp::Any AstVisitor::visitLValExpr(SysyParser::LValExprContext *const ctx)
{
    // 接收一个 LValue（例如变量或数组元素）
    auto const lval = std::any_cast<LValue *>(ctx->lVal()->accept(this));

    // LValue 是 Expression 的子类，直接作为表达式返回
    return static_cast<Expression *>(lval);
}

// 处理十进制整数常量，如 123
antlrcpp::Any AstVisitor::visitDecIntConst(SysyParser::DecIntConstContext *const ctx)
{
    return int(std::stoll(ctx->getText(), nullptr, 10));
}

// 处理八进制整数常量，如 0123
antlrcpp::Any AstVisitor::visitOctIntConst(SysyParser::OctIntConstContext *const ctx)
{
    return int(std::stoll(ctx->getText(), nullptr, 8));
}

// 处理十六进制整数常量，如 0x1A3f
antlrcpp::Any AstVisitor::visitHexIntConst(SysyParser::HexIntConstContext *const ctx)
{
    return int(std::stoll(ctx->getText(), nullptr, 16));
}

// 处理十进制浮点数常量，如 3.14 或 1e-3
antlrcpp::Any AstVisitor::visitDecFloatConst(SysyParser::DecFloatConstContext *const ctx)
{
    return std::stof(ctx->getText());
}

// 处理十六进制浮点数常量，如 0x1.1p+2
antlrcpp::Any AstVisitor::visitHexFloatConst(SysyParser::HexFloatConstContext *const ctx)
{
    return std::stof(ctx->getText());
}

// 处理函数调用表达式，如 foo(1, "str", a + b)
antlrcpp::Any AstVisitor::visitCall(SysyParser::CallContext *const ctx)
{
    // 获取函数标识符名称
    Identifier ident(ctx->Ident()->getText(), false);

    // 构造实参列表
    std::vector<Call::Argument> args;
    auto args_ctx = ctx->funcRParams();
    if (args_ctx)
    {
        for (auto arg_ : args_ctx->funcRParam())
        {
            if (auto exp = arg_->exp())
            {
                // 如果参数是表达式
                auto const arg = std::any_cast<Expression *>(exp->accept(this));
                args.emplace_back(std::unique_ptr<Expression>(arg));
            }
            else if (auto str = arg_->stringConst())
            {
                // 如果参数是字符串常量
                auto arg = std::any_cast<std::shared_ptr<std::string>>(str->accept(this));
                args.emplace_back(std::move(*arg));
            }
            else
            {
                assert(false); // 理论上不会出现其他情况
            }
        }
    }

    // 构造函数调用表达式，记录调用行号（用于错误定位）
    auto const ret = new Call(
        std::move(ident),          // 函数名
        std::move(args),           // 参数列表
        ctx->getStart()->getLine() // 调用源代码中的行号
    );

    return static_cast<Expression *>(ret);
}

// 处理 +a 表达式（通常无效，但保留语义）
antlrcpp::Any AstVisitor::visitUnaryAdd(SysyParser::UnaryAddContext *const ctx)
{
    auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new UnaryExpr(UnaryOp::Add, std::unique_ptr<Expression>(operand));
    return static_cast<Expression *>(ret);
}

// 处理 -a 表达式（数值取负）
antlrcpp::Any AstVisitor::visitUnarySub(SysyParser::UnarySubContext *const ctx)
{
    auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new UnaryExpr(UnaryOp::Sub, std::unique_ptr<Expression>(operand));
    return static_cast<Expression *>(ret);
}

// 处理 逻辑非：!a
antlrcpp::Any AstVisitor::visitNot(SysyParser::NotContext *const ctx)
{
    auto const operand = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new UnaryExpr(UnaryOp::Not, std::unique_ptr<Expression>(operand));
    return static_cast<Expression *>(ret);
}

// 处理字符串常量，如 "hello"
antlrcpp::Any AstVisitor::visitStringConst(SysyParser::StringConstContext *const ctx)
{
    return std::make_shared<std::string>(ctx->getText());
}

// 处理 a * b
antlrcpp::Any AstVisitor::visitMul(SysyParser::MulContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new BinaryExpr(BinaryOp::Mul, std::unique_ptr<Expression>(lhs), std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

// 处理 a / b
antlrcpp::Any AstVisitor::visitDiv(SysyParser::DivContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new BinaryExpr(BinaryOp::Div, std::unique_ptr<Expression>(lhs), std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitMod(SysyParser::ModContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->unaryExp()->accept(this));
    auto const ret = new BinaryExpr(BinaryOp::Mod, std::unique_ptr<Expression>(lhs), std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitAdd(SysyParser::AddContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Add, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitSub(SysyParser::SubContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->mulExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Sub, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitLt(SysyParser::LtContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Lt, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitGt(SysyParser::GtContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Gt, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitLeq(SysyParser::LeqContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Leq, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitGeq(SysyParser::GeqContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->addExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Geq, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitEq(SysyParser::EqContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Eq, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitNeq(SysyParser::NeqContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->relExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Neq, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitAnd(SysyParser::AndContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->lAndExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->eqExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::And, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitOr(SysyParser::OrContext *const ctx)
{
    auto const lhs = std::any_cast<Expression *>(ctx->lOrExp()->accept(this));
    auto const rhs = std::any_cast<Expression *>(ctx->lAndExp()->accept(this));
    auto const ret =
        new BinaryExpr(BinaryOp::Or, std::unique_ptr<Expression>(lhs),
                       std::unique_ptr<Expression>(rhs));
    return static_cast<Expression *>(ret);
}

antlrcpp::Any AstVisitor::visitNumber(SysyParser::NumberContext *const ctx)
{
    if (ctx->intConst())
    {
        auto val = std::any_cast<IntLiteral::Value>(ctx->intConst()->accept(this));
        auto literal = new IntLiteral{val};
        return static_cast<Expression *>(literal);
    }
    if (ctx->floatConst())
    {
        auto val = std::any_cast<FloatLiteral::Value>(ctx->floatConst()->accept(this));
        auto literal = new FloatLiteral{val};
        return static_cast<Expression *>(literal);
    }
    assert(false);
    return static_cast<Expression *>(nullptr);
}

std::vector<std::unique_ptr<Expression>>
AstVisitor::visitDimensions(const std::vector<SysyParser::ExpContext *> &ctxs)
{
    std::vector<std::unique_ptr<Expression>> ret;
    for (auto expr_ctx : ctxs)
    {
        auto expr_ = std::any_cast<Expression *>(expr_ctx->accept(this));
        ret.emplace_back(expr_);
    }
    return ret;
}