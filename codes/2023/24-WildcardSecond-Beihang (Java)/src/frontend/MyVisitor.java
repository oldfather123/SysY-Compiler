package frontend;

import backend.operand.RiscvReg;
import ir.Value;
import ir.instr.*;
import ir.type.*;
import ir.value.*;
import ir.value.Module;
import ir.value.user.Function;
import org.antlr.v4.runtime.tree.TerminalNode;
import tools.*;

import java.util.*;

import static tools.IRBuilder.*;

public class MyVisitor extends sysyBaseVisitor<Void> {

    public Module module;
    private boolean needConst;
    //对于全局变量定义阶段，所有变量都统一通过global语句初始化，故需要立刻计算出结果。
    //needComputeNow的优先级大于isDeclaration，既如果需要立刻计算则尝试编译时计算初始化值
    private boolean needComputeNow;
    //对于函数内的visit过程，需要进行两趟，第一趟定义所有变量，第二趟正式转换为赋值语句。
    //因此，对于如int a = a0;的语句，第一趟中仅添加到局部变量声明，第二趟仅计算赋值。
    //对此，isDeclaration标记了这种区别
    private boolean needDeclaration;
    //----------------------------------------------------------------
    //关键，通过container定义了期望的visit返回，如期望int却下一个visit到了float就可以及时报错退出。
    //同时，visit数组的实现也通过递归下降地设置container为子容器来完成。
    private Object container;
    //----------------------------------------------------------------
    //ret开头用来存储返回值

    private Value retv;
    private Type retType;
    private String retStr;
    private boolean isGlobal = false;

    //时刻管理variables哈希表，每个变量中lastInst记录了上一个给它赋值的指令，用到变量既用到上一个赋值的指令。
    //进出basic块时要处理内外变量切换。
    //键值是变量名
    //存储变量
    //可以通过变量名查找上一阶的变量
    //允许查找该指令的alloca
    private final ValueStack variableStack = new ValueStack();
    //存储常量
    private final ValueStack constStack = new ValueStack();
    //只允许Funcdef压入dispatcher
    private final ArrayList<IrRegDispatcher> dispatcherStack = new ArrayList<>();
    //处理Block递归
    private final ArrayList<BasicBlock> blockStack = new ArrayList<>();
    private final ArrayList<BasicBlock> loopEndStack = new ArrayList<>();
    private final ArrayList<BasicBlock> loopStartStack = new ArrayList<>();
    private Function function = null;
    BasicBlock globalInstr;
    BasicBlock trueTargetBlock;
    BasicBlock falseTargetBlock;
    BasicBlock funcEntryBlock;


    //--------------------------tools----------------------------
    private int blockNo;

    public MyVisitor() {
    }

    public IrRegDispatcher getTopDispatcher() {
        return dispatcherStack.get(dispatcherStack.size() - 1);
    }

    public ArrayList<Integer> parseDimensionByConstExps(List<sysyParser.ConstExpContext> exps) {
        ArrayList<Integer> dimension = new ArrayList<>();
        for (var sub : exps) {
            container = 0; //期望返回类型，
            visit(sub);
            dimension.add((Integer) container);
        }
        return dimension;
    }

    private void addAllBuiltInFunctions() {
        LinkedHashMap<String, ArrayList<Type>> builtInFunctions = new LinkedHashMap<>();
        Type[] args = {new IntType()};
        builtInFunctions.put("getint", new ArrayList<>(Arrays.asList(args)));
        builtInFunctions.put("getch", new ArrayList<>(Arrays.asList(args)));
        args = new Type[] {new FloatType()};
        builtInFunctions.put("getfloat", new ArrayList<>(Arrays.asList(args)));
        args = new Type[] {new IntType(), new Pointer(new IntType())};
        builtInFunctions.put("getarray", new ArrayList<>(Arrays.asList(args)));
        args[1] = new Pointer(new FloatType());
        builtInFunctions.put("getfarray", new ArrayList<>(Arrays.asList(args)));
        args[0] = new VoidType();
        args[1] = new IntType();
        builtInFunctions.put("putint", new ArrayList<>(Arrays.asList(args)));
        builtInFunctions.put("putch", new ArrayList<>(Arrays.asList(args)));
        args[1] = new FloatType();
        builtInFunctions.put("putfloat", new ArrayList<>(Arrays.asList(args)));
        args = new Type[] {new VoidType(), new IntType(), new Pointer(new IntType())};
        builtInFunctions.put("putarray", new ArrayList<>(Arrays.asList(args)));
        args[2] = new Pointer(new FloatType());
        builtInFunctions.put("putfarray", new ArrayList<>(Arrays.asList(args)));
        args[1] = new Pointer(new IntType(8));
        args[2] = new VariLen();
        builtInFunctions.put("putf", new ArrayList<>(Arrays.asList(args)));
        args = new Type[] {new VoidType(), new IntType()};
        builtInFunctions.put("_sysy_starttime", new ArrayList<>(Arrays.asList(args)));
        builtInFunctions.put("_sysy_stoptime", new ArrayList<>(Arrays.asList(args)));

        args =
            new Type[] {new VoidType(), new Pointer(new IntType()), new IntType(), new IntType()};
        builtInFunctions.put("memset", new ArrayList<>(Arrays.asList(args)));

        for (var name : builtInFunctions.keySet()) {
            Function f = new Function(module, name, builtInFunctions.get(name).get(0), null);
            if (builtInFunctions.get(name).size() > 1) {
                for (var arg : builtInFunctions.get(name).subList(1,
                    builtInFunctions.get(name).size())) {
                    f.addArgForBuiltin(arg);
                }
            }
            module.addFunction(name, f);
        }
    }

    //----------------------------------------------------------
    @Override
    public Void visitCompUnit(sysyParser.CompUnitContext ctx) {
        //新建空模块
        this.module = new Module(null, null, "module");
        addAllBuiltInFunctions();
        //遍历模块内部，先处理所有全局变量声明，再处理所有函数声明
        //压入全局变量栈
        variableStack.push(new LinkedHashMap<>());
        constStack.push(new LinkedHashMap<>());
        needDeclaration = true;
        needComputeNow = true;//设置需要立刻计算初始化值
        //加入isGlobal区分全局变量与普通变量的初始化
        isGlobal = true;
        for (var sub : ctx.decl()) {
            visitDecl(sub);
        }
        isGlobal = false;
        needComputeNow = false;
        needDeclaration = false;

        //建立全局变量生成基本块，遍历所有全局变量，生成对应的global指令，维护全局变量hash表
        globalInstr = new BasicBlock("global insts", this.module, false);

        module.buildGlobalAssign(globalInstr);
        module.init = globalInstr;
        // TODO: 部分开始函数visit
        //再处理所有函数声明
        for (var sub : ctx.funcDef()) {
            visitFuncDef(sub);
        }
        return null;
    }

    /*
    decl://声明 Decl → ConstDecl | VarDecl
    constDecl |
    varDecl;
     */
    @Override
    public Void visitDecl(sysyParser.DeclContext ctx) {
        for (var sub : ctx.children) {
            visit(sub);
        }
        return null;
    }

