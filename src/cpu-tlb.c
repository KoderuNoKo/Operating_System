/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */

#include "mm.h"
#include "os-mm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * update the pte of all TLB entries of a process
 * @proc: caller process
 * @mp: tlb memphy struct
 */
int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */
#if ASSOCIATED_MAPPING


#else

  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) 
  {
    struct vm_rg_struct* curr_rg = get_symrg_byid(proc->mm, i);

    if (curr_rg->rg_start == curr_rg->rg_end) { // region id unused - skip
      continue;
    }

    unsigned long rg_start = curr_rg->rg_start;
    unsigned long rg_end = curr_rg->rg_end;

    int pgn = PAGING_PGN(rg_start);
    while(pgn * PAGING_PAGESZ < rg_end) {
      tlb_cache_update(mp, proc->pid, pgn, proc->mm->pgd[pgn]);
      pgn++;
    }
    tlb_cache_update(mp, proc->pid, pgn, proc->mm->pgd[pgn]);
  }

#endif

  return 0;
}

/* 
 * clear all TLB entries of a process
 * @proc: caller process
 * @mp: tlb memphy struct
 */
int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/
#if ASSOCIATED_MAPPING



#else

  for (int i = 0; i < PAGING_MAX_SYMTBL_SZ; i++) 
  {
    struct vm_rg_struct* curr_rg = get_symrg_byid(proc->mm, i);

    if (curr_rg->rg_start == curr_rg->rg_end) { // region id unused - skip
      continue;
    }

    unsigned long rg_start = curr_rg->rg_start;
    unsigned long rg_end = curr_rg->rg_end;

    int pgn = PAGING_PGN(rg_start);
    while(pgn * PAGING_PAGESZ < rg_end) {
      tlb_cache_clear(proc->tlb, proc->pid, pgn);
      pgn++;
    }
    tlb_cache_clear(proc->tlb, proc->pid, pgn);
  }

#endif

  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);
  if(proc == NULL) return -1;
  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  
  if (val == 0 ) 
  {
    int pgn = PAGING_PGN(addr);
    while (pgn * PAGING_PAGESZ < addr + size) {
      tlb_cache_write(proc->tlb, proc->pid, pgn, proc->mm->pgd[pgn]);
      pgn++;
    }
    tlb_cache_write(proc->tlb, proc->pid, pgn, proc->mm->pgd[pgn]);
  }

  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  // free entries in TLB
  if(proc == NULL) return -1;
  struct vm_rg_struct *curr_rg = get_symrg_byid(proc->mm, reg_index);
  unsigned long rg_start = curr_rg->rg_start;
  unsigned long rg_end = curr_rg->rg_end;

  int pgn = PAGING_PGN(rg_start);
  while (pgn * PAGING_PAGESZ < rg_end) {
    tlb_cache_clear(proc->tlb, proc->pid, pgn);
    pgn++;
  }
  tlb_cache_clear(proc->tlb, proc->pid, pgn);

  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  if(proc == NULL) return -1;
  BYTE data = -1;
  int frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  // search in TLB for page
  int val = tlb_cache_read(proc->tlb, proc->pid, source, &frmnum);

  if (frmnum >= 0) 
  {
    // TLB hit & frame is available in MEMRAM
    MEMPHY_read(proc->mram, frmnum*PAGING_PAGESZ + offset, &data);
    destination = (uint32_t) data;
    return 0;
  }

  if (val < 0) 
  {
    // TLB hit but the page is not available in MEMRAM
    // attempt to bring it to MEMRAM

    pg_getpage(proc->mm, source, &frmnum, proc);

    if (frmnum < 0) { // attempt failed
      return -1;
    }

    tlb_cache_update(proc->tlb, proc->pid, source, proc->mm->pgd[source]);
  }
  else 
  {
    // handle TLB miss

    // obtain the frame through page table
    uint32_t pte = proc->mm->pgd[source];

    // update TLB
    tlb_cache_write(proc->tlb, proc->pid, source, pte);
  }

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  // perform the read
  val = __read(proc, 0, source, offset, &data);
  destination = (uint32_t) data;

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return val;
}

/*  
 *  tlbwrite - CPU TLB-based write a region memory
 *  @proc: Process executing the instruction
 *  @data: data to be wrttien into memory
 *  @destination: index of destination register
 *  @offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  if(proc == NULL) return -1;
  int val;
  BYTE frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

  //BYTE data, frmnum;

  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  // search in TLB for page
  val = tlb_cache_read(proc->tlb, proc->pid, destination, &frmnum);

  if (frmnum >= 0) 
  {
    /* TLB hit & frame is available in MEMRAM */
    MEMPHY_write(proc->mram, frmnum*PAGING_PAGESZ + offset, data);
    return 0;
  }

  if (val < 0) 
  {
    /* TLB hit but the page is not available in MEMRAM */

    // attempt to bring it to MEMRAM
    pg_getpage(proc->mm, destination, &frmnum, proc);

    if (frmnum < 0) { // attempt failed
      return -1;
    }

    tlb_cache_update(proc->tlb, proc->pid, destination, proc->mm->pgd[destination]);
  }
  else 
  {
    /* TLB miss */

    // obtain the frame through page table of caller process
    uint32_t pte = proc->mm->pgd[destination];

    // update TLB
    tlb_cache_write(proc->tlb, proc->pid, destination, pte);
  }

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return val;
}

//#endif