package backend.opt;

import backend.asm.structure.ASMGlobalObject;

import java.util.ArrayList;
import java.util.HashMap;

public class ASMGPool extends ASMGlobalObject {
    private final HashMap<ASMGlobalObject, Integer> gv2offset = new HashMap<>();
    private final ArrayList<ASMGlobalObject> glbObjs = new ArrayList<>();

    public static ASMGPool getInstance() {
        return new ASMGPool();
    }

    private ASMGPool() {
        super("ohmGpool", new ArrayList<>(), false);
    }

    public void addGlobal(ASMGlobalObject globalObject) {
        if (!gv2offset.containsKey(globalObject)) {
            gv2offset.put(globalObject, this.getSize());
            glbObjs.add(globalObject);
            this.addSize(4);
        }
    }

    public int getOffset(ASMGlobalObject globalObject) {
        return gv2offset.get(globalObject);
    }

    @Override
    public String toString() {
        if (glbObjs.isEmpty()) return "";
        StringBuilder sb = new StringBuilder();
        sb.append("\t.section .rodata\n\t.align 3\n").append(this.getLabelName()).append(":\n");
        for (ASMGlobalObject obj : glbObjs) {
            sb.append("\t").append(obj.getInitValues().getFirst().toString()).append("\n");
        }
        return sb.toString();
    }
}
