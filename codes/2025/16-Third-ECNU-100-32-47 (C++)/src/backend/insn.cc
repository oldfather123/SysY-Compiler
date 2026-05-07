#include "../include/backend.h"
#include "../include/sym.h"
#include <ostream>

namespace aaac {

namespace backend {
void PhiInsn::appendOperand(std::shared_ptr<sym::Var> var, std::shared_ptr<BasicBlock> from) {
  phi_operands.push_back(std::make_shared<PhiOperand>(
    var->prop<sym::ScopeInfo>().getStackTopSubscriptVar(),
    from
));
}

// dump of Insn
// std::ostream &StartInsn::dump(std::ostream &os) const {
//   return os << "<start>";
// }



} // namespace backend
} // namespace aaac
