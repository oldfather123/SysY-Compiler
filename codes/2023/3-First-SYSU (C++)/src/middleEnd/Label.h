#pragma once
#include <string>
#include <iostream>
#include "utils.h"

using std::string;
using std::cout;
using std::endl;
using std::shared_ptr;

struct Label
{
    string name;
    Label(string name="entry");
    void print();
    string getStr();
};
typedef shared_ptr<Label> LabelPtr;