#include "dominator_tree.h"
#include "../../include/ir.h"

int inv_dfs_num = 0;
std::map<CFG *, DominatorTree *> DomInfo;

void DomAnalysis::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        DomInfo[cfg] = new DominatorTree(cfg);
        DomInfo[cfg]->BuildDominatorTree(false);
        cfg->DomTree = DomInfo[cfg];
    }
}

void DomAnalysis::invExecute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        DomInfo[cfg] = new DominatorTree(cfg);
        DomInfo[cfg]->BuildDominatorTree(true);
        cfg->PostDomTree = DomInfo[cfg];
    }
}

int DominatorTree::find(std::map<int,int>&mn_map, std::map<int,int> &fa_map, int id){
    if(fa_map[id]==id)return id;
    int temp = fa_map[id];
    fa_map[id] = find(mn_map, fa_map, fa_map[id]); // 路径压缩

    int fa_sdom = sdom_map[mn_map[temp]];
    int id_sdom = sdom_map[mn_map[id]];
    // 检查基本块是否存在
    if (C->block_map->find(fa_sdom) != C->block_map->end() && 
        C->block_map->find(id_sdom) != C->block_map->end() &&
        (*(C->block_map))[fa_sdom]->dfs_id < (*(C->block_map))[id_sdom]->dfs_id){
        mn_map[id] = mn_map[temp];
    }
    return fa_map[id];
}

int DominatorTree::invfind(std::map<int,int>&mn_map, std::map<int,int> &fa_map, int id){
    if(fa_map[id]==id)return id;
    int temp = fa_map[id];
    fa_map[id] = invfind(mn_map, fa_map, fa_map[id]); // 路径压缩

    int fa_sdom = sdom_map[mn_map[temp]];
    int id_sdom = sdom_map[mn_map[id]];
    // 检查基本块是否存在
    if (C->block_map->find(fa_sdom) != C->block_map->end() && 
        C->block_map->find(id_sdom) != C->block_map->end() &&
        dfs[(*(C->block_map))[fa_sdom]->block_id] < dfs[(*(C->block_map))[id_sdom]->block_id]){
        mn_map[id] = mn_map[temp];
    }
    return fa_map[id];
}

void DominatorTree::SearchInvB(int bbid){
    if(dfs[bbid]!=-1) return;
    dfs[bbid] = ++inv_dfs_num;
    //std::cout<<bbid<<" "<<inv_dfs_num<<std::endl;

    for(auto block: C->invG[bbid]){
        SearchInvB(block->block_id);
    }
}

