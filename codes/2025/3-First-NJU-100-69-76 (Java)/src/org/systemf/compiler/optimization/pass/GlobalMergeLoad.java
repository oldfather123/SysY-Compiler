package org.systemf.compiler.optimization.pass;

import org.systemf.compiler.analysis.CFGAnalysisResult;
import org.systemf.compiler.analysis.DominanceAnalysisResult;
import org.systemf.compiler.analysis.PointerAnalysisResult;
import org.systemf.compiler.analysis.ReachabilityAnalysisResult;
import org.systemf.compiler.ir.Module;
import org.systemf.compiler.ir.block.BasicBlock;
import org.systemf.compiler.ir.global.Function;
import org.systemf.compiler.ir.value.Value;
import org.systemf.compiler.ir.value.instruction.nonterminal.memory.Load;
import org.systemf.compiler.optimization.pass.util.MergeHelper;
import org.systemf.compiler.query.QueryManager;
import org.systemf.compiler.util.Tree;

import java.util.*;

/**
 * Depend on: CFGAnalysis, PointerAnalysis, ReachabilityAnalysis
 * <p>
 * Applicable to: IR
 */
public enum GlobalMergeLoad implements OptPass {
	INSTANCE;

	@Override
	public boolean run(Module module) {
		return new GlobalMergeLoadContext(module).run();
	}

	private static class GlobalMergeLoadContext {
		private final QueryManager query = QueryManager.getInstance();
		private final Module module;
		private final PointerAnalysisResult ptrResult;
		private CFGAnalysisResult cfg;
		private ReachabilityAnalysisResult reachability;
		private Map<BasicBlock, Optional<Set<Value>>> affecting;
		private Map<BasicBlock, Map<Value, Value>> loading;
		private Tree<BasicBlock> domTree;

		public GlobalMergeLoadContext(Module module) {
			this.module = module;
			this.ptrResult = query.getAttribute(module, PointerAnalysisResult.class);
		}

		private void collectLoading(BasicBlock block) {
			loading.put(block, MergeHelper.constructLoadMap(block, module, ptrResult));
		}

		private boolean processBlock(BasicBlock block) {
			var res = false;
			var affected = new HashSet<Value>();
			for (var inst : block.instructions) {
				if (MergeHelper.manipulateAffected(module, inst, affected, ptrResult)) break;
				if (!(inst instanceof Load load)) continue;
				var loadPtr = load.getPointer();
				var loadFrom = ptrResult.pointTo(loadPtr);
				if (loadFrom.stream().anyMatch(affected::contains)) continue;

				var upper = domTree.getParent(block);
				while (upper != null) {
					if (loading.get(upper).containsKey(loadPtr)) break;
					upper = domTree.getParent(upper);
				}
				if (upper == null) continue;

				if (!MergeHelper.affectingFree(upper, block, loadFrom, affecting, cfg, reachability)) continue;

				res = true;
				load.replaceAllUsage(loading.get(upper).get(loadPtr));
			}
			return res;
		}

		private boolean processFunction(Function function) {
			cfg = query.getAttribute(function, CFGAnalysisResult.class);
			domTree = query.getAttribute(function, DominanceAnalysisResult.class).dominance();
			reachability = query.getAttribute(function, ReachabilityAnalysisResult.class);
			affecting = MergeHelper.constructAffecting(module, function, ptrResult);
			loading = new HashMap<>();
			function.getBlocks().forEach(this::collectLoading);

			var res = function.getBlocks().stream().sorted(Comparator.comparingInt(domTree::getDfn))
					.map(this::processBlock).reduce(false, (a, b) -> a || b);
			if (res) query.invalidateAllAttributes(function);
			return res;
		}

		public boolean run() {
			var res = module.getFunctions().values().stream().map(this::processFunction)
					.reduce(false, (a, b) -> a || b);
			if (res) query.invalidateAllAttributes(module);
			return res;
		}
	}
}
