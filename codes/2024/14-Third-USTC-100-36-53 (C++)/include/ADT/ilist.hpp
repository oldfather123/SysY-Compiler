#pragma once

#include "ilist_node.hpp"
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace ADT {
template <typename T> class ilist {
    static_assert(std::is_base_of<ilist_node<T>, T>::value,
                  "T must inherit from ilist_node<T>");
    ilist_node<T> sentinel; // 哨兵节点简化边界条件的处理
    size_t count = 0;

  public:
    ilist() {
        // 初始化哨兵节点，形成一个循环链表
        sentinel.setNext(static_cast<T *>(&sentinel));
        sentinel.setPrev(static_cast<T *>(&sentinel));
    }

    ~ilist() { clear(); }

    // 禁止拷贝和赋值
    ilist(const ilist &) = delete;
    ilist &operator=(const ilist &) = delete;

    ilist(ilist &&other) noexcept {
        sentinel = other.sentinel;
        count = other.count;
        sentinel.getNext()->setPrev(static_cast<T *>(&sentinel));
        sentinel.getPrev()->setNext(static_cast<T *>(&sentinel));
        // clear other
        other.sentinel.setNext(static_cast<T *>(&other.sentinel));
        other.sentinel.setPrev(static_cast<T *>(&other.sentinel));
        other.count = 0;
    }

    ilist &operator=(ilist &&other) noexcept {
        if (this == &other)
            return *this;
        sentinel = other.sentinel;
        count = other.count;
        sentinel.getNext()->setPrev(static_cast<T *>(&sentinel));
        sentinel.getPrev()->setNext(static_cast<T *>(&sentinel));

        // clear other
        other.sentinel.setNext(static_cast<T *>(&other.sentinel));
        other.sentinel.setPrev(static_cast<T *>(&other.sentinel));
        other.count = 0;
        return *this;
    }

    // 添加节点到链表末尾
    void push_back(T *node) {
        ilist_node<T> *last = sentinel.getPrev();
        node->setNext(static_cast<T *>(&sentinel));
        node->setPrev(static_cast<T *>(last));
        last->setNext(node);
        sentinel.setPrev(node);
        count++;
    }

    // 移除链表末尾节点
    void pop_back() {
        if (!empty()) {
            ilist_node<T> *last = sentinel.getPrev();
            ilist_node<T> *new_last = last->getPrev();
            new_last->setNext(static_cast<T *>(&sentinel));
            sentinel.setPrev(static_cast<T *>(new_last));
            delete static_cast<T *>(last);
        }
        else assert(false && "Cannot pop from an empty list");
        count--;
    }

    // 在链表前端添加节点
    void push_front(T *node) {
        ilist_node<T> *first = sentinel.getNext();
        node->setNext(static_cast<T *>(first));
        node->setPrev(static_cast<T *>(&sentinel));
        first->setPrev(node);
        sentinel.setNext(node);
        count++; // 更新节点数量
    }

    // 移除链表前端节点
    void pop_front() {
        if (!empty()) {
            ilist_node<T> *first = sentinel.getNext();
            ilist_node<T> *new_first = first->getNext();
            new_first->setPrev(static_cast<T *>(&sentinel));
            sentinel.setNext(static_cast<T *>(new_first));
            delete static_cast<T *>(first);
        }
        else assert(false && "Cannot pop from an empty list");
        count--;
    }

    void insert_before(T *node, T *after) {
        T *prev = after->getPrev();
        node->setNext(after);
        node->setPrev(prev);
        prev->setNext(node);
        after->setPrev(node);
        ++count;
    }


    // 检查链表是否为空
    bool empty() const { return sentinel.getNext() == &sentinel; }

    // 获取链表中节点的数量
    size_t size() const { return count; }

    // 清空链表
    void clear() {
        while (!empty()) {
            pop_back();
        }
        count = 0;
    }

    // 从链表中移除指定的节点，但不删除它
    void remove(T *node) {
        if (node != nullptr) {
            ilist_node<T> *prev_node = node->getPrev();
            ilist_node<T> *next_node = node->getNext();
            if (prev_node != nullptr)
                prev_node->setNext(static_cast<T *>(next_node));
            if (next_node != nullptr)
                next_node->setPrev(static_cast<T *>(prev_node));

            // Reset the node's pointers
            node->setNext(nullptr);
            node->setPrev(nullptr);
        }
        else assert(false && "Cannot remove a nullptr node");
        count--;
    }

    // 删除指定的节点
    void erase(T *node) {
        remove(node);
        delete node;
    }

    // 返回最后一个元素的可修改引用

    T &back() {
        if (empty()) {
            assert(false && "Cannot access back of an empty list");
        }
        return *static_cast<T *>(sentinel.getPrev());
    }

    // 返回最后一个元素的常量引用
    const T &back() const {
        if (empty()) {
            assert(false && "Cannot access back of an empty list");
        }
        return *static_cast<const T *>(sentinel.getPrev());
    }

    T& front() {
        assert(!empty() && "Cannot access front of an empty list");
        return *static_cast<T*>(sentinel.getNext());
    }

    const T& front() const {
        assert(!empty() && "Cannot access front of an empty list");
        return *static_cast<T*>(sentinel.getNext());
    }


    // 定义迭代器
    class iterator {
        friend class ilist<T>;
        T *node;

      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = value_type *;
        using reference         = value_type &;

        explicit iterator(T *n) : node(n) {}

        T &operator*() const { return *node; }
        T *operator->() const { return node; }

        iterator &operator++() {
            node = node->getNext();
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator &other) const {
            return node == other.node;
        }
        bool operator!=(const iterator &other) const {
            return node != other.node;
        }
    };

    iterator begin() { return iterator(sentinel.getNext()); }
    iterator end() { return iterator(static_cast<T *>(&sentinel)); }

    class reverse_iterator {
        friend class ilist<T>;
        T *node;

      public:
        explicit reverse_iterator(T *n) : node(n) {}

        T &operator*() const { return *node; }
        T *operator->() const { return node; }

        reverse_iterator &operator++() {
            node = node->getPrev();
            return *this;
        }

        reverse_iterator operator++(int) {
            reverse_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const reverse_iterator &other) const {
            return node == other.node;
        }
        bool operator!=(const reverse_iterator &other) const {
            return node != other.node;
        }
    };

    reverse_iterator rbegin() { return reverse_iterator(sentinel.getPrev()); }

    reverse_iterator rend() {
        return reverse_iterator(static_cast<T *>(&sentinel));
    }

    // 插入节点到指定位置，即迭代器指向的节点之前
    void insert(iterator pos, T *node) {
        ilist_node<T> *next_node = pos.node;
        ilist_node<T> *prev_node = next_node->getPrev();

        node->setNext(static_cast<T *>(next_node));
        node->setPrev(static_cast<T *>(prev_node));
        if (prev_node != nullptr) {
            prev_node->setNext(node);
        }
        next_node->setPrev(node);

        count++;
    }

    void insert(reverse_iterator pos, T *node) {
        ilist_node<T> *next_node = pos.node;
        ilist_node<T> *prev_node = next_node->getPrev();

        node->setNext(static_cast<T *>(next_node));
        node->setPrev(static_cast<T *>(prev_node));
        if (prev_node != nullptr) {
            prev_node->setNext(node);
        }
        next_node->setPrev(node);

        count++;
    }

};

}; // namespace ADT
