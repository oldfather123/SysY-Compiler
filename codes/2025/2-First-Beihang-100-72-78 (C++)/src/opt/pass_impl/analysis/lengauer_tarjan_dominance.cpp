#include <optional>

#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

class LengauerTarjan {
public:
    /**
     * @brief Default constructor.
     */
    LengauerTarjan() = default;

    /**
     * @brief Initializes or resets the analyzer's state.
     * @param paramN The number of nodes in the graph. Nodes are numbered from 1 to N.
     * @param paramM An estimate of the maximum number of edges in the graph.
     */
    void init(int paramN, int paramM) {
        n = paramN;
        m = paramM;

        // Initialize vectors using assign or resize.
        ans.assign(n + 1, 0);
        fst.assign(n + 1, std::vector<int>(3, 0));

        // Allocate enough space for edges.
        int edge_size = m + m + n + 1;
        nxt.assign(edge_size, 0);
        to.assign(edge_size, 0);

        dfn.assign(n + 1, 0);
        ord.assign(n + 1, 0);
        fth.assign(n + 1, 0);
        idom.assign(n + 1, 0);
        semi.assign(n + 1, 0);
        uni.assign(n + 1, 0);
        mn.assign(n + 1, 0);

        cnt = 0;
        tot = 0;
    }

    /**
     * @brief Adds an edge to the graph.
     * @param u The starting node of the edge.
     * @param v The destination node of the edge.
     */
    void add_edge(int u, int v) {
        // Forward edge, used for the Tarjan DFS.
        add(u, v, 0);
        // Reverse edge, used for finding semi-dominators.
        add(v, u, 1);
    }

    /**
     * @brief Runs the Lengauer-Tarjan algorithm for analysis.
     * @param root The entry node of the graph (usually 1).
     */
    void run(int root) { lengauer_tarjan(root); }

    // --- Getters ---

    int get_dfs_count() const { return cnt; }

    // Returns a const reference to avoid unnecessary copying and to protect the data.
    const std::vector<int> &get_dfs_order() const { return ord; }

    const std::vector<int> &get_idom_result() const { return idom; }

private:
    // Member variables
    int n = 0, m = 0;
    int tot = 0;  // Edge counter
    int cnt = 0;  // DFS counter

    std::vector<int> ans;
    std::vector<std::vector<int>> fst;  // Adjacency list head
    std::vector<int> nxt;               // Adjacency list next edge
    std::vector<int> to;                // Adjacency list destination node

    // Variables required for the Lengauer-Tarjan algorithm
    std::vector<int> dfn;   // DFS number
    std::vector<int> ord;   // Node corresponding to a DFS number
    std::vector<int> fth;   // Parent in the DFS tree (father)
    std::vector<int> idom;  // Immediate dominator
    std::vector<int> semi;  // Semi-dominator
    std::vector<int> uni;   // Union-find data structure
    std::vector<int> mn;    // Node with the minimum semi-dominator on a path in the union-find structure

    /**
     * @brief Internal function to add an edge to the adjacency list.
     * @param id The type of edge (0: forward, 1: reverse, 2: bucket)
     */
    void add(int u, int v, int id) {
        nxt[++tot] = fst[u][id];
        to[tot] = v;
        fst[u][id] = tot;
    }

    /**
     * @brief Step 1: Perform a Depth-First Search to compute the DFS tree and related information.
     */
    void tarjan_dfs(int u) {
        ord[dfn[u] = ++cnt] = u;
        // Traverse the forward graph
        for (int i = fst[u][0]; i != 0; i = nxt[i]) {
            int v = to[i];
            if (dfn[v] == 0) {
                fth[v] = u;
                tarjan_dfs(v);
            }
        }
    }

    /**
     * @brief Union-find query operation with path compression and minimum semi-dominator updates.
     */
    int uni_query(int u) {
        if (u == uni[u]) {
            return u;
        }
        int root = uni_query(uni[u]);
        // While performing path compression, update the mn value of the nodes on the path.
        if (dfn[semi[mn[u]]] > dfn[semi[mn[uni[u]]]]) {
            mn[u] = mn[uni[u]];
        }
        return uni[u] = root;
    }

    /**
     * @brief The main process of the Lengauer-Tarjan algorithm.
     */
    void lengauer_tarjan(int s) {
        // Step 1: DFS
        tarjan_dfs(s);

        // Initialize the union-find and semi arrays
        // std::iota(semi.begin(), semi.end(), 0); // C++20 is cleaner
        for (int i = 1; i <= n; ++i) {
            semi[i] = uni[i] = mn[i] = i;
        }

        // Steps 2 & 3: Iterate through DFS nodes in reverse order to compute semi-dominators and immediate dominators.
        for (int i = cnt; i >= 2; --i) {
            int u = ord[i];

            // Compute semi[u]
            for (int j = fst[u][1]; j != 0; j = nxt[j]) {
                int v = to[j];
                if (dfn[v] == 0) continue;
                uni_query(v);
                if (dfn[semi[u]] > dfn[semi[mn[v]]]) {
                    semi[u] = semi[mn[v]];
                }
            }

            // Add u to the "bucket" of semi[u]
            add(semi[u], u, 2);
            uni[u] = fth[u];  // Union u into its parent's set
            int parent = fth[u];

            // Process the parent's bucket
            for (int j = fst[parent][2]; j != 0; j = nxt[j]) {
                int v = to[j];
                uni_query(v);
                idom[v] = (parent == semi[mn[v]]) ? parent : mn[v];
            }
            fst[parent][2] = 0;  // Clear the bucket
        }

        // Step 4: Explicitly compute the final immediate dominators
        for (int i = 2; i <= cnt; ++i) {
            int u = ord[i];
            if (idom[u] != semi[u]) {
                idom[u] = idom[idom[u]];
            }
        }
    }
};

