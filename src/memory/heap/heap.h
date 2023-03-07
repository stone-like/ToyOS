#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>


#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_TS_FIRST 0b01000000

//heapEntryは1entry8bitで
//前半4bit           ->   |has_Next|IS_FIRST|NaN|NaN|
//後半4bit entryType ->   |    Free or Taken        |

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table{
    HEAP_BLOCK_TABLE_ENTRY* entries;
    size_t total;
};

//heapの構造はsaddrから実際のheap、ここからメモリを持ってきたり、開放したり
//heap_tableはblockの情報とtotalBlock数

//tableで情報を取って来てからsAddr経由で実際のメモリ操作1
struct heap{
    struct heap_table* table;

    void* saddr;
};

int heap_create(struct heap* heap,void* ptr,void* end,struct heap_table* table);
void* heap_malloc(struct heap* heap,size_t size);

void heap_free(struct heap* heap,void* ptr);

#endif