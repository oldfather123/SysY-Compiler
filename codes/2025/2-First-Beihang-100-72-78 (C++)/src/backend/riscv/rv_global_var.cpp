#include "rv_global_var.hpp"

#include <cstring>
#include <vector>

namespace {
// Helper to collect constant values
void collect_constant_values(ir::Constant *constant, std::vector<std::string> &values) {
    if (auto fp_val = dynamic_cast<ir::ConstantFloat *>(constant)) {
        float f_val = fp_val->get_val();
        int i_val;
        std::memcpy(&i_val, &f_val, sizeof(int));
        values.push_back(std::to_string(i_val));
    } else if (auto int_val = dynamic_cast<ir::ConstantInt *>(constant)) {
        values.push_back(std::to_string(int_val->get_val()));
    } else if (auto arr_val = dynamic_cast<ir::ConstantArray *>(constant)) {
        // 获取数组类型信息
        auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(arr_val->get_type());
        int size = arr_type ? arr_type->get_size() : (int)arr_val->get_vals().size();
        auto elem_type = arr_type ? arr_type->get_element_type() : nullptr;
        for (int i = 0; i < size; ++i) {
            if ((size_t)i < arr_val->get_vals().size()) {
                collect_constant_values(arr_val->get_vals()[i].get(), values);
            } else if (elem_type) {
                // 补零
                if (elem_type->is_integer_ty()) {
                    values.push_back("0");
                } else if (elem_type->is_float_ty()) {
                    values.push_back("0");
                } else if (elem_type->is_array_ty()) {
                    // 递归补零
                    auto zero = std::make_shared<ir::ZeroInitializer>(elem_type);
                    collect_constant_values(zero.get(), values);
                }
            }
        }
    } else if (auto zero_init = dynamic_cast<ir::ZeroInitializer *>(constant)) {
        // 获取类型
        auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(zero_init->get_type());
        if (arr_type) {
            int size = arr_type->get_size();
            auto elem_type = arr_type->get_element_type();
            for (int i = 0; i < size; ++i) {
                auto zero = std::make_shared<ir::ZeroInitializer>(elem_type);
                collect_constant_values(zero.get(), values);
            }
        } else if (zero_init->get_type()->is_integer_ty() || zero_init->get_type()->is_float_ty()) {
            values.push_back("0");
        }
    }
}

// Helper to print constants in a compact format
void print_constant(ir::Constant *constant, std::string &out) {
    std::vector<std::string> values;
    collect_constant_values(constant, values);

    if (!values.empty()) {
        out += "\t.word\t";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) out += ", ";
            out += values[i];
        }
        out += "\n";
    }
}
}  // anonymous namespace

namespace backend::riscv {

RVGlobalVar::RVGlobalVar(ir::GlobalVariable *gv, bool is_zero_init) : gv(gv), is_zero_init(is_zero_init) {}

RVGlobalVar::~RVGlobalVar() {
    // 假设gv的生命周期由外部管理，这里不delete
    // 如果需要delete，请将此处改为 delete gv;
}

std::string RVGlobalVar::to_string() const {
    std::string ret;
    std::string var_name = gv->get_name();

    // 移除LLVM IR中的@符号，转换为有效的RISC-V符号名
    if (!var_name.empty() && var_name[0] == '@') {
        var_name = var_name.substr(1);
    }

    ret += var_name + ":\n";

    auto init = gv->get_init_value();
    if (is_zero_init) {
        // 全零初始化
        auto ptr_type = dynamic_cast<ir::PointerType *>(gv->get_type().get());
        int size_in_bits = ptr_type->get_reference_type()->bits_num();
        int size_in_bytes = (size_in_bits + 7) / 8;
        ret += "\t.zero\t" + std::to_string(size_in_bytes) + "\n";
    } else if (init) {
        // 非零初始化，后续会修正print_constant递归
        print_constant(init.get(), ret);
    }
    return ret;
}

bool RVGlobalVar::is_zero_initialized() const { return is_zero_init; }

}  // namespace backend::riscv
