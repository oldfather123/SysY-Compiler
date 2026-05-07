//! 基于slotmap的双向映射 RefMap
//!

use refkey_utils::RefUtils;
use slotmap::{Key, SlotMap, new_key_type};
use std::collections::HashMap;
use std::hash::Hash;
use std::ops::{Index, IndexMut};

// 使用SlotMap解决enum递归的Box/Rc/Arc装箱麻烦
new_key_type! {
    #[derive(RefUtils)]
    pub struct RefKey;

    // === AST节点引用 ===
    /// 编译单元引用
    #[derive(RefUtils)]
    pub struct CompUnitRef;
    /// 声明引用
    #[derive(RefUtils)]
    pub struct DeclRef;
    /// 语句引用
    #[derive(RefUtils)]
    pub struct StmtRef;
    /// 表达式引用
    #[derive(RefUtils)]
    pub struct ExprRef;

    // === SSA/IR引用 ===
    /// 基本块引用
    #[derive(RefUtils)]
    pub struct Block;
    /// 指令引用
    #[derive(RefUtils)]
    pub struct Inst;
    /// 值
    #[derive(RefUtils)]
    pub struct Value;
    /// 名字
    #[derive(RefUtils)]
    pub struct NameRef;
    /// 函数签名
    #[derive(RefUtils)]
    pub struct FuncSigRef;
    /// 跳转表
    #[derive(RefUtils)]
    pub struct JumpTable;
    /// 循环引用
    #[derive(RefUtils)]
    pub struct Loop;

    // === ISLE Copy trait ===
    #[derive(RefUtils)]
    pub struct ValueVec;
    #[derive(RefUtils)]
    pub struct U32OptVec;

    // === 优化分析 ===
    /// 强连通分量
    #[derive(RefUtils)]
    pub struct Scc;
    /// 栈槽
    #[derive(RefUtils)]
    pub struct StackSlot;
}

#[derive(Debug, Clone)]
pub struct RefMap<K: Key, V> {
    // 主要的前向映射 Key -> Value
    forward: SlotMap<K, V>,
    // 反向映射 Value -> Key 用于高效的值查找和intern操作
    backward: HashMap<V, K>,
    // 字符串索引 String -> Vec<K> 用于优化字符串查找
    string_index: HashMap<String, Vec<K>>,
}

impl<K: Key, V> RefMap<K, V> {
    pub fn new() -> Self {
        Self {
            forward: SlotMap::with_key(),
            backward: HashMap::new(),
            string_index: HashMap::new(),
        }
    }

    pub fn clear(&mut self) {
        self.forward.clear();
        self.backward.clear();
        self.string_index.clear();
    }

    pub fn with_key() -> Self {
        Self::new()
    }

    pub fn get(&self, key: K) -> Option<&V> {
        self.forward.get(key)
    }

    pub fn get_mut(&mut self, key: K) -> Option<&mut V> {
        self.forward.get_mut(key)
    }

    pub fn len(&self) -> usize {
        self.forward.len()
    }

    pub fn is_empty(&self) -> bool {
        self.forward.is_empty()
    }

    pub fn iter(&self) -> impl Iterator<Item = (K, &V)> {
        self.forward.iter()
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = (K, &mut V)> {
        self.forward.iter_mut()
    }

    /// 基础插入操作 -- 仅支持不需要反向映射的类型
    pub fn insert(&mut self, val: V) -> K {
        self.forward.insert(val)
    }

    /// 带反向映射的插入操作 -- 用于支持高效查找的类型
    pub fn insert_with_backward(&mut self, val: V) -> K
    where
        V: Hash + Eq + Clone,
    {
        let key = self.forward.insert(val.clone());
        self.backward.insert(val, key);
        key
    }

    /// 带字符串索引的插入操作 -- 用于支持字符串查找的类型
    pub fn insert_with_string_index<F>(&mut self, val: V, string_extractor: F) -> K
    where
        F: Fn(&V) -> &str,
        V: Clone,
    {
        let key = self.forward.insert(val.clone());
        let string_key = string_extractor(&val).to_string();
        self.string_index.entry(string_key).or_default().push(key);
        key
    }

    /// 完整插入操作 -- 同时维护所有索引
    pub fn insert_full<F>(&mut self, val: V, string_extractor: F) -> K
    where
        F: Fn(&V) -> &str,
        V: Hash + Eq + Clone,
    {
        let key = self.forward.insert(val.clone());
        self.backward.insert(val.clone(), key);
        let string_key = string_extractor(&val).to_string();
        self.string_index.entry(string_key).or_default().push(key);
        key
    }

    /// 移除指定键的条目
    pub fn remove(&mut self, key: K) -> Option<V> {
        self.forward.remove(key)
    }

    /// 移除指定键的条目，同时维护反向映射
    pub fn remove_with_backward(&mut self, key: K) -> Option<V>
    where
        V: Hash + Eq,
    {
        if let Some(val) = self.forward.remove(key) {
            self.backward.remove(&val);
            Some(val)
        } else {
            None
        }
    }

    /// 移除指定键的条目，同时维护所有索引
    pub fn remove_full<F>(&mut self, key: K, string_extractor: F) -> Option<V>
    where
        F: Fn(&V) -> &str,
        V: Hash + Eq,
    {
        if let Some(val) = self.forward.remove(key) {
            // 从反向映射中移除
            self.backward.remove(&val);

            // 从字符串索引中移除
            let string_key = string_extractor(&val).to_string();
            if let Some(keys) = self.string_index.get_mut(&string_key) {
                keys.retain(|&k| k != key);
                if keys.is_empty() {
                    self.string_index.remove(&string_key);
                }
            }

            Some(val)
        } else {
            None
        }
    }

    /// 获取所有键的迭代器
    pub fn keys(&self) -> impl Iterator<Item = K> + '_ {
        self.forward.keys()
    }

