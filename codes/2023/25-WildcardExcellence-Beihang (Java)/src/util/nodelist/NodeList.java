package util.nodelist;

import java.util.Collection;
import java.util.Iterator;
import java.util.function.Consumer;
import java.util.function.Predicate;


@SuppressWarnings({"UnusedReturnValue", "unused"})
public class NodeList<T> implements Iterable<Node<T>> {
    Node<T> head, tail;

    /**
     * 构造一个空链表。
     */
    public NodeList() {
        head = new Node<>(null);
        tail = new Node<>(null);
        head.next = tail;
        tail.prev = head;
    }

    /**
     * 从一个集合构造链表
     *
     * @param collection 元素集合
     */
    public NodeList(Collection<? extends T> collection) {
        this();
        addAllLast(collection);
    }

    public boolean isEmpty() {
        return head.next == tail;
    }

    public T firstElement() {
        if (isEmpty()) return null;
        else return firstNode().get();
    }

    public T lastElement() {
        if (isEmpty()) return null;
        else return lastNode().get();
    }

    /**
     * 在列表开头插入新节点
     *
     * @param e 新节点中的数据
     * @return 新节点的引用
     */
    public Node<T> addFirst(T e) {
        return head.insertAfter(e);
    }

    /**
     * 在列表末尾插入新节点
     *
     * @param e 新节点中的数据
     * @return 新节点的引用
     */
    public Node<T> addLast(T e) {
        return tail.insertBefore(e);
    }

    /**
     * 在链表开头插入一组数据
     *
     * @param collection 可迭代的一组数据
     */
    public void addAllFirst(Collection<? extends T> collection) {
        head.insertAllAfter(collection);
    }

    /**
     * 在链表末尾插入一组数据
     *
     * @param collection 可迭代的一组数据
     */
    public void addAllLast(Collection<? extends T> collection) {
        tail.prev.insertAllAfter(collection);
    }

    /**
     * 弹出第一个元素并返回，如果链表为空返回 null
     *
     * @return 第一个元素，或 null
     */
    public T pollFirst() {
        if (isEmpty()) return null;
        else {
            T ret = head.next.data;
            head.next.remove();
            return ret;
        }
    }

    /**
     * 弹出最后一个元素并返回，如果链表为空返回 null
     *
     * @return 最后一个元素，或 null
     */
    public T pollLast() {
        if (isEmpty()) return null;
        else {
            T ret = tail.prev.data;
            tail.prev.remove();
            return ret;
        }
    }

    public void clear() {
        head.next = tail;
        tail.prev = head;
    }

    /**
     * 遍历一次链表，删除所有元素符合条件的节点
     *
     * @param cond 元素的条件
     */
    public void removeIf(Predicate<T> cond) {
        for (var p : this) {
            if (cond.test(p.get())) {
                p.remove();
            }
        }
    }

    public Node<T> firstNode() {
        return head.next;
    }

    public Node<T> lastNode() {
        return tail.prev;
    }

    public Node<T> headNode() {
        return head;
    }

    public Node<T> tailNode() {
        return tail;
    }

    public boolean hasNext(Node<T> p) {
        return p != tail && p.next != tail;
    }

    public boolean hasPrev(Node<T> p) {
        return p != head && p.prev != head;
    }

    /**
     * 正序遍历链表
     *
     * @param func 每次循环的回调函数，接收一个参数 p，表示当前节点
     */
    @Override
    public void forEach(Consumer<? super Node<T>> func) {
        for (var p = head.next; p != tail; p = p.next) {
            func.accept(p);
        }
    }

    /**
     * 逆序遍历链表
     *
     * @param func 每次循环的回调函数，接收一个参数 p，表示当前节点
     */
    public void forEachReverse(Consumer<? super Node<T>> func) {
        for (var p = tail.prev; p != head; p = p.prev) {
            func.accept(p);
        }
    }

    /**
     * 从节点开始正序遍历到链表末尾
     *
     * @param node 节点
     * @param func 每次循环的回调函数，接收一个参数 p，表示当前节点
     */
    public void forEachRemaining(Node<T> node, Consumer<? super Node<T>> func) {
        for (var p = node; p != tail; p = p.next) {
            func.accept(p);
        }
    }

    /**
     * 从节点开始逆序遍历到链表开头
     *
     * @param node 节点
     * @param func 每次循环的回调函数，接收一个参数 p，表示当前节点
     */
    public void forEachRemainingReverse(Node<T> node, Consumer<? super Node<T>> func) {
        for (var p = node; p != head; p = p.prev) {
            func.accept(p);
        }
    }

    /**
     * @deprecated 只是为了让 list 支持增强的 for 循环语法才实现的 Iterable 接口。正常情况下操作 Node 就可以了。
     */
    @Override
    @Deprecated
    public Iterator<Node<T>> iterator() {
        return new NodeListIterator<>(head, tail);
    }

    /**
     * 获取 SubList，这只是由两个端点（闭区间）组成的“视图”，不会复制两端点之间的节点。
     * 请保证，first 端点能够正向遍历到 last
     *
     * @param first SubList 的第一个节点
     * @param last  SubList 的最后一个节点
     * @return 创建的 SubList
     */
    public NodeList<T> subList(Node<T> first, Node<T> last) {
        if (first == head || first == tail || last == head || last == tail) {
            throw new IllegalNodeAccessException("SubList 的端点不能是头尾节点");
        }
        return new SubNodeList<>(first, last);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("[");
        boolean first = true;
        for (var p : this) {
            if (first) {
                sb.append(p.get().toString());
                first = false;
            }
            else {
                sb.append(", ").append(p.get().toString());
            }
        }
        sb.append(']');
        return sb.toString();
    }
}
