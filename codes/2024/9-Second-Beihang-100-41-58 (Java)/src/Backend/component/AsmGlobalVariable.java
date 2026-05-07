package Backend.component;

import java.util.ArrayList;

public class AsmGlobalVariable extends AsmComponent {
    public static final String GLOBAL = "\t.globl\t";
    public static final String SECTION_RODATA = "\t.section .rodata\n\t.align 3\n";
    public static final String SECTION_DATA = "\t.data\n";
    public static final String SECTION_BSS = "\t.bss\n";
    public static final String TYPE = "\t.type\t%s,\t%s\n";
    public static final String SIZE = "\t.size\t%s,\t%s\n";
    public static final String WORD = "\t.word\t%d\n";
    public static final String ZERO = "\t.zero\t%d\n";

    public String name;
    public ArrayList<Integer> elements;
    public boolean isInit;
    public int size;

    public String getName() {
        return name;
    }

    public void setName(String name) {
        if (name == null || name.isEmpty()) {
            throw new IllegalArgumentException("Name cannot be null or empty");
        }
        this.name = name.substring(1);
    }
}