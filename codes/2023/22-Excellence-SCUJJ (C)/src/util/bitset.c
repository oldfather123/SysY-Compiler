

#include "../include/bitset.h"

#include "../include/m-bitset.h"

/**
 * @brief 弹出一个元素为1的索引后将原值设置为0
 * @param[in] set
 * @param[in] *index
 * @return flase set中没有有效值了
 */
bool bitset_pop_index(bitset_t set, size_t *index) {
    bitset_it_t it;
    for (bitset_it(it, set); !bitset_end_p(it); bitset_next(it)) {
        bool c = *bitset_cref(it);
        if (c) {
            m_bitset_set_at(set, it->index, 0);
            *index = it->index;
            return true;
        }
    }
    return false;
}
/**
 * @brief: 集合中是否含有1的元素
 * @param {bitset_t} set
 * @return {1含有,0没有元素1了}
 */
bool bitset_is_empty(bitset_t set) {
    bitset_it_t it;
    for (bitset_it(it, set); !bitset_end_p(it); bitset_next(it)) {
        bool c = *bitset_cref(it);
        if (c) {
            return 0;
        }
    }
    return 1;
}
/**
 * @brief: 集合中元素全部初始化为0
 * @param {bitset_t} set
 */
void bitset_init_empty(bitset_t set) {
    bitset_it_t it;
    for (bitset_it(it, set); !bitset_end_p(it); bitset_next(it)) {
        bitset_set_at(set, it->index, 0);
    }
}

void bitset_init_true(bitset_t set) {
    bitset_it_t it;
    for (bitset_it(it, set); !bitset_end_p(it); bitset_next(it)) {
        bitset_set_at(set, it->index, 1);
    }
}
/**
 * @brief: 集合的绝对值(集合中1的个数)
 * @param {bitset_t} set
 * @return {集合中元素1的总数}
 */
size_t bitset_absolute(bitset_t set) {
    size_t      size = 0;
    bitset_it_t it;
    for (bitset_it(it, set); !bitset_end_p(it); bitset_next(it)) {
        bool c = *bitset_cref(it);
        if (c) {
            size++;
        }
    }
    return size;
}
/**
 * @brief: left-right集合的差值,结果传递给ret
 * @param {bitset_t} left
 * @param {bitset_t} right
 * @param {bitset_t} result
 */
void bitset_relative(bitset_t left, bitset_t right, bitset_t ret) {
    int      size = 0;
    bitset_t result;
    bitset_init_set(result, left);
    // if(result->size == 0) goto _end;
    assert(left->size == right->size);
    bitset_it_t it;
    for (bitset_it(it, left); !bitset_end_p(it); bitset_next(it)) {
        bool l = *bitset_cref(it);
        bool r = bitset_get(right, it->index);
        if (!r) {
            bitset_set_at(result, it->index, l);
        } else {
            bitset_set_at(result, it->index, 0);
        }
    }
_end:
    *ret = *result;
}
/**
 * @brief: 填充集合的大小,并初始化0
 * @param {bitset_t} set
 * @param {size_t} size
 * @return {*}
 */
void bitset_set_size(bitset_t set, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) {
        bitset_push_back(set, 0);
    }
}