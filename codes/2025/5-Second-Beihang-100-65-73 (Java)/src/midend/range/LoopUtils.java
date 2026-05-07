package midend.range;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

public class LoopUtils {
    private LoopUtils() {
    }

    public static Map<BasicBlock, LoopInfo> buildBlockToInnermostLoop(Function f) {
        Map<BasicBlock, LoopInfo> map = new HashMap<>();
        for (LoopInfo top : f.getAncientLoopInfo()) fillLoop(top, map);
        return map;
    }

    private static void fillLoop(LoopInfo L, Map<BasicBlock, LoopInfo> map) {
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        L.getAllBlocks(allBlocks);
        for (BasicBlock bb : allBlocks) map.put(bb, L);
        for (LoopInfo ch : L.getChildLoop()) fillLoop(ch, map);
    }

    public static boolean inBodyBlocksButNotHeader(BasicBlock b, LoopInfo l) {
        return l.getBodyBlocks().contains(b) && b != l.getLoopHeader();
    }
}
