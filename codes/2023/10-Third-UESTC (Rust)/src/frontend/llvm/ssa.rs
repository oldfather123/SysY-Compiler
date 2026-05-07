use crate::common::constant::ADDRESS_SIZE;
use crate::common::{immediate::Immediate, r#type::Type};
use enum_as_inner::EnumAsInner;
use getset::{Getters, Setters};
use std::fmt::Display;
use std::hash::{Hash, Hasher};

#[derive(PartialEq, Debug, Clone, Eq, Hash, EnumAsInner)]
pub enum AddrType {
    Local, // local left value address or argument address
    Global(String),
}

#[derive(PartialEq, Debug, Clone, EnumAsInner)]
pub enum SSARightValueInner {
    Immediate(Immediate),                         // immediate value
    Normal(i32, Type),                            // id, ty, just a basic register
    Address(i32, Type, Vec<i32>, AddrType, bool), // left_value_id, ty, address_shape, addr_type, is_arg
}

impl Default for SSARightValueInner {
    fn default() -> Self {
        Self::Immediate(Immediate::Int(0))
    }
}

impl Eq for SSARightValueInner {}

// TODO: hash immediate
impl Hash for SSARightValueInner {
    fn hash<H: Hasher>(&self, state: &mut H) {
        match self {
            Self::Immediate(imme) => {
                1i32.hash(state);
                imme.hash(state)
            }
            Self::Normal(id, _) => {
                2i32.hash(state);
                id.hash(state);
            }
            Self::Address(id, _, _, _, _) => {
                4i32.hash(state);
                id.hash(state);
            }
        }
    }
}

#[derive(Debug, PartialEq, Clone, Setters, Getters)]
pub struct SSARightValue {
    #[getset(get = "pub")]
    inner: SSARightValueInner,
    // context
    #[getset(set = "pub")]
    origin_id_and_version: Option<(i32, i32)>, // before ssa, only generate in ssa construction
}

impl Eq for SSARightValue {}

// TODO: hash immediate
impl Hash for SSARightValue {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.inner.hash(state);
    }
}

impl SSARightValue {
    /// for const value
    pub fn new_imme(imme: Immediate) -> Self {
        Self {
            inner: SSARightValueInner::Immediate(imme),
            origin_id_and_version: None,
        }
    }

    /// for variable
    pub fn new_reg(id: i32, ty: Type) -> Self {
        Self {
            inner: SSARightValueInner::Normal(id, ty),
            origin_id_and_version: None,
        }
    }

    /// for address
    pub fn new_addr(id: i32, ty: Type, shape: Vec<i32>) -> Self {
        Self {
            inner: SSARightValueInner::Address(id, ty, shape, AddrType::Local, false),
            origin_id_and_version: None,
        }
    }

    pub fn get_type(&self) -> Type {
        let ty = match self.inner {
            SSARightValueInner::Immediate(imme) => imme.get_type(),
            SSARightValueInner::Normal(_, ty) => ty,
            SSARightValueInner::Address(_, ty, _, _, _) => ty,
        };
        assert!(ty != Type::Void);
        ty
    }

    pub fn get_value(&self) -> Option<Immediate> {
        match self.inner {
            SSARightValueInner::Immediate(imme) => Some(imme),
            _ => None,
        }
    }

    pub fn is_immediate(&self) -> bool {
        match self.inner {
            SSARightValueInner::Immediate(_) => true,
            _ => false,
        }
    }

    pub fn is_global(&self) -> bool {
        match self.inner {
            SSARightValueInner::Address(_, _, _, AddrType::Global(_), _) => true,
            _ => false,
        }
    }

    pub fn global_name(&self) -> Option<&str> {
        match &self.inner {
            SSARightValueInner::Address(_, _, _, AddrType::Global(name), _) => Some(name.as_str()),
            _ => None,
        }
    }

