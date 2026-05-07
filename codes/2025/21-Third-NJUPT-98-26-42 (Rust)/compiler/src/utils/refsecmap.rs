//! RefSecMap - 支持嵌套索引的二级映射
//!
//! 该模块提供了对 slotmap::SecondaryMap 的包装，支持自动创建嵌套结构

use slotmap::{Key, SecondaryMap};
use std::ops::{Index, IndexMut};

/// RefSecMap - 二级映射的包装器
/// 用于关联主映射的键与值
#[derive(Debug, Clone)]
pub struct RefSecMap<K: Key, V> {
    inner: SecondaryMap<K, V>,
}

impl<K: Key, V> RefSecMap<K, V> {
    /// 创建一个新的 RefSecMap
    pub fn new() -> Self {
        Self {
            inner: SecondaryMap::new(),
        }
    }

    /// 创建指定容量的 RefSecMap
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            inner: SecondaryMap::with_capacity(capacity),
        }
    }

    /// 插入键值对
    pub fn insert(&mut self, key: K, value: V) -> Option<V> {
        self.inner.insert(key, value)
    }

    /// 获取值的引用
    pub fn get(&self, key: K) -> Option<&V> {
        self.inner.get(key)
    }

    /// 获取值的可变引用
    pub fn get_mut(&mut self, key: K) -> Option<&mut V> {
        self.inner.get_mut(key)
    }

    /// 检查是否包含键
    pub fn contains_key(&self, key: K) -> bool {
        self.inner.contains_key(key)
    }

    /// 移除键值对
    pub fn remove(&mut self, key: K) -> Option<V> {
        self.inner.remove(key)
    }

    /// 清空所有键值对
    pub fn clear(&mut self) {
        self.inner.clear()
    }

    /// 获取元素数量
    pub fn len(&self) -> usize {
        self.inner.len()
    }

    /// 检查是否为空
    pub fn is_empty(&self) -> bool {
        self.inner.is_empty()
    }

    /// 获取或插入默认值
    pub fn get_or_insert_with<F>(&mut self, key: K, f: F) -> &mut V
    where
        F: FnOnce() -> V,
    {
        if !self.contains_key(key) {
            self.insert(key, f());
        }
        self.inner.get_mut(key).unwrap()
    }

    /// 获取或插入默认值（使用 Default trait）
    pub fn get_or_insert_default(&mut self, key: K) -> &mut V
    where
        V: Default,
    {
        self.get_or_insert_with(key, V::default)
    }

    /// 返回键值对的迭代器
    pub fn iter(&self) -> impl Iterator<Item = (K, &V)> {
        self.inner.iter()
    }

    /// 返回键值对的可变迭代器
    pub fn iter_mut(&mut self) -> impl Iterator<Item = (K, &mut V)> {
        self.inner.iter_mut()
    }

    /// 返回所有键的迭代器
    pub fn keys(&self) -> impl Iterator<Item = K> + '_ {
        self.inner.keys()
    }

    /// 返回所有值的迭代器
    pub fn values(&self) -> impl Iterator<Item = &V> {
        self.inner.values()
    }

    /// 返回所有值的可变迭代器
    pub fn values_mut(&mut self) -> impl Iterator<Item = &mut V> {
        self.inner.values_mut()
    }
}

impl<K: Key, V> Default for RefSecMap<K, V> {
    fn default() -> Self {
        Self::new()
    }
}

impl<K: Key, V> Index<K> for RefSecMap<K, V> {
    type Output = V;

    fn index(&self, key: K) -> &Self::Output {
        self.get(key).expect("invalid RefSecMap key used")
    }
}

impl<K: Key, V> IndexMut<K> for RefSecMap<K, V>
where
    V: Default,
{
    fn index_mut(&mut self, key: K) -> &mut Self::Output {
        self.get_or_insert_default(key)
    }
}

/// 支持嵌套索引的二级映射
/// 专门用于处理类似 map[key1][key2] = value 的场景
#[derive(Debug, Clone)]
pub struct NestedRefSecMap<K1: Key, K2: Key, V> {
    inner: RefSecMap<K1, RefSecMap<K2, V>>,
}

impl<K1: Key, K2: Key, V> NestedRefSecMap<K1, K2, V> {
    /// 创建新的嵌套映射
    pub fn new() -> Self {
        Self {
            inner: RefSecMap::new(),
        }
    }

    /// 设置值
    pub fn set(&mut self, key1: K1, key2: K2, value: V) {
        self.inner.get_or_insert_default(key1).insert(key2, value);
    }

    /// 获取值
    pub fn get(&self, key1: K1, key2: K2) -> Option<&V> {
        self.inner.get(key1)?.get(key2)
    }

    /// 安全获取值，如果不存在返回默认值
    pub fn get_or_default(&self, key1: K1, key2: K2) -> V
    where
        V: Default + Clone,
    {
        self.get(key1, key2).cloned().unwrap_or_default()
    }

    /// 获取可变引用
    pub fn get_mut(&mut self, key1: K1, key2: K2) -> Option<&mut V> {
        self.inner.get_mut(key1)?.get_mut(key2)
    }

    /// 检查是否包含键
    pub fn contains(&self, key1: K1, key2: K2) -> bool {
        self.inner.get(key1).is_some_and(|m| m.contains_key(key2))
    }

    /// 清空所有数据
    pub fn clear(&mut self) {
        self.inner.clear()
    }

    pub fn is_empty(&self) -> bool {
        self.inner.is_empty()
    }
}

impl<K1: Key, K2: Key, V> Default for NestedRefSecMap<K1, K2, V> {
    fn default() -> Self {
        Self::new()
    }
}

// 实现 Index trait 以支持 map[key1][key2] 读取
impl<K1: Key, K2: Key, V> Index<K1> for NestedRefSecMap<K1, K2, V> {
    type Output = RefSecMap<K2, V>;

    fn index(&self, key: K1) -> &Self::Output {
        &self.inner[key]
    }
}

// 实现 IndexMut trait 以支持 map[key1][key2] = value 写入
impl<K1: Key, K2: Key, V> IndexMut<K1> for NestedRefSecMap<K1, K2, V> {
    fn index_mut(&mut self, key: K1) -> &mut Self::Output {
        &mut self.inner[key]
    }
}
