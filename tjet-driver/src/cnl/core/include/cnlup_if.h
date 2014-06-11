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
 *  @file     cnlup_if.h
 *
 *  @brief    CNL upper layer common interface header file.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNLUP_IF_H__)
#define __CNLUP_IF_H__

#include "cnl_if.h"

/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External Functions
 *-----------------------------------------------------------------*/
//
// for CNL module
//
extern T_CMN_ERR CNLUP_registerCNL  (void *, S_CNL_OPS *);
extern T_CMN_ERR CNLUP_unregisterCNL(void *);


#endif /* __CNLUP_IF_H__ */
