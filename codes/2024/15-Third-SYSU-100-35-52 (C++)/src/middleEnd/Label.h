#pragma once
#include "utils.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

// struct Label {
//     string name;
//     Label(string name = "entry");
//     void print(std::ostream& out = std::cout);
//     string getStr();
// };
// using LabelPtr = shared_ptr<Label>;
// NOLINTBEGIN

class StringManager final {
    std::unordered_map<std::string, size_t> mUniqueStrings;

public:
    StringManager() = default;

    std::string getOrCreateString(const std::string& str) {
        if(str.empty())
            return str;
        string newName = str;
        // length > 1024 直接截断
        if(str.length() > 1024) {
            newName = str.substr(0, 1024);
        }

        auto [iter, inserted] = mUniqueStrings.try_emplace(newName, 0);
        if(inserted) {
            return newName;
        }

        std::string newStr;
        do {
            newStr = newName + std::to_string(++iter->second);
        } while(mUniqueStrings.find(newStr) != mUniqueStrings.end());

        mUniqueStrings[newStr] = 0;
        return newStr;
    }

    static StringManager& instance() {
        static StringManager manager;
        return manager;
    }
};

class Label {
    std::string mName;

public:
    Label() = default;
    explicit Label(const std::string& name) : mName(StringManager::instance().getOrCreateString(name)) {}

    // 返回常量引用，防止意外修改
    [[nodiscard]] const std::string& name() const {
        return mName;
    }

    // 提供一个设置新名称的方法
    void setName(const std::string& newName) {
        mName = StringManager::instance().getOrCreateString(newName);
    }

    friend std::ostream& operator<<(std::ostream& os, const Label& label) {
        os << label.mName;
        return os;
    }
};

using LabelPtr = Label*;