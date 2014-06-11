/*
    Copyright 2011-2014 Toshiba Corporation.
    All Rights Reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/**
 *  @file     cmn_mem.c
 *
 *  @brief    This file defines the functions which handle the memory
 *            pool and memory block.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/module.h>  // EXPORT_SYMBOL
#include <linux/slab.h>    // kmalloc/kfree

#include <linux/spinlock.h> // spinlock API
#include <linux/list.h>     // list API
#include <linux/sched.h>    // waitqueue API
#include <linux/wait.h>     // waitqueue API
#include <linux/time.h>     // waitqueue API

#include <linux/version.h> // MUTEX API.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief memory pool block.
 */
typedef struct tagS_MEMBLK_MGR{
    struct list_head  list;
    void             *pMemBlk;
} S_MEMBLK_MGR;


/**
 * @brief memory pool manager
 */
typedef struct tagS_CMN_MPF{
    u8                 id;
    uint               size;
    u16                maxcnt;

    spinlock_t         lock;
    struct list_head   readyQ;
    struct list_head   usedQ;
    wait_queue_head_t  delWait;
    wait_queue_head_t  getWait;
    u8                 deleting;

    S_MEMBLK_MGR      *pMgr;
} S_CMN_MPF;


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static DECLARE_MUTEX(mutex);
#else
static DEFINE_SEMAPHORE(mutex); 
#endif
static S_CMN_MPF g_cmnMemPool[CMN_MEM_POOL_MAX_NUM] = {};


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/
inline static T_CMN_ERR
CMN_waitMemBlkReady(S_CMN_MPF *pCmnMpf, u16 timeOut) 
{

    int retval;

    if(timeOut == CMN_TIME_FEVR) {
        retval = 
            wait_event_interruptible(pCmnMpf->getWait, 
                                     (!list_empty(&pCmnMpf->readyQ)) || 
                                     (pCmnMpf->deleting));
        if(retval != 0) {
            // interrupted.
            return ERR_RLWAIT;
        }
    } else {
        retval = 
            wait_event_interruptible_timeout(pCmnMpf->getWait, 
                                             (!list_empty(&pCmnMpf->readyQ)) || 
                                             (pCmnMpf->deleting),
                                             msecs_to_jiffies(timeOut));
        if(retval < 0) {
            // interrupted.
            return ERR_RLWAIT;
        }
        if(retval == 0) {
            // timeout.
            return ERR_TIMEOUT;
        }
    }

    if(pCmnMpf->deleting) {
        // MPF is deleted.
        return ERR_DELETED;
    }

    return SUCCESS;

}

/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
static T_CMN_ERR CMN_initMpf(S_CMN_MPF *);
static void      CMN_cleanMpf(S_CMN_MPF *);
extern void      CMN_initFixedMemPool(void);


/*-------------------------------------------------------------------
 * Function   : CMN_initMpf
 *-----------------------------------------------------------------*/
/**
 * initialize a fixed memory pool.
 * @param     pCmnMpf : the pointer to the fixed memory pool.
 * @return    SUCCESS     (normally completion)
 * @return    ERR_NOMEM   (the memory or resource is depleted)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR 
CMN_initMpf(S_CMN_MPF *pCmnMpf)
{

    int           i;
    S_MEMBLK_MGR *pMgr;

    spin_lock_init(&pCmnMpf->lock);
    INIT_LIST_HEAD(&pCmnMpf->usedQ);
    INIT_LIST_HEAD(&pCmnMpf->readyQ);
    init_waitqueue_head(&pCmnMpf->delWait);
    init_waitqueue_head(&pCmnMpf->getWait);
    pCmnMpf->deleting = 0;

    pCmnMpf->pMgr   = 
        (S_MEMBLK_MGR *)kmalloc(sizeof(S_MEMBLK_MGR) * pCmnMpf->maxcnt, GFP_KERNEL);
    if(pCmnMpf->pMgr == NULL) {
        return ERR_NOMEM;
    }

    for(i=0; i<pCmnMpf->maxcnt; i++) {
        pMgr = &(pCmnMpf->pMgr[i]);
        INIT_LIST_HEAD(&pMgr->list);
        pMgr->pMemBlk = (void *)kmalloc(pCmnMpf->size, GFP_KERNEL);
        if(pMgr->pMemBlk == NULL) {
            goto ERR;
        }
        list_add(&pMgr->list, &pCmnMpf->readyQ);
    }
    
    return SUCCESS;

ERR:
    for(i=i-1;i>=0; i--){
        pMgr = &(pCmnMpf->pMgr[i]);
        list_del_init(&pMgr->list);
        kfree(pMgr->pMemBlk);
    }
    kfree(pCmnMpf->pMgr);

    return ERR_NOMEM;
}


/*-------------------------------------------------------------------
 * Function   : CMN_cleanMpf
 *-----------------------------------------------------------------*/