void DominatorTree::BuildDominatorTree(bool reverse) {

	// C->display(reverse);
	
    // 清空所有内容
    dom_tree.clear();
    sdom_map.clear();
    dfs_map.clear();
    dfs.clear();
    DF_map.clear();

    // 初始化 dom_tree
    dom_tree.resize(C->max_label + 1);

    if(!reverse){
        std::map<int, int> fa_map{};  // 这是用于记录路径压缩的祖宗节点的map
        std::map<int, int> mn_map{};  // 这是用于记录一个block顺着逆向图目前可以找到的最小sdom的block_id

        // 遍历原图，获取dfs序号-->block_id的对应map
        // 初始化辅助map
        for(auto iter = C->block_map->begin(); iter != C->block_map->end(); iter++){
            int block_id = iter->first;
            LLVMBlock block = iter->second;

            dfs_map[block->dfs_id] = block_id;
            sdom_map[block_id] = block_id;
            fa_map[block_id] = block_id;
            mn_map[block_id] = block_id;
        }
        
        // 按照dfs的逆序遍历和invG遍历以获取idom
        std::map<int, int>::reverse_iterator iter;
        for(iter=dfs_map.rbegin(); iter!=dfs_map.rend(); iter++){
            int dfn = iter->first;
            int block_id = iter->second;
            if(block_id==0) continue;

            std::set<LLVMBlock> block_set = C->invG[block_id];
            int res = INT32_MAX;

            for(auto &block: block_set){
                if(dfn > block->dfs_id){
                    res = std::min(res, block->dfs_id);
                }
                else{
                    find(mn_map, fa_map, block->block_id);
                    int sdom = sdom_map[mn_map[block->block_id]];
                    // 检查基本块是否存在
                    if (C->block_map->find(sdom) != C->block_map->end()) {
                        res = std::min(res, (*(C->block_map))[sdom]->dfs_id);
                    }
                }
            }
            sdom_map[block_id] = dfs_map[res];
            fa_map[block_id] = block_id==0?0:dfs_map[dfn-1];
        }


        // 建立DF_map
        for(int i=0; i<C->invG.size(); i++){
            if(C->invG[i].size()>=2){
                for(auto &block: C->invG[i]){
                    int runner = block->block_id;
                    while(runner!=sdom_map[i]){
                        DF_map[runner].insert(i);
                        if(runner==0)break;
                        runner = sdom_map[runner];
                    }
                }
            }
        }
    }
    else{
        std::set<int> inv_start;
        for (auto iter = C->block_map->begin(); iter != C->block_map->end(); ++iter) {
            int block_id = iter->first;
            if (C->G[block_id].size() == 0) {
                inv_start.insert(block_id);
            }
            dfs[block_id] = -1;
        }

        // 计算反图的dfs
        for(auto& start: inv_start){
            SearchInvB(start);
        }
        
        std::map<int, int> fa_map{};  // 这是用于记录路径压缩的祖宗节点的map
        std::map<int, int> mn_map{};  // 这是用于记录一个block顺着逆向图目前可以找到的最小sdom的block_id

        // 遍历原图，获取dfs序号-->block_id的对应map
        // 初始化辅助map
        for(auto iter = C->block_map->begin(); iter != C->block_map->end(); iter++){
            int block_id = iter->first;
            LLVMBlock block = iter->second;

            dfs_map[dfs[block_id]] = block_id;
            sdom_map[block_id] = block_id;
            fa_map[block_id] = block_id;
            mn_map[block_id] = block_id;
        }
        
        // 按照dfs的逆序遍历和invG遍历以获取idom（这里的idom在带权并查集其实就是sdom）
        std::map<int, int>::reverse_iterator iter;
        for(iter=dfs_map.rbegin(); iter!=dfs_map.rend(); iter++){
            int dfn = iter->first;
            int block_id = iter->second;
            std::set<LLVMBlock> block_set = C->G[block_id];
            int res = INT32_MAX;

           for(auto &block: block_set){
                if(dfn > dfs[block->block_id]){
                    res = std::min(res, dfs[block->block_id]);
                }
                else{
                    invfind(mn_map, fa_map, block->block_id);
                    int sdom = sdom_map[mn_map[block->block_id]];
                    // 检查基本块是否存在
                    if (C->block_map->find(sdom) != C->block_map->end()) {
                        res = std::min(res, dfs[(*(C->block_map))[sdom]->block_id]);
                    }
                }
            }
            sdom_map[block_id] = dfs_map[res];
            fa_map[block_id] = block_id==0?0:dfs_map[dfn-1];
        }


        // 建立DF_map
        for(int i=0; i<C->G.size(); i++){
            if(C->G[i].size()>=2){
                for(auto &block: C->G[i]){
                    int runner = block->block_id;
                    while(runner!=sdom_map[i]){
                        DF_map[runner].insert(i);
                        if(inv_start.find(runner)!=inv_start.end())
                            break;
                        runner = sdom_map[runner];
                    }
                }
            }
        }
    }

	// 构建支配树
	std::set<int> visited;
	for(auto iter = C->block_map->begin(); iter != C->block_map->end(); iter++){
		int block_id = iter->first;
		if(block_id == 0 || visited.count(block_id) > 0) continue;
		
		int sdom_id = sdom_map[block_id];
		if(sdom_id >= 0 && sdom_id < dom_tree.size()) {
			dom_tree[sdom_id].push_back(iter->second);
			visited.insert(block_id);
		}
	}

    // 输出sdom_map
	// display_sdom_map();
}


std::set<int> DominatorTree::GetDF(int id) {
    return DF_map[id];
}

void DominatorTree::display() {
    std::cout << "\n=== Dominator Tree Structure ===" << std::endl;
    for (size_t i = 0; i < dom_tree.size(); i++) {
        if (dom_tree[i].empty()) continue;
        std::cout << "Block " << i << " dominates: ";
        for (auto block : dom_tree[i]) {
            std::cout << block->block_id << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "=== End of Dominator Tree ===\n" << std::endl;
}

void DominatorTree::display_sdom_map() {
    std::cout << "\n=== Semi-Dominator Map ===" << std::endl;
    for(auto &[block_id, sdom_id] : sdom_map) {
        std::cout << "Block " << block_id << " 的半支配点是: Block " << sdom_id << std::endl;
    }
    std::cout << "=== End of Semi-Dominator Map ===\n" << std::endl;
}

bool DominatorTree::dominates(LLVMBlock a, LLVMBlock b) {
	if (a == nullptr || b == nullptr) {
		return false;
	}
	
	int current_id = b->block_id;
	while (current_id != -1) {
		if (current_id == a->block_id) return true;
		if (current_id == 0) break;  // no exit loop
		current_id = sdom_map.count(current_id) ? sdom_map[current_id] : -1;
	}
	return false;
}

// getDominators from b block to root
std::unordered_set<LLVMBlock> DominatorTree::getDominators(LLVMBlock b) {
	std::unordered_set<LLVMBlock> result;
	
	if (b == nullptr) {
		return result;
	}
	
	int current_id = b->block_id;
	auto bmap = *(C->block_map);
	while (current_id != -1) {
		if (bmap.count(current_id)) {
			result.insert(bmap[current_id]);
		}
		if (current_id == 0) break;  // no exit loop
		current_id = sdom_map.count(current_id) ? sdom_map[current_id] : -1;
	}
	
	return result;
}
