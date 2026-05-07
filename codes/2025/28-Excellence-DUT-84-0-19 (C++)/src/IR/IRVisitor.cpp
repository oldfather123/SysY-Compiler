#include <memory>
#include <string>
#include <any> // 添加any_cast需要的头文件
#include <string>
#include <functional>
#include <limits>
#include <utility>

#include "IRVisitor.h"
#include "IR.h"

std::vector<IRType *> params;           //形参类型表
std::vector<std::string> paramNames;    //形参名表
Function *curFun = nullptr;             //当前函数
bool newFun = false;                    //新函数flag
Value *retAlloca = nullptr;             //返回值
BasicBlock *retBB = nullptr;            //返回块
BasicBlock *whileCondBB = nullptr;      //while语句cond分支
BasicBlock * whileFalseBB;              //while语句false分支
bool br = false;            
Value *recVal = nullptr;                //最近的表达式的value
Value * recVal_init = nullptr;          //value初始化
BasicBlock *trueBB = nullptr;           //判定为true时的Block
BasicBlock *falseBB = nullptr;          //判定为false时的Block
int id = 1;
bool LValflag = false;                  //左值相关flag
bool con = false;                       //常量初始化相关flag
bool lvcon = false;                     //常量初始化相关flag
bool flag = false;                      //全局作用域内相关flag
//bool gvflag = false;                  //已废弃     
bool initflag = false;                  //数组初始化相关flag
bool conflag = false;                   //常量初始化相关flag

using namespace IR;

std::unique_ptr<Module> IRVisitor::getModule() {
    return std::move(module);
}

//Visit函数
void visit(frontend::ast::CompileUnit &ast);
void visit(frontend::ast::Declaration &ast);
void visit(frontend::ast::Function &ast);
void visit(frontend::ast::Parameter &ast);
void visit(frontend::ast::Block &ast);
void visit(frontend::ast::Statement &ast);
void visit(frontend::ast::Return &ast);
void visit(frontend::ast::ExprStmt &ast);
void visit(frontend::ast::Expression &ast);


void IRVisitor::visit(frontend::ast::CompileUnit &ast)  //访问CompileUint节点
{
    for (const auto &child : ast.children()) {
        std::visit([this](const auto &node_ptr) {
            // node_ptr 是 const std::unique_ptr<Declaration> 或 const std::unique_ptr<Function>
            if (node_ptr) {
                this->visit(*node_ptr);  // 递归访问实际节点
                 
            }
        }, child);
    }
}

