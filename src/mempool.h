#pragma once

#include <stddef.h>

void mempool_init();
void *mempool_alloc(size_t size);
void mempool_free(void *ptr);
