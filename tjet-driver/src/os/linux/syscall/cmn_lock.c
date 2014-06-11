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
 *  @file     cmn_lock.c
 *
 *  @brief    This file defines the functions which handle cpu lock
 *            object and method 
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/module.h>   // EXPORT_SYMBOL
#include <linux/spinlock.h> // spinlock API

#include <linux/version.h>   // EXPORT_SYMBOL
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
 * @breif lock cpu(spinlock) object (for linux)
 */
typedef struct tagS_CMN_LOC {
    u8            id;
    spinlock_t    lock;
    unsigned long flag;
} S_CMN_LOC;


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static DECLARE_MUTEX(mutex);
#else
static DEFINE_SEMAPHORE(mutex); 
#endif
static S_CMN_LOC g_cmnLoc[CMN_LOC_MAX_NUM] = {};


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
extern void CMN_initCpuLock(void);


/*-------------------------------------------------------------------
 * Function   : CMN_initCpuLock
 *-----------------------------------------------------------------*/
/**
 * This function initialize Lock manager.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_initCpuLock()
{

    int i;

    for(i=0; i<CMN_LOC_MAX_NUM; i++) {
        g_cmnLoc[i].id = 0; // 0 means unused.
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createCpuLock
 *-----------------------------------------------------------------*/
/**
 * This function creates a CPU lock object with the specified ID.
 * attribute and priority is not effective
 * @param     locID    : ID of the Cpu Lock
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createCpuLock(u8 locID)
{

    S_CMN_LOC      *pCmnLoc;

    // check parameter
    if (locID == 0 || locID > CMN_LOC_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnLoc = &(g_cmnLoc[locID-1]);

    down(&mutex);
    if(pCmnLoc->id != 0) {
        up(&mutex);
        return ERR_INVSTAT;
    }
    // attribute and priority is not effective.
    pCmnLoc->id = locID;
    spin_lock_init(&pCmnLoc->lock);

    up(&mutex);
    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_acreateCpuLock
 *-----------------------------------------------------------------*/
/**
 * This function creates a CPU loc object and return assigned ID.
 * @return    assigned ID (normally completion)
 * @return    ERR_NOID    (no more lock object is availbale)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_acreateCpuLock()
{

    S_CMN_LOC      *pCmnLoc;
    u8              locID;
    
    down(&mutex);
    for(locID=1; locID<=CMN_LOC_MAX_NUM; locID++) {
        pCmnLoc = &(g_cmnLoc[locID-1]);
        if(pCmnLoc->id == 0) {
            // found.
            // max value is not effective.
            pCmnLoc->id = locID;
            spin_lock_init(&pCmnLoc->lock);
            up(&mutex);
            return locID;
        }
    }
    up(&mutex);
    return ERR_NOID;
}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteCpuLock
 *-----------------------------------------------------------------*/
/**
 * This function deletes a mutex with the specified ID.
 * @param     locID  : ID of the CPU lock
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteCpuLock(u8 locID)
{

    S_CMN_LOC      *pCmnLoc;

    // check parameter
    if (locID == 0 || locID > CMN_LOC_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnLoc = &(g_cmnLoc[locID-1]);

    down(&mutex);
    if(pCmnLoc->id == 0) {
        up(&mutex);
        return ERR_NOOBJ;
    }
    pCmnLoc->id = 0;
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_lockCpu
 *-----------------------------------------------------------------*/
/**
 * This function lock a CPU lock with specified ID.
 * @param     locID    : ID of the CPU lock
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_lockCpu(u8 locID)
{

    S_CMN_LOC      *pCmnLoc;

    // check parameter
    if (locID == 0 || locID > CMN_LOC_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnLoc = &(g_cmnLoc[locID-1]);
    if(pCmnLoc->id == 0) {
        return ERR_NOOBJ;
    }

    // always use spinlock_irqsave for safe
    spin_lock_irqsave(&pCmnLoc->lock, pCmnLoc->flag);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_unlockCpu
 *-----------------------------------------------------------------*/
/**
 * This function unlock a CPU lock with specified ID.
 * @param     locID    : ID of the mutex
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_unlockCpu(u8 locID)
{

    S_CMN_LOC      *pCmnLoc;

    // check parameter
    if (locID == 0 || locID > CMN_LOC_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnLoc = &(g_cmnLoc[locID-1]);

    if(pCmnLoc->id == 0) {
        return ERR_NOOBJ;
    }

    // always use spinlock_irqsave for safe
    spin_unlock_irqrestore(&pCmnLoc->lock, pCmnLoc->flag);

    return SUCCESS;
}
