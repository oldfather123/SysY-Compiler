package tools;

public class DoubleNode<T> {
    private DoubleNode<T> pred;
    private DoubleNode<T> succ;
    private T content;
    private DoubleLinkedList<T> parentList;


    public DoubleNode(T content) {
        this.content = content;
        pred = null;
        succ = null;
        parentList = null;
    }

    public DoubleNode<T> getPred() {
        return pred;
    }

    public void setPred(DoubleNode<T> pred) {
        this.pred = pred;
    }

    public DoubleNode<T> getSucc() {
        return succ;
    }

    public void setSucc(DoubleNode<T> succ) {
        this.succ = succ;
    }

    public T getContent() {
        return content;
    }

    public DoubleLinkedList<T> getParentList() {
        return parentList;
    }

    public void setParentList(DoubleLinkedList<T> parentList) {
        this.parentList = parentList;
    }
}
