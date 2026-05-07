#include "backend/opt/cfg_opt/gen_cfg.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <iostream>

namespace backend::opt::cfg_opt {

// 辅助函数：根据分支指令类型计算"是"分支的概率
static double get_branch_yes_prob(backend::riscv::RVInstruction *inst) {
    using namespace backend::riscv;

    // 根据分支指令类型返回不同的概率
    switch (inst->get_type()) {
        case RVInstType::BEQ:  // 等于
            return 0.2;
        case RVInstType::BNE:  // 不等于
            return 0.8;
        case RVInstType::BLT:  // 小于
            return 0.5;
        case RVInstType::BGE:  // 大于等于
            return 0.5;
        case RVInstType::BGT:  // 大于
            return 0.5;
        case RVInstType::BLE:  // 小于等于
            return 0.5;
        default:
            return 0.5;
    }
}

// 辅助函数：根据标签名称找到对应的基本块
static backend::riscv::RVBasicBlock *find_block_by_label(backend::riscv::RVFunction *func,
                                                         const std::string &label_name) {
    for (auto *block : func->get_blocks()) {
        if (block->get_name() == label_name) {
            return block;
        }
    }
    return nullptr;
}

std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> GenCFG::calc_cfg(backend::riscv::RVFunction *func) {
    std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> result;

    // 为每个基本块创建CFG节点
    for (auto *block : func->get_blocks()) {
        result[block] = new BackCFGNode();
    }

    // 遍历每个基本块，建立控制流图
    for (auto *block : func->get_blocks()) {
        double prob = 1.0;  // 默认概率

        // 遍历基本块中的每条指令
        for (auto *inst : block->get_insts()) {
            // 如果是分支指令
            if (inst->is_branch_ins()) {
                // 获取分支指令的目标标签（通常是第三个操作数）
                const auto &operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);
                auto *target_block = find_block_by_label(func, label->name());
                double yes_prob = get_branch_yes_prob(inst);
                BackCFGNode::connect(result, block, target_block, yes_prob);
                prob *= (1.0 - yes_prob);  // 更新剩余概率
            }
        }

        // 检查最后一条指令是否为跳转指令
        if (!block->get_insts().empty()) {
            auto *last_inst = block->get_insts().back();

            // 如果是返回指令，不需要添加后继
            if (last_inst->get_type() == backend::riscv::RVInstType::JR ||
                last_inst->get_type() == backend::riscv::RVInstType::RET) {
                continue;
            }

            assert(last_inst->get_type() == backend::riscv::RVInstType::J);

            // 获取跳转指令的目标标签
            const auto &operands = last_inst->get_operands();
            auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
            auto *target_block = find_block_by_label(func, label->name());
            BackCFGNode::connect(result, block, target_block, prob);
        }
    }

    return result;
}

void GenCFG::debug(const std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &cfg) {
    for (const auto &[block, node] : cfg) {
        std::cout << block->get_name() << std::endl;
        for (const auto &[succ_block, prob] : node->suc) {
            std::cout << succ_block->get_name() << " -- " << prob << std::endl;
        }
    }
}

std::vector<double> GenCFG::solve(int n, double *mat) {
    std::vector<int> p(n);
    for (int i = 0; i < n; ++i) {
        p[i] = i;
    }

    // 高斯消元
    for (int i = 0; i < n; ++i) {
        int x = INT_MAX;
        double maxv = EPS;

        // 寻找主元
        for (int j = i; j < n; ++j) {
            double pivot = std::abs(mat[i * n + j]);
            if (pivot > maxv) {
                maxv = pivot;
                x = j;
            }
        }

        if (maxv == EPS) {
            return std::vector<double>();  // 无解
        }

        // 交换行
        if (i != x) {
            std::swap(p[i], p[x]);
            for (int j = 0; j < n; ++j) {
                std::swap(mat[i * n + j], mat[x * n + j]);
            }
        }

        // 消元
        double pivot = mat[i * n + i];
        for (int j = i + 1; j < n; ++j) {
            mat[j * n + i] /= pivot;
            double scale = mat[j * n + i];
            for (int k = i + 1; k < n; ++k) {
                mat[j * n + k] -= scale * mat[i * n + k];
            }
        }
    }

    // 前向代入
    std::vector<double> c(n);
    for (int i = 0; i < n; ++i) {
        double sum = (p[i] == 0) ? 1.0 : 0.0;
        for (int j = 0; j < i; ++j) {
            sum -= mat[i * n + j] * c[j];
        }
        c[i] = sum;
    }

    // 后向代入
    std::vector<double> d(n);
    for (int i = n - 1; i >= 0; --i) {
        double sum = c[i];
        for (int j = i + 1; j < n; ++j) {
            sum -= mat[i * n + j] * d[j];
        }
        d[i] = std::max(1e-4, sum / mat[i * n + i]);
    }

    return d;
}

std::unordered_map<backend::riscv::RVBasicBlock *, double> GenCFG::call_freq(
    backend::riscv::RVFunction *func, std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &cfg) {
    std::unordered_map<backend::riscv::RVBasicBlock *, double> res;

    int n = func->get_blocks().size();
    int allocate_id = 0;
    std::unordered_map<backend::riscv::RVBasicBlock *, int> node_map;

    // 为每个基本块分配ID
    for (auto *block : func->get_blocks()) {
        node_map[block] = allocate_id++;
    }

    // 分配矩阵内存
    std::vector<double> a(n * n);

    // 初始化矩阵
    for (int i = 0; i < n; ++i) {
        a[i * n + i] = 1.0;
    }

    // 填充转移概率
    for (auto *block : func->get_blocks()) {
        int u = node_map[block];
        for (const auto &[succ_block, prob] : cfg[block]->suc) {
            a[node_map[succ_block] * n + u] -= prob;
        }
    }

    // 求解线性方程组
    std::vector<double> d = solve(n, a.data());

    if (d.empty()) {
        // 如果无解，为每个块分配相等的概率
        for (auto *block : func->get_blocks()) {
            res[block] = 1.0;
        }
        return res;
    }

    // 将解映射回基本块
    for (auto *block : func->get_blocks()) {
        res[block] = d[node_map[block]];
    }

    return res;
}

}  // namespace backend::opt::cfg_opt
