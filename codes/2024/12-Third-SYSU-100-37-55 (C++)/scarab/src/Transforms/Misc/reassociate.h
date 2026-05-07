#ifndef REASSOCIATE_HPP
#define REASSOCIATE_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <set>
#include <climits>
#include <map>
#include <algorithm>

#include "Module.h"
using namespace std;

struct valEntry {
  unsigned rank;
  ValuePtr Op;

  valEntry(unsigned R, ValuePtr O) : rank(R), Op(O) {}
};

struct factor{
    ValuePtr Base;
    unsigned Power;
    factor(ValuePtr B,unsigned P):Base{B}, Power{P}{}
};
void reassociate(FunctionPtr func);

#endif