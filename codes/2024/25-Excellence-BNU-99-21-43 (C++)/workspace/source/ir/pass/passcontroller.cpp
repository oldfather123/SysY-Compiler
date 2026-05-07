#include "passcontroller.h"

namespace IR
{
    void PassController::run(bool opt, IR::Module *module)
    {
        if (opt)
        {
            IR::Mem2RegPass mem2reg;
            IR::InlineFunctionPass inlineFunction;
            IR::SimplifyCodePass simplify;
            IR::ConstantFoldPass cf;
            IR::CommonSubExpressionEliminationPass cse;
            IR::StrengthReductionPass sr;
            IR::FinalPass final;

            inlineFunction.run(module);
            mem2reg.run(module);
            simplify.run(module);
            IR::LoopUnrollPass::run(module);

            sr.run(module);
            simplify.run(module);

            cf.run(module);
            simplify.run(module);

            cse.run(module);
            simplify.run(module);

            cf.run(module);
            simplify.run(module);

            sr.run(module);
            simplify.run(module);

            final.run(module);
        }
        else
        {
            IR::Mem2RegPass mem2reg;
            IR::InlineFunctionPass inlineFunction;
            IR::SimplifyCodePass simplify;
            IR::ConstantFoldPass cf;
            IR::CommonSubExpressionEliminationPass cse;
            IR::StrengthReductionPass sr;
            IR::FinalPass final;

            

            // inlineFunction.run(module);
            // mem2reg.run(module);
            // simplify.run(module);

            // IR::LoopUnrollPass::run(module);

            // sr.run(module);
            // simplify.run(module);

            // cf.run(module);
            // simplify.run(module);

            cse.run(module);
            simplify.run(module);

            // cf.run(module);
            // simplify.run(module);

            final.run(module);
        }
    }
}