void IRVisitor::visit(frontend::ast::Declaration &ast)  //访问Declaration节点
{
    std::string name  = ast.ident().name();
    if(scope.global_scope())                               //是否为全局作用域
    {
        if (ast.is_const() == true)                         //全局常量
        {   
            if (auto scalar = dynamic_cast<const frontend::ast::ScalarType *>(ast.type().get()))        //非数组全局常量
            {
                GlobalVariable* gloVar;
                const auto &expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(ast.init().get()->value());
                frontend::ast::Expression *test = expr_ptr.get();
                const frontend::ast::Expression *expr = expr_ptr.get();
                if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(expr)) {            //返回值是int类型
                    if (scalar->type() == Int)                                                          //声明是int类型
                    {
                        std::int32_t value = int_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        gloVar = new GlobalVariable(name,module.get(),INT32_T,true,valpoint);
                        scope.push(name,valpoint);
                    }
                    else if (scalar->type() == Float)                                                   //声明是float类型
                    {
                        std::int32_t value = int_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,(float)value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,valpoint);
                        scope.push(name,valpoint);
                    }
                } else if (auto *float_lit = dynamic_cast<const frontend::ast::FloatLiteral *>(expr)) { //返回值是float类型

                    if (scalar->type() == Int)                                                          //声明是int类型
                    {
                        float value = float_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,(int)value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        gloVar = new GlobalVariable(name,module.get(),INT32_T,true,valpoint);
                        scope.push(name,valpoint);
                    }
                    else if (scalar->type() == Float)                                                   //声明是float类型
                    {   
                        float value = float_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,valpoint);
                        scope.push(name,valpoint);
                    }
                } else if (auto *bina = dynamic_cast<frontend::ast::BinaryExpr *>(test)){               //访问双目运算符
                    con = true;
                    conflag = true;
                    visit(*bina);
                    con = false;
                    conflag = false;
                    if (recVal->IRType_ == INT32_T)                                                     //返回值是int类型
                    {
                        if (scalar->type() == Int)                                                      //声明是int类型
                        {
                            scope.push(name, dynamic_cast<ConstantInt *>(recVal));
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,false,dynamic_cast<ConstantInt *>(recVal));
                        }
                        else if (scalar->type() == Float)                                               //声明是float类型
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,temp);
                            scope.push(name, temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)                                                //返回值是float类型
                    {
                        if (scalar->type() == Int)                                                      //返回值是int类型
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,temp);
                            scope.push(name, temp);
                        }
                        else if (scalar->type() == Float)                                               //声明是float类型
                        {
                            scope.push(name, dynamic_cast<ConstantFloat *>(recVal));
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                }
                else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(test))
                {
                    con = true;
                    visit(*lval);
                    con = false;
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {   
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,dynamic_cast<ConstantInt *>(recVal));
                            scope.push(name, dynamic_cast<ConstantInt *>(recVal));
                        }
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,temp);
                            scope.push(name, temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,temp);
                            scope.push(name, temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,dynamic_cast<ConstantFloat *>(recVal));
                            scope.push(name, dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                }
                else if (auto *luna = dynamic_cast<frontend::ast::UnaryExpr *>(test))
                {
                    con = true;
                    visit(*luna);
                    con = false;
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {   
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,dynamic_cast<ConstantInt *>(recVal));
                            scope.push(name, dynamic_cast<ConstantInt *>(recVal));
                        }
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,temp);
                            scope.push(name, temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,temp);
                            scope.push(name, temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,dynamic_cast<ConstantFloat *>(recVal));
                            scope.push(name, dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }
                }
                else if (auto *luna = dynamic_cast<frontend::ast::Call *>(test))
                {
                    con = true;
                    visit(*luna);
                    con = false;

                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {   
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,dynamic_cast<ConstantInt *>(recVal));
                            scope.push(name, dynamic_cast<ConstantInt *>(recVal));
                        }
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,temp);
                            scope.push(name, temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            gloVar = new GlobalVariable(name,module.get(),INT32_T,true,temp);
                            scope.push(name, temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            gloVar = new GlobalVariable(name,module.get(),FLOAT_T,true,dynamic_cast<ConstantFloat *>(recVal));
                            scope.push(name, dynamic_cast<ConstantFloat *>(recVal));
                        }
                    } 
                }
                return;
            } 
            else if (auto array = dynamic_cast<const frontend::ast::ArrayType *>(ast.type().get())) 
            {   
                // 递归构造多维数组类型
                std::function<IRType*(const frontend::ast::ArrayType*, size_t)> build_array_type;
                build_array_type = [this, &build_array_type](const frontend::ast::ArrayType* arr_type, size_t dim_idx) -> IRType* {
                    using BaseType = frontend::ast::ScalarType::Type;
                    if (dim_idx == arr_type->dimensions().size()) {
                        if (arr_type->base_type() == Int)
                            return INT32_T;
                        else if (arr_type->base_type() == Float)
                            return FLOAT_T;
                        else
                            assert(false && "未知基础类型");
                    }
                    else {
                        // 获取维度大小
                        size_t dim_size = 0;
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                            dim_size = int_lit->value();
                        }
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
                            con = true;
                            visit(*lval);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else {
                            assert(false && "数组维度表达式必须是整型字面量");
                        }
                        IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
                        return new ArrayIRType(elem_type, dim_size);
                    }
                };


                std::function<Constant*(const frontend::ast::ArrayType*, const std::vector<Constant*>&, size_t, size_t)> create_flattened_array = 
                [&](const frontend::ast::ArrayType* arr_type, 
                    const std::vector<Constant*>& flat_elements,
                    size_t dim_idx,
                    size_t total_elements) -> Constant* {
                    
                    // 如果当前是最后一维，直接创建数组
                    if (dim_idx == arr_type->dimensions().size() - 1) {
                        IRType* elem_type = nullptr;
                        if (arr_type->base_type() == Int)
                            elem_type = INT32_T;
                        else if (arr_type->base_type() == Float)
                            elem_type = FLOAT_T;
                        else
                            assert(false && "未知基础类型");
                            
                        ArrayIRType* array_type = new ArrayIRType(elem_type, flat_elements.size());
                        return new ConstantArray(array_type, flat_elements);
                    }
                    
                    // 计算当前维度大小
                    size_t dim_size = 0;
                    if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                        dim_size = int_lit->value();
                    } 
                    else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
                        con = true;
                        visit(*bin);
                        con = false;
                        auto v = dynamic_cast<ConstantInt *>(recVal);
                        dim_size = v->value_;
                    }
                    else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
                        con = true;
                        visit(*lval);
                        con = false;
                        auto v = dynamic_cast<ConstantInt *>(recVal);
                        dim_size = v->value_;
                    }
                    else {
                        assert(false && "数组维度表达式必须是整型字面量");
                    }
                    
                    // 计算子数组大小
                    size_t sub_array_size = total_elements / dim_size;
                    
                    // 创建子数组
                    std::vector<Constant*> sub_arrays;
                    for (size_t i = 0; i < dim_size; i++) {
                        size_t start = i * sub_array_size;
                        size_t end = (i + 1) * sub_array_size;
                        
                        // 提取子数组元素
                        std::vector<Constant*> sub_elements(
                            flat_elements.begin() + start,
                            flat_elements.begin() + end
                        );
                        
                        // 递归创建子数组
                        Constant* sub_array = create_flattened_array(
                            arr_type, sub_elements, dim_idx + 1, sub_array_size
                        );
                        sub_arrays.push_back(sub_array);
                    }
                    
                    // 创建当前维度数组
                    IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
                    ArrayIRType* array_type = new ArrayIRType(elem_type, dim_size);
                    return new ConstantArray(array_type, sub_arrays);
                };

                // 递归构造嵌套常量数组
                std::function<Constant*(const frontend::ast::ArrayType*, const frontend::ast::Initializer*, size_t)> build_constant_array;
                build_constant_array = [this, &build_constant_array, &build_array_type, &create_flattened_array](const frontend::ast::ArrayType* arr_type, const frontend::ast::Initializer* init, size_t dim_idx) -> Constant* {
                    using BaseType = frontend::ast::ScalarType::Type;

                    // 获取当前维度大小
                    size_t dim_size = 0;
                    if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                        dim_size = int_lit->value();
                    } 
                    else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
                        con = true;
                        visit(*bin);
                        con = false;
                        auto v = dynamic_cast<ConstantInt *>(recVal);
                        dim_size = v->value_;
                    }
                    else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
                        con = true;
                        visit(*lval);
                        con = false;
                        auto v = dynamic_cast<ConstantInt *>(recVal);
                        dim_size = v->value_;
                    }
                    else {
                        assert(false && "数组维度表达式必须是整型字面量");
                    }

                    // 如果init为空，返回全零数组
                    if (init == nullptr && initflag == true) {
                        IRType* elem_type = nullptr;
                        if (dim_idx == arr_type->dimensions().size() - 1) {
                            if (arr_type->base_type() == Int)
                                elem_type = INT32_T;
                            else if (arr_type->base_type() == Float)
                                elem_type = FLOAT_T;
                            else
                                assert(false && "未知基础类型");
                        } else {
                            elem_type = build_array_type(arr_type, dim_idx + 1);
                        }

                        ArrayIRType* cur_array_type = new ArrayIRType(elem_type, dim_size);
                        std::vector<Constant*> zero_elements(dim_size, nullptr);

                        if (auto arr_elem_type = dynamic_cast<ArrayIRType*>(elem_type)) {
                            for (size_t i = 0; i < dim_size; ++i) {
                                zero_elements[i] = build_constant_array(arr_type, nullptr, dim_idx + 1);
                            }
                        } else {
                            Constant* zero_const = nullptr;
                            if (arr_type->base_type() == Int) {
                                zero_const = new ConstantInt(INT32_T, 0);
                            } else if (arr_type->base_type() == Float) {
                                zero_const = new ConstantFloat(FLOAT_T, 0.0f);
                            } else {
                                assert(false && "未知基础类型");
                            }
                            std::fill(zero_elements.begin(), zero_elements.end(), zero_const);
                        }
                        return new ConstantArray(cur_array_type, zero_elements);
                    }
                    else if (init == nullptr && initflag == false) {
                        return nullptr;
                    }

                    initflag = true;
                    const auto& val = init->value();

                    if (init != nullptr &&
                            std::holds_alternative < std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val) &&
                            std::get < std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val).empty()) {
                        return nullptr;
                    }                   

                    std::vector<Constant*> elements;

                    // 计算从当前维度开始的总元素数
                    size_t total_elements = 1;
                    for (size_t i = dim_idx; i < arr_type->dimensions().size(); i++) {
                        size_t dim = 0;
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[i].get())) {
                            dim = int_lit->value();
                        }
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[i].get())) {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim = v->value_;
                        }
                        else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[i].get())) {
                            con = true;
                            visit(*lval);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim = v->value_;
                        }
                        total_elements *= dim;
                    }

                    if (dim_idx < arr_type->dimensions().size() - 1 &&
                        std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val)) 
                    {
                        const auto& inits = std::get<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val);
                        size_t sub_array_size = total_elements / dim_size; // 子数组元素数
                        std::vector<Constant*> current_sub_elements; // 当前子数组的元素缓冲区
                        std::vector<Constant*> sub_arrays; // 当前维度的子数组集合

                        for (const auto& subinit : inits) {
                            const auto& sub_val = subinit->value();

                            if (std::holds_alternative<std::unique_ptr<frontend::ast::Expression>>(sub_val)) {
                                // 处理标量元素，添加到当前子数组缓冲区
                                const auto& expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(sub_val);
                                const frontend::ast::Expression* expr = expr_ptr.get();
                                frontend::ast::Expression* test = expr_ptr.get();
                                Constant* elem = nullptr;

                                // 解析标量表达式为Constant
                                if (auto* int_lit = dynamic_cast<const frontend::ast::IntLiteral*>(expr)) {
                                    if (arr_type->base_type() == Int) {
                                        elem = new ConstantInt(INT32_T, int_lit->value());
                                    } else if (arr_type->base_type() == Float) {
                                        elem = new ConstantFloat(FLOAT_T, (float)int_lit->value());
                                    }
                                } else if (auto* float_lit = dynamic_cast<const frontend::ast::FloatLiteral*>(expr)) {
                                    if (arr_type->base_type() == Int) {
                                        elem = new ConstantInt(INT32_T, (int)float_lit->value());
                                    } else if (arr_type->base_type() == Float) {
                                        elem = new ConstantFloat(FLOAT_T, float_lit->value());
                                    }
                                } else if (auto* bin = dynamic_cast<frontend::ast::BinaryExpr*>(test)) {
                                    con = true;
                                    visit(*bin);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* lval = dynamic_cast<frontend::ast::LValue*>(test)) {
                                    con = true;
                                    visit(*lval);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* unary = dynamic_cast<frontend::ast::UnaryExpr*>(test)) {
                                    con = true;
                                    visit(*unary);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* call = dynamic_cast<frontend::ast::Call*>(test)) {
                                    con = true;
                                    visit(*call);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else {
                                    assert(false && "不支持的标量表达式类型");
                                }

                                current_sub_elements.push_back(elem);

                                // 缓冲区满时，创建子数组
                                if (current_sub_elements.size() == sub_array_size) {
                                    Constant* sub_array = create_flattened_array(arr_type, current_sub_elements, dim_idx + 1, sub_array_size);
                                    sub_arrays.push_back(sub_array);
                                    current_sub_elements.clear();
                                }
                            } else if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(sub_val)) {
                                // 处理子初始化器，先清空当前缓冲区
                                if (!current_sub_elements.empty()) {
                                    // 补零到子数组大小
                                    Constant* zero_const;
                                    if (arr_type->base_type() == Int )
                                        zero_const = new ConstantInt(INT32_T, 0);
                                    else
                                        zero_const = new ConstantFloat(FLOAT_T, 0.0f);
                                    while (current_sub_elements.size() < sub_array_size) {
                                        current_sub_elements.push_back(zero_const);
                                    }
                                    // 创建子数组
                                    Constant* sub_array = create_flattened_array(arr_type, current_sub_elements, dim_idx + 1, sub_array_size);
                                    sub_arrays.push_back(sub_array);
                                    current_sub_elements.clear();
                                }

                                // 递归处理子初始化器
                                Constant* sub_array = build_constant_array(arr_type, subinit.get(), dim_idx + 1);
                                sub_arrays.push_back(sub_array);
                            } else {
                                assert(false && "非最后一维初始化器元素类型不支持");
                            }
                        }

                        // 处理剩余的缓冲区元素
                        if (!current_sub_elements.empty()) {
                            Constant* zero_const;
                            if (arr_type->base_type() == Int )
                                zero_const = new ConstantInt(INT32_T, 0);
                            else
                                zero_const = new ConstantFloat(FLOAT_T, 0.0f);
                            while (current_sub_elements.size() < sub_array_size) {
                                current_sub_elements.push_back(zero_const);
                            }
                            Constant* sub_array = create_flattened_array(arr_type, current_sub_elements, dim_idx + 1, sub_array_size);
                            sub_arrays.push_back(sub_array);
                        }

                        // 补零当前维度的子数组
                        if (sub_arrays.size() < dim_size) {
                            Constant* zero_sub_array = build_constant_array(arr_type, nullptr, dim_idx + 1);
                            while (sub_arrays.size() < dim_size) {
                                sub_arrays.push_back(zero_sub_array);
                            }
                        }

                        elements = sub_arrays;
                    } else if (dim_idx == arr_type->dimensions().size() - 1) {
                        // 最后一维，处理标量列表
                        if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val)) {
                            const auto& inits = std::get<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val);
                            for (const auto& subinit : inits) {
                                const auto& sub_val = subinit->value();
                                assert(std::holds_alternative<std::unique_ptr<frontend::ast::Expression>>(sub_val) && "最后一维初始化器必须是表达式");
                                const auto& expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(sub_val);
                                const frontend::ast::Expression* expr = expr_ptr.get();
                                frontend::ast::Expression* test = expr_ptr.get();
                                Constant* elem = nullptr;

                                if (auto* int_lit = dynamic_cast<const frontend::ast::IntLiteral*>(expr)) {
                                    if (arr_type->base_type() == Int) {
                                        elem = new ConstantInt(INT32_T, int_lit->value());
                                    } else if (arr_type->base_type() == Float) {
                                        elem = new ConstantFloat(FLOAT_T, (float)int_lit->value());
                                    }
                                } else if (auto* float_lit = dynamic_cast<const frontend::ast::FloatLiteral*>(expr)) {
                                    if (arr_type->base_type() == Int) {
                                        elem = new ConstantInt(INT32_T, (int)float_lit->value());
                                    } else if (arr_type->base_type() == Float) {
                                        elem = new ConstantFloat(FLOAT_T, float_lit->value());
                                    }
                                } else if (auto* bin = dynamic_cast<frontend::ast::BinaryExpr*>(test)) {
                                    con = true;
                                    visit(*bin);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* lval = dynamic_cast<frontend::ast::LValue*>(test)) {
                                    con = true;
                                    visit(*lval);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* unary = dynamic_cast<frontend::ast::UnaryExpr*>(test)) {
                                    con = true;
                                    visit(*unary);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else if (auto* call = dynamic_cast<frontend::ast::Call*>(test)) {
                                    con = true;
                                    visit(*call);
                                    con = false;
                                    if (recVal->IRType_ == INT32_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantInt(INT32_T, v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantInt*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, (float)v->value_);
                                        }
                                    } else if (recVal->IRType_ == FLOAT_T) {
                                        if (arr_type->base_type() == Int) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantInt(INT32_T, (int)v->value_);
                                        } else if (arr_type->base_type() == Float) {
                                            auto v = dynamic_cast<ConstantFloat*>(recVal);
                                            elem = new ConstantFloat(FLOAT_T, v->value_);
                                        }
                                    }
                                } else {
                                    assert(false && "不支持的标量类型");
                                }

                                elements.push_back(elem);
                            }
                        } else {
                            assert(false && "最后一维初始化器必须是表达式列表");
                        }

                        // 补零最后一维
                        if (elements.size() < dim_size) {
                            Constant* zero_const = nullptr;
                            if (arr_type->base_type() == Int) {
                                zero_const = new ConstantInt(INT32_T, 0);
                            } else if (arr_type->base_type() == Float) {
                                zero_const = new ConstantFloat(FLOAT_T, 0.0f);
                            } else {
                                assert(false && "未知基础类型");
                            }
                            elements.resize(dim_size, zero_const);
                        }
                    }

                    // 构造当前维度的数组类型
                    IRType* elem_type = (dim_idx == arr_type->dimensions().size() - 1) ? 
                        (arr_type->base_type() == Int ? INT32_T : FLOAT_T) : 
                        build_array_type(arr_type, dim_idx + 1);

                    ArrayIRType* cur_array_type = new ArrayIRType(elem_type, dim_size);
                    return new ConstantArray(cur_array_type, elements);
                };

                // 构造类型和常量
                IRType* array_type = build_array_type(array, 0);
                Constant* arr = build_constant_array(array, ast.init().get(), 0);
                
                GlobalVariable* var = new GlobalVariable(name, module.get(), array_type, true, arr);
                scope.push(name, var);
                initflag = false;
                return;
            }
        }
        else//global vari...
        {
            if (auto scalar = dynamic_cast<const frontend::ast::ScalarType *>(ast.type().get())) 
            {
                GlobalVariable* var;
                if (ast.init().get() == NULL)
                {
                    if (scalar->type() == Int)
                    {
                        var = new GlobalVariable(name,module.get(),INT32_T,false,new ConstantInt(module->int32_ty_, 0));
                        scope.push(name, var);
                        return;
                    }
                    else if (scalar->type() == Float)
                    {
                        var = new GlobalVariable(name,module.get(),FLOAT_T,false,new ConstantFloat(module->float32_ty_, 0));
                        scope.push(name, var);
                        return;
                    }
                }
                const auto &expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(ast.init().get()->value());
                frontend::ast::Expression *test = expr_ptr.get();
                const frontend::ast::Expression *expr = expr_ptr.get();
                if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(expr)) {
                    if (scalar->type() == Int)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        var = new GlobalVariable(name,module.get(),INT32_T,false,valpoint);
                    }
                    else if (scalar->type() == Float)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,(float)value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        var = new GlobalVariable(name,module.get(),FLOAT_T,false,valpoint);

                    }
                    scope.push(name, var);
                } else if (auto *float_lit = dynamic_cast<const frontend::ast::FloatLiteral *>(expr)) {

                    if (scalar->type() == Int)
                    {
                        float value = float_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,(int)value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        var = new GlobalVariable(name,module.get(),INT32_T,false,valpoint);
                    }
                    else if (scalar->type() == Float)
                    {
                        float value = float_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        var = new GlobalVariable(name,module.get(),FLOAT_T,false,valpoint);

                    }
                    scope.push(name, var);
                } else if (auto *bina = dynamic_cast<frontend::ast::BinaryExpr *>(test)){
                    con = true;
                    conflag = true;
                    visit(*bina);
                    con = false;
                    conflag = false;
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                            var = new GlobalVariable(name,module.get(),INT32_T,false,dynamic_cast<ConstantInt *>(recVal));
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            var = new GlobalVariable(name,module.get(),INT32_T,false,temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                    scope.push(name, var);
                }
                else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(test))
                {
                    con = true;
                    visit(*lval);
                    con = false;
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                            var = new GlobalVariable(name,module.get(),INT32_T,false,dynamic_cast<ConstantInt *>(recVal));
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            var = new GlobalVariable(name,module.get(),INT32_T,false,temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                    scope.push(name, var);
                }
                else if (auto *luna = dynamic_cast<frontend::ast::UnaryExpr *>(test))
                {
                    con = true;
                    visit(*luna);
                    con = false;
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                            var = new GlobalVariable(name,module.get(),INT32_T,false,dynamic_cast<ConstantInt *>(recVal));
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            var = new GlobalVariable(name,module.get(),INT32_T,false,temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                    scope.push(name, var);
                }
                else if (auto *luna = dynamic_cast<frontend::ast::Call *>(test))
                {
                    con = true;
                    visit(*luna);
                    con = false;

                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                            var = new GlobalVariable(name,module.get(),INT32_T,false,dynamic_cast<ConstantInt *>(recVal));
                        else if (scalar->type() == Float)
                        {
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            Constant* temp = new ConstantFloat(FLOAT_T,(float)v->value_);
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,temp);
                        }
                    }   
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if (scalar->type() == Int)
                        {
                            auto v = dynamic_cast<ConstantFloat *>(recVal);
                            Constant* temp = new ConstantInt(INT32_T,(int)v->value_);
                            var = new GlobalVariable(name,module.get(),INT32_T,false,temp);
                        }
                        else if (scalar->type() == Float)
                        {
                            var = new GlobalVariable(name,module.get(),FLOAT_T,false,dynamic_cast<ConstantFloat *>(recVal));
                        }
                    }  
                    scope.push(name, var);
                }
                return;
            } 
            else if (auto array = dynamic_cast<const frontend::ast::ArrayType *>(ast.type().get())) 
{
    // 递归构造多维数组类型
    std::function<IRType*(const frontend::ast::ArrayType*, size_t)> build_array_type;
    build_array_type = [this, &build_array_type](const frontend::ast::ArrayType* arr_type, size_t dim_idx) -> IRType* {
        using BaseType = frontend::ast::ScalarType::Type;
        if (dim_idx == arr_type->dimensions().size()) {
            if (arr_type->base_type() == Int)
                return INT32_T;
            else if (arr_type->base_type() == Float)
                return FLOAT_T;
            else
                assert(false && "未知基础类型");
        }
        else {
            // 获取维度大小
            size_t dim_size = 0;
            if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                dim_size = int_lit->value();
            }
            else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
                con = true;
                visit(*bin);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim_size = v->value_;
            }
            else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
                con = true;
                visit(*lval);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim_size = v->value_;
            }
            else {
                assert(false && "数组维度表达式必须是整型字面量");
            }
            IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
            return new ArrayIRType(elem_type, dim_size);
        }
    };

    // 辅助函数：将常量（标量或数组）展开为扁平的标量列表
    std::function<void(Constant*, std::vector<Constant*>&)> flatten_constant;
    flatten_constant = [&](Constant* c, std::vector<Constant*>& flat) {
        if (auto arr = dynamic_cast<ConstantArray*>(c)) {
            for (auto elem : arr->const_array) {
                flatten_constant(elem, flat);
            }
        }
        else {
            flat.push_back(c);
        }
    };

    // 递归构造嵌套常量数组
    std::function<Constant*(const frontend::ast::ArrayType*, const std::vector<Constant*>&, size_t, size_t)> create_flattened_array = 
    [&](const frontend::ast::ArrayType* arr_type, 
        const std::vector<Constant*>& flat_elements,
        size_t dim_idx,
        size_t total_elements) -> Constant* {
        
        // 如果当前是最后一维，直接创建数组
        if (dim_idx == arr_type->dimensions().size() - 1) {
            IRType* elem_type = nullptr;
            if (arr_type->base_type() == Int)
                elem_type = INT32_T;
            else if (arr_type->base_type() == Float)
                elem_type = FLOAT_T;
            else
                assert(false && "未知基础类型");
                
            ArrayIRType* array_type = new ArrayIRType(elem_type, flat_elements.size());
            return new ConstantArray(array_type, flat_elements);
        }
        
        // 计算当前维度大小
        size_t dim_size = 0;
        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
            dim_size = int_lit->value();
        } 
        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
            con = true;
            visit(*bin);
            con = false;
            auto v = dynamic_cast<ConstantInt *>(recVal);
            dim_size = v->value_;
        }
        else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
            con = true;
            visit(*lval);
            con = false;
            auto v = dynamic_cast<ConstantInt *>(recVal);
            dim_size = v->value_;
        }
        else {
            assert(false && "数组维度表达式必须是整型字面量");
        }
        
        // 计算子数组大小
        size_t sub_array_size = total_elements / dim_size;
        
        // 创建子数组
        std::vector<Constant*> sub_arrays;
        for (size_t i = 0; i < dim_size; i++) {
            size_t start = i * sub_array_size;
            size_t end = (i + 1) * sub_array_size;
            
            // 提取子数组元素
            std::vector<Constant*> sub_elements(
                flat_elements.begin() + start,
                flat_elements.begin() + end
            );
            
            // 递归创建子数组
            Constant* sub_array = create_flattened_array(
                arr_type, sub_elements, dim_idx + 1, sub_array_size
            );
            sub_arrays.push_back(sub_array);
        }
        
        // 创建当前维度数组
        IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
        ArrayIRType* array_type = new ArrayIRType(elem_type, dim_size);
        return new ConstantArray(array_type, sub_arrays);
    };

    // 递归构造嵌套常量数组
    std::function<Constant*(const frontend::ast::ArrayType*, const frontend::ast::Initializer*, size_t)> build_constant_array;
    build_constant_array = [this, &build_constant_array, &build_array_type, &create_flattened_array, &flatten_constant](const frontend::ast::ArrayType* arr_type, const frontend::ast::Initializer* init, size_t dim_idx) -> Constant* {
        using BaseType = frontend::ast::ScalarType::Type;

        // 获取当前维度大小
        size_t dim_size = 0;
        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
            dim_size = int_lit->value();
        } 
        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
            con = true;
            visit(*bin);
            con = false;
            auto v = dynamic_cast<ConstantInt *>(recVal);
            dim_size = v->value_;
        }
        else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
            con = true;
            visit(*lval);
            con = false;
            auto v = dynamic_cast<ConstantInt *>(recVal);
            dim_size = v->value_;
        }
        else {
            assert(false && "数组维度表达式必须是整型字面量");
        }

        // 如果init为空，直接返回全零数组
        if (init == nullptr && initflag == true) {
            IRType* elem_type = nullptr;
            if (dim_idx == arr_type->dimensions().size() - 1) {
                if (arr_type->base_type() == Int)
                    elem_type = INT32_T;
                else if (arr_type->base_type() == Float)
                    elem_type = FLOAT_T;
                else
                    assert(false && "未知基础类型");
            } else {
                elem_type = build_array_type(arr_type, dim_idx + 1);
            }

            ArrayIRType* cur_array_type = new ArrayIRType(elem_type, dim_size);

            std::vector<Constant*> zero_elements(dim_size, nullptr);

            if (auto arr_elem_type = dynamic_cast<ArrayIRType*>(elem_type)) {
                size_t sub_dim_size = arr_elem_type->num_elements_;
                for (size_t i = 0; i < dim_size; ++i) {
                    // 递归生成子数组的零值
                    zero_elements[i] = build_constant_array(arr_type, nullptr, dim_idx + 1);
                }
            } else {
                Constant* zero_const = nullptr;
                if (arr_type->base_type() == Int) {
                    zero_const = new ConstantInt(INT32_T, 0);
                } else if (arr_type->base_type() == Float) {
                    zero_const = new ConstantFloat(FLOAT_T, 0.0f);
                } else {
                    assert(false && "未知基础类型");
                }
                std::fill(zero_elements.begin(), zero_elements.end(), zero_const);
            }
            return new ConstantArray(cur_array_type, zero_elements);
        }
        else if (init == nullptr && initflag == false)
        {
            return nullptr;
        }

        initflag = true;

        const auto& val = init->value();

        if (init != nullptr &&
                 std::holds_alternative < std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val) &&
                 std::get < std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val).empty())
        {
            return nullptr;
        }                   

        // 计算从当前维度开始的总元素数
        size_t total_elements = 1;
        for (size_t i = dim_idx; i < arr_type->dimensions().size(); i++) {
            size_t dim = 0;
            if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[i].get())) {
                dim = int_lit->value();
            }
            else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[i].get())) {
                con = true;
                visit(*bin);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim = v->value_;
            }
            else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[i].get())) {
                con = true;
                visit(*lval);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim = v->value_;
            }
            total_elements *= dim;
        }

        // 处理所有维度（包括混合标量和嵌套初始化器的情况）
        std::vector<Constant*> flat_elements;

        if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val)) {
            const auto& inits = std::get<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val);
            
            // 展开所有子初始化器（无论标量还是嵌套）到扁平列表
            for (const auto& subinit : inits) {
                const auto& sub_val = subinit->value();
                
                if (std::holds_alternative<std::unique_ptr<frontend::ast::Expression>>(sub_val)) {
                    // 标量表达式，直接转换为常量
                    const auto& expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(sub_val);
                    const frontend::ast::Expression* expr = expr_ptr.get();
                    frontend::ast::Expression* test = expr_ptr.get();
                    
                    if (auto* int_lit = dynamic_cast<const frontend::ast::IntLiteral*>(expr)) {
                        if (arr_type->base_type() == Int)
                        {
                            flat_elements.push_back(new ConstantInt(INT32_T, int_lit->value()));
                        }
                        else if (arr_type->base_type() == Float)
                        {
                            flat_elements.push_back(new ConstantFloat(FLOAT_T, (float)int_lit->value()));
                        }
                    }
                    else if (auto* float_lit = dynamic_cast<const frontend::ast::FloatLiteral*>(expr)) {
                        if (arr_type->base_type() == Int)
                        {
                            flat_elements.push_back(new ConstantInt(INT32_T, (int)float_lit->value()));
                        }
                        else if (arr_type->base_type() == Float)
                        {
                            flat_elements.push_back(new ConstantFloat(FLOAT_T, float_lit->value()));
                        }
                    }
                    else if (auto *bina = dynamic_cast<frontend::ast::BinaryExpr *>(test))
                    {
                        con = true;
                        visit(*bina);
                        con = false;
                        if(recVal->IRType_ == INT32_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, v->value_));
                            }
                            else if (arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, (float)v->value_));
                            }
                        }
                        else if (recVal->IRType_ == FLOAT_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, (int)v->value_));
                            }
                            else if(arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, v->value_));
                            }
                        }
                    }
                    else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(test))
                    {
                        con = true;
                        visit(*lval);
                        con = false;
                        if(recVal->IRType_ == INT32_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, v->value_));
                            }
                            else if (arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, (float)v->value_));
                            }
                        }
                        else if (recVal->IRType_ == FLOAT_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, (int)v->value_));
                            }
                            else if(arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, v->value_));
                            }
                        }
                    }
                    else if (auto *unary = dynamic_cast<frontend::ast::UnaryExpr *>(test))
                    {
                        con = true;
                        visit(*unary);
                        con = false;
                        if(recVal->IRType_ == INT32_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, v->value_));
                            }
                            else if (arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, (float)v->value_));
                            }
                        }
                        else if (recVal->IRType_ == FLOAT_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, (int)v->value_));
                            }
                            else if(arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, v->value_));
                            }
                        }
                    }
                    else if (auto *call = dynamic_cast<frontend::ast::Call *>(test))
                    {
                        con = true;
                        visit(*call);
                        con = false;
                        if(recVal->IRType_ == INT32_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, v->value_));
                            }
                            else if (arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantInt *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, (float)v->value_));
                            }
                        }
                        else if (recVal->IRType_ == FLOAT_T)
                        {
                            if(arr_type->base_type() == Int)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantInt(INT32_T, (int)v->value_));
                            }
                            else if(arr_type->base_type() == Float)
                            {
                                auto v = dynamic_cast<ConstantFloat *>(recVal);
                                flat_elements.push_back(new ConstantFloat(FLOAT_T, v->value_));
                            }
                        }
                    }
                    else {
                        assert(false && "不支持的标量表达式类型");
                    }
                }
                else if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(sub_val)) {
                    // 嵌套初始化器，递归处理后展开
                    Constant* sub_array = build_constant_array(arr_type, subinit.get(), dim_idx + 1);
                    if (sub_array != nullptr) {
                        flatten_constant(sub_array, flat_elements);
                    }
                }
            }
        }

        // 处理元素数量：截断多余元素或补零
        if (flat_elements.size() > total_elements) {
            flat_elements.resize(total_elements);
        } else if (flat_elements.size() < total_elements) {
            Constant* zero_const = nullptr;
            if (arr_type->base_type() == Int) {
                zero_const = new ConstantInt(INT32_T, 0);
            } else if (arr_type->base_type() == Float) {
                zero_const = new ConstantFloat(FLOAT_T, 0.0f);
            } else {
                assert(false && "未知基础类型");
            }
            flat_elements.resize(total_elements, zero_const);
        }

        // 创建嵌套数组结构
        return create_flattened_array(arr_type, flat_elements, dim_idx, total_elements);
    };

    // 构造类型和常量
    IRType* array_type = build_array_type(array, 0);
    Constant* arr = build_constant_array(array, ast.init().get(), 0);
    
    GlobalVariable* var = new GlobalVariable(name, module.get(), array_type, false, arr);
    scope.push(name, var);
    initflag = false;
    return;
}
        }
    }
    else // not global
    {
        if (ast.is_const() == true)
        {   
            AllocaInst *varAlloca;
            if (auto scalar = dynamic_cast<const frontend::ast::ScalarType *>(ast.type().get())) 
            {
                if (ast.init().get() == NULL)
                {
                    if (scalar->type() == Int)
                    {
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                    }
                    return;
                }
                
                const auto &expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(ast.init().get()->value());
                const frontend::ast::Expression *expr = expr_ptr.get();
                frontend::ast::Expression *test = expr_ptr.get();
                if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(expr)) {
                    if (scalar->type() == Int)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,(float)value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                } else if (auto *float_lit = dynamic_cast<const frontend::ast::FloatLiteral *>(expr)) {
                    if (scalar->type() == Int)
                    {
                        float value = float_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,(int)value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        float value = float_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                } else if (auto *bina = dynamic_cast<frontend::ast::BinaryExpr *>(test)){
                    visit(*bina);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(test))
                {
                    visit(*lval);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *luna = dynamic_cast<frontend::ast::UnaryExpr *>(test))
                {
                    visit(*luna);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *luna = dynamic_cast<frontend::ast::Call *>(test))
                {
                    visit(*luna);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }
                return;
                } 
            else if (auto array = dynamic_cast<const frontend::ast::ArrayType *>(ast.type().get())) 
            {
                // 递归构造多维数组类型
                std::function<IRType*(const frontend::ast::ArrayType*, size_t)> build_array_type;
                build_array_type = [this, &build_array_type](const frontend::ast::ArrayType* arr_type, size_t dim_idx) -> IRType* {
                    using BaseType = frontend::ast::ScalarType::Type;
                    if (dim_idx == arr_type->dimensions().size()) {
                        if (arr_type->base_type() == Int)
                            return INT32_T;
                        else if (arr_type->base_type() == Float)
                            return FLOAT_T;
                        else
                            assert(false && "未知基础类型");
                    }
                    else {
                        // 获取维度大小
                        size_t dim_size = 0;
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                            dim_size = int_lit->value();
                        }
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get()))
                        {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else if(auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get()))
                        {
                            visit(*lval);
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else {
                            assert(false && "数组维度表达式必须是整型字面量");
                        }
                        IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
                        return new ArrayIRType(elem_type, dim_size);
                    }
                };

                std::function<std::vector<Value*>(const frontend::ast::ArrayType*, const frontend::ast::Initializer*, size_t)> build_variable_array;
                build_variable_array = [this, &build_variable_array, &build_array_type](const frontend::ast::ArrayType* arr_type, const frontend::ast::Initializer* init, size_t dim_idx) -> std::vector<Value*> {
                    // 计算数组总元素个数（所有维度乘积）
                    auto get_dim_size = [&](const auto& dim_expr) -> size_t {
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(dim_expr)) {
                            return int_lit->value();
                        } 
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(dim_expr)) {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            return v->value_;
                        }
                        else if(auto *lval = dynamic_cast<frontend::ast::LValue *>(dim_expr)) {
                            visit(*lval);
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            return v->value_;
                        }
                        else {
                            assert(false && "数组维度表达式必须是整型字面量");
                            return 0;
                        }
                    };

                    size_t total_elements = 1;
                    for (size_t i = 0; i < arr_type->dimensions().size(); ++i) {
                        total_elements *= get_dim_size(arr_type->dimensions()[i].get());
                    }

                    // 处理无初始化器的情况
                    if (init == nullptr) {
                        if (initflag) {
                            // 生成全零数组
                            Value* zero_val = nullptr;
                            if (arr_type->base_type() == Int) {
                                zero_val = new ConstantInt(INT32_T, 0);
                            } else if (arr_type->base_type() == Float) {
                                zero_val = new ConstantFloat(FLOAT_T, 0.0f);
                            } else {
                                assert(false && "未知基础类型");
                            }
                            return std::vector<Value*>(total_elements, zero_val);
                        } else {
                            return {};
                        }
                    }

                    initflag = true;

                    // 显式声明递归函数类型，解决auto类型推断问题
                    std::function<void(const frontend::ast::Initializer*, std::vector<Value*>&)> flatten_initializer;
                    
                    // 定义扁平化初始化器函数，处理标量和子数组的递归收集
                    flatten_initializer = [&](const frontend::ast::Initializer* init, std::vector<Value*>& list) -> void {
                        if (!init) return;

                        const auto& val = init->value();
                        if (std::holds_alternative<std::unique_ptr<frontend::ast::Expression>>(val)) {
                            // 处理标量表达式
                            const auto& expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(val);
                            frontend::ast::Expression* expr = expr_ptr.get();
                            Value* val = nullptr;

                            if (auto* int_lit = dynamic_cast<const frontend::ast::IntLiteral*>(expr)) {
                                if (arr_type->base_type() == Int) {
                                    val = new ConstantInt(INT32_T, int_lit->value());
                                } else if (arr_type->base_type() == Float) {
                                    val = new ConstantFloat(FLOAT_T, (float)int_lit->value());
                                }
                            } else if (auto* float_lit = dynamic_cast<const frontend::ast::FloatLiteral*>(expr)) {
                                if (arr_type->base_type() == Int) {
                                    val = new ConstantInt(INT32_T, (int)float_lit->value());
                                } else if (arr_type->base_type() == Float) {
                                    val = new ConstantFloat(FLOAT_T, float_lit->value());
                                }
                            } else if (auto* bina = dynamic_cast<frontend::ast::BinaryExpr*>(expr)) {
                                visit(*bina);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* lval = dynamic_cast<frontend::ast::LValue*>(expr)) {
                                visit(*lval);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* unary = dynamic_cast<frontend::ast::UnaryExpr*>(expr)) {
                                visit(*unary);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* call = dynamic_cast<frontend::ast::Call*>(expr)) {
                                visit(*call);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else {
                                assert(false && "不支持的标量类型");
                            }
                            list.push_back(val);
                        } else if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val)) {
                            // 递归处理子数组初始化器
                            const auto& inits = std::get<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val);
                            for (const auto& subinit : inits) {
                                flatten_initializer(subinit.get(), list);
                            }
                        } else {
                            assert(false && "初始化器类型错误");
                        }
                    };

                    // 执行扁平化
                    std::vector<Value*> flat_elements;
                    flatten_initializer(init, flat_elements);

                    // 截断或补零到数组总长度
                    if (flat_elements.size() > total_elements) {
                        flat_elements.resize(total_elements);
                    } else {
                        Value* zero_val = nullptr;
                        if (arr_type->base_type() == Int) {
                            zero_val = new ConstantInt(INT32_T, 0);
                        } else if (arr_type->base_type() == Float) {
                            zero_val = new ConstantFloat(FLOAT_T, 0.0f);
                        } else {
                            assert(false && "未知基础类型");
                        }
                        flat_elements.resize(total_elements, zero_val);
                    }

                    return flat_elements;
                };

                std::function<int(IRType*)> get_array_dimension_count;
                get_array_dimension_count = [&get_array_dimension_count](IRType* type) -> int {
                    if (auto* array_type = dynamic_cast<ArrayIRType*>(type)) {
                        return 1 + get_array_dimension_count(array_type->contained_);
                    }
                    else {
                        return 0; // 基础类型没有维度
                    }
                    assert(false && "不支持的类型");
                    return 0;
                };

                std::function<void(Value*, const std::vector<Value*>&, size_t&, std::vector<Value*>, IRType*)> store_array_elements =
                    [this, &store_array_elements](Value* array_ptr, const std::vector<Value*>& elements, size_t& offset, std::vector<Value*> indices, IRType* array_type) {
                        if (auto* array_ir_type = dynamic_cast<ArrayIRType*>(array_type)) {
                            // 若子元素是数组（非最后一维），递归累积索引
                            if (auto* contained_array = dynamic_cast<ArrayIRType*>(array_ir_type->contained_)) {
                                for (size_t i = 0; i < array_ir_type->num_elements_; ++i) {
                                    std::vector<Value*> new_indices = indices;
                                    new_indices.push_back(builder->get_int32(i)); // 累积当前维度索引
                                    store_array_elements(array_ptr, elements, offset, new_indices, contained_array);
                                }
                            } else {
                                // 最后一维，生成合并的GEP索引
                                IRType* elem_type = array_ir_type->contained_;
                                for (size_t j = 0; j < array_ir_type->num_elements_; ++j) {
                                    // 完整GEP索引：[0]（指针索引） + 累积的维度索引 + [j]（当前元素索引）
                                    std::vector<Value*> gep_indices = {builder->get_int32(0)};
                                    gep_indices.insert(gep_indices.end(), indices.begin(), indices.end());
                                    gep_indices.push_back(builder->get_int32(j));

                                    Value* elem_ptr = builder->create_gep(array_ptr, gep_indices);
                                    Value* elem_val = (offset < elements.size()) ? elements[offset] : builder->get_int32(0);
                                    builder->create_store(elem_val, elem_ptr);
                                    offset++;
                                }
                            }
                        } else {
                            assert(false && "store_array_elements: 类型不支持");
                        }
                    };

                // 构造类型和元素列表
                IRType* array_type = build_array_type(array, 0); 
                std::vector<Value *> arr = build_variable_array(array, ast.init().get(), 0); 
                auto va = builder->create_alloca(array_type); // 分配数组空间
                scope.push(name, va);
                initflag = false;

                // 存储初始化元素
                if (ast.init().get() != nullptr) {
                    size_t offset = 0; 
                    std::vector<Value*> indices; 
                    store_array_elements(va, arr, offset, indices, array_type);
                }
                return;
            }

            
        }
        else
        {
            AllocaInst *varAlloca;
            if (auto scalar = dynamic_cast<const frontend::ast::ScalarType *>(ast.type().get())) 
            {
                if (ast.init().get() == NULL)
                {
                    if (scalar->type() == Int)
                    {
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                    }
                    return;
                }
                
                const auto &expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(ast.init().get()->value());
                const frontend::ast::Expression *expr = expr_ptr.get();
                frontend::ast::Expression *test = expr_ptr.get();
                if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(expr)) {
                    if (scalar->type() == Int)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        std::int32_t value = int_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,(float)value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                } else if (auto *float_lit = dynamic_cast<const frontend::ast::FloatLiteral *>(expr)) {
                    if (scalar->type() == Int)
                    {
                        float value = float_lit->value();
                        ConstantInt val = ConstantInt(INT32_T,(int)value);
                        Constant * valpoint = new ConstantInt(INT32_T, value); 
                        varAlloca = builder->create_alloca(INT32_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                    else if (scalar->type() == Float)
                    {
                        float value = float_lit->value();
                        ConstantFloat val = ConstantFloat(FLOAT_T,value);
                        Constant * valpoint = new ConstantFloat(FLOAT_T, value); 
                        varAlloca = builder->create_alloca(FLOAT_T);
                        scope.push(name, varAlloca);
                        builder->create_store(valpoint, varAlloca);
                    }
                } else if (auto *bina = dynamic_cast<frontend::ast::BinaryExpr *>(test)){
                    visit(*bina);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(test))
                {
                    visit(*lval);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *luna = dynamic_cast<frontend::ast::UnaryExpr *>(test))
                {
                    visit(*luna);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }else if (auto *luna = dynamic_cast<frontend::ast::Call *>(test))
                {
                    visit(*luna);
                    if (recVal->IRType_ == INT32_T)
                    {
                        if (scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_sitofp(recVal,FLOAT_T);
                            builder->create_store(recVal, varAlloca);
                        }

                    } 
                    else if (recVal->IRType_ == FLOAT_T)
                    {
                        if(scalar->type() == Int)
                        {
                            varAlloca = builder->create_alloca(INT32_T);
                            scope.push(name, varAlloca);
                            recVal = builder->create_fptosi(recVal,INT32_T);
                            builder->create_store(recVal, varAlloca);
                        }
                        else if (scalar->type() == Float)
                        {
                            varAlloca = builder->create_alloca(FLOAT_T);
                            scope.push(name, varAlloca);
                            builder->create_store(recVal, varAlloca);
                        }
                    }
                }
                return;
            } 
            else if (auto array = dynamic_cast<const frontend::ast::ArrayType *>(ast.type().get())) 
            {
                // 递归构造多维数组类型
                std::function<IRType*(const frontend::ast::ArrayType*, size_t)> build_array_type;
                build_array_type = [this, &build_array_type](const frontend::ast::ArrayType* arr_type, size_t dim_idx) -> IRType* {
                    using BaseType = frontend::ast::ScalarType::Type;
                    if (dim_idx == arr_type->dimensions().size()) {
                        if (arr_type->base_type() == Int)
                            return INT32_T;
                        else if (arr_type->base_type() == Float)
                            return FLOAT_T;
                        else
                            assert(false && "未知基础类型");
                    }
                    else {
                        // 获取维度大小
                        size_t dim_size = 0;
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                            dim_size = int_lit->value();
                        }
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get()))
                        {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else if(auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get()))
                        {
                            visit(*lval);
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            dim_size = v->value_;
                        }
                        else {
                            assert(false && "数组维度表达式必须是整型字面量");
                        }
                        IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
                        return new ArrayIRType(elem_type, dim_size);
                    }
                };

                std::function<std::vector<Value*>(const frontend::ast::ArrayType*, const frontend::ast::Initializer*, size_t)> build_variable_array;
                build_variable_array = [this, &build_variable_array, &build_array_type](const frontend::ast::ArrayType* arr_type, const frontend::ast::Initializer* init, size_t dim_idx) -> std::vector<Value*> {
                    // 计算数组总元素个数（所有维度乘积）
                    auto get_dim_size = [&](const auto& dim_expr) -> size_t {
                        if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(dim_expr)) {
                            return int_lit->value();
                        } 
                        else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(dim_expr)) {
                            con = true;
                            visit(*bin);
                            con = false;
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            return v->value_;
                        }
                        else if(auto *lval = dynamic_cast<frontend::ast::LValue *>(dim_expr)) {
                            visit(*lval);
                            auto v = dynamic_cast<ConstantInt *>(recVal);
                            return v->value_;
                        }
                        else {
                            assert(false && "数组维度表达式必须是整型字面量");
                            return 0;
                        }
                    };

                    size_t total_elements = 1;
                    for (size_t i = 0; i < arr_type->dimensions().size(); ++i) {
                        total_elements *= get_dim_size(arr_type->dimensions()[i].get());
                    }

                    // 处理无初始化器的情况
                    if (init == nullptr) {
                        if (initflag) {
                            // 生成全零数组
                            Value* zero_val = nullptr;
                            if (arr_type->base_type() == Int) {
                                zero_val = new ConstantInt(INT32_T, 0);
                            } else if (arr_type->base_type() == Float) {
                                zero_val = new ConstantFloat(FLOAT_T, 0.0f);
                            } else {
                                assert(false && "未知基础类型");
                            }
                            return std::vector<Value*>(total_elements, zero_val);
                        } else {
                            return {};
                        }
                    }

                    initflag = true;

                    // 显式声明递归函数类型，解决auto类型推断问题
                    std::function<void(const frontend::ast::Initializer*, std::vector<Value*>&)> flatten_initializer;
                    
                    // 定义扁平化初始化器函数，处理标量和子数组的递归收集
                    flatten_initializer = [&](const frontend::ast::Initializer* init, std::vector<Value*>& list) -> void {
                        if (!init) return;

                        const auto& val = init->value();
                        if (std::holds_alternative<std::unique_ptr<frontend::ast::Expression>>(val)) {
                            // 处理标量表达式
                            const auto& expr_ptr = std::get<std::unique_ptr<frontend::ast::Expression>>(val);
                            frontend::ast::Expression* expr = expr_ptr.get();
                            Value* val = nullptr;

                            if (auto* int_lit = dynamic_cast<const frontend::ast::IntLiteral*>(expr)) {
                                if (arr_type->base_type() == Int) {
                                    val = new ConstantInt(INT32_T, int_lit->value());
                                } else if (arr_type->base_type() == Float) {
                                    val = new ConstantFloat(FLOAT_T, (float)int_lit->value());
                                }
                            } else if (auto* float_lit = dynamic_cast<const frontend::ast::FloatLiteral*>(expr)) {
                                if (arr_type->base_type() == Int) {
                                    val = new ConstantInt(INT32_T, (int)float_lit->value());
                                } else if (arr_type->base_type() == Float) {
                                    val = new ConstantFloat(FLOAT_T, float_lit->value());
                                }
                            } else if (auto* bina = dynamic_cast<frontend::ast::BinaryExpr*>(expr)) {
                                visit(*bina);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* lval = dynamic_cast<frontend::ast::LValue*>(expr)) {
                                visit(*lval);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* unary = dynamic_cast<frontend::ast::UnaryExpr*>(expr)) {
                                visit(*unary);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else if (auto* call = dynamic_cast<frontend::ast::Call*>(expr)) {
                                visit(*call);
                                if (recVal->IRType_ == INT32_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = recVal;
                                    } else if (arr_type->base_type() == Float) {
                                        val = builder->create_sitofp(recVal, FLOAT_T);
                                    }
                                } else if (recVal->IRType_ == FLOAT_T) {
                                    if (arr_type->base_type() == Int) {
                                        val = builder->create_fptosi(recVal, INT32_T);
                                    } else if (arr_type->base_type() == Float) {
                                        val = recVal;
                                    }
                                }
                            } else {
                                assert(false && "不支持的标量类型");
                            }
                            list.push_back(val);
                        } else if (std::holds_alternative<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val)) {
                            // 递归处理子数组初始化器
                            const auto& inits = std::get<std::vector<std::unique_ptr<frontend::ast::Initializer>>>(val);
                            for (const auto& subinit : inits) {
                                flatten_initializer(subinit.get(), list);
                            }
                        } else {
                            assert(false && "初始化器类型错误");
                        }
                    };

                    // 执行扁平化
                    std::vector<Value*> flat_elements;
                    flatten_initializer(init, flat_elements);

                    // 截断或补零到数组总长度
                    if (flat_elements.size() > total_elements) {
                        flat_elements.resize(total_elements);
                    } else {
                        Value* zero_val = nullptr;
                        if (arr_type->base_type() == Int) {
                            zero_val = new ConstantInt(INT32_T, 0);
                        } else if (arr_type->base_type() == Float) {
                            zero_val = new ConstantFloat(FLOAT_T, 0.0f);
                        } else {
                            assert(false && "未知基础类型");
                        }
                        flat_elements.resize(total_elements, zero_val);
                    }

                    return flat_elements;
                };

                std::function<int(IRType*)> get_array_dimension_count;
                get_array_dimension_count = [&get_array_dimension_count](IRType* type) -> int {
                    if (auto* array_type = dynamic_cast<ArrayIRType*>(type)) {
                        return 1 + get_array_dimension_count(array_type->contained_);
                    }
                    else {
                        return 0; // 基础类型没有维度
                    }
                    assert(false && "不支持的类型");
                    return 0;
                };

                std::function<void(Value*, const std::vector<Value*>&, size_t&, std::vector<Value*>, IRType*)> store_array_elements =
                    [this, &store_array_elements](Value* array_ptr, const std::vector<Value*>& elements, size_t& offset, std::vector<Value*> indices, IRType* array_type) {
                        if (auto* array_ir_type = dynamic_cast<ArrayIRType*>(array_type)) {
                            // 若子元素是数组（非最后一维），递归累积索引
                            if (auto* contained_array = dynamic_cast<ArrayIRType*>(array_ir_type->contained_)) {
                                for (size_t i = 0; i < array_ir_type->num_elements_; ++i) {
                                    std::vector<Value*> new_indices = indices;
                                    new_indices.push_back(builder->get_int32(i)); // 累积当前维度索引
                                    store_array_elements(array_ptr, elements, offset, new_indices, contained_array);
                                }
                            } else {
                                // 最后一维，生成合并的GEP索引
                                IRType* elem_type = array_ir_type->contained_;
                                for (size_t j = 0; j < array_ir_type->num_elements_; ++j) {
                                    // 完整GEP索引：[0]（指针索引） + 累积的维度索引 + [j]（当前元素索引）
                                    std::vector<Value*> gep_indices = {builder->get_int32(0)};
                                    gep_indices.insert(gep_indices.end(), indices.begin(), indices.end());
                                    gep_indices.push_back(builder->get_int32(j));

                                    Value* elem_ptr = builder->create_gep(array_ptr, gep_indices);
                                    Value* elem_val = (offset < elements.size()) ? elements[offset] : builder->get_int32(0);
                                    builder->create_store(elem_val, elem_ptr);
                                    offset++;
                                }
                            }
                        } else {
                            assert(false && "store_array_elements: 类型不支持");
                        }
                    };

                // 构造类型和元素列表
                IRType* array_type = build_array_type(array, 0); 
                std::vector<Value *> arr = build_variable_array(array, ast.init().get(), 0); 
                auto va = builder->create_alloca(array_type); // 分配数组空间
                scope.push(name, va);
                initflag = false;

                // 存储初始化元素
                if (ast.init().get() != nullptr) {
                    size_t offset = 0; 
                    std::vector<Value*> indices; 
                    store_array_elements(va, arr, offset, indices, array_type);
                }
                return;
            }


        }
    }
}




