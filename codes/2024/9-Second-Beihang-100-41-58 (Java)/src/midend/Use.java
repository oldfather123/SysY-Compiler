package midend;

public class Use {
    private Value value;
    private User user;

    public Use(Value value, User user) {
        this.value = value;
        this.user = user;
    }

    public Value getValue() {
        return this.value;
    }

    public User getUser() {
        return this.user;
    }


}
