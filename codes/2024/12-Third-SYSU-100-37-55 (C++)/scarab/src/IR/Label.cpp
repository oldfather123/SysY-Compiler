#include"Label.h"

Label::Label(string name): name(name)
{
}

void Label::print(){
    cout << name << ":";
}

string Label::toString()
{
    return "label %" + name;
}