void IRVisitor::visit(frontend::ast::Function &ast)
{
    newFun = true;
    params.clear();
    paramNames.clear();
    std::string name = ast.ident().name();
    IRType *type;
    if (ast.type() && (ast.type().get())->type() == Int)
    {
        type = INT32_T;
    }
    else if (ast.type() && (ast.type().get())->type() == Float)
    {
        type = FLOAT_T;
    }
    else
    {
        type = VOID_T;
    }
    for (const auto &param : ast.params()) {
        if (param)
            visit(*param);
    }
    auto funTy = new FunctionIRType(type, params);
    auto func = new ::Function(funTy,name,module.get());
    curFun = func;
    scope.push(name, func); 
    scope.enter();
    std::vector<Value *> args; 
    for (auto arg = func->arguments_.begin(); arg != func->arguments_.end(); arg++)
        args.push_back(*arg);
    auto bb = new BasicBlock(module.get(), "label_entry", func);
    builder->BB_ = bb;
    for (int i = 0; i < (int)(paramNames.size()); i++) {
        auto alloc = builder->create_alloca(params[i]); 
        builder->create_store(args[i], alloc);          
        scope.push(paramNames[i], alloc);         
    }
    //创建统一return分支
    retBB = new BasicBlock(module.get(), "label_ret", func);
    if (type == VOID_T) {
        builder->BB_ = retBB;
        builder->create_void_ret();
    } else {
        retAlloca = builder->create_alloca(type); 
        builder->BB_ = retBB;
        auto retLoad = builder->create_load(retAlloca);
        builder->create_ret(retLoad);
    }

    builder->BB_ = bb;
    br = false;
    visit(*ast.body());

    if (!builder->BB_->get_terminator())
        builder->create_br(retBB);

}

