package cn.edu.bit.newnewcc.ir.exception;

public class SemanticCheckFailedException extends CompilationProcessCheckFailedException {
    public SemanticCheckFailedException() {
    }

    public SemanticCheckFailedException(String message) {
        super(message);
    }

    public SemanticCheckFailedException(String message, Throwable cause) {
        super(message, cause);
    }

    public SemanticCheckFailedException(Throwable cause) {
        super(cause);
    }
}
