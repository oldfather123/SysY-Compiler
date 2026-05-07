package midend;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

/**
 * 图级别缓存：用于缓存正向与反向可达块
 */
public class ReachabilityCache {
    private final Map<BasicBlock, Set<BasicBlock>> forwardReachableMap = new HashMap<>();
    private final Map<BasicBlock, Set<BasicBlock>> backwardReachableMap = new HashMap<>();
    // Memo: 起点 block -> 到终点 to 的所有路径（不含 from 本身，路径仍以 to 结尾）
    private final Map<BasicBlock, Map<BasicBlock, List<List<BasicBlock>>>> pathMemo = new HashMap<>();
    private boolean hasDfsFailed = false;

    /**
     * 构建缓存：在函数级别上一次性完成
     */
    public ReachabilityCache(Function function) {
        forwardReachableMap.clear();
        backwardReachableMap.clear();
        pathMemo.clear();

        buildForwardCache(function);
        buildBackwardCache(function);
    }

    /**
     * 获取所有 from → to 的路径的中间块并集（不含from和to），每条路径不含中间的 to（但保留首尾）
     * 若返回值为NULL，说明find失败了
     */
    public Set<BasicBlock> findIntermediateBlocksPro(BasicBlock from, BasicBlock to) {
        List<List<BasicBlock>> paths = findAllPaths(from, to);
        if (paths == null) return null;

        Set<BasicBlock> res = new HashSet<>();
        for (List<BasicBlock> path : paths) {
            for (BasicBlock block : path) {
                if (block == from || block == to) continue;
                res.add(block);
            }
        }
        return res;
    }

    /**
     * 枚举所有 from → to 的路径，每条路径不含中间的 to（但保留首尾）
     * 启用剪枝与 memo 缓存，防止回边死循环与性能爆炸
     * 若返回值为NULL，说明find失败了
     */
    public List<List<BasicBlock>> findAllPaths(BasicBlock from, BasicBlock to) {
        List<List<BasicBlock>> allPaths = new ArrayList<>();
        LinkedList<BasicBlock> currentPath = new LinkedList<>();
        Set<BasicBlock> visitedInPath = new HashSet<>();

        // 限制最多返回多少条路径
        int maxPathCount = 1000; // todo 保守处理false
        hasDfsFailed = false;

        // 提前剪枝用
        Set<BasicBlock> reachableFromFrom = getForwardReachable(from);

        dfsWithOptimization(from, to, currentPath, visitedInPath, allPaths, maxPathCount, reachableFromFrom);

        if (hasDfsFailed) return null;
        else return allPaths;
    }

    /**
     * 带 memo 和剪枝的 DFS 实现
     */
    private void dfsWithOptimization(BasicBlock current, BasicBlock target,
                                     LinkedList<BasicBlock> currentPath,
                                     Set<BasicBlock> visitedInPath,
                                     List<List<BasicBlock>> allPaths,
                                     int maxPathCount,
                                     Set<BasicBlock> forwardReachableTo) {

        if (allPaths.size() >= maxPathCount) {
            hasDfsFailed = true; // dfs爆炸了，那就不算了
            return;
        }
        if (visitedInPath.contains(current)) return;

//        // 中间不能重复含 to，可以含 from
//        if (!currentPath.isEmpty()) return;

        currentPath.addLast(current);
        visitedInPath.add(current);

        if (current.equals(target)) {
            allPaths.add(new ArrayList<>(currentPath));
        } else {
            // 使用路径缓存
            List<List<BasicBlock>> memo = getMemo(current, target);
            if (memo != null) {
                for (List<BasicBlock> suffix : memo) {
                    List<BasicBlock> full = new ArrayList<>(currentPath);
                    full.addAll(suffix);
                    allPaths.add(full);
                    if (allPaths.size() >= maxPathCount) break;
                }
            } else {
                List<List<BasicBlock>> suffixPaths = new ArrayList<>();
                for (BasicBlock succ : current.getSucs()) {
                    if (!forwardReachableTo.contains(succ)) continue;
                    if(succ==current) continue;

                    int beforeSize = allPaths.size();
                    dfsWithOptimization(succ, target, currentPath, visitedInPath, allPaths, maxPathCount, forwardReachableTo);
                    if (allPaths.size() >= maxPathCount) break;

                    // 提取新增加的路径中后缀段（用于 memo）
                    for (int i = beforeSize; i < allPaths.size(); ++i) {
                        List<BasicBlock> path = allPaths.get(i);
                        int startIdx = path.indexOf(current);
                        if (startIdx != -1) {
                            List<BasicBlock> suffix = path.subList(startIdx + 1, path.size());
                            suffixPaths.add(new ArrayList<>(suffix));
                        }
                    }
                }

                // 写入缓存
                putMemo(current, target, suffixPaths);
            }
        }

        currentPath.removeLast();
        visitedInPath.remove(current);
    }

    /**
     * 取出 current → to 的路径缓存
     */
    private List<List<BasicBlock>> getMemo(BasicBlock current, BasicBlock target) {
        Map<BasicBlock, List<List<BasicBlock>>> inner = pathMemo.get(current);
        return inner != null ? inner.get(target) : null;
    }

    /**
     * 写入 current → to 的路径缓存
     */
    private void putMemo(BasicBlock current, BasicBlock target, List<List<BasicBlock>> paths) {
        if (!pathMemo.containsKey(current)) {
            pathMemo.put(current, new HashMap<>());
        }
        pathMemo.get(current).put(target, paths);
    }

    /**
     * 获取从某 BasicBlock 正向可达的所有块
     */
    public Set<BasicBlock> getForwardReachable(BasicBlock start) {
        return forwardReachableMap.getOrDefault(start, Collections.emptySet());
    }

    /**
     * 获取某 BasicBlock 能反向到达的所有块
     */
    public Set<BasicBlock> getBackwardReachable(BasicBlock target) {
        return backwardReachableMap.getOrDefault(target, Collections.emptySet());
    }

    /**
     * （对外）查询 from → to 路径上的中间块（并集），不含 from/to 本身
     */
    public Set<BasicBlock> findIntermediateBlocks(BasicBlock from, BasicBlock to) {
        Set<BasicBlock> forward = getForwardReachable(from);
        Set<BasicBlock> backward = getBackwardReachable(to);
        Set<BasicBlock> result = new HashSet<>();

        for (BasicBlock bb : forward.size() < backward.size() ? forward : backward) {
            if (bb != from && bb != to && forward.contains(bb) && backward.contains(bb)) {
                result.add(bb);
            }
        }
        return result;
    }

    /**
     * 全函数正向可达构建
     */
    private void buildForwardCache(Function func) {
        for (BasicBlock bb : func.getBasicBlockList()) {
            Set<BasicBlock> visited = new HashSet<>();
            Deque<BasicBlock> stack = new ArrayDeque<>();
            stack.push(bb);
            while (!stack.isEmpty()) {
                BasicBlock now = stack.pop();
                if (visited.add(now)) {
                    stack.addAll(now.getSucs());
                }
            }
            forwardReachableMap.put(bb, visited);
        }
    }

    /**
     * 全函数反向可达构建
     */
    private void buildBackwardCache(Function func) {
        for (BasicBlock bb : func.getBasicBlockList()) {
            Set<BasicBlock> visited = new HashSet<>();
            Deque<BasicBlock> stack = new ArrayDeque<>();
            stack.push(bb);
            while (!stack.isEmpty()) {
                BasicBlock now = stack.pop();
                if (visited.add(now)) {
                    stack.addAll(now.getPres());
                }
            }
            backwardReachableMap.put(bb, visited);
        }
    }
}