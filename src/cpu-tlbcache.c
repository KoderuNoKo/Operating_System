/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include "bitops.h"
#include <stdlib.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))


/*
 *  tlb_cache_read read TLB cache device for process page number
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @frnum: obtained frame number if hit
 */
int tlb_cache_read(struct memphy_struct * mp, uint32_t pid, int pgnum, int *frnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   *frnum = -1;
   int val = 0;

#if ASSOCIATED_MAPPING

   int set_idx = pid % DIV_ROUND_DOWN(mp->maxsz, N_WAY);

   // translate set index to TLB entry
   int entr = set_idx * N_WAY;

   // traverse the set for designated pgnum of pid
   for (; entr < (set_idx + 1)*N_WAY && entr*TLB_ENTRY_SZ < mp->maxsz; entr++)
   {
      // retrieve the TLB entry
      BYTE byte = 0;
      uint64_t tlb_entr = 0;

      for (int i = TLB_ENTRY_SZ - 1; i >= 0; i--) {
         TLBMEMPHY_READ(mp, entr*TLB_ENTRY_SZ + i, byte);
         tlb_entr |= ((unsigned char)byte << i*sizeof(BYTE));
      }

      // extract the pid and corresponding page number
      uint32_t epid = EXTRACT_NBITS64(tlb_entr, TLB_PID_HBIT, TLB_PID_LBIT);
      int epgnum = EXTRACT_NBITS64(tlb_entr, TLB_PTE_LBIT + PAGING_PTE_USRNUM_LOBIT, TLB_PTE_LBIT + PAGING_PTE_USRNUM_HIBIT);

      if (epid == pid && epgnum == pgnum) 
      {
         // TLB hit - attempt to extract frame number of page if it is present in RAM
         int present = EXTRACT_NBITS64(tlb_entr, 
            TLB_PTE_LBIT + PAGING_PTE_PRESENT_BIT, 
            TLB_PTE_LBIT + PAGING_PTE_PRESENT_BIT);

         if (present) {
            *frnum = EXTRACT_NBITS64(tlb_entr, TLB_PTE_LBIT + PAGING_PTE_FPN_HIBIT, TLB_PTE_LBIT + PAGING_PTE_FPN_LOBIT);
         }
         else {
            *frnum = -1;
         }

         // update LRU
         TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ, byte);
         byte = byte >> 1; // shift right
         byte |= 0x80; // insert 1
         TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ, byte);
      }
      else 
      {
         // update LRU
         TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ, byte);
         byte = byte >> 1; // shift right
         TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ, byte);
      }
   }

#else   

   // go to the entry where pid and pgnum will be mapped to
   int entr = (pid * pgnum) % DIV_ROUND_DOWN(mp->maxsz, TLB_ENTRY_SZ);

   // retrieve information from entry
   BYTE byte = 0;
   uint32_t epid, epte;
   int epgnum;
   
   // pid
   for (int i = 0; i < sizeof(uint32_t); i++) 
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epid |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   // page number
   for (int i = sizeof(uint32_t); i < 2*sizeof(uint32_t); i++)
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epgnum |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   // pte
   for (int i = sizeof(uint32_t) - 1; i >= 0; i--)
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epte |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   if (epid == pid && epgnum == pgnum) 
   {
      // TLB hit - attempt to extract frame number of page if it is present in RAM
      int present = EXTRACT_NBITS64(epte, PAGING_PTE_PRESENT_BIT, PAGING_PTE_PRESENT_BIT);

      if (present) {
         *frnum = EXTRACT_NBITS64(epte, PAGING_PTE_FPN_HIBIT, PAGING_PTE_FPN_LOBIT);
         val = 0;
      }
      else {
         // it is a hit but the frame is unavailable in MEMRAM
         *frnum = -1;
         val = -1;
      }
   }

#endif

   return val;
}

/*
 *  tlb_cache_write write new entry to TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @pte: to write pte
 */
int tlb_cache_write(struct memphy_struct *mp, uint32_t pid, int pgnum, uint32_t pte)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */

