/**
 * LRU cache
 */
#ifndef _LRU_CACHE_
#define _LRU_CACHE_

#include "Optimization.hh"

class LruCache : public Optimization {
 private:
  unordered_map<Function*, Function*> replaceCacheFuncs;

 public:
  LruCache() {}
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif