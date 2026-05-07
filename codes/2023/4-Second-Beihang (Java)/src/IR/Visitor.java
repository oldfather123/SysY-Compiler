package IR;

import Driver.Config;
import Frontend.AST;
import IR.Type.*;
import IR.Value.*;
import IR.Value.Instructions.*;
import Utils.DataStruct.IList;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class Visitor {
    private IRModule module;
    private Function CurFunction;
    private BasicBlock CurBasicBlock;
    private Value CurValue;
    //  符号表
    private final ArrayList<HashMap<String, Value>> symTbls = new ArrayList<>();
    private final IRBuildFactory f = IRBuildFactory.getInstance();
    private final ArrayList<GlobalVar> globalVars = new ArrayList<>();

    //  库函数
    private final Function PrintFunc = new Function("@putint", VoidType.voidType);
    private final Function InputFunc = new Function("@getint", IntegerType.I32);
    private final Function PrintChFunc = new Function("@putch", VoidType.voidType);
    private final Function InputChFunc = new Function("@getch", IntegerType.I32);
    private final Function PrintArrFunc = new Function("@putarray", VoidType.voidType);
    private final Function InputArrFunc = new Function("@getarray", IntegerType.I32);
    private final Function PrintFloatFunc = new Function("@putfloat", VoidType.voidType);
    private final Function InputFloatFunc = new Function("@getfloat", FloatType.F32);
    private final Function PrintFArrFunc = new Function("@putfarray", VoidType.voidType);
    private final Function InputFArrFunc = new Function("@getfarray", IntegerType.I32);
    public static final Function MemsetFunc = new Function("@memset", VoidType.voidType);
    private final Function StartTimeFunc = new Function("@_sysy_starttime", VoidType.voidType);
    private final Function StopTimeFunc = new Function("@_sysy_stoptime", VoidType.voidType);


    private boolean isPrint = false;

    //  从符号表中查找某个ident
    private Value find(String ident){
        int len = symTbls.size();
        for(int i = len - 1; i >= 0; i--){
            HashMap<String, Value> symTbl = symTbls.get(i);
            Value res = symTbl.get(ident);
            if(res != null){
                return res;
            }
        }

        if(ident.equals("putint") || ident.equals("putch") || ident.equals("putarray")){
            isPrint = true;
        }

        return switch (ident) {
            case "getint" -> InputFunc;
            case "putint" -> PrintFunc;
            case "getch" -> InputChFunc;
            case "putch" -> PrintChFunc;
            case "getarray" -> InputArrFunc;
            case "putarray" -> PrintArrFunc;
            case "getfloat" -> InputFloatFunc;
            case "putfloat" -> PrintFloatFunc;
            case "getfarray" -> InputFArrFunc;
            case "putfarray" -> PrintFArrFunc;
            case "starttime" -> StartTimeFunc;
            case "stoptime" -> StopTimeFunc;
            default -> null;
        };

    }
    //  向符号表中放入元素
    private void pushSymbol(String ident, Value value){
        int len = symTbls.size();
        symTbls.get(len - 1).put(ident, value);
    }

    private void pushSymTbl(){
        symTbls.add(new HashMap<>());
    }

    private void popSymTbl(){
        int len = symTbls.size();
        symTbls.remove(len - 1);
    }

    private final ArrayList<BasicBlock> whileEntryBLocks = new ArrayList<>();
    private final ArrayList<BasicBlock> whileOutBlocks = new ArrayList<>();

    //  argHashMap用于保存FuncFParams
    //  因为当你访问FuncFParams时，你还没有进入Block，而只有进入Block你才能push新的符号表
    //  所以为了把FuncFParams的声明也放进符号表，我们用tmpHashMap来保存
    private final HashMap<String, Value> argHashMap = new HashMap<>();

    //  isFetch表示当目标变量为指针的情况是否要取值
    //  true返回值，false返回指针
    private void visitLValAST(AST.LVal lValAST, boolean isFetch, boolean isConst){
        String ident = lValAST.getIdent();
        CurValue = find(ident);

        assert CurValue != null;
        /*
        * 传入的LVal参数有几种情况
        * 1. 普通的i32*
        * 2. 数组[4 * i32]*
        * 3. 数组参数 [4 * i32]**
        * 4. 数组参数 i32**
        * 5. 常数
        * 6. 全局const数组
        *
        * 注意，数组参数一定和isFetch=True配套出现，
        * 因为不会出现为数组指针赋值的情况
        *
        * */

        //  处理const数组
        if(isConst && CurValue instanceof GlobalVar globalVar && globalVar.isConst()){
            ArrayList<Integer> indexs = new ArrayList<>();
            for (AST.Exp exp : lValAST.getIndexes()) {
                visitExpAST(exp, true);
                indexs.add(((ConstInteger) CurValue).getValue());
            }
            CurValue = getGlobalArrValue(globalVar, indexs);
            return;
        }

        if(CurValue.getType().isPointerType()) {
            Type eleType = ((PointerType) CurValue.getType()).getEleType();
            //  处理数组参数

            //  处理indexes
            if (lValAST.getIndexes().size() != 0) {
                ArrayList<Value> indexs = new ArrayList<>();
                if (eleType instanceof PointerType) {
                    CurValue = f.buildLoadInst(CurValue, CurBasicBlock);
                }
                else {
                    indexs.add(f.buildNumber(0));
                }
                Value TmpValue = CurValue;
                for (AST.Exp exp : lValAST.getIndexes()) {
                    visitExpAST(exp, false);
                    indexs.add(CurValue);
                }
                CurValue = f.buildGepInst(TmpValue, indexs, CurBasicBlock);
            }

            if (isFetch) {
                Type type = ((PointerType) CurValue.getType()).getEleType();
                if (type.isArrayType()) {
                    ArrayList<Value> indexs = new ArrayList<>();
                    indexs.add(f.buildNumber(0));
                    indexs.add(f.buildNumber(0));
                    CurValue = f.buildGepInst(CurValue, indexs, CurBasicBlock);
                }
                else  {
                    CurValue = f.buildLoadInst(CurValue, CurBasicBlock);
                }
            }
        }
    }

    private void visitPrimaryExpAST(AST.PrimaryExp primaryExpAST, boolean isConst){
        if(primaryExpAST instanceof AST.Number number){
            if(number.isIntConst){
                CurValue = f.buildNumber(number.getIntConstVal());
            }
            else if(number.isFloatConst){
                CurValue = f.buildNumber(number.getFloatConstVal());
            }
            assert false;
        }
        else if(primaryExpAST instanceof AST.Exp expAST){
            visitExpAST(expAST, isConst);
        }
        else if(primaryExpAST instanceof AST.LVal lValAST){
            visitLValAST(lValAST, true, isConst);
        }
        else if(primaryExpAST instanceof AST.Call callAST){
            Function function = (Function) find(callAST.getIdent());
            ArrayList<AST.Exp> exps = callAST.getParams();

            ArrayList<Value> values = new ArrayList<>();

            for(AST.Exp exp : exps){
                visitExpAST(exp, isConst);
                values.add(CurValue);
            }

            ArrayList<Argument> arguments = function.getArgs();
            for(int i = 0; i < values.size(); i++){
                Value value = values.get(i);
                Type argType = arguments.get(i).getType();
                Type CurType = value.getType();
                if(CurType == IntegerType.I32 && argType == FloatType.F32){
                    values.set(i, f.buildConversionInst(value, OP.Itof, CurBasicBlock));
                }
                else if(CurType == FloatType.F32 && argType == IntegerType.I32){
                    values.set(i, f.buildConversionInst(value, OP.Ftoi, CurBasicBlock));
                }
            }

            if(function.equals(StartTimeFunc) || function.equals(StopTimeFunc)){
                values.add(f.buildNumber(0));
            }
            CurValue = f.buildCallInst(function, values, CurBasicBlock);
        }
    }

    private void visitUnaryExpAST(AST.UnaryExp unaryExpAST, boolean isConst){
        visitPrimaryExpAST(unaryExpAST.getPrimary(), isConst);
        ArrayList<String> unaryOPs = unaryExpAST.getUnaryOps();
        int count = 0;
        for (int i = unaryOPs.size() - 1; i >= 0; i--) {
            String unaryOP = unaryOPs.get(i);
            if (unaryOP.equals("-")) {
                count++;
            } else if (unaryOP.equals("!")) {
                count = 0;
                CurValue = f.buildBinaryInst(CurValue, f.buildI1Number(0), OP.Eq, CurBasicBlock);
            }
        }
        if (count % 2 == 1) {
            if(CurValue instanceof ConstInteger constInt){
                CurValue = f.buildNumber(-constInt.getValue());
            }
            else if(CurValue instanceof ConstFloat constFloat){
                CurValue = f.buildNumber(-constFloat.getValue());
            }
            else {
                CurValue = f.buildBinaryInst(f.buildNumber(0), CurValue, OP.Sub, CurBasicBlock);
            }
        }
    }

    private void visitBinaryExpAST(AST.BinaryExp binaryExpAST, boolean isConst){
        //  将binaryExp的first赋值到CurValue上
        visitExpAST(binaryExpAST.getFirst(), isConst);
        ArrayList<String> ops = binaryExpAST.getOperators();
        ArrayList<AST.Exp> exps = binaryExpAST.getFollows();
        for(int i = 0; i < ops.size(); i++){
            Value TmpValue = CurValue;
            visitExpAST(exps.get(i), isConst);

            if(TmpValue instanceof Const left && CurValue instanceof Const right){
                CurValue = f.buildCalculateNumber(left, right, ops.get(i));
            }
            else {
                CurValue = f.buildBinaryInst(TmpValue, CurValue, OP.str2op(ops.get(i)), CurBasicBlock);
            }
        }
    }

    private void visitExpAST(AST.Exp expAST, boolean isConst){
        if(expAST instanceof AST.UnaryExp unaryExpAST){
            visitUnaryExpAST(unaryExpAST, isConst);
        }
        else if(expAST instanceof AST.BinaryExp binaryExpAST){
            visitBinaryExpAST(binaryExpAST, isConst);
        }
    }

    private void visitLAndExpAST(AST.Exp lAndExpAST, BasicBlock TrueBlock, BasicBlock FalseBlock){
        BasicBlock NxtLAndBlock;
        AST.BinaryExp binaryExp = (AST.BinaryExp) lAndExpAST;
        AST.Exp nowExp;
        ArrayList<AST.Exp> follows = binaryExp.getFollows();

        for(int i = 0; i <= follows.size(); i++){
            if(i == 0) nowExp = binaryExp.getFirst();
            else nowExp = follows.get(i - 1);

            if(i != follows.size()){
                NxtLAndBlock = f.buildBasicBlock(CurFunction);
            }
            else NxtLAndBlock = TrueBlock;

            visitExpAST(nowExp, false);

            if(CurValue.getType() == IntegerType.I32){
                CurValue = f.buildBinaryInst(CurValue, f.buildNumber(0), OP.Ne, CurBasicBlock);
            }
            else{
                CurValue = f.buildBinaryInst(CurValue, f.buildI1Number(0), OP.Ne, CurBasicBlock);
            }
            f.buildBrInst(CurValue, NxtLAndBlock, FalseBlock, CurBasicBlock);

            CurBasicBlock = NxtLAndBlock;
        }
    }

    private void visitLOrExpAST(AST.Exp lOrExpAST, BasicBlock TrueBlock, BasicBlock FalseBlock){
        BasicBlock NxtLOrBlock;
        AST.BinaryExp binaryExpAST = (AST.BinaryExp) lOrExpAST;
        AST.Exp nowExp;
        ArrayList<AST.Exp> follows = binaryExpAST.getFollows();

        for(int i = 0; i <= follows.size(); i++){
            //  确定nowExp(需要计算是否为true的值)
            if(i == 0) nowExp = binaryExpAST.getFirst();
            else nowExp = follows.get(i - 1);
            //  确定目标跳转的TrueBlock和NxtBlock
            if(i != follows.size()){
                NxtLOrBlock = f.buildBasicBlock(CurFunction);
            }
            else NxtLOrBlock = FalseBlock;

            visitLAndExpAST(nowExp, TrueBlock, NxtLOrBlock);

            CurBasicBlock = NxtLOrBlock;
        }
    }

    private void visitStmtAST(AST.Stmt stmtAST){
        if(stmtAST instanceof AST.Return retAST){
            if(retAST.getRetExp() != null) {
                visitExpAST(retAST.getRetExp(), false);
                if(Config.outputReturn && CurFunction.getName().equals("@main")){
                    if(isPrint){
                        ArrayList<Value> values = new ArrayList<>();
                        values.add(new ConstInteger(10, IntegerType.I32));
                        f.buildCallInst(PrintChFunc, values, CurBasicBlock);
                    }
                    //  输出返回值
                    ArrayList<Value> values_2 = new ArrayList<>();
                    values_2.add(CurValue);
                    f.buildCallInst(PrintFunc, values_2, CurBasicBlock);
                }

                Type CurType = CurValue.getType();
                Type CurFuncType = CurFunction.getType();
                if(CurType == IntegerType.I32 && CurFuncType == FloatType.F32){
                    CurValue = f.buildConversionInst(CurValue, OP.Itof, CurBasicBlock);
                }
                else if(CurFuncType == IntegerType.I32 && CurType == FloatType.F32){
                    CurValue = f.buildConversionInst(CurValue, OP.Ftoi, CurBasicBlock);
                }

                CurValue = f.buildRetInst(CurValue, CurBasicBlock);
            }
            else{
                f.buildRetInst(CurBasicBlock);
            }
        }
        else if(stmtAST instanceof AST.Assign assignAST){
            visitLValAST(assignAST.getLVal(), false, false);
            Value pointer = CurValue;
            visitExpAST(assignAST.getValue(), false);
            f.buildStoreInst(CurValue, pointer, CurBasicBlock);
        }
        else if(stmtAST instanceof AST.ExpStmt expStmtAST){
            visitExpAST(expStmtAST.getExp(), false);
        }
        else if(stmtAST instanceof AST.Block blockAST){
            pushSymTbl();
            visitBlockAST(blockAST);
            popSymTbl();
        }
        else if(stmtAST instanceof AST.IfStmt ifStmt){
            BasicBlock TrueBlock = f.buildBasicBlock(CurFunction);
            BasicBlock NxtBlock = f.buildBasicBlock(CurFunction);
            BasicBlock FalseBlock = null;
            boolean hasElse = (ifStmt.getElseTarget() != null);
            if(hasElse){
                FalseBlock = f.buildBasicBlock(CurFunction);
                visitLOrExpAST(ifStmt.getCond(), TrueBlock, FalseBlock);
            }
            else{
                visitLOrExpAST(ifStmt.getCond(), TrueBlock, NxtBlock);
            }
            //  VisitCondAST之后，CurBlock的br已经构建完并指向正确的Block
            //  接下来我们为TrueBlock填写指令
            CurBasicBlock = TrueBlock;
            visitStmtAST(ifStmt.getThenTarget());

            //  下面先考虑ifStmt中CurBlock不发生变化的情况
            //  即TrueBlock没有被构建br指令
            //  那么我们显然要给它构建br指令并设置CurBlock为NxtBlock
            //  然后我们发现就算CurBlock发生了变化，那么也是变成了TrueBlock里的NxtBlock
            //  而且是没有终结的状态，因此我们下面两行代码也可以适用于这种情况,令其跳转
            f.buildBrInst(NxtBlock, CurBasicBlock);

            if(ifStmt.getElseTarget() != null){
                //  开始构建FalseBlock
                CurBasicBlock = FalseBlock;
                visitStmtAST(ifStmt.getElseTarget());

                //  原理同上，为CurBLock构建Br指令
                f.buildBrInst(NxtBlock, CurBasicBlock);
            }
            CurBasicBlock = NxtBlock;
        }
        else if(stmtAST instanceof AST.WhileStmt whileStmt){
            //  构建要跳转的CurCondBlock
            BasicBlock CondBlock = f.buildBasicBlock(CurFunction);
            f.buildBrInst(CondBlock, CurBasicBlock);
            CurBasicBlock = CondBlock;

            BasicBlock TrueBlock = f.buildBasicBlock(CurFunction);
            BasicBlock FalseBlock = f.buildBasicBlock(CurFunction);
            //  入栈，注意这里entry为CurCondBlock，因为continue要重新判断条件
            whileEntryBLocks.add(CondBlock);
            whileOutBlocks.add(FalseBlock);

            visitLOrExpAST(whileStmt.getCond(), TrueBlock, FalseBlock);

            CurBasicBlock = TrueBlock;
            visitStmtAST(whileStmt.getBody());
            f.buildBrInst(CondBlock, CurBasicBlock);
            CurBasicBlock = FalseBlock;

            //  while内的指令构建完了，出栈
            whileEntryBLocks.remove(whileEntryBLocks.size() - 1);
            whileOutBlocks.remove(whileOutBlocks.size() - 1);
        }
        else if(stmtAST instanceof AST.Break ){
            if(whileOutBlocks.size() == 0) return;

            int len = whileOutBlocks.size();
            BasicBlock whileOutBlock = whileOutBlocks.get(len - 1);

            f.buildBrInst(whileOutBlock, CurBasicBlock);
            CurBasicBlock = f.buildBasicBlock(CurFunction);
        }
        else if(stmtAST instanceof AST.Continue ){
            if(whileEntryBLocks.size() == 0) return;

            int len = whileEntryBLocks.size();
            BasicBlock whileEntryBlock = whileEntryBLocks.get(len - 1);

            f.buildBrInst(whileEntryBlock, CurBasicBlock);
            CurBasicBlock = f.buildBasicBlock(CurFunction);
        }
    }

    //  很重要的一个函数，visit杂乱无章的初始值并分析出哪里需要填充value
    private ArrayList<Value> visitInitArray(ArrayList<Integer> indexs, AST.InitArray initArray, Value fillValue, boolean isConst){
        int curNum = 0;
        int totSize = 1;
        ArrayList<Value> values = new ArrayList<>();
        for (Integer index : indexs) {
            totSize *= index;
        }

        for(AST.Init init : initArray.init){
            if(init instanceof AST.Exp expAST){
                curNum++;
                visitExpAST(expAST, isConst);
                values.add(CurValue);
                //  TODO ！！！常量数组的优化！！！
            }
            else if(init instanceof AST.InitArray newInitArray){
                ArrayList<Integer> newIndexs = new ArrayList<>();
                int start = 0;
                if(curNum == 0){
                    start = 1;
                }
                else{
                    int tmpMul = 1;
                    for(int i = indexs.size() - 1; i >= 0; i--){
                        tmpMul *= indexs.get(i);
                        if(curNum % tmpMul != 0){
                            start = i + 1;
                            break;
                        }
                    }
                }

                for(int i = start; i < indexs.size(); i++){
                    newIndexs.add(indexs.get(i));
                }
                ArrayList<Value> newValues = visitInitArray(newIndexs, newInitArray, fillValue, isConst);
                values.addAll(newValues);
                curNum += newValues.size();
            }
        }

        //  填充元素
        for(int i = curNum; i < totSize; i++){
            values.add(fillValue);
        }
        return values;
    }

    private void visitConstDef(AST.Def def, Type type, boolean isGlobal){
        String ident = def.getIdent();
        AST.Init init = def.getInit();
        //  常量数组
        if (def.indexes.size() != 0) {
            visitArray(ident, init, def.indexes, type, isGlobal, true);

        }
        //  普通常量
        else {
            visitExpAST((AST.Exp) init, true);
            //  根据定义纠正类型
            if(type == IntegerType.I32 && CurValue.getType().isFloatTy()){
                CurValue = f.buildNumber((int) ((ConstFloat) CurValue).getValue());
            }
            else if(type == FloatType.F32 && CurValue.getType().isIntegerTy()){
                CurValue = f.buildNumber((float) ((ConstInteger) CurValue).getValue());
            }
            pushSymbol(ident, CurValue);
        }
    }

    private void visitVarDef(AST.Def def, Type type ,boolean isGlobal){
        String ident = def.getIdent();
        AST.Init init = def.getInit();
        Value fillValue = (type == IntegerType.I32 ? f.buildNumber(0) : f.buildNumber((float) 0.0));

        //  数组
        if(def.indexes.size() != 0){
            visitArray(ident, init, def.indexes, type, isGlobal, isGlobal);
        }
        //  普通变量
        else{
            if(isGlobal){
                if(init instanceof AST.Exp expAST){
                    visitExpAST(expAST, true);
                    CurValue = f.buildGlobalVar(ident, CurValue.getType(), CurValue);
                }
                else if(init == null){
                    CurValue = f.buildGlobalVar(ident, type, fillValue);
                }
                globalVars.add((GlobalVar) CurValue);
            }
            else{
                CurValue = f.buildAllocInst(type, CurBasicBlock);
                if (init instanceof AST.Exp expAST){
                    Value TmpValue = CurValue;
                    visitExpAST(expAST, false);
                    f.buildStoreInst(CurValue, TmpValue, CurBasicBlock);
                    CurValue = TmpValue;
                }
            }
            pushSymbol(ident, CurValue);
        }
    }

    private void visitArray(String ident, AST.Init init, ArrayList<AST.Exp> indexsAST, Type type, boolean isGlobal, boolean isConst){
        ArrayList<Integer> dimIndexs = new ArrayList<>();
        Value fillValue = (type == IntegerType.I32 ? f.buildNumber(0) : f.buildNumber((float) 0.0));
        //  局部数组填充的地方用flag先标记上，之后gep会跳过这些默认填充的数据
        if(!isGlobal) fillValue = new Value("flag", VoidType.voidType);

        for(AST.Exp exp : indexsAST){
            visitExpAST(exp, true);
            dimIndexs.add(((ConstInteger) CurValue).getValue());
        }
        ArrayList<Value> values = new ArrayList<>();
        if(init instanceof AST.InitArray initArray){
            if(initArray.init.size() != 0 || !isGlobal) {
                values = visitInitArray(dimIndexs, initArray, fillValue, isConst);
            }
        }

        if(isGlobal){
            CurValue = f.getGlobalArray(ident, type, dimIndexs, values);
            globalVars.add((GlobalVar) CurValue);
            ((GlobalVar) CurValue).setConst(true);
            pushSymbol(ident, CurValue);
        }
        else{
            CurValue = f.buildArray(type, dimIndexs, CurBasicBlock);
            pushSymbol(ident, CurValue);
            AllocInst baseValue = (AllocInst) CurValue;

            ArrayType arrayType = (ArrayType) ((PointerType) CurValue.getType()).getEleType();
            ArrayList<Value> indexs = new ArrayList<>();
            int dim = arrayType.getDim();

            //  memset
            //  先用gep获取头部i32*指针
            for(int i = 0; i <= dim; i++){
                indexs.add(f.buildNumber(0));
            }
            CurValue = f.buildGepInst(baseValue, indexs, CurBasicBlock);
            if(type.isFloatTy()){
                CurValue = f.buildConversionInst(CurValue, OP.BitCast, CurBasicBlock);
            }

            ArrayList<Value> memValues = new ArrayList<>();
            memValues.add(CurValue);
            memValues.add(f.buildNumber(0));
            memValues.add(new ConstInteger(values.size() * 4, IntegerType.I32));
            f.buildCallInst(MemsetFunc, memValues, CurBasicBlock);

            //  开始gep+store初始化局部数组值
            //  这里的initValue中用null表示填充值
            ArrayList<Value> initValue = new ArrayList<>();
            indexs.remove(indexs.size() - 1);
            for(int i = 0; i < values.size(); i++){
                Value value = values.get(i);
                if(value.getName().equals("flag")){
                    if(type.isIntegerTy()) values.set(i, f.buildNumber(0));
                    else if(type.isFloatTy()) values.set(i, f.buildNumber((float) 0.0));
                    initValue.add(null);
                    continue;
                }
                indexs.add(f.buildNumber(i));
                CurValue = f.buildGepInst(baseValue, indexs, CurBasicBlock);
                indexs.remove(indexs.size() - 1);
                f.buildStoreInst(value, CurValue, CurBasicBlock);
                initValue.add(value);
            }
            //  将初始值信息存入局部数组
            baseValue.setInitValues(initValue);
        }
    }

    private void visitDeclAST(AST.Decl declAST, boolean isGlobal){
        boolean isConst = declAST.isConstant();
        String typeStr = declAST.getBType();
        ArrayList<AST.Def> defs = declAST.getDefs();

        for (AST.Def def : defs) {
            Type type = null;
            if(typeStr.equals("int")) type = IntegerType.I32;
            else if(typeStr.equals("float")) type = FloatType.F32;
            if(isConst) visitConstDef(def, type, isGlobal);
            else visitVarDef(def, type, isGlobal);
        }
    }

    private void visitBlockItemAST(AST.BlockItem blockItemAST){
        if(blockItemAST instanceof AST.Stmt stmtAST){
            visitStmtAST(stmtAST);
        }
        else if(blockItemAST instanceof AST.Decl declAST){
            visitDeclAST(declAST, false);
        }
    }

    private void visitBlockAST(AST.Block blockAST){
        ArrayList<AST.BlockItem> blockItemASTS = blockAST.getItems();
        for(AST.BlockItem blockItemAST : blockItemASTS){
            visitBlockItemAST(blockItemAST);
        }
    }


    private void visitFuncDefAST(AST.FuncDef funcDefAST) {
        String ident = funcDefAST.getIdent();
        String type = funcDefAST.getType();
        CurFunction = f.buildFunction("@" + ident, type, module);

        pushSymbol(ident, CurFunction);
        argHashMap.clear();

        pushSymTbl();
        CurBasicBlock = f.buildBasicBlock(CurFunction);
        if(funcDefAST.getFParams().size() > 0){
            //  开始构建entry基本块
            ArrayList<AST.FuncFParam> funcFParams = funcDefAST.getFParams();
            for(AST.FuncFParam funcFParam : funcFParams){
                //  平平无奇的起名环节
                String argName = funcFParam.getIdent();
                String argType = funcFParam.getBType();
                Argument argument;
                if(funcFParam.array){
                    ArrayList<Integer> sizes = new ArrayList<>();
                    ArrayList<AST.Exp> sizesExp = funcFParam.getSizes();
                    for (AST.Exp exp : sizesExp) {
                        visitExpAST(exp, true);
                        sizes.add(((ConstInteger) CurValue).getValue());
                    }
                    if(sizes.size() != 0) {
                        argument = f.buildArgument(argName, argType, CurFunction, sizes);
                    }
                    else argument = f.buildArgument(argName, argType + "*", CurFunction);
                }
                else {
                    argument = f.buildArgument(argName, argType, CurFunction);
                }
                AllocInst allocInst = f.buildAllocInst(argument.getType(), CurBasicBlock);
                f.buildStoreInst(argument, allocInst, CurBasicBlock);
                pushSymbol(argName, allocInst);
            }
            BasicBlock TmpBasicBlock = f.buildBasicBlock(CurFunction);
            f.buildBrInst(TmpBasicBlock, CurBasicBlock);
            CurBasicBlock = TmpBasicBlock;
        }

        visitBlockAST(funcDefAST.getBody());

        popSymTbl();

        //  删除掉br/ret后面不可达的语句以及无语句的bb
        IList.INode<BasicBlock, Function> itBbNode = CurFunction.getBbs().getHead();
        while (itBbNode != null){
            BasicBlock bb = itBbNode.getValue();
            itBbNode = itBbNode.getNext();
            boolean isTerminal = false;
            IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();

            while (itInstNode != null){
                Instruction inst = itInstNode.getValue();
                itInstNode = itInstNode.getNext();
                if (isTerminal){
                    inst.removeSelf();
                }
                else{
                    if(inst instanceof RetInst || inst instanceof BrInst){
                        isTerminal = true;
                        if(inst instanceof RetInst)
                        {
//                            CurFunction.setBbExit(bb);
                        }
                    }
                }
            }

            // 如果没有ret语句，构建一个ret void
            if(!isTerminal){
//                CurFunction.setBbExit(CurBasicBlock);
                if(CurFunction.getType().isVoidTy()){
                    f.buildRetInst(CurBasicBlock);
                }
                else if(CurFunction.getType().isIntegerTy()) f.buildRetInst(f.buildNumber(0), CurBasicBlock);
                else if(CurFunction.getType().isFloatTy()) f.buildRetInst(f.buildNumber((float) 0.0), CurBasicBlock);
            }
        }
    }

    private void initLibFunc(){
        PrintFunc.addArg(new Argument("x", IntegerType.I32, PrintFunc));
        PrintChFunc.addArg(new Argument("x", IntegerType.I32, PrintChFunc));
        PrintFloatFunc.addArg(new Argument("x", FloatType.F32, PrintFloatFunc));
        PrintArrFunc.addArg(new Argument("x", IntegerType.I32, PrintArrFunc));
        PrintArrFunc.addArg(new Argument("x2", new PointerType(IntegerType.I32), PrintArrFunc));
        PrintFArrFunc.addArg(new Argument("x", IntegerType.I32, PrintFArrFunc));
        PrintFArrFunc.addArg(new Argument("x2", new PointerType(FloatType.F32), PrintFArrFunc));

        InputArrFunc.addArg(new Argument("x", new PointerType(IntegerType.I32), InputArrFunc));
        InputFArrFunc.addArg(new Argument("x", new PointerType(FloatType.F32), InputFArrFunc));

        MemsetFunc.addArg(new Argument("x", new PointerType(IntegerType.I32), MemsetFunc));
        MemsetFunc.addArg(new Argument("x2", IntegerType.I32, MemsetFunc));
        MemsetFunc.addArg(new Argument("x3", IntegerType.I32, MemsetFunc));

        StartTimeFunc.addArg(new Argument("x", IntegerType.I32, StartTimeFunc));
        StopTimeFunc.addArg(new Argument("x2", IntegerType.I32, StopTimeFunc));

        PrintFunc.setAsLibFunction();
        PrintChFunc.setAsLibFunction();
        PrintFloatFunc.setAsLibFunction();
        PrintArrFunc.setAsLibFunction();
        PrintFArrFunc.setAsLibFunction();

        InputFunc.setAsLibFunction();
        InputChFunc.setAsLibFunction();
        InputFloatFunc.setAsLibFunction();
        InputArrFunc.setAsLibFunction();
        InputFArrFunc.setAsLibFunction();

        MemsetFunc.setAsLibFunction();
        StartTimeFunc.setAsLibFunction();
        StopTimeFunc.setAsLibFunction();
    }

    public IRModule visitAST(AST compAST) {
        ArrayList<Function> functions = new ArrayList<>();
        module = new IRModule(functions, globalVars);

        initLibFunc();
        pushSymTbl();

        for (AST.CompUnit compUnit : compAST.getUnits()) {
            if (compUnit instanceof AST.FuncDef funcDefAST) {
                visitFuncDefAST(funcDefAST);
            }
            else if(compUnit instanceof AST.Decl declAST){
                visitDeclAST(declAST, true);
            }
        }
        return module;
    }

    private Value getGlobalArrValue(GlobalVar globalVar, ArrayList<Integer> indexs){
        ArrayList<Value> initVal = globalVar.getValues();
        //  全局数组的init value应该都是常数
        ArrayType arrayType = (ArrayType) ((PointerType)globalVar.getType()).getEleType();

        if(globalVar.isAllZero()){
            if(arrayType.isIntegerArr()){
                return f.buildNumber(0);
            }
            else return f.buildNumber((float) 0.0);
        }

        ArrayList<Integer> numList = arrayType.getNumList();
        //  如果gep的是一个i32*，那么indexSize应该正好等于维度的size
        if(numList.size() != indexs.size()){
            return null;
        }

        ArrayList<Integer> mulFactors = new ArrayList<>();
        mulFactors.add(0, 1);
        for(int i = numList.size() - 1; i > 0; i--){
            int factor = mulFactors.get(0);
            mulFactors.add(0, factor * numList.get(i));
        }

        int totIndex = 0;
        for(int i = 0; i < indexs.size(); i++){
            totIndex += indexs.get(i) * mulFactors.get(i);
        }

        return initVal.get(totIndex);
    }
}
