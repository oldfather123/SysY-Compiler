package cn.edu.bit.newnewcc.ir.exception;

public class IllegalBitWidthException extends CompilationProcessCheckFailedException {
    public IllegalBitWidthException() {
    }

    public IllegalBitWidthException(String message) {
        super(message);
    }

    public IllegalBitWidthException(String message, Throwable cause) {
        super(message, cause);
    }

    public IllegalBitWidthException(Throwable cause) {
        super(cause);
    }
}
