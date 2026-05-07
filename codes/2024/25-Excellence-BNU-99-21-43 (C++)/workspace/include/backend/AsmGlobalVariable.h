#pragma once

#include <string>
#include <vector>
#include "globalvalue.h"
#include "constant.h"
#include "AsmConstantData.h"
#include "AsmOperandLabel.h"

namespace Backend
{
    class AsmGlobalVariable
    {
    private:
        std::string name;
        bool isSmallSection, isInitialized, isConstant;
        int size;                              // size in bytes
        std::vector<AsmConstantData> dataList; // data list
        int align;                             // 2^align bytes

    public:
        AsmGlobalVariable(IR::GlobalVariable *globalVar)
        {
            this->name = "acm_global_variable_" + globalVar->name;
            this->isConstant = globalVar->isConstant;
            this->isInitialized = globalVar->isInit;

            IR::BasicType *type = globalVar->getType();
            if (!type->isPoint())
                Error::Error(__PRETTY_FUNCTION__, "The type of global variable must be pointer");

            if (!globalVar->isInitialized())
            {
                IR::BasicType *baseType = dynamic_cast<IR::PointerType *>(type)->getBaseType();
                if (baseType->isInt32())
                {
                    this->size = 4;
                    this->align = 2;
                    this->isSmallSection = true;
                    this->dataList.push_back(AsmConstantData::zero(4));
                }
                else if (baseType->isFloat())
                {
                    this->size = 4;
                    this->align = 2;
                    this->isSmallSection = true;
                    this->dataList.push_back(AsmConstantData::zero(4));
                }
                else if (baseType->isArray())
                {
                    this->size = baseType->getSize();
                    this->align = 3;
                    this->isSmallSection = false;
                    this->dataList.push_back(AsmConstantData::zero(this->size));
                }
                else
                {
                    Error::Error(__PRETTY_FUNCTION__, "Unsupported global variable type");
                }
            }
            else
            {
                IR::Constant *initializer = globalVar->initializer;
                if (initializer->isConstantInt32())
                {
                    this->size = 4;
                    this->align = 2;
                    this->isSmallSection = true;
                    this->dataList.push_back(AsmConstantData(dynamic_cast<IR::ConstantInt32 *>(initializer)->getValue()));
                }
                else if (initializer->isConstantFloat())
                {
                    this->size = 4;
                    this->align = 2;
                    this->isSmallSection = true;
                    this->dataList.push_back(AsmConstantData(dynamic_cast<IR::ConstantFloat *>(initializer)->getValue()));
                }
                else if (initializer->isConstantArray())
                {
                    this->size = dynamic_cast<IR::ConstantArray *>(initializer)->getType()->getSize();
                    this->align = 3;
                    this->isSmallSection = false;
                    initializeArrayData(dynamic_cast<IR::ConstantArray *>(initializer));
                }
                // // global variable do not contain pointer
                // else if (initializer->isConstantPointer())
                // {
                //     this->size = 8;
                //     this->align = 3;
                //     this->isSmallSection = true;
                //     this->dataList.push_back(AsmConstantData(0));
                // }
                else
                {
                    Error::Error(__PRETTY_FUNCTION__, "Unsupported global variable initializer");
                }
            }
        }

    private:
        // dfs to get all the data
        void initializeArrayData(IR::ConstantArray *constantArray)
        {
            IR::BasicType *arrayType = constantArray->getType();
            // int length = arrayType->getLength();
            IR::BasicType *baseType = arrayType->getBaseType();

            int lastInitIndex = -1;
            for (auto &[index, constant] : constantArray->elements)
            {
                if ((int)index != lastInitIndex + 1)
                {
                    dataList.push_back(AsmConstantData::zero((index - lastInitIndex - 1) * baseType->getSize()));
                }

                if (baseType->isArray())
                {
                    initializeArrayData(dynamic_cast<IR::ConstantArray *>(constant));
                }
                else
                {
                    if (baseType->isInt32())
                    {
                        dataList.push_back(AsmConstantData(dynamic_cast<IR::ConstantInt32 *>(constant)->getValue()));
                    }
                    else if (baseType->isFloat())
                    {
                        dataList.push_back(AsmConstantData(dynamic_cast<IR::ConstantFloat *>(constant)->getValue()));
                    }
                    else
                    {
                        Error::Error(__PRETTY_FUNCTION__, "Unsupported type");
                    }
                }
                lastInitIndex = index;
            }

            if (lastInitIndex + 1 < arrayType->getLength())
            {
                dataList.push_back(AsmConstantData::zero((arrayType->getLength() - lastInitIndex - 1) * baseType->getSize()));
            }
        }

    public:
        std::string emit()
        {
            std::string s;

            s += "\t.globl " + name + "\n";

            if (isConstant)
            {
                s += isSmallSection ? "\t.section .srodata,\"a\"" : "\t.section .rodata";
            }
            else if (isInitialized)
            {
                s += isSmallSection ? "\t.section .sdata,\"aw\"" : "\t.section .data";
            }
            else
            {
                s += isSmallSection ? "\t.section .sbss,\"aw\",@nobits" : "\t.bss";
            }
            s += '\n';

            s += "\t.align " + std::to_string(align) + '\n';
            s += "\t.type " + name + ", @object\n";
            s += "\t.size " + name + ", " + std::to_string(size) + '\n';

            s += name + ":\n";
            for (auto &data : dataList)
            {
                s += data.emit();
            }

            return s;
        }

        // used to get the global variable label
        AsmOperandLabel *getAsmOperandLabel()
        {
            return new AsmOperandLabel(name, true);
        }
    };
}