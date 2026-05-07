#pragma once

#include <iostream>
#include <sstream>

/// 用于展示的基类，定于了展示的接口to_string和友元操作符<<
/// 需要展示的类可以以此为父类，重写函数print
class Display {
    public:
    virtual ~Display() = default;

    [[nodiscard]] std::string to_string() const {
        std::ostringstream buffer;
        this->print(buffer, 0);
        return buffer.str();
    }

    /// 需要重写的函数
    virtual void print(std::ostream &out, unsigned int indent) const = 0;

    friend std::ostream &operator<<(std::ostream &out, Display const &self) {
        self.print(out, 0);
        return out;
    }    
};
