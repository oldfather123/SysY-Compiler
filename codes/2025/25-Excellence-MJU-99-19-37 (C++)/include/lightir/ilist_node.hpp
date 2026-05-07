#pragma once

// 独立的 ilist_node 类，兼容 LLVM 的设计
template <typename T>
class ilist_node {
private:
    T *prev_{nullptr};
    T *next_{nullptr};
    size_t tag_{0};  // 用于 ilist 的标记
    
    template<typename U> friend class ilist;
    
public:
    ilist_node() = default;
    virtual ~ilist_node() = default;
    
    T* getPrev() const { return prev_; }
    T* getNext() const { return next_; }
    
    // 防止拷贝
    ilist_node(const ilist_node&) = delete;
    ilist_node& operator=(const ilist_node&) = delete;
};