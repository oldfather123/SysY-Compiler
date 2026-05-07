/* self-defined data structure */
#pragma once

#include <functional>
#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#include <functional>

template <class Node>
struct flist {
    Node *head;
    Node *tail;

    flist() {
        head = new Node(); // Sentinel node for head
        tail = new Node(); // Sentinel node for tail
        head->next = tail;
        tail->prev = head;
    }

    ~flist() {
        clear();
        delete head;
        delete tail;
    }

    // insert newNode between prevNode and nextNode
    void insertBetween(Node *newNode, Node *prevNode, Node *nextNode) {
        newNode->prev = prevNode;
        newNode->next = nextNode;
        prevNode->next = newNode;
        nextNode->prev = newNode;
    }

    // insert newNode before insertBefore
    void insertBefore(Node *newNode, Node *insertBefore) {
        insertBetween(newNode, insertBefore->prev, insertBefore);
        if (insertBefore == head->next) {
            head->next = newNode; // update head snetinel node if necessary
        }
    }

    // insert newNode after insertAfter
    void insertAfter(Node *newNode, Node *insertAfter) {
        insertBetween(newNode, insertAfter, insertAfter->next);
        if (insertAfter == tail->prev) {
            tail->prev = newNode; // update tail sentinel node if necessary
        }
    }

    // insert newNode at the end of flist
    void insertAtEnd(Node *newNode) {
        insertAfter(newNode, tail->prev);
    }

    // insert newNode at the begin of flist
    void insertAtBegin(Node *newNode) {
        insertBefore(newNode, head->next);
    }

    // remove node from flist
    void remove(Node *node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    // clear the flist
    void clear() {
        Node *current = head->next;
        while (current != tail) {
            Node *next = current->next;
            delete current;
            current = next;
        }
        head->next = tail;
        tail->prev = head;
    }
};

struct FixedPoint {
    String name, insideName;
    unsigned iterationCount;
    bool aborted, changed;
    FixedPoint(std::function<void(bool &)> f, bool debugPrint = false, const String &name = "", const String &insideName = "", unsigned maxIter = INT_MAX) : name(name), insideName(insideName) {
        iterationCount = 0;
        bool _changed;
        aborted = false;
        do {
            _changed = false;
            f(_changed);
            ++iterationCount;
            if (iterationCount >= maxIter) {
                aborted = true;
                break;
            }
            changed |= _changed;
        } while (_changed);
        if (debugPrint) {
            printDebug();
        }
    }
    operator bool() const {
        return changed;
    }
    void printDebug() const {
        dbgout << "├── Fixed point " << (aborted ? "aborted after reaching iteration count limit" : "converged") << ": " << name << (insideName.empty() ? "" : " (" + insideName + ")") << ", iteration count: " << iterationCount << "." << std::endl;
    }
};

/// @brief Reference to a shared pointer slot
template <typename T>
struct PtrRef {
    friend class std::hash<PtrRef<T>>;

private:
    Ptr<T> *_ref; // Address of the slot

public:
    /// @brief Change the value stored in the slot
    /// @param value New value to be stored in the slot
    void replace(Ptr<T> &ptr) {
        *this->_ref = ptr;
    }

    PtrRef() : _ref(nullptr) { }
    PtrRef(Ptr<T> *ref) : _ref(ref) { }
    PtrRef(const PtrRef<T> &other) = default;
    PtrRef(PtrRef<T> &&other) = default;
    ~PtrRef() = default;

    PtrRef<T> &operator=(const PtrRef<T> &other) = default;
    T *operator->() {
        return (*_ref).get();
    }
    const T *operator->() const {
        return (*_ref).get();
    }

    bool operator==(const PtrRef<T> &other) const {
        return *this->_ref == *other._ref;
    }
    bool operator!=(const PtrRef<T> &other) const {
        return !(*this == other);
    }
    operator bool() const {
        return *_ref != nullptr;
    }
    operator Ptr<T>() const {
        return *_ref;
    }
};

namespace std {
    template <typename T>
    struct hash<PtrRef<T>> {
        std::size_t operator()(const PtrRef<T> &ptrRef) const {
            return std::hash<Ptr<T>>()(*ptrRef._ref);
        }
    };
}