    //visit非const变量声明，当前只完成全局变量，因为不带初始化initval
    @Override
    public Void visitVarDecl(sysyParser.VarDeclContext ctx) {
        //先visit变量类型
        visit(ctx.bType());
        for (var sub : ctx.varDef()) {
            visit(sub);
            if (needDeclaration) {
                variableStack.addValue(retv.name, retv);
                if (isGlobal) {
                    module.addGlobal((Variable) retv);
                } else {
                    Instr alloca = new Alloca(retv.type, retv.name, getTopBlock(),
                        getTopDispatcher());
                    ((Variable) retv).allocInst = alloca;
                    funcEntryBlock.addValue(alloca);
                }
            }
        }
        return null;
    }

    @Override
    public Void visitConstDecl(sysyParser.ConstDeclContext ctx) {
        if (needDeclaration) {//const在编译的声明轮次中完成所有操作。
            visit(ctx.bType());
            for (var sub : ctx.constDef()) {
                visit(sub);
                constStack.addValue(retv.name, retv);
                if(retv.type instanceof ArrayType) {
                    if (isGlobal) {
                        module.addConstGlobal((ConstVariable) retv);
                    } else {
                        retv.name =
                                function.name + "_" + retv.name + "_" + getTopDispatcher().allocId("const");
                        module.addGlobalAssign((ConstVariable) retv, globalInstr);
                    }
                }
            }
        }
        return null;
    }

    //根据变量类型，保存类型对象到retType
    @Override
    public Void visitBType(sysyParser.BTypeContext ctx) {
        switch (ctx.children.get(0).toString()) {
            case "int" -> retType = new IntType();
            case "float" -> retType = new FloatType();
            case "void" -> retType = new VoidType();
            default -> retType = null;
        }
        return null;
    }


    //读取带有维度的变量声明，目前只完成了不带有初始化initval的变量
    /*
    varDef://int a[1][2][3]... = {...}
    Ident ('[' constExp ']')* |
    Ident ('[' constExp ']')* '=' initVal;
     */
    @Override
    public Void visitVarDef(sysyParser.VarDefContext ctx) {
        //读取变量ident，在末尾用到
        visit(ctx.children.get(0));
        String variName = retStr;
        //无论是定义还是赋值，都需要先知道维度类型，故提取到if外面
        boolean pastneedCompute = needComputeNow;
        needComputeNow = true;
        ArrayList<Integer> dimension = parseDimensionByConstExps(ctx.constExp());
        needComputeNow = pastneedCompute;
        Type typeNow = retType;
        for (int i = dimension.size() - 1; i >= 0; i--) {
            int num = dimension.get(i);
            typeNow = new ArrayType(num, typeNow);
        }
        if (needDeclaration || needComputeNow) {
            //retv:variable
            //解析声明，保存到retv，无论是needDeclaration或者needcomputeNow都说明需要保存这个新的variable
            retv = new Variable(this.module, typeNow, variName);
            ((Variable)retv).isInit =false;
        } else {
            //retv:alloca指令
            //其他情况，variableStack中应当已有，直接获取对应alloca指令。
            retv = variableStack.get(variName);
        }
//        Alloca allocaInst = buildAlloca(variName, typeNow,
//                blockNow, getTopDispatcher());
//        ((Variable) retv).lastInst = allocaInst; // 设置最后一条指令
        //加入表达式
        if (needComputeNow) {//这种情况，说明需要马上计算出来初始化的结果
            ((Variable) retv).construct();
            Type innertype = retv.type.innerType();
            if (innertype instanceof IntType) {
                container = 0;
            } else {
                container = (double) 0;
            }
            //container只是暂时容器，数据保存在retv.value中。
            if (ctx.initVal() != null) {//此时，retv中的变量代表了当前初始化对应的变量，故所有计算出的值保留在其中。
                ((Variable) retv).isInit = true;
                visitInitVal(ctx.initVal());
            }
        } else if (!needDeclaration && ctx.initVal() != null) {//不需要定义、存在初始化，说明需要赋值
            //在该轮visit中需要进行赋值
            //这时，typeNow保存着该变量对应的类型
            //又由于此时已经在解析函数内部，故我们需要的并不是const，而是一个Value，无论这个Value是Number还是Inst
            //同样，也只能是Number或者Inst，有可能是：ConstNumber、Load、二元语句结果。
            //需要考虑type，如果返回的是非32位，如是boolean的i1类型，则需要扩展到32位int/float。
            //需要考虑数组情况，此时应逐个赋值，如int a[2]={b+c[0],c[1]}需要分别计算出两个元素，并将计算的%x=...inst
            // Value放到container中。
            container = new Value();//仅代表期望返回value，没有任何内容
            if (ctx.constExp().size() == 0) { //处理非数组初始化
                if (ctx.initVal().exp() == null) {//初始化initval里面没有exp，说明是数组初始化值，错误
                    ErrOutput.outputErr("error: use un-array value to init array");
                }
                visitInitVal(ctx.initVal(), 0);
                Value v = TypeCaster.generateCastNumTypeInstr(
                    //检测处理是否需要隐式转换，包括int->float和float->int
                    (Value) container, ((Value) container).type,
                    typeNow, getTopDispatcher(), getTopBlock()
                );
                if (v != null) {
                    getTopBlock().addValue(v);
                    container = v;
                }
                buildAssign(((Variable) retv).allocInst, (Value) container, getTopBlock(),
                    getTopDispatcher());
            } else { //数组初始化
                if (ctx.initVal().exp() != null) {//初始化initval里有exp，说明不是数组初始化。错误
                    ErrOutput.outputErr("error: use un-array to initialize array");
                }
                if (!isGlobal) {
                    Bitcast bitcast =
                        buildBitcast(new Pointer(new IntType()), ((Variable) retv).allocInst,
                            getTopBlock(), getTopDispatcher());
                    getTopBlock().addValue(bitcast);
                    buildMemset(bitcast, new ConstNumber(0), typeNow,
                        getTopBlock(), module.getFunction(new String("memset")),
                        getTopDispatcher());
                }
                visitInitVal(ctx.initVal(), 1);
            }
        }
        return null;
    }

    @Override
    public Void visitInitVal(sysyParser.InitValContext ctx) {
        visitInitVal(ctx, 1);
        return null;
    }

    /*
    initVal: //{a, {b,c, ...}, ...}
    exp|
    '{' ( initVal (',' initVal)*)? '}';
     */
    public Void visitInitVal(sysyParser.InitValContext ctx, int mode) {
        ((Variable) retv).posNext();
        if (ctx.exp() != null) {
            //对应初始化值的一个数，调用retv的saveValue
            visit((ctx.exp()));
            if (mode == 0) {
                return null;
            }
            if (container instanceof Value) {
                ((Variable) retv).preparePos();
                ArrayList<Integer> pos = new ArrayList<>(((Variable) retv).currentInitPos);
                pos.add(0, 0);//加入第一维度，消除最外层pointer。
                LinkedList<Value> uses = new LinkedList<>();
                LinkedList<Value> finalUses = uses;
                pos.forEach(i -> finalUses.add(new ConstNumber(i)));
                uses.add(0, ((Variable) retv).allocInst);//第零个代表指针变量
                GetElementPtrInst ptr = new GetElementPtrInst(
                    uses, new Pointer(TypeCaster.realType(retv.type).innerType()),
                    getTopDispatcher().allocId(), getTopBlock()
                );
                getTopBlock().addValue(ptr);
                //构建用得到的指针store的指令----------------------------------------------------------
                uses = new LinkedList<>();
                uses.add(ptr);
                buildAssign(ptr, (Value) container, getTopBlock(), getTopDispatcher());
                return null;
            } else {
                ((Variable) retv).saveValue(container);
            }
        } else {
            ((Variable) retv).posIn();
            for (var sub : ctx.initVal()) {
                visit(sub);
            }
            ((Variable) retv).posOut();
        }
        return null;
    }

