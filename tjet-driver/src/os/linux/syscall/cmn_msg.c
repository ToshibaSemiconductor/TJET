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
 *  @file     cmn_msg.c
 *
 *  @brief    This file defines the functions which handle the message
 *            and message box.
 *
 *
 *  @note
 */
/*=================================================================*/

#include "oscmn.h"
#include "cmn_err.h"


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes Functions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function   : CMN_initMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function initializes the message box managers.
 * And, this must be called once before creating the message box.
 * @param     nothing
 * @return    SUCCESS (normally completion)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
int
CMN_initMsgBox(void)
{
    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CMN_createMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function creates a message box with the specified ID.
 * @param     msgBoxID    : ID of the message box
 * @param     msgBoxAttr  : the attribute of the message box
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_createMsgBox(u8  msgBoxID,
                 u32 msgBoxAttr)
{
    return ERR_INVID; // not supported.
}


/*-------------------------------------------------------------------
 * Function   : CMN_deleteMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function deletes a message box with the specified ID.
 * @param     msgBoxID  : ID of message box
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_deleteMsgBox(u8 msgBoxID)
{
    return ERR_INVID; // not supported.
}


/*-------------------------------------------------------------------
 * Function   : CMN_sendMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function sends any type message to the specified message box.
 * And, timeout isn't supported yet.
 * @param     msgBoxID  : ID of message box
 * @param     msgPri    : the priority of message
 * @param     pMsg      : the pointer to message
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_sendMsgBox(u8        msgBoxID,
               u8        msgPri,
               S_CMN_MSG *pMsg)
{
    return ERR_INVID; // not supported.
}


/*-------------------------------------------------------------------
 * Function   : CMN_receiveMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function gets any type messages from the specified messge box.
 * @param     msgBoxID  : ID of message box
 * @param     pMsg      : the pointer to the pointer to message
 * @param     timeOut   : the value of timeout (ms)
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @return    ERR_TIMEOUT (the timeout occured/ the polling failed)
 * @return    ERR_DELETED (the object is deleted)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_receiveMsgBox(u8        msgBoxID,
                  S_CMN_MSG **pMsg,
                  u16       timeOut)
{
    return ERR_INVID; // not supported.
}


/*-------------------------------------------------------------------
 * Function   : CMN_referMsgBox
 *-----------------------------------------------------------------*/
/**
 * This function refers the status of the specified message box.
 * @param     msgBoxID    : ID of message box
 * @param     pRefMsgBox  : the pointer to the structure S_CMN_REF_MBX
 * @return    SUCCESS     (normally completion)
 * @return    ERR_INVID   (the ID is invalid)
 * @return    ERR_BADPARM (the input parameter is invalid)
 * @return    ERR_INVSTAT (the internal status of the object is invalid)
 * @return    ERR_NOOBJ   (the object don't exist)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CMN_referMsgBox(u8            msgBoxID,
                S_CMN_REF_MBX *pRefMsgBox)
{
    return ERR_INVID; // not supported.
}
