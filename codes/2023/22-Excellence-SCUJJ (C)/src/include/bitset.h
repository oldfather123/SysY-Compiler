#ifndef SHAYLIB_BITSET_H
#define SHAYLIB_BITSET_H

#include <stdint.h>

#include "m-bitset.h"


bool   bitset_is_empty(bitset_t set);
void   bitset_init_empty(bitset_t set);
void bitset_init_true(bitset_t set);
size_t bitset_absolute(bitset_t set);
void   bitset_relative(bitset_t left, bitset_t right, bitset_t ret);
void   bitset_set_size(bitset_t set, size_t size);
bool   bitset_pop_index(bitset_t set, size_t *index);

#endif