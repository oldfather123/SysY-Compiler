package util;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

public class CustomList implements Iterable<CustomList.Node> {
    
    private Node head;
    private Node tail;
    private int size;
    private final Object owner;
    
    public CustomList(Object owner) {
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
    
    public Node getTail() {
        return tail;
    }
    
    public int getSize() {
        return size;
    }
    
    public void incrementSize() {
        this.size++;
    }
    
    public void decrementSize() {
        this.size--;
    }
    
    public boolean isEmpty() {
        return this.size == 0;
    }
    
    public void addToTail(Node node) {
        if (this.tail == null) {
            this.head = this.tail = node;
            node.prev = node.next = null;
        } else {
            this.tail.next = node;
            node.prev = this.tail;
            this.tail = node;
            node.next = null;
        }
        node.parent = this;
        size++;
    }
    
    public void addToHead(Node node) {
        if (this.head == null) {
            this.head = this.tail = node;
            node.prev = node.next = null;
        } else {
            this.head.prev = node;
            node.next = this.head;
            this.head = node;
            node.prev = null;
        }
        node.parent = this;
        size++;
    }
    
    public void addCustomListToTail(CustomList other) {
        if (other == null || other.isEmpty()) {
            return; // 如果要合并的链表为空，则无需操作
        }
        
        if (this.isEmpty()) {
            // 如果当前链表为空，则直接使用其他链表的头
            this.head = other.head;
        } else {
            // 否则，将其他链表的头追加到当前链表的尾部
            this.tail.next = other.head;
            other.head.prev = this.tail;
        }
        this.tail = other.tail;
        
        // 更新当前链表的大小
        this.size = this.size + other.size;
        
        // 更新合并后节点的父引用
        Node current = other.head;
        while (current != null) {
            current.parent = this;
            current = current.next;
        }
    }
    
    @Override
    public Iterator<Node> iterator() {
        return new CustomIterator(this.head);
    }
    
    public static class CustomIterator implements Iterator<Node> {
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
        
        public void setParent(Node node) {
            this.parent = node.parent;
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
        
        /**
         * 获取从自己开始到 node 的连续 Node 列表，如果找不到这样的列表抛异常
         * 返回列表包括两端 (this & node)
         */
        public List<Node> getSubListTo(Node node) {
            List<Node> subList = new ArrayList<>();
            Node current;
            for (current = this; current != null && current != node; current = current.next) {
                subList.add(current);
            }
            if (current == node) {
                subList.add(node);
                return subList;
            } else {
                throw new RuntimeException("找不到要求的列表");
            }
        }
    }
}
