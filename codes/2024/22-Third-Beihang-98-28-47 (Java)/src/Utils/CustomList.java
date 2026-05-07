package Utils;

import java.util.Iterator;
import java.util.NoSuchElementException;

public class CustomList implements Iterable<CustomList.Node> {
    private Node head;
    private Node tail;
    //    private L value;
    private int size;

    private Object owner;

    public CustomList() {
//        this.value = value;
        this.head = null;
        this.tail = null;
        this.size = 0;
    }

    public CustomList(Object owner) {
//        this.value = value;
        this.head = null;
        this.tail = null;
        this.size = 0;
        this.owner = owner;
    }

    public Object getOwner() {
        return owner;
    }

    public Node getHead() {
        return head;
    }

    private void setHead(Node head) {
        this.head = head;
    }

    public Node getTail() {
        return tail;
    }

    private void setTail(Node tail) {
        this.tail = tail;
    }

//    public L getValue() {
//        return value;
//    }

//    public void setValue(L value) {
//        this.value = value;
//    }

    public int getSize() {
        return size;
    }

    private void incrementSize() {
        this.size++;
    }

    private void decrementSize() {
        this.size--;
    }

    public void addToTail(Node node) {
        if (this.tail == null) {
            this.head = this.tail = node;
        } else {
            this.tail.next = node;
            node.prev = this.tail;
            this.tail = node;
            node.next = null;
        }
        node.parent = this;
        incrementSize();
    }

    public void addToHead(Node node) {
        if (this.head == null) {
            this.head = this.tail = node;
        } else {
            this.head.prev = node;
            node.next = this.head;
            this.head = node;
            node.prev = null;
        }
        node.parent = this;
        incrementSize();
    }

    public void addCustomListToTail(CustomList other) {
        if (other == null || other.isEmpty()) {
            return; // 如果要合并的链表为空，则无需操作
        }

        if (this.isEmpty()) {
            // 如果当前链表为空，则直接使用其他链表的头和尾
            this.head = other.head;
            this.tail = other.tail;
        } else {
            // 否则，将其他链表的头追加到当前链表的尾部
            this.tail.next = other.head;
            other.head.prev = this.tail;
            this.tail = other.tail;
        }

        // 更新当前链表的大小
        this.size = this.size + other.size;

        // 更新合并后节点的父引用
        Node current = other.head;
        while (current != null) {
            current.parent = this;
            current = current.next;
        }

        // 清空其他链表的引用
//        other.head = null;
//        other.tail = null;
//        other.size = 0;
    }

//    public N getHeadValue() {
//        return getHead().getValue();
//    }

//    public N getTailValue() {
//        return getTail().getValue();
//    }

    public boolean isEmpty() {
        return size == 0;
    }

    @Override
    public Iterator<Node> iterator() {
        return new CustomIterator(this.head);
    }

    class CustomIterator implements Iterator<Node> {
        private Node current;

        CustomIterator(Node head) {
            this.current = head;
        }

        @Override
        public boolean hasNext() {
            return current != null;
        }

        @Override
        public Node next() {
            if (!hasNext()) {
                throw new NoSuchElementException();
            }
            Node temp = current;
            current = current.next;
            return temp;
        }

        @Override
        public void remove() {
            if (current == null || current.parent == null) {
                throw new IllegalStateException();
            }
            Node temp = current;
            current = current.next;
            temp.removeFromList();
        }
    }

    public static class Node {
        private Node prev;
        private Node next;
        private CustomList parent;

        public void insertAfter(Node node) {
            if (node == null) {
                throw new IllegalArgumentException("The given node cannot be null.");
            }
            this.next = node.next;
            node.next = this;
            this.prev = node;
            this.parent = node.parent;
            if (this.next != null) {
                this.next.prev = this;
            } else {
                this.parent.tail = this;
            }
            this.parent.incrementSize();
        }

        public void insertBefore(Node node) {
            if (node == null) {
                throw new IllegalArgumentException("The given node cannot be null.");
            }
            this.prev = node.prev;
            node.prev = this;
            this.next = node;
            this.parent = node.parent;
            if (this.prev != null) {
                this.prev.next = this;
            } else {
                this.parent.head = this;
            }
            this.parent.incrementSize();
        }

        public void removeFromList() {
            if (this.parent == null) {
                return;
            }
            if (this.prev != null) {
                this.prev.next = this.next;
            } else {
                this.parent.head = this.next;
            }
            if (this.next != null) {
                this.next.prev = this.prev;
            } else {
                this.parent.tail = this.prev;
            }
            this.parent.decrementSize();
        }

//        public void setValue(N value) {
//            this.value = value;
//        }

        public Node getPrev() {
            return prev;
        }

        public void setPrev(Node prev) {
            this.prev = prev;
        }

        public Node getNext() {
            return next;
        }

        public void setNext(Node next) {
            this.next = next;
        }

        public CustomList getParent() {
            return parent;
        }

        public void linked(Node oldHead) {
            oldHead.parent.size = (oldHead.parent.size + this.parent.size);
            this.next = oldHead;
            oldHead.prev = this;
            oldHead.parent.head = this.parent.head;
            Node node = this;
            while (node != null) {
                node.setParent(oldHead);
                node = node.getPrev();
            }
        }

        public void setParent(Node node) {
            this.parent = node.parent;
        }
    }
}
