package cn.edu.bit.newnewcc.ir.exception;

public class UsageRelationshipCheckFailedException extends CompilationProcessCheckFailedException {
    public UsageRelationshipCheckFailedException() {
    }

    public UsageRelationshipCheckFailedException(String message) {
        super(message);
    }

    public UsageRelationshipCheckFailedException(String message, Throwable cause) {
        super(message, cause);
    }

    public UsageRelationshipCheckFailedException(Throwable cause) {
        super(cause);
    }
}
