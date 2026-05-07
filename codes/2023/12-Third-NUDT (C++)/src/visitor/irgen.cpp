#include "irgen.hpp"

IR::Generator::Generator(CompileUnit *cu) : _cu(cu) {
}

void IR::Generator::generate(LogLevel log_level) {
  configuration.saveLogLevel();
  configuration.setLogLevel(log_level);
  generate(_cu);
  configuration.loadLogLevel();
}

void IR::Generator::generate(CompileUnit *cu) {
  for (const auto &bb : cu->initFunction()->basicBlocks())
    for (const auto &inst : bb->instructions()) generate(inst.get());
  info << "\n";
  for (const auto &[name, f] : *cu->functionTable())
    if (!builtin_function_names.count(name)) generate(f.get());
}

void IR::Generator::generate(Function *f) {
  info << "define " << f->retType()->name() << " @" << f->name() << "(";
  for (auto it = f->params().begin(); it != f->params().end(); it++) {
    info << (it == f->params().begin() ? "" : ", ");
    info << (*it)->type()->name() << " " << (*it)->name();
  }
  info << ") {\n";
  for (const auto &bb : f->basicBlocks()) generate(bb.get());
  info << "}\n\n";
}

void IR::Generator::generate(BasicBlock *bb) {
  info << bb->name().substr(1) << ":";
  info << (bb->preds().size() || bb->succs().size() ? " ;" : "");
  if (bb->preds().size()) info << " preds = ";
  for (auto it = bb->preds().begin(); it != bb->preds().end(); it++) {
    info << (it == bb->preds().begin() ? "" : ", ");
    info << (*it)->name();
  }
  if (bb->succs().size()) info << " succs = ";
  for (auto it = bb->succs().begin(); it != bb->succs().end(); it++) {
    info << (it == bb->succs().begin() ? "" : ", ");
    info << (*it)->name();
  }
  info << "\n";
  for (const auto &inst : bb->instructions()) generate(inst.get());
  info << "\n";
}

static void dfsGlobalGenerate(Instruction *inst, Type *type, size_t idx) {
  if (type->in({Type::i32_t(), Type::float_t()})) {
    info << type->name() << " " << inst->operand(idx)->name();
    return;
  }
  info << type->name() << " [";
  for (size_t i = 0; i < type->dim(); i++) {
    info << (i ? ", " : "");
    dfsGlobalGenerate(inst, type->deref(1), idx + i * type->deref(1)->size());
  }
  info << "]";
}

void IR::Generator::generate(Instruction *inst) {
  info << (inst->itype() == GLOBAL ? "" : IDENT);
  if (!inst->type()->isVoid()) {
    debug << "{" << inst->type()->name() << "} ";
    info << inst->name() << " = ";
  }
  info << inst->itypeName() << " ";
  switch (inst->itype()) {
    case GLOBAL: {
      dfsGlobalGenerate(inst, inst->type()->deref(1), 0);
      info << "\n";
    } break;
    case RET: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << "\n";
    } break;
    case JMP: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << "\n";
    } break;
    case BR: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << ", "
           << inst->operand(1)->type()->name() << " " << inst->operand(1)->name() << ", "
           << inst->operand(2)->type()->name() << " " << inst->operand(2)->name() << "\n";
    } break;
    case FNEG: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << "\n";
    } break;
    case ADD: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case FADD: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case SUB: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case FSUB: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case MUL: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case FMUL: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case SDIV: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case FDIV: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case SREM: {
      info << inst->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name() << "\n";
    } break;
    case SHL: {
      throw Unreachable();
    } break;
    case LSHR: {
      throw Unreachable();
    } break;
    case ASHR: {
      throw Unreachable();
    } break;
    case AND: {
      throw Unreachable();
    } break;
    case OR: {
      throw Unreachable();
    } break;
    case XOR: {
      throw Unreachable();
    } break;
    case ALLOCA: {
      info << inst->type()->deref(1)->name() << "\n";
    } break;
    case LOAD: {
      info << inst->type()->name() << ", " << inst->operand(0)->type()->name() << " " << inst->operand(0)->name()
           << "\n";
    } break;
    case STORE: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << ", "
           << inst->operand(1)->type()->name() << " " << inst->operand(1)->name() << "\n";
    } break;
    case GETELEMENTPTR: {
      info << inst->operand(0)->type()->deref(1)->name();
      for (size_t i = 0; i < inst->operands().size(); i++)
        info << ", " << inst->operand(i)->type()->name() << " " << inst->operand(i)->name();
      info << "\n";
    } break;
    case TRUNC: {
      throw Unreachable();
    } break;
    case ZEXT: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << " to " << inst->type()->name()
           << "\n";
    } break;
    case SEXT: {
      throw Unreachable();
    } break;
    case FPTRUNC: {
      throw Unreachable();
    } break;
    case FPEXT: {
      throw Unreachable();
    } break;
    case FPTOSI: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << " to " << inst->type()->name()
           << "\n";
    } break;
    case SITOFP: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << " to " << inst->type()->name()
           << "\n";
    } break;
    case PTRTOINT: {
      throw Unreachable();
    } break;
    case INTTOPTR: {
      throw Unreachable();
    } break;
    case IEQ:
    case INE:
    case ISGT:
    case ISGE:
    case ISLT:
    case ISLE:
    case FOEQ:
    case FONE:
    case FOGT:
    case FOGE:
    case FOLT:
    case FOLE: {
      info << inst->operand(0)->type()->name() << " " << inst->operand(0)->name() << ", " << inst->operand(1)->name()
           << "\n";
    } break;
    case PHI: {
      for (size_t i = 0; i < dynamic_cast<PhiInst *>(inst)->vals().size(); i++)
        info << (i ? ", " : "") << "[ " << inst->operand(i) << ", "
             << inst->operand(i + dynamic_cast<PhiInst *>(inst)->vals().size()) << " ]";
      info << "\n";
    } break;
    case CALL: {
      info << inst->operand(0)->type()->retType()->name() << " @" << inst->operand(0)->name() << "(";
      for (size_t i = 1; i < inst->operands().size(); i++)
        info << (i == 1 ? "" : ", ") << inst->operand(i)->type()->name() << " " << inst->operand(i)->name();
      info << ")\n";
    } break;
    default: {
      throw Unreachable();
    } break;
  }
}
