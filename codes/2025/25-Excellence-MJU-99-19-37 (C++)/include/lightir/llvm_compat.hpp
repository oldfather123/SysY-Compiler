#pragma once

#include "ilist_node.hpp"
#include "ilist.hpp"

// LLVM 兼容层
namespace llvm {
    template<typename T>
    using ilist_node = ::ilist_node<T>;
    
    template<typename T>
    using ilist = ::ilist<T>;
}