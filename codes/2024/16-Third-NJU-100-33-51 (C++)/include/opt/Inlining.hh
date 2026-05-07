/**
 * Function inlining
 */
#ifndef _INLINING_H_
#define _INLINING_H_

// Max function size to preform inlining
#define INLINE_THRESHOLD 1000

#include <set>

#include "Optimization.hh"
using std::set;

class Inlining : public Optimization {
 private:
  bool needInline(Function* func);

 public:
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif