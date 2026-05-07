/*
    SPDX-License-Identifier: Apache-2.0
    Copyright 2023 CMMC Authors
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/





#pragma once
#include "Block.h"
#include "CFGAnalysis.h"
#include <Function.h>
#include <array>
#include <cstdint>
#include <limits>

// cmmc version

struct DomTreeNode final {
    static constexpr uint32_t maxDepth = 20;
    using NodeIndex = uint32_t;
    static constexpr auto invalidNode = std::numeric_limits<NodeIndex>::max();

    std::array<NodeIndex, maxDepth + 1> ancestor;
    BasicBlockPtr block;

    NodeIndex depth;
    // 子树大小
    NodeIndex size;  // inclusive

    [[nodiscard]] NodeIndex parent() const noexcept {
        return ancestor[0];
    }
};
using DominanceFrontier = std::unordered_map<BasicBlockPtr, std::unordered_set<BasicBlockPtr>>;


class DominateAnalysisResult final {
    std::unordered_map<BasicBlockPtr, DomTreeNode::NodeIndex> mDomTreeInvMap;
    std::vector<DomTreeNode> mDomTree;

    std::vector<BasicBlockPtr> mOrder, mReversedOrder;

public:
    explicit DominateAnalysisResult(std::unordered_map<BasicBlockPtr, DomTreeNode::NodeIndex> invMap,
                                    std::vector<DomTreeNode> domTree);
    DomTreeNode::NodeIndex getIndex(BasicBlockPtr block) const;
    bool reachable(BasicBlockPtr block) const;
    // 直接支配者（最近的严格支配 (严格支配 = 支配且不等于)）)
    BasicBlockPtr parent(BasicBlockPtr node) const;
    std::vector<BasicBlockPtr> allDoms(BasicBlockPtr node) const;
    DomTreeNode::NodeIndex subTreeSize(BasicBlockPtr node) const;
    const std::vector<BasicBlockPtr>& blocks() const {
        return mOrder;
    }
    const std::vector<BasicBlockPtr>& reversedBlocks() const {
        return mReversedOrder;
    }
    void dump(std::ostream& out = std::cout) const;

    BasicBlockPtr lca(BasicBlockPtr a, BasicBlockPtr b) const;
    // a dominates b?  a 支配 b
    bool dominate(BasicBlockPtr a, BasicBlockPtr b) const;
};

DominateAnalysisResult runDominateTreeAnalysis(FunctionPtr func, CFGAnalysisResult cfgResult);
DominanceFrontier computeDominanceFrontier(const DominateAnalysisResult& domResult, const CFGAnalysisResult& cfgResult);