    /*
    constDef://常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
    Ident ('[' constExp ']')* '=' constInitVal;
     */
    @Override
    public Void visitConstDef(sysyParser.ConstDefContext ctx) {
        visit(ctx.children.get(0));
        String varName = retStr;
        ArrayList<Integer> dimension = new ArrayList<>();
        for (var sub : ctx.constExp()) {
            container = 0;
            visit(sub);
            dimension.add((Integer) container);
        }
        Type typeNow = retType;
        for (int i = dimension.size() - 1; i >= 0; i--) {
            int num = dimension.get(i);
            typeNow = new ArrayType(num, typeNow);
        }
        retv = new ConstVariable(this.module, typeNow, varName, ctx.constInitVal() != null);
        ((Variable) retv).construct();//构造初始化数据容器
        Type innertype = retv.type.innerType();
        if (innertype instanceof IntType) {
            container = 0;
        } else {
            container = (double) 0;
        }
        boolean pastNeedCompute = needComputeNow;
        needComputeNow = true;
        if (ctx.constInitVal() != null) {
            visitConstInitVal(ctx.constInitVal());
        }
        needComputeNow = pastNeedCompute;
        return null;
    }


    //进入时retv一定是当前等待保存初始化值的变量。对于const的init，均可以初始化。
    @Override
    public Void visitConstInitVal(sysyParser.ConstInitValContext ctx) {
        ((Variable) retv).posNext();
        if (ctx.constExp() != null) {
            //对应初始化值的一个数，调用retv的saveValue
            visitConstExp((ctx.constExp()));
            ((Variable) retv).saveValue(container);
        } else {
            ((Variable) retv).posIn();
            for (var sub : ctx.constInitVal()) {
                visitConstInitVal(sub);
            }
            ((Variable) retv).posOut();
        }
        return null;
    }

    //终结符，直接返回字符串
    @Override
    public Void visitTerminal(TerminalNode node) {
        retStr = node.toString();
        return null;
    }

    //常量表达式，只是visit，期望返回类型由上级设置，需要设置needconst。
    /*
    constExp:
    addExp;
     */
    @Override
    public Void visitConstExp(sysyParser.ConstExpContext ctx) {
        needConst = true;
        visitAddExp(ctx.addExp());
        needConst = false;
        return null;
    }

    //仅完成常量
    //同时处理加减操作
    /*
    addExp:
    mulExp | addExp (Add | Sub) mulExp;
     */
    @Override
    public Void visitAddExp(sysyParser.AddExpContext ctx) {
        //当需要const返回值时，意味着到这一步，上层要么需要int，要么需要float，而如果container代表的需求不是这二者，
        // 说明出现了int a[1] = 0这样的错误
        if (ctx.children.size() > 1) {

            LinkedList<sysyParser.MulExpContext> muls = new LinkedList<>();
            LinkedList<Boolean> isAdds = new LinkedList<>();
            boolean alladd = true;
            while (ctx.children.size() > 1) {//先把所有muls都加进来
                muls.add((sysyParser.MulExpContext) ctx.children.get(2));
                isAdds.add(ctx.children.get(1).toString().equals("+"));
                alladd = alladd && ctx.children.get(1).toString().equals("+");
                ctx = (sysyParser.AddExpContext) ctx.children.get(0);
            }
            muls.add((sysyParser.MulExpContext) ctx.children.get(0));

            alladd = false;


            visitMulExp(alladd ? muls.removeFirst() : muls.removeLast());
            while (!muls.isEmpty()) {
                Object tmp = container;
                visit(alladd ? muls.removeFirst() : muls.removeLast());
                if(alladd){
                    Object tmp2 = tmp;
                    tmp = container;
                    container = tmp2;
                }
                boolean isAdd = alladd ? isAdds.removeFirst():isAdds.removeLast();
                if (container instanceof Integer) {
                    container = isAdd ? (Integer) tmp + (Integer) container :
                        (Integer) tmp - (Integer) container;
                } else if (container instanceof Double) {
                    container = isAdd ? (Double) tmp + (Double) container :
                        (Double) tmp - (Double) container;
                } else if (container instanceof Value) {
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add((Value) tmp);
                    uses.add((Value) container);
                    Instr instr = BinaryInstGenerator.genBinaryComputeInst(
                        uses, getTopDispatcher(),
                        isAdd ? InstType.BinaryType.add : InstType.BinaryType.sub,
                        getTopBlock()
                    );
                    getTopBlock().addValue(instr);
                    container = instr;
                    //供上一级用
                }
            }
        } else {
            visit(ctx.children.get(0));
        }

        return null;
    }

    /*
    mulExp:
    unaryExp | mulExp (Mul | Div | Mod) unaryExp;
     */
    @Override
    public Void visitMulExp(sysyParser.MulExpContext ctx) {
        if (ctx.children.size() == 1) {
            visit(ctx.children.get(0));
        } else {
            Stack<Boolean> isMuls = new Stack<>();
            Stack<Boolean> isDivs = new Stack<>();
            Stack<sysyParser.UnaryExpContext> unarys = new Stack<>();
            while (ctx.children.size() > 1) {
                unarys.add((sysyParser.UnaryExpContext) ctx.children.get(2));
                isMuls.add(ctx.children.get(1).toString().equals("*"));
                isDivs.add(ctx.children.get(1).toString().equals("/"));
                ctx = (sysyParser.MulExpContext) ctx.children.get(0);
            }
            unarys.add((sysyParser.UnaryExpContext) ctx.children.get(0));
            visitUnaryExp(unarys.pop());
            while (!unarys.isEmpty()) {
                Object tmp = container;
                visit(unarys.pop());
                boolean ismul = isMuls.pop();
                boolean isdiv = isDivs.pop();
                if (container instanceof Integer) {
                    if ((Integer) container == 0) {
                        ErrOutput.outputErr("error: div 0 when computing int");
                    }
                    container = ismul ? (Integer) tmp * (Integer) container :
                        isdiv ? (Integer) tmp / (Integer) container :
                            (Integer) tmp % (Integer) container;
                } else if (container instanceof Double) {
                    if (!(ismul || isdiv)) {
                        ErrOutput.outputErr("mod with double");
                    }
                    container = ismul ? (Double) tmp * (Double) container :
                        (Double) tmp / (Double) container;
                } else if (container instanceof Value) {
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add((Value) tmp);
                    uses.add((Value) container);
                    Instr instr = BinaryInstGenerator.genBinaryComputeInst(
                        uses, getTopDispatcher(),
                        ismul ? InstType.BinaryType.mul : isdiv ? InstType.BinaryType.sdiv :
                            InstType.BinaryType.srem,
                        getTopBlock()
                    );
                    getTopBlock().addValue(instr);
                    container = instr;
                }
            }
        }
        return null;
    }

