//! 暴露低级地址计算的优化
//!
//! opts2 所有数组操作均为Alloc/Load/Store的低级地址操作

pub mod addr_expand;
pub use addr_expand::AddressExpander;
pub mod licm;
pub use licm::licm;
