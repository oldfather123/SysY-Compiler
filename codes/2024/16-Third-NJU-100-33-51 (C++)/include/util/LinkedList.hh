#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

#include <assert.h>

#include <memory>

template <typename T>
struct Node {
  T data;
  Node* next;
  Node* prev;

  Node(const T& data) : data(data), next(nullptr), prev(nullptr) {}
};

template <typename T>
class LinkedList {
 private:
  Node<T>* head;
  Node<T>* tail;
  int size;

 public:
  LinkedList() : head(nullptr), tail(nullptr), size(0) {}
  ~LinkedList() {
    Node<T>* current = head;
    Node<T>* next;
    while (current != nullptr) {
      next = current->next;
      delete current;
      current = next;
    }
  }

  Node<T>* getHead() const { return head; }
  Node<T>* getTail() const { return tail; }

  T front() { return head ? head->data : nullptr; }
  T back() { return tail ? tail->data : nullptr; }

  void pushFront(const T& data) {
    Node<T>* newNode = new Node<T>(data);
    newNode->next = head;
    if (head != nullptr) {
      head->prev = newNode;
    }
    head = newNode;
    if (tail == nullptr) {
      tail = newNode;
    }
    size++;
  }

  void pushBack(const T& data) {
    Node<T>* newNode = new Node<T>(data);
    newNode->prev = tail;
    if (tail != nullptr) {
      tail->next = newNode;
    }
    tail = newNode;
    if (head == nullptr) {
      head = newNode;
    }
    size++;
  }

  void remove(const T& data) {
    Node<T>* current = head;

    while (current != nullptr && current->data != data) {
      current = current->next;
    }

    if (current == nullptr) {
      return;
    }

    if (current->prev != nullptr) {
      current->prev->next = current->next;
    } else {
      head = current->next;
    }

    if (current->next != nullptr) {
      current->next->prev = current->prev;
    } else {
      tail = current->prev;
    }

    delete current;
    size--;
  }

  T popBack() {
    T old = tail->data;
    remove(old);
    return old;
  }

  T popFront() {
    T old = head->data;
    remove(old);
    return old;
  }

  bool isEmpty() const { return head == nullptr; }

  int getSize() const { return size; }

  void clear() {
    Node<T>* current = head;
    Node<T>* next = nullptr;

    while (current != nullptr) {
      next = current->next;
      delete current;
      current = next;
    }

    head = nullptr;
    tail = nullptr;
    size = 0;
  }

  class Iterator {
   private:
    Node<T>* current;

   public:
    Iterator(Node<T>* node) : current(node) {}

    Iterator& operator++() {
      if (current) {
        current = current->next;
      }
      return *this;
    }

    Iterator& operator--() {
      if (current) {
        current = current->prev;
      }
      return *this;
    }

    Iterator operator+(int n) const {
      Iterator temp = *this;
      for (int i = 0; i < n && temp.current; ++i) {
        temp.current = temp.current->next;
      }
      return temp;
    }

    Iterator operator-(int n) const {
      Iterator temp = *this;
      for (int i = 0; i < n && temp.current; ++i) {
        temp.current = temp.current->prev;
      }
      return temp;
    }

    T& operator*() const { return current->data; }

    bool operator!=(const Iterator& other) const {
      return current != other.current;
    }

    bool operator==(const Iterator& other) const {
      return current == other.current;
    }

    Node<T>* getCurrentNode() const { return current; }
  };

  Iterator begin() const { return Iterator(head); }

  Iterator end() const { return Iterator(nullptr); }

  void insertBefore(const Iterator& it, const T& data) {
    if (it == begin()) {
      pushFront(data);
      return;
    }

    Node<T>* newNode = new Node<T>(data);
    Node<T>* current = it.getCurrentNode();
    Node<T>* previous = current->prev;

    if (previous != nullptr) {
      previous->next = newNode;
    }
    newNode->prev = previous;
    newNode->next = current;
    current->prev = newNode;

    if (current == head) {
      head = newNode;
    }

    size++;
  }
  void insertBefore(const T& loc, const T& data) {
    Node<T>* newNode = new Node<T>(data);
    Node<T>* current = head;

    if (head == nullptr) {
      head = newNode;
      tail = newNode;
      size++;
      return;
    }

    if (head->data == loc) {
      newNode->next = head;
      head->prev = newNode;
      head = newNode;
      size++;
      return;
    }
    if (tail->data == loc) {
      current = tail;
    } else {
      while (current != nullptr && current->data != loc) {
        current = current->next;
      }
    }

    if (current == nullptr) {
      delete newNode;
      return;
    }

    Node<T>* previous = current->prev;
    previous->next = newNode;
    newNode->prev = previous;
    newNode->next = current;
    current->prev = newNode;
    size++;
  }

  void insertAfter(const Iterator& it, const T& data) {
    if (it == end()) {
      return;
    }

    Node<T>* newNode = new Node<T>(data);
    Node<T>* current = it.getCurrentNode();
    Node<T>* next = current->next;

    if (next != nullptr) {
      next->prev = newNode;
    }
    newNode->next = next;
    newNode->prev = current;
    current->next = newNode;

    if (current == tail) {
      tail = newNode;
    }

    size++;
  }

  void splitAfter(const Iterator& it, LinkedList<T>* newList) {
    if (newList == nullptr || it == end() || it.getCurrentNode() == tail) {
      return;
    }

    Node<T>* current = it.getCurrentNode();
    Node<T>* newHead = current->next;

    current->next = nullptr;
    if (newHead != nullptr) {
      newHead->prev = nullptr;
    }
    newList->head = newHead;
    newList->tail = tail;

    int newSize = 0;
    while (newHead != nullptr) {
      newSize++;
      newHead = newHead->next;
    }

    newList->size = newSize;
    this->size -= newSize;
    this->tail = current;
  }

  T at(int n) const {
    assert(n >= 0 && n < size);

    Node<T>* current;
    if (n < size / 2) {
      current = head;
      for (int i = 0; i < n; ++i) {
        current = current->next;
      }
    } else {
      current = tail;
      for (int i = size - 1; i > n; --i) {
        current = current->prev;
      }
    }
    return current->data;
  }
};

#endif
