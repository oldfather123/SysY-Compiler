#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

namespace Backend
{
    // 使用add、sub、shl三种操作，进行乘法计算
    enum class MulPlannerOperationType
    {
        ADD,
        SUB,
        SHL
    };

    class MulPlannerOperand
    {
    public:
        // 操作的等级，每进行一次算数运算加一级
        const int level;

    protected:
        explicit MulPlannerOperand(int level) : level(level) {}

        virtual ~MulPlannerOperand() = default;
    };

    class MulPlannerConstantOperand : public MulPlannerOperand
    {
    public:
        const int value;

        // 构造函数，初始化常数值并设置等级为0
        explicit MulPlannerConstantOperand(int value) : MulPlannerOperand(0), value(value) {}
    };

    class MulPlannerVariableOperand : public MulPlannerOperand
    {
    public:
        // 获取唯一实例
        static MulPlannerVariableOperand *getInstance();

    private:
        // 私有构造函数，单例模式
        MulPlannerVariableOperand() : MulPlannerOperand(0) {}

        static MulPlannerVariableOperand *instance;
    };

    class MulPlannerNotReducible : public MulPlannerOperand
    {
    public:
        // 获取唯一实例
        static MulPlannerNotReducible *getInstance();

    private:
        // 私有构造函数，等级设为最大值
        MulPlannerNotReducible() : MulPlannerOperand(0x3fffffff) {}

        static MulPlannerNotReducible *instance;
    };

    // 表示操作的类，如加法、减法、左移
    class MulPlannerOperation : public MulPlannerOperand
    {
    public:
        const MulPlannerOperationType type; // 操作类型
        MulPlannerOperand *operand1;
        MulPlannerOperand *operand2;

        // 构造函数，初始化操作类型和操作数
        MulPlannerOperation(MulPlannerOperationType type, MulPlannerOperand *operand1, MulPlannerOperand *operand2);
    };

    // 常量乘法计划器类，用于生成乘法的操作数计划
    class ConstantMultiplyPlanner
    {
    public:
        // 构造乘法操作计划
        static MulPlannerOperand *makePlan(long multipliedConstant);

    private:
        static const int OPERATION_LIMIT = 3;                  // 操作等级限制
        static std::unordered_map<long, MulPlannerOperand *> operandMap; // 操作数映射表

        // 初始化映射表
        static void initialize();

        // 添加可行的操作到操作数映射表
        static void addOperationIfPossible(std::vector<std::pair<long, MulPlannerOperand *>> &levelLdOperations, long idAdd, MulPlannerOperand *odAdd);

        static bool isInitialized;
    };

} // namespace Backend
