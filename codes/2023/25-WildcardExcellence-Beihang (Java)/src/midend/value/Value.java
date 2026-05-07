package midend.value;

import midend.Type;
import midend.Use;
import util.nodelist.NodeList;

public class Value {
    public static int REG_NUM = 0;
    public static int LOC_NUM = 0;
    public static int GLO_NUM = 0;
    public static int STR_NUM = 0;
    public static int BLOCK_NUM = 0;
    public static int FR_NUM = 0;
    public static boolean flag = true;
    private Type initialType;

    public void setInitialType(Type type) {
        this.initialType = type;
    }

    public Type getInitialType() {
        return initialType;
    }

    private Type type;
    private String name;

    public NodeList<Use> getUsedOpList() {
        return usedOpList;
    }

    /**
     * 将所有使用该value的使用者都改为使用other，使用者记录的 use 本质上没有变
     * @param other
     */
    public void changeAllUse2UseOther(Value other) {
        flag = true;
        for (var useNode : usedOpList) {
            Use use = useNode.get();
            // 使用者对应位置的操作值修改为 other
            use.getUser().getOperandList().set(use.getPosition(), other);
            // 被使用者 other 要记录该 use
            other.addUsedOp(use);
            // 不再记录该 use
            useNode.remove();
        }
    }

    /**
    *指令曾作为被使用者，记录了Use在UsedList中，现在使用者已经被去除，则该Use应无效
     */
    public void removeUseFromUsedList(User user) {
        this.usedOpList.removeIf(it -> it.getUser() == user);
    }

    /**
     * 该value 被使用时的 use 形式，可以获取user和其在user OpList中的位置
     */
    private NodeList<Use> usedOpList;

    public void addUsedOp(Use used) {
        this.usedOpList.addLast(used);
    }

    public Value(Type type, String name) {
        this.type = type;
        this.name = name;
        this.usedOpList = new NodeList<>();
    }

    public Value(Type type) {
        this.type = type;
        this.name = "%reg" + REG_NUM++;
        this.usedOpList = new NodeList<>();
    }

    public Type getType() {
        return type;
    }

    public void setType(Type type) {
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public String getEleName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }
}
