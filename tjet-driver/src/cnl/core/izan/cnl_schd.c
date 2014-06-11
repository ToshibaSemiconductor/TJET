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
 *  @file     cnl_schd.c
 *
 *  @brief    CNL scheduler.
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

#include "cnl_type.h"
#include "cnl_err.h"
#include "cnl_if.h"
#include "cnl.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define CNL_CANCEL_TOUT_VAL_MS    5000   /* 5000ms */


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
typedef struct tagS_CNL_CANCEL_TOUT_ARG {
    S_CNL_DEV      *pCnlDev;
    S_CNL_CMN_REQ  *pDummy;
}S_CNL_CANCEL_TOUT_ARG;


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
// scheduling function.
static void CNL_checkCancel(S_CNL_DEV *pCnlDev);
static void CNL_checkInitClose(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_checkEvent(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_checkCtrlRequest(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_checkRxRequest(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_checkTxRequest(S_CNL_DEV *, S_CNL_ACTION *);

// convert event to action function.
static void CNL_deviceRemovedEventToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_wakeRequiredEventToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_errorEventToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_timeoutEventToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_compReqEventToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_recvdMngFrameEventToAction(S_CNL_DEV *, S_CNL_ACTION *);

// convert request to action function.
static void CNL_initReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_closeReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_connectReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_wconnectReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_acceptReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_acceptResToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_releaseReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_sendDataReqToAction(S_CNL_DEV *, S_CNL_ACTION  *);
static void CNL_recvDataReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION  *);
static void CNL_reqTsLhToAction(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_checkSleep(S_CNL_DEV *, S_CNL_ACTION *);
static void CNL_regDataIndToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_unregDataIndToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);
static void CNL_getStatsToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION  *);
static void CNL_regPassthroughToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION  *);
static void CNL_powersaveReqToAction(S_CNL_DEV *, S_CNL_CMN_REQ *, S_CNL_ACTION *);

static void CNL_cancelToutCallback(unsigned long);

/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
extern u8 g_rssi;

static S_CNL_CANCEL_TOUT_ARG g_cnlCancelToutArg = {NULL, NULL};

/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/
static inline void 
CNL_initAction(S_CNL_ACTION *pAction)
{
    DBG_ASSERT(pAction != NULL);
    pAction->type        = CNL_ACTION_NOP;
    pAction->pReq        = NULL;
    pAction->compReq     = CNL_COMP_NULL;
    pAction->timer       = 0;
    pAction->licc        = 0;
    pAction->profileId   = 0;
    pAction->order       = 0;
    pAction->readyLength = 0;
    pAction->status      = 0;
    pAction->extra       = 0;
    CMN_LIST_INIT(&pAction->compQueue);
    return;
}


static inline void
CNL_setCtrlReqCompAction(S_CNL_DEV     *pCnlDev,
                         S_CNL_CMN_REQ *pReq,
                         T_CNL_ERR      status,
                         S_CNL_ACTION  *pAction)
{
    // call with holding manage lock.
    pAction->type    = CNL_ACTION_HANDLE_COMPLETE;
    CNL_removeRequestFromCtrlQueue(pCnlDev, pReq);
    pAction->pReq    = pReq;
    pAction->compReq = CNL_COMP_NULL;
    pReq->status     = status;
    return;
}


static inline void
CNL_setUnexpectedEventAction(S_CNL_ACTION *pAction)
{
    pAction->type   = CNL_ACTION_HANDLE_ERROR;
    pAction->status = CNL_ERR_UNEXP_EVENT;
    return;
}


static inline u8
CNL_needDataInd(S_CNL_DEV *pCnlDev,
                u8         profileId)
{
    if((pCnlDev->recvCbk[profileId].pCbk != NULL) &&
       (pCnlDev->recvCbk[profileId].call == TRUE)) {
        pCnlDev->recvCbk[profileId].call = FALSE;
        return TRUE;
    }
    return FALSE;
}


static inline void
CNL_resetDataInd(S_CNL_DEV *pCnlDev,
                 u8         profileId)
{
    if((pCnlDev->recvCbk[profileId].pCbk != NULL) &&
       (pCnlDev->recvCbk[profileId].call == FALSE)) {
        pCnlDev->recvCbk[profileId].call = TRUE;
    }
    return;
}


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*=================================================================*/
/* Schedule main checker                                           */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_checkCancel
 *-----------------------------------------------------------------*/
/**
 * cancel a processing request.
 * @param   pCnlDev : the pointer to the CNL device
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkCancel(S_CNL_DEV *pCnlDev)
{

    S_CNL_CMN_REQ *pReq   = NULL;
    S_CNL_CMN_REQ *pDummy = NULL;

    CMN_lockCpu(pCnlDev->mngLockId);

    pDummy = CMN_LIST_REMOVE_HEAD(&pCnlDev->cancelQueue, S_CNL_CMN_REQ, list);
    if(pDummy == NULL) {
        // cancel queue is empty, do nothing.
        CMN_unlockCpu(pCnlDev->mngLockId);
        return;
    }

    DBG_INFO("CancelRequest target Id is 0x%x\n", pDummy->id);

    //
    // search target request from request queues.
    //
    pReq = CNL_searchCancelRequest(pCnlDev, pDummy->id);
    if(pReq == NULL) {
        DBG_INFO("target request is not found.\n");
        pCnlDev->cancelStatus = ERR_NOOBJ;
        goto EXIT;
    }

    //
    // target request is found in queue, cancel it.
    //
    switch(pReq->state) {
    case CNL_REQ_QUEUED :
        DBG_INFO("target request is only queued, complete immediately.\n");

        switch(pReq->type) {
        case CNL_REQ_TYPE_RECEIVE_REQ :
            CNL_removeRequestFromRxQueue(pCnlDev, pReq);
            break;

        case CNL_REQ_TYPE_SEND_REQ :
            CNL_removeRequestFromTxQueue(pCnlDev, pReq);
            break;

        default :
            // cancel request is assumed to be used only data request.
            CNL_removeRequestFromCtrlQueue(pCnlDev, pReq);
            break;
        }
        pCnlDev->cancelStatus = SUCCESS;
        break;

    case CNL_REQ_PROCESSING :
        DBG_INFO("target request is processing, swap to dummy request.\n");

        switch(pReq->type) {
        case CNL_REQ_TYPE_RECEIVE_REQ :
            // normally cancel request.
            CNL_removeRequestFromRxQueue(pCnlDev, pReq);
            break;

        case CNL_REQ_TYPE_SEND_REQ :
            // swap to dummy request.
            CMN_MEMCPY(pDummy, pReq, sizeof(S_CNL_CMN_REQ));
            pDummy->state = CNL_REQ_CANCELLING;

            // set length to current position not to send data any more.
            pDummy->dataReq.length = 
                ((S_DATA_REQ_EXT *)&pReq->extData)->position;
            CMN_LIST_SWAP_ELEM(&pCnlDev->txQueue, pReq, pDummy, S_CNL_CMN_REQ, list);

            // release dummy request when completed.
            pDummy = NULL;

            break;

        default :
            // cancel request is assumed to be used only data request.
            CMN_MEMCPY(pDummy, pReq, sizeof(S_CNL_CMN_REQ));
            pDummy->state = CNL_REQ_CANCELLING;
            CMN_LIST_SWAP_ELEM(&pCnlDev->ctrlQueue, pReq, pDummy, S_CNL_CMN_REQ, list);
            pDummy = NULL;
            break;
        }
        pCnlDev->cancelStatus = SUCCESS;
        break;

    default :
        pReq = NULL; // cancelling cancelled request. ignore.
        pCnlDev->cancelStatus = ERR_NOOBJ;
        break;
    }

EXIT:
    CMN_unlockCpu(pCnlDev->mngLockId);
    
    if(pReq) {
        pReq->status = CNL_ERR_CANCELLED;
        CNL_completeRequest(pCnlDev, pReq);
    }

    if(pDummy) {
        CMN_releaseFixedMemPool(pCnlDev->dummyReqMplId, pDummy);
    }

    CMN_REL_WAIT(pCnlDev->cancelWaitId);
        
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkInitClose
 *-----------------------------------------------------------------*/
/**
 * check CNL init/close request is ready.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   if Action is generated, Action type indicate it.
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkInitClose(S_CNL_DEV    *pCnlDev, 
                   S_CNL_ACTION *pAction)
{

    S_CNL_CMN_REQ *pReq;

    CMN_lockCpu(pCnlDev->mngLockId);

    //
    // check Init/Close request is queued.
    //
    pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->ctrlQueue);
    if((pReq == NULL) || (pReq->state != CNL_REQ_QUEUED)) {
        // no control request queued. or already started processing.
        CMN_unlockCpu(pCnlDev->mngLockId);
        return;
    }

    //
    // Init/Close request is pending.
    // if CNL state is valid, generate Init/Close action.
    //
    switch(pReq->type) {
    case CNL_REQ_TYPE_INIT_REQ :
        CNL_initReqToAction(pCnlDev, pReq, pAction);
        break;            
    case CNL_REQ_TYPE_CLOSE_REQ :
        CNL_closeReqToAction(pCnlDev, pReq, pAction);
        break;
    default :
        // other request.(send management frame etc.), do not handle here.
        break;
    }

    CMN_unlockCpu(pCnlDev->mngLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkEvent
 *-----------------------------------------------------------------*/
/**
 * check interrupt status.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   if Action is generated, Action type indicate it.
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkEvent(S_CNL_DEV    *pCnlDev, 
               S_CNL_ACTION *pAction)
{

    T_CNL_EVENT    bit = 0;

    CMN_lockCpu(pCnlDev->mngLockId);
    bit = GET_PRIOR_BIT(pCnlDev->event.type & ~(pCnlDev->eventFilter.type));

    //
    // one Event to one Action.
    //
    switch(bit) {
    case CNL_EVENT_DEVICE_REMOVED: 
        CNL_deviceRemovedEventToAction(pCnlDev, pAction);
        break;
    case CNL_EVENT_WAKE_REQUIRED : 
        CNL_wakeRequiredEventToAction(pCnlDev, pAction);
        break;
    case CNL_EVENT_ERROR_OCCURRED :
        CNL_errorEventToAction(pCnlDev, pAction);
        break;
    case CNL_EVENT_COMP_REQUEST :
        CNL_compReqEventToAction(pCnlDev, pAction);
        break;
    case CNL_EVENT_TIMEOUT :
        CNL_timeoutEventToAction(pCnlDev, pAction);
        break;
    case CNL_EVENT_RECVD_MNG_FRAME :
        CNL_recvdMngFrameEventToAction(pCnlDev, pAction);
        break;
    default : 
        break;
    }

    if(pAction->type == CNL_ACTION_NOP) {
        //
        // Action is not generated.
        // check DATA TX/RX events, at same time.
        // this check generate no Action except for special case
        //
        if(pCnlDev->event.type & CNL_EVENT_RX_READY) {
            //
            // RX Ready.
            //
            pCnlDev->rxReady     = TRUE;
            CLEAR_BIT(pCnlDev->event.type, CNL_EVENT_RX_READY);

            // 
            // special case :
            // if RESPONDER_RESPONSE state, the ACK for C-ACC.
            // generate Action.HandleMngFrame(ACK for C-ACC) for changing state to CONNECTED.
            // 
            if(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_RESPONDER_RESPONSE) {
                pCnlDev->missCaccAck      = TRUE;
                pCnlDev->event.type      |= CNL_EVENT_RECVD_MNG_FRAME;
                pCnlDev->event.recvdLicc |= CNL_EXT_LICC_ACK_FOR_C_ACC;
                CNL_recvdMngFrameEventToAction(pCnlDev, pAction);
            }
        }
        if(pCnlDev->event.type & CNL_EVENT_TX_READY) {
            //
            // TX Ready
            // 
            pCnlDev->txReady     = TRUE;
            CLEAR_BIT(pCnlDev->event.type, CNL_EVENT_TX_READY);
        }
        if(pCnlDev->event.type & CNL_EVENT_TX_COMP) {
            //
            // TX Completed.
            //
            pCnlDev->txComp      = TRUE;
            CLEAR_BIT(pCnlDev->event.type, CNL_EVENT_TX_COMP);

        }
    }

    CMN_unlockCpu(pCnlDev->mngLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkCtrlRequest
 *-----------------------------------------------------------------*/
/**
 * check request.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   if Action is generated, Action type indicate it.
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkCtrlRequest(S_CNL_DEV    *pCnlDev, 
                     S_CNL_ACTION *pAction)
{

    S_CNL_CMN_REQ *pReq;

    CMN_lockCpu(pCnlDev->mngLockId);

    pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->ctrlQueue);
    if((pReq == NULL) || (pReq->state == CNL_REQ_PROCESSING)){
        // no ctrl request queued or processing.
        CMN_unlockCpu(pCnlDev->mngLockId);
        return;
    }

    switch(pReq->type) {
    case CNL_REQ_TYPE_CONNECT_REQ :
        CNL_connectReqToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_WAIT_CONNECT_REQ :
        CNL_wconnectReqToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_ACCEPT_REQ :
        CNL_acceptReqToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_ACCEPT_RES :
        CNL_acceptResToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_RELEASE_REQ :
        CNL_releaseReqToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_REG_DATA_IND :
        CNL_regDataIndToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_UNREG_DATA_IND :
        CNL_unregDataIndToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_GET_STATS :
        CNL_getStatsToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_REG_PASSTHROUGH :
        CNL_regPassthroughToAction(pCnlDev, pReq, pAction);
        break;
    case CNL_REQ_TYPE_POWERSAVE_REQ:
        CNL_powersaveReqToAction(pCnlDev, pReq, pAction);
        break;
    default :
        DBG_ASSERT(0);
        break;
    }

    CMN_unlockCpu(pCnlDev->mngLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkRxRequest
 *-----------------------------------------------------------------*/
/**
 * check Rx Data request..
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkRxRequest(S_CNL_DEV    *pCnlDev, 
                   S_CNL_ACTION *pAction)
{

    T_CNL_ERR       retval;
    S_CNL_CMN_REQ  *pReq;
    u8              profileId;
    S_LIST         *pHead;


    //
    // check RX ready ProfileId.
    //
    if((pCnlDev->rxReady) && (pCnlDev->rxReadyPid == CNL_EMPTY_PID)) {
        retval = pCnlDev->pDeviceOps->pReadReadyPid(pCnlDev, &profileId);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("read ready PID information failed.\n");
            pAction->type   = CNL_ACTION_HANDLE_ERROR;
            pAction->status = retval;
            return;
        }
        pCnlDev->rxReadyPid  = profileId;
        DBG_ASSERT((profileId == 0) || (profileId == 1));
    }

    // 
    // search RX request have same ProfileId from RX queue.
    //
    pHead = (pCnlDev->rxReadyPid == CNL_PROFILE_ID_0) ? &pCnlDev->rx0Queue : &pCnlDev->rx1Queue;
    pReq  = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(pHead);

    //
    // check RX criteria.
    // - ((rxReady) && (pReq != NULL))
    //
    // DataInd criteria.
    // - ((rxReady) && (pReq == NULL))
    //
    if(pCnlDev->rxReady) {
        // RX CSDU is not ready.
        DBG_INFO("RX criteria is satisfied, generate Action.\n");
        CNL_recvDataReqToAction(pCnlDev, pReq, pAction);
    }

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_checkTxReuqst
 *-----------------------------------------------------------------*/
/**
 * check Tx data request.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkTxRequest(S_CNL_DEV    *pCnlDev, 
                   S_CNL_ACTION *pAction)
{

    S_CNL_CMN_REQ  *pHeadReq, *pNextReq;
    u8              subState;

    
    pHeadReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->txQueue);
    CMN_lockCpu(pCnlDev->mngLockId);
    pNextReq = CNL_searchNextSendRequest(pCnlDev);
    CMN_unlockCpu(pCnlDev->mngLockId);
    subState = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    //
    // check TX criteria.
    // txReady indicates exisiting empty TX(CSDU) buffer.
    // txComp indicates completing TX DataFrame(s).
    //
    // - (txReady) && (pNextReq != NULL) 
    // or 
    // - (txComp)
    // or 
    // - (pHeadReq != NULL) && (subState == TARGET_SLEEP or LOCAL_HIBERNATE)
    //
    if((pCnlDev->txComp) ||
       ((pCnlDev->txReady) && (pNextReq != NULL)) ||
       ((pHeadReq != NULL) && 
        ((subState == CNL_SUBSTATE_TARGET_SLEEP) || (subState == CNL_SUBSTATE_LOCAL_HIBERNATE)))) {
        DBG_INFO("TX criteria is satisfied, generate Action.\n");
        CNL_sendDataReqToAction(pCnlDev, pAction);
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkSleep
 *-----------------------------------------------------------------*/
/**
 * check sleep is ready(go hibernate)
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   if Action is generated, Action type indicate it.
 */
/*-----------------------------------------------------------------*/
static void
CNL_checkSleep(S_CNL_DEV    *pCnlDev, 
               S_CNL_ACTION *pAction)
{

    u8 mainState;
    u8 subState;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    //
    // terminal action generator.
    // - TARGET_SLEEP           : Action.Order(POWERSAVE)
    // - LOCAL_HIBERNATE(AWAKE) : Action.Sleep
    // - SEARCH(AWAKE)          : Action.Sleep(current disable)
    //
    if(subState == CNL_SUBSTATE_TARGET_SLEEP) {
        pAction->type  = CNL_ACTION_HANDLE_UPPER_ORDER;
        pAction->order = CNL_REQ_TYPE_POWERSAVE_REQ;
        return;
    }

    if((subState          == CNL_SUBSTATE_LOCAL_HIBERNATE) && 
       (pCnlDev->pwrState == CNL_PWR_STATE_AWAKE)) {
        pAction->type = CNL_ACTION_SLEEP;
        return;
    }

    if((mainState         == CNL_STATE_SEARCH) && 
       (pCnlDev->pwrState == CNL_PWR_STATE_AWAKE)) {
        pAction->type = CNL_ACTION_SLEEP;
        return;
    }

    return;
}


/*=================================================================*/
/* Action generator for EVENT                                      */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_deviceRemovedEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert DeviceRemoved event to Action.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void 
CNL_deviceRemovedEventToAction(S_CNL_DEV        *pCnlDev,
                               S_CNL_ACTION     *pAction)
{

    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;

    //
    // force generate Action.HandleExtraEvent
    //
    pAction->type  = CNL_ACTION_HANDLE_EXTRA_EVENT;
    pAction->extra = CNL_EXTRA_DEVICE_REMOVED;

    CLEAR_BIT(pEvent->type, CNL_EVENT_DEVICE_REMOVED);

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_wakeRequiredEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert WAKE_REQUIRE event to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void 
CNL_wakeRequiredEventToAction(S_CNL_DEV        *pCnlDev,
                              S_CNL_ACTION     *pAction)
{

    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;

    switch(pCnlDev->pwrState) {
    case CNL_PWR_STATE_HIBERNATE :
        // generate Action.Wake
        pAction->type  = CNL_ACTION_WAKE;
        break;
    case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
        // Action.Wake is already generated, do nothing.
        pAction->type  = CNL_ACTION_NOP;
        break;
    default :
        DBG_ERR("WakeRquired event raised in PWR_STATE[%x].\n", pCnlDev->pwrState);
        DBG_ASSERT(0);
        CNL_setUnexpectedEventAction(pAction);
        break;
    }

    // clear wake required event bit.(always)
    CLEAR_BIT(pEvent->type, CNL_EVENT_WAKE_REQUIRED);

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_errorEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert ERROR event to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_errorEventToAction(S_CNL_DEV        *pCnlDev,
                       S_CNL_ACTION     *pAction)
{

    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;

    // generate Action.Error
    pAction->type        = CNL_ACTION_HANDLE_ERROR;
    pAction->status      = pEvent->error; 

    // clear error event bit and error code(always)
    pEvent->error        = 0;
    CLEAR_BIT(pEvent->type, CNL_EVENT_ERROR_OCCURRED);
    
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_timeoutEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert TIMEOUT event to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void 
CNL_timeoutEventToAction(S_CNL_DEV        *pCnlDev,
                         S_CNL_ACTION     *pAction)
{

    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;
    u8                  mainState;
    u8                  subState;
    u16                 bit;
    u8                  valid = FALSE;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);
    bit = GET_PRIOR_BIT(pEvent->timer & ~(pCnlDev->eventFilter.timer));

    switch(bit) {
    case CNL_T_CONNECT :
        if(mainState == CNL_STATE_CONNECTION_REQUEST) {
            valid = TRUE;
        } else {
            DBG_ERR("T_Connect expired in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_T_ACCEPT :
        if(mainState == CNL_STATE_RESPONDER_RESPONSE) {
            valid = TRUE;
        } else {
            DBG_ERR("T_Accept expired in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_T_RETRY :
        if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
           (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
            valid = TRUE;
        } else {
            DBG_ERR("T_Retry expired in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_T_RESEND :
        if(((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
            (mainState == CNL_STATE_RESPONDER_CONNECTED)) &&
           (subState == CNL_SUBSTATE_CONNECTED)) {
            valid = TRUE;
        } else {
            DBG_ERR("T_Resend expired in invalid state[%x:%x].\n", 
                    mainState, subState);
        }
        break;

    case CNL_T_KEEPALIVE : // no use.
    default :
        goto EXIT;
    }
    
    // generate Action.Timeout
    if(valid) {
        // check power state.
        switch(pCnlDev->pwrState) {
        case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
        case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
            DBG_INFO("power state is changing, skip handling timer event.\n");
            pAction->type = CNL_ACTION_NOP;
            goto EXIT;

        case CNL_PWR_STATE_HIBERNATE :
            DBG_INFO("power state is hibernate, wake first.\n");
            pAction->type = CNL_ACTION_WAKE;
            goto EXIT;

        case CNL_PWR_STATE_AWAKE :
        default :
            break;
        }

        //
        // generate Action.timeout
        //
        pAction->type  = CNL_ACTION_HANDLE_TIMEOUT;
        pAction->timer = bit;
    } else {
        CNL_setUnexpectedEventAction(pAction);
    }

    // clear timer bit.
    CLEAR_BIT(pEvent->timer, bit);
    if(pEvent->timer == 0) {
        // no more timer event, clear timer event bit
        CLEAR_BIT(pEvent->type, CNL_EVENT_TIMEOUT);
    }

EXIT :

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_compReqEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert COMP_REQUEST event to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_compReqEventToAction(S_CNL_DEV        *pCnlDev,
                         S_CNL_ACTION     *pAction)
{

    S_CNL_CMN_REQ      *pReq;
    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;
    u8                  mainState;
    u8                  subState;
    u16                 bit;
    u8                  valid = FALSE;
    u8                  internalSleep = FALSE;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    bit = GET_PRIOR_BIT(pEvent->compReq & ~(pCnlDev->eventFilter.compReq));

    switch(bit) {
    case CNL_COMP_INIT :
        if(mainState == CNL_STATE_CLOSE) {
            valid = TRUE;
        } else {
            DBG_ERR("Init Completed in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_COMP_CLOSE :
        if(mainState != CNL_STATE_CLOSE) {
            valid = TRUE;
        } else {
            DBG_ERR("Close Completed in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_COMP_CONNECT_REQ :
        if(mainState == CNL_STATE_CONNECTION_REQUEST) {
            valid = TRUE;
        } else {
            DBG_ERR("ConnectReq completed in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_COMP_ACCEPT_REQ :
        if(mainState == CNL_STATE_RESPONDER_RESPONSE) {
            valid = TRUE;
        } else {
            DBG_ERR("AcceptReq completed in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_COMP_ACCEPT_RES :
        if(mainState == CNL_STATE_RESPONSE_WAITING) {
            valid = TRUE;
        } else {
            DBG_ERR("AcceptRes completed in invalid state[%x].\n", mainState);
        }
         break;

    case CNL_COMP_RELEASE_REQ :
        if((mainState != CNL_STATE_RESPONDER_RESPONSE) &&
           (mainState != CNL_STATE_SEARCH) &&
           (mainState != CNL_STATE_CLOSE)) {
            valid = TRUE;
        } else {
            DBG_ERR("ReleaseReq completed in invalid state[%x].\n", mainState);
        }
        break;

    case CNL_COMP_SLEEP_REQ :
        if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
           (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
            valid = TRUE;
        } else {
            DBG_ERR("SleepReq completed in invalid state[%x:%x].\n", mainState, subState);
        }
        break;

    case CNL_COMP_WAKE_REQ :
        if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
           (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
            valid = TRUE;
        } else {
            DBG_ERR("WakeReq completed in invalid state[%x:%x].\n", mainState, subState);
        }
        break;

    case CNL_COMP_PROBE_REQ :
            if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
               (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
                valid = TRUE;

                //
                // special case.
                // if C-Probe completed in CONNECTED state, and no Ctrl/TX request queued,
                // send C-Sleep to enter LocalHibernate state.
                // C-Probe completion is not effect to state.
                //
                if((CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState) == CNL_SUBSTATE_CONNECTED) &&
                   (pCnlDev->ctrlReqCnt == 0) && (pCnlDev->txReqCnt == 0)) {
                    pAction->type        = CNL_ACTION_SEND_MNG_FRAME;
                    pAction->pReq        = &pCnlDev->pwrReq;
                    pCnlDev->pwrReq.type = CNL_REQ_TYPE_POWERSAVE_REQ;
                    pCnlDev->pwrReq.powersaveReq.dormantPeriod = 
                        CNL_TDC_TO_DORMANT_PERIOD(pCnlDev->deviceParam.tdc);
                    pCnlDev->pwrReq.powersaveReq.awakePeriod =
                        CNL_TAC_TO_AWAKE_PERIOD(pCnlDev->deviceParam.tac);

                    DBG_INFO("AwakePeriod = %u, DormantPeriod = %u\n",
                             pCnlDev->pwrReq.powersaveReq.awakePeriod,
                             pCnlDev->pwrReq.powersaveReq.dormantPeriod);

                } else {
                    // C-Probe completed but sub state is not CONNECTED or request is queued, 
                    // do nothing.
                    pAction->type = CNL_ACTION_NOP;
                }
                goto EXIT;

            } else {
                DBG_ERR("ProbeReq completed in invalid state[%x].\n", mainState);
            }
        break;

    case CNL_COMP_WAKE :
    case CNL_COMP_SLEEP :
        valid = TRUE;
        break;

    default :
        goto EXIT_2;
    }

    // generate Action.Complete
    if(valid) {
        // check power state.
        switch(pCnlDev->pwrState) {
        case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
            if(bit != CNL_COMP_SLEEP) {
                DBG_INFO("power state is changing, skip handling complete event.\n");
                pAction->type = CNL_ACTION_NOP;
                goto EXIT_2;
            }
            break;
        case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
            if(bit != CNL_COMP_WAKE) {
                DBG_INFO("power state is changing, skip handling complete event.\n");
                pAction->type = CNL_ACTION_NOP;
                goto EXIT_2;
            }
            break;

        case CNL_PWR_STATE_HIBERNATE:
            // invalid case.
            DBG_INFO("power state is hibernate, wake first.\n");
            pAction->type = CNL_ACTION_WAKE;
            goto EXIT_2;
            
        case CNL_PWR_STATE_AWAKE :
        default :
            break;
        }

        //
        // generate Action.Complete
        //
        pAction->type    = CNL_ACTION_HANDLE_COMPLETE;
        pAction->compReq = bit;

        // if request from upper layer, delete from ctrl queue.
        if(!IS_INTERNAL_REQ(pAction->compReq)) {
            pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->ctrlQueue);
            // check for internal Sleep.Req
            if (bit == CNL_COMP_SLEEP_REQ) {
                if (pReq == NULL) {
                    internalSleep = TRUE;
                } else if (pReq->type != CNL_REQ_TYPE_POWERSAVE_REQ) {
                    internalSleep = TRUE;
                }
            }
            if (internalSleep == FALSE) {
                if(pReq == NULL) {
                    DBG_WARN("CtrlRequest is returned by ErrorHandler.\n");
                    // force to Nop
                    pAction->type  = CNL_ACTION_NOP;
                } else {
                    CNL_removeRequestFromCtrlQueue(pCnlDev, pReq);
                    DBG_ASSERT(pReq->state != CNL_REQ_QUEUED);
                    pAction->pReq  = pReq;
                    pReq->status   = CNL_SUCCESS;
                    pReq->state    = CNL_REQ_COMPLETED;
                }
            }
        }
    } else {
        CNL_setUnexpectedEventAction(pAction);
    }

EXIT:
    // clear complete request bit.
    CLEAR_BIT(pEvent->compReq, bit);
    if(pEvent->compReq == 0) {
         // no more complete request event, clear event bit.
        CLEAR_BIT(pEvent->type, CNL_EVENT_COMP_REQUEST);
    }

EXIT_2:

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_recvdMngFrameEventToAction
 *-----------------------------------------------------------------*/
/**
 * convert RECVED_MNG_FRAME event to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void 
CNL_recvdMngFrameEventToAction(S_CNL_DEV        *pCnlDev,
                               S_CNL_ACTION     *pAction)
{

    T_CNL_ERR           retval;
    S_CNL_DEVICE_EVENT *pEvent = &pCnlDev->event;
    u8                  mainState;
    u16                 bit;
    u8                  valid = FALSE;
    u8                  skip  = FALSE;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    bit = GET_PRIOR_BIT(pEvent->recvdLicc & ~(pCnlDev->eventFilter.recvdLicc));

    switch(bit) {
    case CNL_EXT_LICC_C_RLS :
        //
        // valid state for C-Rls receiving is
        // except CLOSE and SEARCH
        //
        switch(mainState) {
        case CNL_STATE_CONNECTION_REQUEST :
        case CNL_STATE_ACCEPT_WAITING :
        case CNL_STATE_RESPONSE_WAITING :
        case CNL_STATE_RESPONDER_RESPONSE :
        case CNL_STATE_INITIATOR_CONNECTED :
        case CNL_STATE_RESPONDER_CONNECTED :
            valid = TRUE;
            break;
        default :
            DBG_ERR("C-Rls received in invalid state[%x].\n", mainState);
            break;
        }
        break;

    case CNL_EXT_LICC_C_REQ :
        //
        // valid state for C-Req receiving is :
        // SEARCH
        // CONNECTION_REQUEST(1st time)     : set crossover and handle normaly
        // CONNECTION_REQUEST(2nd or later) : skip handling
        // ACCEPT_WAITING(delay receive)    : skip handling
        //
        switch(mainState) {
        case CNL_STATE_SEARCH :
            valid = TRUE;
            break;
        case CNL_STATE_CONNECTION_REQUEST :
            valid = TRUE;
            if(pCnlDev->crossover == FALSE) {
                pCnlDev->crossover = TRUE;                
            } else {
                skip = TRUE;
            }
            break;
        case CNL_STATE_ACCEPT_WAITING :
        case CNL_STATE_RESPONDER_RESPONSE :
            valid = TRUE;
            skip  = TRUE;
            break;
        default :
            DBG_ERR("C-Req received in invalid state[%x].\n", mainState);
            break;
        }
        break;
        
    case CNL_EXT_LICC_C_ACC :
        //
        // valid state for C-Acc receiving is :
        // CONNECTION_REQUEST
        // RESPONSE_WAITING(delay receive)    : skip handling
        // INITIATOR_CONNECTED(delay receive) : skip handling
        //
        switch(mainState) {
        case CNL_STATE_CONNECTION_REQUEST :
            valid = TRUE;
            break;
        case CNL_STATE_RESPONSE_WAITING :
        case CNL_STATE_INITIATOR_CONNECTED :
            valid = TRUE;
            skip  = TRUE;
            break;
        default :
            DBG_ERR("C-Acc received in invalid state[%x].\n", mainState);
            break;
        }
        break;
        
    case CNL_EXT_LICC_ACK_FOR_C_ACC :
        //
        // valid state for ACK for C-Acc receiving is :
        // RESPONDER_RESPONSE
        // RESPONSE_WAITING(delay receive)    : skip handling
        // INITIATOR_CONNECTED(delay receive) : skip handling
        //
        switch(mainState) {
        case CNL_STATE_RESPONDER_RESPONSE :
            valid = TRUE;
            break;
        default :
            DBG_ERR("C-Acc received in invalid state[%x].\n", mainState);
            break;
        }
        break;

    case CNL_EXT_LICC_C_SLEEP :
        //
        // valid state for C-Sleep receiving is :
        // RESPONDER_RESPONSE(missing the ACK for C-Acc)
        // INITIATOR_CONNECTED
        // RESPONDER_CONNECTED
        //
        switch(mainState) {
        case CNL_STATE_RESPONDER_RESPONSE :
            valid                = TRUE;
            pCnlDev->missCaccAck = TRUE;
            break;
        case CNL_STATE_INITIATOR_CONNECTED :
        case CNL_STATE_RESPONDER_CONNECTED :
            valid = TRUE;
            break;
        default :
            DBG_ERR("C-Sleep received in invalid state[%x].\n", mainState);
            break;
        }
        break;

    case CNL_EXT_LICC_C_WAKE :
        //
        // valid state for C-Wake receiving is :
        // RESPONDER_RESPONSE(missing the ACK for C-Acc)
        // INITIATOR_CONNECTED
        // RESPONDER_CONNECTED
        //
        switch(mainState) {
        case CNL_STATE_RESPONDER_RESPONSE :
            valid = TRUE;
            pCnlDev->missCaccAck = TRUE;
            break;
        case CNL_STATE_INITIATOR_CONNECTED :
        case CNL_STATE_RESPONDER_CONNECTED :
            valid = TRUE;
            break;
        default :
            DBG_ERR("C-Wake received in invalid state[%x].\n", mainState);
            break;
        }
        break;

    case CNL_EXT_LICC_C_PROBE : // not used.
    default :
        goto EXIT2;
    }

    if(valid) {
        // check power state.
        switch(pCnlDev->pwrState) {
        case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
        case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
            DBG_INFO("power state is changing, skip handling mngframe event.\n");
            pAction->type = CNL_ACTION_NOP;
            goto EXIT;

        case CNL_PWR_STATE_HIBERNATE :
            DBG_INFO("power state is hibernate, wake first.\n");
            pAction->type = CNL_ACTION_WAKE;
            goto EXIT;

        case CNL_PWR_STATE_AWAKE :
        default :
            break;
        }

        //
        // generate Action.handleMngFrame
        //
        pAction->type = CNL_ACTION_HANDLE_MNG_FRAME;
        pAction->licc = bit;

        CMN_unlockCpu(pCnlDev->mngLockId);

        //
        // need to read management frame body.
        //
        retval = pCnlDev->pDeviceOps->pReadMngBody(pCnlDev,
                                                   &pAction->licc,
                                                   &pAction->targetUID,
                                                   &pAction->liccInfo);

        CMN_lockCpu(pCnlDev->mngLockId);

        if(retval != CNL_SUCCESS) {
            pAction->type   = CNL_ACTION_HANDLE_ERROR;
            pAction->status = retval;
        } else if(skip) {
            // skip handling this management frame.
            pAction->type = CNL_ACTION_NOP;
        }
    } else {
        CNL_setUnexpectedEventAction(pAction);
    }

EXIT2:
    CLEAR_BIT(pEvent->recvdLicc, bit);
    if((pEvent->recvdLicc & 0xFFFF) == 0) {
        // no managementframe received, clear event bit.
        CLEAR_BIT(pEvent->type, CNL_EVENT_RECVD_MNG_FRAME);
    }

EXIT :
    return;
}


/*=================================================================*/
/* Action generator for Ctrl REQUEST                               */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_initReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert INIT.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_initReqToAction(S_CNL_DEV     *pCnlDev,
                    S_CNL_CMN_REQ *pReq,
                    S_CNL_ACTION  *pAction)
{

    u8 mainState;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // INIT request
    // CLOSE state : Action.Init
    // else        : Action.Complete(INVSTAT)
    //

    if(mainState != CNL_STATE_CLOSE) {
        // invalid state request.
        DBG_WARN("invalid state[0x%x] for INIT request.\n", mainState);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        return;
    }
    pAction->type      = CNL_ACTION_INIT;
    pAction->pReq      = pReq;
    pReq->state        = CNL_REQ_PROCESSING;

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_closeReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert CLOSE.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_closeReqToAction(S_CNL_DEV     *pCnlDev,
                     S_CNL_CMN_REQ *pReq,
                     S_CNL_ACTION  *pAction)
{

    u8 mainState;
    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // CLOSE request
    // any state except CLOSE(AWAKE)     : Action.Close
    // any state except CLOSE(HIBERNATE) : Action.Wake
    // CLOSE state                       : Action.Complete(INVSTAT)
    //

    if((mainState == CNL_STATE_CLOSE) || (mainState == CNL_STATE_NULL)) {
        // invalid state request.
        DBG_WARN("invalid state[0x%x] for CLOSE request.\n", mainState);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        return;
    }

    // check power state.
    switch(pCnlDev->pwrState) {
    case CNL_PWR_STATE_HIBERNATE :
        DBG_INFO("CLOSE request during Hinbernate, awake first.\n");
        pAction->type      = CNL_ACTION_WAKE;
        break;

    case CNL_PWR_STATE_AWAKE :
        pAction->type      = CNL_ACTION_CLOSE;
        pAction->pReq      = pReq;
        pReq->state        = CNL_REQ_PROCESSING;
        break;
    case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
    case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
    default : 
        DBG_INFO("currently pwr state is changing, pedning it.\n");
        break;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_connectReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert CONNECT.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_connectReqToAction(S_CNL_DEV     *pCnlDev,
                       S_CNL_CMN_REQ *pReq,
                       S_CNL_ACTION  *pAction)
{

    u8 mainState;
    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // CONNECT request
    // SEARCH state(AWAKE)      : Action.Order
    // SEARCH state(HIBERNATE)  : Action.Wake
    // CONNECTION_REQUEST state : Action.SendMngFrame
    // else                     : Action.Complete(INVSTAT)
    //

    switch(mainState) {
    case CNL_STATE_SEARCH :
        // change state to CONNECTION_REQUEST first.
        switch(pCnlDev->pwrState) {
        case CNL_PWR_STATE_HIBERNATE :
            DBG_INFO("CONNECT request during Hinbernate, awake first.\n");
            pAction->type      = CNL_ACTION_WAKE;
            break;
        case CNL_PWR_STATE_AWAKE :
            pAction->type  = CNL_ACTION_HANDLE_UPPER_ORDER;
            pAction->order = pReq->type;
            break;
        case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
        case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
        default : 
            DBG_INFO("currently pwr state is changing, pedning it.\n");
            break;
        }
        break;

    case CNL_STATE_CONNECTION_REQUEST :
        // send management frame action.
        DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);
        pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
        pAction->pReq      = pReq;
        pReq->state        = CNL_REQ_PROCESSING;
        break;

    default :
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        break;
    }

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_wconnectReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert WAIT_CONNECT.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_wconnectReqToAction(S_CNL_DEV     *pCnlDev,
                       S_CNL_CMN_REQ *pReq,
                       S_CNL_ACTION  *pAction)
{
    
    T_CNL_ERR retval;
    u8        mainState;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // WAIT_CONNECT request
    // SEARCH and AWAKE     : Action.Complete(SUCCESS)(execute)
    // SEARCH and HIBERNATE : Action.Wake
    // else                 : Action.Complete(INVSTAT)
    //

    if((mainState != CNL_STATE_SEARCH) || (pCnlDev->waitConnect != FALSE)) {
        DBG_WARN("wait connect invalid state.\n", mainState);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        return;
    }

    switch(pCnlDev->pwrState) {
    case CNL_PWR_STATE_HIBERNATE :
        DBG_INFO("WCONNECT request during Hinbernate, awake first.\n");
        pAction->type = CNL_ACTION_WAKE;
        break;

    case CNL_PWR_STATE_AWAKE :
        CMN_unlockCpu(pCnlDev->mngLockId);
        retval = pCnlDev->pDeviceOps->pWaitConnect(pCnlDev);
        CMN_lockCpu(pCnlDev->mngLockId);

        if(retval == CNL_SUCCESS) {
            DBG_INFO("wait connect success, complete immediately.\n");
            pCnlDev->waitConnect = TRUE;
            CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_SUCCESS, pAction);
        } else {
            DBG_ERR("wait connect failed[%d].\n");
            CNL_setCtrlReqCompAction(pCnlDev, pReq, retval, pAction);
        }
        break;

    case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
    case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
    default : 
        DBG_INFO("currently pwr state is changing, pedning it.\n");
        break;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_acceptReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert ACCEPT.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_acceptReqToAction(S_CNL_DEV     *pCnlDev,
                      S_CNL_CMN_REQ *pReq,
                      S_CNL_ACTION  *pAction)
{

    u8 mainState;
    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // ACCEPT request
    // CONNECTION_REQUEST state : Action.Order
    // ACCEPT_WAITING state     : Action.Order
    // RESPONDER_RESPONSE state : Action.SendMngFrame
    // else                     : Action.Complete(INVSTAT)
    //

    DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);

    switch(mainState) {
    case CNL_STATE_CONNECTION_REQUEST :
    case CNL_STATE_ACCEPT_WAITING :
        // change state to RESPONDER_RESPONSE first.
        pAction->type  = CNL_ACTION_HANDLE_UPPER_ORDER;
        pAction->order = pReq->type;
        break;
    case CNL_STATE_RESPONDER_RESPONSE :
        // send management frame action.
        pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
        pAction->pReq      = pReq;
        pReq->state        = CNL_REQ_PROCESSING;
        break;
    default :
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        break;
    }

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_acceptResToAction
 *-----------------------------------------------------------------*/
/**
 * convert ACCEPT.response to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_acceptResToAction(S_CNL_DEV     *pCnlDev,
                      S_CNL_CMN_REQ *pReq,
                      S_CNL_ACTION  *pAction)
{

    u8 mainState;
    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // ACCEPT response
    // RESPONSE_WAITING state : Action.SendMngFrame
    // else                   : Action.Complete(INVSTAT)
    //

    DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);

    // accept response is valid in RESPONSE_WAITING, 
    // and send ACK for C-Acc immediately.
    switch(mainState) {
    case CNL_STATE_RESPONSE_WAITING :
        // send management frame action.
        pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
        pAction->pReq      = pReq;
        pReq->state        = CNL_REQ_PROCESSING;
        break;
    default :
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        break;
    }
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_releaseReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert RELEASE.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_releaseReqToAction(S_CNL_DEV     *pCnlDev,
                       S_CNL_CMN_REQ *pReq,
                       S_CNL_ACTION  *pAction)
{

    u8 mainState;
    u8 subState;

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    //
    // RELEASE request
    // CONNECTION_REQUEST state            : Action.SendMngFrame(C-Rls)
    // ACCEPT_WAITING state                : Action.SendMngFrame(C-Rls)
    // RESPONSE_WAITING state              : Action.SendMngFrame(C-Rls)
    // CONNECTED substate                  : Action.SendMngFrame(C-Rls)
    // TARGET_SLEEP substate               : Action.SendMngFrame(C-Wake)
    // LOCAL_HIBERNATE substate(AWAKE)     : Action.Order(C-Wake)
    // LOCAL_HIBERNATE substate(HIBERNATE) : Action.Wake
    //

    switch(mainState) {
    case CNL_STATE_CONNECTION_REQUEST :
    case CNL_STATE_ACCEPT_WAITING :
    case CNL_STATE_RESPONSE_WAITING :
        DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);
        // send management frame action.
        pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
        pAction->pReq      = pReq;
        pReq->state        = CNL_REQ_PROCESSING;
        break;

    case CNL_STATE_INITIATOR_CONNECTED :
    case CNL_STATE_RESPONDER_CONNECTED :
        switch(subState) {
        case CNL_SUBSTATE_CONNECTED :
            DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);
            pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
            pAction->pReq      = pReq;
            pReq->state        = CNL_REQ_PROCESSING;
            break;

        case CNL_SUBSTATE_TARGET_SLEEP :
        case CNL_SUBSTATE_LOCAL_HIBERNATE :
            CNL_reqTsLhToAction(pCnlDev, pAction);
            break;
        default :
            DBG_ASSERT(0);
            break;
        }
        break;
    case CNL_STATE_SEARCH:
        if( pCnlDev->waitConnect != FALSE ) {
            T_CNL_ERR retval;
            switch(pCnlDev->pwrState) {
            case CNL_PWR_STATE_HIBERNATE:
                DBG_INFO("Release request during Hibernate, awake first.\n");
                pAction->type = CNL_ACTION_WAKE;
                break;
            case CNL_PWR_STATE_AWAKE:
                // Cancel WaitConnect
                CMN_unlockCpu(pCnlDev->mngLockId);
                retval = pCnlDev->pDeviceOps->pCancelWaitConnect(pCnlDev);
                CMN_lockCpu(pCnlDev->mngLockId);
                if(retval == CNL_SUCCESS) {
                    DBG_INFO("cancel wait connect success, complete immediately.\n");
                    pCnlDev->waitConnect = FALSE;
                    CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_SUCCESS, pAction);
                } else {
                    DBG_ERR("CancelWaitConnect failed[%d].\n", retval);
                    CNL_setCtrlReqCompAction(pCnlDev, pReq, retval, pAction);
                }
                break;
            case CNL_PWR_STATE_AWAKE_TO_HIBERNATE:
            case CNL_PWR_STATE_HIBERNATE_TO_AWAKE:
            default:
                DBG_INFO("currently pwr state is changing, pendingit.\n");
                break;
            }
        } else {
            CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        }
        break;
    default :
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        break;
    }
    return;

}


/*=================================================================*/
/* Action generator for Data REQUEST                               */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_sendDataReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert send data request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note
 */
/*-----------------------------------------------------------------*/
static void
CNL_sendDataReqToAction(S_CNL_DEV     *pCnlDev,
                        S_CNL_ACTION  *pAction)
{

    T_CNL_ERR retval;
    u8        subState;
    u32       length;

    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    //
    // send data request
    // CONNECTED substate                  : Action.SendData
    // TARGET_SLEEP substate               : Action.SendMngFrame(C-Wake)
    // LOCAL_HIBERNATE substate(AWAKE)     : Action.Order(C-Wake)
    // LOCAL_HIBERNATE substate(HIBERNATE) : Action.Wake
    //

    switch(subState) {
    case CNL_SUBSTATE_CONNECTED :
        DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);

        // read ready TX buffer length.(should be CSDU_SIZE * n)
        retval = pCnlDev->pDeviceOps->pReadReadyBuffer(pCnlDev, CNL_TX_CSDU, &length);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("read ready buffer(TX) failed[%d].\n", retval);
            pAction->type   = CNL_ACTION_HANDLE_ERROR;
            pAction->status = retval;
            break;
        }

        DBG_ASSERT(IS_MULTI_OF_4K(length));

        pAction->type        = CNL_ACTION_SEND_DATA;
        pAction->readyLength = length;
        
        break;

    case CNL_SUBSTATE_TARGET_SLEEP :
    case CNL_SUBSTATE_LOCAL_HIBERNATE :
        CNL_reqTsLhToAction(pCnlDev, pAction);
        break;
    default :
        DBG_ASSERT(0);
        break;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_recvDataReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert receive data request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_recvDataReqToAction(S_CNL_DEV     *pCnlDev,
                        S_CNL_CMN_REQ *pReq,
                        S_CNL_ACTION  *pAction)
{

    T_CNL_ERR retval;
    u32       length;

    //
    // recieve data request
    // pReq == NULL and need DataInd               : Action.HandleExtraEvent
    // INITIATOR/RESPONDER_CONNECTED and AWAKE     : Action.ReceiveData
    // INITIATOR/RESPONDER_CONNECTED and HIBERNATE : Action.Wake
    //

    if(pReq == NULL) {
        if(CNL_needDataInd(pCnlDev, pCnlDev->rxReadyPid)) {
            pAction->type      = CNL_ACTION_HANDLE_EXTRA_EVENT;
            pAction->extra     = CNL_EXTRA_DATA_INDICATION;
            pAction->profileId = pCnlDev->rxReadyPid;
            return;
        } else {
            return; // return NOP
        }
    }

    switch(pCnlDev->pwrState) {
    case CNL_PWR_STATE_AWAKE :
        // read ready TX buffer length.(should be CSDU_SIZE * n)
        retval = pCnlDev->pDeviceOps->pReadReadyBuffer(pCnlDev, CNL_RX_CSDU, &length);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("read ready buffer(TX) failed[%d].\n", retval);
            pAction->type   = CNL_ACTION_HANDLE_ERROR;
            pAction->status = retval;
            break;
        }
        pAction->type        = CNL_ACTION_RECEIVE_DATA;
        pAction->readyLength = length;

        // reset DATA Ind,
        CNL_resetDataInd(pCnlDev, pCnlDev->rxReadyPid);
        break;
    case CNL_PWR_STATE_HIBERNATE :
        pAction->type      = CNL_ACTION_WAKE;
        break;
    default :
        break;
    }

    return;
}


/*=================================================================*/
/* Action generator for TargetSleep/LocalHibernate                 */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_reqTsLhToAction
 *-----------------------------------------------------------------*/
/**
 * generate Action Ctrl/Data request queued when TARGET_SLEEP/LOCAL_HIBERNATE
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_reqTsLhToAction(S_CNL_DEV     *pCnlDev,
                    S_CNL_ACTION  *pAction)
{

    u8 subState;

    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);

    //
    // request while TARGET_SLEEP or LOCAL_HIBERNATE.
    // TARGET_SLEEP substate               : Action.SendMngFrame(C-Wake)
    // LOCAL_HIBERNATE substate(AWAKE)     : Action.Order(C-Wake)
    // LOCAL_HIBERNATE substate(HIBERNATE) : Action.Wake
    //

    switch(subState) {
    case CNL_SUBSTATE_TARGET_SLEEP :
        DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);
        pAction->type        = CNL_ACTION_SEND_MNG_FRAME;
        pAction->pReq        = &pCnlDev->pwrReq;
        pCnlDev->pwrReq.type = CNL_REQ_TYPE_WAKE_REQ;
        break;
    case CNL_SUBSTATE_LOCAL_HIBERNATE :
        switch(pCnlDev->pwrState) {
        case CNL_PWR_STATE_HIBERNATE :
            pAction->type      = CNL_ACTION_WAKE;
            break;
        case CNL_PWR_STATE_AWAKE :
            pAction->type  = CNL_ACTION_HANDLE_UPPER_ORDER;
            pAction->order = CNL_REQ_TYPE_WAKE_REQ;
            break;
        case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
        case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
        default : 
            DBG_INFO("currently pwr state is changing, pedning it.\n");
            break;
        }
        break;
    case CNL_SUBSTATE_CONNECTED :
    default :
        DBG_ASSERT(0);
        break;
    }
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_regDataIndToAction
 *-----------------------------------------------------------------*/
/**
 * convert register DATA Ind request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_regDataIndToAction(S_CNL_DEV     *pCnlDev,
                       S_CNL_CMN_REQ *pReq,
                       S_CNL_ACTION  *pAction)
{
    // all state is valid. only check pid and multiple call.
    if((pReq->regDataInd.profileId != CNL_PROFILE_ID_0) &&
       (pReq->regDataInd.profileId != CNL_PROFILE_ID_1)) {
        DBG_WARN("invalid PID[%u] to register callback.\n", pReq->regDataInd.profileId);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_BADPARM, pAction);
        return;
    }
    if(pCnlDev->recvCbk[pReq->regDataInd.profileId].pCbk != NULL) {
        DBG_WARN("DATA.ind callback is arleady registered.\n");
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        return;
    }

    // store callbacks. and complete.
    pCnlDev->recvCbk[pReq->regDataInd.profileId].pCbk = pReq->regDataInd.pCbk;
    pCnlDev->recvCbk[pReq->regDataInd.profileId].pArg = pReq->regDataInd.pArg;
    pCnlDev->recvCbk[pReq->regDataInd.profileId].call = TRUE;
    CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_SUCCESS, pAction);
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_unregDataIndToAction
 *-----------------------------------------------------------------*/
/**
 * convert register DATA Ind request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_unregDataIndToAction(S_CNL_DEV     *pCnlDev,
                       S_CNL_CMN_REQ *pReq,
                       S_CNL_ACTION  *pAction)
{
    // all state is valid. only check pid and multiple call.
    if((pReq->unregDataInd.profileId != CNL_PROFILE_ID_0) &&
       (pReq->unregDataInd.profileId != CNL_PROFILE_ID_1)) {
        DBG_WARN("invalid PID[%u] to unregister callback.\n", pReq->unregDataInd.profileId);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_BADPARM, pAction);
        return;
    }
    if(pCnlDev->recvCbk[pReq->unregDataInd.profileId].pCbk == NULL) {
        DBG_WARN("DATA.ind callback is not registered.\n");
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        return;
    }

    // store callbacks and complete
    pCnlDev->recvCbk[pReq->unregDataInd.profileId].pCbk = NULL;
    pCnlDev->recvCbk[pReq->unregDataInd.profileId].pArg = NULL;
    pCnlDev->recvCbk[pReq->unregDataInd.profileId].call = FALSE;
    CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_SUCCESS, pAction);
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_getStatsToAction
 *-----------------------------------------------------------------*/
/**
 * convert GET_STATS.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_getStatsToAction(S_CNL_DEV     *pCnlDev,
                     S_CNL_CMN_REQ *pReq,
                     S_CNL_ACTION  *pAction)
{

    T_CNL_ERR retval = CNL_SUCCESS;
    u8 mainState;

    // GET_STATUS_request
    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
       (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
        pReq->stats.RSSI = g_rssi;
    } else {
        pReq->stats.RSSI = CNL_RSSI_VALUE_MIN;
    }
    CNL_setCtrlReqCompAction(pCnlDev, pReq, retval, pAction);
	
    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_regPassthroughToAction
 *-----------------------------------------------------------------*/
/**
 * convert REG_PASSTHROUGH.request to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_regPassthroughToAction(S_CNL_DEV     *pCnlDev,
                           S_CNL_CMN_REQ *pReq,
                           S_CNL_ACTION  *pAction)
{
    T_CNL_ERR retval;

    switch(pCnlDev->pwrState) {
    case CNL_PWR_STATE_HIBERNATE :
        DBG_INFO("RegPassthrough request during Hinbernate, awake first.\n");
        pAction->type = CNL_ACTION_WAKE;
        break;
    case CNL_PWR_STATE_AWAKE :
        CMN_unlockCpu(pCnlDev->mngLockId);
        retval = pCnlDev->pDeviceOps->pRegPassthrough(pCnlDev, &pReq->regPT);
        CMN_lockCpu(pCnlDev->mngLockId);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, retval, pAction);
        break;
    case CNL_PWR_STATE_AWAKE_TO_HIBERNATE :
    case CNL_PWR_STATE_HIBERNATE_TO_AWAKE :
    default : 
        DBG_INFO("currently pwr state is changing, pending it.\n");
        break;
    }
    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_powersaveReqToAction
 *-----------------------------------------------------------------*/
/**
 * convert POWERSAVE.requrst to ACTION.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   assumed to call with holding manage lock.
 */
/*-----------------------------------------------------------------*/
static void
CNL_powersaveReqToAction(S_CNL_DEV     *pCnlDev,
                         S_CNL_CMN_REQ *pReq,
                         S_CNL_ACTION  *pAction)
{
    u8 mainState;

    // parameter valid range check
    if((pReq->powersaveReq.dormantPeriod < CNL_POWERSAVE_TDC_RANGE_MIN) ||
       (pReq->powersaveReq.dormantPeriod > CNL_POWERSAVE_TDC_RANGE_MAX)) {
        DBG_WARN("invalid dormantPeriod[%u].\n", pReq->powersaveReq.dormantPeriod);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_BADPARM, pAction);
        return;
    }
    if((pReq->powersaveReq.awakePeriod < CNL_POWERSAVE_TAC_RANGE_MIN) ||
       (pReq->powersaveReq.awakePeriod > CNL_POWERSAVE_TAC_RANGE_MAX)) {
        DBG_WARN("invalid awakePeriod[%u].\n", pReq->powersaveReq.awakePeriod);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_BADPARM, pAction);
        return;
    }
    if((pReq->powersaveReq.keepAlive < CNL_POWERSAVE_TKEEPALIVE_RANGE_MIN) ||
       (pReq->powersaveReq.keepAlive > CNL_POWERSAVE_TKEEPALIVE_RANGE_MAX)) {
        DBG_WARN("invalid keepAlive[%u].\n", pReq->powersaveReq.keepAlive);
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_BADPARM, pAction);
        return;
    }

    //
    // POWERSAVE.request
    // CONNECTED state   : Action.SendMngFrame
    // else              : Action.Complete(INVSTAT)
    //

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    DBG_ASSERT(pCnlDev->pwrState == CNL_PWR_STATE_AWAKE);

    switch(mainState) {
    case CNL_STATE_INITIATOR_CONNECTED :
    case CNL_STATE_RESPONDER_CONNECTED :
        // In the case of LOCAL_HIBERNATE state and the same preset value, 
        // it is returned in the state of a success. 
        if((CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState) == CNL_SUBSTATE_LOCAL_HIBERNATE) && 
           (pCnlDev->deviceParam.tKeepAlive == CNL_POWERSAVE_KEEPALIVE_TO_TKEEPALIVE(pReq->powersaveReq.keepAlive)) && 
           (pCnlDev->deviceParam.tdc == CNL_POWERSAVE_DORMANT_TO_TDC(pReq->powersaveReq.dormantPeriod)) && 
           (pCnlDev->deviceParam.tac == CNL_POWERSAVE_AWAKE_TO_TAC(pReq->powersaveReq.awakePeriod))) {
            DBG_INFO("POWERSAVE.request but already LocalHibernate state.\n");
            CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_SUCCESS, pAction);
            break;
        }
        // check powersave.request criteria
        //  (1)CNL substate is CONNECTED or TARGET_SLEEP
        //  (2)control request count <= 1
        //  (3)TX queue empty.
        //  (4)TX/RX buffer empty.
        if(((CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState) == CNL_SUBSTATE_CONNECTED) ||
            (CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState) == CNL_SUBSTATE_TARGET_SLEEP)) &&
                   (pCnlDev->ctrlReqCnt <= 1) && (pCnlDev->txReqCnt == 0) &&
                   (pCnlDev->txReady) && (pCnlDev->rxReady == FALSE)) {
            /* 
            This is a timer value of a hardware setup of the self-equipment of powersave.request. 
            In case of Local Hibernate state, a timer value is not rewritten.
            Otherwise, a timver value is updated. 
            */
            pCnlDev->deviceParam.tKeepAlive = CNL_POWERSAVE_KEEPALIVE_TO_TKEEPALIVE(pReq->powersaveReq.keepAlive);
            pCnlDev->deviceParam.tdc        = CNL_POWERSAVE_DORMANT_TO_TDC(pReq->powersaveReq.dormantPeriod);
            pCnlDev->deviceParam.tac        = CNL_POWERSAVE_AWAKE_TO_TAC(pReq->powersaveReq.awakePeriod);
            // send management frame action.
            pAction->type      = CNL_ACTION_SEND_MNG_FRAME;
            pAction->pReq      = pReq;
            pReq->state        = CNL_REQ_PROCESSING;
        }
        else{
            CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_CANCELLED, pAction);
        }
        break;
    default :
        CNL_setCtrlReqCompAction(pCnlDev, pReq, CNL_ERR_INVSTAT, pAction);
        break;
    }
    return;
}


/*=================================================================*/
/* External functions                                              */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_addRequest
 *-----------------------------------------------------------------*/
/**
 * add request to the CNL request queue and set the device signaled.
 * this function is assumed to be called by CNL interface functions.
 * @param   pCnlDev  : the pointer to the CNL device
 * @param   pReq     : the pointer to the CNL request.
 * @param   queue    : target queue to chain the request.
 * @return  SUCCESS     (normally completion)
 * @return  ERR_BADPRM  (unknown queue type)
 * @return  ERR_QOVR    (queue overflow)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_addRequest(S_CNL_DEV     *pCnlDev,
               S_CNL_CMN_REQ *pReq)

{

    T_CMN_ERR retval = SUCCESS;

    CMN_lockCpu(pCnlDev->mngLockId);
  
    switch(pReq->type) {
    case CNL_REQ_TYPE_INIT_REQ :
    case CNL_REQ_TYPE_CLOSE_REQ :
    case CNL_REQ_TYPE_CONNECT_REQ :
    case CNL_REQ_TYPE_WAIT_CONNECT_REQ :
    case CNL_REQ_TYPE_ACCEPT_REQ :
    case CNL_REQ_TYPE_ACCEPT_RES :
    case CNL_REQ_TYPE_RELEASE_REQ :
    case CNL_REQ_TYPE_REG_DATA_IND :
    case CNL_REQ_TYPE_UNREG_DATA_IND :
    case CNL_REQ_TYPE_GET_STATS :
    case CNL_REQ_TYPE_REG_PASSTHROUGH :
    case CNL_REQ_TYPE_POWERSAVE_REQ :
        //
        // chain to CTRL queue.
        //
        retval = CNL_addRequestToCtrlQueue(pCnlDev, pReq);
        break;


    case CNL_REQ_TYPE_SEND_REQ :
        //
        // chain to TX queue
        //
        retval = CNL_addRequestToTxQueue(pCnlDev, pReq);
        break;

    case CNL_REQ_TYPE_RECEIVE_REQ :
        //
        // chain to RX queue(pid 0 or 1)
        //
        retval = CNL_addRequestToRxQueue(pCnlDev, pReq);
        break;

    default :
        // unknown request type.
        DBG_ASSERT(0);
        retval =  ERR_INVSTAT;
        break;
    }

    CMN_unlockCpu(pCnlDev->mngLockId);
    
    if(retval != SUCCESS) {
        return retval;
    }


    //
    // signal device to schedule.
    //
    CNL_signalDev(pCnlDev);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_cancelRequest
 *-----------------------------------------------------------------*/
/**
 * cancel a request.
 * @param   pCnlDev : the pointer to the CNL device
 * @param   reqId   : request ID of the CNL request to cancel.
 * @return  SUCCESS     (normally completion)
 * @return  ERR_NOOBJ   (target request is not found)
 * @return  ERR_NOMEM   (the memory or resource is depleted)
 * @return  ERR_TIMEOUT (the timeout occured)
 * @return  ERR_RSVFUNC (the function has already reserved)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_cancelRequest(S_CNL_DEV    *pCnlDev,
                  T_CNL_REQ_ID  reqId)
{

    T_CMN_ERR      retval;
    S_CNL_CMN_REQ *pDummy;

    // 
    // alloc dummy request. 
    //
    retval = CMN_getFixedMemPool(pCnlDev->dummyReqMplId, (void **)(&pDummy), CMN_TIME_FEVR);
    if((retval != SUCCESS) || (pDummy == NULL)) {
        DBG_ERR("allocate Dummy Request for cancelling failed[%d].\n", retval);
        return ERR_QOVR;
    }
    
    // setting alarm timer for cancel request.
    if (g_cnlCancelToutArg.pCnlDev != NULL) {
        return ERR_RSVFUNC;
    }
    g_cnlCancelToutArg.pCnlDev = pCnlDev;
    g_cnlCancelToutArg.pDummy = pDummy;
    retval = CMN_createAlarmTim(CNL_CANCEL_TOUT_TIM_ID, 0, NULL, CNL_cancelToutCallback);
    if (retval != SUCCESS) {
        DBG_ERR("CMN_createAlarmTim failed[%d].\n", retval);
    } else {
        retval = CMN_startAlarmTim(CNL_CANCEL_TOUT_TIM_ID, CNL_CANCEL_TOUT_VAL_MS);
        if (retval != SUCCESS) {
            DBG_ERR("CMN_startAlarmTim failed[%d].\n", retval);
            retval = CMN_deleteAlarmTim(CNL_CANCEL_TOUT_TIM_ID);
            if (retval != SUCCESS) {
                DBG_ERR("CMN_deleteAlarmTim failed[%d].\n", retval);
            }
        }
    }

    CMN_lockCpu(pCnlDev->mngLockId);

    pDummy->id       = reqId;
    CMN_LIST_ADD_TAIL(&pCnlDev->cancelQueue, pDummy, S_CNL_CMN_REQ, list);

    CMN_unlockCpu(pCnlDev->mngLockId);

    CNL_signalDev(pCnlDev);

    // wait for completion cancel.
    CMN_WAIT(pCnlDev->cancelWaitId);

    // stop alarm timer for cancel request.
    retval = CMN_stopAlarmTim(CNL_CANCEL_TOUT_TIM_ID);
    if (retval != SUCCESS) {
        DBG_ERR("CMN_stopAlarmTim failed[%d].\n", retval);
    }
    retval = CMN_deleteAlarmTim(CNL_CANCEL_TOUT_TIM_ID);
    if (retval != SUCCESS) {
        DBG_ERR("CMN_deleteAlarmTim failed[%d].\n", retval);
    }
    g_cnlCancelToutArg.pCnlDev = NULL;
    g_cnlCancelToutArg.pDummy = NULL;


    return pCnlDev->cancelStatus;

}


/*-------------------------------------------------------------------
 * Function : CNL_addEvent
 *-----------------------------------------------------------------*/
/**
 * input an event to the CNL device and set the signaled.
 * this function is assumed to be called by IRQ handler.
 * @param   pCnlDev   : the pointer to the S_CNL_DEV
 * @param   pEvent    : the pointer to the S_CNL_DEVICE_EVENT
 * @return  SUCCESS     (normally completion)
 * @return  ERR_INVSTAT (invalid state error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_addEvent(S_CNL_DEV          *pCnlDev,
             S_CNL_DEVICE_EVENT *pEvent)
{

    T_CMN_ERR   retval = SUCCESS;
    T_CNL_EVENT eventType, bit;

    eventType = pEvent->type;

    CMN_lockCpu(pCnlDev->mngLockId);

    if(pCnlDev->devState == CNL_DEV_GONE) {
        DBG_ERR("this device is already removed.\n");
        CMN_unlockCpu(pCnlDev->mngLockId);
        return ERR_INVSTAT;
    }

    do {
        // add event to CNL scheduler
        bit        = GET_PRIOR_BIT(eventType);

        switch(bit) {
        case CNL_EVENT_RX_READY :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_RX_READY) == 0);
            // no information.
            break;
        case CNL_EVENT_TX_READY :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_TX_READY) == 0);
            // no information.
            break;
        case CNL_EVENT_TX_COMP :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_TX_COMP) == 0);
            // no information.
            break;
        case CNL_EVENT_DEVICE_REMOVED :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_DEVICE_REMOVED) == 0);
            // no information
            break;
        case CNL_EVENT_WAKE_REQUIRED :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_WAKE_REQUIRED) == 0);
            // no information.
            break;

        case CNL_EVENT_ERROR_OCCURRED :
            DBG_ASSERT((pCnlDev->event.type & CNL_EVENT_ERROR_OCCURRED) == 0);
            pCnlDev->event.error = pEvent->error;
            break;
        case CNL_EVENT_TIMEOUT :
            pCnlDev->event.timer     |= pEvent->timer;
            break;
        case CNL_EVENT_COMP_REQUEST :
            pCnlDev->event.compReq   |= pEvent->compReq;   // completed request type.
            break;
        case CNL_EVENT_RECVD_MNG_FRAME :
            pCnlDev->event.recvdLicc |= pEvent->recvdLicc; // LiCC of received management frame.
            break;
        default :
            DBG_ERR("unknown event to add[0x%x].\n", eventType);
            break;
        }
        CLEAR_BIT(eventType, bit);
        pCnlDev->event.type |= bit;
    }while(eventType != 0);

    CMN_unlockCpu(pCnlDev->mngLockId);


    retval = CNL_signalDev(pCnlDev);

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_clearEvent
 *-----------------------------------------------------------------*/
/**
 * clear unnecessary event.
 * called when state change is occured to clear event should be ignored.
 * in new state.
 * @param   pCnlDev   : the pointer to the S_CNL_DEV
 * @param   pEvent    : the pointer to the S_CNL_DEVICE_EVENT
 * @return  SUCCESS     (normally completion)
 * @return  ERR_INVSTAT (invalid state error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_clearEvent(S_CNL_DEV          *pCnlDev,
               S_CNL_DEVICE_EVENT *pEvent)
{

    T_CMN_ERR   retval = SUCCESS;
    T_CNL_EVENT eventType, bit;
    u16         clear;

    eventType = pEvent->type;

    CMN_lockCpu(pCnlDev->mngLockId);

    if(pCnlDev->devState == CNL_DEV_GONE) {
        DBG_ERR("this device is already removed.\n");
        CMN_unlockCpu(pCnlDev->mngLockId);
        return ERR_INVSTAT;
    }

    do {
        // add event to CNL scheduler
        bit        = GET_PRIOR_BIT(eventType);

        switch(bit) {
        case CNL_EVENT_RX_READY :
        case CNL_EVENT_TX_READY :
        case CNL_EVENT_TX_COMP :
        case CNL_EVENT_DEVICE_REMOVED :
        case CNL_EVENT_WAKE_REQUIRED :
        case CNL_EVENT_ERROR_OCCURRED :
            // no sub parameter.
            if(pCnlDev->event.type & bit) {
                DBG_INFO("Event[0x%x] has been cleared.\n", bit);
                CLEAR_BIT(pCnlDev->event.type, bit);
            } else {
                CLEAR_BIT(pEvent->type, bit);
            }
            break;

        case CNL_EVENT_TIMEOUT :
            clear = pCnlDev->event.timer & pEvent->timer;
            if(clear) {
                DBG_INFO("EventTimeout[0x%x] has been cleared.\n", clear);

                CLEAR_BIT(pCnlDev->event.timer, clear);
                CLEAR_BIT(pEvent->timer, ~clear);

                if(pCnlDev->event.timer == 0) {
                    CLEAR_BIT(pCnlDev->event.type, bit);
                }
            } else {
                // no clear timer event.
                CLEAR_BIT(pEvent->timer, pEvent->timer);
                CLEAR_BIT(pEvent->type, bit);
            }
            break;
        case CNL_EVENT_COMP_REQUEST :
            clear = pCnlDev->event.compReq & pEvent->compReq;
            if(clear) {
                DBG_INFO("EventCompReq[0x%x] has been cleared.\n", clear);

                CLEAR_BIT(pCnlDev->event.compReq, clear);
                CLEAR_BIT(pEvent->compReq, ~clear);

                if(pCnlDev->event.compReq == 0) {
                    CLEAR_BIT(pCnlDev->event.type, bit);
                }
            } else {
                // no clear comp event.
                CLEAR_BIT(pEvent->compReq, pEvent->compReq);
                CLEAR_BIT(pEvent->type, bit);
            }
            break;

        case CNL_EVENT_RECVD_MNG_FRAME :
            clear = pCnlDev->event.recvdLicc & pEvent->recvdLicc;
            if(clear) {
                DBG_INFO("EventRecvdLicc[0x%x] has been cleared.\n", clear);

                CLEAR_BIT(pCnlDev->event.recvdLicc, clear);
                CLEAR_BIT(pEvent->recvdLicc, ~clear);

                if(pCnlDev->event.recvdLicc == 0) {
                    CLEAR_BIT(pCnlDev->event.type, bit);
                }
            } else {
                // no clear comp event.
                CLEAR_BIT(pEvent->recvdLicc, pEvent->recvdLicc);
                CLEAR_BIT(pEvent->type, bit);
            }
            break;
        default :
            DBG_ERR("unknown event to clear[0x%x].\n", bit);
            break;
        }
        CLEAR_BIT(eventType, bit);
    }while(eventType != 0);

    CMN_unlockCpu(pCnlDev->mngLockId);

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_getAction
 *-----------------------------------------------------------------*/
/**
 * schedule CNL action.
 * @param  *pCnlDev : the pointer to the S_CNL_DEV
 * @param  *pAction : the pointer to the S_CNL_ACTION list
 * @return SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_getAction(S_CNL_DEV    *pCnlDev, 
              S_CNL_ACTION *pAction)
{

    u8  mainState;

    int i;
    for(i=0; i<CNL_ACTION_LIST_NUM; i++) {
        CNL_initAction(&(pAction[i]));
    }


    //
    // if cancel is called handle it first, and fall through to scheduling.
    //
    CNL_checkCancel(pCnlDev);

    //
    // check Init/Close request and generate action.
    //
    CNL_checkInitClose(pCnlDev, pAction);
    if(pAction->type != CNL_ACTION_NOP) {
        goto EXIT;
    }

    //
    // check event and generate action.
    //
    CNL_checkEvent(pCnlDev, pAction);
    if(pAction->type != CNL_ACTION_NOP) {
        goto EXIT;
    }

    //
    // check control request and generate action
    // if control Action is accessing, skip checking.
    //
    if(pCnlDev->procAction == CNL_ACTION_NOP) {
        CNL_checkCtrlRequest(pCnlDev, pAction);
        if(pAction->type != CNL_ACTION_NOP) {
            goto EXIT;
        }
    }

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    //
    // check Rx data request and generate action.
    // if CNL state is not CONNECTED, skip checking.
    //
    if((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
       (mainState == CNL_STATE_RESPONDER_CONNECTED)) {
        CNL_checkRxRequest(pCnlDev, pAction);
        if(pAction->type != CNL_ACTION_NOP) {
            goto EXIT;
        }
    }
       

    //
    // check Tx data request and generate action.
    //  if CNL state is not CONNECTED or control Action is accessing, skip checking.
    //
    if(((mainState == CNL_STATE_INITIATOR_CONNECTED) || 
        (mainState == CNL_STATE_RESPONDER_CONNECTED)) && 
       (pCnlDev->procAction == CNL_ACTION_NOP)) {

        CNL_checkTxRequest(pCnlDev, pAction);
        if(pAction->type != CNL_ACTION_NOP) {
            goto EXIT;
        }
    }


    //
    // check Sleep 
    // if control Action is accessing, skip checking.
    //
    if(pCnlDev->procAction == CNL_ACTION_NOP) {
        CNL_checkSleep(pCnlDev, pAction);
    }


EXIT:



    DBG_INFO("scheduled action is [0x%x].\n",pAction->type);

    return SUCCESS;
}

/*-------------------------------------------------------------------
 * Function : CNL_cancelToutCallback
 *-----------------------------------------------------------------*/
/**
 * CNL cancel timeout callback function.
 * @param   arg     :not used.
 * @return  nothing
 * @note    use g_cnlCancelToutArg 
 */
/*-----------------------------------------------------------------*/
static void CNL_cancelToutCallback(unsigned long arg)
{

    DBG_INFO("CNL_cancelToutCallback Called.\n");

    if ((g_cnlCancelToutArg.pCnlDev == NULL) || 
        (g_cnlCancelToutArg.pDummy == NULL)){
        return;
    }

    CMN_lockCpu(g_cnlCancelToutArg.pCnlDev->mngLockId);
    if (CMN_IS_IN_LIST(&(g_cnlCancelToutArg.pCnlDev->cancelQueue), g_cnlCancelToutArg.pDummy, S_CNL_CMN_REQ, list)) {
        CMN_LIST_REMOVE(&(g_cnlCancelToutArg.pCnlDev->cancelQueue), g_cnlCancelToutArg.pDummy, S_CNL_CMN_REQ, list);
        g_cnlCancelToutArg.pCnlDev->cancelStatus = ERR_TIMEOUT;
        CMN_releaseFixedMemPool(g_cnlCancelToutArg.pCnlDev->dummyReqMplId, g_cnlCancelToutArg.pDummy);
        CMN_unlockCpu(g_cnlCancelToutArg.pCnlDev->mngLockId);

        // release WAIT
        CMN_REL_WAIT(g_cnlCancelToutArg.pCnlDev->cancelWaitId);
    } else {
        CMN_unlockCpu(g_cnlCancelToutArg.pCnlDev->mngLockId);
    }

    return;
}
