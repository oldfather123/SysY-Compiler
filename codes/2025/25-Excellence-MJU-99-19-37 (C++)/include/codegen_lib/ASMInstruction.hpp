#pragma once

#include <cassert>
#include <string>
#include <stdexcept>  // 添加异常支持

struct ASMInstruction {
    friend class CodeGen;
    enum InstType { Instruction, Attribute, Label, Comment } type;

    // 工厂函数提供更清晰的创建方式
    static ASMInstruction makeInstruction(std::string s) {
        return ASMInstruction(std::move(s), Instruction);
    }
    
    static ASMInstruction makeAttribute(std::string s) {
        return ASMInstruction(std::move(s), Attribute);
    }
    
    static ASMInstruction makeLabel(std::string s) {
        return ASMInstruction(std::move(s), Label);
    }
    
    static ASMInstruction makeComment(std::string s) {
        return ASMInstruction(std::move(s), Comment);
    }

    // 格式化输出，根据不同类型返回不同的格式
    std::string format() const {
        switch (type) {
        case Instruction:
        case Attribute:
            return "\t" + content + "\n";
        case Label:
            return content + ":\n";
        case Comment:
            return "# " + content + "\n";
        default:
            throw std::runtime_error("Invalid ASM instruction type: " + std::to_string(type));
        }
    }

    // 默认的拷贝和移动构造函数、赋值操作符
    ASMInstruction(const ASMInstruction &) = default;
    ASMInstruction(ASMInstruction &&) = default;
    ASMInstruction &operator=(const ASMInstruction &) = default;
    ASMInstruction &operator=(ASMInstruction &&) = default;

private:
    // 私有构造函数，避免外部直接创建对象
    explicit ASMInstruction(std::string s, InstType ty) : type(ty), content(std::move(s)) {}

    // 存储指令的内容
    std::string content;
};
