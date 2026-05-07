package cn.edu.bit.newnewcc.ir.exception;

public class CompilationProcessCheckFailedException extends RuntimeException {
    public CompilationProcessCheckFailedException() {
    }

    public CompilationProcessCheckFailedException(String message) {
        super(message);
    }

    public CompilationProcessCheckFailedException(String message, Throwable cause) {
        super(message, cause);
    }

    public CompilationProcessCheckFailedException(Throwable cause) {
        super(cause);
    }
}
