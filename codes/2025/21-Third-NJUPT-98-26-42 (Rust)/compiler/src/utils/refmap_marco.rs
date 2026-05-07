//! RefMap 方法转发宏
//!
//! 为包装了 RefMap 的结构体自动实现所有 RefMap 方法的转发。
//! 这个宏用于简化 Blocks, Insts 等包装结构体的实现。

/// 为包装了 `RefMap<K, V>` 的结构体实现所有 RefMap 方法的转发
///
/// # 用法
/// ```rust
/// use crate::utils::refmap_marco::impl_refmap_wrapper;
///
/// pub struct Blocks(pub RefMap<Block, BlockData>);
/// impl_refmap_wrapper!(Blocks, Block, BlockData);
/// ```
#[macro_export]
macro_rules! impl_refmap_wrapper {
    ($wrapper:ident, $key:ty, $value:ty) => {
        impl $wrapper {
            // === 基础方法 ===

            /// 创建新的空包装器
            pub fn new() -> Self {
                Self(RefMap::new())
            }

            /// 清空所有内容
            pub fn clear(&mut self) {
                self.0.clear()
            }

            /// 使用key创建新的包装器
            pub fn with_key() -> Self {
                Self(RefMap::with_key())
            }

            /// 获取指定键的值的不可变引用
            pub fn get(&self, key: $key) -> Option<&$value> {
                self.0.get(key)
            }

            /// 获取指定键的值的可变引用
            pub fn get_mut(&mut self, key: $key) -> Option<&mut $value> {
                self.0.get_mut(key)
            }

            /// 获取条目数量
            pub fn len(&self) -> usize {
                self.0.len()
            }

            /// 检查是否为空
            pub fn is_empty(&self) -> bool {
                self.0.is_empty()
            }

            /// 获取所有键值对的不可变迭代器
            pub fn iter_all(&self) -> impl Iterator<Item = ($key, &$value)> {
                self.0.iter()
            }

            /// 获取所有键值对的可变迭代器
            pub fn iter_all_mut(&mut self) -> impl Iterator<Item = ($key, &mut $value)> {
                self.0.iter_mut()
            }

            /// 基础插入操作
            pub fn insert(&mut self, val: $value) -> $key {
                self.0.insert(val)
            }

            /// 移除指定键的条目
            pub fn remove(&mut self, key: $key) -> Option<$value> {
                self.0.remove(key)
            }

            /// 获取所有键的迭代器
            pub fn keys(&self) -> impl Iterator<Item = $key> + '_ {
                self.0.keys()
            }

            /// Returns true if the slot map contains key.
            pub fn contains_key(&self, key: $key) -> bool {
                self.0.contains_key(key)
            }
        }

        impl $wrapper
        where
            $value: std::hash::Hash + Eq + Clone,
        {
            // === 带反向映射的方法 ===

            /// 带反向映射的插入操作
            pub fn insert_with_backward(&mut self, val: $value) -> $key {
                self.0.insert_with_backward(val)
            }

            /// 移除指定键的条目，同时维护反向映射
            pub fn remove_with_backward(&mut self, key: $key) -> Option<$value> {
                self.0.remove_with_backward(key)
            }

            /// intern 操作 -- 使用引用, 用于大型类型
            pub fn intern(&mut self, val: &$value) -> $key {
                self.0.intern(val)
            }

            /// intern 操作 -- 使用拥有的值, 用于小型类型
            pub fn intern_owned(&mut self, val: $value) -> $key {
                self.0.intern_owned(val)
            }

            /// 值查找 -- 使用引用
            pub fn find(&self, val: &$value) -> Option<$key> {
                self.0.find(val)
            }

            /// 值查找 -- 使用拥有的值
            pub fn find_owned(&self, val: $value) -> Option<$key> {
                self.0.find_owned(val)
            }

            /// 获取索引统计信息
            pub fn index_stats(&self) -> (usize, usize, usize) {
                self.0.index_stats()
            }

            /// 获取详细的统计信息
            pub fn detailed_stats(&self) -> String {
                self.0.detailed_stats()
            }

            /// 获取内存使用情况
            pub fn memory_usage(&self) -> (usize, usize, usize) {
                self.0.memory_usage()
            }
        }

        impl $wrapper {
            // === 字符串索引相关方法 ===

            /// 带字符串索引的插入操作
            pub fn insert_with_string_index<F>(&mut self, val: $value, string_extractor: F) -> $key
            where
                F: Fn(&$value) -> &str,
                $value: Clone,
            {
                self.0.insert_with_string_index(val, string_extractor)
            }

            /// 完整插入操作 -- 同时维护所有索引
            pub fn insert_full<F>(&mut self, val: $value, string_extractor: F) -> $key
            where
                F: Fn(&$value) -> &str,
                $value: std::hash::Hash + Eq + Clone,
            {
                self.0.insert_full(val, string_extractor)
            }

            /// 移除指定键的条目，同时维护所有索引
            pub fn remove_full<F>(&mut self, key: $key, string_extractor: F) -> Option<$value>
            where
                F: Fn(&$value) -> &str,
                $value: std::hash::Hash + Eq,
            {
                self.0.remove_full(key, string_extractor)
            }

            /// 字符串查找 -- 使用字符串索引
            pub fn find_str_indexed(&self, target: &str) -> Option<$key> {
                self.0.find_str_indexed(target)
            }

            /// 混合模式字符串查找
            pub fn find_str<F>(&self, target: &str, extractor: F) -> Option<$key>
            where
                F: Fn(&$value) -> &str,
            {
                self.0.find_str(target, extractor)
            }

            /// 根据提取函数查找匹配的键
            pub fn find_by<T, F>(&self, target: T, extractor: F) -> Option<$key>
            where
                T: PartialEq,
                F: Fn(&$value) -> T,
            {
                self.0.find_by(target, extractor)
            }

            /// 根据提取函数查找匹配的键 -- 引用版本
            pub fn find_by_ref<T, U, F>(&self, target: T, extractor: F) -> Option<$key>
            where
                T: PartialEq<U>,
                F: Fn(&$value) -> U,
            {
                self.0.find_by_ref(target, extractor)
            }
        }

        impl $wrapper
        where
            $value: std::hash::Hash + Eq + Clone,
        {
            // === 高级优化方法 ===

            /// 完整的intern操作 -- 使用引用
            pub fn intern_with_string_index<F>(&mut self, val: &$value, string_extractor: F) -> $key
            where
                F: Fn(&$value) -> &str,
            {
                self.0.intern_with_string_index(val, string_extractor)
            }

            /// 完整的intern操作 -- 使用拥有的值
            pub fn intern_with_string_index_owned<F>(
                &mut self,
                val: $value,
                string_extractor: F,
            ) -> $key
            where
                F: Fn(&$value) -> &str,
            {
                self.0.intern_with_string_index_owned(val, string_extractor)
            }

            /// 批量插入操作
            pub fn insert_batch<I, F>(&mut self, values: I, string_extractor: F) -> Vec<$key>
            where
                I: IntoIterator<Item = $value>,
                F: Fn(&$value) -> &str,
            {
                self.0.insert_batch(values, string_extractor)
            }

            /// 清理索引
            pub fn cleanup_string_index(&mut self) {
                self.0.cleanup_string_index()
            }
        }

        impl $wrapper {
            // === 兼容性方法 ===

            /// 兼容版本的find方法
            pub fn find_compat(&self, val: $value) -> Option<$key>
            where
                $value: PartialEq,
            {
                self.0.find_compat(val)
            }

            /// 兼容版本的intern方法
            pub fn intern_compat(&mut self, val: $value) -> $key
            where
                $value: PartialEq + Clone,
            {
                self.0.intern_compat(val)
            }
        }

        // === Trait 实现 ===

        impl Default for $wrapper {
            fn default() -> Self {
                Self::new()
            }
        }

        impl std::ops::Index<$key> for $wrapper {
            type Output = $value;

            fn index(&self, key: $key) -> &Self::Output {
                &self.0[key]
            }
        }

        impl std::ops::IndexMut<$key> for $wrapper {
            fn index_mut(&mut self, key: $key) -> &mut Self::Output {
                &mut self.0[key]
            }
        }

        // 为 &$wrapper 实现 IntoIterator
        impl<'a> IntoIterator for &'a $wrapper {
            type Item = ($key, &'a $value);
            type IntoIter = slotmap::basic::Iter<'a, $key, $value>;

            fn into_iter(self) -> Self::IntoIter {
                (&self.0).into_iter()
            }
        }

        // 为 &mut $wrapper 实现 IntoIterator
        impl<'a> IntoIterator for &'a mut $wrapper {
            type Item = ($key, &'a mut $value);
            type IntoIter = slotmap::basic::IterMut<'a, $key, $value>;

            fn into_iter(self) -> Self::IntoIter {
                (&mut self.0).into_iter()
            }
        }
    };
}

// 导出宏以便在其他模块中使用
pub use impl_refmap_wrapper;
