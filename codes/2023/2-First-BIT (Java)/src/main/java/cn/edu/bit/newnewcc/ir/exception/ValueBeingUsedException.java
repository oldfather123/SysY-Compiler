package cn.edu.bit.newnewcc.ir.exception;

public class ValueBeingUsedException extends CompilationProcessCheckFailedException {
    public ValueBeingUsedException() {
    }

    public ValueBeingUsedException(String message) {
        super(message);
    }

    public ValueBeingUsedException(String message, Throwable cause) {
        super(message, cause);
    }

    public ValueBeingUsedException(Throwable cause) {
        super(cause);
    }
}