    /*
    unaryExp:
    primaryExp |
    Ident '(' (funcRParams)?')' |
    unaryOp unaryExp;
     */
    @Override
    public Void visitUnaryExp(sysyParser.UnaryExpContext ctx) {
        if (ctx.primaryExp() == null && ctx.unaryOp() == null) {
            if (needComputeNow) {//需要及时计算，则funccall不符合预期，报错
                ErrOutput.outputErr("error: invalid non-const value");
            } else {
                visitFuncCall(ctx);
            }
        } else if (ctx.children.size() == 1) {
            visitPrimaryExp((sysyParser.PrimaryExpContext) ctx.children.get(0));// 下一个visit的是primaryExp
        } else {
            visit(ctx.children.get(0));
            String unaryOp = retStr;
            if (unaryOp.equals("+")) {
                visit(ctx.children.get(1));
            } else if (unaryOp.equals("!")) {
                if (container instanceof Integer || container instanceof Double) {
                    visit(ctx.children.get(1));
                    if (container instanceof Integer) {
                        container =
                            (Integer) container == 0 ? (double) 1 : (double) 0;
                    } else {
                        container = (Double) container == 0 ? (Integer) 1 : (Integer) 0;
                    }
                } else if (container instanceof Value) {
                    visit(ctx.children.get(1));
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add((Value) container);
                    uses.add(new ConstNumber(0));
                    Instr instr = BinaryInstGenerator.genBinaryCmpInst(
                        uses, getTopDispatcher(), InstType.CmpType.eq, getTopBlock()
                    );
                    getTopBlock().addValue(instr);
                    container = instr;
                }
            } else {    // '-'号
                if (container instanceof Integer || container instanceof Double) {
                    visit(ctx.children.get(1));
                    if (container instanceof Integer) {
                        container = ((Integer) container).intValue() * -1;
                    } else {
                        container = ((Double) container).doubleValue() * -1;
                    }
                } else {
                    visit(ctx.children.get(1));
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add((Value) container);
                    uses.add(new ConstNumber(-1));
                    Instr instr = BinaryInstGenerator.genBinaryComputeInst(
                        uses, getTopDispatcher(), InstType.BinaryType.mul, getTopBlock()
                    );
                    getTopBlock().addValue(instr);
                    container = instr;
                }
            }
        }
        return null;
    }

    /*
    unaryExp:
    primaryExp |
    Ident '(' (funcRParams)?')' |
    unaryOp unaryExp;
     */
    private Value visitFuncCall(sysyParser.UnaryExpContext ctx) {
        assert ctx.Ident() != null;
        Function function = module.getFunction(ctx.Ident().getText());
        LinkedList<Value> args = new LinkedList<>();
        if(function == null){
            function = module.getFunction("_sysy_"+ctx.Ident().getText());
            if(function == null){
                ErrOutput.outputErr("error: can't find function "+ctx.Ident().getText());
            }
            args.add(new ConstNumber(0));
        }

        if (ctx.funcRParams() != null) {
            visitFuncRParams(ctx.funcRParams(), args, function);
        }

        Call call = buildCall(function, args, getTopBlock(), getTopDispatcher());
        container = call;
        getTopBlock().addValue(call);
        return call;
    }

    @Override
    public Void visitFuncRParams(sysyParser.FuncRParamsContext ctx) {
        return super.visitFuncRParams(ctx);
    }

    //visitExp and add the retv to the args used to call
    /*
    funcRParams:
    exp (',' exp)*;
     */
    public void visitFuncRParams(sysyParser.FuncRParamsContext ctx,
                                 LinkedList<Value> args, Function function) {
        int cnt = 0;
        for (var sub : ctx.exp()) {
            if (cnt > function.getArgs().size()) {
                ErrOutput.outputErr("error: too Many arguments for " + function.name);
            }
            // 标记期望类型为Value
            container = new Value();
            visit(sub);
            Value instr = TypeCaster.generateCastNumTypeInstrTry((Value) container,
                ((Value) container).getType(), function.getArgs().get(cnt), getTopDispatcher(),
                getTopBlock());
            if (instr != null) {
                getTopBlock().addValue(instr);
                container = instr;
            }
            args.add((Value) container);
            if (!((Value) container).type.equals(function.getArgs().get(cnt))) {
                ErrOutput.outputErr(
                    "error: " + cnt + " argument is of fault type in " + function.name);
            }
            cnt++;
        }
    }

    @Override
    /*
    primaryExp:
        '(' exp ')' |
        lVal|
        number;
     */
    public Void visitPrimaryExp(sysyParser.PrimaryExpContext ctx) { //是(exp), lval, number之一。
        if (!(container instanceof Value)) {
            if (ctx.exp() != null) {
                //是( exp )的情况, 直接visit
                visitExp(ctx.exp());
            } else if (ctx.number() != null) {
                //是number的情况
                visitNumber(ctx.number());
            } else {
                visitLVal(ctx.lVal());
            }
        } else {
            if (ctx.exp() != null) {
                visitExp(ctx.exp()); // (exp), visit exp即可
            } else if (ctx.number() != null) { //number, 直接读即可
                visitNumber(ctx.number());
            } else { //lval 有点难度，交给visitlVal，相信这个函数能完成。
                visitLVal(ctx.lVal());
                if(container instanceof Integer || container instanceof Float
                    || container instanceof ConstNumber) {
                    return null;
                }
                if (((Value) container).type instanceof Pointer) {
                    if (((Pointer) ((Value) container).type).pointTo instanceof ArrayType) {
                        LinkedList<Value> uses = new LinkedList<>();
                        uses.add((Value) container);
                        uses.add(new ConstNumber(0));
                        uses.add(new ConstNumber(0));
                        Instr getptr = new GetElementPtrInst(uses,
                            new Pointer(
                                ((ArrayType) ((Pointer) ((Value) container).type).pointTo).t),
                            getTopDispatcher().allocId(), getTopBlock());
                        getTopBlock().addValue(getptr);
                        container = getptr;
                        return null;
                    }
                }
                LinkedList<Value> uses = new LinkedList<>();
                uses.add((Value) container);
                Load load = new Load(uses, getTopDispatcher().allocId(), getTopBlock());
                getTopBlock().addValue(load);
                container = load;
            }
        }
        return null;
    }

