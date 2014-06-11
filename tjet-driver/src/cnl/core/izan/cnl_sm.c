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
 *  @file     cnl_sm.c
 *
 *  @brief    CNL state machine.
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


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static T_CNL_ERR CNL_changeState(S_CNL_DEV *, T_CNL_STATE);
static void      CNL_closeReset(S_CNL_DEV *);
static void      CNL_releaseReset(S_CNL_DEV *);
static void      CNL_purgeDataRequest(S_CNL_DEV *, T_CNL_ERR);
static void      CNL_purgeAllRequest(S_CNL_DEV *, S_CNL_ACTION *);
static void      CNL_completeDataRequest(S_CNL_DEV *, S_CNL_ACTION *, T_CNL_ERR);
static void      CNL_notifyEvent(S_CNL_DEV *, S_CNL_CMN_EVT *);
static T_CNL_ERR CNL_smHandleComplete(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR CNL_smHandleMngFrame(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR CNL_smHandleTimeout(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR CNL_smHandleError(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR CNL_smHandleUpperOrder(S_CNL_DEV *, S_CNL_ACTION *);
static T_CNL_ERR CNL_smHandleExtraEvent(S_CNL_DEV *, S_CNL_ACTION *);


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : CNL_changeState
 *-----------------------------------------------------------------*/
/**
 * change CNL state
 * @param  pCnlDev  : the pointer to the S_CNL_DEV
 * @param  newState : new CNL state.
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_changeState(S_CNL_DEV   *pCnlDev,
                T_CNL_STATE  newState)
{
    
    T_CNL_ERR retval;

    if(pCnlDev->cnlState == newState) {
        // state no change, do nothing.
        return CNL_SUCCESS;
    }

    retval = pCnlDev->pDeviceOps->pChangeState(pCnlDev, pCnlDev->cnlState, newState);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("state change failed[%d].\n", retval);
        return retval;
    }

    DBG_INFO("CNL state has been changed[0x%x] -> [0x%x].\n", pCnlDev->cnlState, newState);

    CMN_lockCpu(pCnlDev->mngLockId);

    pCnlDev->cnlState = newState;

    CMN_unlockCpu(pCnlDev->mngLockId);

    switch(CNLSTATE_TO_MAINSTATE(newState)) {
    case CNL_STATE_CLOSE :
        CNL_closeReset(pCnlDev);
        break;
    case CNL_STATE_SEARCH :
        if(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) != CNL_STATE_CLOSE) {
            // release occured.
            CNL_releaseReset(pCnlDev);
        }
        break;
    default : 
        // do nothing.
        break;
    }



    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_closeReset
 *-----------------------------------------------------------------*/
/**
 * reset parameters of CNL device when state is changed to CLOSE
 * @param  pCnlDev  : the pointer to the S_CNL_DEV
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_closeReset(S_CNL_DEV *pCnlDev)
{
    CNL_purgeDataRequest(pCnlDev, CNL_ERR_LINKDOWN);

    CMN_lockCpu(pCnlDev->mngLockId);

    pCnlDev->waitConnect   = FALSE;
    pCnlDev->crossover     = FALSE;
    pCnlDev->missCaccAck   = FALSE;
    pCnlDev->txReady       = FALSE;
    pCnlDev->txComp        = FALSE;
    pCnlDev->txSendingCsdu = 0;
    pCnlDev->rxReady       = FALSE;
    pCnlDev->rxReadyPid    = CNL_EMPTY_PID;
    pCnlDev->pwrState      = CNL_PWR_STATE_AWAKE;

    // clear event.
    pCnlDev->event.type      = 0;
    pCnlDev->event.recvdLicc = 0;
    pCnlDev->event.compReq   = 0;
    pCnlDev->event.timer     = 0;

    CNL_clearEventFilter(pCnlDev);

    CMN_unlockCpu(pCnlDev->mngLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_releaseReset
 *-----------------------------------------------------------------*/
/**
 * reset parameters of CNL device when state is changed to SEARCH
 * @param  pCnlDev  : the pointer to the S_CNL_DEV
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_releaseReset(S_CNL_DEV *pCnlDev)
{

    S_CNL_CMN_REQ *pReq;

    // complete all data request
    CNL_purgeDataRequest(pCnlDev, CNL_ERR_LINKDOWN);

    CMN_lockCpu(pCnlDev->mngLockId);

    pCnlDev->waitConnect   = FALSE;
    pCnlDev->crossover     = FALSE;
    pCnlDev->missCaccAck   = FALSE;
    pCnlDev->txReady       = FALSE;
    pCnlDev->txComp        = FALSE;
    pCnlDev->txSendingCsdu = 0;
    pCnlDev->rxReady       = FALSE;
    pCnlDev->rxReadyPid    = CNL_EMPTY_PID;
    pCnlDev->pwrState      = CNL_PWR_STATE_AWAKE;

    // clear event except ERROR_OCCURRED. and TX_READY
    pCnlDev->event.type      &= (CNL_EVENT_ERROR_OCCURRED | CNL_EVENT_TX_READY);
    pCnlDev->event.recvdLicc  = 0;
    pCnlDev->event.compReq    = 0;
    pCnlDev->event.timer      = 0;

    // clear Processing Action if needed
    if(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME) {
        DBG_INFO("Connection Released while handling SEND_MNG_FRAME Action, reset procAction.\n");
        pCnlDev->procAction = CNL_ACTION_NOP;
    }

    // complete Control request.
    pReq = (S_CNL_CMN_REQ *)CMN_LIST_LOOKUP(&pCnlDev->ctrlQueue);
    if(pReq) {
        switch(pReq->type) {
        case CNL_REQ_TYPE_RELEASE_REQ :
        case CNL_REQ_TYPE_ACCEPT_REQ :
        case CNL_REQ_TYPE_ACCEPT_RES :
            DBG_WARN("RELEASE.Indication occurred while executing request.\n");
            CNL_removeRequestFromCtrlQueue(pCnlDev, pReq);

            CMN_unlockCpu(pCnlDev->mngLockId);

            pReq->status = CNL_ERR_INVSTAT;
            CNL_completeRequest(pCnlDev, pReq);

            CMN_lockCpu(pCnlDev->mngLockId);

            break;
        case CNL_REQ_TYPE_INIT_REQ :
        case CNL_REQ_TYPE_CLOSE_REQ :
        case CNL_REQ_TYPE_CONNECT_REQ :
        case CNL_REQ_TYPE_WAIT_CONNECT_REQ :
        default : 
            DBG_ASSERT(0);
            break;
        }
    }

    CNL_clearEventFilter(pCnlDev);
    CMN_unlockCpu(pCnlDev->mngLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_purgeDataRequest
 *-----------------------------------------------------------------*/
/**
 * purge all queued data request
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  status  : requst status.
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_purgeDataRequest(S_CNL_DEV *pCnlDev,
                     T_CNL_ERR  status)
{

    S_LIST         compQueue;
    S_CNL_CMN_REQ *pReq;

    CMN_LIST_INIT(&compQueue);

    CMN_lockCpu(pCnlDev->mngLockId);

    CMN_LIST_SPLICE(&pCnlDev->txQueue,  &compQueue, S_CNL_CMN_REQ, list);
    CMN_LIST_SPLICE(&pCnlDev->rx0Queue, &compQueue, S_CNL_CMN_REQ, list);
    CMN_LIST_SPLICE(&pCnlDev->rx1Queue, &compQueue, S_CNL_CMN_REQ, list);
    pCnlDev->txReqCnt  = 0;
    pCnlDev->rx0ReqCnt = 0;
    pCnlDev->rx1ReqCnt = 0;

    CMN_unlockCpu(pCnlDev->mngLockId);

    // complete all data request
    while(1) {
        pReq = CMN_LIST_REMOVE_HEAD(&compQueue, S_CNL_CMN_REQ, list);
        if(pReq == NULL) {
            break;
        }
        pReq->status = status;
        CNL_completeRequest(pCnlDev, pReq);
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_purgeAllRequest.
 *-----------------------------------------------------------------*/
/**
 * purge all queued ctrl/data request
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_purgeAllRequest(S_CNL_DEV    *pCnlDev,
                    S_CNL_ACTION *pAction)
{

    S_LIST         compQueue;
    S_CNL_CMN_REQ *pReq;

    CMN_LIST_INIT(&compQueue);

    CMN_lockCpu(pCnlDev->mngLockId);

    CMN_LIST_SPLICE(&pCnlDev->txQueue,  &compQueue, S_CNL_CMN_REQ, list);
    CMN_LIST_SPLICE(&pCnlDev->rx0Queue, &compQueue, S_CNL_CMN_REQ, list);
    CMN_LIST_SPLICE(&pCnlDev->rx1Queue, &compQueue, S_CNL_CMN_REQ, list);
    CMN_LIST_SPLICE(&pCnlDev->ctrlQueue, &compQueue, S_CNL_CMN_REQ, list);
    pCnlDev->txReqCnt  = 0;
    pCnlDev->rx0ReqCnt = 0;
    pCnlDev->rx1ReqCnt = 0;
    pCnlDev->ctrlReqCnt = 0;

    CMN_unlockCpu(pCnlDev->mngLockId);

    DBG_INFO("Purge Requests in Action.\n");

    //
    // if error occurred when handling action,
    // complete the action's request if exist.
    //
    if((pAction->pReq != NULL) && (pAction->pReq->state == CNL_REQ_COMPLETED)) {
        pAction->pReq->status = pAction->status;
        CNL_completeRequest(pCnlDev, pAction->pReq);
    }
    
    CNL_completeDataRequest(pCnlDev, pAction, pAction->status);

    
    //
    // complete all request in qurur.
    //
    DBG_INFO("Purge Requests in Queue.\n");

    while(1) {
        pReq = CMN_LIST_REMOVE_HEAD(&compQueue, S_CNL_CMN_REQ, list);
        if(pReq == NULL) {
            break;
        }
        pReq->status = pAction->status;
        CNL_completeRequest(pCnlDev, pReq);
    }


    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_completeDataRequest
 *-----------------------------------------------------------------*/
/**
 * complete all request chained on the compReq queue of Action
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @param  status  : request completion status.
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_completeDataRequest(S_CNL_DEV    *pCnlDev,
                        S_CNL_ACTION *pAction,
                        T_CNL_ERR     status)
{

    S_CNL_CMN_REQ *pReq;

    do {
        pReq = CMN_LIST_REMOVE_HEAD(&pAction->compQueue, S_CNL_CMN_REQ, list);
        if(pReq == NULL) {
            break;
        }
        pReq->status = status;
        DBG_INFO("complete data request[%u:0x%x]:status[%d],length[%u],pid[%u],fragment[%u]\n",
                 pReq->type, pReq->id, pReq->status, pReq->dataReq.length,
                 pReq->dataReq.profileId, pReq->dataReq.fragmented);
        CNL_completeRequest(pCnlDev, pReq);
    } while(1);
    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_notifyEvent
 *-----------------------------------------------------------------*/
/**
 * 
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pEvt    : the pointer to the S_CNL_CMN_EVT
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_notifyEvent(S_CNL_DEV *pCnlDev, S_CNL_CMN_EVT *pEvt)
{

    if((pCnlDev->pPclCbks != NULL) && (pCnlDev->pPclCbks->pCnlEvent != NULL))
        pCnlDev->pPclCbks->pCnlEvent(pCnlDev->pPclCbks->pArg, pEvt);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleComplete
 *-----------------------------------------------------------------*/
/**
 * CNL handle complete
 * @param  *pCnlDev : the pointer to the S_CNL_DEV
 * @param  *pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleComplete(S_CNL_DEV    *pCnlDev, 
                     S_CNL_ACTION *pAction)
{

    T_CNL_ERR      retval;
    T_CNL_STATE    state;
    S_CNL_CMN_REQ *pReq;

    S_CNL_CMN_EVT  evt;

    pReq     = pAction->pReq;
    evt.type = 0;

    if((pReq != NULL) && (pReq->status != CNL_SUCCESS)) {
        // if request completed as error, do not change state.
        goto COMPLETE;
    }

    switch(pAction->compReq) {
    case CNL_COMP_NULL : 
        //
        // this Action is not genereted by CompReq event.
        // the request is utility or error occured while handling.
        // only complete it.
        //
        goto COMPLETE;
    case CNL_COMP_INIT :
        //
        // INIT.request completed
        // - change state to SEARCH
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_INIT);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_CLOSE :
        //
        // CLOSE.request completed
        // - change state to CLOSE
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_CLOSE, CNL_SUBSTATE_NULL);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_CLOSE);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_CONNECT_REQ :
        //
        // CONNECT.request completed
        // - stay current state(CONNECTION_REQUEST)
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_CONNECTION_REQUEST, CNL_SUBSTATE_NULL); // no change.
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_ACCEPT_REQ :
        //
        // ACCEPT.request completed
        // - stay current state(RESPONDER_RESPONSE)
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_RESPONDER_RESPONSE, CNL_SUBSTATE_NULL); // no change.
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_ACCEPT_RES :
        //
        // special case, completed sending ACK for C-Acc
        // - change state to INITIATOR_CONNECTED
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_INITIATOR_CONNECTED, CNL_SUBSTATE_CONNECTED);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_RELEASE_REQ :
        //
        // RELEASE.request completed
        // - change state to SEARCH
        // - complete the request
        //
        state = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        break;
    case CNL_COMP_SLEEP_REQ :
        //
        // POWERSAVE.request completed(internal request)
        // - change state to LOCAL_HIBERNATE
        //
        state = MAKE_CNLSTATE(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState), CNL_SUBSTATE_LOCAL_HIBERNATE);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        // clear event filter (C-Sleep/C-Wake received).
        CNL_clearEventFilter(pCnlDev);
        break;
    case CNL_COMP_WAKE_REQ :
        //
        // WAKE.request completed(internal request)
        // - change state to CONNECTED
        //
        state = MAKE_CNLSTATE(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState), CNL_SUBSTATE_CONNECTED);
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SEND_MNG_FRAME);
        pCnlDev->procAction = CNL_ACTION_NOP;
        // clear event filter (C-Sleep/C-Wake received).
        CNL_clearEventFilter(pCnlDev);
        break;
    case CNL_COMP_DATA_REQ :
        //
        // data request completion. no change state and no event.
        // - no change state
        // - complete the request(s) and return immediately
        //
        CNL_completeDataRequest(pCnlDev, pAction, pAction->status);
        return CNL_SUCCESS;

    case CNL_COMP_WAKE :
        pCnlDev->pwrState   = CNL_PWR_STATE_AWAKE;
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_WAKE);
        pCnlDev->procAction = CNL_ACTION_NOP;
        goto COMPLETE;

    case CNL_COMP_SLEEP :
        pCnlDev->pwrState   = CNL_PWR_STATE_HIBERNATE;
        DBG_ASSERT(pCnlDev->procAction == CNL_ACTION_SLEEP);
        pCnlDev->procAction = CNL_ACTION_NOP;
        goto COMPLETE;

    default :
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
    }

    // change CNL state.
    retval = CNL_changeState(pCnlDev, state);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionHandleComplete : state change failed[%d].\n", retval);
        return retval;
    }

COMPLETE :
    // complete the request if necessary
    if(pReq) {
        CNL_completeRequest(pCnlDev, pReq);
    }

    // notify event if necessary
    if(evt.type != 0) {
        CNL_notifyEvent(pCnlDev, &evt);
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleMngFrame
 *-----------------------------------------------------------------*/
/**
 * CNL handle management frame received.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleMngFrame(S_CNL_DEV    *pCnlDev, 
                     S_CNL_ACTION *pAction)
{

    T_CNL_ERR     retval = CNL_SUCCESS;
    T_CNL_STATE   state;
    u8            mainState;
    u8            subState;
    S_CNL_CMN_EVT evt = {0};

    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    subState  = CNLSTATE_TO_SUBSTATE(pCnlDev->cnlState);
    evt.type  = 0;

    switch(pAction->licc) {
    case CNL_EXT_LICC_C_RLS :
        //
        // C-Rls received, 
        // - change state to SEARCH
        // - notify RELEASE.indication(PCL origin)
        //
        state    = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        evt.type = CNL_EVT_TYPE_RELEASE_IND;
        evt.releaseInd.causeOrigin = CNL_RELEASE_ORIGIN_PCL;
        CMN_MEMCPY(&evt.releaseInd.pclParam, &pAction->liccInfo, CNL_PCL_PARAM_SIZE);
        break;
    case CNL_EXT_LICC_C_REQ :
        //
        // C-Req received,
        // - change state to ACCEPT_WAITING from SEARCH(stay if in CONNECTION_REQUEST state)
        // - notify CONNECT.indication 
        //
        if(mainState == CNL_STATE_SEARCH) {
            state = MAKE_CNLSTATE(CNL_STATE_ACCEPT_WAITING, CNL_SUBSTATE_NULL);
        } else {
            state = pCnlDev->cnlState;
        }
        evt.type = CNL_EVT_TYPE_CONNECT_IND;
        CMN_MEMCPY(&evt.connectInd.targetUID, &pAction->targetUID, CNL_UID_SIZE);
        CMN_MEMCPY(&evt.connectInd.pclParam,  &pAction->liccInfo,  CNL_PCL_PARAM_SIZE);
        break;
    case CNL_EXT_LICC_C_ACC :
        //
        // C-Acc received
        // - change state to RESPONSE_WAITING
        // - notify ACCEPT.indication
        //
        state    = MAKE_CNLSTATE(CNL_STATE_RESPONSE_WAITING, CNL_SUBSTATE_NULL);
        evt.type = CNL_EVT_TYPE_ACCEPT_IND;
        CMN_MEMCPY(&evt.acceptInd.targetUID, &pAction->targetUID, CNL_UID_SIZE);
        CMN_MEMCPY(&evt.acceptInd.pclParam,  &pAction->liccInfo,  CNL_PCL_PARAM_SIZE);
        break;
    case CNL_EXT_LICC_ACK_FOR_C_ACC :
        //
        // received ACK for C-Acc
        // - change state to RESPONDER_CONNECTED
        // - notify ACCEPT.confirm
        //
        state = MAKE_CNLSTATE(CNL_STATE_RESPONDER_CONNECTED, CNL_SUBSTATE_CONNECTED);
        evt.type = CNL_EVT_TYPE_ACCEPT_CNF;
        CMN_MEMCPY(&evt.acceptCnf.targetUID, &pAction->targetUID, CNL_UID_SIZE);
        break;
    case CNL_EXT_LICC_C_SLEEP :
        //
        // C-Sleep received
        // - change state to TARGET_SLEEP
        //
        if(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_RESPONDER_RESPONSE) {
            //
            // missing ACK for C-Acc, notify ACCEPT.confirm.
            //
            evt.type = CNL_EVT_TYPE_ACCEPT_CNF;
            CMN_MEMCPY(&evt.acceptCnf.targetUID, &pAction->targetUID, CNL_UID_SIZE);
            state = MAKE_CNLSTATE(CNL_STATE_RESPONDER_CONNECTED, CNL_SUBSTATE_TARGET_SLEEP);
        } else {
            // already connected.
            if(subState == CNL_SUBSTATE_LOCAL_HIBERNATE) {
                DBG_INFO("C-Sleep received but already LocalHibernate, no change state.\n");
                state = pCnlDev->cnlState;
            } else {
                state = MAKE_CNLSTATE(mainState, CNL_SUBSTATE_TARGET_SLEEP);
                evt.type = CNL_EVT_TYPE_POWERSAVE_IND;
                evt.powersaveInd.dormantPeriod = pAction->liccInfo[18];
                evt.powersaveInd.awakePeriod   = pAction->liccInfo[19];
            }
        }
        break;
    case CNL_EXT_LICC_C_WAKE :
        //
        // C-Wake received
        // - change state to CONNECTED
        //
        if(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_RESPONDER_RESPONSE) {
            //
            // missing ACK for C-Acc, notify ACCEPT.confirm.
            //
            evt.type = CNL_EVT_TYPE_ACCEPT_CNF;
            CMN_MEMCPY(&evt.acceptCnf.targetUID, &pAction->targetUID, CNL_UID_SIZE);
            state = MAKE_CNLSTATE(CNL_STATE_RESPONDER_CONNECTED, CNL_SUBSTATE_CONNECTED);
        } else {
            state = MAKE_CNLSTATE(mainState, CNL_SUBSTATE_CONNECTED);
        }
        break;

    case CNL_EXT_LICC_C_PROBE : // not used.
    default :
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
    }

    retval = CNL_changeState(pCnlDev, state);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionHandleMngFrame : state change failed[%d].\n", retval);
        return retval;
    }

    if(evt.type != 0) {
        CNL_notifyEvent(pCnlDev, &evt);
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleTimeout
 *-----------------------------------------------------------------*/
/**
 * CNL handle timeout.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleTimeout(S_CNL_DEV    *pCnlDev, 
                    S_CNL_ACTION *pAction)
{

    T_CNL_ERR     retval = CNL_SUCCESS;
    T_CNL_STATE   state;

    S_CNL_CMN_EVT evt;
    u8            causeOrigin = 0;
    u8            causeCode   = 0;

    switch(pAction->timer) {
    case CNL_T_CONNECT :
        //
        // T_Connect timer expired
        // - change state to SEARCH
        // - notify RELEASE.indication(origin CNL, cause CONNECT_TIMEOUT)
        //
        state       = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        causeOrigin = CNL_RELEASE_ORIGIN_CNL;
        causeCode   = CNL_RELEASE_CAUSE_CONNECT_TIMEOUT;
        break;
    case CNL_T_ACCEPT :
        //
        // T_Accept timer expired
        // - change state to SEARCH
        // - notify RELEASE.indication(origin CNL, cause ACCEPT_TIMEOUT)
        //
        state       = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        causeOrigin = CNL_RELEASE_ORIGIN_CNL;
        causeCode   = CNL_RELEASE_CAUSE_ACCEPT_TIMEOUT;
        break;
    case CNL_T_RETRY :
        //
        // T_Retry timer expired
        // - change state to SEARCH
        // - notify RELEASE.indication(origin CNL, cause RETRY_TIMEOUT)
        //
        state       = MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL);
        causeOrigin = CNL_RELEASE_ORIGIN_CNL;
        causeCode   = CNL_RELEASE_CAUSE_RETRY_TIMEOUT;
        break;
    case CNL_T_RESEND :
        //
        // T_Resend timer expired
        // - change state to TARGET_SLEEP
        // 
        state = MAKE_CNLSTATE(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState), CNL_SUBSTATE_TARGET_SLEEP);
        break;
    case CNL_T_KEEPALIVE : // not used.
    default : 
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
    }

    retval = CNL_changeState(pCnlDev, state);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionHandleTimer : state change failed[%d].\n", retval);
        return retval;
    }

    if(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_SEARCH) {
        //
        // notify RELEASE.indication
        //
        evt.type  = CNL_EVT_TYPE_RELEASE_IND;
        evt.releaseInd.causeOrigin = causeOrigin;
        evt.releaseInd.causeCode   = causeCode;
        CMN_MEMSET(&evt.releaseInd.pclParam, 0x00, CNL_PCL_PARAM_SIZE);
        CNL_notifyEvent(pCnlDev, &evt);
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleError
 *-----------------------------------------------------------------*/
/**
 * CNL handle error.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleError(S_CNL_DEV    *pCnlDev, 
                  S_CNL_ACTION *pAction)
{

    S_CNL_CMN_EVT       evt;

    //
    // choose error handling.
    //
    switch(pAction->status) {
    case CNL_ERR_HW_SUSPEND :
    case CNL_ERR_HW_RESUME :
        DBG_INFO("Don't need purge for suspend/resume!\n");
        break;
    case CNL_ERR_HOST_IO :
    case CNL_ERR_HW_PROT :
    case CNL_ERR_HW_ERROR1 :
    case CNL_ERR_HW_ERROR2 :
    case CNL_ERR_HW_ERROR3 :
    default :
        CNL_purgeAllRequest(pCnlDev, pAction);
        break;
    }

    //
    // notify event(ERROR_IND) to upper layer.
    //
    evt.type  = CNL_EVT_TYPE_ERROR_IND;
    evt.error = pAction->status;
    CNL_notifyEvent(pCnlDev, &evt);

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleUpperOrder
 *-----------------------------------------------------------------*/
/**
 * CNL handle upper layer order.(only state change.)
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleUpperOrder(S_CNL_DEV    *pCnlDev, 
                       S_CNL_ACTION *pAction)
{

    T_CNL_ERR   retval;
    T_CNL_STATE state;

    switch(pAction->order) {
    case CNL_REQ_TYPE_CONNECT_REQ :
        state = MAKE_CNLSTATE(CNL_STATE_CONNECTION_REQUEST, CNL_SUBSTATE_NULL);
        break;
    case CNL_REQ_TYPE_ACCEPT_REQ :
        state = MAKE_CNLSTATE(CNL_STATE_RESPONDER_RESPONSE, CNL_SUBSTATE_NULL);
        break;
    case CNL_REQ_TYPE_POWERSAVE_REQ :
        state = MAKE_CNLSTATE(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState), CNL_SUBSTATE_LOCAL_HIBERNATE);
        break;
    case CNL_REQ_TYPE_WAKE_REQ :
        state = MAKE_CNLSTATE(CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState), CNL_SUBSTATE_TARGET_SLEEP);
        break;
    default :
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
    }
    
    retval = CNL_changeState(pCnlDev, state);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ActionHandleOrder : state change failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_smHandleExtraEvent
 *-----------------------------------------------------------------*/
/**
 * CNL handle extra event.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
CNL_smHandleExtraEvent(S_CNL_DEV    *pCnlDev, 
                       S_CNL_ACTION *pAction)
{

    S_CNL_CMN_EVT       evt;
    S_CNL_RECV_CBK_MGR *pMgr;

    switch(pAction->extra) {
    case CNL_EXTRA_DEVICE_REMOVED  :
        //
        // if device is removed, complete all queued request and notify event.
        //
        pAction->status = CNL_ERR_DEVICE_REMOVED;
        CNL_purgeAllRequest(pCnlDev, pAction);

        //
        // clear cnlState.
        //
        pCnlDev->cnlState = MAKE_CNLSTATE(CNL_STATE_NULL, CNL_SUBSTATE_NULL);
        
        //
        // clear all parameters and event.
        //
        CNL_closeReset(pCnlDev);

        //
        // notify event(ERROR_IND) to upper layer.
        //
        evt.type  = CNL_EVT_TYPE_ERROR_IND;
        evt.error = CNL_ERR_DEVICE_REMOVED;
        CNL_notifyEvent(pCnlDev, &evt);
        break;

    case CNL_EXTRA_DATA_INDICATION :
        //
        // callback DataIndication.
        //
        pMgr = &(pCnlDev->recvCbk[pAction->profileId]);

        DBG_ASSERT(pMgr->pCbk);
        
        pMgr->pCbk(pMgr->pArg, pAction->profileId);
        break;
    default :
        DBG_ASSERT(0);
        break;
    }

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_stateMachine
 *-----------------------------------------------------------------*/
/**
 * CNL state machine
 * @param  *pCnlDev : the pointer to the S_CNL_DEV
 * @param  *pAction : the pointer to the S_CNL_ACTION
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CNL_ERR
CNL_stateMachine(S_CNL_DEV    *pCnlDev, 
                 S_CNL_ACTION *pAction)
{

    T_CNL_ERR retval = CNL_SUCCESS;


    switch(pAction->type) {
    case CNL_ACTION_HANDLE_COMPLETE :
        retval = CNL_smHandleComplete(pCnlDev, pAction);
        break;
    case CNL_ACTION_HANDLE_MNG_FRAME :
        retval = CNL_smHandleMngFrame(pCnlDev, pAction);
        break;
    case CNL_ACTION_HANDLE_TIMEOUT :
        retval = CNL_smHandleTimeout(pCnlDev, pAction);
        break;
    case CNL_ACTION_HANDLE_UPPER_ORDER :
        retval = CNL_smHandleUpperOrder(pCnlDev, pAction);
        break;
    case CNL_ACTION_HANDLE_EXTRA_EVENT :
        retval = CNL_smHandleExtraEvent(pCnlDev, pAction);
        break;
    case CNL_ACTION_HANDLE_ERROR :
        goto ERR_HANDLER;

    case CNL_ACTION_INIT :
    case CNL_ACTION_CLOSE :
    case CNL_ACTION_WAKE :
    case CNL_ACTION_SLEEP :
    case CNL_ACTION_SEND_MNG_FRAME :
    case CNL_ACTION_SEND_DATA :
    case CNL_ACTION_RECEIVE_DATA :
        DBG_ERR("this Action is not for StateMachine[0x%x].\n", pAction->type);
        return CNL_ERR_BADPARM;
    default :
        DBG_ERR("Unknown Action in StateMachine[0x%x].\n", pAction->type);
        return CNL_ERR_BADPARM;
    }

    if(retval != CNL_SUCCESS) {
        DBG_ERR("handling Action[%x] failed[%d]. convert to ActionHandleError.\n",
                pAction->type, retval);
        pAction->type   = CNL_ACTION_HANDLE_ERROR;
        pAction->status = retval;
    }

ERR_HANDLER:
    if(pAction->type == CNL_ACTION_HANDLE_ERROR) {
        CNL_smHandleError(pCnlDev, pAction);
    }

    return retval;
}
