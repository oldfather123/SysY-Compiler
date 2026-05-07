use super::ssa::SSALeftValue;
use derive_new::new;
use getset::{Getters, MutGetters, Setters};
use std::collections::HashMap;

#[derive(Debug, new, Default, Getters, Setters, MutGetters)]
pub struct MemScope {
    #[getset(get = "pub", set = "pub")]
    name: String,
    #[new(default)]
    #[getset(get = "pub", set = "pub")]
    size: i32,
    #[getset(get = "pub", set = "pub")]
    is_global: bool,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    objects: HashMap<i32, SSALeftValue>, // ssa left value id -> mem object
    #[new(default)]
    #[getset(get = "pub", set = "pub")]
    arg_idx2lvalue: HashMap<usize, i32>, // arg index -> ssa left value id
    #[new(default)]
    #[getset(get = "pub", set = "pub")]
    lvalue_2arg_idx: HashMap<i32, usize>, // ssa left value id -> arg index
}

impl MemScope {
    pub fn new_mem_object(&mut self, lvalue: SSALeftValue) {
        let ssa_id = *lvalue.id();
        assert!(!self.objects.contains_key(&ssa_id));
        self.objects.insert(ssa_id, lvalue);
    }
    pub fn new_argument(&mut self, arg_idx: usize, lvalue: SSALeftValue) {
        let ssa_id = *lvalue.id();
        assert!(!self.objects.contains_key(&ssa_id));
        self.objects.insert(ssa_id, lvalue);
        self.arg_idx2lvalue.insert(arg_idx, ssa_id);
        self.lvalue_2arg_idx.insert(ssa_id, arg_idx);
    }
}
