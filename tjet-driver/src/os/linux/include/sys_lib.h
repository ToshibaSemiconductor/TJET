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
 *  @file     sys_lib.h
 *
 *  @brief    This header file defines the convenient macro/inline function
 *            used in the OS common library and the functions exported
 *            from each modulees depended on system.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__SYS_LIB_H__)
#define __SYS_LIB_H__


#if defined(USE_OS_LINUX)
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <pthread.h>
#include <time.h>

#include "cmn_type.h"
#include "cmn_err.h"
#else
#error You shoud include system specific header files
#endif	/* defined(USE_OS_LINUX) */

#include "cmn_debug.h"

/*------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief the type of the time
 */
enum tagE_CMN_SYS_TIME_TYPE {
    CMN_SYS_TIME_TYPE_ABS,
    CMN_SYS_TIME_TYPE_REL,
};


/*------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/
/**
 *	@brief inline function to check whethrer the specified thread exist
 *         or not.
 */
static inline int
CMN_doesThreadExist(pthread_t thHL)
{
    int  retval = FALSE;
    int  policy;
    struct sched_param  param;


    retval = pthread_getschedparam(thHL, &policy, &param);

    if (retval == SUCCESS)
    {
        retval = TRUE;
    }


    return retval;
}


/**
 *	@brief inline function to check whethrer the specified thread is
 *         same as the thread which calls this.
 */
static inline int
CMN_isThisSameAs(pthread_t thHL)
{
    int  retval = FALSE;
    pthread_t  this;


    this = pthread_self();

    if (this == thHL)
    {
        retval = TRUE;
    }


    return retval;
}


/**
 *	@brief inline function to wait for the completion of thread.
 */
static inline int
CMN_waitForCompletionOfThread(pthread_t thHL)
{
    int  retval = SUCCESS;


    DBG_TRACE("wait for the completion of the thread (0x%x) \n",
              (uint)thHL);

    retval = pthread_join(thHL, NULL);

    DBG_INFO("\"pthread_join\" return (%d) (thread HL = 0x%x)\n",
             retval, (uint)thHL);

    if (retval != SUCCESS)
    {
        if (retval == ESRCH)
        {
            DBG_WARN("\"pthread_join\" return error (%d), but treat as SUCCESS \n", retval);
            retval = SUCCESS;
        }
        else
        {
            // DBG_ERR("\"pthread_join\" return error (%d) \n", errno);
            retval = ERR_INVSTAT;
        }
    }


    return retval;
}


/**
 *	@brief inline function to sets the attribute of the thread.
 */
static inline void
CMN_makeThreadAttr(pthread_attr_t *pAttr)
{
    struct sched_param  scheParam;


    // attribute setting
    {
        // initialize the attribute of pthread
        pthread_attr_init(pAttr);

        // about scheduling
        pthread_attr_setschedpolicy(pAttr, SCHED_RR);
        pthread_attr_getschedparam(pAttr, &scheParam);
        pthread_attr_setschedparam(pAttr, &scheParam);
    }


    return;
}


/*------------------------------------------------------------------
 * External Functions
 *-----------------------------------------------------------------*/
extern int   CMN_convTimeRelToAbs(u16 , struct timespec *, u8);
extern int   CMN_waitForCompletionOfTask(u8);


#endif	/* __SYS_LIB_H__ */
