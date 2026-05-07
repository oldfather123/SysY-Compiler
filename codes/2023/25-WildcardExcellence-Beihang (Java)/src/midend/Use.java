package midend;

import midend.value.User;

public class Use {
    private User user;

    private int position;

    public Use(User user, int position) {
        this.user = user;
        this.position = position;
    }

    public User getUser() {
        return user;
    }

    public int getPosition() {
        return position;
    }

    public void setUser(User user) {
        this.user = user;
    }

    public void setPosition(int position) {
        this.position = position;
    }
}
