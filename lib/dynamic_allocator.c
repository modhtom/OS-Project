/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
bool is_initialized = 0;
struct MemBlock_LIST mem_block_lIST;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	//=========================================
	//=========================================
	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	// Create the first block and initialize its metadata
	// Initialize the first block metadata
	struct BlockMetaData *block = (struct BlockMetaData *)daStart;
	block-> size = initSizeOfAllocatedSpace;
	block->is_free = 1;
	block->prev_next_info.le_prev=NULL;
	block->prev_next_info.le_next=NULL;
	LIST_INIT(&(mem_block_lIST));
	LIST_INSERT_TAIL(&(mem_block_lIST), block);
}
//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	if(size == 0)
		return NULL;
	if (!is_initialized)
	{
	uint32 required_size = size + sizeOfMetaData();
	uint32 da_start = (uint32)sbrk(required_size);
	//get new break since it's page aligned! thus, the size can be more than the required one
	uint32 da_break = (uint32)sbrk(0);
	initialize_dynamic_allocator(da_start, da_break - da_start);
	}
	uint32 req_size = size + sizeOfMetaData();
	struct BlockMetaData *element;
	LIST_FOREACH(element, &(mem_block_lIST))
	{
		if(element != NULL)
		{
			if(element->is_free == 1)
			{
				if(element->size == req_size)
				{
					element->is_free = 0;
					return ((void *)((uint32)element + sizeOfMetaData()));
				}
				if(element->size > req_size)
				{
					if((element->size - req_size) < sizeOfMetaData())
					{
						element->is_free = 0;
						return ((void *)((uint32)element + sizeOfMetaData()));
					}
					struct BlockMetaData *alloc_block = (struct BlockMetaData *)((uint32)element + req_size);
					alloc_block->is_free = 1;
					alloc_block->size = element->size - req_size;
					LIST_INSERT_AFTER(&mem_block_lIST, element, alloc_block);
					element->is_free = 0;
					element->size = req_size;
					return ((void *)((uint32)element + sizeOfMetaData()));
				}
			}
		}
	}
	void *return_val = sbrk(req_size);
	if(return_val == (void*)-1)
	{
		return NULL;
	}
	if(ROUNDUP(req_size, PAGE_SIZE) == req_size)
	{
		struct BlockMetaData *new_block = (struct BlockMetaData *)return_val;
		new_block->size = req_size;
		new_block->is_free = 0;
		LIST_INSERT_TAIL(&mem_block_lIST, new_block);
		return ((void *)((uint32)new_block + sizeOfMetaData()));
	}
	if(ROUNDUP(req_size, PAGE_SIZE) > req_size)
	{
		struct BlockMetaData *new_block = (struct BlockMetaData *)return_val;
		new_block->size = ROUNDUP(req_size, PAGE_SIZE);
		new_block->is_free = 0;
		LIST_INSERT_TAIL(&mem_block_lIST, new_block);
		if((ROUNDUP(req_size, PAGE_SIZE) - req_size) < sizeOfMetaData())
		{
			return ((void *)((uint32)new_block + sizeOfMetaData()));
		}
		struct BlockMetaData *alloc_block = (struct BlockMetaData *)((uint32)new_block + req_size);
		alloc_block->is_free = 1;
		alloc_block->size = (ROUNDUP(req_size, PAGE_SIZE) - req_size);
		LIST_INSERT_AFTER(&mem_block_lIST, new_block, alloc_block);
		new_block->size = req_size;
		return ((void *)((uint32)new_block + sizeOfMetaData()));
	}
	return NULL;
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	if(size == 0)
		return NULL;
	uint32 req_size = size + sizeOfMetaData();
	struct BlockMetaData *element;
    struct BlockMetaData *min_size_block = LIST_LAST(&(mem_block_lIST));
	LIST_FOREACH(element, &(mem_block_lIST))
	{
		if(element != NULL)
		{
			if(element->is_free == 1)
			{
				if(element->size >= req_size)
				{
					if(element->size < min_size_block->size)
					{
						min_size_block = element;
					}
				}
			}
		}
	}
	element = min_size_block;
	if(element != NULL)
	{
		if(element->is_free == 1)
		{
			if(element->size == req_size)
			{
				element->is_free = 0;
				return ((void *)((uint32)element + sizeOfMetaData()));
			}
			if(element->size > req_size)
			{
				if((element->size - req_size) < sizeOfMetaData())
				{
					element->is_free = 0;
					return ((void *)((uint32)element + sizeOfMetaData()));
				}
				struct BlockMetaData *alloc_block = (struct BlockMetaData *)((uint32)element + req_size);
				alloc_block->is_free = 1;
				alloc_block->size = element->size - req_size;
				LIST_INSERT_AFTER(&mem_block_lIST, element, alloc_block);
				element->is_free = 0;
				element->size = req_size;
				return ((void *)((uint32)element + sizeOfMetaData()));
			}
		}
	}
	void *return_val = sbrk(req_size);
	if(return_val == (void *)-1)
		return NULL;
	struct BlockMetaData *new_block = (struct BlockMetaData *)return_val;
	new_block->size = req_size;
	new_block->is_free = 0;
	LIST_INSERT_TAIL(&mem_block_lIST, new_block);
	return ((void *)((uint32)new_block + sizeOfMetaData()));
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	if (va == NULL)
		return;
	struct BlockMetaData *free_block = (struct BlockMetaData *)((uint32)va - sizeOfMetaData());
	struct BlockMetaData *next_block = LIST_NEXT(free_block);
	struct BlockMetaData *prev_block = LIST_PREV(free_block);
	if((prev_block == NULL || prev_block->is_free == 0) &&
	   (next_block == NULL || next_block->is_free == 0))
	{
		free_block->is_free = 1;
	}
	if((prev_block != NULL) && (prev_block->is_free == 1) &&
	   (next_block == NULL || next_block->is_free == 0))
	{
		prev_block->size += free_block->size;
		LIST_REMOVE(&mem_block_lIST, free_block);
		free_block->is_free = 0;
		free_block->size = 0;
		free_block->prev_next_info.le_prev = 0;
		free_block->prev_next_info.le_next = 0;
	}
	if((next_block != NULL) && (next_block->is_free == 1) &&
	   (prev_block == NULL || prev_block->is_free == 0))
	{
		free_block->size += next_block->size;
		free_block->is_free = 1;
		LIST_REMOVE(&mem_block_lIST, next_block);
		next_block->is_free = 0;
		next_block->size = 0;
		next_block->prev_next_info.le_prev = 0;
		next_block->prev_next_info.le_next = 0;
	}
	if((prev_block != NULL) && (prev_block->is_free == 1) &&
	   (next_block != NULL) && (next_block->is_free == 1))
	{
		prev_block->size += free_block->size + next_block->size;
		LIST_REMOVE(&mem_block_lIST, next_block);
		next_block->is_free = 0;
		next_block->size = 0;
		next_block->prev_next_info.le_prev = 0;
		next_block->prev_next_info.le_next = 0;
		LIST_REMOVE(&mem_block_lIST, free_block);
		free_block->is_free = 0;
		free_block->size = 0;
		free_block->prev_next_info.le_prev = 0;
		free_block->prev_next_info.le_next = 0;
	}
}


