#ifndef _ALIASANALYSISRESULT_H_
#define _ALIASANALYSISRESULT_H_
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Value.hh"
using std::unordered_map;
using std::unordered_set;
using std::vector;
typedef uint32_t attr_t;

class AliasAnalysisResult {
 private:
  unordered_set<uint64_t> distinctPairs;
  vector<unordered_set<attr_t>> distinctSets;
  unordered_map<Value*, vector<attr_t>> valueAttrsMap;
  uint64_t hash(attr_t attr1, attr_t attr2);
 public:
  void addDistinctPair(attr_t attr1, attr_t attr2);
  void addDistinctSet(unordered_set<attr_t> distinctSet);
  bool isDistinct(Value* v1, Value* v2);
  void pushAttrs(Value* value, vector<attr_t>& attrs);
  void pushAttr(Value* value, attr_t attr);
  vector<attr_t>& getAttrs(Value* value);
};

#endif