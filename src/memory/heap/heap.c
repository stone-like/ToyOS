#include "heap.h"
#include "kernel.h"
#include "memory/memory.h"
#include "status.h"
#include <stdbool.h>

static int heap_validate_table(void *start,void *end,struct heap_table* table){

  size_t table_size = (size_t)(end-start);
  size_t total_blocks = table_size / TOYOS_HEAP_BLOCK_SIZE;
  if (table->total != total_blocks){
   return -EINVARG;
  }

  return 0;
}

static bool heap_validate_alignment(void* ptr){
  return ((unsigned int)ptr % TOYOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap* heap,void* start,void* end,struct heap_table* table){
  if(!heap_validate_alignment(start) || !heap_validate_alignment(end)){
    return -EINVARG;
  }

  memset(heap,0,sizeof(struct heap));
  heap->saddr = start;
  heap->table = table;

  int res = heap_validate_table(start,end,table);
  if (res < 0){
    return -EINVARG;
  }

  size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
  memset(table->entries,HEAP_BLOCK_TABLE_ENTRY_FREE,table_size);

  return 0;
}



static uint32_t heap_align_value_to_upper(uint32_t val){
  if ((val % TOYOS_HEAP_BLOCK_SIZE) == 0){
    return val;
  }

  val = val - (val % TOYOS_HEAP_BLOCK_SIZE);
  return val + TOYOS_HEAP_BLOCK_SIZE;
}

//entryTypeはentryの後半4bit
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry){
  return entry & 0x0f;
}

int heap_get_start_block(struct heap* heap,uint32_t total_blocks){
  struct heap_table* table = heap->table;

  int block_count = 0;
  int start_block = -1;

  for (size_t i = 0; i < table->total; i++)
  {
    if(heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE){
      block_count = 0;
      start_block = -1;
      continue;
    }

    if (start_block == -1){
      start_block = i;
    }

    block_count++;

    if(block_count == total_blocks){
      break;
    }
  }

  if(start_block == -1){
    return -ENOMEM;
  }

  return start_block;
  
}

void* heap_block_to_address(struct heap* heap,int block){
  return heap->saddr + (block*TOYOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap* heap,int start_block,int total_blocks){

  int end_block = (start_block + total_blocks) -1;
  //start = 0 + total = 5 = 5で0スタートになっているので-1

  HEAP_BLOCK_TABLE_ENTRY firstEntry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_TS_FIRST;

  if(total_blocks > 1){
    firstEntry |= HEAP_BLOCK_HAS_NEXT;
  }

  heap->table->entries[start_block] = firstEntry;

  for (int i = start_block+1; i <= end_block; i++)
  {
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

    if (i != end_block){
      entry |= HEAP_BLOCK_HAS_NEXT;
    }
    heap->table->entries[i] = entry;
  }
  
}

void* heap_malloc_blocks(struct heap* heap,uint32_t total_blocks){

  void *address = 0;
  int start_block = heap_get_start_block(heap,total_blocks);

  if(start_block < 0){
    return address;
  }
  
  address = heap_block_to_address(heap,start_block);

  heap_mark_blocks_taken(heap,start_block,total_blocks);

  return address;
 
}



void* heap_malloc(struct heap* heap,size_t size){
  size_t aligned_size = heap_align_value_to_upper(size);
  uint32_t total_blocks = aligned_size / TOYOS_HEAP_BLOCK_SIZE;
  return heap_malloc_blocks(heap,total_blocks);
}

void heap_mark_blocks_free(struct heap* heap,int starting_block){
  struct heap_table* table = heap->table;

  for (int i = starting_block; i < (int)table->total; i++)
  {
    HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
    table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

    if (!(entry & HEAP_BLOCK_HAS_NEXT)){
      break;
    }

  }
  
}

int heap_address_to_block(struct heap* heap,void* address){
  return ((int)(address - heap->saddr)/TOYOS_HEAP_BLOCK_SIZE);
}

void heap_free(struct heap* heap,void* ptr){
  heap_mark_blocks_free(heap,heap_address_to_block(heap,ptr));
}