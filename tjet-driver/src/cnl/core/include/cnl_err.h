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
 *  @file     cnl_err.h
 *
 *  @brief    CNL error definitions
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNL_ERR_H__)
#define __CNL_ERR_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief CNL request status type.
 */
typedef int T_CNL_ERR;

// general error. wrap common error
#define CNL_SUCCESS             (0)
#define CNL_ERR                 (-1)  // 
#define CNL_ERR_INVSTAT         (-2)  // invalid state error.
#define CNL_ERR_BADPARM         (-3)  // bad parameter error.
#define CNL_ERR_NOOBJ           (-4)  // noobject error.
#define CNL_ERR_QOVR            (-5)  // queuing overflow error.
#define CNL_ERR_TIMEOUT         (-6)  // timeout error.

// CNL depended error.
#define CNL_ERR_HOST_IO         (-7)  // host dependent operation error.(e.g. SD command)
#define CNL_ERR_HW_PROT         (-8)  // HW protocol error.
#define CNL_ERR_CANCELLED       (-9)  // request is cancelled.
#define CNL_ERR_LINKDOWN        (-10) // CNL link down error.
#define CNL_ERR_DEVICE_REMOVED  (-11) // the device is removed from bus.
#define CNL_ERR_REQ_NOT_READY   (-12) // data received but receive request is not queued.
#define CNL_ERR_UNEXP_EVENT     (-13) // unexpected event has occured.

#define CNL_ERR_HW_ERROR1       (-14) // LV1 HW error occurred, need to POR
#define CNL_ERR_HW_ERROR2       (-15) // LV2 HW error occurred, need to Close
#define CNL_ERR_HW_ERROR3       (-16) // LV3 HW error occurred, need to Release.

#define CNL_ERR_HW_SUSPEND      (-17) // Received Suspend request from OS
#define CNL_ERR_HW_RESUME       (-18) // Received Resume request from OS


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
#endif /* __CNL_ERR_H__ */
