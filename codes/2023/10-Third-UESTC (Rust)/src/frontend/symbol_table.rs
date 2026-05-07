use crate::frontend::llvm::ssa::*;
use derive_new::new;
use std::collections::HashMap;

pub type VariableEntry = SSALeftValue;

#[derive(Debug, Clone, PartialEq, Default, new)]
pub struct VariableTable {
    pub ptable: Option<Box<VariableTable>>,
    #[new(default)]
    pub vtable: HashMap<String, VariableEntry>,
    #[new(default)]
    pub id_table: HashMap<i32, VariableEntry>,
}

impl VariableTable {
    pub fn new_ctable(self) -> Box<VariableTable> {
        let ret = VariableTable {
            ptable: Some(Box::new(self)),
            vtable: HashMap::new(),
            id_table: HashMap::new(),
        };
        Box::new(ret)
    }

    pub fn append(&mut self, name: String, entry: VariableEntry) {
        self.vtable.insert(name, entry.clone());
        self.id_table.insert(*entry.id(), entry);
    }

    pub fn get_variable(&self, name: &str) -> Option<&VariableEntry> {
        match self.vtable.get(name) {
            Some(v) => Some(v),
            None => match &self.ptable {
                Some(ptable) => ptable.get_variable(name),
                None => None,
            },
        }
    }

    pub fn get_variable_mut(&mut self, name: &str) -> Option<&mut VariableEntry> {
        match self.vtable.get_mut(name) {
            Some(v) => Some(v),
            None => match &mut self.ptable {
                Some(ptable) => ptable.get_variable_mut(name),
                None => None,
            },
        }
    }

    pub fn is_exist(&self, name: &String) -> bool {
        match self.vtable.get(name) {
            Some(_) => true,
            None => false,
        }
    }

    // pub fn traverse(&self, depth: i32) {
    //     for (key, entry) in &self.vtable {
    //         for _i in 0..depth {
    //             print!("\t");
    //         }
    //         if entry.is_const() {
    //             match entry.get_value().last() {
    //                 Some(last_entry) => match last_entry.get_value() {
    //                     Some(const_val) => {
    //                         println!("{}:{}", key, const_val);
    //                     }
    //                     None => {
    //                         println!("{}:None", key);
    //                     }
    //                 },
    //                 None => {
    //                     println!("{}:None", key);
    //                 }
    //             }
    //         } else {
    //             println!("{}:{}", key, entry);
    //         }
    //     }
    //     match &self.ptable {
    //         Some(ptable) => ptable.traverse(depth + 1),
    //         None => {}
    //     }
    // }
}
