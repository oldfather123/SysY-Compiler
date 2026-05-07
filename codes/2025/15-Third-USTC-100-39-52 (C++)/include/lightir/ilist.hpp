#pragma once
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <list>
#include "../common/logging.hpp"

template <typename T>
class ilist
{
public:
    class node
    {
        friend class ilist<T>;

    private:
        T *prev_{nullptr};
        T *next_{nullptr};
        size_t tag_{0};

    public:
        node() = default;
        virtual ~node() = 0;

        node(const node &) = delete;
        node &operator=(const node &) = delete;
        size_t tag() const { return tag_; }
    };

    // 双向链表迭代器
    template <typename elem, bool reverse = false>
    class my_iterator
    {
        friend class ilist<T>;

    public:
        using iterator_type = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = elem;
        using ptr = value_type *;
        using ref = value_type &;

        my_iterator(ptr p) : ptr_(p) {}

        ref operator*() const { return *ptr_; }
        ptr operator->() const { return ptr_; }

        bool operator==(const my_iterator &rhs) const
        {
            return ptr_ == rhs.ptr_;
        }
        bool operator!=(const my_iterator &rhs) const
        {
            return ptr_ != rhs.ptr_;
        }

        my_iterator &operator--()
        {
            ptr_ = reverse ? ptr_->next_ : ptr_->prev_;
            return *this;
        }

        my_iterator &operator++()
        {
            ptr_ = reverse ? ptr_->prev_ : ptr_->next_;
            return *this;
        }

        my_iterator operator--(int)
        {
            auto ret = *this;
            ptr_ = reverse ? ptr_->next_ : ptr_->prev_;
            return ret;
        }

        my_iterator operator++(int)
        {
            auto ret = *this;
            ptr_ = reverse ? ptr_->prev_ : ptr_->next_;
            return ret;
        }

    private:
        ptr ptr_{nullptr};
    };

    using iterator = my_iterator<T>;
    using reverse_iterator = my_iterator<T, true>;
    using const_iterator = my_iterator<const T>;
    using const_reverse_iterator = my_iterator<const T, true>;

private:
    T *head_{nullptr};
    T *tail_{nullptr};
    size_t size_{0};
    size_t tag_{0};

    void mark_node(T *p) { p->tag_ = tag_; }
    void unmark_node(T *p) { p->tag_ = 0; }

    bool is_node(T *p) { return p->tag_ == tag_; }

    static size_t alloc_tag()
    {
        static size_t next_tag{0};
        next_tag++;
        return next_tag;
    }

    // 通用的节点插入方法
    void insert_node(T *p, T *prev, T *next)
    {
        p->prev_ = prev;
        p->next_ = next;
        prev->next_ = p;
        next->prev_ = p;
        mark_node(p);
        size_++;
    }

    // 通用的节点删除方法
    void remove_node(T *p)
    {
        p->prev_->next_ = p->next_;
        p->next_->prev_ = p->prev_;
        unmark_node(p);
        size_--;
        // delete p;
    }

public:
    ilist()
    {
        head_ = static_cast<T *>(::operator new(sizeof(T)));
        tail_ = static_cast<T *>(::operator new(sizeof(T)));
        head_->next_ = tail_;
        tail_->prev_ = head_;
        tag_ = alloc_tag();
        mark_node(head_);
        mark_node(tail_);
    }

    ~ilist()
    {
        while (size_ > 0)
        {
            pop_back();
        }
        ::operator delete(head_);
        ::operator delete(tail_);
    }

    iterator begin() { return iterator(head_->next_); }
    iterator end() { return iterator(tail_); }
    reverse_iterator rbegin() { return reverse_iterator(tail_->prev_); }
    reverse_iterator rend() { return reverse_iterator(head_); }

 

    size_t size() const { return size_; }
    size_t tag() const { return tag_; }

    void push_back(T *p)
    {
        if (is_node(p))
        {
            throw std::runtime_error("node already in ilist");
        }
        insert_node(p, tail_->prev_, tail_);
    }

    template <typename... Args>
    void emplace_back(Args &&...args)
    {
        push_back(new T(std::forward<Args>(args)...));
    }

    void push_front(T *p)
    {
        if (is_node(p))
        {
            throw std::runtime_error("node already in ilist");
        }
        insert_node(p, head_, head_->next_);
    }

