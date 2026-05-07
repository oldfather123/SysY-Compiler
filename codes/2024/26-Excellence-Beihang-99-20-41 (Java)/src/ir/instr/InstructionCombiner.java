package ir.instr;

import ir.IrInstr;
import ir.IrModule;
import ir.value.*;
import pass.Pass;

import java.util.HashMap;
import java.util.Objects;

class StoredValue {
    private Value op1;
    private Value op2;
    private IrInstr.OpCode opCode;

    public StoredValue(Value op1, Value op2, IrInstr.OpCode opCode) {
        this.op1 = op1;
        this.op2 = op2;
        this.opCode = opCode;
    }

    public StoredValue(StoredValue other) {
        this(other.op1, other.op2, other.opCode);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof StoredValue other) {
            return Objects.equals(op1, other.op1) && Objects.equals(op2, other.op2) && Objects.equals(opCode, other.opCode);
        }
        return false;
    }

    private static StoredValue tryAdd(StoredValue val, Value op) {
        StoredValue self = new StoredValue(val);
        // %B = %A; %C = %B + %A => %C = %A * 2
        if (self.opCode.equals(IrInstr.OpCode.MOVE) && op.equals(self.op1)) {
            self.op2 = new Literal(2);
            self.opCode = IrInstr.OpCode.MUL;
            return self;
        }
        // %B = %A; %C = %B + op => %C = %A + op
        if (self.opCode.equals(IrInstr.OpCode.MOVE)) {
            self.op2 = op;
            self.opCode = IrInstr.OpCode.ADD;
            return self;
        }
        // %B = %A + int; %C = %B + int => %C = %A + int
        if (self.opCode.equals(IrInstr.OpCode.ADD) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op1 instanceof Literal literal && literal.getType().is("INT")) {
                self.op1 = new Literal(literal.asInt() + income.asInt(), income.getType());
                return self;
            }
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(income.asInt() + literal.asInt(), income.getType());
                return self;
            }
        }
        // %B = %A - int; %C = %B + int => %C = %A [{-|+} int]
        if (self.opCode.equals(IrInstr.OpCode.SUB) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(literal.asInt() - income.asInt(), income.getType());
                if (((Literal) self.op2).asInt() == 0) {
                    self.opCode = IrInstr.OpCode.MOVE;
                    self.op2 = null;
                } else if (((Literal) self.op2).asInt() < 0) {
                    self.opCode = IrInstr.OpCode.ADD;
                    self.op2 = new Literal(-((Literal) self.op2).asInt(), income.getType());
                }
                return self;
            }
        }
        // %B = %A * int; %C = %B + %A => %C = %A * (int + 1)
        if (self.opCode.equals(IrInstr.OpCode.MUL) && (op.equals(self.op1) || op.equals(self.op2)) && op instanceof Intermediate) {
            if (self.op1.equals(op) && self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(literal.asInt() + 1);
                return self;
            }
            if (self.op2.equals(op) && self.op1 instanceof Literal literal && literal.getType().is("INT")) {
                self.op1 = new Literal(literal.asInt() + 1);
                return self;
            }
        }

        return null;
    }

    private static StoredValue trySub(StoredValue val, Value op) {
        StoredValue self = new StoredValue(val);
        // %B = %A; %C = %B - %A => %C = 0
        if (self.opCode.equals(IrInstr.OpCode.MOVE) && op.equals(self.op1)) {
            self.op2 = new Literal(0);
            self.opCode = IrInstr.OpCode.MOVE;
            return self;
        }
        // %B = %A; %C = %B - op => %C = %A - op
        if (self.opCode.equals(IrInstr.OpCode.MOVE)) {
            self.op2 = op;
            self.opCode = IrInstr.OpCode.SUB;
            return self;
        }
        // %B = %A - int; %C = %B - int => %C = %A - int
        if (self.opCode.equals(IrInstr.OpCode.SUB) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(literal.asInt() + income.asInt(), income.getType());
                return self;
            }
        }
        // %B = %A + int; %C = %B - int => %C = %A [{+|-} int]
        if (self.opCode.equals(IrInstr.OpCode.ADD) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(literal.asInt() - income.asInt(), income.getType());
                if (((Literal) self.op2).asInt() == 0) {
                    self.opCode = IrInstr.OpCode.MOVE;
                    self.op2 = null;
                } else if (((Literal) self.op2).asInt() < 0) {
                    self.opCode = IrInstr.OpCode.SUB;
                    self.op2 = new Literal(-((Literal) self.op2).asInt(), income.getType());
                }
                return self;
            }
        }
        // %B = %A * int; %C = %B - %A => %C = %A * (int - 1)
        if (self.opCode.equals(IrInstr.OpCode.MUL) && (op.equals(self.op1) || op.equals(self.op2)) && op instanceof Intermediate) {
            if (self.op1.equals(op)) {
                self.op2 = new Literal(((Literal) self.op2).asInt() - 1);
                return self;
            }
            if (self.op2.equals(op)) {
                self.op1 = new Literal(((Literal) self.op1).asInt() - 1);
                return self;
            }
        }

        return null;
    }

    private static StoredValue tryMul(StoredValue val, Value op) {
        StoredValue self = new StoredValue(val);
        // %B = %A; %C = %B * op => %C = %A * op
        if (self.opCode.equals(IrInstr.OpCode.MOVE)) {
            self.op2 = op;
            self.opCode = IrInstr.OpCode.MUL;
            if (self.op2 instanceof Literal literal && literal.getType().is("INT") && literal.asInt() == 1) {
                self.opCode = IrInstr.OpCode.MOVE;
                self.op2 = null;
            }
            return self;
        }
        // %B = %A * int; %C = %B * int => %C = %A * (int * int)
        if (self.opCode.equals(IrInstr.OpCode.MUL) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op1 instanceof Literal literal && literal.getType().is("INT")) {
                self.op1 = new Literal(literal.asInt() * income.asInt(), income.getType());
                return self;
            }
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(income.asInt() * literal.asInt(), income.getType());
                return self;
            }
        }

        return null;
    }

    private static StoredValue tryDiv(StoredValue val, Value op) {
        StoredValue self = new StoredValue(val);
        // %B = %A; %C = %B / op => %C = %A / op
        if (self.opCode.equals(IrInstr.OpCode.MOVE)) {
            self.op2 = op;
            self.opCode = IrInstr.OpCode.SDIV;
            if (self.op2 instanceof Literal literal && literal.getType().is("INT") && literal.asInt() == 1) {
                self.opCode = IrInstr.OpCode.MOVE;
                self.op2 = null;
            }
            return self;
        }
        // %B = %A * %C; %D = %B / %C => %D = %A
        if (self.opCode.equals(IrInstr.OpCode.MUL) && op.equals(self.op2)) {
            self.opCode = IrInstr.OpCode.MOVE;
            self.op2 = self.op1;
            return self;
        }
        // %B = %A * int; %C = %B / int => %C = %A [* int]
        if (self.opCode.equals(IrInstr.OpCode.MUL) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op1 instanceof Literal literal && literal.getType().is("INT")
                    && literal.asInt() % income.asInt() == 0) {
                self.op1 = new Literal(literal.asInt() / income.asInt(), income.getType());
                if (((Literal) self.op1).asInt() == 1) {
                    self.opCode = IrInstr.OpCode.MOVE;
                    self.op1 = null;
                }
                return self;
            }
        }
        // %B = %A / int; %C = %B / int => %C = %A / (int * int)
        if (self.opCode.equals(IrInstr.OpCode.SDIV) && op instanceof Literal income && income.getType().is("INT")) {
            if (self.op2 instanceof Literal literal && literal.getType().is("INT")) {
                self.op2 = new Literal(literal.asInt() / income.asInt(), income.getType());
                return self;
            }
        }

        return null;
    }

    public IrInstr spawnInstruction(Value res) {
        if (opCode.equals(IrInstr.OpCode.MOVE)) {
            return new MoveInstr(res, op1);
        }
        return new BinaInstr(op1, op2, opCode, res);
    }

    private void apply(StoredValue val) {
        if (val != null) {
            op1 = val.op1;
            op2 = val.op2;
            opCode = val.opCode;
        }
    }

    private void simplifyAdd(HashMap<String, StoredValue> stored) {
        if (op1 instanceof Intermediate && stored.containsKey(op1.getName())) {
            apply(tryAdd(stored.get(op1.getName()), op2));
        }
        if (op2 instanceof Intermediate && stored.containsKey(op2.getName())) {
            apply(tryAdd(stored.get(op2.getName()), op1));
        }
    }

    private void simplifySub(HashMap<String, StoredValue> stored) {
        if (op1 instanceof Intermediate && stored.containsKey(op1.getName())) {
            apply(trySub(stored.get(op1.getName()), op2));
        }
    }

    private void simplifyMul(HashMap<String, StoredValue> stored) {
        if (op1 instanceof Intermediate && stored.containsKey(op1.getName())) {
            apply(tryMul(stored.get(op1.getName()), op2));
        }
        if (op2 instanceof Intermediate && stored.containsKey(op2.getName())) {
            apply(tryMul(stored.get(op2.getName()), op1));
        }
    }

    private void simplifyDiv(HashMap<String, StoredValue> stored) {
        if (op1 instanceof Intermediate && stored.containsKey(op1.getName())) {
            apply(tryDiv(stored.get(op1.getName()), op2));
        }
    }

    public void simplify(HashMap<String, StoredValue> stored) {
        if (opCode.equals(IrInstr.OpCode.MOVE)) {
            if (op1 instanceof Intermediate && stored.containsKey(op1.getName())) {
                var storedValue = stored.get(op1.getName());
                op1 = storedValue.op1;
                op2 = storedValue.op2;
                opCode = storedValue.opCode;
            }
        } else {
            switch (opCode) {
                case ADD -> simplifyAdd(stored);
                case SUB -> simplifySub(stored);
                case MUL -> simplifyMul(stored);
                case SDIV -> simplifyDiv(stored);
                default -> {}
            }
        }
    }
}

public class InstructionCombiner implements Pass.IrPass {
    @Override
    public void run(IrModule module) {
        for (Function function : module.getFunctions()) {
            for (BasicBlock block : function.getBasicBlocks()) {
                run(block);
            }
        }
    }

    private final HashMap<String, StoredValue> stored = new HashMap<>();

    private void run(BasicBlock block) {
        stored.clear();
        var iter = block.getInstrs().listIterator();
        while (iter.hasNext()) {
            if (!(iter.next() instanceof BinaInstr binary)) {
                continue;
            }
            if (binary.pointerOffset) {
                // TODO: we now ignore pointer offset
                continue;
            }
            StoredValue val = new StoredValue(binary.getOperands()[1], binary.getOperands()[2], binary.getOpCode());
            StoredValue copied;
            do {
                copied = new StoredValue(val);
                val.simplify(stored);
            } while (!copied.equals(val));
            stored.put(binary.getOperands()[0].getName(), val);
            iter.set(val.spawnInstruction(binary.getInitialValue()));
        }
    }

    @Override
    public String getName() {
        return "inst-comb";
    }
}
