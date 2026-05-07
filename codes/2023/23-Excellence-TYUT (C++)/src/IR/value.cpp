//
// Created by 牛奕博 on 2023/4/18.
//

#include "value.h"

using namespace anuc;

void Value::insertBackToUses(Use *u) {
    u->parent = this;
    uses.insert_back(u);
}

void Value::printAllUsers() {
    for (auto i = uses.begin(); i != uses.end(); ++i) {
        (*i).user->print();
    }
}

//不删除被替换调的use链，rapu完记得手动erasefromparent
void Value::replaceAllUseWith(Value *v) {
    for (auto i = uses.begin(); i != uses.end();) {
        Use *u = &*i;
        ++i;
        u->value = v;
        v->insertBackToUses(u);
    }
}
void Value::eraseFromList(Use *node) {
    uses.erase_from_list(static_cast<base_node*>(node));
}
//注意，删除或节点不会自动自增
//在链表中使用注意
void Use::replaceValueWith(anuc::Value *v) {
    value = v;
    eraseFromParent();
    v->insertBackToUses(this);
}

void Use::eraseFromParent() {
    parent->eraseFromList(this);
}