    template <typename... Args>
    void emplace_front(Args... args)
    {
        push_front(new T{args...});
    }

    void pop_back()
    {
        if (size_ == 0)
        {
            throw std::runtime_error("pop_back on empty ilist");
        }
        remove_node(tail_->prev_);
    }

    void pop_front()
    {
        if (size_ == 0)
        {
            throw std::runtime_error("pop_front on empty ilist");
        }
        remove_node(head_->next_);
    }

    // 删除节点
    T *remove(const iterator &it)
    {
        auto p = it.ptr_;
        if (p == head_ || p == tail_)
        {
            throw std::runtime_error("release head or tail");
        }
        else if (!is_node(p))
        {
            throw std::runtime_error("release a node not in the list");
        }
        T *next = p->next_;
        remove_node(p);
        return dynamic_cast<T *>(next); // 返回下一个节点
    }

    iterator erase(const iterator &it)
    {
        auto p = it.ptr_;
        if (p == head_ || p == tail_)
        {
            throw std::runtime_error("erase head or tail");
        }
        else if (!is_node(p))
        {
            throw std::runtime_error("erase a node not in the list");
        }
        auto ret = iterator{p->next_};
        remove_node(p);
        return ret;
    }

    T *get_prev(const iterator &it)
    {
        auto p = it.ptr_;
        if (p == head_)
        {
            throw std::runtime_error("get_prev on head");
        }
        else if (!is_node(p))
        {
            throw std::runtime_error("get_prev a node not in the list");
        }
        return p->prev_;
    }

    T *get_next(const iterator &it)
    {
        auto p = it.ptr_;
        if (p == tail_)
        {
            throw std::runtime_error("get_next on tail");
        }
        else if (!is_node(p))
        {
            throw std::runtime_error("get_next a node not in the list");
        }
        return p->next_;
    }

    void insert_after(const iterator &it, T *p)
    {
        if (is_node(p))
        {
            throw std::runtime_error("node already in ilist");
        }
        insert_node(p, it.ptr_, it.ptr_->next_);
    }

    void insert_before(const iterator &it, T *p)
    {
        if (is_node(p))
        {
            throw std::runtime_error("node already in ilist");
        }
        insert_node(p, it.ptr_->prev_, it.ptr_);
    }

    bool empty() const { return size_ == 0; }

    template <typename... Args>
    iterator emplace(const iterator &it, Args &&...args)
    {
        return insert(it, new T(std::forward<Args>(args)...));
    }

    T &front()
    {
        if (size_ == 0)
        {
            throw std::runtime_error("front on empty ilist");
        }
        return *(head_->next_);
    }

    T &back()
    {
        if (size_ == 0)
        {
            throw std::runtime_error("back on empty ilist");
        }
        return *(tail_->prev_);
    }

    // const methods
    const_iterator cbegin() const { return const_iterator{head_->next_}; }
    const_iterator cend() const { return const_iterator{tail_}; }

    const_iterator begin() const { return cbegin(); }
    const_iterator end() const { return cend(); }

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator{tail_->prev_};
    }

    const_reverse_iterator rend() const
    {
        return const_reverse_iterator{head_};
    }

    const T &front() const
    {
        if (size_ == 0)
        {
            throw std::runtime_error("front on empty ilist");
        }
        return *(head_->next_);
    }

    const T &back() const
    {
        if (size_ == 0)
        {
            throw std::runtime_error("back on empty ilist");
        }
        return *(tail_->prev_);
    }
    iterator getIterator() { return iterator(*this); }
    const_iterator getIterator() const { return const_iterator(*this); }
    iterator insert(const iterator &it, T *p)
    {
        auto p_it = it.ptr_;
        if (p_it == head_)
        {
            throw std::runtime_error("insert before head");
        }
        else if (!is_node(p_it))
        {
            throw std::runtime_error("insert a node not in the list");
            // return nullptr;
        }
        p->prev_ = p_it->prev_;
        p->next_ = p_it;
        p_it->prev_->next_ = p;
        p_it->prev_ = p;
        size_++;
        mark_node(p);
        return iterator{p};
    }
};

template <typename T>
ilist<T>::node::~node() {}