    pub fn is_addr(&self) -> bool {
        match self.inner {
            SSARightValueInner::Address(_, _, _, _, _) => true,
            _ => false,
        }
    }

    pub fn is_array_addr(&self) -> bool {
        match &self.inner {
            SSARightValueInner::Address(_, _, shape, _, _) => shape.len() > 0,
            _ => false,
        }
    }

    pub fn is_arg(&self) -> bool {
        match self.inner {
            SSARightValueInner::Address(_, _, _, _, is_arg) => is_arg,
            _ => false,
        }
    }

    pub fn addr_shape(&self) -> Option<Vec<i32>> {
        match &self.inner {
            SSARightValueInner::Address(_, _, shape, _, _) => Some(shape.clone()),
            _ => None,
        }
    }

    // TODO: argument is lvalue or not?
    pub fn have_lvalue(&self) -> bool {
        match self.inner {
            SSARightValueInner::Address(_, _, _, _, _) => true,
            _ => false,
        }
    }

    pub fn ty(&self) -> Type {
        match self.inner {
            SSARightValueInner::Immediate(imme) => imme.get_type(),
            SSARightValueInner::Normal(_, ty) => ty,
            SSARightValueInner::Address(_, ty, _, _, _) => ty,
        }
    }

    pub fn id(&self) -> &i32 {
        match &self.inner {
            SSARightValueInner::Immediate(_) => panic!("id: immediate"),
            SSARightValueInner::Normal(id, _) => id,
            SSARightValueInner::Address(id, _, _, _, _) => id,
        }
    }

    pub fn set_id(&mut self, id: i32) {
        match &mut self.inner {
            SSARightValueInner::Immediate(_) => panic!("set_id: immediate"),
            SSARightValueInner::Normal(id_, _) => *id_ = id,
            SSARightValueInner::Address(id_, _, _, _, _) => *id_ = id,
        }
    }
}

impl Display for SSARightValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match &self.inner {
            SSARightValueInner::Immediate(imme) => write!(f, "{}", imme),
            SSARightValueInner::Normal(id, _) => write!(f, "%{}", id),
            SSARightValueInner::Address(id, _, _, addr_type, _) => match addr_type {
                AddrType::Local => write!(f, "%{}", id),
                AddrType::Global(name) => write!(f, "@{}", name),
            },
        }
    }
}

#[derive(Debug, PartialEq, Clone, Default, Getters, Setters)]
pub struct SSALeftValue {
    #[getset(get = "pub")]
    name: String,
    #[getset(get = "pub")]
    is_arg: bool, // logical arg or arg on stack
    #[getset(get = "pub")]
    is_const: bool,
    #[getset(get = "pub")]
    is_global: bool,
    #[getset(get = "pub")]
    shape: Vec<i32>,
    #[getset(get = "pub", set = "pub")]
    id: i32,
    #[getset(get = "pub", set = "pub")]
    ty: Type,
    #[getset(get = "pub", set = "pub")]
    init_value: Option<Vec<Immediate>>, // only const value has init value
    #[getset(get = "pub", set = "pub")]
    offset: i32,
    #[getset(get = "pub", set = "pub")]
    is_volatile: bool,
    #[getset(get = "pub")]
    is_omit_first_dim: bool, // 1. is_arg == true && is_omit_first_dim == true for array argument
                             // 2. is_arg == true && is_omit_first_dim == false for array in function
}

impl Eq for SSALeftValue {}

impl Hash for SSALeftValue {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.id.hash(state);
    }
}

