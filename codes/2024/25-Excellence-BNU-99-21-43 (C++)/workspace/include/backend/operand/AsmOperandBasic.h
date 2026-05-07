#pragma once
#include <string>
#include "error.h"

namespace Backend
{
    class AsmOperandBasic
    {
    public:
        enum class OperandType
        {
            REGISTER_INT,
            REGISTER_FLOAT,
            MEMORY_ADDRESS,
            IMMEDIATE,
            LABEL,
            STACK_VARIABLE,
            EX_STACK_VAR_CONTENT,
            EX_STACK_VAR_OFFSET,
        };

        static std::string operandTypeToString(OperandType type)
        {
            switch (type)
            {
            case OperandType::REGISTER_INT:
                return "register_int";
            case OperandType::REGISTER_FLOAT:
                return "register_float";
            case OperandType::MEMORY_ADDRESS:
                return "memory_address";
            case OperandType::IMMEDIATE:
                return "immediate";
            case OperandType::LABEL:
                return "label";
            case OperandType::STACK_VARIABLE:
                return "stack_variable";
            case OperandType::EX_STACK_VAR_CONTENT:
                return "ex_stack_var_content";
            case OperandType::EX_STACK_VAR_OFFSET:
                return "ex_stack_var_offset";
            default:
                return "";
            }
        }

    private:
        OperandType type;

    public:
        virtual std::string emit() const = 0;
        virtual std::string toString() const = 0;

    public:
        AsmOperandBasic(OperandType type) : type(type) {}
        bool isRegister() const { return type == OperandType::REGISTER_INT || type == OperandType::REGISTER_FLOAT; }
        bool isRegisterInt() const { return type == OperandType::REGISTER_INT; }
        bool isRegisterFloat() const { return type == OperandType::REGISTER_FLOAT; }
        bool isMemoryAddress() const { return type == OperandType::MEMORY_ADDRESS; }
        bool isImmediate() const { return type == OperandType::IMMEDIATE || isExStackVarOffset(); }
        bool isLabel() const { return type == OperandType::LABEL; }
        bool isStackVariable() const { return type == OperandType::STACK_VARIABLE || isExStackVarContent(); }
        bool isRegisterReplaceable() const { return isRegister() || isMemoryAddress() || isStackVariable(); }
        bool isExStackVarContent() const { return type == OperandType::EX_STACK_VAR_CONTENT; }
        bool isExStackVarOffset() const { return type == OperandType::EX_STACK_VAR_OFFSET; }
        bool isEXStackVarAdd() const { return isExStackVarContent() || isExStackVarOffset(); }
        OperandType getType() const { return type; }
        void setType(OperandType type) { this->type = type; }
    };
} // namespace Backend