#if ASSOCIATED_MAPPING
   // TODO: implement cache writing for associated cache mapping technique
   // handle case when it's a TLB hit the page is not present
   

#else

   // find the entry index in which new entry will be written
   int entr = (pid * pgnum) % DIV_ROUND_DOWN(mp->maxsz, TLB_ENTRY_SZ);

   // write down the new pid
   for (int i = TLB_PID_LBIT/sizeof(BYTE); i < TLB_PID_HBIT/sizeof(BYTE); i++) {
      TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ + i, (BYTE)(pid >> i*sizeof(BYTE) & 0xFF));
   }

   // write down new page number
   for (int i = TLB_PGN_LBIT/sizeof(BYTE); i < TLB_PGN_HBIT/sizeof(BYTE); i++) {
      TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ + i, (BYTE)(pgnum >> i*sizeof(BYTE) & 0xFF));
   }

   // write down the new pte
   for (int i = TLB_PTE_LBIT/sizeof(BYTE); i < TLB_PTE_HBIT/sizeof(BYTE); i++) {
      TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ + i, (BYTE)(pte >> i*sizeof(BYTE) & 0xFF));
   }

#endif

   
   return 0;
}
/*
 * tlb_cache_clear: clear the cache entry (if hit)
 * @mp: tlb memphy struct
 * @pid: process id
 * @pgnum: page number
 */
int tlb_cache_clear(struct memphy_struct *mp, uint32_t pid, int pgnum)
{
#if ASSOCIATED_MAPPING


#else 

   int entr = (pid * pgnum) % DIV_ROUND_DOWN(mp->maxsz, TLB_ENTRY_SZ);

   // check if it is a hit
   // retrieve information from entry
   BYTE byte = 0;
   uint32_t epid;
   int epgnum;
   
   // pid
   for (int i = 0; i < sizeof(uint32_t); i++) 
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epid |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   // page number
   for (int i = sizeof(uint32_t); i < 2*sizeof(uint32_t); i++)
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epgnum |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   if (epid == pid && epgnum == pgnum) 
   {
      // hit - clear this entry
      for (int i = TLB_ENTRY_SZ - 1; i >= 0; i--) {
         TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ + i, 0);
      }
   }

#endif

   return 0;
}

/*
 * tlb_cache_update_pte: update the pte of pid and pgnum (if hit)
 * @mp: tlb memphy struct
 * @pid: calling process id
 * @pgnum: page nunber 
 * @pte: new pte
 */
int tlb_cache_update(struct memphy_struct *mp, uint32_t pid, int pgnum, uint32_t pte)
{
#if ASSOCIATED_MAPPING



#else 

   int entr = (pid * pgnum) % DIV_ROUND_DOWN(mp->maxsz, TLB_ENTRY_SZ);

   // check if it is a hit
   // retrieve information from entry
   BYTE byte = 0;
   uint32_t epid;
   int epgnum;
   
   // pid
   for (int i = 0; i < sizeof(uint32_t); i++) 
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epid |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   // page number
   for (int i = sizeof(uint32_t); i < 2*sizeof(uint32_t); i++)
   {
      TLBMEMPHY_read(mp, entr*TLB_ENTRY_SZ + i, &byte);
      epgnum |= ((unsigned char)byte << i*sizeof(BYTE));
   }

   if (epid == pid && epgnum == pgnum) 
   {
      // hit - update pte of this entry
      for (int i = TLB_PTE_LBIT/sizeof(BYTE); i < TLB_PTE_HBIT/sizeof(BYTE); i++) {
         TLBMEMPHY_write(mp, entr*TLB_ENTRY_SZ + i, (BYTE)(pte >> i*sizeof(BYTE) & 0xFF));
      }
   }

#endif

   return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL) {
      return -1;
   }
   else if (addr >= mp->maxsz) {
      return -1;
   }
   

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL) {
      return -1;
   }
   else if (addr >= mp->maxsz) {
      return -1;
   }

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */
int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */

   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   return 0;
}

//#endif

// Student costume functions