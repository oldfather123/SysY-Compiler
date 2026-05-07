package util.nodelist;


import java.util.Collection;

@SuppressWarnings({"UnusedReturnValue", "unused"})
public class Node<T> {
    T data;
    Node<T> prev, next;
    boolean isRemoved;

    Node(T e) {
        data = e;
    }

    public T get() {
        if (isHead()) throw new IllegalNodeAccessException("禁止对头节点使用 get 方法");
        else if (isTail()) throw new IllegalNodeAccessException("禁止对尾节点使用 get 方法");

        return data;
    }

    public void set(T e) {
        if (isHead()) throw new IllegalNodeAccessException("禁止对头节点使用 set 方法");
        else if (isTail()) throw new IllegalNodeAccessException("禁止对尾节点使用 set 方法");

        data = e;
    }

    public Node<T> prev() {
        if (isHead()) return null;

        return prev;
    }

    public Node<T> next() {
        if (isTail()) return null;

        return next;
    }

    /**
     * 在节点之前插入新节点
     *
     * @param e 新节点中的数据
     * @return 新节点的引用
     * @throws IllegalNodeAccessException 如果尝试在头节点前插入
     */
    public Node<T> insertBefore(T e) {
        if (isHead()) throw new IllegalNodeAccessException("不可对头节点调用 insertBefore 方法");

        return this.prev.insertAfter(e);
    }

    /**
     * 在节点之后插入新节点
     *
     * @param e 新节点中的数据
     * @return 新节点的引用
     * @throws IllegalNodeAccessException 如果尝试在尾节点后插入
     */
    public Node<T> insertAfter(T e) {
        if (isTail()) throw new IllegalNodeAccessException("不可对尾节点调用 insertAfter 方法");

        Node<T> newNode = new Node<>(e);
        newNode.prev = this;
        newNode.next = this.next;
        this.next.prev = newNode;
        this.next = newNode;
        return newNode;
    }

    /**
     * 在节点之前插入一组数据
     *
     * @param collection 可迭代的一组数据
     * @throws IllegalNodeAccessException 如果尝试在头节点前插入
     */
    public void insertAllBefore(Collection<? extends T> collection) {
        if (isHead()) throw new IllegalNodeAccessException("不可对头节点调用 insertAllBefore 方法");

        Node<T> p = this.prev;
        for (var e : collection) {
            p = p.insertAfter(e);
        }
    }

    /**
     * 在此节点之后插入一组数据
     *
     * @param collection 可迭代的一组数据
     * @throws IllegalNodeAccessException 如果尝试在头节点前插入
     */
    public void insertAllAfter(Collection<? extends T> collection) {
        if (isTail()) throw new IllegalNodeAccessException("不可对尾节点调用 insertAllAfter 方法");

        Node<T> p = this;
        for (var e : collection) {
            p = p.insertAfter(e);
        }
    }

    /**
     * 在链表中删除一个节点
     *
     * @throws IllegalNodeAccessException 如果尝试删除头尾节点
     */
    public void remove() {
        if (isHead()) throw new IllegalNodeAccessException("不可对头节点调用 removeNode 方法");
        else if (isTail()) throw new IllegalNodeAccessException("不可对尾节点调用 removeNode 方法");

        this.prev.next = this.next;
        this.next.prev = this.prev;
        isRemoved = true;
    }

    private boolean isHead() {
        return prev == null;
    }

    private boolean isTail() {
        return next == null;
    }
}
