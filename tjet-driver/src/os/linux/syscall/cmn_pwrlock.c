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
 *  @file     cmn_pwrlock.c
 *
 *  @brief    This file defines the functions which handle power lock
 *            object and method 
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_cnf.h"

#include <linux/module.h>   // EXPORT_SYMBOL
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h> // for wake_lock
#endif


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/
#ifdef CONFIG_HAS_WAKELOCK
static struct wake_lock lock;
#endif

/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function   : CMN_createPowerLock
 *-----------------------------------------------------------------*/
/**
 * This function create Lock manager.
 * @param     nothing.
 * @return    nothing
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_createPowerLock(void)
{
#ifdef CONFIG_HAS_WAKELOCK
    wake_lock_init(&lock, WAKE_LOCK_SUSPEND, "tsbpm");
#endif
}

/*-------------------------------------------------------------------
 * Function   : CMN_deletePowerLock
 *-----------------------------------------------------------------*/
/**
 * This function deletes Power lock.
 * @param     nothing.
 * @return    nothing
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
void
CMN_deletePowerLock(void)
{
#ifdef CONFIG_HAS_WAKELOCK
    wake_lock_destroy(&lock);
#endif
}


/*-------------------------------------------------------------------
 * Function   : CMN_lockPower
 *-----------------------------------------------------------------*/
/**
 * This function lock Power lock.
 * @param     nothing.
 * @return    SUCCESS     (normally completion)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_lockPower(void)
{
#ifdef CONFIG_HAS_WAKELOCK
    if( wake_lock_active(&lock) == 0 ) {
        wake_lock(&lock);
    }
#endif
    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_unlockPower
 *-----------------------------------------------------------------*/
/**
 * This function unlock Power lock.
 * @param     nothing.
 * @return    SUCCESS     (normally completion)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_unlockPower(void)
{
#ifdef CONFIG_HAS_WAKELOCK
    if( wake_lock_active(&lock) != 0 ) {
        wake_unlock(&lock);
    }
#endif
    return SUCCESS;
}

