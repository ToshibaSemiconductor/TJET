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
 *  @file     oscmn.c
 *
 *  @brief    This file defines the functions which handles module.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/kthread.h> // kthread API.
#include <linux/module.h>  // EXPORT_SYMBOL
#include <linux/wait.h>    // waitqueue API
#include <linux/delay.h>   // msleep.
#include <asm/atomic.h>    // atomic API.
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.0";
#define DRIVER_DESC "CNL OS Abstraction Driver";

/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
extern void CMN_initSem(void);
extern void CMN_initCpuLock(void);
extern void CMN_initFixedMemPool(void);
extern void CMN_initTask(void);
extern void CMN_initTimer(void);
#ifdef CONFIG_HAS_EARLYSUSPEND
extern void CMN_initEarlySuspend(void);
extern void CMN_exitEarlySuspend(void);
#endif

/*-------------------------------------------------------------------
 * Function : OSCMN_init
 *-----------------------------------------------------------------*/
/**
 * Initialize routine called when module inserted to kernel.
 * @param   nothing.
 * @return  0  (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static int __init
OSCMN_init(void) 
{
    
    CMN_initSem();
    CMN_initCpuLock();
    CMN_initFixedMemPool();
    CMN_initTask();
    CMN_initTimer();
#ifdef CONFIG_HAS_EARLYSUSPEND
    CMN_initEarlySuspend();
#endif
    return 0;

}


/*-------------------------------------------------------------------
 * Function : CNL_exit
 *-----------------------------------------------------------------*/
/**
 * Initialize routine called when module inserted to kernel.
 * @param   nothing.
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void __exit
OSCMN_exit(void) 
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    CMN_exitEarlySuspend();
#endif
    return;
}


module_init(OSCMN_init);
module_exit(OSCMN_exit);

/* Module information */
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_VERSION( DRIVER_VERSION );

//
// EXPORT_SYMBOLs
//
// from cmn_mem.c
EXPORT_SYMBOL(CMN_createFixedMemPool);
EXPORT_SYMBOL(CMN_acreateFixedMemPool);
EXPORT_SYMBOL(CMN_deleteFixedMemPool);
EXPORT_SYMBOL(CMN_getFixedMemPool);
EXPORT_SYMBOL(CMN_releaseFixedMemPool);
EXPORT_SYMBOL(CMN_allocMem);
EXPORT_SYMBOL(CMN_releaseMem);

// from cmn_sync.c
EXPORT_SYMBOL(CMN_createSem);
EXPORT_SYMBOL(CMN_acreateSem);
EXPORT_SYMBOL(CMN_deleteSem);
EXPORT_SYMBOL(CMN_signalSem);
EXPORT_SYMBOL(CMN_waitSem);

// from cmn_lock.c
EXPORT_SYMBOL(CMN_createCpuLock);
EXPORT_SYMBOL(CMN_acreateCpuLock);
EXPORT_SYMBOL(CMN_deleteCpuLock);
EXPORT_SYMBOL(CMN_lockCpu);
EXPORT_SYMBOL(CMN_unlockCpu);

// from cmn_task.c
EXPORT_SYMBOL(CMN_createTask);
EXPORT_SYMBOL(CMN_acreateTask);
EXPORT_SYMBOL(CMN_deleteTask);
EXPORT_SYMBOL(CMN_startTask);
EXPORT_SYMBOL(CMN_exitTask);
EXPORT_SYMBOL(CMN_terminateTask);
EXPORT_SYMBOL(CMN_sleepTask);
EXPORT_SYMBOL(CMN_wakeupTask);
EXPORT_SYMBOL(CMN_delayTask);

// from cmn_time.c
EXPORT_SYMBOL(CMN_createAlarmTim);
EXPORT_SYMBOL(CMN_acreateAlarmTim);
EXPORT_SYMBOL(CMN_deleteAlarmTim);
EXPORT_SYMBOL(CMN_startAlarmTim);
EXPORT_SYMBOL(CMN_stopAlarmTim);
EXPORT_SYMBOL(CMN_getTime);

// from cmn_util.c
EXPORT_SYMBOL(CMN_print);
EXPORT_SYMBOL(CMN_byteSwap16);
EXPORT_SYMBOL(CMN_byteSwap32);
EXPORT_SYMBOL(CMN_getMonitorSwitch);
EXPORT_SYMBOL(CMN_getModeSelect);
EXPORT_SYMBOL(CMN_getRfParam);
EXPORT_SYMBOL(CMN_getFreqUpdN);
EXPORT_SYMBOL(CMN_getSuspendState);
EXPORT_SYMBOL(CMN_setSuspendEvent);
EXPORT_SYMBOL(CMN_clearSuspendEvent);

// from cmn_pwrlock.c
EXPORT_SYMBOL(CMN_createPowerLock);
EXPORT_SYMBOL(CMN_deletePowerLock);
EXPORT_SYMBOL(CMN_lockPower);
EXPORT_SYMBOL(CMN_unlockPower);

//
// declare MODULE_LICENCE,AUTHOR if necessary.
//
