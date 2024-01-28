#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

uint32 user_alloc_block_arr[NUM_OF_KHEAP_PAGES];
bool is_init = 0;
//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
	if(sys_isUHeapPlacementStrategyFIRSTFIT() == 1)
	{
    	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
    	{
			if(!is_init)
			{
				initialize_dynamic_allocator(USER_HEAP_START, 200 * (3*sizeof(int) + sizeOfMetaData() + 2*sizeOfMetaData() + 20*sizeof(char) + sizeOfMetaData() + 1024/2 + 1*1024 + 3*1024/2 + 2*1024));
				is_init = 1;
			}
    		return alloc_block_FF(size);
    	}
    	else
    	{
    		int no_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
    		int count;
    		for(uint32 i = (sys_get_user_hard_limit() + PAGE_SIZE); i < USER_HEAP_MAX; i += PAGE_SIZE)
    		{
    			count = 0;
				uint32 mapped = sys_get_frame_info(i);
				if(mapped == 0)
				{
					for(int j = i; j < (i + ROUNDUP(size, PAGE_SIZE)); j += PAGE_SIZE)
					{
						if(j >= USER_HEAP_MAX)
							return NULL;
						mapped = sys_get_frame_info(j);
						if(mapped == 0)
						{
							count++;
						}
						else
						{
							i = j;
							break;
						}
					}
				}
				if(count == no_pages)
				{
					sys_allocate_user_mem(i, size);
					user_alloc_block_arr[(i - (sys_get_user_hard_limit() + PAGE_SIZE)) / PAGE_SIZE] = ROUNDUP(size, PAGE_SIZE);
					return (void*)i;
				}
    		}
    		return NULL;
    	}
	}
	return NULL;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	if(((uint32)virtual_address >= USER_HEAP_START) && ((uint32)virtual_address < sys_get_user_hard_limit()))
	{
		free_block(virtual_address);
	}
	else if(((uint32)virtual_address >= (sys_get_user_hard_limit() + PAGE_SIZE)) && ((uint32)virtual_address < USER_HEAP_MAX))
	{
		sys_free_user_mem((uint32)virtual_address, user_alloc_block_arr[((uint32)virtual_address - (sys_get_user_hard_limit() + PAGE_SIZE)) / PAGE_SIZE]);
	}
	else
		panic("INVALID ADDRESS");
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
