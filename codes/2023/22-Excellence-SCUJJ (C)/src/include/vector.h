#ifndef _VECTOR_H
#define _VECTOR_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

#define vector_template(data_t, vector) \
    struct {                            \
        size_t  capacity;               \
        size_t  count;                  \
        data_t *data;                   \
    } vector

#define vector_init(this)     \
    do {                      \
        (this)->count = 0;    \
        (this)->capacity = 0; \
        (this)->data = NULL;  \
    } while (0)

#define vector_move(dest, source)              \
    do {                                       \
        (dest)->count = (source)->count;       \
        (dest)->capacity = (source)->capacity; \
        (dest)->data = (source)->data;         \
    } while (0)
#define vector_reserve(this, size)                  \
    do {                                            \
        size_t item_size = sizeof((this)->data[0]); \
        size_t vector_size;                         \
        (this)->count = 0;                          \
        (this)->capacity = size;                    \
        vector_size = (this)->capacity * item_size; \
        (this)->data = sy_malloc(vector_size);      \
    } while (0)

#define vector_destroy(this) free((this)->data)

#define vector_push_back(this, item)                           \
    do {                                                       \
        size_t item_size = sizeof((this)->data[0]);            \
        size_t vector_size;                                    \
        if ((this)->capacity == 0) {                           \
            (this)->capacity = 2;                              \
            vector_size = (this)->capacity * item_size;        \
            (this)->data = malloc(vector_size);                \
            assert((this)->data && "Out of Memory");           \
        }                                                      \
        if ((this)->count >= (this)->capacity) {               \
            (this)->capacity *= 2;                             \
            vector_size = (this)->capacity * item_size;        \
            (this)->data = realloc((this)->data, vector_size); \
            assert((this)->data);                              \
        }                                                      \
        (this)->data[(this)->count] = (item);                  \
        (this)->count++;                                       \
    } while (0)
#define vector_insert(this, index, item)                            \
    do {                                                            \
        size_t item_size = sizeof((this)->data[0]);                 \
        size_t vector_size;                                         \
        if ((this)->count == (this)->capacity) {                    \
            (this)->capacity *= 1.5;                                \
            vector_size = (this)->capacity * item_size;             \
            (this)->data = realloc((this)->data, vector_size);      \
            assert((this)->data);                                   \
        }                                                           \
        memmove((this)->data + (index) + 1, (this)->data + (index), \
                ((this)->count - (index)) * item_size);             \
        (this)->count++;                                            \
        (this)->data[(index)] = (item);                             \
    } while (0)
#define vector_remove(this, index)                                      \
    do {                                                                \
        if ((this)->count - 1 == index) {                               \
            (this)->count--;                                            \
        } else {                                                        \
            size_t item_size = sizeof((this)->data[0]);                 \
            memmove((this)->data + (index), (this)->data + (index) + 1, \
                    ((this)->count - (index)) * item_size);             \
            (this)->count--;                                            \
        }                                                               \
    } while (0)

#define vector_pop_back(this, dest)           \
    {                                         \
        assert((this)->count > 0);            \
        (this)->count--;                      \
        (dest) = (this)->data[(this)->count]; \
    }

#define vector_get(this, index)         ((this)->data[(index)])
#define vector_get_address(this, index) (&((this)->data[(index)]))

#define vector_set(this, index, item)    \
    do {                                 \
        assert((index) < (this)->count); \
        (this)->data[(index)] = item;    \
    } while (0)

#define vector_len(this)           ((this)->count)
#define vector_data(this)          ((this)->data)

/**
 * @brief 得到末尾的元素
 */
#define vector_final(this)         vector_get(this, vector_len(this) - 1)
/**
 * @brief 得到第一个元素
 */
#define vector_first(this)         vector_get(this, 0)
#define vector_first_address(this) (&(vector_get(this, 0)))

#define vector_clear(this) \
    do {                   \
        (this)->count = 0; \
    } while (0)

#define vector_free(this) free((this)->data)

#define vector_each(this, i, item) \
    for (i = 0; i < (this)->count ? (item = (this)->data[i], 1) : 0; i++)
#define vector_each3(this, item) \
    for (size_t i = 0; i < (this)->count ? (item = (this)->data[i], 1) : 0; i++)

#define vector_each2(this, i, type, item) \
    for (size_t i = 0, type item;         \
         i < (this)->count ? (item = (this)->data[i], 1) : 0; i++)

#define vector_each_reverse(this, i, item) \
    for (i = (this)->count; i > 0 ? (item = (this)->data[i - 1], 1) : 0; i--)

#define vector_each_address(this, i, address) \
    for (i = 0; i < (this)->count ? (address = (this)->data + i, 1) : 0; i++)
#define vector_each_reverse_address(this, i, address)                         \
    for (i = (this)->count - 1; i >= 0 ? (address = (this)->data + i, 1) : 0; \
         i--)

#define vector_copy(this, src)                                         \
    do {                                                               \
        (this)->count = (src)->count;                                  \
        (this)->capacity = (src)->capacity;                            \
        size_t item_size = sizeof((src)->data[0]);                     \
        (this)->data = sy_malloc((src)->capacity * item_size);            \
        memcpy((this)->data, (src)->data, (src)->capacity *item_size); \
    } while (0);

#endif