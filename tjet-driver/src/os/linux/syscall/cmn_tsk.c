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
 *  @file     cmn_tsk.c
 *
 *  @brief    This file defines the functions which handles the task.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"
#include "cmn_err.h"

#include <linux/kthread.h> // kthread API.
#include <linux/module.h>  // EXPORT_SYMBOL
#include <linux/wait.h>    // waitqueue API
#include <linux/delay.h>   // msleep.
#include <asm/atomic.h>    // atomic API.
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

/*------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief the status of the task object
 */
enum tagE_CMN_MSG_TSK_STATUS {
    CMN_TASK_AVAILABLE,
    CMN_TASK_CREATED,
};


/*------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/
/**
 * @breif task object (for linux)
 */
typedef struct tagS_CMN_TSK {
    u8 id;

    struct task_struct *pTsk;     // task struct.
    void               *pTskFunc; // task function.
    void               *pArg;     // task function argument.

    atomic_t            cnt;      // count to implement slp_tsk/wup_tsk
    wait_queue_head_t   wait;     // count to implement slp_tsk/wup_tsk
} S_CMN_TSK;


/*------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static DECLARE_MUTEX(mutex);
#else
static DEFINE_SEMAPHORE(mutex); 
#endif
static S_CMN_TSK g_cmnTsk[CMN_TASK_MAX_NUM] = {};

static const char *task_names[] = 
{
    "CNL_thread",
    "XXX_thread",
    // add if needed.
};

/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
extern void CMN_initTask(void);


/*-------------------------------------------------------------------
 * Function   : CMN_initTask
 *-----------------------------------------------------------------*/
/**
 * This function initialize task manager.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_initTask()
{

    int i;

    for(i=0; i<CMN_TASK_MAX_NUM; i++) {
        g_cmnTsk[i].id = 0; // 0 means unused.
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createTask
 *-----------------------------------------------------------------*/
/**
 * This function creates a task with the specified ID.
 * @param     tskID     : ID of the task
 * @param     tskAttr   : the attribute of the task
 * @param     pExtInfo  : the extended information to the task
 * @param     pTskFunc  : the pointer to the starting address of the task
 * @param     tskPri    : the priority of the task
 * @param     stkSize   : the stack size used in the task
 *                        (ignored in this version)
 * @param     pStkPtr   : the pointer to the stack used in the task
 *                        (ignored in this version)
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createTask(u8   tskID,
               u32  tskAttr,
               void *pExtInfo,
               void *pTskFunc,
               u8   tskPri,
               u32  stkSize,
               void *pStkPtr)
{

    S_CMN_TSK *pCmnTsk;

    // check parameter
    if (tskID == 0 || tskID > CMN_TASK_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTsk = &(g_cmnTsk[tskID-1]);

    down(&mutex);
    // setup common task struct.
    if(pCmnTsk->id != 0) {
        // already used.
        up(&mutex);
        return ERR_INVSTAT;
    }
    pCmnTsk->id       = tskID;
    pCmnTsk->pTskFunc = pTskFunc;
    pCmnTsk->pArg     = pExtInfo;
    
    atomic_set(&(pCmnTsk->cnt), 0);
    init_waitqueue_head(&(pCmnTsk->wait));
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_acreateTask
 *-----------------------------------------------------------------*/
/**
 * This function creates a task with the specified ID.
 * @param     tskAttr   : the attribute of the task
 * @param     pExtInfo  : the extended information to the task
 * @param     pTskFunc  : the pointer to the starting address of the task
 * @param     tskPri    : the priority of the task
 * @param     stkSize   : the stack size used in the task
 *                        (ignored in this version)
 * @param     pStkPtr   : the pointer to the stack used in the task
 *                        (ignored in this version)
 * @return    assigned ID (normally completion)
 * @return    ERR_NOID    (no ID)
 * @note      nothing
 */
/*------------------------------------------------------------------*/
T_CMN_ERR
CMN_acreateTask(u32  tskAttr,
                void *pExtInfo,
                void *pTskFunc,
                u8   tskPri,
                u32  stkSize,
                void *pStkPtr)
{

    S_CMN_TSK *pCmnTsk;
    u8         tskID;

    down(&mutex);
    for(tskID=1; tskID<=CMN_TASK_MAX_NUM; tskID++) {
        pCmnTsk = &(g_cmnTsk[tskID-1]);
        if(pCmnTsk->id == 0) {
            // found.
            // setup common task struct.
            pCmnTsk->id       = tskID;
            pCmnTsk->pTskFunc = pTskFunc;
            pCmnTsk->pArg     = pExtInfo;
            atomic_set(&(pCmnTsk->cnt), 0);
            init_waitqueue_head(&(pCmnTsk->wait));
            up(&mutex);
            return tskID;
        }
    }
    up(&mutex);

    return ERR_NOID; // no more empty task.
}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteTask
 *-----------------------------------------------------------------*/
