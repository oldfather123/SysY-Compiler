use slotmap::{Key, new_key_type};

use crate::{prelude::*, utils::refmap_marco::impl_refmap_wrapper};
use refkey_utils::RefUtils;

#[derive(Debug)]
pub struct Egraph {
    pub(super) eclasses: Eclasses,
    pub(super) value_eclass: RefSecMap<Value, Eclass>,
}

impl Egraph {
    pub(super) fn new() -> Self {
        Self {
            eclasses: Eclasses::new(),
            value_eclass: RefSecMap::new(),
        }
    }

    pub(super) fn insert(&mut self, value: Value) {
        let eclass = self.eclasses.insert(EclassData(vec![value]));
        let old = self.value_eclass.insert(value, eclass);
        assert!(old.is_none());
    }

    pub(super) fn ensure(&mut self, value: Value) {
        if self.value_eclass.contains_key(value) {
            return;
        }
        self.insert(value);
    }

    pub(super) fn get_eclass(&self, value: Value) -> Option<Eclass> {
        self.value_eclass.get(value).cloned()
    }

    pub(super) fn merge(&mut self, from: Eclass, into: Eclass) {
        let mut from_data = self.eclasses.remove(from).unwrap();
        let into_data = self.eclasses.get_mut(into).unwrap();
        for value in from_data.0.iter().cloned() {
            let old = self.value_eclass.insert(value, into).unwrap();
            assert_eq!(old, from);
        }
        into_data.0.append(&mut from_data.0);
    }
}

new_key_type! {
    #[derive(RefUtils)]
    pub struct Eclass;
}

/// contains value refs that belong to this eclass
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct EclassData(pub(super) Vec<Value>);

#[derive(Debug)]
pub struct Eclasses(pub RefMap<Eclass, EclassData>);
impl_refmap_wrapper!(Eclasses, Eclass, EclassData);
