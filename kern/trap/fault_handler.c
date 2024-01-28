/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
	if(isPageReplacmentAlgorithmFIFO())
	{
		struct FrameInfo *ptr_frame_info = NULL;
		allocate_frame(&ptr_frame_info);
		map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE);
		int ret = pf_read_env_page(curenv, (void*)fault_va);
		if (ret == E_PAGE_NOT_EXIST_IN_PF)
		{
			if(fault_va < USER_HEAP_START)
			{
				sched_kill_env(curenv->env_id);
			}
			else if((fault_va >= USER_HEAP_MAX) && (fault_va < USTACKBOTTOM))
			{
				sched_kill_env(curenv->env_id);
			}
			else if(fault_va >= USTACKTOP)
			{
				sched_kill_env(curenv->env_id);
			}
		}
		struct WorkingSetElement *element = env_page_ws_list_create_element(curenv, fault_va);
		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			if(curenv->page_WS_list.size == 0)
			{
				LIST_INSERT_HEAD(&curenv->page_WS_list, element);
				curenv->page_last_WS_element = NULL;
			}
			struct WorkingSetElement *last_element = NULL;
			int counter = 0;
			struct WorkingSetElement *ptr_WS_element = NULL;
			LIST_FOREACH(ptr_WS_element, &curenv->page_WS_list)
			{
				if(ptr_WS_element != NULL)
				{
					last_element = ptr_WS_element;
					counter++;
				}
				else
					break;
			}
			if((counter + 1) == curenv->page_WS_max_size)
			{
				struct WorkingSetElement *first_element = LIST_FIRST(&curenv->page_WS_list);
				LIST_NEXT(element) = first_element;
				LIST_PREV(first_element) = element;
				LIST_INSERT_AFTER(&curenv->page_WS_list, last_element, element);
				curenv->page_last_WS_element = first_element;
			}
			else
			{
				LIST_INSERT_AFTER(&curenv->page_WS_list, last_element, element);
				curenv->page_last_WS_element = NULL;
			}
			//refer to the project presentation and documentation for details
		}
		else
		{
			//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
			//refer to the project presentation and documentation for details
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			struct WorkingSetElement *prev_element = curenv->page_last_WS_element;
			uint32 *ptr_page_table = NULL;
			uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, prev_element->virtual_address);
			if(page_permissions & PERM_MODIFIED)
			{
				pf_update_env_page(curenv, prev_element->virtual_address, get_frame_info(curenv->env_page_directory, prev_element->virtual_address, &ptr_page_table));
			}
			pt_set_page_permissions(curenv->env_page_directory, prev_element->virtual_address, 0, PERM_PRESENT);
			ptr_frame_info = get_frame_info(curenv->env_page_directory, prev_element->virtual_address, &ptr_page_table);
			if(ptr_frame_info != NULL)
			{
				ptr_frame_info->references = 0;
				free_frame(ptr_frame_info);
			}
			struct WorkingSetElement *list_last_element = LIST_LAST(&curenv->page_WS_list);
			if((curenv->page_last_WS_element) == list_last_element)
			{
				struct WorkingSetElement *list_first_element = LIST_FIRST(&curenv->page_WS_list);
				curenv->page_last_WS_element = list_first_element;
				LIST_REMOVE(&(curenv->page_WS_list), prev_element);
				LIST_INSERT_TAIL(&curenv->page_WS_list, element);
			}
			else
			{
				struct WorkingSetElement *list_next = LIST_NEXT(curenv->page_last_WS_element);
				curenv->page_last_WS_element = list_next;
				LIST_REMOVE(&(curenv->page_WS_list), prev_element);
				LIST_INSERT_BEFORE(&curenv->page_WS_list, (curenv->page_last_WS_element), element);
			}
		}
	}
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		if((curenv->ActiveList.size + curenv->SecondList.size) < (curenv->ActiveListSize + curenv->SecondListSize))
		{
			if(curenv->ActiveList.size < curenv->ActiveListSize)
			{
				struct FrameInfo *ptr_frame_info = NULL;
				allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE);
				int ret = pf_read_env_page(curenv, (void*)fault_va);
				if (ret == E_PAGE_NOT_EXIST_IN_PF)
				{
					if(fault_va < USER_HEAP_START)
					{
						sched_kill_env(curenv->env_id);
					}
					else if((fault_va >= USER_HEAP_MAX) && (fault_va < USTACKBOTTOM))
					{
						sched_kill_env(curenv->env_id);
					}
					else if(fault_va >= USTACKTOP)
					{
						sched_kill_env(curenv->env_id);
					}
				}
				struct WorkingSetElement *element = env_page_ws_list_create_element(curenv, fault_va);
				pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(fault_va, PAGE_SIZE), PERM_PRESENT, 0);
				LIST_INSERT_HEAD(&curenv->ActiveList, element);
			}
			else
			{
				struct WorkingSetElement *last_element = LIST_LAST(&curenv->ActiveList);
				LIST_REMOVE(&curenv->ActiveList, last_element);
				pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(last_element->virtual_address, PAGE_SIZE), 0, PERM_PRESENT);
				LIST_INSERT_HEAD(&curenv->SecondList, last_element);
				struct WorkingSetElement *ptr_WS_element = NULL;
				LIST_FOREACH(ptr_WS_element, &(curenv->SecondList))
				{
					if(ROUNDDOWN(ptr_WS_element->virtual_address,PAGE_SIZE) == ROUNDDOWN(fault_va,PAGE_SIZE))
					{
						LIST_REMOVE(&curenv->SecondList, ptr_WS_element);
						pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE), PERM_PRESENT, 0);
						LIST_INSERT_HEAD(&curenv->ActiveList, ptr_WS_element);
						return;
					}
				}
				struct FrameInfo *ptr_frame_info = NULL;
				allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE);
				int ret = pf_read_env_page(curenv, (void*)fault_va);
				if (ret == E_PAGE_NOT_EXIST_IN_PF)
				{
					if(fault_va < USER_HEAP_START)
					{
						sched_kill_env(curenv->env_id);
					}
					else if((fault_va >= USER_HEAP_MAX) && (fault_va < USTACKBOTTOM))
					{
						sched_kill_env(curenv->env_id);
					}
					else if(fault_va >= USTACKTOP)
					{
						sched_kill_env(curenv->env_id);
					}
				}
				struct WorkingSetElement *element = env_page_ws_list_create_element(curenv, fault_va);
				pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(fault_va, PAGE_SIZE), PERM_PRESENT, 0);
				LIST_INSERT_HEAD(&curenv->ActiveList, element);
			}
		}
		else
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			struct WorkingSetElement *ptr_WS_element = NULL;
			LIST_FOREACH(ptr_WS_element, &(curenv->SecondList))
			{
				if(ROUNDDOWN(ptr_WS_element->virtual_address,PAGE_SIZE) == ROUNDDOWN(fault_va,PAGE_SIZE))
				{
					LIST_REMOVE(&curenv->SecondList, ptr_WS_element);
					struct WorkingSetElement *last_element = LIST_LAST(&curenv->ActiveList);
					LIST_REMOVE(&curenv->ActiveList, last_element);
					pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(last_element->virtual_address, PAGE_SIZE), 0, PERM_PRESENT);
					LIST_INSERT_HEAD(&curenv->SecondList, last_element);
					pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(ptr_WS_element->virtual_address, PAGE_SIZE), PERM_PRESENT, 0);
					LIST_INSERT_HEAD(&curenv->ActiveList, ptr_WS_element);
					return;
				}
			}
			struct FrameInfo *ptr_frame_info = NULL;
			allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, fault_va, PERM_USER | PERM_WRITEABLE);
			int ret = pf_read_env_page(curenv, (void*)fault_va);
			if (ret == E_PAGE_NOT_EXIST_IN_PF)
			{
				if(fault_va < USER_HEAP_START)
				{
					sched_kill_env(curenv->env_id);
				}
				else if((fault_va >= USER_HEAP_MAX) && (fault_va < USTACKBOTTOM))
				{
					sched_kill_env(curenv->env_id);
				}
				else if(fault_va >= USTACKTOP)
				{
					sched_kill_env(curenv->env_id);
				}
			}
			struct WorkingSetElement *element = env_page_ws_list_create_element(curenv, fault_va);
			struct WorkingSetElement *last_second_element = LIST_LAST(&curenv->SecondList);
			uint32 *ptr_page_table = NULL;
			uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, last_second_element->virtual_address);
			if(page_permissions & PERM_MODIFIED)
			{
				pf_update_env_page(curenv, last_second_element->virtual_address, get_frame_info(curenv->env_page_directory, last_second_element->virtual_address, &ptr_page_table));
			}
			pt_set_page_permissions(curenv->env_page_directory, last_second_element->virtual_address, 0, PERM_PRESENT);
			ptr_frame_info = get_frame_info(curenv->env_page_directory, last_second_element->virtual_address, &ptr_page_table);
			if(ptr_frame_info != NULL)
			{
				ptr_frame_info->references = 0;
				free_frame(ptr_frame_info);
			}
			LIST_REMOVE(&curenv->SecondList, last_second_element);
			struct WorkingSetElement *last_element = LIST_LAST(&curenv->ActiveList);
			LIST_REMOVE(&curenv->ActiveList, last_element);
			pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(last_element->virtual_address, PAGE_SIZE), 0, PERM_PRESENT);
			LIST_INSERT_HEAD(&curenv->SecondList, last_element);
			pt_set_page_permissions(curenv->env_page_directory, ROUNDDOWN(fault_va, PAGE_SIZE), PERM_PRESENT, 0);
			LIST_INSERT_HEAD(&curenv->ActiveList, element);
			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



