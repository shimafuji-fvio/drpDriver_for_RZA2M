/* ******************************************************************************** *
   Name   : DRP driver for RZ/A2M
   Date   : 2020.05.08
   Auther : Shimafuji Electric inc.

   Copyright (C) 2020 Shimafuji Electric inc. All rights reserved.
 * ******************************************************************************** */

#include "FreeRTOS.h"
#include "task.h"

#include "r_typedefs.h"
#include "iodefine.h"
#include "r_mmu_lld_rza2m.h"

#include "drp_iodefine.h"
#include "r_dk2_core.h"
#include "r_dk2_if.h"

#include "sf_drp.h"

static uint32_t sf_desc[4*4*SF_TASK_NUM] __attribute__ ((aligned(16), section("UNCACHED_BSS")));

typedef struct
{
  uint32_t         *desc;
  int_cb_t          cb;
  struct sf_task_t *next;
  uint8_t           tile_pos;
  uint8_t           busy;
  uint8_t           dummy[2];
} sf_task_t;

static void sf_drp_semInit(void);
static void sf_drp_semGive_isr(uint8_t tile_pos);
static void sf_drp_semTake(uint8_t tile_pos);
static void sf_dk2_isr(uint8_t id);
static void sf_dk2_start(unsigned char id, unsigned long desc);
static int32_t sf_task_get(sf_task_t **pTask);
static uint32_t sf_get_dscc_pamoni(uint8_t tile_pos);
static uint32_t sf_get_dscc_dctli(uint8_t tile_pos);
static int32_t sf_task_dequeue_isr(uint8_t id);

static sf_task_t gTask[SF_TASK_NUM];
static sf_task_t *gTaskQueueHead[R_DK2_TILE_NUM];
static sf_task_t *gTaskQueueTail[R_DK2_TILE_NUM];
static uint32_t sf_drp_sem[R_DK2_TILE_NUM];


static void sf_drp_semInit(void)
{
  uint32_t i;
  
  for(i=0;i<R_DK2_TILE_NUM;i++){
    sf_drp_sem[i]=2;
  }
}

static void sf_drp_semGive_isr(uint8_t tile_pos)
{
  sf_drp_sem[tile_pos]++;
}

static void sf_drp_semTake(uint8_t tile_pos)
{
  while(1){
    if(sf_drp_sem[tile_pos]>0){
      break;
    }
  }

  taskENTER_CRITICAL();
  sf_drp_sem[tile_pos]--;
  taskEXIT_CRITICAL();
  
}

static void sf_dk2_isr(uint8_t id)
{
  uint8_t tile_pos;
  uint32_t num,i;

  tile_pos = (uint8_t)R_DK2_GetTilePos(id);
  
  /* clear int reg */
  DRP_DSCC_INT      = (1uL << tile_pos);
  DRP_ODIF_INT      = (1uL << tile_pos);
            
  if      (tile_pos==0){ num=DRP_ODIF_INTCNTO0;
  }else if(tile_pos==1){ num=DRP_ODIF_INTCNTO1;
  }else if(tile_pos==2){ num=DRP_ODIF_INTCNTO2;
  }else if(tile_pos==3){ num=DRP_ODIF_INTCNTO3;
  }else if(tile_pos==4){ num=DRP_ODIF_INTCNTO4;
  }else                { num=DRP_ODIF_INTCNTO5;
  }
    
  for(i=0;i<num;i++){
    sf_task_dequeue_isr(id);
  }

  return;
}

