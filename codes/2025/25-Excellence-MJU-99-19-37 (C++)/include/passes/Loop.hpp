#pragma once

#include "BasicBlock.hpp"
#include <set>
#include <vector>
#include <memory>

class Loop {
public:
    explicit Loop(BasicBlock *header) : header_(header) {}

    // 获取循环头
    BasicBlock *get_header() const {
        return header_;
    }

    // 设置 preheader
    void set_preheader(BasicBlock *bb) {
        preheader_ = bb;
    }

    // 获取 preheader（可为空）
    BasicBlock *get_preheader() const {
        return preheader_;
    }

    // 添加一个基本块到循环体中
    void add_block(BasicBlock *bb) {
        blocks_.insert(bb);
    }

    // 获取循环体所有基本块
    const std::set<BasicBlock *> &get_blocks() const {
        return blocks_;
    }

    // 添加一个子循环
    void add_sub_loop(std::shared_ptr<Loop> loop) {
        sub_loops_.push_back(loop);
    }

    // 获取所有子循环
    const std::vector<std::shared_ptr<Loop>> &get_sub_loops() const {
        return sub_loops_;
    }

    // 设置父循环
    void set_parent(std::shared_ptr<Loop> parent) {
        parent_ = parent;
    }

    // 获取父循环
    std::shared_ptr<Loop> get_parent() const {
        return parent_;
    }

    // 添加一个 latch（回边块）
    void add_latch(BasicBlock *bb) {
        latches_.insert(bb);
    }

    // 获取所有 latch 块
    const std::set<BasicBlock *> &get_latches() const {
        return latches_;
    }

private:
    BasicBlock *header_;                         // 循环头
    BasicBlock *preheader_ = nullptr;            // preheader（可能为空）
    std::set<BasicBlock *> blocks_;              // 循环体中的所有块
    std::set<BasicBlock *> latches_;             // 回边块（从其跳转回 header）
    std::vector<std::shared_ptr<Loop>> sub_loops_; // 子循环列表
    std::shared_ptr<Loop> parent_;               // 父循环
};