impl SSALeftValue {
    pub fn new(id: i32, ty: Type) -> Self {
        Self {
            name: String::new(),
            is_arg: false,
            is_const: false,
            is_global: false,
            shape: Vec::new(),
            id,
            ty,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: false,
        }
    }
    // normal variable with name and shape, not arg or global or const
    pub fn new_normal(id: i32, ty: Type, name: String, shape: Vec<i32>) -> Self {
        Self {
            name,
            is_arg: false,
            is_const: false,
            is_global: false,
            shape,
            id,
            ty,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: false,
        }
    }
    pub fn new_arg_scalar(name: String, id: i32, ty: Type) -> Self {
        Self {
            name,
            is_arg: true,
            is_const: false,
            is_global: false,
            shape: Vec::new(),
            id,
            ty,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: false,
        }
    }
    pub fn new_arg_array(name: String, id: i32, ty: Type, shape: Vec<i32>) -> Self {
        Self {
            name,
            is_arg: true,
            is_const: false,
            is_global: false,
            shape,
            id,
            ty,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: true,
        }
    }
    pub fn new_arg_unknown_length_array(name: String, id: i32, ty: Type) -> Self {
        Self {
            name,
            is_arg: true,
            is_const: false,
            is_global: false,
            shape: vec![-1],
            ty,
            id,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: true,
        }
    }
    /// gep source address gen a new address
    pub fn new_addr(id: i32, ty: Type, shape: Vec<i32>) -> Self {
        Self {
            name: String::new(),
            is_arg: false,
            is_const: false,
            is_global: false,
            shape,
            id,
            ty,
            init_value: None,
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: false,
        }
    }

    pub fn new_const_array(
        id: i32,
        ty: Type,
        name: String,
        shape: Vec<i32>,
        init_value: Vec<Immediate>,
    ) -> Self {
        Self {
            name,
            is_arg: false,
            is_const: true,
            is_global: false,
            shape,
            id,
            ty,
            init_value: Some(init_value),
            offset: 0,
            is_volatile: false,
            is_omit_first_dim: false,
        }
    }

    pub fn is_promotable(&self) -> bool {
        !self.is_global && self.shape.is_empty() && self.size() == 4 // && !self.is_arg
    }

    pub fn set_global(&mut self) {
        self.is_global = true;
    }

    pub fn get_name(&self) -> String {
        self.name.clone()
    }

    pub fn get_type(&self) -> Type {
        self.ty
    }

    pub fn get_shape(&self) -> Vec<i32> {
        self.shape.clone()
    }

    pub fn set(&mut self, value: SSALeftValue) {
        *self = value;
    }

    pub fn gen_save_arg_lvalue(&self, id: i32) -> Self {
        let mut new_lvalue = self.clone();
        new_lvalue.id = id;
        new_lvalue
    }

    pub fn to_address(&self) -> SSARightValue {
        if self.is_global {
            SSARightValue {
                inner: SSARightValueInner::Address(
                    self.id,
                    self.ty,
                    self.shape.clone(),
                    AddrType::Global(self.name.clone()),
                    false,
                ),
                origin_id_and_version: None,
            }
        } else {
            SSARightValue {
                inner: SSARightValueInner::Address(
                    self.id,
                    self.ty,
                    self.shape.clone(),
                    AddrType::Local,
                    self.is_arg,
                ),
                origin_id_and_version: None,
            }
        }
    }

    /// if arg is address, keep arg shape
    pub fn to_arg_rvalue(&self) -> SSARightValue {
        assert!(self.is_arg);
        if self.is_omit_first_dim {
            // array arg
            self.to_address()
        } else {
            // scalar arg
            SSARightValue {
                inner: SSARightValueInner::Normal(self.id, self.ty),
                origin_id_and_version: None,
            }
        }
    }

    pub fn size(&self) -> u32 {
        if self.is_omit_first_dim {
            // array arg, must be address, but memory stack not in callee stack, so size is address size
            ADDRESS_SIZE
        } else {
            self.ty.size() * self.shape.iter().product::<i32>() as u32
        }
    }

    pub fn is_array_arg(&self) -> bool {
        self.is_arg && self.shape.len() > 0
    }
}

impl Display for SSALeftValue {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if *self.is_global() {
            write!(f, "@{}", self.name)
        } else {
            write!(f, "%{}", self.id)
        }
    }
}