static void sf_dk2_start(unsigned char id, unsigned long desc)
{
  if ((id & 0x01) != 0){
    DRP_IDIF_DMACNTI0 = 0x00040001;
    DRP_DSCC_DCTLI0   = 0x00000000; // 0xEAFF8100
    DRP_DSCC_DPAI0    = desc;       // 0xEAFF8108
    DRP_DSCC_DCTLI0   = 0x00000001; // 0xEAFF8100
  }else if ((id & 0x02) != 0){
    DRP_IDIF_DMACNTI1 = 0x00040001;
    DRP_DSCC_DCTLI1   = 0x00000000; // 0xEAFF8140
    DRP_DSCC_DPAI1    = desc;       // 0xEAFF8148
    DRP_DSCC_DCTLI1   = 0x00000001; // 0xEAFF8140
  }else if ((id & 0x04) != 0){
    DRP_IDIF_DMACNTI2 = 0x00040001;
    DRP_DSCC_DCTLI2   = 0x00000000; // 0xEAFF8180
    DRP_DSCC_DPAI2    = desc;       // 0xEAFF8188
    DRP_DSCC_DCTLI2   = 0x00000001; // 0xEAFF8180
  }else if ((id & 0x08) != 0){
    DRP_IDIF_DMACNTI3 = 0x00040001;
    DRP_DSCC_DCTLI3   = 0x00000000; // 0xEAFF81C0
    DRP_DSCC_DPAI3    = desc;       // 0xEAFF81C8
    DRP_DSCC_DCTLI3   = 0x00000001; // 0xEAFF81C0
  }else if ((id & 0x10) != 0){
    DRP_IDIF_DMACNTI4 = 0x00040001;
    DRP_DSCC_DCTLI4   = 0x00000000; // 0xEAFF8200
    DRP_DSCC_DPAI4    = desc;       // 0xEAFF8208
    DRP_DSCC_DCTLI4   = 0x00000001; // 0xEAFF8200
  }else if ((id & 0x20) != 0){
    DRP_IDIF_DMACNTI5 = 0x00040001;
    DRP_DSCC_DCTLI5   = 0x00000000; // 0xEAFF8240
    DRP_DSCC_DPAI5    = desc;       // 0xEAFF8248
    DRP_DSCC_DCTLI5   = 0x00000001; // 0xEAFF8240
  }
}

static int32_t sf_task_get(sf_task_t **pTask)
{
  uint32_t i;

  i=0;
  while(1){
    if(gTask[i].busy==0){
      gTask[i].busy=1;
      break;
    }
    if(i>=SF_TASK_NUM){
      i=0;
    }else{
      i++;
    }
  }
  *pTask=&gTask[i];
  
  return(0);
}

static uint32_t sf_get_dscc_pamoni(uint8_t tile_pos)
{
  uint32_t addr=0;

  if      (tile_pos==0){ addr = DRP_DSCC_PAMONI0;
  }else if(tile_pos==1){ addr = DRP_DSCC_PAMONI1;
  }else if(tile_pos==2){ addr = DRP_DSCC_PAMONI2;
  }else if(tile_pos==3){ addr = DRP_DSCC_PAMONI3;
  }else if(tile_pos==4){ addr = DRP_DSCC_PAMONI4;
  }else if(tile_pos==5){ addr = DRP_DSCC_PAMONI5;
  }

  return(addr);
}

static uint32_t sf_get_dscc_dctli(uint8_t tile_pos)
{
  uint32_t addr=0;

  if      (tile_pos==0){ addr = DRP_DSCC_DCTLI0;
  }else if(tile_pos==1){ addr = DRP_DSCC_DCTLI1;
  }else if(tile_pos==2){ addr = DRP_DSCC_DCTLI2;
  }else if(tile_pos==3){ addr = DRP_DSCC_DCTLI3;
  }else if(tile_pos==4){ addr = DRP_DSCC_DCTLI4;
  }else if(tile_pos==5){ addr = DRP_DSCC_DCTLI5;
  }

  return(addr);
}

static int32_t sf_task_dequeue_isr(uint8_t id)
{
  uint8_t tile_pos;
  sf_task_t **pTaskQueTail;
  sf_task_t **pTaskQueHead;
  sf_task_t *pTask;

  tile_pos = (uint8_t)R_DK2_GetTilePos(id);
  
  pTaskQueHead = &gTaskQueueHead[tile_pos];
  pTaskQueTail = &gTaskQueueTail[tile_pos];
  
  if((*pTaskQueHead)!=NULL){
    pTask = (*pTaskQueHead);
    pTask->busy=0;
    if(pTask->cb != NULL){
      (pTask->cb)(id);
    }
    pTask->cb=NULL;
    (*pTaskQueHead) = (sf_task_t *)pTask->next;
    if((*pTaskQueHead) == NULL){
      (*pTaskQueTail) = NULL;
    }
    pTask->next = NULL;
  }

  sf_drp_semGive_isr(tile_pos);
  
  return(0);
}

/*
  ================================================================================
 */

