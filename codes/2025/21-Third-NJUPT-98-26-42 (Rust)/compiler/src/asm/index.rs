//! 类型化索引
//!
//! 专门用于处理从RefKey到ASM内部使用的index的转换
//! 1. define_index!(IndexType) - 只定义索引类型
//! 2. define_index!(IndexType, StorageType, ElementType) - 定义索引类型和对应的存储容器
#[macro_export]
macro_rules! define_index {
    // 完整模式：定义索引类型和存储容器
    ($ix:ident, $storage:ident, $elem:ident) => {
        // 先定义索引类型
        define_index!($ix);

        /// 索引化存储容器
        #[derive(Clone, Debug, Default)]
        pub struct $storage {
            storage: Vec<$elem>,
        }

        impl $storage {
            /// 创建新的存储容器
            #[inline(always)]
            pub fn new() -> Self {
                Self {
                    storage: Vec::new(),
                }
            }

            /// 返回容器长度
            #[inline(always)]
            pub fn len(&self) -> usize {
                self.storage.len()
            }

            /// 判断容器是否为空
            #[inline(always)]
            pub fn is_empty(&self) -> bool {
                self.storage.is_empty()
            }

            /// 迭代器
            #[inline(always)]
            pub fn iter(&self) -> impl Iterator<Item = &$elem> {
                self.storage.iter()
            }

            /// 可变迭代器
            #[inline(always)]
            pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut $elem> {
                self.storage.iter_mut()
            }

            /// 添加元素并返回索引
            #[inline(always)]
            pub fn push(&mut self, value: $elem) -> $ix {
                let idx = $ix(self.storage.len() as u32);
                self.storage.push(value);
                idx
            }

            /// 根据索引获取元素
            #[inline(always)]
            pub fn get(&self, idx: $ix) -> Option<&$elem> {
                if idx.is_valid() {
                    self.storage.get(idx.index())
                } else {
                    None
                }
            }

            /// 根据索引获取可变元素
            #[inline(always)]
            pub fn get_mut(&mut self, idx: $ix) -> Option<&mut $elem> {
                if idx.is_valid() {
                    self.storage.get_mut(idx.index())
                } else {
                    None
                }
            }
        }

        // 实现 Index trait，允许使用 [] 操作符
        impl std::ops::Index<$ix> for $storage {
            type Output = $elem;

            #[inline(always)]
            fn index(&self, i: $ix) -> &Self::Output {
                &self.storage[i.index()]
            }
        }

        // 实现 IndexMut trait，允许使用 [] 操作符进行可变访问
        impl std::ops::IndexMut<$ix> for $storage {
            #[inline(always)]
            fn index_mut(&mut self, i: $ix) -> &mut Self::Output {
                &mut self.storage[i.index()]
            }
        }

        // 为存储容器实现 IntoIterator
        impl<'a> IntoIterator for &'a $storage {
            type Item = &'a $elem;
            type IntoIter = std::slice::Iter<'a, $elem>;

            #[inline(always)]
            fn into_iter(self) -> Self::IntoIter {
                self.storage.iter()
            }
        }
    };

    // 简单模式：只定义索引类型
    ($ix:ident) => {
        /// 类型安全的索引包装器
        #[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
        pub struct $ix(pub u32);

        impl $ix {
            /// 从 usize 创建索引
            #[inline(always)]
            pub fn new(i: usize) -> Self {
                debug_assert!(i < u32::MAX as usize, "索引溢出");
                Self(i as u32)
            }

            /// 获取内部索引值
            #[inline(always)]
            pub fn index(self) -> usize {
                debug_assert!(self.is_valid(), "使用无效索引");
                self.0 as usize
            }

            /// 创建无效索引
            #[inline(always)]
            pub fn invalid() -> Self {
                Self(u32::MAX)
            }

            /// 检查索引是否无效
            #[inline(always)]
            pub fn is_invalid(self) -> bool {
                self.0 == u32::MAX
            }

            /// 检查索引是否有效
            #[inline(always)]
            pub fn is_valid(self) -> bool {
                self.0 != u32::MAX
            }

            /// 获取下一个索引
            #[inline(always)]
            pub fn next(self) -> $ix {
                debug_assert!(self.is_valid(), "无效索引不能递增");
                debug_assert!(self.0 < u32::MAX - 1, "索引递增溢出");
                Self(self.0 + 1)
            }

            /// 获取上一个索引
            #[inline(always)]
            pub fn prev(self) -> $ix {
                debug_assert!(self.is_valid(), "无效索引不能递减");
                debug_assert!(self.0 > 0, "索引递减下溢");
                Self(self.0 - 1)
            }

            /// 获取原始 u32 值
            #[inline(always)]
            pub fn raw_u32(self) -> u32 {
                self.0
            }
        }

        // 实现 Display，直接显示数字值，方便嵌入到strum宏中
        impl std::fmt::Display for $ix {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                if self.is_valid() {
                    write!(f, "{}", self.0)
                } else {
                    write!(f, "invalid")
                }
            }
        }

        // 实现 Debug，显示带类型名的完整信息
        impl std::fmt::Debug for $ix {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                if self.is_valid() {
                    write!(f, "{}({})", stringify!($ix), self.0)
                } else {
                    write!(f, "{}(invalid)", stringify!($ix))
                }
            }
        }
    };
}