    /*
    lVal:
        Ident ('[' exp ']')*;
     */
    @Override
    public Void visitLVal(sysyParser.LValContext ctx) {
        visit(ctx.children.get(0));//visit ident and save to retStr
        String ident = retStr;
        Variable v;
        if (needComputeNow) {
            int priv = variableStack.getKeyPriority(ident);
            int pricv  = constStack.getKeyPriority(ident);
            if (priv>pricv) {
                v = (Variable) variableStack.get(ident);
            } else {
                if (pricv==-1) {
                    ErrOutput.outputErr("error: there is no ident named: " + ident);
                }
                v = (Variable) constStack.get(ident);
            }
            Object containerPast = container;
            ArrayList<Integer> dim = new ArrayList<>();
            for (var sub : ctx.exp()) {
                container = 1;
                visit(sub);
                dim.add((Integer) container);
            }
            container = containerPast;
            if (dim.size() > 0) {
                if (!(v.type instanceof ArrayType)) {
                    ErrOutput.outputErr("error: trying to use index for un-array");
                }
                if (!((ArrayType) v.type).isValidPos(dim)) {
                    ErrOutput.outputErr("error: wrong index for array");
                }
                ArrayList<Object> targetarr = v.getArrayListByPos(dim);
                int lastIndex = dim.get(dim.size() - 1);
                if (lastIndex >= targetarr.size()) {
                    ErrOutput.outputErr("error: index out of range");
                }
                Object targetObject = targetarr.get(lastIndex);
                if (!(targetObject instanceof Integer || targetObject instanceof Double)) {
                    ErrOutput.outputErr("error: wrong type of arrly elem");
                }
                if (container instanceof Integer) {
                    if (targetObject instanceof Integer) {
                        container = targetObject;
                    } else {
                        assert targetObject instanceof Double;
                        double tmp = (Double) targetObject;
                        container = (int) tmp;
                    }
                } else {
                    container = Double.parseDouble(targetObject.toString());
                }
            } else {
                if (v.type instanceof ArrayType) {
                    ErrOutput.outputErr("error: wrong type");
                }
                if (container instanceof Integer) {
                    if (v.type instanceof IntType) {
                        container = v.value;
                    } else {
                        container = (int) (double) (Double) v.value;
                    }
                } else if (container instanceof Double) {
                    if (v.type instanceof IntType) {
                        container = 1.0 * (Integer)v.value;
                    } else {
                        container = v.value;
                    }
                }
            }
        } else {
            int priv = variableStack.getKeyPriority(ident);
            int pricv  = constStack.getKeyPriority(ident);

            if (priv>pricv) {
                v = (Variable) variableStack.get(ident);
            } else {
                if (pricv==-1) {
                    ErrOutput.outputErr("error: there is no ident named: " + ident);
                }
                v = (Variable) constStack.get(ident);
                if(!(v.type instanceof ArrayType)) {
                    if(container instanceof Double) {
                        if (v.type instanceof IntType) {
                            container = 1.0 * (Integer)v.value;
                        } else {
                            container = v.value;
                        }
                    } else if(container instanceof Integer) {
                        if (v.type instanceof IntType) {
                            container = v.value;
                        } else {
                            container = (int) (double) (Double) v.value;
                        }
                    } else if(container instanceof Value) {
                        if(v.type instanceof FloatType) {
                            container = new ConstNumber((Double) v.value);
                        } else if(v.type instanceof IntType) {
                            container = new ConstNumber((Integer) v.value);
                        }
                    }
                    return null;
                }
            }
            ArrayList<Value> dimension = new ArrayList<>();
            if (!(container instanceof Value)) {
                ErrOutput.outputErr("error: try to get lval but container is not value");
            }
            //增加指针作为左值的情况
            if (ctx.exp().size() == 0) {
                /*if (TypeCaster.realType(v.type) instanceof ArrayType) {//如果是指针，提取指针对应的sysy变量类型
                    ErrOutput.outputErr("error: wrong index, try to use pointer");
                    return null;
                }
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(v.allocInst);
                Load instr = new Load(uses, getTopDispatcher().allocId(), getTopBlock());
                getTopBlock().addValue(instr);*/
                container = v.allocInst;
            } else {
                if (!(TypeCaster.realType(v.type) instanceof ArrayType)) {
                    ErrOutput.outputErr("error: try to use index on un-array variable");
                    return null;
                }
                for (var sub : ctx.exp()) {
                    visit(sub);
                    dimension.add((Value) container);
                }
                if (dimension.size() > ((ArrayType) TypeCaster.realType(v.type)).getDim()) {
                    ErrOutput.outputErr("error: wrong dimension");
                }
                //构建获取指针的指令---------------------------------------------------------------
                //如果是形参，构建获取原始形参的指令
                Instr targetinst;
                if (v.type instanceof Pointer) {
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add(v.allocInst);
                    targetinst = new Load(uses, getTopDispatcher().allocId(), getTopBlock());
                    getTopBlock().addValue(targetinst);
                } else {
                    targetinst = v.allocInst;
                }
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(targetinst);
                if (!(v.type instanceof Pointer)) {
                    //如果是数组形参，不需要添加维度,否则需要添加一个维度下标。
                    uses.add(new ConstNumber(0));
                }
                uses.addAll(dimension);
                GetElementPtrInst ptr = new GetElementPtrInst(
                    uses, new Pointer(TypeCaster.realType(v.type).innerType(dimension.size())),
                    getTopDispatcher().allocId(), getTopBlock()
                );
                getTopBlock().addValue(ptr);
                //构建用得到的指针load的指令----------------------------------------------------------
                /*uses = new LinkedList<>();
                uses.add(ptr);
                Load load = new Load(uses, getTopDispatcher().allocId(), getTopBlock());
                getTopBlock().addValue(load);*/
                //返回load出来的指令
                container = ptr;
            }
        }
        return null;
    }

    @Override
    public Void visitExp(sysyParser.ExpContext ctx) {
        visitAddExp(ctx.addExp());
        return null;
    }

    //仅完成int情况的十进制子情况
    @Override
    public Void visitNumber(sysyParser.NumberContext ctx) {
        return super.visitNumber(ctx);
    }


    // parse8 10 16进制int数
    @Override
    public Void visitIntConst(sysyParser.IntConstContext ctx) {
        //sysy的float可以用整数初始化，但是整数不能用浮点数初始化
        //这是因为sysy可以int隐式转为float，但是不允许反向转换
        //如果container希望得到一个Value，则构造一个继承Value的ConstNumber
        int temp;
        String num = ctx.children.get(0).toString();
        if (ctx.getText().length() >= 2 && ctx.HexConst() != null) {
            temp = Integer.parseInt(num.substring(2), 16);
        } else if (ctx.getText().length() >= 1 && ctx.OctalConst() != null) {
            temp = Integer.parseInt(num.substring(1), 8);
        } else {
            temp = Integer.parseInt(num, 10);
        }

        if (container instanceof Integer) {
            container = temp;
        } else if (container instanceof Double) {
            container = (double) temp;
        } else if (container instanceof Value) {
            container = new ConstNumber(temp);
        }
        return null;
    }

    @Override
    public Void visitFloatConst(sysyParser.FloatConstContext ctx) {
        double temp = (float)Float.parseFloat(ctx.getText());
        if (container instanceof Double) {
            container = temp;
        } else if (container instanceof Integer) {
            container = (int) Double.parseDouble(ctx.getText());
        } else if (container instanceof Value) {
            container = new ConstNumber(temp);
        } else {
            ErrOutput.outputErr("error: un supported container of float");
        }
        return null;
    }