    /// Returns true if the slot map contains key.
    pub fn contains_key(&self, key: K) -> bool {
        self.forward.contains_key(key)
    }
}

impl<K: Key, V> Default for RefMap<K, V> {
    fn default() -> Self {
        Self::new()
    }
}

impl<K: Key, V> Index<K> for RefMap<K, V> {
    type Output = V;

    fn index(&self, key: K) -> &Self::Output {
        &self.forward[key]
    }
}

impl<K: Key, V> IndexMut<K> for RefMap<K, V> {
    fn index_mut(&mut self, key: K) -> &mut Self::Output {
        &mut self.forward[key]
    }
}

/// === 优化后 O(1)复杂度 ===

/// 反向映射高效查找
impl<K: Key + 'static, V: Hash + Eq + Clone> RefMap<K, V> {
    /// intern 操作 -- 使用引用, 用于大型类型如 TypeData、Vec<_>
    pub fn intern(&mut self, val: &V) -> K {
        if let Some(&existing_key) = self.backward.get(val) {
            existing_key
        } else {
            self.insert_with_backward(val.clone())
        }
    }

    /// intern 操作 -- 使用拥有的值, 用于小型类型如基本数值类型
    pub fn intern_owned(&mut self, val: V) -> K {
        if let Some(&existing_key) = self.backward.get(&val) {
            existing_key
        } else {
            self.insert_with_backward(val)
        }
    }

    /// 值查找 -- 使用引用, 用于大型类型如 TypeData、Vec<_>
    pub fn find(&self, val: &V) -> Option<K> {
        self.backward.get(val).copied()
    }

    /// 值查找 -- 使用拥有的值, 用于小型类型如基本数值类型
    pub fn find_owned(&self, val: V) -> Option<K> {
        self.backward.get(&val).copied()
    }
}

/// 支持字符串索引的高效字符串查找操作
impl<K: Key + 'static, V> RefMap<K, V> {
    /// 字符串查找 -- 使用字符串索引
    pub fn find_str_indexed(&self, target: &str) -> Option<K> {
        self.string_index
            .get(target)
            .and_then(|keys| keys.first().copied())
    }

    /// 混合模式字符串查找 -- 优先使用索引，回退到线性查找
    pub fn find_str<F>(&self, target: &str, extractor: F) -> Option<K>
    where
        F: Fn(&V) -> &str,
    {
        // 首先使用字符串索引进行O(1)查找
        if let Some(key) = self.find_str_indexed(target) {
            return Some(key);
        }

        // 回退到线性查找 兼容性
        self.with_predicate_fallback(|v| extractor(v) == target)
    }

    /// 根据提取函数查找匹配的键 -- 优化版本
    pub fn find_by<T, F>(&self, target: T, extractor: F) -> Option<K>
    where
        T: PartialEq,
        F: Fn(&V) -> T,
    {
        self.with_predicate_fallback(|v| extractor(v) == target)
    }

    /// 根据提取函数查找匹配的键 -- 引用版本
    pub fn find_by_ref<T, U, F>(&self, target: T, extractor: F) -> Option<K>
    where
        T: PartialEq<U>,
        F: Fn(&V) -> U,
    {
        self.with_predicate_fallback(|v| target == extractor(v))
    }

    /// 线性查找的回退实现 -- 简化版本
    fn with_predicate_fallback<F>(&self, predicate: F) -> Option<K>
    where
        F: Fn(&V) -> bool,
    {
        // 直接线性查找，放弃性能
        for (k, v) in self.iter() {
            if predicate(v) {
                return Some(k);
            }
        }
        None
    }

    /// 旧版本 with_predicate 方法
    #[allow(dead_code)]
    fn with_predicate<F>(&self, predicate: F) -> Option<K>
    where
        F: Fn(&V) -> bool,
    {
        self.with_predicate_fallback(predicate)
    }
}

