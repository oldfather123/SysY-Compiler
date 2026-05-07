package cn.edu.bit.newnewcc.ir.exception;

public class IndexOutOfBoundsException extends CompilationProcessCheckFailedException {
    public IndexOutOfBoundsException() {
    }

    public IndexOutOfBoundsException(String message) {
        super(message);
    }

    public IndexOutOfBoundsException(String message, Throwable cause) {
        super(message, cause);
    }

    public IndexOutOfBoundsException(Throwable cause) {
        super(cause);
    }

    public IndexOutOfBoundsException(int requestIndex, int minimumIndex, int maximumIndex) {
        this(String.format("Index %d out of range [%d,%d).", requestIndex, minimumIndex, maximumIndex));
    }
}
