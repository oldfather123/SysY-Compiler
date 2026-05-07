#include "backend/opt/gp_pooling/gp_pool.hpp"

namespace backend::opt::gp_pooling {

GPpool::GPpool() : size(0), name("pool"), type(GlobType::FLOAT) {}

void GPpool::add(backend::riscv::RVGlobalVar *global_var) {
    // TODO: 添加全局变量
}

void GPpool::init() {
    // TODO: 初始化
}

int GPpool::query_offset(backend::riscv::RVGlobalVar *global_var) {
    // TODO: 查询偏移
    return -1;
}

std::string GPpool::to_string() const {
    // TODO: 转换为字符串
    return "";
}

}  // namespace backend::opt::gp_pooling
