package util.nodelist;

import java.util.function.Consumer;

public class SubNodeList<T> extends NodeList<T>{
    // 非 public 的构造函数，只能使用 NodeList.subList 方法构造
    SubNodeList(Node<T> first, Node<T> last) {
        super.head = first.prev;
        super.tail = last.next;
    }

    // 开始循环前，检验一下端点是否被删除

    @Override
    public void forEach(Consumer<? super Node<T>> func) {
        // 不检查 head（first.prev），
        // 因为 head 即使被删除，若此 SubList 存在，它不会被 GC，
        // head.next 依然可以访问，但若 tail 被删除，就找不到停止节点了。
        if (tail.isRemoved) throw new IllegalNodeAccessException("尾节点被删除，遍历此 SubList 将越界");
        super.forEach(func);
    }

    @Override
    public void forEachReverse(Consumer<? super Node<T>> func) {
        if (head.isRemoved) throw new IllegalNodeAccessException("头节点被删除，遍历此 SubList 将越界");
        super.forEachReverse(func);
    }

    @Override
    public void forEachRemaining(Node<T> node, Consumer<? super Node<T>> func) {
        if (tail.isRemoved) throw new IllegalNodeAccessException("尾节点被删除，遍历此 SubList 将越界");
        super.forEachRemaining(node, func);
    }

    @Override
    public void forEachRemainingReverse(Node<T> node, Consumer<? super Node<T>> func) {
        if (head.isRemoved) throw new IllegalNodeAccessException("头节点被删除，遍历此 SubList 将越界");
        super.forEachRemainingReverse(node, func);
    }
}
