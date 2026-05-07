package pass.ir;

import pass.Pass;
import pass.utils.ControlFlowGraph;
import pass.utils.DominatorTree;

public class DomTree implements Pass.IrPass {
    @Override
    public void run(ir.IrModule module) {
        for (var function : module.getFunctions()) {
            function.setCFG(ControlFlowGraph.fromFunction(function));
            function.setDomTree(new DominatorTree(function));
        }
    }

    @Override
    public String getName() {
        return "dom-tree";
    }
}
