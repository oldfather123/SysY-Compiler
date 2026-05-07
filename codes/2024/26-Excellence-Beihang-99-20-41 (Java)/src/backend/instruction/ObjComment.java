package backend.instruction;

import backend.component.ObjInstr;
import ir.instr.CommentInstr;

public class ObjComment extends ObjInstr {
    private final String comment;

    public ObjComment(CommentInstr instr) {
        super("Comment");
        this.comment = "# " + instr.toString();
    }

    @Override
    public String toString() {
        return this.comment;
    }
}
