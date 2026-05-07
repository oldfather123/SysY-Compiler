//! VecPool - 向量池

use slotmap::{Key, SlotMap};
use std::ops::{Deref, DerefMut, Index, IndexMut};

/// 向量池 - 用于高效管理和重用向量
///
/// VecPool 使用 SlotMap 存储向量，主要用于无需O(1)反向查找的情况
#[derive(Debug, Clone)]
pub struct VecPool<K: Key, T> {
    inner: SlotMap<K, Vec<T>>,
}

impl<K: Key, T> VecPool<K, T> {
    pub fn new() -> Self {
        Self {
            inner: SlotMap::with_key(),
        }
    }

    /// 创建指定容量的向量池
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            inner: SlotMap::with_capacity_and_key(capacity),
        }
    }

    /// 插入一个新的向量到池中
    pub fn insert(&mut self, vec: Vec<T>) -> K {
        self.inner.insert(vec)
    }

    /// 插入一个新的空向量到池中
    pub fn insert_empty(&mut self) -> K {
        self.insert(Vec::new())
    }

    /// 从可迭代对象创建并插入向量
    pub fn insert_from_iter<I>(&mut self, iter: I) -> K
    where
        I: IntoIterator<Item = T>,
    {
        self.insert(iter.into_iter().collect())
    }

    /// 获取向量的引用
    pub fn get(&self, key: K) -> Option<&Vec<T>> {
        self.inner.get(key)
    }

    /// 获取向量的可变引用
    pub fn get_mut(&mut self, key: K) -> Option<&mut Vec<T>> {
        self.inner.get_mut(key)
    }

    /// 移除并返回向量
    pub fn remove(&mut self, key: K) -> Option<Vec<T>> {
        self.inner.remove(key)
    }

    /// 清空池中的所有向量
    pub fn clear(&mut self) {
        self.inner.clear()
    }

    /// 获取池中向量的数量
    pub fn len(&self) -> usize {
        self.inner.len()
    }

    /// 检查池是否为空
    pub fn is_empty(&self) -> bool {
        self.inner.is_empty()
    }

    /// 获取向量作为切片
    pub fn as_slice(&self, key: K) -> Option<&[T]> {
        self.get(key).map(|v| v.as_slice())
    }

    /// 获取向量作为可变切片
    pub fn as_mut_slice(&mut self, key: K) -> Option<&mut [T]> {
        self.get_mut(key).map(|v| v.as_mut_slice())
    }

    /// 向指定向量追加元素
    pub fn push(&mut self, key: K, value: T) -> bool {
        if let Some(vec) = self.get_mut(key) {
            vec.push(value);
            true
        } else {
            false
        }
    }

    /// 从指定向量弹出元素
    pub fn pop(&mut self, key: K) -> Option<T> {
        self.get_mut(key)?.pop()
    }

    /// 扩展指定向量
    pub fn extend<I>(&mut self, key: K, iter: I) -> bool
    where
        I: IntoIterator<Item = T>,
    {
        if let Some(vec) = self.get_mut(key) {
            vec.extend(iter);
            true
        } else {
            false
        }
    }

    /// 清空指定向量的内容（但保留容量）
    pub fn clear_vec(&mut self, key: K) -> bool {
        if let Some(vec) = self.get_mut(key) {
            vec.clear();
            true
        } else {
            false
        }
    }

    /// 获取指定向量的长度
    pub fn vec_len(&self, key: K) -> Option<usize> {
        self.get(key).map(|v| v.len())
    }

    /// 检查指定向量是否为空
    pub fn vec_is_empty(&self, key: K) -> Option<bool> {
        self.get(key).map(|v| v.is_empty())
    }
}

impl<K: Key, T> Default for VecPool<K, T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<K: Key, T> Index<K> for VecPool<K, T> {
    type Output = Vec<T>;

    fn index(&self, key: K) -> &Self::Output {
        self.get(key).expect("invalid VecPool key used")
    }
}

impl<K: Key, T> IndexMut<K> for VecPool<K, T> {
    fn index_mut(&mut self, key: K) -> &mut Self::Output {
        self.get_mut(key).expect("invalid VecPool key used")
    }
}

/// VecPool 的视图，提供对池中向量的引用
#[derive(Debug)]
pub struct VecView<'a, T> {
    vec: &'a Vec<T>,
}

impl<T> Deref for VecView<'_, T> {
    type Target = Vec<T>;

    fn deref(&self) -> &Self::Target {
        self.vec
    }
}

/// VecPool 的可变视图，提供对池中向量的可变引用
#[derive(Debug)]
pub struct VecViewMut<'a, T> {
    vec: &'a mut Vec<T>,
}

impl<T> Deref for VecViewMut<'_, T> {
    type Target = Vec<T>;

    fn deref(&self) -> &Self::Target {
        self.vec
    }
}

impl<T> DerefMut for VecViewMut<'_, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.vec
    }
}

impl<K: Key, T> VecPool<K, T> {
    /// 获取向量的视图
    pub fn view(&self, key: K) -> Option<VecView<'_, T>> {
        self.get(key).map(|vec| VecView { vec })
    }

    /// 获取向量的可变视图
    pub fn view_mut(&mut self, key: K) -> Option<VecViewMut<'_, T>> {
        self.get_mut(key).map(|vec| VecViewMut { vec })
    }
}
