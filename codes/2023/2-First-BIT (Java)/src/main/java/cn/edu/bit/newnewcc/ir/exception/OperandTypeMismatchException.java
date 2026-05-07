package cn.edu.bit.newnewcc.ir.exception;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Instruction;

public class OperandTypeMismatchException extends CompilationProcessCheckFailedException {
    public OperandTypeMismatchException() {
    }

    public OperandTypeMismatchException(String message) {
        super(message);
    }

    public OperandTypeMismatchException(String message, Throwable cause) {
        super(message, cause);
    }

    public OperandTypeMismatchException(Throwable cause) {
        super(cause);
    }

    public OperandTypeMismatchException(Instruction context, Type expectedType, Type actualType) {
        this(String.format("In instruction %s, expected type %s, got type %s", context, expectedType.getTypeName(), actualType.getTypeName()));
    }
}
