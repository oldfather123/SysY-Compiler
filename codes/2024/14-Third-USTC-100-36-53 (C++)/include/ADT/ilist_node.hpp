# pragma once
namespace ADT {
template<typename T> class ilist_node {
    T *prev = nullptr;
    T *next = nullptr;

  public:
    ilist_node() : prev(nullptr), next(nullptr) {}

    virtual ~ilist_node() = default;

    void setPrev(T *p) { prev = p; }
    void setNext(T *n) { next = n; }
    T *getPrev() const { return prev; }
    T *getNext() const { return next; }
};
}; // namespace ADT