    /*
    funcDef://void fname ( int a[][exp], int b[] ...){ ... }
    funcType Ident '(' (funcFParams)? ')' block;
     */
    //函数调用
    @Override
    public Void visitFuncDef(sysyParser.FuncDefContext ctx) {
        visit(ctx.funcType());
        visit(ctx.children.get(1));
        String funcName = retStr;
        Type funcType = retType;
        //这里的function的User需要设置 可能也需要设置一个栈或者保存一下 用于恢复到上一个user
        IrRegDispatcher irRegDispatcher = new IrRegDispatcher();
        dispatcherStack.add(irRegDispatcher);
        function = new Function(this.module, funcName,
            funcType, irRegDispatcher);
        module.addFunction(retStr, function);


        //use topBlock to deal the function paras
        BasicBlock topBlock = new BasicBlock(funcName + ".entry", function, false);
        funcEntryBlock = topBlock;
        //----------------------------------------------------------------函数外层套了一层变量栈
        LinkedHashMap<String, Value> blockConstVar = new LinkedHashMap<>();
        LinkedHashMap<String, Value> blockVar = new LinkedHashMap<>();
        constStack.push(blockConstVar);
        variableStack.push(blockVar);

        //处理参数
        if (ctx.funcFParams() != null) {
            visitFuncFParams(ctx.funcFParams(), topBlock);
        }

        BasicBlock exitBlock = buildNewBlock(funcName + ".exit",
            function, false, false, true);

        BasicBlock mainBlock = buildNewBlock(funcName + ".main",
            function, true, true, false);
        function.addBlock(topBlock);
        function.addBlock(getTopBlock());
        function.addBlock(exitBlock);

        // 建立返回值
        if (!(funcType instanceof VoidType)) {
            Value var = buildAlloca(funcName + ".ret", funcType, topBlock, getTopDispatcher());
            buildAssign(var, new ConstNumber(0), topBlock, getTopDispatcher());
            function.setReturnValue(var);
        }
        //处理函数体
        visitBlock(ctx.block());

        //函数exitblockbuild--------------------------------
        LinkedList<Value> uses;
        Value loadRet;
        if (!(funcType instanceof VoidType)) {
            uses = new LinkedList<Value>();
            uses.add(function.returnValue);
            loadRet = new Load(uses, getTopDispatcher().allocId(), exitBlock);
            exitBlock.addValue(loadRet);
        } else {
            loadRet = new VoidValue();
        }

        uses = new LinkedList<>();
        uses.add(loadRet);
        exitBlock.addValue(new Ret(uses, exitBlock));
        //end----------------------------------------------
        if (!topBlock.isEnd) {
            topBlock.addValue(buildBr(null, mainBlock, null, null));
        }
        if (!mainBlock.isEnd) {
            mainBlock.addValue(buildBr(null, exitBlock, null, null));
        }

        popBlock(true);
        dispatcherStack.remove(
            dispatcherStack.size() - 1);// only allows the function to use the dispatcherStack
        constStack.pop();
        variableStack.pop();
        return null;
    }

    /*
    funcType:
    Void | Int | Float;
     */
    @Override
    public Void visitFuncType(sysyParser.FuncTypeContext ctx) {
        switch (ctx.children.get(0).toString()) {
            case "int" -> retType = new IntType();
            case "float" -> retType = new FloatType();
            case "void" -> retType = new VoidType();
            default -> retType = null;
        }
        return null;
    }

    @Override
    public Void visitFuncFParams(sysyParser.FuncFParamsContext ctx) {
        return null;
    }

    /*
    funcFParams://int a, int b, ...
    funcFParam (',' funcFParam)*;
     */
    public Void visitFuncFParams(sysyParser.FuncFParamsContext ctx, BasicBlock topBlock) {
        int cnt = 0;
        for (var sub : ctx.funcFParam()) {
            visitFuncFParam(sub, topBlock, cnt++);
        }
        return null;
    }

    /*
    funcFParam: //int a[][exp][exp]...
    bType Ident (
        '[' ']' //第一维大小不能指定，与c语言相似
        (
            '[' exp ']'
        )*
    )?;
     */
    @Override
    public Void visitFuncFParam(sysyParser.FuncFParamContext ctx) {
        return null;
    }

    public Void visitFuncFParam(sysyParser.FuncFParamContext ctx,
                                BasicBlock entranceBlock, int argNo) {
        visitBType(ctx.bType());
        visit(ctx.children.get(1));
        String variName = retStr;
        //开始识别维度
        needComputeNow = true;
        ArrayList<Integer> dimension = parseDimensionByConstExps(ctx.constExp());
        needComputeNow = false;
        //忽略第一维 Dimension.add(0,0);
        Type typeNow = retType;
        for (int i = dimension.size() - 1; i >= 0; i--) {
            int num = dimension.get(i);
            typeNow = new ArrayType(num, typeNow);
        }

        //设置类型为pointer类型，表示对应数组形参
        if (typeNow instanceof ArrayType) {
            typeNow = new Pointer(typeNow);
        } else if (typeNow instanceof IntType && ctx.getText().contains("[")) {
            typeNow = new Pointer(new IntType());
        } else if (typeNow instanceof FloatType && ctx.getText().contains("[")) {
            typeNow = new Pointer(new FloatType());
        }
        //将Argument视为function刚开局定义的变量 第i个变量 由%i赋值
        Variable arg = new Variable(function, typeNow, variName);
        //占用对应的标识符
        getTopDispatcher().allocId();
        Alloca var = buildAlloca(variName, typeNow, entranceBlock, getTopDispatcher());
        Arg arg1 = new Arg(Integer.toString(argNo), typeNow);
        Store store = new Store(var, arg1, entranceBlock);
        entranceBlock.addValue(store);
        arg.allocInst = var;
        variableStack.addValue(variName, arg);
        function.addArg(typeNow, arg, arg1);

        return null;
    }

    @Override
    public Void visitBlock(sysyParser.BlockContext ctx) {
        for (var sub : ctx.blockItem()) {
            if(getTopBlock().isEnd){
                return null;
            }
            visitBlockItem(sub);
        }
        return null;
    }

    public Void visitBlockItem(sysyParser.BlockItemContext ctx) {
        if (ctx.decl() != null) {//定义的赋值阶段也需要在此时完成，故无论是不是declMode都需要visit
            needDeclaration = true;
            visitDecl(ctx.decl());
            needDeclaration = false;
            visitDecl(ctx.decl());
        } else if (ctx.stmt() != null) {
            visitStmt(ctx.stmt());
        }
        return null;
    }

