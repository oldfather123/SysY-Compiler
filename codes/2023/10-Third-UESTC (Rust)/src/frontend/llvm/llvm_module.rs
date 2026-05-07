use super::{
    function::{ArgumentList, Function},
    mem_scope::MemScope,
};
use crate::common::r#type::Type;
use getset::{Getters, MutGetters};
use std::{
    collections::HashMap,
    fmt::{Display, Formatter},
};

#[derive(Debug, Default, Getters, MutGetters)]
pub struct LLVMModule {
    #[getset(get = "pub", get_mut = "pub")]
    pub global_scope: MemScope,
    #[getset(get = "pub", get_mut = "pub")]
    pub functions: HashMap<String, Function>,
}

impl LLVMModule {
    pub fn new() -> Self {
        Self {
            global_scope: MemScope::new("global".to_string(), true),
            functions: HashMap::new(),
        }
    }

    pub fn function(&self, name: &String) -> &Function {
        self.functions
            .get(name)
            .expect(format!("function {} not found", name.clone()).as_str())
    }

    pub fn have_function(&self, name: &String) -> bool {
        self.functions.contains_key(name)
    }

    pub fn add_function(&mut self, func: Function) {
        self.functions.insert(func.name().to_string(), func);
    }

    pub fn register_lib_func(&mut self, name: &str, ret_type: Type, arg_list: ArgumentList) {
        let entry = Function::new_lib_func(name.to_string(), ret_type, arg_list);
        self.functions.insert(name.to_string(), entry);
    }

    pub fn for_each_func<F>(&self, f: F)
    where
        F: Fn(&Function),
    {
        for (_, func) in &self.functions {
            f(func);
        }
    }
    /// for each user function
    pub fn for_each_user_func<F>(&self, f: F)
    where
        F: Fn(&Function),
    {
        for (name, func) in &self.functions {
            if !func.is_lib_func() && name != "_init" {
                f(func);
            }
        }
    }
    /// for each user function mut
    pub fn for_each_user_func_mut<F>(&mut self, f: F)
    where
        F: Fn(&mut Function),
    {
        for (name, func) in &mut self.functions {
            if !func.is_lib_func() && name != "_init" {
                f(func);
            }
        }
    }

    /* passes */
    pub fn before_backend(&mut self) {
        self.for_each_user_func_mut(|func| {
            func.before_backend();
        });
    }
}

impl Display for LLVMModule {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        for (key, func) in &self.functions {
            if key == &"_init".to_string() {
                let inst_iter = func.layout().inst_iter(0);
                for inst_id in inst_iter {
                    let inst = func.instructions().get(&inst_id).unwrap();
                    write!(f, "{:?}", inst)?;
                }
            } else {
                if !func.is_lib_func() {
                    write!(f, "{}", func)?;
                }
            }
        }
        Ok(())
    }
}
