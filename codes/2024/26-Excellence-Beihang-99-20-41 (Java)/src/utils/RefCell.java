package utils;

/**
 * 用于包装可变对象，以便在 record 中修改引用对象
 */
public class RefCell<T> {
    private T value;

    public RefCell(T value) {
        this.value = value;
    }

    public T get() {
        return value;
    }

    public void set(T value) {
        this.value = value;
    }

    public T getNotNull() {
        if (value == null) {
            throw new RuntimeException("RefCell is null");
        }
        return value;
    }

    @Override
    public String toString() {
        return value != null ? value.toString() : "null";
    }

    @Override
    public int hashCode() {
        return value != null ? value.hashCode() : 0;
    }
}