void IRVisitor::visit(frontend::ast::Parameter &ast) {
    IRType *paramType = nullptr;

    std::function<IRType*(const frontend::ast::ArrayType*, size_t)> build_array_type;
    build_array_type = [&](const frontend::ast::ArrayType* arr_type, size_t dim_idx) -> IRType* {
        if (dim_idx == arr_type->dimensions().size()) {
            if (arr_type->base_type() == Int)
                return INT32_T;
            else if (arr_type->base_type() == Float)
                return FLOAT_T;
            else
                assert(false && "未知基础类型");
        } 
        else {
            // 先检查当前维度是否为空（表示该维度是退化维度）
            if (!arr_type->dimensions()[dim_idx]) {
                // 这个维度不定长，直接递归下一维
                return build_array_type(arr_type, dim_idx + 1);
            }
            // 当前维度有维度大小

            unsigned dim_size = 0;
            if (auto *int_lit = dynamic_cast<const frontend::ast::IntLiteral *>(arr_type->dimensions()[dim_idx].get())) {
                dim_size = int_lit->value();
            } 
            else if (auto *bin = dynamic_cast<frontend::ast::BinaryExpr *>(arr_type->dimensions()[dim_idx].get())) {
                con = true;
                visit(*bin);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim_size = v->value_;
            }
            else if (auto *lval = dynamic_cast<frontend::ast::LValue *>(arr_type->dimensions()[dim_idx].get())) {
                con = true;
                visit(*lval);
                con = false;
                auto v = dynamic_cast<ConstantInt *>(recVal);
                dim_size = v->value_;
            }
            else {
                assert(false && "数组维度表达式必须是整型字面量");
            }

            IRType* elem_type = build_array_type(arr_type, dim_idx + 1);
            return new ArrayIRType(elem_type, dim_size);
        }
    };

    if (auto scalar = dynamic_cast<const frontend::ast::ScalarType *>(ast.type().get())) {
        if (scalar->type() == Int)
            paramType = INT32_T;
        else if (scalar->type() == Float)
            paramType = FLOAT_T;
    }
    else if (auto array = dynamic_cast<const frontend::ast::ArrayType *>(ast.type().get())) {
        size_t dims = array->dimensions().size();
        if (dims == 0) {
            // int a[]，无维度时，退化为指针指向基础类型
            if (array->base_type() == Int)
                paramType = new PointerIRType(INT32_T);
            else if (array->base_type() == Float)
                paramType = new PointerIRType(FLOAT_T);
            else
                assert(false && "未知基础类型");
        } 
        else {
            
            if (!array->dimensions()[0]) {
                // 退化为指针，指向剩余维度数组类型
                IRType* inner_type = build_array_type(array, 1);
                paramType = new PointerIRType(inner_type);
            } else {
                // 第0维有维度大小，构造完整数组类型
                paramType = build_array_type(array, 0);
                paramType = new PointerIRType(paramType);
            }
        }
    }
    params.push_back(paramType);
    paramNames.push_back(ast.ident().name());
}

