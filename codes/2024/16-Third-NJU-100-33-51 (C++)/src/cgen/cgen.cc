#include "cgen.hh"

#include <fstream>
#include <iostream>

void generate_code(MModule *res, ANTPIE::Module *ir, bool opt) {
  select_instruction(res, ir);
  allocate_register(res);
  peephole_optimize(res);
  schedule_instruction(res);
  branch_simplify(res);
}