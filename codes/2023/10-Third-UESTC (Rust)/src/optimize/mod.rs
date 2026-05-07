pub mod passes;

use crate::frontend::llvm::llvm_module::LLVMModule;
use passes::bb_ops::remove_phi;
use passes::check_ir::check_module;
use passes::dce::{remove_unused_def, remove_useless_bb};
use passes::gcm::gcm_for_module;
use passes::gvn::global_value_numbering;
use passes::mem2reg::mem2reg;

fn gvn(llvm_module: &mut LLVMModule, enable_passes: &Vec<String>) {
    if enable_passes.contains(&"gvn".to_string()) {
        log::trace!("GVN...");
        global_value_numbering(llvm_module);
        remove_unused_def(llvm_module);
        check_module(llvm_module);
        log::trace!("GVN Done!")
    }
}

fn gcm(llvm_module: &mut LLVMModule, enable_passes: &Vec<String>) {
    if enable_passes.contains(&"gcm".to_string()) {
        log::trace!("GCM...");
        gcm_for_module(llvm_module);
        check_module(llvm_module);
        log::trace!("GCM Done!")
    }
}

pub fn optimize_ir(llvm_module: &mut LLVMModule, enable_passes: &Vec<String>) {
    if enable_passes.contains(&"opt".to_string()) {
        log::trace!("Optimizing...");
        remove_useless_bb(llvm_module);

        if enable_passes.contains(&"mem2reg".to_string()) {
            log::trace!("Mem2Reg...");
            mem2reg(llvm_module);
            remove_unused_def(llvm_module);
            check_module(&llvm_module);
            log::trace!("Mem2Reg Done!")
        }

        gvn(llvm_module, enable_passes);
        gcm(llvm_module, enable_passes);
        gvn(llvm_module, enable_passes);

        if enable_passes.contains(&"misc".to_string()) {
            log::trace!("Misc...");
            log::trace!("Stength Reduction...");
            passes::misc::strength_reduction(llvm_module);
            log::trace!("Stength Reduction Done!");
            log::trace!("Misc Done!")
        }
        remove_phi(llvm_module);
        log::trace!("Optimizing Done!")
    }
    llvm_module.before_backend();
}