//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	if(va != NULL && new_size == 0)
	{
		free_block(va);
		return NULL;
	}
	if(va == NULL && new_size != 0)
	{
		void * ret = alloc_block_FF(new_size);
		if(ret == NULL)
			return(void *)-1;
		else
			return ret;
	}
	if(va == NULL && new_size == 0)
		return NULL;
	struct BlockMetaData *old_block = (struct BlockMetaData *)((uint32)va - sizeOfMetaData());
	old_block->is_free = 0;
	if(new_size == (old_block->size - sizeOfMetaData()))
		return va;
	struct BlockMetaData *next_block = LIST_NEXT(old_block);
	struct BlockMetaData *next_next_block = LIST_NEXT(next_block);
	if(new_size > (old_block->size - sizeOfMetaData()))
	{
		if((next_block == NULL) || (next_block->is_free == 0))
		{
			void * ret = alloc_block_FF(new_size);
			if(ret == NULL)
				return(void *)-1;
			else
			{
				free_block(va);
				return ret;
			}
		}
		if(next_block != NULL && next_block->is_free == 1)
		{
			if(next_block->size == (new_size - old_block->size - sizeOfMetaData()))
			{
				old_block->size += next_block->size;
				LIST_REMOVE(&mem_block_lIST, next_block);
				next_block->size = 0;
				next_block->is_free = 0;
				next_block->prev_next_info.le_prev = 0;
				next_block->prev_next_info.le_next = 0;
				return va;
			}
			if(next_block->size > (new_size - old_block->size - sizeOfMetaData()))
			{
				if(next_next_block == NULL || next_next_block->is_free == 0)
				{
					if((next_block->size - (new_size - old_block->size - sizeOfMetaData())) < sizeOfMetaData())
					{
						old_block->size += next_block->size;
						LIST_REMOVE(&mem_block_lIST, next_block);
						next_block->size = 0;
						next_block->is_free = 0;
						next_block->prev_next_info.le_prev = 0;
						next_block->prev_next_info.le_next = 0;
						return va;
					}
					LIST_REMOVE(&mem_block_lIST, next_block);
					struct BlockMetaData *new_block = (struct BlockMetaData *)((uint32)va + sizeOfMetaData() + new_size);
					new_block->is_free = 1;
					new_block->size -= (new_size - old_block->size - sizeOfMetaData());
					old_block->size = new_size + sizeOfMetaData();
					LIST_INSERT_AFTER(&mem_block_lIST, old_block, new_block);
					next_block->size = 0;
					next_block->is_free = 0;
					next_block->prev_next_info.le_prev = 0;
					next_block->prev_next_info.le_next = 0;
					return va;
				}
				if(next_next_block != NULL && next_next_block->is_free == 1)
				{
					LIST_REMOVE(&mem_block_lIST, next_block);
					LIST_REMOVE(&mem_block_lIST, next_next_block);
					struct BlockMetaData * merged_block = (struct BlockMetaData *)((uint32)va + sizeOfMetaData() + new_size);
					merged_block->is_free = 1;
					next_block->size -= (new_size - old_block->size - sizeOfMetaData());
					old_block->size = new_size + sizeOfMetaData();
					merged_block->size = next_next_block->size + next_block->size;
					LIST_INSERT_AFTER(&mem_block_lIST, old_block, merged_block);
					next_block->size = 0;
					next_block->is_free = 0;
					next_block->prev_next_info.le_prev = 0;
					next_block->prev_next_info.le_next = 0;
					next_next_block->is_free = 0;
					next_next_block->size = 0;
					next_next_block->prev_next_info.le_prev = 0;
					next_next_block->prev_next_info.le_next = 0;
					return va;
				}
			}
			if(next_block->size < (new_size - old_block->size - sizeOfMetaData()))
			{
				void * ret = alloc_block_FF(new_size);
				if(ret == NULL)
					return(void *)-1;
				else
				{
					free_block(va);
					return ret;
				}
			}
		}
	}
	if(new_size < (old_block->size - sizeOfMetaData()))
	{
		if((next_block == NULL) || (next_block->is_free == 0))
		{
			if(((old_block->size - sizeOfMetaData()) - new_size) < sizeOfMetaData()) // remain doesn't fit meta data
			{
				return va;
			}
			struct BlockMetaData * remain = (struct BlockMetaData *)((uint32)va + new_size);
			remain->is_free = 1;
			remain->size = old_block->size - sizeOfMetaData() - new_size;
		    old_block->size = sizeOfMetaData() + new_size;
			LIST_INSERT_AFTER(&mem_block_lIST, old_block, remain);
			return va;
		}
		if(next_block != NULL && next_block->is_free == 1)
		{
			LIST_REMOVE(&mem_block_lIST, next_block);
			struct BlockMetaData * merged_block = (struct BlockMetaData *)((uint32)va + sizeOfMetaData() +new_size);
			merged_block->size = next_block->size + ((old_block->size - sizeOfMetaData()) - new_size);
			old_block->size = sizeOfMetaData() + new_size;
			merged_block->is_free = 1;
			LIST_INSERT_AFTER(&mem_block_lIST, old_block, merged_block);
			next_block->size = 0;
			next_block->is_free = 0;
			next_block->prev_next_info.le_prev = 0;
			next_block->prev_next_info.le_next = 0;
			return va;
		}
	}
	return NULL;
}