void IRVisitor::visit(frontend::ast::Block &ast)
{
    if (newFun)
        newFun = false;
    else {
        scope.enter();
    }
    //遍历每一个语句块
    for (const auto &child : ast.children()) {
        if (std::holds_alternative<std::unique_ptr<frontend::ast::Declaration>>(child)) {
            auto &decl = std::get<std::unique_ptr<frontend::ast::Declaration>>(child);
            visit(*decl);
        } else if (std::holds_alternative<std::unique_ptr<frontend::ast::Statement>>(child)) {
            auto &stmt = std::get<std::unique_ptr<frontend::ast::Statement>>(child);
            visit(*stmt);
        }
    }
    scope.exit();

}


void IRVisitor::visit(frontend::ast::Statement &ast)
{
    if (auto expr = dynamic_cast<frontend::ast::ExprStmt*>(&ast)) {
        lvcon = true;
        visit(*expr);
        lvcon = false;
    } else if (auto l = dynamic_cast<frontend::ast::Assignment*>(&ast)) {
        LValflag = true;
        visit(*l->lhs());
        auto lVal = recVal;
        auto lValType = static_cast<PointerIRType *>(lVal->IRType_)->contained_;
        visit(*l->rhs());
        auto rVal = recVal;
        if (lValType != recVal->IRType_) {
            if (lValType == FLOAT_T) {
                rVal = builder->create_sitofp(recVal, FLOAT_T);
            } else {
                rVal = builder->create_fptosi(recVal, INT32_T);
            }
        }
        if (rVal->IRType_->tid_ != lValType ->tid_)
        {
            std::cout << lVal->IRType_->tid_ << "     " << rVal->IRType_->tid_ << std::endl;
        }
        builder->create_store(rVal, lVal);
    } else if (auto exp = dynamic_cast<frontend::ast::Expression*>(&ast)) {
        visit(*exp);
    } else if (auto ifel = dynamic_cast<frontend::ast::IfElse*>(&ast)) {

        auto tempTrue = trueBB;
        auto tempFalse = falseBB;

        trueBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
        falseBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
        BasicBlock* nextIf; // if语句后的基本块
        if (ifel->otherwise() == nullptr) nextIf = falseBB;
        else nextIf = new BasicBlock(module.get(), std::to_string(id++), curFun);
        visit(*ifel->cond());

        if (recVal->IRType_ == INT32_T) {
            recVal = builder->create_icmp_ne(recVal, new ConstantInt(module->int32_ty_, 0));
        } else if (recVal->IRType_ == FLOAT_T) {
            recVal = builder->create_fcmp_ne(recVal, new ConstantFloat(module->float32_ty_, 0));
        }
        builder->create_cond_br(recVal, trueBB, falseBB);

        builder->BB_ = trueBB; 
        br = false;
        visit(*ifel->then());
        if (!builder->BB_->get_terminator()) {
            builder->create_br(nextIf);
        }

        if (ifel->otherwise() != nullptr) { 
            builder->BB_ = falseBB;
            br = false;
            visit(*ifel->otherwise());
            if (!builder->BB_->get_terminator()) {
                builder->create_br(nextIf);
            }
        }

        builder->BB_ = nextIf;
        br = false;
        // 还原trueBB和falseBB
        trueBB = tempTrue;
        falseBB = tempFalse;
    } else if (auto whi = dynamic_cast<frontend::ast::While*>(&ast)) {
        
        auto tempTrue = trueBB;
        auto tempFalse = falseBB; 
        auto tempCond = whileCondBB;
        auto tempWhileFalseBB = whileFalseBB; 

        whileCondBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
        trueBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
        falseBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
        whileFalseBB = falseBB;

        builder->create_br(whileCondBB);
        builder->BB_ = whileCondBB; //条件也是一个基本块
        br = false;
        visit(*whi->cond());
        if (recVal->IRType_ == INT32_T) {
            recVal = builder->create_icmp_ne(recVal, new ConstantInt(module->int32_ty_, 0));
        } else if (recVal->IRType_ == FLOAT_T) {
            recVal = builder->create_fcmp_ne(recVal, new ConstantFloat(module->float32_ty_, 0));
        }
        builder->create_cond_br(recVal, trueBB, falseBB);

        builder->BB_ = trueBB;
        br = false;
        visit(*whi->body());

        if (!builder->BB_->get_terminator()) {
            builder->create_br(whileCondBB);
        }

        builder->BB_ = falseBB;
        br = false;

        trueBB = tempTrue;
        falseBB = tempFalse;
        whileCondBB = tempCond;
        whileFalseBB = tempWhileFalseBB;
    } else if (dynamic_cast<frontend::ast::Break*>(&ast)) {
        builder->create_br(whileFalseBB);
        br = true;
    } else if (dynamic_cast<frontend::ast::Continue*>(&ast)) {
        builder->create_br(whileCondBB);
        br = true;
    } else if (auto ret = dynamic_cast<frontend::ast::Return*>(&ast)) {
        visit(*ret);
    } else if (auto BB = dynamic_cast<frontend::ast::Block*>(&ast)) {
        visit(*BB);
    } else {
        std::cout << "ERROR.\n";
        assert("ERROR");
    }
}

