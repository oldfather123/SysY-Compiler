package cn.edu.bit.newnewcc.frontend;

import cn.edu.bit.newnewcc.frontend.antlr.SysYBaseVisitor;
import cn.edu.bit.newnewcc.frontend.antlr.SysYParser;
import cn.edu.bit.newnewcc.frontend.util.Constants;
import cn.edu.bit.newnewcc.frontend.util.Types;
import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.*;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.constant.VoidValue;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.tree.ParseTree;

import java.util.*;

public class Translator extends SysYBaseVisitor<Void> {
    private interface ControlFlowContext {
    }

    private static class WhileContext implements ControlFlowContext {
        BasicBlock testBlock;
        BasicBlock doneBlock;

        public WhileContext(BasicBlock testBlock, BasicBlock doneBlock) {
            this.testBlock = testBlock;
            this.doneBlock = doneBlock;
        }

        public BasicBlock getTestBlock() {
            return testBlock;
        }

        public BasicBlock getDoneBlock() {
            return doneBlock;
        }
    }

    private static class Parameter {
        Type type;
        String name;

        public Parameter(Type type, String name) {
            this.type = type;
            this.name = name;
        }

        public Type getType() {
            return type;
        }

        public String getName() {
            return name;
        }
    }

    private final SymbolTable symbolTable = new SymbolTable();
    private final Deque<ControlFlowContext> controlFlowStack = new LinkedList<>();
    private final Module module = new Module();

    private Function currentFunction;
    private BasicBlock currentBasicBlock;
    private Value result;
    private Value resultAddress;
    private BasicBlock currentTrueExit, currentFalseExit;

    public Module translate(ParseTree tree, List<ExternalFunction> externalFunctions) {
        for (ExternalFunction externalFunction : externalFunctions) {
            module.addExternalFunction(externalFunction);
            symbolTable.putFunction(externalFunction.getValueName(), externalFunction);
        }

        visit(tree);
        return module;
    }

    private Type makeType(Token token) {
        return switch (token.getType()) {
            case SysYParser.VOID -> VoidType.getInstance();
            case SysYParser.INT -> IntegerType.getI32();
            case SysYParser.FLOAT -> FloatType.getFloat();
            default -> throw new IllegalArgumentException();
        };
    }

    private Operator makeUnaryOperator(Token token) {
        return switch (token.getType()) {
            case SysYParser.ADD -> Operator.POS;
            case SysYParser.SUB -> Operator.NEG;
            case SysYParser.LNOT -> Operator.LNOT;
            default -> throw new IllegalArgumentException();
        };
    }

    private Operator makeBinaryOperator(Token token) {
        return switch (token.getType()) {
            case SysYParser.ADD -> Operator.ADD;
            case SysYParser.SUB -> Operator.SUB;
            case SysYParser.MUL -> Operator.MUL;
            case SysYParser.DIV -> Operator.DIV;
            case SysYParser.MOD -> Operator.MOD;
            case SysYParser.LT -> Operator.LT;
            case SysYParser.GT -> Operator.GT;
            case SysYParser.LE -> Operator.LE;
            case SysYParser.GE -> Operator.GE;
            case SysYParser.EQ -> Operator.EQ;
            case SysYParser.NE -> Operator.NE;
            case SysYParser.LAND -> Operator.LAND;
            case SysYParser.LOR -> Operator.LOR;
            default -> throw new IllegalArgumentException();
        };
    }

    private List<Parameter> makeParameterList(SysYParser.ParameterListContext ctx) {
        List<Parameter> parameters = new ArrayList<>();

        for (var parameterDeclaration : ctx.parameterDeclaration()) {
            Type type = makeType(parameterDeclaration.typeSpecifier().type);

            if (!parameterDeclaration.LBRACKET().isEmpty()) {
                var listIterator = parameterDeclaration.expression()
                        .listIterator(parameterDeclaration.expression().size());
                while (listIterator.hasPrevious()) {
                    visit(listIterator.previous());

                    type = ArrayType.getInstance(((ConstInt) result).getValue(), type);
                }

                type = PointerType.getInstance(type);
            }

            String name = parameterDeclaration.Identifier().getText();

            parameters.add(new Parameter(type, name));
        }

        return parameters;
    }

