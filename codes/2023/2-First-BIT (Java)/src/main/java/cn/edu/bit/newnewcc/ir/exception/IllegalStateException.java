package cn.edu.bit.newnewcc.ir.exception;

public class IllegalStateException extends CompilationProcessCheckFailedException {
    public IllegalStateException() {
    }

    public IllegalStateException(String message) {
        super(message);
    }

    public IllegalStateException(String message, Throwable cause) {
        super(message, cause);
    }

    public IllegalStateException(Throwable cause) {
        super(cause);
    }
}
