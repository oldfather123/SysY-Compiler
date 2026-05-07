#ifndef AAAC_SCOPED_TABLE
#define AAAC_SCOPED_TABLE

#include <cassert>
#include <map>
#include <unordered_map>
#pragma once

namespace aaac {
namespace ADT {

template<typename KeyType, typename ValueType>
class ScopedTable {
public:
    ScopedTable() = default;
    ScopedTable(const ScopedTable &) = delete;
    ScopedTable &operator=(const ScopedTable &) = delete;
    ~ScopedTable() {
        assert(CurrentScope == nullptr && "illegal destruct\n");
    }

    void insert(KeyType key, ValueType *value) {
        assert(CurrentScope != nullptr && "should have a scope");
        Entry* &prevEntry = map[key];
        prevEntry = new Entry{key,value,CurrentScope->lastEntry,prevEntry};
        CurrentScope->lastEntry = prevEntry;
    }

    ValueType *find(KeyType key) {
        if(map.find(key) != map.end()) {
            return map[key]->value;
        }
        return nullptr;
    }

    void set(KeyType key, ValueType *value) {
        if(map.find(key) == map.end()) {
            return;
        }
        map[key]->value = value;
    }

    void BeginScope() {
        CurrentScope = new Scope{nullptr, CurrentScope};
    }

    void EndScope() {
        assert(CurrentScope != nullptr && "should have a scope");
        while(Entry *entry = CurrentScope->lastEntry) {
            map[entry->key] = entry->nextForKey;
            CurrentScope->lastEntry = entry->nextInScope;
        }
        Scope *lastScope = CurrentScope->lastScope;
        delete CurrentScope;
        CurrentScope = lastScope;
    }

private:
    struct Entry {
        KeyType key;
        ValueType *value;
        Entry *nextInScope;
        Entry *nextForKey;
    };

    struct Scope {
        Entry *lastEntry = nullptr;
        Scope *lastScope = nullptr;
    };

    std::unordered_map<KeyType, Entry*> map;
    Scope *CurrentScope = nullptr;

};

} // namespace ADT
} // namespace aaac

#endif