    /*
    stmt:
    lVal '=' exp End|
    (exp)? End |
    block |
    If '(' cond ')' stmt (Else stmt)? |
    While '(' cond ')' stmt |
    Break End |
    Continue End |
    Return (exp)? End;
    */
    //setRetv as a new BasicBlock
    @Override
    public Void visitStmt(sysyParser.StmtContext ctx) {
        if (getTopBlock().isEnd) {
            return null;
        }
        BasicBlock block;
        if (ctx.lVal() != null) {
            block = buildNewBlock("tempBlock", function, false, true, false);
            container = new Value();
            visitLVal(ctx.lVal());
            Value dest = (Value) container;
            container = new Value();
            visitExp(ctx.exp());
            Value val = (Value) container;
            buildAssign(dest, val, block, getTopDispatcher());
            popBlock(false);

            getTopBlock().merge(block);
        } else if (ctx.block() != null) {
            block = buildNewBlock("Block" + getTopDispatcher().allocId(),
                function, true, true, false);
            visitBlock(ctx.block());
            popBlock(true);

            getTopBlock().merge(block);
        } else if (ctx.If() != null) {
//            buildNewBlock("if"+getTopDispatcher().allocId(),function,false, true, false);
            buildNewBlock("Cond" + getTopDispatcher().allocId(),
                function, false, true, false);//加入一个块到stack

            //开始设置trueblock和falseblock，此时topblock仍然是condblock
            //trueblock:一定存在
            BasicBlock blockTrue = new BasicBlock("tmp", function, true);
            this.trueTargetBlock = blockTrue;
            //falseblock:取决于是否有else，没有则是endblock
            BasicBlock blockFalse = null;
            if (ctx.Else() != null) {
                blockFalse = new BasicBlock("tmp", function, true);
            }
            BasicBlock blockEnd = new BasicBlock("tmp", function, true);
            this.falseTargetBlock = blockFalse == null ? blockEnd : blockFalse;
            //开始用condblock来visitCond
            visitCond(ctx.cond());
            Value value = (Value) container;
            BasicBlock tmp = popBlock(false);
            getTopBlock().addValue(tmp);

            //设置trueblock的name
            blockTrue.name = getTopDispatcher().allocId();
            //使用Default模式的allocId 因为这个label i 同样也表示%i
            blockTrue = buildNewBlock(blockTrue, function,
                false, true, true);
            visitStmt(ctx.stmt(0));
            popBlock(false);

            //build BlockFalse
            if (blockFalse != null) {
                blockFalse.name = getTopDispatcher().allocId();
                blockFalse = buildNewBlock(blockFalse, function,
                    false, true, true);
                visitStmt(ctx.stmt(1));
                popBlock(false);
            }

            //作为结尾跳转空间
            blockEnd.name = getTopDispatcher().allocId();
            if (!blockTrue.isEnd) {
                Br brEnd = buildBr(null, blockEnd, null, getTopBlock());
                blockTrue.addValue(brEnd);
            }
            if (blockFalse != null && !blockFalse.isEnd) {
                Br brEnd2 = buildBr(null, blockEnd, null, getTopBlock());
                blockFalse.addValue(brEnd2);
            }
            getTopBlock().addValue(blockTrue);
            if (blockFalse != null) {
                getTopBlock().addValue(blockFalse);
            }
            getTopBlock().addValue(blockEnd);
        } else if (ctx.While() != null) {
            BasicBlock mainLoop = buildNewBlock("tmp",
                function, false, false, true);
            BasicBlock loopEnd = buildNewBlock("tmp", function,
                false, false, true);
            this.trueTargetBlock = mainLoop;
            this.falseTargetBlock = loopEnd;
            //build BlockCond
            BasicBlock blockCond = buildNewBlock(getTopDispatcher().allocId(),
                function, false, true, true);
            blockCond.isLoopHead = true;
            visitCond(ctx.cond());
            BasicBlock tmp = popBlock(false);
            getTopBlock().addValue(tmp);

            loopStartStack.add(blockCond);
            loopEndStack.add(loopEnd);

            //处理Loop中央stmt
            mainLoop.name = getTopDispatcher().allocId();
            mainLoop = buildNewBlock(mainLoop,
                function, false, true, true);
            visitStmt(ctx.stmt().get(0));
            popBlock(false);
            if(!mainLoop.isEnd)
                mainLoop.addValue(buildBr(null, blockCond, null, getTopBlock()));


            loopStartStack.remove(loopStartStack.size() - 1);
            loopEndStack.remove(loopEndStack.size() - 1);

            getTopBlock().addValue(mainLoop);
            loopEnd.name = getTopDispatcher().allocId();
            getTopBlock().addValue(loopEnd);
        } else if (ctx.Break() != null) { //通过设置Loop栈 我们能够找到上一个的End 或者Continue
            getTopBlock().addValue(buildBr(null,
                loopEndStack.get(loopEndStack.size() - 1), null, getTopBlock()));
            getTopBlock().isEnd = true;
        } else if (ctx.Continue() != null) {
            getTopBlock().addValue(buildBr(null,
                loopStartStack.get(loopStartStack.size() - 1), null, getTopBlock()));
            getTopBlock().isEnd = true;
        } else if (ctx.Return() != null) {
            getTopBlock().isEnd = true;
            container = new Value();
            if (ctx.exp() != null) {
                visitExp(ctx.exp());
                if (function.getRetType() instanceof ArrayType ||
                    function.getRetType() instanceof VoidType) {
                    ErrOutput.outputErr("error: wrong type of return");
                }
                if(container instanceof Double || container instanceof Integer) {
                    if(container instanceof Double) {
                        container = new ConstNumber((Double)container);
                    } else {
                        container = new ConstNumber((Integer) container);
                    }
                }
                if (!(((Value) container).type instanceof IntType ||
                    ((Value) container).type instanceof FloatType)) {
                    System.out.println(ctx.getText());
                    ErrOutput.outputErr("error: return type is not int or float in un-void func");
                }
                Value ins =
                    TypeCaster.generateCastNumTypeInstr((Value) container, ((Value) container).type,
                        function.getRetType(), getTopDispatcher(), getTopBlock());
                if (ins != null) {
                    getTopBlock().addValue(ins);
                    container = ins;
                }
                buildRet((Value) container, getTopBlock(), function);
            } else {
                if (!(function.getRetType() instanceof VoidType)) {
                    ErrOutput.outputErr("error: wrong type of return");
                }
                buildRet(new ConstNumber(0), getTopBlock(), function);
            }
        } else { //对应于第二种情况
            container = new Value();
            if (ctx.exp() != null) {
                visitExp(ctx.exp());
            }
        }
        return null;
    }

    @Override
    /*
    cond:
        lOrExp;
     */
    //container装最后一条指令 要求返回值为i1
    public Void visitCond(sysyParser.CondContext ctx) {
        container = new Value();
        visitLOrExp(ctx.lOrExp());
        // TODO: 2023/7/21 这里之下需要删掉

        return null;
    }

    /*
    lOrExp:
        lAndExp | lOrExp Or lAndExp;
     */
    @Override
    public Void visitLOrExp(sysyParser.LOrExpContext ctx) {
        if (ctx.lOrExp() != null) {
            BasicBlock blockFalse = new BasicBlock("tmp", function, true);
            BasicBlock lastBlockFalse = this.falseTargetBlock;
            this.falseTargetBlock = blockFalse;
            visitLOrExp(ctx.lOrExp());
            BasicBlock tmp = popBlock(false);
            getTopBlock().addValue(tmp);
            blockFalse.name = getTopDispatcher().allocId();
            buildNewBlock(blockFalse, function, false, true, true);
            this.falseTargetBlock = lastBlockFalse;
            visit(ctx.lAndExp());
        } else {
            visit(ctx.lAndExp());
        }
        return null;
    }

