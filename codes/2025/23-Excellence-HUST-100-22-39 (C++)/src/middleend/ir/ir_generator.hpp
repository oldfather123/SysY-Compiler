#ifndef __IR_GENERATOR_HPP__
#define __IR_GENERATOR_HPP__

#include "ast.hpp"
#include "ir.hpp"

class IRGenerator: public Visitor {
private:
    Module* module;     // 中间代码模块
    Scope* scope;       // 作用域管理器
    IRBuilder* builder; // 中间代码生成器
    //* 解析过程中的“全局”变量
    Type* cur_type;     // 当前变量的类型
    Value* last_val;    // 上一个访问的表达式对应的值
    bool is_const;      // 当前变量是否为常量
    bool use_const;     // 当前表达式是否要求是常量
    bool has_br;        // 当前基本块是否有分支指令
    bool is_new_func;           // 是否是新函数
    Function* cur_func;         // 当前函数
    vector<Type*> param_types;  // 函数参数类型列表
    vector<string> param_names; // 函数参数名称列表
    BasicBlock* func_bb;        // 当前函数的开始块
    BasicBlock* ret_bb;         // 函数返回的返回块
    Value* ret_val;             // 函数返回值
    bool require_l_val; // 告诉 l_val 结点不需要进行 Load
    bool is_single_exp; // 用于判断是否是单个表达式
    BasicBlock* while_cond_bb;  // while 循环的条件块
    BasicBlock* while_next_bb;  // while 循环的下一步块
    BasicBlock* true_bb;        // 用于短路逻辑的true分支
    BasicBlock* false_bb;       // 用于短路逻辑的false分支
    int id; // 用于生成唯一的基本块名称 // TODO: 考虑到会统一设置名称，这里可以考虑删除

    /// @brief 对 last_val 进行隐式类型转换，使其与 cur_type 匹配
    void implicit_cast();

    /// @brief 获取下一个维度的索引（带计数限制）
    /// @param dim_count 各维度元素数量
    /// @param up 当前对齐的维度下标
    /// @param cnt 当前已初始化的元素数
    /// @return 下一个可对齐的维度索引，若找不到则返回 0。
    /// 如：嵌套大括号数组的维度，即倒数连续0的第一个。
    /// 即：[0,1,0,0]，返回2；[0,0,0,1]，返回4； 若全是0，[0,0,0,0],返回1
    int get_next_dim(vector<int>& dim_count, int up, int cnt);

    /// @brief 获取下一个维度的索引（全局初始化专用）
    /// 根据初始化的量决定嵌套数组的维度
    /// @param element_count 各维度已初始化的元素数量
    /// @param up 当前对齐的维度下标
    /// @return 下一个嵌套维度的索引
    int get_next_dim(vector<int>& element_count, int up);

    /// @brief 递归初始化局部数组
    /// @param ptr 数组首地址
    /// @param list 初始化值列表（可能包含子列表）
    /// @param dim_count 各维度元素数量
    /// @param up 子数组的最高对齐维度索引
    void init_local_array(Value* ptr, vector<unique_ptr<InitValAST>>& list, vector<int>& dim_count, int up);

    /// @brief 递归初始化全局数组
    /// @param dimensions 各维度长度
    /// @param array_types 各维度对应的 ArrayType 类型
    /// @param up 当前对齐的维度下标
    /// @param list 初始化值列表
    /// @return 构造好的 ConstArray 常量
    ConstArray* init_global_array(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, vector<unique_ptr<InitValAST>>& list);

    /// @brief 向元素列表添加元素后，合并可对齐的子数组
    /// @param dimensions 各维度长度
    /// @param array_types 各维度类型
    /// @param up 起始维度下标
    /// @param dim_add 刚添加元素所在的维度下标
    /// @param elements 元素列表（可能是常量或 ConstArray）
    /// @param element_count 各维度已初始化的元素数量
    void merge_elements(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, int dim_add, vector<Const*>& elements, vector<int>& element_count);

    /// @brief 最终合并所有元素，补齐缺失部分
    /// @param dimensions 各维度长度
    /// @param array_types 各维度类型
    /// @param up 起始维度下标
    /// @param elements 元素列表
    /// @param element_count 各维度已初始化的元素数量
    void final_merge(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, vector<Const*>& elements, vector<int>& element_count);

    /// @brief 检查并提取常量运算的类型与值
    /// @param val 两个操作数
    /// @param int_vals 存放整型常量值的数组
    /// @param float_vals 存放浮点常量值的数组
    /// @return 是否为整型常量运算
    bool check_const_cal_type(Value* val[], int int_vals[], float float_vals[]);

    /// @brief 检查并统一非常量运算的操作数类型
    /// @note 将 int1 扩展为 int32
    /// @note 若一方为 int32，另一方为 float，则将 int32 转为 float
    /// @note 用于生成二元运算的前置类型匹配
    /// @param val 两个操作数（可能被修改）
    void check_not_const_cal_type(Value* val[]);
public:
    /// @brief 构造函数
    /// @note 初始化 Module、Scope 和 IRBuilder
    /// @note 初始化库函数，将其声明添加至模块
    IRGenerator();

    /// @brief 析构函数
    /// @note 释放 Module、Scope 和 IRBuilder
    ~IRGenerator();

    /// @brief 获取当前模块，用于传递中间代码
    /// @return 当前模块指针
    Module* get_module();

    /// @brief 中间代码生成器主体逻辑
    void visit(CompUnitAST& ast) override;
    void visit(DeclDefAST& ast) override;
    void visit(DeclAST& ast) override;
    void visit(DefAST& ast) override;
    void visit(InitValAST& ast) override;
    void visit(FuncDefAST& ast) override;
    void visit(FuncParamAST& ast) override;
    void visit(BlockAST& ast) override;
    void visit(BlockItemAST& ast) override;
    void visit(StmtAST& ast) override;
    void visit(ReturnStmtAST& ast) override;
    void visit(CondStmtAST& ast) override;
    void visit(LoopStmtAST& ast) override;
    void visit(AddExpAST& ast) override;
    void visit(LValAST& ast) override;
    void visit(MulExpAST& ast) override;
    void visit(UnaryExpAST& ast) override;
    void visit(PrimaryExpAST& ast) override;
    void visit(CallAST& ast) override;
    void visit(NumberAST& ast) override;
    void visit(RelExpAST& ast) override;
    void visit(EqExpAST& ast) override;
    void visit(LAndExpAST& ast) override;
    void visit(LOrExpAST& ast) override;
};

#endif // __IR_GENERATOR_HPP__