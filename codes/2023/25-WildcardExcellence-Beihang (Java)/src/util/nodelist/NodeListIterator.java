package util.nodelist;

import java.util.Iterator;

/**
 * 只是为了让 list 支持增强的 for 循环语法才实现的 Iterator 接口。不对外暴露
 *
 * @param <T> 元素类型
 */
class NodeListIterator<T> implements Iterator<Node<T>> {
    private Node<T> cur;
    private final Node<T> tail;

    public NodeListIterator(Node<T> cur, Node<T> tail) {
        if (tail.isRemoved) throw new IllegalNodeAccessException("尾节点被删除，遍历此 SubList 将越界");
        this.cur = cur;
        this.tail = tail;
    }

    @Override
    public boolean hasNext() {
        return cur != tail && cur.next != tail;
    }

    @Override
    public Node<T> next() {
        cur = cur.next;
        return cur;
    }
}
