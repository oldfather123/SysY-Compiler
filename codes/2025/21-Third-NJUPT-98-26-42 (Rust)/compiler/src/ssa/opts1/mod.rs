//! 含有内存令牌的高阶形式优化
//!
//! opts1 所有数组操作均为ArrayGet/Put的高级形式所进行的优化
//!

pub mod const_phi_remove;
pub mod inline;
// pub mod licm;
pub mod remove_unreach;
pub use const_phi_remove::remove_const_phi;
pub use inline::Inliner;
pub use remove_unreach::remove_unreach;

use super::*;