/**
 * cleanup a fixed memory pool.
 * @param     pCmnMpf : the pointer to the fixed memory pool.
 * @return    nothing
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static void
CMN_cleanMpf(S_CMN_MPF *pCmnMpf)
{

    S_MEMBLK_MGR      *pMgr;
    int               i;

    for(i=0; i<pCmnMpf->maxcnt; i++) {
        pMgr = &(pCmnMpf->pMgr[i]);
        list_del_init(&pMgr->list);
        kfree(pMgr->pMemBlk);
    }

    kfree(pCmnMpf->pMgr);

}


/*-------------------------------------------------------------------
 * Function   : CMN_initFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function initialize fixed memory pool manager.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_initFixedMemPool()
{

    int i;

    for(i=0; i<CMN_MEM_POOL_MAX_NUM; i++) {
        g_cmnMemPool[i].id = 0; // 0 means unused.
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function creates a fixed memory pool with the specified ID.
 * @param     memPoolID    : ID of the memory pool
 * @param     memAttr      : the attribute of the memory pool
 * @param     memBlkCount  : the number of memory block to allocate
 * @param     memBlkSize   : the size of memory block to allocate
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_NOMEM   (the momory or resource is depleted)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createFixedMemPool(u8   memPoolID,
                       u32  memAttr,
                       u16  memBlkCount,
                       uint memBlkSize)
{

    S_CMN_MPF *pCmnMpf;

    // check parameter
    if (memPoolID == 0 || memPoolID > CMN_MEM_POOL_MAX_NUM) {
        return ERR_INVID;
    }

    if (memBlkCount == 0 || memBlkSize == 0) {
        return ERR_BADPARM;
    }

    pCmnMpf = &(g_cmnMemPool[memPoolID-1]);
    down(&mutex);
    if(pCmnMpf->id != 0) {
        // arleady used.
        up(&mutex);
        return ERR_INVSTAT;
    }

    pCmnMpf->id     = memPoolID;
    pCmnMpf->maxcnt = memBlkCount;
    pCmnMpf->size   = memBlkSize;
    CMN_initMpf(pCmnMpf);
    up(&mutex);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function   : CMN_acreateFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function creates a fixed memory pool with the specified ID.
 * @param     memAttr      : the attribute of the memory pool
 * @param     memBlkCount  : the number of memory block to allocate
 * @param     memBlkSize   : the size of memory block to allocate
 * @return    assigned ID (normally completion)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_NOMEM   (the momory or resource is depleted)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_acreateFixedMemPool(u32  memAttr,
                        u16  memBlkCount,
                        uint memBlkSize)
{

    S_CMN_MPF *pCmnMpf;
    u8         memPoolID;

    down(&mutex);
    for(memPoolID=1; memPoolID<=CMN_MEM_POOL_MAX_NUM; memPoolID++) {
        pCmnMpf = &(g_cmnMemPool[memPoolID-1]);
        if(pCmnMpf->id == 0) {
            // found
            pCmnMpf->id = memPoolID;
            pCmnMpf->maxcnt = memBlkCount;
            pCmnMpf->size   = memBlkSize;
            CMN_initMpf(pCmnMpf);
            up(&mutex);
            return memPoolID;
        }
    }
    up(&mutex);
    return ERR_NOID;

}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function deletes a memory pool with the specified ID.
 * @param     memPoolID  : ID of memory pool
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteFixedMemPool(u8 memPoolID)
{

    S_CMN_MPF *pCmnMpf;

    // check parameter
    if (memPoolID == 0 || memPoolID > CMN_MEM_POOL_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnMpf = &(g_cmnMemPool[memPoolID-1]);
    down(&mutex);
    if(pCmnMpf->id == 0) {
        up(&mutex);
        return ERR_NOOBJ;
    }
    CMN_cleanMpf(pCmnMpf);
    pCmnMpf->id = 0;
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_getFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function gets a memory block from the specified fixed memory pool.
 * @param     memPoolID  : ID of message box
 * @param     pMemBlk    : the pointer to the pointer to memory block
 * @param     timeOut    : the value of timeout (ms) // not supported.
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @return    ERR_DELETED (the object is deleted)
 * @return    ERR_RLWAIT  (force release during wait)
 * @return    ERR_TIMEOUT (force release during wait)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_getFixedMemPool(u8   memPoolID,
                    void **pMemBlk,
                    u16  timeOut)
{

    S_CMN_MPF    *pCmnMpf;
    S_MEMBLK_MGR *pMgr = NULL;

    T_CMN_ERR     retval;
    unsigned long flag;

    // check parameter
    if (memPoolID == 0 || memPoolID > CMN_MEM_POOL_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnMpf = &(g_cmnMemPool[memPoolID-1]);

    if(pCmnMpf->id == 0) {
        return ERR_NOOBJ;
    }

    do {

        retval = CMN_waitMemBlkReady(pCmnMpf, timeOut);
        if(retval != SUCCESS) {
            return retval;
        }

        spin_lock_irqsave(&pCmnMpf->lock, flag);
        if(!list_empty(&pCmnMpf->readyQ)) {
            // chain to used queue.
            pMgr = list_entry(pCmnMpf->readyQ.next, S_MEMBLK_MGR, list);
            list_del_init(&pMgr->list);
            list_add(&pMgr->list, &pCmnMpf->usedQ);
        }
        spin_unlock_irqrestore(&pCmnMpf->lock, flag);

        if(pMgr) {
            // found available memory block
            break;
        }
    } while(1);
    
    *pMemBlk = pMgr->pMemBlk;

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function   : CMN_releaseFixedMemPool
 *-----------------------------------------------------------------*/
