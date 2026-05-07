#include "Operand.hpp"
#include "MachineBasicBlock.hpp"
#include "MachineFunction.hpp"
#include "exitcode.hpp"
#include <memory>
#include <string>
#include <unordered_map>

std::unordered_map<std::shared_ptr<VirtualRegister>,
                   std::shared_ptr<PhysicalRegister>>
    Register::color = {};
std::unordered_set<std::shared_ptr<VirtualRegister>> Register::temp_regs = {};

Label::Label(std::weak_ptr<MachineBasicBlock> block) {
    name = block.lock()->get_name();
    this->block = block;
}

Label::Label(std::weak_ptr<MachineFunction> func) {
    name = func.lock()->get_name();
    this->func = func;
}

std::shared_ptr<Immediate> Immediate::create(int value) {
    static std::unordered_map<int, std::shared_ptr<Immediate>> pool;
    auto it = pool.find(value);
    if (it != pool.end()) {
        return it->second;
    } else {
        auto obj = std::make_shared<Immediate>(value);
        pool[value] = obj;
        return obj;
    }
}

std::string VirtualRegister::get_name() const {
    std::string ret;
    switch (type) {
    case RegisterType::General:
        ret += "v_r";
        break;
    case RegisterType::Float:
        ret += "v_f";
        break;
    }
    return ret + std::to_string(id);
}

std::vector<std::shared_ptr<PhysicalRegister>>
PhysicalRegister::callee_saved_regs() {
    std::vector<std::shared_ptr<PhysicalRegister>> regs;
    regs.push_back(ra());
    for (int i = 1; i <= 11; i++) {
        regs.push_back(s(i));
    }

    for (int i = 0; i <= 11; i++) {
        regs.push_back(fs(i));
    }
    return regs;
}

std::vector<std::shared_ptr<PhysicalRegister>>
PhysicalRegister::caller_saved_regs() {
    std::vector<std::shared_ptr<PhysicalRegister>> regs;
    for (int i = 0; i <= 7; i++) {
        regs.push_back(a(i));
    }
    for (int i = 0; i <= 6; i++) {
        regs.push_back(t(i));
    }

    for (int i = 0; i <= 7; i++) {
        regs.push_back(fa(i));
    }
    for (int i = 0; i <= 11; i++) {
        regs.push_back(ft(i));
    }

    return regs;
}

std::string PhysicalRegister::get_name() const {
    if (type == RegisterType::General) {
        if (id == 0)
            return "zero";
        if (id == 1)
            return "ra";
        if (id == 2)
            return "sp";
        if (id >= 5 && id <= 7)
            return "t" + std::to_string(id - 5);
        if (id == 8)
            return "fp";
        if (id == 9)
            return "s1";
        if (id >= 10 && id <= 17)
            return "a" + std::to_string(id - 10);
        if (id >= 18 && id <= 27)
            return "s" + std::to_string(id - 16);
        if (id >= 28 && id <= 32)
            return "t" + std::to_string(id - 25);
        ASSERT(false);
    }

    if (type == RegisterType::Float) {
        if (id <= 7)
            return "ft" + std::to_string(id);
        if (id <= 9)
            return "fs" + std::to_string(id - 8);
        if (id <= 17)
            return "fa" + std::to_string(id - 10);
        if (id <= 27)
            return "fs" + std::to_string(id - 16);
        if (id <= 31)
            return "ft" + std::to_string(id - 20);
        ASSERT(false);
    }

    ASSERT(false);
}