std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::weak_ptr<ir::BasicBlock>>> dominate_map;  // a -> b
std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::weak_ptr<ir::BasicBlock>>>
    dominator_map;  // b <- a
std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::weak_ptr<ir::BasicBlock>>> immediate_dominate_map;
void init_function();
void calculate_immediate_dominance(std::shared_ptr<ir::Function> func);
void extract_dominance(std::shared_ptr<ir::BasicBlock> root, std::shared_ptr<ir::Function> func);
void write_back_to_basic_blocks(std::shared_ptr<ir::Function> func);
static void build_dominance_depth(std::shared_ptr<ir::BasicBlock> root, int current_depth);
static void build_dominance_frontier(std::shared_ptr<ir::Function> func);

bool LengauerTarjanDominanceAnalyzation::run(ir::Module *module) {
    logger::INFO("Running Lengauer-Tarjan Dominance Analyzation...");
    module->for_each_func([&](const auto &func) {
        init_function();
        calculate_immediate_dominance(func);
        extract_dominance(func->get_basic_blocks_ref().front(), func);
        write_back_to_basic_blocks(func);
        build_dominance_depth(func->get_basic_blocks_ref().front(), 0);
        build_dominance_frontier(func);
    });

    return false;
}

void init_function() {
    dominate_map.clear();
    dominator_map.clear();
    immediate_dominate_map.clear();
}

void calculate_immediate_dominance(std::shared_ptr<ir::Function> func) {
    if (func->get_basic_blocks_ref().size() == 1) {
        auto single_block = func->get_basic_blocks_ref().front();
        dominate_map[single_block] = {single_block};
        dominator_map[single_block] = {single_block};
        return;
    }

    std::unordered_map<std::shared_ptr<ir::BasicBlock>, int> basic_block_to_index;
    std::unordered_map<int, std::shared_ptr<ir::BasicBlock>> index_to_basic_block;

    int index = 1;
    for (auto &basic_block : func->get_basic_blocks_ref()) {
        basic_block_to_index[basic_block] = index;
        index_to_basic_block[index] = basic_block;
        index++;
    }

    int estimate = 0;
    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        estimate += static_cast<int>(basic_block->opt_info.successors.size());
    }

    // initialize the Lengauer-Tarjan algorithm
    auto lt = LengauerTarjan();
    lt.init(index, estimate);

    // add edges for the graph
    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        for (auto &weak_successor : basic_block->opt_info.successors) {
            lt.add_edge(basic_block_to_index[basic_block], basic_block_to_index[weak_successor.lock()]);
        }
    }

    // run the Lengauer-Tarjan algorithm
    lt.run(basic_block_to_index[func->get_basic_blocks_ref().front()]);
    int cnt = lt.get_dfs_count();
    auto order = lt.get_dfs_order();
    auto immediate_dominators = lt.get_idom_result();
    for (auto i = cnt; i >= 2; i--) {
        immediate_dominate_map[index_to_basic_block[immediate_dominators[order[i]]]].push_back(
            index_to_basic_block[order[i]]);
    }
}

void extract_dominance(std::shared_ptr<ir::BasicBlock> root, std::shared_ptr<ir::Function> func) {
    if (immediate_dominate_map[root].empty()) {
        dominate_map[root].push_back(root);
        dominator_map[root].push_back(root);
    } else {
        for (auto &immediate_dominated : immediate_dominate_map[root]) {
            extract_dominance(immediate_dominated.lock(), func);
            dominate_map[root].insert(dominate_map[root].end(),
                                      dominate_map[immediate_dominated.lock()].begin(),
                                      dominate_map[immediate_dominated.lock()].end());
        }
        dominate_map[root].push_back(root);
        for (auto &dominated : dominate_map[root]) {
            dominator_map[dominated.lock()].push_back(root);
        }
    }
}

void write_back_to_basic_blocks(std::shared_ptr<ir::Function> func) {
    for (auto &basic_block : func->get_basic_blocks_ref()) {
        basic_block->opt_info.dominated = dominate_map[basic_block];
        basic_block->opt_info.dominator = dominator_map[basic_block];
        basic_block->opt_info.immediate_dominated = immediate_dominate_map[basic_block];
        for (auto &weak_dominated : basic_block->opt_info.immediate_dominated) {
            weak_dominated.lock()->opt_info.immediate_dominator = basic_block;
        }
    }
}

static void build_dominance_frontier(std::shared_ptr<ir::Function> func) {
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::shared_ptr<ir::BasicBlock>>>
        dominance_frontier_map;

    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        for (const auto &weak_successor : basic_block->opt_info.successors) {
            auto successor = weak_successor.lock();
            auto predecessor = basic_block;

            // predecessor dominates the predecessor of successor and does not strictly dominate successor
            while (!predecessor->opt_info.dominates(successor) || predecessor == successor) {
                dominance_frontier_map[predecessor].push_back(successor);
                predecessor = predecessor->opt_info.immediate_dominator.lock();
            }
        }
    }

    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        basic_block->opt_info.dominance_frontier = to_weak_vector(dominance_frontier_map[basic_block]);
    }
}

static void build_dominance_depth(std::shared_ptr<ir::BasicBlock> root, int current_depth) {
    root->opt_info.dominance_depth = current_depth;

    for (const std::weak_ptr<ir::BasicBlock> &immediate_dominated : root->opt_info.immediate_dominated) {
        build_dominance_depth(immediate_dominated.lock(), current_depth + 1);
    }
}

}  // namespace opt::pass