    private Node makeTree(SysYParser.InitializerContext ctx) {
        if (ctx.expression() != null)
            return new TerminalNode(ctx.expression());

        Node node = new Node();

        for (var initializer : ctx.initializer())
            node.addChild(makeTree(initializer));

        return node;
    }

    private Constant makeConstant(Node node, Type type) {
        if (node instanceof TerminalNode) {
            visit((SysYParser.ExpressionContext) ((TerminalNode) node).getValue());
            Constant constant = (Constant) result;

            if (constant.getType() != type)
                constant = Constants.convertType(constant, type);

            return constant;
        }

        List<Constant> elements = new ArrayList<>();

        for (Node child : node.getChildren())
            elements.add(makeConstant(child, ((ArrayType) type).getBaseType()));

        return new ConstArray(((ArrayType) type).getBaseType(), ((ArrayType) type).getLength(), elements);
    }

    private void convertType(Value value, Type targetType) {
        if (value.getType() == IntegerType.getI32() && targetType == FloatType.getFloat())
            result = new SignedIntegerToFloatInst(value, FloatType.getFloat());
        else if (value.getType() == FloatType.getFloat() && targetType == IntegerType.getI32())
            result = new FloatToSignedIntegerInst(value, IntegerType.getI32());
        else if (value.getType() == IntegerType.getI32() && targetType == IntegerType.getI1())
            result = new IntegerCompareInst(
                    IntegerType.getI32(), IntegerCompareInst.Condition.NE,
                    value, ConstInt.getInstance(0)
            );
        else if (value.getType() == FloatType.getFloat() && targetType == IntegerType.getI1())
            result = new FloatCompareInst(
                    FloatType.getFloat(), FloatCompareInst.Condition.ONE,
                    value, ConstFloat.getInstance(0)
            );
        else if (value.getType() == IntegerType.getI1() && targetType == IntegerType.getI32())
            result = new ZeroExtensionInst(value, IntegerType.getI32());
        else if (value.getType() instanceof PointerType && targetType instanceof PointerType)
            result = new BitCastInst(value, targetType);
        else
            throw new IllegalArgumentException();

        currentBasicBlock.addInstruction((Instruction) result);
    }

