package midend;

import midend.LLVMType.Type;
import midend.instr.PhiInstr;

import java.util.ArrayList;
import java.util.HashSet;

public class Value {
    private Type type;
    private String name;
    private ArrayList<Use> useList = new ArrayList<>();
    public static int num = 0;

    public Value(Type type, String name) {
        this.type = type;
        this.name = name;
    }

    public Type getType() {
        return this.type;
    }

    public String getName() {
        return this.name;
    }

    public void addUse(Use user) {
        this.useList.add(user);
    }

    public void replaceUse(Value value) {
//        if (value.name.equals("%reg59")) {
//            System.out.println("");
//        }
        for (Use use : useList) {
            User user = use.getUser();
            int pos = user.getValueList().indexOf(this);
            user.setValue(pos, value);
        }
        this.useList.clear();
    }

    public void removeUserFromUse(User user) {
        ArrayList<Use> temp = new ArrayList<>(useList);
        for (Use use : useList) {
            if (use.getUser().equals(user)) {
                temp.remove(use);
            }
        }
        this.useList = temp;
    }

    public ArrayList<Use> getUseList() {
        return useList;
    }

    public ArrayList<User> getUserList() {
        ArrayList<User> users = new ArrayList<>();
        HashSet<User> visited = new HashSet<>();
        for (Use use : useList) {
            User user = use.getUser();
            if (!visited.contains(user)) {
                users.add(user);
                visited.add(user);
            }
        }
        return users;
    }

}
