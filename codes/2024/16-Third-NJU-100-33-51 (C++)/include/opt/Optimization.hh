/**
 * An abstract class of ir optimization
 */

#ifndef _OPTIMIZATION_H_
#define _OPTIMIZATION_H_
#include "Module.hh"


class Optimization
{
public:
  virtual bool runOnModule(ANTPIE::Module* module) = 0;
  virtual bool runOnFunction(Function* function) = 0;
};

#endif