    private void applyUnaryOperator(Value operand, Operator operator) {
        if (operator == Operator.POS) {
            result = operand;
            return;
        }

        if (operand.getType() == IntegerType.getI32())
            result = switch (operator) {
                case NEG -> new IntegerSubInst(IntegerType.getI32(), ConstInt.getInstance(0), operand);
                case LNOT -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.EQ,
                        operand, ConstInt.getInstance(0)
                );
                default -> throw new IllegalArgumentException();
            };
        else if (operand.getType() == FloatType.getFloat())
            result = switch (operator) {
                case NEG -> new FloatNegateInst(FloatType.getFloat(), operand);
                case LNOT -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OEQ,
                        operand, ConstFloat.getInstance(0)
                );
                default -> throw new IllegalArgumentException();
            };
        else
            throw new IllegalArgumentException();

        currentBasicBlock.addInstruction((Instruction) result);

        if (operator == Operator.LNOT)
            convertType(result, IntegerType.getI32());
    }

    private void applyBinaryArithmeticOperator(Value leftOperand, Value rightOperand, Operator operator) {
        Type operandType = Types.getCommonType(leftOperand.getType(), rightOperand.getType());

        if (!leftOperand.getType().equals(operandType)) {
            convertType(leftOperand, operandType);
            leftOperand = result;
        }
        if (!rightOperand.getType().equals(operandType)) {
            convertType(rightOperand, operandType);
            rightOperand = result;
        }

        if (operandType == IntegerType.getI32())
            result = switch (operator) {
                case ADD -> new IntegerAddInst(IntegerType.getI32(), leftOperand, rightOperand);
                case SUB -> new IntegerSubInst(IntegerType.getI32(), leftOperand, rightOperand);
                case MUL -> new IntegerMultiplyInst(IntegerType.getI32(), leftOperand, rightOperand);
                case DIV -> new IntegerSignedDivideInst(IntegerType.getI32(), leftOperand, rightOperand);
                case MOD -> new IntegerSignedRemainderInst(IntegerType.getI32(), leftOperand, rightOperand);
                default -> throw new IllegalArgumentException();
            };
        else if (operandType == FloatType.getFloat())
            result = switch (operator) {
                case ADD -> new FloatAddInst(FloatType.getFloat(), leftOperand, rightOperand);
                case SUB -> new FloatSubInst(FloatType.getFloat(), leftOperand, rightOperand);
                case MUL -> new FloatMultiplyInst(FloatType.getFloat(), leftOperand, rightOperand);
                case DIV -> new FloatDivideInst(FloatType.getFloat(), leftOperand, rightOperand);
                default -> throw new IllegalArgumentException();
            };
        else
            throw new IllegalArgumentException();

        currentBasicBlock.addInstruction((Instruction) result);
    }

    private void applyBinaryRelationalOperator(Value leftOperand, Value rightOperand, Operator operator) {
        Type operandType = Types.getCommonType(leftOperand.getType(), rightOperand.getType());

        if (!leftOperand.getType().equals(operandType)) {
            convertType(leftOperand, operandType);
            leftOperand = result;
        }
        if (!rightOperand.getType().equals(operandType)) {
            convertType(rightOperand, operandType);
            rightOperand = result;
        }

        if (operandType == IntegerType.getI32())
            result = switch (operator) {
                case LT -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.SLT,
                        leftOperand, rightOperand
                );
                case GT -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.SGT,
                        leftOperand, rightOperand
                );
                case LE -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.SLE,
                        leftOperand, rightOperand
                );
                case GE -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.SGE,
                        leftOperand, rightOperand
                );
                case EQ -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.EQ,
                        leftOperand, rightOperand
                );
                case NE -> new IntegerCompareInst(
                        IntegerType.getI32(), IntegerCompareInst.Condition.NE,
                        leftOperand, rightOperand
                );
                default -> throw new IllegalArgumentException();
            };
        else if (operandType == FloatType.getFloat())
            result = switch (operator) {
                case LT -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OLT,
                        leftOperand, rightOperand
                );
                case GT -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OGT,
                        leftOperand, rightOperand
                );
                case LE -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OLE,
                        leftOperand, rightOperand
                );
                case GE -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OGE,
                        leftOperand, rightOperand
                );
                case EQ -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.OEQ,
                        leftOperand, rightOperand
                );
                case NE -> new FloatCompareInst(
                        FloatType.getFloat(), FloatCompareInst.Condition.ONE,
                        leftOperand, rightOperand
                );
                default -> throw new IllegalArgumentException();
            };
        else
            throw new IllegalArgumentException();

        currentBasicBlock.addInstruction((Instruction) result);
        convertType(result, IntegerType.getI32());
    }

    private void initializeVariable(Node node, Value address) {
        if (node instanceof TerminalNode) {
            visit((SysYParser.ExpressionContext) ((TerminalNode) node).getValue());

            if (result.getType() != ((PointerType) address.getType()).getBaseType())
                convertType(result, ((PointerType) address.getType()).getBaseType());

            currentBasicBlock.addInstruction(new StoreInst(address, result));
        } else {
            int index = 0;
            for (Node child : node.getChildren()) {
                var childAddress = new GetElementPtrInst(
                    address, List.of(ConstInt.getInstance(0), ConstInt.getInstance(index++)));
                currentBasicBlock.addInstruction(childAddress);
                initializeVariable(child, childAddress);
            }
        }
    }

    @Override
    public Void visitConstantDeclaration(SysYParser.ConstantDeclarationContext ctx) {
        Type baseType = makeType(ctx.typeSpecifier().type);

        for (var constantDefinition : ctx.constantDefinition()) {
            Type type = baseType;

            var listIterator = constantDefinition.expression()
                    .listIterator(constantDefinition.expression().size());
            while (listIterator.hasPrevious()) {
                visit(listIterator.previous());
                type = ArrayType.getInstance(((ConstInt) result).getValue(), type);
            }

            if (symbolTable.getScopeDepth() > 0) {
                var address = new AllocateInst(type);
                currentFunction.getEntryBasicBlock().addInstruction(address);

                Constant initialValue = makeConstant(
                    TreeNormalizer.normalize(makeTree(constantDefinition.initializer()), Types.getShape(type)),
                    type);
                currentBasicBlock.addInstruction(new StoreInst(address, initialValue));

                String name = constantDefinition.Identifier().getText();
                symbolTable.putLocalVariable(name, address, initialValue);
            } else {
                String name = constantDefinition.Identifier().getText();

                Constant initialValue;
                if (constantDefinition.initializer() == null)
                    initialValue = type.getZeroInitialization();
                else
                    initialValue = makeConstant(
                        TreeNormalizer.normalize(makeTree(constantDefinition.initializer()), Types.getShape(type)),
                        type);

                GlobalVariable globalVariable = new GlobalVariable(true, initialValue);
                globalVariable.setValueName(name);

                module.addGlobalVariable(globalVariable);
                symbolTable.putGlobalVariable(name, globalVariable);
            }
        }

        return null;
    }

    @Override
    public Void visitVariableDeclaration(SysYParser.VariableDeclarationContext ctx) {
        Type baseType = makeType(ctx.typeSpecifier().type);

        for (var variableDefinition : ctx.variableDefinition()) {
            Type type = baseType;

            var listIterator = variableDefinition.expression()
                    .listIterator(variableDefinition.expression().size());
            while (listIterator.hasPrevious()) {
                visit(listIterator.previous());
                type = ArrayType.getInstance(((ConstInt) result).getValue(), type);
            }

            if (symbolTable.getScopeDepth() > 0) {
                var address = new AllocateInst(type);
                currentFunction.getEntryBasicBlock().addInstruction(address);
                String name = variableDefinition.Identifier().getText();
                symbolTable.putLocalVariable(name, address, null);

                if (variableDefinition.initializer() != null) {
                    currentBasicBlock.addInstruction(new StoreInst(address, type.getZeroInitialization()));
                    initializeVariable(
                        TreeNormalizer.normalize(makeTree(variableDefinition.initializer()), Types.getShape(type)),
                        address);
                }
            } else {
                Constant initialValue;
                if (variableDefinition.initializer() == null)
                    initialValue = type.getZeroInitialization();
                else
                    initialValue = makeConstant(
                        TreeNormalizer.normalize(makeTree(variableDefinition.initializer()), Types.getShape(type)),
                        type);

                String name = variableDefinition.Identifier().getText();

                GlobalVariable globalVariable = new GlobalVariable(false, initialValue);
                globalVariable.setValueName(name);
                module.addGlobalVariable(globalVariable);
                symbolTable.putGlobalVariable(name, globalVariable);
            }
        }

        return null;
    }

    @Override
    public Void visitFunctionDefinition(SysYParser.FunctionDefinitionContext ctx) {
        String name = ctx.Identifier().getText();
        Type returnType = makeType(ctx.typeSpecifier().type);
        List<Parameter> parameters = List.of();
        if (ctx.parameterList() != null)
            parameters = makeParameterList(ctx.parameterList());
        List<Type> parameterTypes = parameters.stream().map(Parameter::getType).toList();

        FunctionType type = FunctionType.getInstance(returnType, parameterTypes);

        currentFunction = new Function(type);
        currentFunction.setValueName(name);

        module.addFunction(currentFunction);
        symbolTable.putFunction(name, currentFunction);

        currentBasicBlock = currentFunction.getEntryBasicBlock();

        visit(ctx.compoundStatement());

        if (returnType == VoidType.getInstance())
            currentBasicBlock.addInstruction(new ReturnInst(VoidValue.getInstance()));
        if (returnType == IntegerType.getI32())
            currentBasicBlock.addInstruction(new ReturnInst(ConstInt.getInstance(0)));
        if (returnType == FloatType.getFloat())
            currentBasicBlock.addInstruction(new ReturnInst(ConstFloat.getInstance(0f)));

        return null;
    }

    @Override
    public Void visitCompoundStatement(SysYParser.CompoundStatementContext ctx) {
        symbolTable.pushScope();

        if (ctx.getParent() instanceof SysYParser.FunctionDefinitionContext functionDefinition) {
            if (functionDefinition.parameterList() != null) {
                List<Parameter> parameters = makeParameterList(functionDefinition.parameterList());

                for (int i = 0; i < parameters.size(); ++i) {
                    Parameter parameter = parameters.get(i);

                    var address = new AllocateInst(parameter.getType());
                    currentFunction.getEntryBasicBlock().addInstruction(address);
                    currentBasicBlock.addInstruction(new StoreInst(address, currentFunction.getFormalParameters().get(i)));

                    symbolTable.putLocalVariable(parameter.getName(), address, null);
                }
            }
        }

        if (ctx.blockItem() != null)
            for (var blockItem : ctx.blockItem())
                visit(blockItem);

        symbolTable.popScope();
        return null;
    }

    @Override
    public Void visitAssignmentStatement(SysYParser.AssignmentStatementContext ctx) {
        visit(ctx.lValue());
        Value address = resultAddress;

        visit(ctx.expression());
        if (result.getType() != ((PointerType) address.getType()).getBaseType())
            convertType(result, ((PointerType) address.getType()).getBaseType());
        Value value = result;

        currentBasicBlock.addInstruction(new StoreInst(address, value));

        return null;
    }

    @Override
    public Void visitIfStatement(SysYParser.IfStatementContext ctx) {
        var savedCurrentTrueExit = currentTrueExit;
        var savedCurrentFalseExit = currentFalseExit;

        BasicBlock thenBlock = new BasicBlock();
        BasicBlock elseBlock = new BasicBlock();
        BasicBlock doneBlock = new BasicBlock();

        currentTrueExit = thenBlock;
        currentFalseExit = elseBlock;

        currentFunction.addBasicBlock(thenBlock);
        currentFunction.addBasicBlock(elseBlock);
        currentFunction.addBasicBlock(doneBlock);

        visit(ctx.expression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        currentBasicBlock.addInstruction(new BranchInst(result, currentTrueExit, currentFalseExit));

        currentBasicBlock = thenBlock;
        visit(ctx.statement(0));
        currentBasicBlock.addInstruction(new JumpInst(doneBlock));

        currentBasicBlock = elseBlock;
        if (ctx.statement().size() == 2)
            visit(ctx.statement(1));
        currentBasicBlock.addInstruction(new JumpInst(doneBlock));

        currentBasicBlock = doneBlock;

        currentTrueExit = savedCurrentTrueExit;
        currentFalseExit = savedCurrentFalseExit;
        return null;
    }

    @Override
    public Void visitWhileStatement(SysYParser.WhileStatementContext ctx) {
        var savedCurrentTrueExit = currentTrueExit;
        var savedCurrentFalseExit = currentFalseExit;

        BasicBlock testBlock = new BasicBlock();
        BasicBlock bodyBlock = new BasicBlock();
        BasicBlock doneBlock = new BasicBlock();

        currentTrueExit = bodyBlock;
        currentFalseExit = doneBlock;

        currentFunction.addBasicBlock(testBlock);
        currentFunction.addBasicBlock(bodyBlock);
        currentFunction.addBasicBlock(doneBlock);

        controlFlowStack.push(new WhileContext(testBlock, doneBlock));

        currentBasicBlock.addInstruction(new JumpInst(testBlock));

        currentBasicBlock = testBlock;

        visit(ctx.expression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        currentBasicBlock.addInstruction(new BranchInst(result, currentTrueExit, currentFalseExit));

        currentBasicBlock = bodyBlock;

        visit(ctx.statement());

        currentBasicBlock.addInstruction(new JumpInst(testBlock));

        currentBasicBlock = doneBlock;

        controlFlowStack.pop();

        currentTrueExit = savedCurrentTrueExit;
        currentFalseExit = savedCurrentFalseExit;
        return null;
    }

    @Override
    public Void visitBreakStatement(SysYParser.BreakStatementContext ctx) {
        BasicBlock doneBlock = ((WhileContext) controlFlowStack.element()).getDoneBlock();
        currentBasicBlock.addInstruction(new JumpInst(doneBlock));

        currentBasicBlock = new BasicBlock();
        currentFunction.addBasicBlock(currentBasicBlock);

        return null;
    }

    @Override
    public Void visitContinueStatement(SysYParser.ContinueStatementContext ctx) {
        BasicBlock testBlock = ((WhileContext) controlFlowStack.element()).getTestBlock();
        currentBasicBlock.addInstruction(new JumpInst(testBlock));

        currentBasicBlock = new BasicBlock();
        currentFunction.addBasicBlock(currentBasicBlock);

        return null;
    }

    @Override
    public Void visitReturnStatement(SysYParser.ReturnStatementContext ctx) {
        if (ctx.expression() != null) {
            visit(ctx.expression());
            if (result.getType() != currentFunction.getReturnType())
                convertType(result, currentFunction.getReturnType());

            currentBasicBlock.addInstruction(new ReturnInst(result));
        } else
            currentBasicBlock.addInstruction(new ReturnInst(VoidValue.getInstance()));

        currentBasicBlock = new BasicBlock();
        currentFunction.addBasicBlock(currentBasicBlock);

        return null;
    }

    @Override
    public Void visitBinaryLogicalOrExpression(SysYParser.BinaryLogicalOrExpressionContext ctx) {
        BasicBlock falseBlock = new BasicBlock();

        currentFunction.addBasicBlock(falseBlock);

        var savedCurrentFalseExit = currentFalseExit;
        currentFalseExit = falseBlock;

        visit(ctx.logicalOrExpression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        currentBasicBlock.addInstruction(new BranchInst(result, currentTrueExit, currentFalseExit));

        currentBasicBlock = falseBlock;
        currentFalseExit = savedCurrentFalseExit;

        visit(ctx.logicalAndExpression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        return null;
    }

    @Override
    public Void visitBinaryLogicalAndExpression(SysYParser.BinaryLogicalAndExpressionContext ctx) {
        BasicBlock trueBlock = new BasicBlock();

        currentFunction.addBasicBlock(trueBlock);

        var savedCurrentTrueExit = currentTrueExit;
        currentTrueExit = trueBlock;

        visit(ctx.logicalAndExpression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        currentBasicBlock.addInstruction(new BranchInst(result, currentTrueExit, currentFalseExit));

        currentBasicBlock = trueBlock;
        currentTrueExit = savedCurrentTrueExit;

        visit(ctx.equalityExpression());
        if (result.getType() != IntegerType.getI1())
            convertType(result, IntegerType.getI1());

        return null;
    }

    @Override
    public Void visitBinaryEqualityExpression(SysYParser.BinaryEqualityExpressionContext ctx) {
        visit(ctx.equalityExpression());
        Value leftOperand = result;

        visit(ctx.relationalExpression());
        Value rightOperand = result;

        applyBinaryRelationalOperator(leftOperand, rightOperand, makeBinaryOperator(ctx.op));
        return null;
    }

    @Override
    public Void visitBinaryRelationalExpression(SysYParser.BinaryRelationalExpressionContext ctx) {
        visit(ctx.relationalExpression());
        Value leftOperand = result;

        visit(ctx.additiveExpression());
        Value rightOperand = result;

        applyBinaryRelationalOperator(leftOperand, rightOperand, makeBinaryOperator(ctx.op));
        return null;
    }

    @Override
    public Void visitBinaryAdditiveExpression(SysYParser.BinaryAdditiveExpressionContext ctx) {
        visit(ctx.additiveExpression());
        Value leftOperand = result;

        visit(ctx.multiplicativeExpression());
        Value rightOperand = result;

        Operator operator = makeBinaryOperator(ctx.op);

        if (leftOperand instanceof Constant && rightOperand instanceof Constant)
            result = Constants.applyBinaryOperator((Constant) leftOperand, (Constant) rightOperand, operator);
        else
            applyBinaryArithmeticOperator(leftOperand, rightOperand, operator);

        return null;
    }

    @Override
    public Void visitBinaryMultiplicativeExpression(SysYParser.BinaryMultiplicativeExpressionContext ctx) {
        visit(ctx.multiplicativeExpression());
        Value leftOperand = result;

        visit(ctx.unaryExpression());
        Value rightOperand = result;

        Operator operator = makeBinaryOperator(ctx.op);

        if (leftOperand instanceof Constant && rightOperand instanceof Constant)
            result = Constants.applyBinaryOperator((Constant) leftOperand, (Constant) rightOperand, operator);
        else
            applyBinaryArithmeticOperator(leftOperand, rightOperand, operator);

        return null;
    }

    @Override
    public Void visitFunctionCallExpression(SysYParser.FunctionCallExpressionContext ctx) {
        String name = ctx.Identifier().getText();
        BaseFunction function = symbolTable.getFunction(name);

        List<Value> arguments = new ArrayList<>();
        if (ctx.argumentExpressionList() != null) {
            var parameterTypeIterator = function.getParameterTypes().iterator();
            var expressionIterator = ctx.argumentExpressionList().expression().iterator();

            while (parameterTypeIterator.hasNext() && expressionIterator.hasNext()) {
                Type parameterType = parameterTypeIterator.next();
                var expression = expressionIterator.next();

                visit(expression);
                if (result.getType() != parameterType)
                    convertType(result, parameterType);
                arguments.add(result);
            }
        }

        result = new CallInst(function, arguments);
        currentBasicBlock.addInstruction((Instruction) result);
        return null;
    }

    @Override
    public Void visitUnaryOperatorExpression(SysYParser.UnaryOperatorExpressionContext ctx) {
        visit(ctx.unaryExpression());
        Value operand = result;
        Operator operator = makeUnaryOperator(ctx.unaryOperator().op);

        if (operand instanceof Constant)
            result = Constants.applyUnaryOperator((Constant) operand, operator);
        else
            applyUnaryOperator(operand, operator);

        return null;
    }

    @Override
    public Void visitLValue(SysYParser.LValueContext ctx) {
        String name = ctx.Identifier().getText();
        SymbolTable.Entry entry = symbolTable.getVariable(name);

        List<Value> indices = new ArrayList<>();
        for (var expression : ctx.expression()) {
            visit(expression);
            indices.add(result);
        }

        resultAddress = entry.getAddress();
        for (Value index : indices) {
            if (((PointerType) resultAddress.getType()).getBaseType() instanceof PointerType) {
                resultAddress = new LoadInst(resultAddress);
                currentBasicBlock.addInstruction((Instruction) resultAddress);
                resultAddress = new GetElementPtrInst(resultAddress, List.of(index));
                currentBasicBlock.addInstruction((Instruction) resultAddress);
            } else {
                resultAddress = new GetElementPtrInst(resultAddress, List.of(ConstInt.getInstance(0), index));
                currentBasicBlock.addInstruction((Instruction) resultAddress);
            }
        }

        if (entry.getConstantValue() != null && indices.stream().allMatch(index -> index instanceof ConstInt)) {
            result = entry.getConstantValue();
            for (Value index : indices)
                result = ((ConstArray) result).getValueAt(((ConstInt) index).getValue());
        } else {
            result = new LoadInst(resultAddress);
            currentBasicBlock.addInstruction((Instruction) result);
        }
        if (result.getType() instanceof ArrayType) {
            result = new GetElementPtrInst(resultAddress, List.of(ConstInt.getInstance(0), ConstInt.getInstance(0)));
            currentBasicBlock.addInstruction((Instruction) result);
        }

        return null;
    }

    @Override
    public Void visitIntegerConstant(SysYParser.IntegerConstantContext ctx) {
        String text = ctx.IntegerConstant().getText();

        int value;
        if ("0".equals(text))
            value = 0;
        else if (text.startsWith("0x"))
            value = Integer.parseInt(text.substring(2), 16);
        else if (text.startsWith("0"))
            value = Integer.parseInt(text.substring(1), 8);
        else
            value = Integer.parseInt(text);

        result = ConstInt.getInstance(value);
        return null;
    }

    @Override
    public Void visitFloatingConstant(SysYParser.FloatingConstantContext ctx) {
        result = ConstFloat.getInstance(Float.parseFloat(ctx.FloatingConstant().getText()));
        return null;
    }
}
