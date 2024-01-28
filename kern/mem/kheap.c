#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

uint32 alloc_block_arr[NUM_OF_KHEAP_PAGES];
uint32 fno_va_arr[NUM_OF_KHEAP_PAGES];
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM
	if((daStart + initSizeToAllocate) > daLimit)
		return E_NO_MEM;
	segstart = daStart;
	segbrk= ROUNDUP(daStart + initSizeToAllocate, PAGE_SIZE);
	seglimit = daLimit;
	uint32 start_va = daStart;
	uint32 end_va = segbrk;
	uint32 i = start_va;
	while(i < end_va)
	{
		struct FrameInfo *ptr_frame_info = NULL;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, i, PERM_WRITEABLE);
		uint32 *ptr_page_table = NULL;
		get_page_table(ptr_page_directory, i, &ptr_page_table);
		fno_va_arr[ptr_page_table[PTX(i)] >> 12] = i;
		i += PAGE_SIZE;
	}
	initialize_dynamic_allocator(daStart,initSizeToAllocate);
	return 0;
}

void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */
	if(increment == 0)
		return (void*)segbrk;
	 if(increment > 0)
	 {
         uint32 newbrk = ROUNDUP(segbrk + increment, PAGE_SIZE);
		 if(newbrk > seglimit)
			 panic("exceeds hard limit");
		 uint32 oldbrk = segbrk;
		 segbrk = newbrk;
		 for(uint32 i = ROUNDDOWN(oldbrk, PAGE_SIZE); i < segbrk; i += PAGE_SIZE)
		 {
			uint32 *ptr_page_table = NULL;
			struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory, i, &ptr_page_table);
			if(ptr_frame_info == NULL)
			{
				allocate_frame(&ptr_frame_info);
				map_frame(ptr_page_directory, ptr_frame_info, i, PERM_WRITEABLE);
			}
		 }
		 return (void*)oldbrk;
	 }
     if(increment < 0)
     {
    	 uint32 newbrk = ROUNDDOWN(segbrk + increment, PAGE_SIZE);
    	 uint32 oldbrk = segbrk;
    	 segbrk = segbrk + increment;
    	 for(uint32 i = newbrk; i < ROUNDUP(oldbrk, PAGE_SIZE); i += PAGE_SIZE)
		 {
			unmap_frame(ptr_page_directory, i);
		 }
		 return (void*)segbrk;
     }
	return NULL;
}

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
    if(isKHeapPlacementStrategyFIRSTFIT() == 1)
    {
    	if (size == 0)
		{
			return NULL;
		}
    	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
    	{
    		uint32 va = (uint32)alloc_block_FF(size);
    		if(va != 0)
    		{
				for(uint32 i = ROUNDDOWN(va - sizeOfMetaData(), PAGE_SIZE); i < (ROUNDUP(va + size, PAGE_SIZE)); i += PAGE_SIZE)
				{
					uint32 *ptr_page_table = NULL;
					get_page_table(ptr_page_directory, i, &ptr_page_table);
					fno_va_arr[ptr_page_table[PTX(i)] >> 12] = i;
				}
    		}
    		return (void*)va;
    	}
    	else
    	{
    		int no_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
    		int count;
    		for(uint32 i = (seglimit + PAGE_SIZE); i < KERNEL_HEAP_MAX; i += PAGE_SIZE)
    		{
    			count = 0;
    			struct FrameInfo *ptr_frame_info = NULL;
				uint32 *ptr_page_table = NULL;
				ptr_frame_info = get_frame_info(ptr_page_directory, i, &ptr_page_table);
				if(ptr_frame_info == NULL)
				{
					for(uint32 j = i; j < (i + ROUNDUP(size, PAGE_SIZE)); j += PAGE_SIZE)
					{
						ptr_frame_info = get_frame_info(ptr_page_directory, j, &ptr_page_table);
						if(ptr_frame_info == NULL)
						{
							count++;
						}
						else
							break;
					}
				}
				if(count == no_pages)
				{
					for(uint32 j = i; j < (i + ROUNDUP(size, PAGE_SIZE)); j += PAGE_SIZE)
					{
						allocate_frame(&ptr_frame_info);
						map_frame(ptr_page_directory, ptr_frame_info, j, PERM_WRITEABLE);
						get_page_table(ptr_page_directory, j, &ptr_page_table);
						fno_va_arr[ptr_page_table[PTX(j)] >> 12] = j;
					}
					alloc_block_arr[(i - (seglimit + PAGE_SIZE)) / PAGE_SIZE] = ROUNDUP(size, PAGE_SIZE);
					return (void*)i;
				}
    		}
    		return NULL;
    	}
    }
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	if(((uint32)virtual_address >= segstart) && ((uint32)virtual_address < seglimit))
	{
		free_block(virtual_address);
	}
	else if(((uint32)virtual_address >= (seglimit + PAGE_SIZE)) && ((uint32)virtual_address < KERNEL_HEAP_MAX))
	{
		for(uint32 i = (uint32)virtual_address; i < ((uint32)virtual_address + alloc_block_arr[((uint32)virtual_address - (seglimit + PAGE_SIZE)) / PAGE_SIZE]); i += PAGE_SIZE)
		{
			uint32 *ptr_page_table = NULL;
			get_page_table(ptr_page_directory, i, &ptr_page_table);
			fno_va_arr[ptr_page_table[PTX(i)] >> 12] = 0;
			unmap_frame(ptr_page_directory, i);
		}
	}
	else
		panic("INVALID ADDRESS");
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	uint32 va = fno_va_arr[physical_address >> 12];
	if(va == 0)
		return 0;
	uint32 offset = physical_address << 20;
	offset = offset >> 20;
	va = va >> 12;
	va = va << 12;
	return (va | offset);
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	uint32 *ptr_page_table = NULL;
	struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory, virtual_address, &ptr_page_table);
	if(ptr_frame_info == NULL)
		return 0;
	uint32 offset = virtual_address << 20;
	offset = offset >> 20;
	unsigned int phys_address = (ptr_page_table[PTX(virtual_address)] >> 12) * PAGE_SIZE + offset;
	return phys_address;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	if(virtual_address != NULL && new_size == 0)
	{
		kfree(virtual_address);
		return NULL;
	}
	if(virtual_address == NULL && new_size != 0)
	{
		return kmalloc(new_size);
	}
	if(virtual_address == NULL && new_size == 0)
		return NULL;
	if((((uint32)virtual_address >= segstart) && ((uint32)virtual_address < seglimit)) && (new_size <= DYN_ALLOC_MAX_BLOCK_SIZE))
	{
		return realloc_block_FF(virtual_address, new_size);
	}
	if((((uint32)virtual_address >= (seglimit + PAGE_SIZE)) && ((uint32)virtual_address < KERNEL_HEAP_MAX)) && (new_size > DYN_ALLOC_MAX_BLOCK_SIZE))
	{
		uint32 old_size = alloc_block_arr[((uint32)virtual_address - (seglimit + PAGE_SIZE)) / PAGE_SIZE];
		if(new_size == old_size)
			return virtual_address;
		if(new_size > old_size)
		{
			int no_pages = ROUNDUP(new_size - old_size, PAGE_SIZE) / PAGE_SIZE;
			int count;
			count = 0;
			struct FrameInfo *ptr_frame_info = NULL;
			uint32 *ptr_page_table = NULL;
			for(uint32 j = ((uint32)virtual_address + old_size); j < ROUNDUP((uint32)virtual_address + new_size, PAGE_SIZE); j += PAGE_SIZE)
			{
				ptr_frame_info = get_frame_info(ptr_page_directory, j, &ptr_page_table);
				if(ptr_frame_info == NULL)
				{
					count++;
				}
				else
					break;
			}
			if(count == no_pages)
			{
				for(uint32 j = ((uint32)virtual_address + old_size); j < ROUNDUP((uint32)virtual_address + new_size, PAGE_SIZE); j += PAGE_SIZE)
				{
					allocate_frame(&ptr_frame_info);
					map_frame(ptr_page_directory, ptr_frame_info, j, PERM_WRITEABLE);
					get_page_table(ptr_page_directory, j, &ptr_page_table);
					fno_va_arr[ptr_page_table[PTX(j)] >> 12] = j;
				}
				alloc_block_arr[((uint32)virtual_address - (seglimit + PAGE_SIZE)) / PAGE_SIZE] = ROUNDUP(new_size, PAGE_SIZE);
				return virtual_address;
			}
			else
			{
				void *ret = kmalloc(new_size);
				if(ret == NULL)
					return NULL;
				else
				{
					kfree(virtual_address);
					return ret;
				}
			}
		}
		if(new_size < old_size)
		{
			for(uint32 i = ROUNDDOWN((uint32)virtual_address + new_size, PAGE_SIZE); i < ((uint32)virtual_address + old_size); i += PAGE_SIZE)
			{
				uint32 *ptr_page_table = NULL;
				get_page_table(ptr_page_directory, i, &ptr_page_table);
				fno_va_arr[ptr_page_table[PTX(i)] >> 12] = 0;
				unmap_frame(ptr_page_directory, i);
			}
			alloc_block_arr[((uint32)virtual_address - (seglimit + PAGE_SIZE)) / PAGE_SIZE] = ROUNDUP(new_size, PAGE_SIZE);
			return virtual_address;
		}
	}
	return NULL;
}


