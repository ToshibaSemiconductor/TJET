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
 *  @file     sys_base.h
 *
 *  @brief    This header file defines ths ANSI C functions depended on system.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__SYS_BASE_H__)
#define __SYS_BASE_H__

#include <linux/module.h>


/*------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 *	@brief macros to convert the byte order
 */
#if defined(USE_LE_CPU)
#define	CMN_H2BE32(x)                  CMN_byteSwap32(x)
#define	CMN_H2BE16(x)                  CMN_byteSwap16(x)
#define	CMN_BE2H32(x)                  CMN_byteSwap32(x)
#define	CMN_BE2H16(x)                  CMN_byteSwap16(x)
#define	CMN_H2LE32(x)                  (x)
#define	CMN_H2LE16(x)                  (x)
#define	CMN_LE2H32(x)                  (x)
#define	CMN_LE2H16(x)                  (x)
#else   /* USE_LE_CPU */
#define	CMN_H2BE32(x)                  (x)
#define	CMN_H2BE16(x)                  (x)
#define	CMN_BE2H32(x)                  (x)
#define	CMN_BE2H16(x)                  (x)
#define	CMN_H2LE32(x)                  CMN_byteSwap32(x)
#define	CMN_H2LE16(x)                  CMN_byteSwap16(x)
#define	CMN_LE2H32(x)                  CMN_byteSwap32(x)
#define	CMN_LE2H16(x)                  CMN_byteSwap16(x)
#endif	/* USE_LE_CPU */


/**
 *	@brief macros to handle the memory
 */
#define	CMN_MEMCPY                     memcpy     // ANSI C
#define	CMN_MEMSET                     memset     // ANSI C
#define	CMN_MEMCMP                     memcmp     // ANSI C


/**
 *	@brief macros to handle character string
 */
#define	CMN_STRCAT                     strcat     // ANSI C
#define	CMN_STRNCAT                    strncat    // ANSI C
#define	CMN_STRCHR                     strchr     // ANSI C
#define	CMN_STRRCHR                    strrchr    // ANSI C
#define	CMN_STRCMP                     strcmp     // ANSI C
#define	CMN_STRNCMP                    strncmp    // ANSI C
#define	CMN_STRCPY                     strcpy     // ANSI C
#define	CMN_STRNCPY                    strncpy    // ANSI C
#define	CMN_STRLEN                     strlen     // ANSI C


#define MAX(x,y) ((x) > (y)) ? (x) : (y)
#define MIN(x,y) ((x) > (y)) ? (y) : (x)
/*------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/
typedef struct tagS_OS_MSG{
    u32 dummy;
}S_OS_MSG;

#endif	/* __SYS_BASE_H__ */