/**
 * This function releases a memory block into the specified fixed memory pool.
 * @param     memPoolID  : ID of memory pool
 * @param     pMemBlk    : the pointer to memory block
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_releaseFixedMemPool(u8   memPoolID,
                        void *pMemBlk)
{

    S_CMN_MPF    *pCmnMpf;
    S_MEMBLK_MGR *pMgr, *pNext;

    unsigned long flag;

    // check parameter
    if (memPoolID == 0 || memPoolID > CMN_MEM_POOL_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnMpf = &(g_cmnMemPool[memPoolID-1]);

    if (pCmnMpf->id == 0) {
        return ERR_NOOBJ;
    }

    spin_lock_irqsave(&pCmnMpf->lock, flag);
    list_for_each_entry_safe(pMgr, pNext, &pCmnMpf->usedQ, list) {
        if(pMgr->pMemBlk == pMemBlk) {
            // chain to available queue.
            list_del_init(&pMgr->list);
            list_add(&pMgr->list, &pCmnMpf->readyQ);
            break;
        }
    }
    spin_unlock_irqrestore(&pCmnMpf->lock, flag);

    wake_up_interruptible(&pCmnMpf->getWait);
    wake_up_interruptible(&pCmnMpf->delWait);

    return SUCCESS;
}

/*-------------------------------------------------------------------
 * Function   : CMN_allocMem
 *-----------------------------------------------------------------*/
/**
 * This function allocate memory and set memory address
 * @param     memAddr    : set allocate memory address pointer
 * @param     memSize    : allocate memory size (byte) 
 * @return    SUCCESS     (normally completion)
 * @return    ERR_NOMEM   (the momory or resource is depleted)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_allocMem(void **memAddr, uint memSize)
{

    *memAddr = kmalloc(memSize, GFP_KERNEL);

    if (*memAddr == NULL) {
        return ERR_NOMEM;
    }

    return SUCCESS; 
}


/*-------------------------------------------------------------------
 * Function   : CMN_releaseMem
 *-----------------------------------------------------------------*/
/**
 * This function release memory
 * @param     memAddr    : release memory address 
 * @return    nothing 
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_releaseMem(void *memAddr)
{

    if (memAddr != NULL) {
        kfree(memAddr);
    }
  
    return;
}