void IRVisitor::visit(frontend::ast::Return &ast)
{
    if (ast.res() == nullptr) {
        recVal = builder->create_br(retBB);
    } else {

        visit(*ast.res());
        //类型转换
        if (recVal->IRType_ == FLOAT_T && curFun->get_return_IRType() == INT32_T) {
            auto temp = builder->create_fptosi(recVal, INT32_T);
            builder->create_store(temp, retAlloca);
        } else if (recVal->IRType_ == INT32_T && curFun->get_return_IRType() == FLOAT_T) {
            auto temp = builder->create_sitofp(recVal, FLOAT_T);
            builder->create_store(temp, retAlloca);
        } else{
            builder->create_store(recVal, retAlloca);
        }
        recVal = builder->create_br(retBB);
    }
    br = true;
}

void IRVisitor::visit(frontend::ast::ExprStmt &ast)
{
    visit(*ast.expr());
}

void IRVisitor::visit(frontend::ast::Expression &ast)
{
    if (auto lval = dynamic_cast<frontend::ast::LValue *>(&ast))
    {
        bool isTrueLVal = LValflag; 
        LValflag = false;
        auto var = scope.find(lval->ident().name());

        if (scope.global_scope() || flag == true)
        {

            if (var->IRType_ == INT32_T || var->IRType_ == FLOAT_T)
            {
                recVal = var;
                return;
            }
            
            std::vector<int> index;
            for (auto &exp : lval->indices())
            {
                visit(*exp);
                index.push_back(dynamic_cast<ConstantInt *>(recVal)->value_);
            }
            recVal = ((GlobalVariable *)var)->init_val_; 
            for (int i : index)
            {
                
                if (dynamic_cast<ConstantZero *>(recVal))
                {
                    IRType *arrayTy = recVal->IRType_;
                   
                    while (dynamic_cast<ArrayIRType *>(arrayTy))
                    {
                        arrayTy = dynamic_cast<ArrayIRType *>(arrayTy)->contained_;
                    }
                    if (arrayTy == INT32_T)
                        recVal = new ConstantInt(module->int32_ty_, 0);
                    else
                        recVal = new ConstantFloat(module->float32_ty_, 0);
                    return;
                }
                if (dynamic_cast<ConstantArray *>(recVal))
                {
                    recVal = ((ConstantArray *)recVal)->const_array[i];
                }
            }
            return;
        }
        // 局部作用域
        if (var->IRType_->tid_ == IRType::IntegerTyID || var->IRType_->tid_ == IRType::FloatTyID)
        { 
            recVal = var;
            return;
        }

        
        IRType *varType = static_cast<PointerIRType *>(var->IRType_)->contained_; 
        if (varType->tid_ == IRType::ArrayTyID || varType->tid_ == IRType::PointerTyID) 
        { 
            std::vector<Value *> idxs;
            for (auto &exp : lval->indices())
            {
                visit(*exp);
                idxs.push_back(recVal);
            }

            if (varType->tid_ == IRType::PointerTyID)
            {
                var = builder->create_load(var);
            }
            else if (varType->tid_ == IRType::ArrayTyID)
            {
                idxs.insert(idxs.begin(), new ConstantInt(module->int32_ty_, 0));
            }
            var = builder->create_gep(var, idxs); 
            varType = ((PointerIRType *)var->IRType_)->contained_;
        }

        if (varType->tid_ == IRType::ArrayTyID)
        {
            recVal = builder->create_gep(var, {new ConstantInt(module->int32_ty_, 0), new ConstantInt(module->int32_ty_, 0)});
        }
        else if (!isTrueLVal)
        { 
            recVal = builder->create_load(var);
        }
        else
        { 
            recVal = var;
        }
    }
    else if (auto una = dynamic_cast<frontend::ast::UnaryExpr *>(&ast))
    {
            if (con) {
                if (auto unal = dynamic_cast<frontend::ast::UnaryExpr *>(una->operand().get()) ) {
                    visit(*unal);
                }
                if (auto uival = dynamic_cast<frontend::ast::IntLiteral *>(una->operand().get()))
                {
                    visit(*uival);
                }
                if (auto ufval = dynamic_cast<frontend::ast::FloatLiteral *>(una->operand().get()))
                {
                    visit(*ufval);
                }
                if (auto ubval = dynamic_cast<frontend::ast::BinaryExpr *>(una->operand().get()))
                {
                    visit(*ubval);
                }
                if (auto ulval = dynamic_cast<frontend::ast::LValue *>(una->operand().get()))
                {
                    visit(*ulval);
                }
                if (una->op() == UnaryOp::Sub) {
                    
                    if (dynamic_cast<ConstantInt*>(recVal)) {
                        auto temp2 = (ConstantInt*)recVal;
                        ConstantInt * temp = new ConstantInt(temp2->IRType_,temp2->value_);
                        temp->value_ = -temp->value_;
                        recVal = temp;
                    } else {
                        auto temp2 = (ConstantFloat*)recVal;
                        ConstantFloat * temp = new ConstantFloat(temp2->IRType_,temp2->value_);
                        temp->value_ = -temp->value_;
                        recVal = temp;
                    }
                }
                return;
            }
       
                if (auto unal = dynamic_cast<frontend::ast::UnaryExpr *>(una->operand().get()) ) {
                    visit(*unal);
                }
                if (auto uival = dynamic_cast<frontend::ast::IntLiteral *>(una->operand().get()))
                {
                    visit(*uival);
                }
                if (auto ufval = dynamic_cast<frontend::ast::FloatLiteral *>(una->operand().get()))
                {
                    visit(*ufval);
                }
                if (auto ubval = dynamic_cast<frontend::ast::BinaryExpr *>(una->operand().get()))
                {
                    visit(*ubval);
                }
                if (auto ulval = dynamic_cast<frontend::ast::LValue *>(una->operand().get()))
                {
                    visit(*ulval);
                }
                if (auto ucal = dynamic_cast<frontend::ast::Call *>(una->operand().get()) ) {
                    visit(*ucal);
                }

                if (recVal->IRType_ == VOID_T)
                    return;
                else if (recVal->IRType_ == INT1_T) // INT1-->INT32
                    recVal = builder->create_zext(recVal, INT32_T);

                if (recVal->IRType_ == INT32_T)
                {
                    switch (una->op())
                    {
                    case UnaryOp::Sub:
                        recVal = builder->create_isub(new ConstantInt(module->int32_ty_, 0), recVal);
                        break;
                    case UnaryOp::Not:
                        recVal = builder->create_icmp_eq(recVal, new ConstantInt(module->int32_ty_, 0));
                        break;
                    case UnaryOp::Add:
                        break;
                    }
                }
                else
                {
                    switch (una->op())
                    {
                    case UnaryOp::Sub:
                        recVal = builder->create_fsub(new ConstantFloat(module->float32_ty_, 0), recVal);
                        break;
                    case UnaryOp::Not:
                        recVal = builder->create_fcmp_eq(recVal, new ConstantFloat(module->float32_ty_, 0));
                        break;
                    case UnaryOp::Add:
                        break;
                    }
                }
    }
    else if (auto cal = dynamic_cast<frontend::ast::Call *>(&ast))
    {
        auto fun = (Function *)scope.find(cal->func().name());
    
        if (fun->basic_blocks_.size() && !lvcon)
            fun->use_ret_cnt ++ ;
        lvcon = false;
        std::vector<Value *> args;
        for (int i = 0; i < cal->args().size(); i++) {
            auto& exprPtr = std::get<std::unique_ptr<frontend::ast::Expression>>(cal->args()[i]);
            visit(*exprPtr.get());

            if (recVal->IRType_ == INT32_T && fun->arguments_[i]->IRType_ == FLOAT_T) {
                recVal = builder->create_sitofp(recVal, FLOAT_T);
            } else if (recVal->IRType_ == FLOAT_T && fun->arguments_[i]->IRType_ == INT32_T) {
                recVal = builder->create_fptosi(recVal, INT32_T);
            }
            args.push_back(recVal);
        }
        recVal = builder->create_call(fun, args);
    }
    else if (auto bin = dynamic_cast<frontend::ast::BinaryExpr *>(&ast))
    {
        BinaryOp option;
        option = bin->op();
        Value *v1 = nullptr;
        Value *v2 = nullptr;
        if (option == BinaryOp::And)
        {
            auto tempTrue = trueBB; 
            trueBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
            visit(*bin->lhs().get());
            if (recVal->IRType_ == INT32_T)
                recVal = builder->create_icmp_ne(recVal, new ConstantInt(module->int32_ty_, 0));
            else if (recVal->IRType_ == FLOAT_T)
                recVal = builder->create_fcmp_ne(recVal, new ConstantFloat(module->float32_ty_, 0));
            builder->create_cond_br(recVal, trueBB, falseBB);
            builder->BB_ = trueBB;
            br = false;
            trueBB = tempTrue; 
            visit(*bin->rhs().get());
        }
        if (option == BinaryOp::Or)
        {
            auto tempFalse = falseBB; 
            falseBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
            visit(*bin->lhs().get());
            if (recVal->IRType_ == INT32_T)
                recVal = builder->create_icmp_ne(recVal, new ConstantInt(module->int32_ty_, 0));
            else if (recVal->IRType_ == FLOAT_T)
                recVal = builder->create_fcmp_ne(recVal, new ConstantFloat(module->float32_ty_, 0));
            builder->create_cond_br(recVal, trueBB, falseBB);
            recVal = v2;
            builder->BB_ = falseBB;
            br = false;
            falseBB = tempFalse;
            visit(*bin->rhs().get());
        }
        if (auto left = dynamic_cast<frontend::ast::BinaryExpr *>(bin->lhs().get()))
        {
            visit(*left);
            v1 = recVal;
        }
        else if (auto llval = dynamic_cast<frontend::ast::LValue *>(bin->lhs().get()))
        {
            visit(*llval);
            v1 = recVal;
        }
        else if (auto luna = dynamic_cast<frontend::ast::UnaryExpr *>(bin->lhs().get()))
        {
            visit(*luna);
            v1 = recVal;
        }
        else if (auto lcall = dynamic_cast<frontend::ast::Call *>(bin->lhs().get()))
        {
            visit(*lcall);
            v1 = recVal;
        }
        else
        {
            if (auto left = dynamic_cast<frontend::ast::IntLiteral *>(bin->lhs().get()))
            {
                visit(*left);
                v1 = recVal;
            }
            else if (auto left = dynamic_cast<frontend::ast::FloatLiteral *>(bin->lhs().get()))
            {
                visit(*left);
                v1 = recVal;
            }
        }
        if (auto right = dynamic_cast<frontend::ast::BinaryExpr *>(bin->rhs().get()))
        {
            visit(*right);
            v2 = recVal;
        }
        else if (auto rlval = dynamic_cast<frontend::ast::LValue *>(bin->rhs().get()))
        {
            visit(*rlval);
            v2 = recVal;
        }
        else if (auto runa = dynamic_cast<frontend::ast::UnaryExpr *>(bin->rhs().get()))
        {
            visit(*runa);
            v2 = recVal;
        }
        else if (auto rcall = dynamic_cast<frontend::ast::Call *>(bin->rhs().get()))
        {
            visit(*rcall);
            v2 = recVal;
        }
        else
        {
            if (auto right = dynamic_cast<frontend::ast::IntLiteral *>(bin->rhs().get()))
            {
                visit(*right);
                v2 = recVal;
            }
            else if (auto right = dynamic_cast<frontend::ast::FloatLiteral *>(bin->rhs().get()))
            {
                visit(*right);
                v2 = recVal;
            }
        }
        if (v1->IRType_ == INT32_T && v2->IRType_ == FLOAT_T)
        {   
            if (conflag == true)
            {
                auto ls = dynamic_cast<ConstantInt *>(v1);
                ConstantFloat* temp = new ConstantFloat(FLOAT_T,(float)ls->value_); 
                v1 = temp;
            }
            else
                v1 = builder->create_sitofp(v1,FLOAT_T);

        }
        if (v2->IRType_ == INT32_T && v1->IRType_ == FLOAT_T)
        {
            if(conflag == true)
            {
                auto ls = dynamic_cast<ConstantInt *>(v2);
                ConstantFloat* temp = new ConstantFloat(FLOAT_T,(float)ls->value_); 
                v2 = temp;
            }
            else
                v2 = builder->create_sitofp(v2,FLOAT_T);
            
        }

        if (con == false)
        {
            if (option == BinaryOp::Add)
            {
                if (v1->IRType_ == INT32_T)
                {
                    recVal = builder->create_iadd(v1, v2);
                }    
                else
                    recVal = builder->create_fadd(v1, v2);
                return;
            }
            else if (option == BinaryOp::Sub)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = builder->create_isub(v1, v2);
                else
                    recVal = builder->create_fsub(v1, v2);
                return;
            }
            else if (option == BinaryOp::Mul)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = builder->create_imul(v1, v2);
                else
                    recVal = builder->create_fmul(v1, v2);
                return;
            }
            else if (option == BinaryOp::Div)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = builder->create_isdiv(v1, v2);
                else
                    recVal = builder->create_fdiv(v1, v2);
                return;
            }
            else if (option == BinaryOp::Mod)
            {
                recVal = builder->create_isrem(v1, v2);
                return;
            }
            else if (option == BinaryOp::Eq)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_eq(v1, v2);
                else
                    recVal = builder->create_icmp_eq(v1, v2);
                return;
            }
            else if (option == BinaryOp::Geq)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_ge(v1, v2);
                else
                    recVal = builder->create_icmp_ge(v1, v2);
                return;
            }
            else if (option == BinaryOp::Gt)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_gt(v1, v2);
                else
                    recVal = builder->create_icmp_gt(v1, v2);
                return;
            }
            else if (option == BinaryOp::Leq)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_le(v1, v2);
                else
                    recVal = builder->create_icmp_le(v1, v2);
                return;
            }
            else if (option == BinaryOp::Lt)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_lt(v1, v2);
                else
                    recVal = builder->create_icmp_lt(v1, v2);
                return;
            }
            else if (option == BinaryOp::Neq)
            {
                if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_ne(v1, v2);
                else 
                    recVal = builder->create_icmp_ne(v1, v2);
                return;
            }
            /*
            else if (option == BinaryOp::And)
            {
                auto tempTrue = trueBB; // 防止嵌套and导致原trueBB丢失。用于生成短路模块
                trueBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
                if (v1->IRType_ == INT32_T)
                    recVal = builder->create_icmp_ne(v1, CONST_INT(0));
                else if (v1->IRType_ == FLOAT_T)
                    recVal = builder->create_fcmp_ne(v1, CONST_FLOAT(0));
                builder->create_cond_br(recVal, trueBB, falseBB);
                recVal = v2;
                builder->BB_ = trueBB;
                br = false;
                trueBB = tempTrue; // 还原原来的true模块
            }
            else if (option == BinaryOp::Or)
            {
                auto tempFalse = falseBB; // 防止嵌套and导致原trueBB丢失。用于生成短路模块
                falseBB = new BasicBlock(module.get(), std::to_string(id++), curFun);
                recVal = builder->create_icmp_ne(recVal, CONST_INT(0));
                builder->create_cond_br(recVal, trueBB, falseBB);
                recVal = v2;
                builder->BB_ = falseBB;
                br = false;
                falseBB = tempFalse;
            }*/
        }
        else if (con == true)
        {

            if (option == BinaryOp::Add)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = new ConstantInt(module->int32_ty_, dynamic_cast<ConstantInt *>(v1)->value_ + dynamic_cast<ConstantInt *>(v2)->value_);
                else
                    recVal = new ConstantFloat(module->float32_ty_, dynamic_cast<ConstantFloat *>(v1)->value_ + dynamic_cast<ConstantFloat *>(v2)->value_);
                return;
            }
            else if (option == BinaryOp::Sub)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = new ConstantInt(module->int32_ty_, dynamic_cast<ConstantInt *>(v1)->value_ - dynamic_cast<ConstantInt *>(v2)->value_);
                else
                    recVal = new ConstantFloat(module->float32_ty_, dynamic_cast<ConstantFloat *>(v1)->value_ - dynamic_cast<ConstantFloat *>(v2)->value_);
                return;
            }
            else if (option == BinaryOp::Mul)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = new ConstantInt(module->int32_ty_, dynamic_cast<ConstantInt *>(v1)->value_ * dynamic_cast<ConstantInt *>(v2)->value_);
                else
                    recVal = new ConstantFloat(module->float32_ty_, dynamic_cast<ConstantFloat *>(v1)->value_ * dynamic_cast<ConstantFloat *>(v2)->value_);
                return;
            }
            else if (option == BinaryOp::Div)
            {
                if (v1->IRType_ == INT32_T)
                    recVal = new ConstantInt(module->int32_ty_, dynamic_cast<ConstantInt *>(v1)->value_ / dynamic_cast<ConstantInt *>(v2)->value_);
                else
                    recVal = new ConstantFloat(module->float32_ty_, dynamic_cast<ConstantFloat *>(v1)->value_ / dynamic_cast<ConstantFloat *>(v2)->value_);
                return;
            }
            else if (option == BinaryOp::Mod)
            {
                recVal = new ConstantInt(module->int32_ty_, dynamic_cast<ConstantInt *>(v1)->value_ % dynamic_cast<ConstantInt *>(v2)->value_);
                return;
            }
        }
    }
    else if (auto zx = dynamic_cast<frontend::ast::IntLiteral *>(&ast))
    {
        ConstantInt *cst = new ConstantInt(INT32_T, zx->value());
        recVal = cst;
    }
    else if (auto zx = dynamic_cast<frontend::ast::FloatLiteral *>(&ast))
    {
        ConstantFloat *cst = new ConstantFloat(FLOAT_T, zx->value());
        recVal = cst;
    }
    
}
