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
 *  @file     cmn_err.h
 *
 *  @brief    Describes error type.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_ERR_H__)
#define __CMN_ERR_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
typedef int T_CMN_ERR;

#define SUCCESS                      (0)

#define ERR_SYSTEM                  (-5)  // the system error/unknown cause error occured.
#define ERR_RSVFUNC                (-10)  // the function has already reserved.
#define ERR_RSVATTR                (-11)  // the attribute has already reserved.
#define ERR_BADPARM                (-17)  // the input parameter is invalid.
#define ERR_INVID                  (-18)  // the ID is invalid.
#define ERR_CTX                    (-25)  // the context error occured.
#define ERR_MEMVIO                 (-26)  // the memory access violation occured.
#define ERR_OBJVIO                 (-27)  // the object access violation occured.
#define ERR_ILGUSE                 (-28)  // the service call is used illegally
#define ERR_NOMEM                  (-33)  // the memory or resource is depleted.
#define ERR_NOID                   (-34)  // the ID is depleted.
#define ERR_INVSTAT                (-41)  // the internal status of the object is invalid.
#define ERR_NOOBJ                  (-42)  // the object doesn't exist.
#define ERR_QOVR                   (-43)  // the queuing overflow occured.
#define ERR_RLWAIT                 (-49)  // the wating is relseased forcibly.
#define ERR_TIMEOUT                (-50)  // the timeout occured/ the polling failed.
#define ERR_DELETED                (-51)  // the object is deleted.
#define ERR_CHGSTAT                (-52)  // the internal status of the object changed.
#define ERR_ACCNBLK                (-57)  // the Non-Blocking request has been accepted.
#define ERR_BUFOVR                 (-58)  // the buffer overflow occured.

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
#endif /* __CMN_ERR_H__ */
