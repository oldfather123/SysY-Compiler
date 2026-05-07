package ir.traversal;

import frontend.ast.AbstractSyntaxTree;
import ir.type.PointerType;
import ir.value.Intermediate;
import ir.value.Value;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class SymbolTable {
    /**
     * 符号表项，具备全局独立 id 以及对应的 IR 值，全局信息在此处体现
     * @param id 符号表项的全局独立 id
     * @param value 符号表项对应的 IR 值
     * @param global 是否为全局符号
     */
    public record SymbolTableEntry(SymbolId id, Value value, boolean global, AbstractSyntaxTree.CompileUnit initVal) {

        @Override
        public String toString() {
            if (global) {
                return "[G] " + id + " -> " + value + " <" + initVal + ">";
            }
            return id + " -> " + value + " <" + initVal + ">";
        }

        public Value emitValue() {
            String prefix = global ? "@" : "%";
            return new Intermediate(prefix + id.toString(), value.getType());
        }

        public Value emitPointer() {
            String prefix = global ? "@" : "%";
            return new Intermediate(prefix + id.toString(), new PointerType(value.getType()));
        }
    }
    /**
     * 符号表项的全局独立 id，由类型字符串和 id 组成
     * @param type 类型字符串，如 'v' -> Variable, 'c' -> Constant, 'p' -> Parameter，
     *             函数直接使用函数名
     * @param id 由全局计数器生成的 id，-1 表示库函数，-2 表示用户自定义函数
     */
    public record SymbolId(String type, int id) {
        @Override
        public String toString() {
            if (id < 0) {
                return type;
            }
            return type + id;
        }
    }
    private final SymbolTable parent;

    private final AbstractSyntaxTree.Block block;
    private final List<SymbolTable> children = new LinkedList<>();
    private final Map<SymbolId, SymbolTableEntry> table = new HashMap<>();
    public SymbolTable(SymbolTable parent) {
        this(parent, null);
    }

    public SymbolTable(SymbolTable parent, AbstractSyntaxTree.Block block) {
        this.parent = parent;
        this.block = block;
        if (this.parent != null) {
            this.parent.addChild(this);
        }
    }

    public void addChild(SymbolTable child) {
        children.add(child);
    }

    public SymbolTable getParent() {
        return parent;
    }

    public void put(SymbolTableEntry entry) {
        table.put(entry.id, entry);
    }

    public List<SymbolTableEntry> getTable() {
        return table.values().stream().toList();
    }

    public List<SymbolTable> getChildren() {
        return children;
    }

    public AbstractSyntaxTree.Block getBlock() {
        return block;
    }

    public SymbolTableEntry get(SymbolId id) {
        if (table.containsKey(id)) {
            return table.get(id);
        }
        if (parent != null) {
            return parent.get(id);
        }
        return null;
    }

    public SymbolTableEntry search(String name) {
        for (SymbolTableEntry entry : table.values()) {
            if (entry.value().getName().equals(name)) {
                return entry;
            }
        }
        if (parent != null) {
            return parent.search(name);
        }
        return null;
    }

    public SymbolTableEntry searchDown(SymbolId id) {
        if (table.containsKey(id)) {
            return table.get(id);
        }
        for (SymbolTable child : children) {
            SymbolTableEntry entry = child.searchDown(id);
            if (entry != null) {
                return entry;
            }
        }
        return null;
    }

    /**
     * 递归打印符号表及其子符号表
     * @param depth 递归深度
     * @return 符号表的字符串表示
     */
    public String debugPrint(int depth) {
        StringBuilder sb = new StringBuilder();
        sb.append("  ".repeat(depth));
        sb.append("Symbol table of block <");
        if (block != null) {
            sb.append(block.hashCode());
        } else {
            sb.append("Global");
        }
        sb.append(">\n");
        for (SymbolTableEntry entry : table.values()) {
            sb.append("  ".repeat(depth + 1));
            sb.append(entry);
            sb.append("\n");
        }
        for (SymbolTable child : children) {
            sb.append(child.debugPrint(depth + 1));
        }
        return sb.toString();
    }
}
