#include "ConstantMultiplyPlanner.h"

namespace Backend
{

    // 初始化单例实例
    MulPlannerVariableOperand *MulPlannerVariableOperand::instance = nullptr;
    MulPlannerNotReducible *MulPlannerNotReducible::instance = nullptr;
    std::unordered_map<long, MulPlannerOperand *> ConstantMultiplyPlanner::operandMap;
    bool ConstantMultiplyPlanner::isInitialized = false;

    // 获取 MulPlannerVariableOperand 的唯一实例
    MulPlannerVariableOperand *MulPlannerVariableOperand::getInstance()
    {
        if (!instance)
        {
            instance = new MulPlannerVariableOperand();
        }
        return instance;
    }

    // 获取 MulPlannerNotReducible 的唯一实例
    MulPlannerNotReducible *MulPlannerNotReducible::getInstance()
    {
        if (!instance)
        {
            instance = new MulPlannerNotReducible();
        }
        return instance;
    }

    // MulPlannerOperation 类构造函数
    MulPlannerOperation::MulPlannerOperation(MulPlannerOperationType type, MulPlannerOperand *operand1, MulPlannerOperand *operand2)
        : MulPlannerOperand(operand1->level + operand2->level + 1), type(type), operand1(operand1), operand2(operand2)
    {
    }

    // 添加操作到操作数映射表
    void ConstantMultiplyPlanner::addOperationIfPossible(std::vector<std::pair<long, MulPlannerOperand *>> &levelLdOperations, long idAdd, MulPlannerOperand *odAdd)
    {
        if (operandMap.find(idAdd) == operandMap.end())
        {
            operandMap[idAdd] = odAdd;
            levelLdOperations.push_back(std::make_pair(idAdd, odAdd));
        }
    }

    // 初始化映射表
    void ConstantMultiplyPlanner::initialize()
    {
        std::vector<std::vector<std::pair<long, MulPlannerOperand *>>> operations;
        auto level0Operations = std::vector<std::pair<long, MulPlannerOperand *>>();
        addOperationIfPossible(level0Operations, 0, new MulPlannerConstantOperand(0));
        addOperationIfPossible(level0Operations, 1, MulPlannerVariableOperand::getInstance());
        operations.push_back(level0Operations);

        for (int ld = 1; ld <= OPERATION_LIMIT; ++ld)
        {
            auto levelLdOperations = std::vector<std::pair<long, MulPlannerOperand *>>();
            for (int l1 = 0; l1 < ld; ++l1)
            {
                // 处理 SHL 操作
                for (auto &pair1 : operations[l1])
                {
                    auto i1 = pair1.first;
                    auto o1 = pair1.second;
                    if (i1 == 0)
                        continue;
                    for (int i = 0;; ++i)
                    {
                        auto idShl = i1 << i;
                        if (i != 0)
                        {
                            auto odShl = new MulPlannerOperation(MulPlannerOperationType::SHL, o1, new MulPlannerConstantOperand(i));
                            addOperationIfPossible(levelLdOperations, idShl, odShl);
                        }
                        if ((idShl & (1L << 63)) != 0)
                            break;
                    }
                }
                for (int l2 = 0; l1 + l2 < ld; ++l2)
                {
                    // 处理 ADD 和 SUB 操作
                    for (auto &pair1 : operations[l1])
                    {
                        auto i1 = pair1.first;
                        auto o1 = pair1.second;
                        for (auto &pair2 : operations[l2])
                        {
                            auto i2 = pair2.first;
                            auto o2 = pair2.second;
                            auto idAdd = i1 + i2;
                            auto odAdd = new MulPlannerOperation(MulPlannerOperationType::ADD, o1, o2);
                            addOperationIfPossible(levelLdOperations, idAdd, odAdd);
                            auto idSub = i1 - i2;
                            auto odSub = new MulPlannerOperation(MulPlannerOperationType::SUB, o1, o2);
                            addOperationIfPossible(levelLdOperations, idSub, odSub);
                        }
                    }
                }
            }
            operations.push_back(levelLdOperations);
        }

        for (auto &operationList : operations)
        {
            for (auto &pair : operationList)
            {
                operandMap[pair.first] = pair.second;
            }
        }
        isInitialized = true;
    }

    // 构造乘法操作计划
    MulPlannerOperand *ConstantMultiplyPlanner::makePlan(long multipliedConstant)
    {
        if (!isInitialized)
            initialize();
        return operandMap.find(multipliedConstant) != operandMap.end() ? operandMap[multipliedConstant] : MulPlannerNotReducible::getInstance();
    }

} // namespace Backend
