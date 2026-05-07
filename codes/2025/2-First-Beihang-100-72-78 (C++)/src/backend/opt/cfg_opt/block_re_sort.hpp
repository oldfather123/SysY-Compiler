#pragma once

#include <functional>
#include <vector>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::cfg_opt {

class BlockReSort {
public:
    static void block_sort(backend::riscv::RVModule *module);

private:
    struct BranchEdge {
        int source;
        int target;
        double prob;

        BranchEdge(int source, int target, double prob) : source(source), target(target), prob(prob) {}
    };

    static std::vector<int> solve_pettis_hansen(const std::vector<int> &weights,
                                                const std::vector<double> &freq,
                                                const std::vector<BranchEdge> &edges);
    static void optimize_block_layout(backend::riscv::RVFunction *func);
};

}  // namespace backend::opt::cfg_opt
