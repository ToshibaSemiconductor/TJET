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
 *  @file     cmn_dbg.h
 *
 *  @brief    common debug utilities.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_DBG_H__)
#define __CMN_DBG_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "oscmn.h"

#define DBGLV_OFF   0
#define DBGLV_ERR   1
#define DBGLV_WARN  2
#define DBGLV_INFO  3
#define DBGLV_TRACE 4
#define DBGLV_LOUD  5


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define __FILE_NAME__ CMN_STRRCHR(__FILE__, '/') + 1


//
// DBG_ERR    : print error message
// DBG_ASSERT : assertion utility
//   enabled when DEBUG_LVL >= 1
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_ERR)
#define DBG_ERR(...)                               \
    CMN_print("Tag-CNL: %s(%04u) :", __FILE_NAME__, __LINE__); \
    CMN_print(__VA_ARGS__)
#define DBG_ASSERT(x)                                                   \
    do {                                                                \
        if(!(x))                                                        \
            CMN_print("!!Assertion(%s(%04u))\n", __FILE_NAME__, __LINE__);   \
    } while(0)
#else 
#define DBG_ERR(...)     do {} while(0)
#define DBG_ASSERT(...)  do {} while(0)
#endif

//
// DBG_WARN : print warning message
//   enabled when DEBUG_LVL >= 2
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_WARN)
#define DBG_WARN(...)                              \
    CMN_print("Tag-CNL: %s(%04u) :", __FILE_NAME__, __LINE__);   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_WARN(...)  do {} while(0)
#endif

//
// DBG_MON : print warning message
//   enabled when DEBUG_LVL >= 2
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_WARN)
#define DBG_MON(...)                              \
    CMN_print("TC:");   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_MON(...)  do {} while(0)
#endif

//
// DBG_INFO : print information message
//   enabled when DEBUG_LVL >= 3
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_INFO)
#define DBG_INFO(...)                              \
    CMN_print("Tag-CNL: %s(%04u) :", __FILE_NAME__, __LINE__);   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_INFO(...)  do {} while(0)
#endif

//
// DBG_INFO2 : print information message
//   enabled when DEBUG_LVL >= 3
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_INFO)
#define DBG_INFO2(...)                              \
    CMN_print("TC:");   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_INFO2(...)  do {} while(0)
#endif

//
// DBG_TRACE : print trace message
//   enabled when DEBUG_LVL >= 4
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_TRACE)
#define DBG_TRACE(...)                             \
    CMN_print("Tag-CNL: %s(%04u) :", __FILE_NAME__, __LINE__);   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_TRACE(...)  do {} while(0)
#endif


//
// DBG_LOUD : print message 
//   enabled when DEBUG_LVL >= 5
//
#if defined(DEBUG_LVL) && (DEBUG_LVL >= DBGLV_LOUD)
#define DBG_LOUD(...)                              \
    CMN_print("Tag-CNL: %s(%04u) :", __FILE_NAME__, __LINE__);   \
    CMN_print(__VA_ARGS__)
#else 
#define DBG_LOUD(...)  do {} while(0)
#endif


//
// function trace utility(using DBG_LOUD)
//
#define DBG_FUNC_START() DBG_LOUD("Enter into function(%s)\n", __FUNCTION__)
#define DBG_FUNC_EXIT()  DBG_LOUD("Exit  from function(%s)\n", __FUNCTION__)


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
#endif /* __CMN_DBG_H__ */
