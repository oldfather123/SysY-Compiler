#ifndef RISCV64_PASSES_H
#define RISCV64_PASSES_H

#include "Pass.h"
#include "RISCv64LLIR.h"
#include "Peephole.h"
#include "PreRA_Scheduler.h"
#include "PostRA_Scheduler.h"
#include "CalleeSavedHandler.h"
#include "LegalizeImmediates.h"
#include "PrologueEpilogueInsertion.h"
#include "EliminateFrameIndices.h"
#include "DivStrengthReduction.h"

namespace sysy {

} // namespace sysy

#endif // RISCV64_PASSES_H