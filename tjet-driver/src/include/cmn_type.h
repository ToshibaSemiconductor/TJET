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
 *  @file     cmn_type.h
 *
 *  @brief    common type definitions.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_TYPE_H__)
#define __CMN_TYPE_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
// reference types definition from kernel.
#if defined(USE_OS_LINUX)
#include <linux/types.h>
#endif


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define TRUE  (1)
#define FALSE (0)


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
#endif /* __CMN_TYPE_H__ */
