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
 *  @file     cmn_sync.c
 *
 *  @brief    This file defines the functions which handle synchronization
 *            object and method like the semaphore.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/module.h>   // EXPORT_SYMBOL

#include <linux/version.h>
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
 * @breif semaphore object (for linux)
 */
typedef struct tagS_CMN_SEM {
    u8               id;
    struct semaphore sem;
    u16              semMax;
} S_CMN_SEM;


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static DECLARE_MUTEX(mutex);
#else
static DEFINE_SEMAPHORE(mutex); 
#endif
static S_CMN_SEM g_cmnSem[CMN_SEM_MAX_NUM] = {};


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
extern void CMN_initSem(void);


/*-------------------------------------------------------------------
 * Function   : CMN_initSem
 *-----------------------------------------------------------------*/
/**
 * This function initialize semaphore manager.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_initSem()
{

    int i;

    for(i=0; i<CMN_SEM_MAX_NUM; i++) {
        g_cmnSem[i].id = 0; // 0 means unused.
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createSem
 *-----------------------------------------------------------------*/
/**
 * This function creates a semaphore with the specified ID.
 * @param     semID    : ID of the semaphore
 * @param     semAttr  : the attribute of the semaphore
 * @param     semInit  : the number of initial resources
 * @param     semMax   : the number of maximum resources
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createSem(u8   semID,
              u32  semAttr,
              u16  semInit,
              u16  semMax)
{

    S_CMN_SEM      *pCmnSem;

    // check parameter
    if (semID == 0 || semID > CMN_SEM_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnSem = &(g_cmnSem[semID-1]);
    down(&mutex);
    if(pCmnSem->id != 0) {
        up(&mutex);
        return ERR_INVSTAT;
    }

    // max value is not effective.
    pCmnSem->id = semID;
    sema_init(&pCmnSem->sem, semInit);
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_acreateSem
 *-----------------------------------------------------------------*/
/**
 * This function creates a semaphore
 * @param     semAttr  : the attribute of the semaphore
 * @param     semInit  : the number of initial resources
 * @param     semMax   : the number of maximum resources
 * @return    assigned ID (normally completion)
 * @return    ERR_NOID    (no more semaphore object is availbale)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_acreateSem(u32  semAttr,
               u16  semInit,
               u16  semMax)
{

    S_CMN_SEM      *pCmnSem;
    u8              semID;
    
    down(&mutex);
    for(semID=1; semID<=CMN_SEM_MAX_NUM; semID++) {
        pCmnSem = &(g_cmnSem[semID-1]);
        if(pCmnSem->id == 0) {
            // found.
            // max value is not effective.
            pCmnSem->id = semID;
            sema_init(&pCmnSem->sem, semInit);
            up(&mutex);
            return semID;
        }
    }
    up(&mutex);
    return ERR_NOID;
}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteSem
 *-----------------------------------------------------------------*/
/**
 * This function deletes a semaphore with the specified ID.
 * @param     semID  : ID of the semaphore
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteSem(u8 semID)
{

    S_CMN_SEM      *pCmnSem;

    // check parameter
    if (semID == 0 || semID > CMN_SEM_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnSem = &(g_cmnSem[semID-1]);

    down(&mutex);
    if(pCmnSem->id == 0) {
        up(&mutex);
        return ERR_NOOBJ;
    }

    pCmnSem->id = 0;
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_singnalSem
 *-----------------------------------------------------------------*/
/**
 * This function returns a resource to the specified semaphore.
 * @param     semID  : ID of the semaphore
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_signalSem(u8 semID)
{

    S_CMN_SEM      *pCmnSem;

    // check parameter
    if (semID == 0 || semID > CMN_SEM_MAX_NUM)
    {
        return ERR_INVID;
    }
    pCmnSem = &(g_cmnSem[semID-1]);

    if(pCmnSem->id == 0) {
        return ERR_NOOBJ;
    }

    up(&pCmnSem->sem);
    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_waitSem
 *-----------------------------------------------------------------*/
/**
 * This function gets a resource from the specified semaphore.
 * @param     semID    : ID of the semaphore
 * @param     timeOut  : the value of timeout (ms) // not supported
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @return    ERR_RLWAIT  (force release during waiting)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_waitSem(u8  semID,
            u16 timeOut)
{

    int             retval;
    S_CMN_SEM      *pCmnSem;

    // check parameter
    if (semID == 0 || semID > CMN_SEM_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnSem = &(g_cmnSem[semID-1]);

    if(pCmnSem->id == 0) {
        return ERR_NOOBJ;
    }

    retval = down_interruptible(&pCmnSem->sem);
    if(retval != 0) {
        // interrupted.
        return ERR_RLWAIT;
    }

    return SUCCESS;
}