/**
 * This function deletes a task with the specified ID.
 * @param     tskID  : ID of the task
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteTask(u8 tskID)
{
    S_CMN_TSK *pCmnTsk;

    // check parameter
    if (tskID == 0 || tskID > CMN_TASK_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTsk = &(g_cmnTsk[tskID-1]);
    down(&mutex);
    if(pCmnTsk->id == 0) {
        up(&mutex);
        return ERR_NOOBJ;
    }
    pCmnTsk->id = 0;
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_startTask
 *-----------------------------------------------------------------*/
/**
 * This function start the specified task.
 * @param     tskID     : ID of the task
 * @param     pInParam  : the pointer to the parameter on task starting
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @return    ERR_INVSTAT (internal error)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_startTask(u8   tskID,
              void *pInParam)
{

    S_CMN_TSK *pCmnTsk;

    // check parameter
    if (tskID == 0 || tskID > CMN_TASK_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTsk = &(g_cmnTsk[tskID-1]);

    if(pCmnTsk->id == 0) {
        return ERR_NOOBJ;
    }

    if(pInParam)
        pCmnTsk->pArg = pInParam;

    pCmnTsk->pTsk = kthread_create(pCmnTsk->pTskFunc, pCmnTsk->pArg, "%s", task_names[tskID-1]);
    if(IS_ERR(pCmnTsk->pTsk)) {
        return ERR_INVSTAT;
    }
    /* wake up thread */
    wake_up_process(pCmnTsk->pTsk);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_exitTask
 *-----------------------------------------------------------------*/
/**
 * This function ends the own task.
 * @param     nothing
 * @return    nothing
 * @return    nothing
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
void
CMN_exitTask(void)
{
    return; // do nothing
}


/*-------------------------------------------------------------------
 * Function   : CMN_terminateTask
 *-----------------------------------------------------------------*/
/**
 * This function ends the specified task.
 * @param     tskID  : ID of the task
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_terminateTask(u8 tskID)
{

    S_CMN_TSK *pCmnTsk;

    // check parameter
    if (tskID == 0 || tskID > CMN_TASK_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTsk = &(g_cmnTsk[tskID-1]);

    if(pCmnTsk->id == 0) {
        return ERR_NOOBJ;
    }

    kthread_stop(pCmnTsk->pTsk);

    return SUCCESS;
}



/*-------------------------------------------------------------------
 * Function   : CMN_sleepTask
 *-----------------------------------------------------------------*/
/**
 * This function makes the own task wait for starting.
 * (There is still no decision whether this is implemented or not.)
 * @param     timeOut  : value of timeout // not supported.
 * @return    SUCCESS     (normally completion)
 * @return    ERR_RLWAIT  (force release during wait)
 * @return    ERR_SYSTEM  (internal error)
 * @note      
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_sleepTask(u16 timeOut)
{
    
    S_CMN_TSK *pCmnTsk = NULL;
    int        i;
    T_CMN_ERR  retval;

    for(i=0; i<CMN_TASK_MAX_NUM; i++) {
        if(g_cmnTsk[i].pTsk == current) {
            pCmnTsk = &(g_cmnTsk[i]);
            break;
        }
    }

    if(pCmnTsk == NULL) {
        return ERR_SYSTEM;
    }

    // timeout not supported.
    retval = wait_event_interruptible(pCmnTsk->wait,
                                      (atomic_read(&pCmnTsk->cnt) > 0) ||
                                      (kthread_should_stop()));
    if(retval != 0) {
        // interrupted..
        return ERR_RLWAIT;
    }

    if(kthread_should_stop()) {
        // terminateTask called.
        return ERR_RLWAIT; // E_RLWAI
    }
    else {
        // wakeupTask called.
        // decrement wakeup count.
        atomic_dec(&pCmnTsk->cnt);
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_wakeupTask
 *-----------------------------------------------------------------*/
/**
 * This function makes the specified task wake up.
 * (There is still no decision whether this is implemented or not.)
 * @param     tskID     : ID of the task
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      NOT SUPPORTED YET
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_wakeupTask(u8 tskID)
{
    S_CMN_TSK *pCmnTsk;

    // check parameter
    if (tskID == 0 || tskID > CMN_TASK_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTsk = &(g_cmnTsk[tskID-1]);

    if(pCmnTsk->id == 0) {
        return ERR_NOOBJ;
    }

    atomic_inc(&pCmnTsk->cnt);
    wake_up_interruptible(&pCmnTsk->wait);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_delayTask
 *-----------------------------------------------------------------*/
/**
 * This function make the own task delayed.
 * @param     dlyTime  : the value of time to make the task delayed (ms)
 * @return    SUCCESS     (normally completion)
 * @return    ERR_RLWAIT  (force release during wait)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_delayTask(u16 dlyTime)
{

    // not supported.
    unsigned long retval;
    retval = msleep_interruptible(dlyTime);
    if(retval != 0) {
        return ERR_RLWAIT; // E_RLWAI
    }

    return SUCCESS;
}
