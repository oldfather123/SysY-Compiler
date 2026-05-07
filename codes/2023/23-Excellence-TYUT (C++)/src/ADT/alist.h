//
// Created by 牛奕博 on 2023/4/17.
//

#ifndef ANUC_ALIST_H
#define ANUC_ALIST_H
//test
#include <iostream>

using namespace std;

namespace anuc {
    struct base_node {
        base_node *prev{nullptr};
        base_node *next{nullptr};

        base_node() = default;

        base_node(base_node *prev, base_node *next) : prev(prev), next(next) {}

        void erase() {
            prev->next = next;
            next->prev = prev;
            next = nullptr;
            prev = nullptr;
        }
    };

    template<class NodeTy>
    class alist_node : public base_node {

    public:
        alist_node() = default;

        alist_node &operator=(const alist_node &) = delete;
    };

    template<class NodeTy, class ParentTy>
    class alist_node_with_parent : public alist_node<NodeTy> {
        //让子类必须实现getParent，否则编译报错
//        const ParentTy *getParentNode() const {
//            return static_cast<const NodeTy *>(this)->getParent();
//        }
    public:
        virtual void eraseFromParent() {}
    };

/*------------------------------------------------------------------------------------------*/
//将节点封装为链表
    template<class NodeTy>
    class alist {
        //头节点和尾节点都为空值，有值的节点在两者之间
        base_node head;
        base_node tail;
        unsigned length{0};
    public:
        //迭代器
        class iterator {
            base_node *ptr{nullptr};
        public:
            iterator(base_node *ptr) : ptr(ptr) {}

            iterator(const iterator &) = delete;

            iterator &operator=(const iterator &) = delete;

            NodeTy &operator*() {
                return *(static_cast<NodeTy *>(ptr));
            }

            iterator &operator+(int n) {
                while (n--) ptr = ptr->next;
                return *this;
            }

            iterator &operator-(int n) {
                while (n--) ptr = ptr->prev;
                return *this;
            }

            iterator &operator++() {
                ptr = ptr->next;
                return *this;
            }

            iterator &operator--() {
                ptr = ptr->prev;
                return *this;
            }

            bool operator!=(iterator const &that) {
                return ptr != that.ptr;
            }

            bool operator==(iterator const &that) {
                return ptr == that.ptr;
            }

        };

        alist &operator=(alist const &other) = delete;

        alist() : head(), tail() {
            head.next = &tail;
            tail.prev = &head;
        }

        void listMove(alist &other) {
            head.next = other.head.next;
            head.next->prev = &head;
            tail.prev = other.tail.prev;
            tail.prev->next = &tail;
            other.head.next = &other.tail;
            other.tail.prev = &other.head;
        }

        bool empty() {
            if (head.next == &tail) return true;
            else return false;
        }

        iterator begin() {
            return iterator(head.next);
        }

        //返回尾部节点
        iterator end() {
            return iterator(&tail);
        }

        //back节点，tail的prev
        iterator back() {
            return iterator(tail.prev);
        }

        //头节点（为空）
        iterator front() {
            return iterator(&head);
        }

        void insert_front(base_node *node) {
            node->prev = &head;
            node->next = head.next;
            head.next->prev = node;
            head.next = node;
            ++length;
        }

        void insert_back(base_node *node) {
            node->prev = tail.prev;
            node->next = &tail;
            tail.prev->next = node;
            tail.prev = node;
            ++length;
        }

        void erase_from_list(base_node *node) {
            node->erase();
            --length;
        }

        //把first插入进second前面
        void insert_into(base_node *inst, base_node *insert_point) {
            inst->prev = insert_point->prev;
            inst->next = insert_point;
            insert_point->prev = inst;
            inst->prev->next = inst;
        }

        void insert_into_back(base_node *insert_point, base_node *inst) {
            inst->prev = insert_point;
            inst->next = insert_point->next;
            insert_point->next = inst;
            inst->next->prev = inst;
        }


        unsigned getLength() { return length; }

        ~alist() = default;

    };


}

#endif //ANUC_ALIST_H
