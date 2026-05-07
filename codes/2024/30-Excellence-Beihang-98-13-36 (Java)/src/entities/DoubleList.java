package entities;

import java.util.Collection;
import java.util.function.Consumer;

public class DoubleList<T> {

    private int size = 0;
    private final Node head, tail;

    public DoubleList() {
        head = new Node(null);
        tail = new Node(null);
        head.next = tail;
        tail.prev = head;
    }

    public void addHead(T obj) {
        size++;
        Node node = new Node(obj);
        Node next = head.next;
        head.next = node;
        next.prev = node;
        node.prev = head;
        node.next = next;
    }

    public void addTail(T obj) {
        size++;
        Node node = new Node(obj);
        Node prev = tail.prev;
        tail.prev = node;
        prev.next = node;
        node.prev = prev;
        node.next = tail;
    }

    public void add(T obj) {
        addTail(obj);
    }

    public void addAll(Collection<T> list) {
        for (T obj : list) {
            add(obj);
        }
    }

    public void addAll(DoubleList<T> list) {
        Iterator iter = list.iterator();
        while (iter.hasNext()) {
            add(iter.next());
        }
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public T getHead() {
        if (head.next.obj == null) {
            throw new RuntimeException("No Item In List");
        }
        return head.next.obj;
    }

    public T getTail() {
        if (tail.prev.obj == null) {
            throw new RuntimeException("No Item In List");
        }
        return tail.prev.obj;
    }

    public void clear() {
        size = 0;
        head.next = tail;
        tail.prev = head;
    }

    public Iterator iterator() {
        return new Iterator(head, this);
    }

    public Iterator copyIterator(Iterator iter) {
        return new Iterator(iter);
    }

    public void forEach(Consumer<T> consumer) {
        Iterator iter = iterator();
        while (iter.hasNext()) {
            consumer.accept(iter.next());
        }
    }

    @SafeVarargs
    public static <T> DoubleList<T> of(T... objs) {
        return new DoubleList<>() {{
            for (T obj : objs) {
                add(obj);
            }
        }};
    }

    public class Node {
        T obj;
        Node prev, next;

        Node(T obj) {
            this.obj = obj;
        }
    }

    public class Iterator {
        Node node;
        final DoubleList<T> doubleList;

        Iterator(Node node, DoubleList<T> doubleList) {
            this.node = node;
            this.doubleList = doubleList;
        }

        Iterator(Iterator iter) {
            this.node = iter.node;
            this.doubleList = iter.doubleList;
        }

        public T next() {
            node = node.next;
            return node.obj;
        }

        public T nextNotMove() {
            return node.next.obj;
        }

        public boolean hasNext() {
            return node.next != null && node.next.obj != null;
        }

        public void prev() {
            node = node.prev;
        }

        public void remove() {
            node.prev.next = node.next;
            node.next.prev = node.prev;
            doubleList.size--;
        }

        public void add(T obj) {
            Node newNode = new Node(obj);
            Node next = node.next;
            Node prev = node.next.prev;
            next.prev = newNode;
            prev.next = newNode;
            newNode.next = next;
            newNode.prev = prev;
            node = newNode;
            doubleList.size++;
        }

        public void set(T obj) {
            node.obj = obj;
        }

        public T getObj() {
            return node.obj;
        }
    }

}
