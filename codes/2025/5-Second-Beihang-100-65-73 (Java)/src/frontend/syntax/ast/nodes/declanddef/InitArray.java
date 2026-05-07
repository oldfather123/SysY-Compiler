package frontend.syntax.ast.nodes.declanddef;

import frontend.syntax.ast.AST;
import frontend.syntax.ast.nodes.expression.IExp;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class InitArray extends AST.Node implements IInitVal {
    private final List<IInitVal> initValList;
    
    public InitArray(List<IInitVal> initValList, int lineno) {
        super(lineno);
        this.initValList = initValList;
    }
    
    public static List<IInitVal> completeInit(List<Integer> limList, InitArray initArray) {
        List<IInitVal> result = new ArrayList<>();
        if (limList.isEmpty()) {
            throw new RuntimeException("数组不能一维没有");
        }
        
        int dim = limList.size();
        int curLim = limList.get(0);
        
        Iterator<IInitVal> it = initArray.initValList.iterator();
        
        if (dim == 1) {
            while (it.hasNext() && result.size() < curLim) {
                if (it.next() instanceof IExp exp) {
                    result.add(exp);
                    it.remove();
                } else {
                    throw new RuntimeException("最后一维初始化列表里只能有表达式");
                }
            }
        } else {
            List<Integer> nextLimList = limList.subList(1, limList.size());
            while (it.hasNext() && result.size() < curLim) {
                if (it.next() instanceof InitArray innerInitArray) {
                    result.add(new InitArray(completeInit(nextLimList, innerInitArray), initArray.getLineno()));
                    it.remove();
                } else {
                    result.add(new InitArray(completeInit(nextLimList, initArray), initArray.getLineno()));
                    it = initArray.initValList.iterator();
                }
            }
        }
        
        return result;
    }
}
