package frontend.ir;

import Utils.CustomList;
import frontend.ir.instr.Instruction;

public class Use extends CustomList.Node{
    private Instruction user;
    private Value used;
    private static int use_num = 0;
    private int id = ++use_num;

    public Use() {
        super();
    }
    public Use(Instruction user, Value used) {
        super();
        this.user = user;
        this.used = used;
    }
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Use use = (Use) o;
        return id == use.id;
    }

    public Value getUsed() {
        return used;
    }
    public Instruction getUser() {
        return user;
    }
}