int32_t sf_DK2_Initialize(void)
{
  uint32_t i;
  uint32_t paddr;
  uint32_t *work_uc;

  sf_drp_semInit();
  
  for(i=0;i<R_DK2_TILE_NUM;i++){
    gTaskQueueHead[i]=NULL;
    gTaskQueueTail[i]=NULL;
  }

  work_uc = (uint32_t *)&sf_desc[0];
  R_MMU_VAtoPA((uint32_t)work_uc, &paddr);
  
  for(i=0;i<SF_TASK_NUM;i++){

    work_uc[4*4*i+ 0]=0x01000007;
    work_uc[4*4*i+ 1]=0x00000000;
    work_uc[4*4*i+ 2]=0x00000000;
    work_uc[4*4*i+ 3]=0x00000000;
    
    work_uc[4*4*i+ 4]=0x01008000;
    work_uc[4*4*i+ 5]=0xF0FF9100;
    work_uc[4*4*i+ 6]=0x00040101;
    work_uc[4*4*i+ 7]=0x00000000;
    
    work_uc[4*4*i+ 8]=0x09000000;
    work_uc[4*4*i+ 9]=paddr+4*4*i+12*4;
    work_uc[4*4*i+10]=0x00000000;
    work_uc[4*4*i+11]=0x00000000;

    work_uc[4*4*i+12]=0x03008000;
    work_uc[4*4*i+13]=0xF0FFD000;
    work_uc[4*4*i+14]=0x00000000;
    work_uc[4*4*i+15]=0x00000000;
    
    gTask[i].desc     = &work_uc[4*4*i];
    gTask[i].cb       = NULL;
    gTask[i].next     = NULL;
    gTask[i].tile_pos = 0;
    gTask[i].busy     = 0;
  }

  DRP_IDIF_DMACNTI0 = 0x00040001;
  DRP_IDIF_DMACNTI1 = 0x00040001;
  DRP_IDIF_DMACNTI2 = 0x00040001;
  DRP_IDIF_DMACNTI3 = 0x00040001;
  DRP_IDIF_DMACNTI4 = 0x00040001;
  DRP_IDIF_DMACNTI5 = 0x00040001;
  
  return(0);
}

int32_t sf_DK2_Start2(const uint8_t id, const void *const pparam, const uint32_t size, const int_cb_t pint)
{
  uint8_t tile_pos;
  sf_task_t *pTask;
  sf_task_t *pTaskOld;
  uint32_t paddr;
  uint32_t paddrEnd;
  uint32_t paddrOld;
  int shift;
  sf_task_t **pTaskQueTail;
  sf_task_t **pTaskQueHead;

  R_DK2_Start_sub(id);
  tile_pos = (uint8_t)R_DK2_GetTilePos(id);
  shift = 0;
  while (0 == ((id >> shift) & 1)){
    shift++;
  }
  
  sf_task_get(&pTask);

  R_MMU_VAtoPA((uint32_t)pparam, &paddr);
  R_MMU_VAtoPA((uint32_t)&(pTask->desc[12]), &paddrEnd);
  
  pTask->tile_pos = tile_pos;
  if(pint==(int_cb_t)-1){
    pTask->cb       = NULL;
  }else{
    pTask->cb       = (int_cb_t)pint;
  }
  pTask->desc[1]  = paddr;
  pTask->desc[2]  = size;
  pTask->desc[5]  = 0xF0FF0000+(uint32_t)((0x91+shift)<<8);
  pTask->desc[9]  = paddrEnd;

  pTaskQueHead = &gTaskQueueHead[tile_pos];
  pTaskQueTail = &gTaskQueueTail[tile_pos];
  R_MMU_VAtoPA((uint32_t)pTask->desc, &paddr);

  sf_drp_semTake(tile_pos);
  
  taskENTER_CRITICAL();
 
  if((*pTaskQueTail)==NULL){
    (*pTaskQueHead) = pTask;
    (*pTaskQueTail) = pTask;
    sf_dk2_start(id,paddr);
  }else{
    pTaskOld = (*pTaskQueTail);
    R_MMU_VAtoPA((uint32_t)pTaskOld->desc, &paddrOld);
    (*pTaskQueTail)->next = (struct sf_task_t *)pTask;
    (*pTaskQueTail)       = pTask;
    pTaskOld->desc[9] = paddr;
    
    while(1){
      if((sf_get_dscc_dctli(tile_pos)&0x2)==0){ break; }
    }
    if(sf_get_dscc_pamoni(tile_pos)==(paddrOld+12*4)){
      sf_dk2_start(id,paddr);
    }
  }
  
  taskEXIT_CRITICAL();

  if(pint==(int_cb_t)-1){
    while(sf_drp_sem[tile_pos]<2){}
  }
  
  return(0);
}

int32_t sf_DK2_Load(const void *const pconfig,
                    const uint8_t top_tiles,
                    const uint32_t tile_pattern,
                    const load_cb_t pload,
                    const int_cb_t pint,
                    uint8_t *const paid)
{
  (void)pload;
  (void)pint;
  return(R_DK2_Load(pconfig, top_tiles, tile_pattern, NULL, sf_dk2_isr, paid));
}