// === 高级优化方法 ===

/// 完整索引高效操作
impl<K: Key + 'static, V: Hash + Eq + Clone> RefMap<K, V> {
    /// 完整的intern操作 -- 使用引用, 用于大型类型
    pub fn intern_with_string_index<F>(&mut self, val: &V, string_extractor: F) -> K
    where
        F: Fn(&V) -> &str,
    {
        if let Some(&existing_key) = self.backward.get(val) {
            existing_key
        } else {
            self.insert_full(val.clone(), string_extractor)
        }
    }

    /// 完整的intern操作 -- 使用拥有的值, 用于小型类型
    pub fn intern_with_string_index_owned<F>(&mut self, val: V, string_extractor: F) -> K
    where
        F: Fn(&V) -> &str,
    {
        if let Some(&existing_key) = self.backward.get(&val) {
            existing_key
        } else {
            self.insert_full(val, string_extractor)
        }
    }

    /// 批量插入操作 -- 高效构建索引
    pub fn insert_batch<I, F>(&mut self, values: I, string_extractor: F) -> Vec<K>
    where
        I: IntoIterator<Item = V>,
        F: Fn(&V) -> &str,
    {
        let mut keys = Vec::new();
        for val in values {
            keys.push(self.insert_full(val, &string_extractor));
        }
        keys
    }

    /// 清理索引 -- 移除无效的字符串索引项
    pub fn cleanup_string_index(&mut self) {
        self.string_index.retain(|_str, keys| {
            keys.retain(|&key| self.forward.contains_key(key));
            !keys.is_empty()
        });
    }

    /// 获取索引统计信息
    pub fn index_stats(&self) -> (usize, usize, usize) {
        let forward_size = self.forward.len();
        let backward_size = self.backward.len();
        let string_index_size = self.string_index.len();
        (forward_size, backward_size, string_index_size)
    }

    /// 获取详细的统计信息
    pub fn detailed_stats(&self) -> String {
        let (forward_size, backward_size, string_index_size) = self.index_stats();
        let (forward_mem, backward_mem, string_mem) = self.memory_usage();

        format!(
            "RefMap<{}>: entries=[{}/{}/{}], memory={}B",
            std::any::type_name::<K>(),
            forward_size,
            backward_size,
            string_index_size,
            forward_mem + backward_mem + string_mem
        )
    }

    /// 获取内存使用情况
    pub fn memory_usage(&self) -> (usize, usize, usize) {
        let forward_memory = self.forward.len() * std::mem::size_of::<V>();
        let backward_memory =
            self.backward.len() * (std::mem::size_of::<V>() + std::mem::size_of::<K>());
        let string_memory = self
            .string_index
            .iter()
            .map(|(k, v)| k.len() + v.len() * std::mem::size_of::<K>())
            .sum::<usize>();
        (forward_memory, backward_memory, string_memory)
    }
}

// === 兼容性实现 ===

/// 旧版本API的兼容性实现
impl<K: Key + 'static, V> RefMap<K, V> {
    /// 兼容版本的find方法
    pub fn find_compat(&self, val: V) -> Option<K>
    where
        V: PartialEq,
    {
        self.with_predicate_fallback(|v| *v == val)
    }

    /// 兼容版本的intern方法
    pub fn intern_compat(&mut self, val: V) -> K
    where
        V: PartialEq + Clone,
    {
        if let Some(existing_key) = self.find_compat(val.clone()) {
            existing_key
        } else {
            self.insert(val)
        }
    }
}

// 为 &RefMap 实现 IntoIterator
impl<'a, K: Key, V> IntoIterator for &'a RefMap<K, V> {
    type Item = (K, &'a V);
    type IntoIter = slotmap::basic::Iter<'a, K, V>;

    fn into_iter(self) -> Self::IntoIter {
        self.forward.iter()
    }
}

// 为 &mut RefMap 实现 IntoIterator
impl<'a, K: Key, V> IntoIterator for &'a mut RefMap<K, V> {
    type Item = (K, &'a mut V);
    type IntoIter = slotmap::basic::IterMut<'a, K, V>;

    fn into_iter(self) -> Self::IntoIter {
        self.forward.iter_mut()
    }
}