    /*
    lAndExp:
        eqExp | lAndExp And eqExp;
     */
    @Override
    public Void visitLAndExp(sysyParser.LAndExpContext ctx) {
        if (ctx.lAndExp() != null) {
            BasicBlock blockTrue = new BasicBlock("tmp", function, true);
            BasicBlock lastBlockTrue = this.trueTargetBlock;
            this.trueTargetBlock = blockTrue;
            visitLAndExp(ctx.lAndExp());
            BasicBlock tmp = popBlock(false);
            getTopBlock().addValue(tmp);
            blockTrue.name = getTopDispatcher().allocId();
            buildNewBlock(blockTrue, function, false, true, true);
            this.trueTargetBlock = lastBlockTrue;
            visit(ctx.eqExp());
        } else {
            visit(ctx.eqExp());
        }
        return null;
    }

    @Override
    /*
    eqExp:
        relExp | eqExp (Equals | NotEqual) relExp;
     */
    public Void visitEqExp(sysyParser.EqExpContext ctx) {
        Value preValue = null; // 保存前一个Value
        if (ctx.eqExp() != null) {
            visitEqExpSub(ctx.eqExp());
            preValue = (Value) container;
            visitRelExp(ctx.relExp());

            LinkedList<Value> temp = new LinkedList<>();
            temp.add(preValue);
            temp.add((Value) container);
            Instr inst1 = BinaryInstGenerator.genBinaryCmpInst(
                temp, getTopDispatcher(),
                ctx.NotEqual() != null ? InstType.CmpType.ne : InstType.CmpType.eq,
                getTopBlock());
            getTopBlock().addValue(inst1);
            container = inst1;
            Value result = (Value) container;
            Type resultType = result.getType();
            if (!(resultType instanceof IntType)) {
                ErrOutput.outputErr("error");
                return null;
            }
            if (((IntType) resultType).getLen() != 1) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add((Value) container);
                uses.add(new ConstNumber(0));
                Instr instr = BinaryInstGenerator.genBinaryCmpInst(
                    uses, getTopDispatcher(),
                    InstType.CmpType.ne, getTopBlock()
                );
                getTopBlock().addValue(instr);
                container = instr;
            }
            getTopBlock().addValue(
                buildBr((Value) container, this.trueTargetBlock, this.falseTargetBlock,
                    getTopBlock()));
        } else {
            visitRelExp(ctx.relExp());
            Value result = (Value) container;
            Type resultType = result.getType();
            if (!(resultType instanceof IntType) && !(resultType instanceof FloatType)) {
                ErrOutput.outputErr("error: rel return non-number");
                return null;
            }
            if (resultType instanceof IntType && ((IntType) resultType).getLen() != 1) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add((Value) container);
                uses.add(new ConstNumber(0));
                Instr instr = BinaryInstGenerator.genBinaryCmpInst(
                    uses, getTopDispatcher(), InstType.CmpType.ne, getTopBlock()
                );
                getTopBlock().addValue(instr);
                container = instr;
            } else if (resultType instanceof FloatType) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add((Value) container);
                uses.add(new ConstNumber((double) 0));
                Instr instr = BinaryInstGenerator.genBinaryCmpInst(
                    uses, getTopDispatcher(), InstType.CmpType.ne, getTopBlock()
                );
                getTopBlock().addValue(instr);
                container = instr;
            }
            getTopBlock().addValue(
                buildBr((Value) container, this.trueTargetBlock, this.falseTargetBlock,
                    getTopBlock()));
        }
        return null;
    }

    public Void visitEqExpSub(sysyParser.EqExpContext ctx) {
        if(ctx.eqExp()!=null){
            visitEqExpSub(ctx.eqExp());
            Value preValue = (Value) container;
            visitRelExp(ctx.relExp());

            LinkedList<Value> temp = new LinkedList<>();
            temp.add(preValue);
            temp.add((Value) container);
            Instr inst1 = BinaryInstGenerator.genBinaryCmpInst(
                temp, getTopDispatcher(),
                ctx.NotEqual() != null ? InstType.CmpType.ne : InstType.CmpType.eq,
                getTopBlock());
            getTopBlock().addValue(inst1);
            container = inst1;
        }else{
            visit(ctx.relExp());
        }
        return null;
    }

    @Override
    /*
    relExp:
        addExp | relExp (Less | Greater |LessEqual | GreaterEqual) addExp;
     */
    public Void visitRelExp(sysyParser.RelExpContext ctx) {
        Value preValue = null; // 保存前一个Value
        if (ctx.relExp() != null) {
            visitRelExp(ctx.relExp());
            preValue = (Value) container;
        }
        visitAddExp(ctx.addExp());
        if (preValue != null) {
            LinkedList<Value> temp = new LinkedList<>();
            temp.add(preValue);
            temp.add((Value) container);
            InstType.CmpType cmpType = null;
            if (ctx.Less() != null) {
                cmpType = InstType.CmpType.lt;
            } else if (ctx.Greater() != null) {
                cmpType = InstType.CmpType.gt;
            } else if (ctx.GreaterEqual() != null) {
                cmpType = InstType.CmpType.ge;
            } else if (ctx.LessEqual() != null) {
                cmpType = InstType.CmpType.le;
            } else {
                ErrOutput.outputErr("unknown relExp Token");
            }
            Instr inst1 = BinaryInstGenerator.genBinaryCmpInst(
                temp, getTopDispatcher(), cmpType, getTopBlock());
            getTopBlock().addValue(inst1);
            container = inst1;
        }
        return null;
    }

    public void pushBlock(BasicBlock block) {
        blockStack.add(block);
    }

    public BasicBlock getTopBlock() {
        return blockStack.get(blockStack.size() - 1);
    }

    public BasicBlock popBlock(boolean trueBlock) {
        BasicBlock ret = blockStack.get(blockStack.size() - 1);
        blockStack.remove(blockStack.size() - 1);
        if (trueBlock) {
            constStack.pop();
            variableStack.pop();
        }
        return ret;
    }

    //使用trueBlock来表明是实际意义上的Block 还是只是用来存储指令序列，继而影响是否需要压栈
    //intoStack 标记是否要把这个Block入栈，因为有些Block是不需要入栈的， haveLabel标记是否带有标签
    public BasicBlock buildNewBlock(String name, Function function,
                                    boolean trueBlock, boolean intoBlockStack, boolean haveLabel) {
        BasicBlock block = new BasicBlock(name, function, haveLabel);
        if (trueBlock) {
            LinkedHashMap<String, Value> blockConstVar = new LinkedHashMap<>();
            LinkedHashMap<String, Value> blockVar = new LinkedHashMap<>();
            // 这里设置的Stack同样也用于存储形参
            constStack.push(blockConstVar);
            variableStack.push(blockVar);
        }
        if (intoBlockStack) {
            pushBlock(block);
        }
        return block;
    }

    public BasicBlock buildNewBlock(BasicBlock block, Function function,
                                    boolean trueBlock, boolean intoBlockStack, boolean haveLabel) {
        if (trueBlock) {
            LinkedHashMap<String, Value> blockConstVar = new LinkedHashMap<>();
            LinkedHashMap<String, Value> blockVar = new LinkedHashMap<>();
            // 这里设置的Stack同样也用于存储形参
            constStack.push(blockConstVar);
            variableStack.push(blockVar);
        }
        if (intoBlockStack) {
            pushBlock(block);
        }
        return block;
    }
}