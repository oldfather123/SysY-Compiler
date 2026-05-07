package Backend.component;

import java.util.ArrayList;

public class AsmGlobalOther extends AsmGlobalVariable {
    public AsmGlobalOther(String name, boolean isInit, int size, ArrayList<Integer> elements) {
        setName(name);
        this.isInit = isInit;
        if (isInit) {
            this.size = 4 * elements.size();
            this.elements = elements;
        } else {
            this.size = size;
            this.elements = null;
        }
    }

    @Override
    public String toString() {
        StringBuilder output = new StringBuilder();
        output.append(GLOBAL).append(name).append("\n");
        if (isInit) {
            output.append(SECTION_DATA);
        } else {
            output.append(SECTION_BSS);
        }
        output.append(String.format(TYPE, name, "@object"));
        output.append(String.format(SIZE, name, size));
        output.append(name).append(":\n");
        if (isInit) {
            for (int value : elements) {
                output.append(String.format(WORD, value));
            }
            if (elements.size() * 4 < size) {
                int last_size = size - elements.size() * 4;
                output.append(String.format(ZERO, last_size));
            }
        } else {
            output.append(String.format(ZERO, size));
        }
        return output.toString();
    }
}
