package cn.edu.bit.newnewcc.util;

import java.util.*;

/**
 * 可重集合
 *
 * @param <E> 集合中元素的类型
 */
public class HashMultiset<E> implements Iterable<E> {

    private class MultisetIterator implements Iterator<E> {

        private final Iterator<Map.Entry<E, Integer>> mapIterator;
        private Map.Entry<E, Integer> currentEntry;
        private int consumeCount;

        public MultisetIterator(Iterator<Map.Entry<E, Integer>> mapIterator) {
            this.mapIterator = mapIterator;
            bufferNextEntry();
        }

        @Override
        public boolean hasNext() {
            return consumeCount != -1;
        }

        @Override
        public E next() {
            if (consumeCount == -1) {
                throw new NoSuchElementException();
            }
            E e = currentEntry.getKey();
            consumeCount++;
            if (consumeCount == currentEntry.getValue()) {
                bufferNextEntry();
            }
            return e;
        }

        /**
         * 将下一个键值对缓存到当前iterator
         */
        private void bufferNextEntry() {
            if (mapIterator.hasNext()) {
                currentEntry = mapIterator.next();
                consumeCount = 0;
            } else {
                currentEntry = null;
                consumeCount = -1;
            }
        }
    }

    private final HashMap<E, Integer> hashMap = new HashMap<>();

    public boolean contains(E e) {
        return hashMap.containsKey(e);
    }

    /**
     * 向集合中添加一个元素
     *
     * @param e 待添加的元素
     * @return 总是返回true，表示添加成功
     */
    public boolean add(E e) {
        if (hashMap.containsKey(e)) {
            hashMap.put(e, hashMap.get(e) + 1);
        } else {
            hashMap.put(e, 1);
        }
        return true;
    }

    /**
     * 从集合中移除一个元素（若元素出现了多次，只会移除一个）
     *
     * @param e 待移除的元素
     * @return 若移除成功，返回true；否则返回false。
     */
    public boolean remove(E e) {
        if (hashMap.containsKey(e)) {
            int v = hashMap.get(e);
            if (v == 1) {
                hashMap.remove(e);
            } else {
                hashMap.put(e, v - 1);
            }
            return true;
        } else {
            return false;
        }
    }

    /**
     * 从集合中移除一个元素（若元素出现了多次，则全部移除）
     *
     * @param e 待移除的元素
     * @return 成功移除元素的数量（当元素不存在时，返回0）
     */
    public int removeAll(E e) {
        if (hashMap.containsKey(e)) {
            return hashMap.remove(e);
        } else {
            return 0;
        }
    }

    /**
     * 获取当前集合的一个ArrayList版复制
     *
     * @return 当前集合内元素构成的ArrayList
     */
    public ArrayList<E> getArrayCopy() {
        var array = new ArrayList<E>();
        for (E e : this) {
            array.add(e);
        }
        return array;
    }

    @Override
    public Iterator<E> iterator() {
        return new MultisetIterator(hashMap.entrySet().iterator());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        HashMultiset<?> that = (HashMultiset<?>) o;
        return hashMap.equals(that.hashMap);
    }

    @Override
    public int hashCode() {
        return Objects.hash(hashMap);
    }
}
