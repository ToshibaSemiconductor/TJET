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
 *  @file     cnl_task.c
 *
 *  @brief    CNL main task
 *
 *
 *  @note
 */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"
#include "cmn_dbg.h"

#include "oscmn.h"
#include "buscmn.h"

#include "cnl_type.h"
#include "cnl_err.h"
#include "cnl_if.h"
#include "cnl.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static T_CNL_ERR  CNL_actionHandler(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionInit(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionClose(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionWake(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionSleep(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionSendMngFrame(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionSendData(S_CNL_DEV *, S_CNL_ACTION *);
static void       CNL_compSendReq(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_execSendReq(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR  CNL_actionReceiveData(S_CNL_DEV *, S_CNL_ACTION *);


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/**
 * interfaces to CNL core.
 */
/*-------------------------------------------------------------------
 * Function : CNL_actionHandler
 *-----------------------------------------------------------------*/
/**
 * Action handler for CNL .
 * @param  pCnlDev     : the pointer to the S_CNL_DEV
 * @param  pActionList : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionHandler(S_CNL_DEV    *pCnlDev, 
                  S_CNL_ACTION *pActionList)
{

    T_CNL_ERR     retval = CNL_SUCCESS;
    S_CNL_ACTION *pAction;
    int i;

    for(i=0; i<CNL_ACTION_LIST_NUM; i++) {
        pAction = &pActionList[i];
        if(pAction->type == CNL_ACTION_NOP) {
            return CNL_SUCCESS;
        }

        DBG_INFO("HandleAction[%x].\n", pAction->type);

        switch(pAction->type) {
        case CNL_ACTION_INIT :
            retval = CNL_actionInit(pCnlDev, pAction);
            break;
        case CNL_ACTION_CLOSE :
            retval = CNL_actionClose(pCnlDev, pAction);
            break;
        case CNL_ACTION_WAKE :
            retval = CNL_actionWake(pCnlDev, pAction);
            break;
        case CNL_ACTION_SLEEP :
            retval = CNL_actionSleep(pCnlDev, pAction);
            break;
        case CNL_ACTION_SEND_MNG_FRAME :
            retval = CNL_actionSendMngFrame(pCnlDev, pAction);
            break;
        case CNL_ACTION_SEND_DATA :
            retval = CNL_actionSendData(pCnlDev, pAction);
            break;
        case CNL_ACTION_RECEIVE_DATA :
            retval = CNL_actionReceiveData(pCnlDev, pAction);
            break;

        case CNL_ACTION_HANDLE_COMPLETE :
        case CNL_ACTION_HANDLE_MNG_FRAME :
        case CNL_ACTION_HANDLE_TIMEOUT :
        case CNL_ACTION_HANDLE_UPPER_ORDER :
        case CNL_ACTION_HANDLE_EXTRA_EVENT :
        case CNL_ACTION_HANDLE_ERROR :
            goto HANDLER;

        default :
            DBG_ERR("Unknown Action[0x%x].\n", pAction->type);
            return CNL_ERR_BADPARM;
        }

        if(retval != CNL_SUCCESS) {
            // change Action type error to indicate.
            DBG_ERR("handling Action[%x] failed[%d]. convert to ActionHandleError.\n",
                    pAction->type, retval);
            pAction->type   = CNL_ACTION_HANDLE_ERROR;
            pAction->status = retval;
        }

HANDLER:
        if(IS_HANDLER_ACTION(pAction->type)) {
            // this action is fall through to the state machine.
            retval = CNL_stateMachine(pCnlDev, pAction);
        }
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_actionInit
 *-----------------------------------------------------------------*/
/**
 * Action.Init handler for CNL.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionInit(S_CNL_DEV    *pCnlDev, 
               S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;

    pReq = pAction->pReq;
    DBG_ASSERT(pReq != NULL);

    //
    // get device dependent parameter.
    //
    retval = pCnlDev->pDeviceOps->pGetDeviceParam(pCnlDev, &pCnlDev->deviceParam);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionInit : get device parameter failed[%d].\n", retval);
        return retval;
    }

    // Set Request Parameter ownUID
    CMN_MEMCPY(pCnlDev->deviceParam.ownUID, pReq->initReq.ownUID, CNL_UID_SIZE);

    //
    // exec CNL_INIT.request.
    //
    retval = pCnlDev->pDeviceOps->pInit(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionInit : CNL_INIT.request failed[%d].\n", retval);
        return retval;
    }

    // pre-set OWNUID.
    CMN_MEMCPY(pReq->initReq.ownUID, pCnlDev->deviceParam.ownUID, CNL_UID_SIZE);

    //
    // CNL_INIT.request completion will be notified by CNL_EVENT_COMP_REQUEST.
    // store current action.
    //
    pCnlDev->procAction = pAction->type;

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_actionClose
 *-----------------------------------------------------------------*/
/**
 * Action.Close handler for CNL.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionClose(S_CNL_DEV    *pCnlDev, 
                S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;

    pReq = pAction->pReq;
    DBG_ASSERT(pReq != NULL);

    //
    // exec CNL_CLOSE.request.
    //
    retval = pCnlDev->pDeviceOps->pClose(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionClose : CNL_CLOSE.request failed[%d].\n", retval);
        return retval;
    }

    //
    // CNL_CLOSE.request completion will be notified by CNL_EVENT_COMP_REQUEST.
    // store current action.
    //
    pCnlDev->procAction = pAction->type;

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_actionWake
 *-----------------------------------------------------------------*/
/**
 * Action.Wake handler for CNL .
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionWake(S_CNL_DEV    *pCnlDev, 
               S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;

    //
    // exec wake request(Dormant -> Awake).
    //
    retval = pCnlDev->pDeviceOps->pWake(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionWake : wake request failed[%d].\n", retval);
        return retval;
    }

    pCnlDev->pwrState = CNL_PWR_STATE_HIBERNATE_TO_AWAKE;

    //
    // wake request completion will be notified by CNL_EVENT_COMP_REQUEST
    // store current action.
    //
    pCnlDev->procAction = pAction->type;

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_actionSleep
 *-----------------------------------------------------------------*/
/**
 * Action.Sleep handler for CNL .
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionSleep(S_CNL_DEV    *pCnlDev, 
                S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;

    //
    // exec sleep request(Awake -> Dormant).
    //
    retval = pCnlDev->pDeviceOps->pSleep(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionSleep : sleep request failed[%d].\n", retval);
        return retval;
    }

    pCnlDev->pwrState = CNL_PWR_STATE_AWAKE_TO_HIBERNATE;

    //
    // sleep request completion will be notified by CNL_EVENT_COMP_REQUEST
    // store current action.
    //
    pCnlDev->procAction = pAction->type;

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_actionSendMngFrame
 *-----------------------------------------------------------------*/
/**
 * Action.SendMngFrame handler for CNL .
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionSendMngFrame(S_CNL_DEV    *pCnlDev, 
                       S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;
    u16             licc;
    void           *pTargetUID;
    void           *pLiccInfo;
    u8              liccInfo[CNL_LICC_INFO_SIZE];

    S_CNL_DEVICE_EVENT event;

    pReq = pAction->pReq;
    DBG_ASSERT(pReq != NULL);

    // convert request type to management frame informations.
    switch(pReq->type) {
    case CNL_REQ_TYPE_CONNECT_REQ :
        licc       = CNL_EXT_LICC_C_REQ;
        pTargetUID = (void *)((pReq->connectReq.specified == CNL_SPECIFIED_UID) ? \
                              pReq->connectReq.targetUID : NULL);
        pLiccInfo  = (void *)pReq->connectReq.pclParam;
        break;
    case CNL_REQ_TYPE_ACCEPT_REQ :
        licc       = CNL_EXT_LICC_C_ACC;
        pTargetUID = (void *)pReq->acceptReq.targetUID; // mandatory.
        pLiccInfo  = (void *)pReq->acceptReq.pclParam;
        break;
    case CNL_REQ_TYPE_ACCEPT_RES :
        licc       = CNL_EXT_LICC_ACK_FOR_C_ACC;
        pTargetUID = (void *)pReq->acceptRes.targetUID; // mandatory.
        pLiccInfo  = NULL;
        break;
    case CNL_REQ_TYPE_RELEASE_REQ :
        licc       = CNL_EXT_LICC_C_RLS;
        pTargetUID = NULL;
        pLiccInfo  = (void *)pReq->releaseReq.pclParam;
        break;
    case CNL_REQ_TYPE_POWERSAVE_REQ :
        CMN_MEMSET(&liccInfo, 0x00, CNL_LICC_INFO_SIZE);
        licc         = CNL_EXT_LICC_C_SLEEP;
        pTargetUID   = NULL;
        liccInfo[18] = pReq->powersaveReq.dormantPeriod;
        liccInfo[19] = pReq->powersaveReq.awakePeriod;
        pLiccInfo    = &liccInfo;
        break;
    case CNL_REQ_TYPE_WAKE_REQ :
        CMN_MEMSET(&liccInfo, 0x00, CNL_LICC_INFO_SIZE);
        licc         = CNL_EXT_LICC_C_WAKE;
        pTargetUID   = NULL;
        pLiccInfo    = &liccInfo;
        break;
    default :
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
    }

    //
    // exec CNL_XXXX.request/response
    //
    retval = pCnlDev->pDeviceOps->pSendMngFrame(pCnlDev, licc, pTargetUID, pLiccInfo);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionSendMngFrame : send managment frame request failed[%d].\n", retval);
        return retval;
    }

    //
    // CNL_XXXX.request/response completion will be notified by CNL_EVENT_COMP_REQUEST event.
    // store current action.
    //
    pCnlDev->procAction = pAction->type;

    CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
    //
    // avoid receiving mangement frame except C-Rls
    // during C-Sleep/C-Wake sending.
    //
    switch(pReq->type) {
    case CNL_REQ_TYPE_POWERSAVE_REQ :
    case CNL_REQ_TYPE_WAKE_REQ :
        event.recvdLicc = CNL_EXT_LICC_C_SLEEP | CNL_EXT_LICC_C_WAKE;
        CNL_setEventFilter(pCnlDev, &event);
        break;
    default :
        break;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_actionSendData
 *-----------------------------------------------------------------*/
/**
 * Action.SendData handler for CNL .
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionSendData(S_CNL_DEV    *pCnlDev, 
                   S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;


    //
    // check completion of send data request at first
    // if request(s) are completed, 
    // they are chained to compQueue of the Action.
    //
    if(pCnlDev->txComp) {
        CNL_compSendReq(pCnlDev, pAction);
        //
        // reset TX Comp flag.
        //
        pCnlDev->txComp = FALSE;
    }

    //
    // if request(s) are pending to send data, send it.
    //
    retval = CNL_execSendReq(pCnlDev, pAction);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Action Send Data : exec send req failed[%d].\n", retval);
        return retval;
    }

    // pAction->readyLength is updated in CNL_execSendReq.
    if(pAction->readyLength == 0) {
        //
        // all TX(CSDU) buffer is filled, reset TX Ready flag.
        //
        pCnlDev->txReady = FALSE;
    }

    //
    // if request(s) are chained to compQueue,
    // convert Action type to complete request if needed.
    //
    if(!CMN_LIST_IS_EMPTY(&pAction->compQueue)) {
        DBG_INFO("some send request(s) are completed.\n");
        pAction->type    = CNL_ACTION_HANDLE_COMPLETE;
        pAction->compReq = CNL_COMP_DATA_REQ;
        pAction->status  = CNL_SUCCESS;
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_compSendReq
 *-----------------------------------------------------------------*/
/**
 * check send data completion
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_compSendReq(S_CNL_DEV    *pCnlDev, 
                S_CNL_ACTION *pAction)
{

    S_CNL_CMN_REQ  *pReq;
    S_DATA_REQ_EXT *pExt;

    u8              compCsdu;
    u8              readyCsdu;
    u8              maxCsdu;
    u32             compLen;
    u32             rest;

    maxCsdu   = pCnlDev->deviceParam.txCsduNum;
    readyCsdu = LENGTH_TO_CSDU(pAction->readyLength);
    compCsdu  = pCnlDev->txSendingCsdu - (maxCsdu - readyCsdu);

    DBG_INFO("compSendReq : (sending=%u, ready=%u, complete=%u)\n",
             pCnlDev->txSendingCsdu, readyCsdu, compCsdu);

    pCnlDev->txSendingCsdu -= compCsdu;

    while(compCsdu > 0) {
        //
        // get Tx request head of Tx Queue.
        //
        pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->txQueue);
        if(pReq == NULL) {
            DBG_WARN("SendRequest is returned by ErrorHandler.\n");
            break;
        }

        DBG_ASSERT(pReq->state != CNL_REQ_QUEUED);

        pExt = (S_DATA_REQ_EXT *)(&pReq->extData);

        DBG_ASSERT(pReq->dataReq.length > pExt->compLen);

        rest = pReq->dataReq.length - pExt->compLen;

        //
        // update parameter of this request
        //
        compLen           = MIN(compCsdu * CNL_CSDU_SIZE, rest);
        pExt->compLen    += compLen;
        pExt->sendingLen -= compLen;

        if(pReq->dataReq.length > pExt->compLen) {
            // not completed. exit.
            break;
        } else {
            //
            // this request is completed.
            //
            DBG_INFO("one Tx request is completed.\n");
            DBG_ASSERT(compCsdu >= LENGTH_TO_CSDU(compLen));

            compCsdu -= LENGTH_TO_CSDU(compLen); // decrement completed CSDU count.

            // re-chain to compelete queue to complete request.
            CMN_lockCpu(pCnlDev->mngLockId);

            CNL_removeRequestFromTxQueue(pCnlDev, pReq);

            CMN_LIST_ADD_TAIL(&pAction->compQueue, pReq, S_CNL_CMN_REQ, list);
            pReq->status = CNL_SUCCESS;
            if(pReq->state == CNL_REQ_PROCESSING) {
                pReq->state = CNL_REQ_COMPLETED;
            }
            pReq->dataReq.length = pExt->compLen;
            CMN_unlockCpu(pCnlDev->mngLockId);
        }
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_execSendReq
 *-----------------------------------------------------------------*/
/**
 * execute send request
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_execSendReq(S_CNL_DEV    *pCnlDev, 
                S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;
    S_DATA_REQ_EXT *pExt;

    u8              sendCsdu   = 0;
    u32             rest;

    u32             length;
    void           *pDataPtr;
    u8              fragment;


    while(pAction->readyLength > 0) {
        //
        // get next send request.
        //
        CMN_lockCpu(pCnlDev->mngLockId);
        pReq = CNL_searchNextSendRequest(pCnlDev);
        if((pReq != NULL) && (pReq->state == CNL_REQ_QUEUED)) {
            pReq->state = CNL_REQ_PROCESSING;
        }
        CMN_unlockCpu(pCnlDev->mngLockId);

        if(pReq == NULL) {
            // no more request.
            break;
        }

        pExt = (S_DATA_REQ_EXT *)(&pReq->extData);
        rest = pReq->dataReq.length - pExt->position;

        DBG_ASSERT(pReq->dataReq.length > pExt->position);

        //
        // calculate send data length and pointer.
        //
        length   = MIN(pAction->readyLength, rest);
        sendCsdu = LENGTH_TO_CSDU(length);
        pDataPtr = (void *)((u8 *)pReq->dataReq.pData + pExt->position);

        fragment = (pReq->dataReq.length > pExt->position + length) ? \
            CNL_FRAGMENTED_DATA : pReq->dataReq.fragmented;

        DBG_INFO("execSendReq : call sendData(length=%u(%u CSDU(s)), fragment=%u, pid=%u\n",
                 length, sendCsdu, fragment, pReq->dataReq.profileId);


        //
        // call Send Data device operation.
        //
        retval = pCnlDev->pDeviceOps->pSendData(pCnlDev,
                                                pReq->dataReq.profileId,
                                                fragment,
                                                length,
                                                pDataPtr);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendData failed[%d].\n", retval);
            return retval;
        }
            
        // update information.
        pExt->sendingLen       += length;
        pExt->position         += length;
        pAction->readyLength   -= sendCsdu * CNL_CSDU_SIZE; // not length.
        pCnlDev->txSendingCsdu += sendCsdu;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_actionReceiveData
 *-----------------------------------------------------------------*/
/**
 * Action.ReceiveData handler for CNL .
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_actionReceiveData(S_CNL_DEV    *pCnlDev, 
                      S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;
    S_DATA_REQ_EXT *pExt;
    void           *pDataPtr;

    u32             length;
    u8              profileId;
    u8              fragment;
    S_LIST         *pHead;

    profileId = pCnlDev->rxReadyPid;

    while(pAction->readyLength > 0) {
        //
        // get receive data request from RX queue(Pid 0 or 1)
        // 
        pHead = (profileId == CNL_PROFILE_ID_0) ? &pCnlDev->rx0Queue : &pCnlDev->rx1Queue;
        CMN_lockCpu(pCnlDev->mngLockId);
        pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(pHead);
        if((pReq != NULL) && (pReq->state == CNL_REQ_QUEUED)) {
            pReq->state = CNL_REQ_PROCESSING;
        }

        CMN_unlockCpu(pCnlDev->mngLockId);

        if(pReq == NULL) {
            // no more RX request for this PID.
            break;
        }

        pExt     = (S_DATA_REQ_EXT *)(&pReq->extData);

        //
        // exec receive data.
        //
        fragment = 0;
        pDataPtr = (void *)((u8 *)pReq->dataReq.pData + pExt->position);
        length   = MIN(pAction->readyLength, pReq->dataReq.length - pExt->position);

        DBG_INFO("RecvDump(pos=%u, total=%u, rest=%u\n",
                 pExt->position, pReq->dataReq.length, length);

        //
        // read data. (pDataPtr, length)
        //
        retval = pCnlDev->pDeviceOps->pReceiveData(pCnlDev,
                                                   pReq->dataReq.profileId,
                                                   &fragment,
                                                   &length,
                                                   pDataPtr);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReceiveData failed[%d].\n", retval);
            return retval;
        }

        pExt->position       += length;
        pAction->readyLength -= length;

        DBG_INFO("RecvDumpAfter(pos=%u, total=%u, recvd=%u, frag=%d\n",
                 pExt->position, pReq->dataReq.length, length, fragment);

        if((pExt->position >= pReq->dataReq.length) ||
           (fragment       == CNL_NOT_FRAGMENTED_DATA)) {

            // case of discard request 
            if ((pReq->id == CNL_DISCARD_RECVDATA_0) || (pReq->id == CNL_DISCARD_RECVDATA_1)) {
                pExt->position = 0;
                continue;
            }

            //
            // one request completed.
            //
            CMN_lockCpu(pCnlDev->mngLockId);

            CNL_removeRequestFromRxQueue(pCnlDev, pReq);

            CMN_LIST_ADD_TAIL(&pAction->compQueue, pReq, S_CNL_CMN_REQ, list);
            CMN_unlockCpu(pCnlDev->mngLockId);
            pReq->status             = CNL_SUCCESS;
            if(pReq->state == CNL_REQ_PROCESSING) {
                pReq->state = CNL_REQ_COMPLETED;
            }
            pReq->dataReq.fragmented = fragment;
            pReq->dataReq.length     = pExt->position;
        }
    }


    if(pAction->readyLength == 0) {
        // all data is read, set rxReady FALSE
        pCnlDev->rxReady    = FALSE;
        pCnlDev->rxReadyPid = CNL_EMPTY_PID;
    }

    if(!CMN_LIST_IS_EMPTY(&pAction->compQueue)) {
        DBG_INFO("some request(s) are completed.\n");
        pAction->type    = CNL_ACTION_HANDLE_COMPLETE;
        pAction->compReq = CNL_COMP_DATA_REQ;
        pAction->status  = CNL_SUCCESS;
        return CNL_SUCCESS;
    }

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_task
 *-----------------------------------------------------------------*/
/**
 * CNL main thread.
 * @param  pUnuse : unused parameter.
 * @return 
 * @note   
 */
/*-----------------------------------------------------------------*/
void
CNL_task(void *pUnuse) 
{

    T_CMN_ERR     retval;
    S_CNL_DEV    *pCnlDev = NULL;
    S_CNL_ACTION  actionList[CNL_ACTION_LIST_NUM];

    DBG_INFO("CNL core task started\n");

    do {
        //
        // get signaled device.
        //
        retval = CNL_getSignaledDev(&pCnlDev);
        if(retval == ERR_RLWAIT) {
            // terminate task is called or system interrupted.
            break; // exit task.
        }

        do {
            //
            // get Action from scheduler.
            // handle action coutinuously while action exists in one device.
            //
            // -- note --
            // this may cause task occupy when multi-device is supported.
            // consider priority management if necessary.
            //
            retval = CNL_getAction(pCnlDev, actionList);

            if((actionList[0].type == CNL_ACTION_NOP) && (pCnlDev->event.type == 0)) {
                DBG_INFO("no more action is pending to this device.\n");
                // wait for a new device signaled
                break;
            }

            //
            // execute action.
            //
            retval = CNL_actionHandler(pCnlDev, actionList);
            if(retval != 0) {
                DBG_ERR("handle action failed.\n");
                // fall through.
            }
        } while(1);
    } while (1);

    DBG_INFO("CNL core thread exited\n");

    return;

}
