package midend.DCE;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

/**
 * 从头开始遍历后继，遍历不到的块直接删除，但是现在看好像是没用上
 * 在CFGbuild时我会删除不可达块
 * fixme 这里其实有问题，一旦要删除整个块，块内互相引用可能导致删不掉
 */
public class RadicalDeadBlockRemove {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            BasicBlock headBlk = function.getFirstBlk();
            Set<BasicBlock> visited = new HashSet<>();
            visited.add(headBlk);
            Queue<BasicBlock> toBeVisited = new LinkedList<>(headBlk.getSucs());
            while (!toBeVisited.isEmpty()) {
                BasicBlock visit = toBeVisited.poll();
                if (!visited.contains(visit)) {
                    visited.add(visit);
                    toBeVisited.addAll(visit.getSucs());
                }
            }
            
            List<BasicBlock> blkList = new ArrayList<>(function.getBasicBlockList());
            for (BasicBlock blk : blkList) {
                if (!visited.contains(blk)) {
                    blk.removeFromList();
                }
            }
        }
    }
}
