use derive_new::new;
use getset::{Getters, MutGetters, Setters};
use std::fmt::Display;

#[derive(Debug, Clone, PartialEq, Default, new, Getters, Setters, MutGetters)]
pub struct BasicBlock {
    #[getset(get = "pub", set = "pub")]
    label: i32,
    #[new(default)]
    #[getset(get = "pub", set = "pub")]
    alias: String,
    #[new(value = "false")]
    #[getset(get = "pub", set = "pub")]
    have_exit: bool,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    prev_bb: Vec<i32>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    succ_bb: Vec<i32>,
}

impl BasicBlock {
    pub fn new_with_alias(label: i32, alias: String) -> BasicBlock {
        BasicBlock {
            alias,
            label,
            have_exit: false,
            prev_bb: Vec::new(),
            succ_bb: Vec::new(),
        }
    }

    pub fn add_prev_bb(&mut self, bb: i32) {
        //! check if there is already exists, avoid redundance
        let count: usize = self.prev_bb.iter().filter(|&&x| x == bb).count();
        if count == 0 {
            self.prev_bb.push(bb);
        } else {
            panic!("BasicBlock::add_prev_bb: redundance");
        }
    }

    pub fn add_succ_bb(&mut self, bb: i32) {
        //! check if there is already exists, avoid redundance
        let count: usize = self.succ_bb.iter().filter(|&&x| x == bb).count();
        if count == 0 {
            self.succ_bb.push(bb);
        } else {
            panic!("BasicBlock::add_succ_bb: redundance");
        }
    }

    pub fn remove_prev_bb(&mut self, bb: i32) {
        //! check if there is already exists
        let count: usize = self.prev_bb.iter().filter(|&&x| x == bb).count();
        if count != 0 {
            self.prev_bb.retain(|&x| x != bb);
        } else {
            panic!("BasicBlock::remove_prev_bb: bb: {} not exists", bb);
        }
    }

    pub fn remove_succ_bb(&mut self, bb: i32) {
        //! check if there is already exists
        let count: usize = self.succ_bb.iter().filter(|&&x| x == bb).count();
        if count != 0 {
            self.succ_bb.retain(|&x| x != bb);
        } else {
            panic!("BasicBlock::remove_succ_bb: bb: {} not exists", bb);
        }
    }

    pub fn replace_succ_bb(&mut self, old_bb: i32, new_bb: i32) {
        self.remove_succ_bb(old_bb);
        self.add_succ_bb(new_bb);
    }

    pub fn replace_prev_bb(&mut self, old_bb: i32, new_bb: i32) {
        self.remove_prev_bb(old_bb);
        self.add_prev_bb(new_bb);
    }
}

impl Display for BasicBlock {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "bb{}:", self.label)?;
        Ok(())
    }
}
