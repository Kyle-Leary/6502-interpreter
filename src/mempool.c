#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MEMPOOL_SIZE 1024 * 1000

typedef struct BlockHeader {
  size_t size;
  struct BlockHeader *next;
} BlockHeader;

static uint8_t mempool[MEMPOOL_SIZE];
static BlockHeader *freeList = NULL;

void mempool_init() {
  freeList = (BlockHeader *)mempool;
  freeList->size = MEMPOOL_SIZE - sizeof(BlockHeader);
  freeList->next = NULL;
}

void *mempool_alloc(size_t size) {
  BlockHeader **prev = &freeList;
  BlockHeader *curr = freeList;

  while (curr) {
    if (curr->size >= size + sizeof(BlockHeader)) {
      *prev = curr->next;

      BlockHeader *newBlock =
          (BlockHeader *)((uint8_t *)curr + sizeof(BlockHeader) + size);
      newBlock->size = curr->size - size - sizeof(BlockHeader);
      newBlock->next = freeList;
      freeList = newBlock;

      curr->size = size;
      curr->next = NULL;
      return (uint8_t *)curr + sizeof(BlockHeader);
    }

    prev = &(curr->next);
    curr = curr->next;
  }

  fprintf(stderr, "mempool failure to allocate %lu bytes\n", size);
  return NULL;
}

void mempool_free(void *ptr) {
  if (!ptr)
    return;

  BlockHeader *block = (BlockHeader *)((uint8_t *)ptr - sizeof(BlockHeader));
  block->next = freeList;
  freeList = block;

  // Simple coalescing strategy
  BlockHeader **prev = &freeList;
  BlockHeader *curr = freeList;
  while (curr && curr->next) {
    if ((uint8_t *)curr + sizeof(BlockHeader) + curr->size ==
        (uint8_t *)(curr->next)) {
      curr->size += sizeof(BlockHeader) + curr->next->size;
      curr->next = curr->next->next;
      continue;
    }

    prev = &(curr->next);
    curr = curr->next;
  }
}