// // 示例用法：定义一些常用的索引类型
// define_index!(InstId);
// define_index!(BlockId);
// define_index!(RegId);

// // 定义索引范围类型，用于表示连续的索引区间
// #[derive(Clone, Copy, Debug)]
// pub struct IndexRange<T> {
//     pub start: T,
//     pub end: T,
// }

// impl<T: Copy> IndexRange<T> {
//     /// 创建新的索引范围
//     #[inline(always)]
//     pub fn new(start: T, end: T) -> Self {
//         IndexRange { start, end }
//     }
// }

// // 专门为 InstId 实现范围类型
// impl IndexRange<InstId> {
//     /// 获取范围长度
//     #[inline(always)]
//     pub fn len(&self) -> usize {
//         self.end.index() - self.start.index()
//     }

//     /// 判断范围是否为空
//     #[inline(always)]
//     pub fn is_empty(&self) -> bool {
//         self.len() == 0
//     }

//     /// 获取第一个元素
//     #[inline(always)]
//     pub fn first(&self) -> Option<InstId> {
//         if !self.is_empty() {
//             Some(self.start)
//         } else {
//             None
//         }
//     }

//     /// 获取最后一个元素
//     #[inline(always)]
//     pub fn last(&self) -> Option<InstId> {
//         if !self.is_empty() {
//             Some(self.end.prev())
//         } else {
//             None
//         }
//     }

//     /// 创建迭代器
//     #[inline(always)]
//     pub fn iter(&self) -> impl Iterator<Item = InstId> {
//         (self.start.index()..self.end.index()).map(InstId::new)
//     }
// }

// #[cfg(test)]
// mod tests {
//     use super::*;

//     // 定义测试用的索引和存储类型
//     define_index!(TestId, TestStorage, String);

//     #[test]
//     fn test_index_creation() {
//         let idx = TestId::new(42);
//         assert_eq!(idx.index(), 42);
//         assert!(idx.is_valid());

//         let invalid = TestId::invalid();
//         assert!(invalid.is_invalid());
//     }

//     #[test]
//     fn test_index_navigation() {
//         let idx = TestId::new(10);
//         let next = idx.next();
//         assert_eq!(next.index(), 11);

//         let prev = next.prev();
//         assert_eq!(prev.index(), 10);
//     }

//     #[test]
//     fn test_storage() {
//         let mut storage = TestStorage::new();

//         let id1 = storage.push("first".to_string());
//         let id2 = storage.push("second".to_string());

//         assert_eq!(storage.len(), 2);
//         assert_eq!(&storage[id1], "first");
//         assert_eq!(&storage[id2], "second");

//         storage[id1] = "modified".to_string();
//         assert_eq!(&storage[id1], "modified");
//     }

//     #[test]
//     fn test_index_range() {
//         let range = IndexRange::new(InstId::new(5), InstId::new(10));
//         assert_eq!(range.len(), 5);
//         assert_eq!(range.first().unwrap().index(), 5);
//         assert_eq!(range.last().unwrap().index(), 9);

//         let items: Vec<_> = range.iter().collect();
//         assert_eq!(items.len(), 5);
//         assert_eq!(items[0].index(), 5);
//         assert_eq!(items[4].index(), 9);
//     }
// }
