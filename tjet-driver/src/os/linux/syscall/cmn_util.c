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
 *  @file     cmn_util.c
 *
 *  @brief    common utilities.
 *
 *
 *  @note
 *
 *  
 */
/*==================================================================*/

#include "oscmn.h"

#include <linux/module.h>         // EXPORT_SYMBOL
#include <linux/kernel.h>         // printk
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*-------------------------------------------------------------------
 * Macro Definitions
 *------------------------------------------------------------------*/
static int MonitorSwitch = 0; // 0:OFF 1:ON
static int ModeSelect    = 1; // 0:fix rate 1:link adaptation
module_param(MonitorSwitch, int, S_IRUGO | S_IWUSR);
module_param(ModeSelect,    int, S_IRUGO | S_IWUSR);

#if defined(DBG_ARRAYNUM)
    #define ARRAYMAX DBG_ARRAYNUM
#else
    #define ARRAYMAX 1
#endif
static int arr_argc         = ARRAYMAX;
static int RFADRS[ARRAYMAX] = {0};
static int RFVAL[ARRAYMAX]  = {0};
module_param_array(RFADRS,  int, &arr_argc, S_IRUGO);
module_param_array(RFVAL,   int, &arr_argc, S_IRUGO);
static int FreqUpdN      = 10;             // default value 10
module_param(FreqUpdN,      int, S_IRUGO); // Read Only

/*-------------------------------------------------------------------
 * Structure Definitions
 *------------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Global Definitions
 *------------------------------------------------------------------*/
static int    SuspendState = 0; // 0:Resumed 1:Suspended
static void   (*pEventFunc)(int type);
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend cmn_suspend ={{0}};
#endif

/*-------------------------------------------------------------------
 * Inline Functions
 *------------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *------------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function   : CMN_print
 *------------------------------------------------------------------*/
/**
 * This function wraps print function for debugging.
 * @param     pArg : 1st argument of print
 * @return    nothing.
 * @note      nothing
 */
/*------------------------------------------------------------------*/
void
CMN_print(const char *pArg, ...)
{

    va_list va;
    
    va_start(va, pArg);
    vprintk(pArg, va);
    va_end(va);

    return;

}


/*-------------------------------------------------------------------
 * Function   : CMN_byteSwap16
 *------------------------------------------------------------------*/
/**
 * This function swap byte order of 16bit variable.
 * @param     value : 16bit value
 * @return    swapped 16bit value.
 * @note      nothing
 */
/*------------------------------------------------------------------*/
u16
CMN_byteSwap16(u16 value) 
{
    return (u16)(((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8));
}


/*-------------------------------------------------------------------
 * Function   : CMN_byteSwap32
 *------------------------------------------------------------------*/
/**
 * This function swap byte order of 32bit variable.
 * @param     value : 32bit value.
 * @return    swapped 32bit value.
 * @note      nothing
 */
/*------------------------------------------------------------------*/
u32
CMN_byteSwap32(u32 value)
{
    return (u32)(((value & 0x000000FF) << 24) |
                 ((value & 0x0000FF00) << 8)  |
                 ((value & 0x00FF0000) >> 8)  |
                 ((value & 0xFF000000) >> 24));
}

/*-------------------------------------------------------------------
 * Function   : CMN_getMonitorSwitch
 *------------------------------------------------------------------*/
/**
 * This function get monitor switch parameter
 * @param     void
 * @return    monitor switch parameter value
 * @note      nothing
 */
/*------------------------------------------------------------------*/
int
CMN_getMonitorSwitch(void)
{
    return MonitorSwitch;
}

/*-------------------------------------------------------------------
 * Function   : CMN_getModeSelect
 *------------------------------------------------------------------*/
/**
 * This function get mode select parameter
 * @param     void
 * @return    mode select parameter
 * @note      nothing
 */
/*------------------------------------------------------------------*/
int
CMN_getModeSelect(void)
{
    return ModeSelect;
}

/*-------------------------------------------------------------------
 * Function   : CMN_getRfParam
 *------------------------------------------------------------------*/
/**
 * This function get parameter for RF.
 * @param     S_RFPARAM_PLIST : paramter
 * @return    SUCCESS
 * @note      nothing
 */
/*------------------------------------------------------------------*/
T_CMN_ERR
CMN_getRfParam(S_RFPARAM_PLIST *pParam)
{
    pParam->prmArrayMax = &arr_argc;
    pParam->prmRegAddr  = RFADRS;
    pParam->prmRegValue = RFVAL;
    return SUCCESS;
}

/*-------------------------------------------------------------------
 * Function   : CMN_setSuspendEvent
 *------------------------------------------------------------------*/
/**
 * This function set suspend event
 * @param     func
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
void
CMN_setSuspendEvent(void* func)
{
    pEventFunc = func;
}

/*-------------------------------------------------------------------
 * Function   : CMN_clearSuspendEvent
 *------------------------------------------------------------------*/
/**
 * This function clear suspend event
 * @param     nothing
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
void
CMN_clearSuspendEvent(void)
{
    pEventFunc = NULL;
}

/*-------------------------------------------------------------------
 * Function   : CMN_getSuspendState
 *------------------------------------------------------------------*/
/**
 * This function get suspend state
 * @param     nothing
 * @return    SuspendState (0:Resumed 1:Supended)
 * @note      not used
 */
/*------------------------------------------------------------------*/
int
CMN_getSuspendState(void)
{
    return SuspendState;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/*-------------------------------------------------------------------
 * Function   : CMN_early_suspend
 *------------------------------------------------------------------*/
/**
 * This function early suspend notify
 * @param     h : struct early_suspend pointer
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
static void
CMN_early_suspend(struct early_suspend *h)
{
    CMN_print("--------------Early SUSPEND\n");
    SuspendState = 1;
    if( pEventFunc ) {
        pEventFunc( SuspendState );
    }
}

/*-------------------------------------------------------------------
 * Function   : CMN_late_resume
 *------------------------------------------------------------------*/
/**
 * This function late resume notify
 * @param     h : struct early_suspend pointer
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
static void
CMN_late_resume(struct early_suspend *h)
{
    CMN_print("--------------Late RESUME\n");
    SuspendState = 0;
    if( pEventFunc ) {
        pEventFunc( SuspendState );
    }
}

/*-------------------------------------------------------------------
* Function   : CMN_initEarlySuspend
 *------------------------------------------------------------------*/
/**
 * This function init early suspend
 * @param     nothing
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
void
CMN_initEarlySuspend(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    cmn_suspend.suspend = CMN_early_suspend;
    cmn_suspend.resume  = CMN_late_resume;
    cmn_suspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB;
    register_early_suspend(&cmn_suspend);
#endif
}

/*-------------------------------------------------------------------
* Function   : CMN_exitEarlySuspend
 *------------------------------------------------------------------*/
/**
 * This function exit early suspend
 * @param     nothing
 * @return    nothing
 * @note      nothing
 */
/*------------------------------------------------------------------*/
void
CMN_exitEarlySuspend(void)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&cmn_suspend);
#endif
}

#endif


/*-------------------------------------------------------------------
 * Function   : CMN_getFreqUpdN
 *------------------------------------------------------------------*/
/**
 * This function get frequency update maximum number parameter
 * @param     void
 * @return    frequency update maximum number parameter value
 * @note      nothing
 */
/*------------------------------------------------------------------*/
int
CMN_getFreqUpdN(void)
{
    return FreqUpdN;
}
