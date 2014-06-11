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
 *  @file     cmn_time.c
 *
 *  @brief    This file defines the functions which handle the timer.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/module.h>  // EXPORT_SYMBOL
#include <linux/time.h>    // timer API
#include <linux/jiffies.h> // get system time

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
 * @breif alarm/cyclic timer object (for linux)
 */
typedef struct tagS_CMN_TIM {
    u8                id;
    struct timer_list timer;
} S_CMN_TIM;


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
static DECLARE_MUTEX(mutex);
#else
static DEFINE_SEMAPHORE(mutex); 
#endif
static S_CMN_TIM g_cmnTim[CMN_TIM_MAX_NUM] = {};


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/
extern void CMN_initTimer(void);


/*-------------------------------------------------------------------
 * Function   : CMN_initTimer
 *-----------------------------------------------------------------*/
/**
 * This function initialize timer manager.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_initTimer()
{

    int i;

    for(i=0; i<CMN_TIM_MAX_NUM; i++) {
        g_cmnTim[i].id = 0; // 0 means unused.
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createAlarmTim
 *-----------------------------------------------------------------*/
/**
 * This function creates a alarm timer with the specified ID.
 * @param     almID     : ID of the alarm timer
 * @param     almAttr   : the attribute of the alarm timer
 *                        (ignored in this version)
 * @param     pExInfo   : the extended information to the alarm timer
 * @param     pAlmFunc  : the pointer to the starting address
 *                        when the alarm timer expires
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the status of message box is already created)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createAlarmTim(u8   almID,
                   u32  almAttr,
                   void *pExtInfo,
                   void *pAlmFunc)
{

    S_CMN_TIM      *pCmnTim;

    // check parameter
    if (almID == 0 || almID > CMN_TIM_MAX_NUM) {
        return ERR_INVID;
    }
    if (pAlmFunc == NULL) {
        return ERR_BADPARM;
    }

    pCmnTim = &(g_cmnTim[almID-1]);
    
    down(&mutex);
    if(pCmnTim->id != 0) {
        up(&mutex);
        return ERR_INVSTAT;
    }
    pCmnTim->id = almID;
    init_timer(&pCmnTim->timer);
    pCmnTim->timer.data     = (unsigned long)pExtInfo;
    pCmnTim->timer.function = (void *)pAlmFunc;
    // delay will be set later.
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_acreateAlarmTim
 *-----------------------------------------------------------------*/
/**
 * This function creates a alarm timer with the specified ID.
 * @param     almAttr   : the attribute of the alarm timer
 *                        (ignored in this version)
 * @param     pExInfo   : the extended information to the alarm timer
 * @param     pAlmFunc  : the pointer to the starting address
 *                        when the alarm timer expires
 * @return    SUCCESS     (normally completion)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_NOID    (no more timer object is availbale)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_acreateAlarmTim(u32  almAttr,
                    void *pExtInfo,
                    void *pAlmFunc)
{

    S_CMN_TIM      *pCmnTim;
    u8             almID;

    if (pAlmFunc == NULL) {
        return ERR_BADPARM;
    }

    down(&mutex);
    for(almID=1; almID<=CMN_TIM_MAX_NUM; almID++) {
        pCmnTim = &(g_cmnTim[almID-1]);
        if(pCmnTim->id == 0) {
            pCmnTim->id = almID;
            init_timer(&pCmnTim->timer);
            pCmnTim->timer.data     = (unsigned long)pExtInfo;
            pCmnTim->timer.function = (void *)pAlmFunc;
            // delay will be set later.
            up(&mutex);
            return almID;
        }
    }
    up(&mutex);
    return ERR_NOID;
}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteAlarmTim
 *-----------------------------------------------------------------*/
/**
 * This function deletes a alarm timer with the specified ID.
 * If the alarm timer is working, it is deleted after stopping the timer.
 * @param     almID  : ID of the alarm timer
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteAlarmTim(u8 almID)
{
    S_CMN_TIM      *pCmnTim;

    // check parameter
    if (almID == 0 || almID > CMN_TIM_MAX_NUM) {
        return ERR_INVID;
    }
    pCmnTim = &(g_cmnTim[almID-1]);
    
    down(&mutex);
    if(pCmnTim->id == 0) {
        up(&mutex);
        return ERR_NOOBJ;
    }
    pCmnTim->id = 0;
    up(&mutex);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_startAlarmTim
 *-----------------------------------------------------------------*/
/**
 * This function starts the specified  alarm timer with the relative time
 * in which the handler will start.
 * @param     almID   : ID of the alarm timer
 * @param     almTim  : the relative time in which the handler will start(ms)
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object doesn't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_startAlarmTim(u8  almID,
                  u16 almTim)
{

    S_CMN_TIM      *pCmnTim;

    // check parameter
    if (almID == 0 || almID > CMN_TIM_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTim = &(g_cmnTim[almID-1]);

    if(pCmnTim->id == 0) {
        return ERR_NOOBJ;
    }

    // set delay time.
    pCmnTim->timer.expires  = jiffies + msecs_to_jiffies(almTim);
    add_timer(&pCmnTim->timer);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_stopAlarmTim
 *-----------------------------------------------------------------*/
/**
 * This function stops the specified  alarm timer.
 * @param     almID  : ID of the alarm timer
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_NOOBJ   (the object dosen't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_stopAlarmTim(u8 almID)
{

    S_CMN_TIM      *pCmnTim;

    // check parameter
    if (almID == 0 || almID > CMN_TIM_MAX_NUM) {
        return ERR_INVID;
    }

    pCmnTim = &(g_cmnTim[almID-1]);

    if(pCmnTim->id == 0) {
        return ERR_NOOBJ;
    }

    // delete timer
    del_timer_sync(&pCmnTim->timer);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_getTime
 *-----------------------------------------------------------------*/
/**
 * get system time(msecs from system wakeup)
 * @param     pTime   : pointer to the System time stored.
 * @return    SUCCESS     (normally completion)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_getTime(u32 *pTime)
{

    *pTime = jiffies_to_msecs(jiffies);

    return SUCCESS;
}
