//
// Created by 牛奕博 on 2023/4/19.
//

#ifndef ANUC_RTTI_H
#define ANUC_RTTI_H

#include <assert.h>
#include <iostream>
using namespace std;
template<class To, class From>
inline bool isa(From *from) {
    return To::classof(from);
}

template<class To, class From>
inline To *cast(From *from) {
    if(!isa<To>(from)) {
        cout << "错误转换！" << from->toString() << endl;
    }
    assert(isa<To>(from) && "Invalid cast!");
    return (To *) from;
}

template<class To, class From>
inline To *dyn_cast(From *from) {
    return isa<To>(from) ? cast<To>(from) : nullptr;
}

#endif //ANUC_RTTI_H
