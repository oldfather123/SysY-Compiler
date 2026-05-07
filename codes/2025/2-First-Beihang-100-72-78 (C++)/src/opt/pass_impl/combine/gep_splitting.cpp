#include <cstddef>
#include <memory>
#include <queue>
#include <vector>

#include "ir/mod.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

static std::queue<std::shared_ptr<ir::Getelementptr>> work_list;

static void collect_target_geps(ir::Module *module);
static void split(std::shared_ptr<ir::Getelementptr> gep);

bool GepSplitting::run(ir::Module *module) {
    logger::INFO("Running GepSplitting...");
    collect_target_geps(module);
    while (!work_list.empty()) {
        auto gep = work_list.front();
        work_list.pop();
        split(gep);
    }
    return false;
}

static void collect_target_geps(ir::Module *module) {
    module->for_each_func([](auto &function) {
        function->for_each_block([](auto &block) {
            block->for_each_instruction([](auto &inst) {
                if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
                    auto indexes = gep->get_indexes();

                    // 拆分条件：
                    // 1. 索引数量 > 2，或者
                    // 2. 索引数量 == 2 且基址是全局变量且第二个索引是变量
                    if (indexes.size() > 2) {
                        work_list.push(gep);
                    } else if (indexes.size() == 2) {
                        // 检查第二个索引是否是变量（非常量）
                        if (!std::dynamic_pointer_cast<ir::Constant>(indexes[1])) {
                            work_list.push(gep);
                        }
                    }
                }
            });
        });
    });
}

static void split(std::shared_ptr<ir::Getelementptr> gep) {
    auto indexes = gep->get_indexes();
    auto ptr = gep->base_ptr();
    auto parent_block = gep->get_parent_block().lock();
    auto pos = gep;

    if (indexes.size() == 2) {
        // 对于两维数组访问 gep %ptr, 0, %var，拆分为：
        // %base = gep %ptr, 0, 0  (基址)
        // %result = gep %base, %var      (偏移)

        // 创建基址GEP（指向数组首元素）
        auto base_gep =
            ir::Getelementptr::create(parent_block,
                                      ptr,
                                      {indexes[0], std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0)},
                                      gen_local_var_name());
        parent_block->add_instruction(std::next(pos->node), base_gep);

        // 创建偏移GEP（只有一个变量索引）
        auto offset_gep = ir::Getelementptr::create(parent_block,
                                                    base_gep,
                                                    {indexes[1]},  // 只用变量索引
                                                    gen_local_var_name());
        parent_block->add_instruction(std::next(base_gep->node), offset_gep);

        util::substitute(gep, offset_gep);
    } else {
        // 多索引处理逻辑
        std::vector<std::shared_ptr<ir::Value>> pre_indexes;
        pre_indexes.push_back(indexes[0]);
        pre_indexes.push_back(indexes[1]);
        auto pre_offset = ir::Getelementptr::create(parent_block, ptr, pre_indexes, gen_local_var_name());
        parent_block->add_instruction(std::next(pos->node), pre_offset);

        pos = pre_offset;
        ptr = pre_offset;

        for (size_t i = 2; i < indexes.size(); ++i) {
            auto next_gep =
                ir::Getelementptr::create(parent_block,
                                          ptr,
                                          {std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0), indexes[i]},
                                          gen_local_var_name());
            parent_block->add_instruction(std::next(pos->node), next_gep);
            pos = next_gep;
            ptr = next_gep;
        }
        util::substitute(gep, pos);
    }
}

}  // namespace opt::pass
