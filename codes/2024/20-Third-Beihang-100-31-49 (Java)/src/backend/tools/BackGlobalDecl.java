package backend.tools;

import java.util.ArrayList;

public class BackGlobalDecl {
    private String name;
    private static int lcNumber = 100;
    private ArrayList<Integer> numbers;
    private Boolean isConst;
    private Boolean isInit;
    private int size;

    public BackGlobalDecl(String name,ArrayList<Integer> numbers) {
        this.name = name.substring(1);
        this.isInit = true;
        this.isConst = false;
        this.size = 4 * numbers.size();
        this.numbers = numbers;
    }

    public BackGlobalDecl(String name, int size) {
        this.name = name.substring(1);
        this.isInit = false;
        this.size = size;
        this.isConst = false;
    }

    public BackGlobalDecl(Integer floatInit) {
        this.name = ".LC" + lcNumber;
        lcNumber++;
        this.numbers = new ArrayList<>();
        numbers.add(floatInit);
        this.size = 4;
        this.isInit = true;
        this.isConst=true;
    }

    public String getName() {
        return this.name;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (!isConst) {
            sb.append(" .global ").append(this.name).append("\n");
        }
        if (isConst) {
            sb.append(" .section .rodata\n  .align 3\n");
        } else if (this.isInit) {
            sb.append(" .data\n");
        } else {
            sb.append(" .bss\n");
        }
        if (!isConst) {
            sb.append(" .type ").append(this.name).append(", @object\n");
            sb.append(" .size ").append(this.name).append(", ").append(size).append("\n");
        }
        sb.append(name).append(":\n");
        if (this.isInit) {
            for (int number:numbers) {
                sb.append(" .word ").append(number).append("\n");
            }
            if (4 * numbers.size() < size) {
                int left = size - 4 * numbers.size();
                sb.append(" .zero ").append(left).append("\n");
            }
        } else {
            sb.append(" .zero ").append(size).append("\n");
        }
        return sb.toString();
    }
}
