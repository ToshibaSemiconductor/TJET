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
 *  @file     cnl_if.c
 *
 *  @brief    definitions of CNL interface functions for upper layer.
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
#define CNL_CMD_TOUT_VAL_MS      5000  /* 5000ms */

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
typedef struct tagS_CNL_TOUT_ARG {
    S_CNL_DEV      *pCnlDev;
    S_CNL_CMN_REQ  *pReq;
}S_CNL_TOUT_ARG;


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static void      CNL_releaseRequestBlocking(S_CNL_CMN_REQ *, void *, void *);
static T_CMN_ERR CNL_checkRequestStateAndParam(S_CNL_DEV *, S_CNL_CMN_REQ *);

static void      CNL_releaseDiscardBuffer(S_CNL_CMN_REQ *, void *,void *);

static void      CNL_cmdToutCallback(unsigned long);

/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static S_CNL_TOUT_ARG g_cnlToutArg = {NULL, NULL};

/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : CNL_releaseRequestBlocking
 *-----------------------------------------------------------------*/
/**
 * release blocking request.
 * @param   pReq  : the pointer to the S_CNL_CMN_REQ.
 * @param   pArg1 : argument.
 * @param   pArg2 : not used.
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_releaseRequestBlocking(S_CNL_CMN_REQ *pReq,
                           void          *pArg1,
                           void          *pArg2)
{

    CMN_REL_WAIT((u8)((unsigned long)pArg1));

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_checkRequestStateAndParam
 *-----------------------------------------------------------------*/
/**
 * check CNL state and parameter is valid or not.
 * @param   pCnlDev : the pointer to the S_CNL_DEV
 * @param   pReq    : the pointer to the S_CNL_CMN_REQ
 * @return  SUCCESS     (state is valid for request)
 * @return  ERR_INVSTAT (state is invalid for request)
 * @return  ERR_BADPARM (invalid request parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNL_checkRequestStateAndParam(S_CNL_DEV     *pCnlDev,
                              S_CNL_CMN_REQ *pReq)
{

    u8 validState = FALSE;
    u8 validParam = FALSE;
    u8 mainState  = GET_MAINSTATE(pCnlDev->cnlState);
    S_CNL_CMN_REQ *pReqTmp = NULL;

    switch(pReq->type) {
    case CNL_REQ_TYPE_INIT_REQ :
        // CNL_STATE_CLOSE is valid.
        validState = (mainState == CNL_STATE_CLOSE) ? TRUE : FALSE;
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_CLOSE_REQ :
        // all state except for CNL_STATE_CLOSE is valid 
        validState = (mainState != CNL_STATE_CLOSE) ? TRUE : FALSE;
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_CONNECT_REQ :
        // CNL_STATE_SEARCH is valid.
        validState = (mainState == CNL_STATE_SEARCH) ? TRUE : FALSE;
        validParam = 
            ((pReq->connectReq.specified == CNL_SPECIFIED_UID) || 
             (pReq->connectReq.specified == CNL_UNSPECIFIED_UID)) ?
            TRUE : FALSE;
        break;

    case CNL_REQ_TYPE_WAIT_CONNECT_REQ :
        validState = (mainState == CNL_STATE_SEARCH) ? TRUE : FALSE;
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_ACCEPT_REQ :
        // CNL_STATE_CONNECTION_REQUEST and ACCEPT_WAITING is valid.
        validState = ((mainState == CNL_STATE_CONNECTION_REQUEST) ||
                      (mainState == CNL_STATE_ACCEPT_WAITING)) ? TRUE : FALSE;
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_ACCEPT_RES :
        // CNL_STATE_RESPONSE_WAITING is valid.
        validState = (mainState == CNL_STATE_RESPONSE_WAITING) ? TRUE : FALSE;
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_RELEASE_REQ :
        // all state except for CNL_STATE_CLOSE|CNL_STATE_SEARCH is valid
        validState = ((mainState != CNL_STATE_CLOSE) && 
                      (mainState != CNL_STATE_SEARCH)) ? TRUE : FALSE;
        if( ( mainState == CNL_STATE_SEARCH ) && ( pCnlDev->waitConnect != FALSE ) ) {
            validState = TRUE;
        }
        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_SEND_REQ :
        // CNL_STATE_CONNECTED is valid.
        validState = ((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
                      (mainState == CNL_STATE_RESPONDER_CONNECTED)) ? TRUE : FALSE;
        
        if(pReq->dataReq.length == 0) {
            break;
        }
        if((pReq->dataReq.profileId != CNL_PROFILE_ID_0) && 
           (pReq->dataReq.profileId != CNL_PROFILE_ID_1)) {
            break;
        }
        if((pReq->dataReq.fragmented != CNL_NOT_FRAGMENTED_DATA) && 
           (pReq->dataReq.fragmented != CNL_FRAGMENTED_DATA)) {
            break;
        }
        if(pReq->dataReq.pData == NULL) {
            break;
        }
        if ((pReq->id == CNL_DISCARD_RECVDATA_0) || 
            (pReq->id == CNL_DISCARD_RECVDATA_1)) {
            break;
        } 

        validParam = TRUE;
        break;

    case CNL_REQ_TYPE_RECEIVE_REQ :
        // CNL_STATE_CONNECTED is valid.
        validState = ((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
                      (mainState == CNL_STATE_RESPONDER_CONNECTED)) ? TRUE : FALSE;

        if(pReq->dataReq.length == 0) {
            break;
        }
        if((pReq->dataReq.profileId != CNL_PROFILE_ID_0) && 
           (pReq->dataReq.profileId != CNL_PROFILE_ID_1)) {
            break;
        }
        if((pReq->dataReq.fragmented != CNL_NOT_FRAGMENTED_DATA) && 
           (pReq->dataReq.fragmented != CNL_FRAGMENTED_DATA)) {
            break;
        }
        if(pReq->dataReq.pData == NULL) {
            break;
        }
        
        if(!IS_MULTI_OF_4B(pReq->dataReq.length)) {
            DBG_ERR("ReceiveReq length should be 4B * n but actual is [%u]\n", pReq->dataReq.length);
            break;
        }
        if ((pReq->id == CNL_DISCARD_RECVDATA_0) || 
            (pReq->id == CNL_DISCARD_RECVDATA_1)) {
            break;
        } 
        /* check discard request in RX0/RX1 queue */
        CMN_lockCpu(pCnlDev->mngLockId);
        if (pReq->dataReq.profileId == CNL_PROFILE_ID_0) {
            CMN_LIST_FOR(&pCnlDev->rx0Queue, pReqTmp, S_CNL_CMN_REQ, list) {
                if (pReqTmp->id == CNL_DISCARD_RECVDATA_0) {
                    CMN_unlockCpu(pCnlDev->mngLockId);
                    DBG_ERR("Already exist Discard Request in RX0 queue.\n");
                    return ERR_INVSTAT;
                }
            }
        }else if (pReq->dataReq.profileId == CNL_PROFILE_ID_1) {
            CMN_LIST_FOR(&pCnlDev->rx1Queue, pReqTmp, S_CNL_CMN_REQ, list) {
                if (pReqTmp->id == CNL_DISCARD_RECVDATA_1) {
                    CMN_unlockCpu(pCnlDev->mngLockId);
                    DBG_ERR("Already exist Discard Request in RX1 queue.\n");
                    return ERR_INVSTAT;
                }
            }
        }
        CMN_unlockCpu(pCnlDev->mngLockId);
        validParam = TRUE;
        break;
    case CNL_REQ_TYPE_POWERSAVE_REQ :
        // CNL_STATE_INITIATOR_CONNECTED and CNL_STATE_RESPONDER_CONNECTED is valid.
        validState = ((mainState == CNL_STATE_INITIATOR_CONNECTED) ||
                      (mainState == CNL_STATE_RESPONDER_CONNECTED)) ? TRUE : FALSE;
        validParam = TRUE;
        break;
    case CNL_REQ_TYPE_REG_DATA_IND :
    case CNL_REQ_TYPE_UNREG_DATA_IND :
    case CNL_REQ_TYPE_GET_STATS :
        // always ok
        validState = TRUE;
        validParam = TRUE;
        break;
    case CNL_REQ_TYPE_REG_PASSTHROUGH :
        validState = TRUE;
        if(pReq->regPT.length == 0) {
            break;
        }
        if((pReq->regPT.dir != CNL_REG_PASSTHROUGH_READ) && 
           (pReq->regPT.dir != CNL_REG_PASSTHROUGH_WRITE)) {
            break;
        }
        if(pReq->regPT.pData == NULL) {
            break;
        }
        validParam = TRUE;
        break;
    default :
        // unknown request type.
        DBG_ERR("unknown request type[%d]\n", pReq->type);
        return ERR_BADPARM;
    }

    if(validState == FALSE) {
        DBG_ERR("this request[%d] is not allowed in [0x%x] state.\n",
                pReq->type, pCnlDev->cnlState);
        return ERR_INVSTAT;
    }

    if(validParam == FALSE) {
        DBG_ERR("this request[%d] has invalid parameter.\n",pReq->type);
        return ERR_BADPARM;
    }

    // valid state.
    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_request
 *-----------------------------------------------------------------*/
/**
 * execute CNL common request.
 * @param   pDev : the pointer to the CNL device.
 * @param   pReq : the pointer to the CNL common request.
 * @return  SUCCESS     (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_request(void          *pDev,
            S_CNL_CMN_REQ *pReq)
{

    T_CMN_ERR  retval;
    S_CNL_DEV *pCnlDev = (S_CNL_DEV *)pDev;
    u8         block = FALSE;

    //
    // 1.device state check.
    //
    if(pCnlDev->devState != CNL_DEV_ACTIVE) {
        DBG_ERR("this device is not active[%d]\n", pCnlDev->devState);
        pReq->status = CNL_ERR_INVSTAT;
        return SUCCESS;
    }


    //
    // 2.check CNL state is valid for request. and select queue.
    //
    retval = CNL_checkRequestStateAndParam(pCnlDev, pReq);
    if(retval != SUCCESS) {
        switch(retval) {
        case ERR_BADPARM :
            pReq->status = CNL_ERR_BADPARM;
            break;
        case ERR_INVSTAT :
        default :
            pReq->status = CNL_ERR_INVSTAT;
        }
        return SUCCESS;
    }

    CMN_MEMSET(pReq->extData, 0x00, CMN_REQ_EXT_SIZE);

    // pComplete is set NULL means block request.
    if(pReq->pComplete == NULL) {
        // serialize Sync request.
        CMN_LOCK_MUTEX(pCnlDev->reqMtxId);

        /**** consider WaitID assignment ****/
        // wait for completion of request.
        pReq->pComplete = CNL_releaseRequestBlocking;
        pReq->pArg1     = (void *)((unsigned long)(pCnlDev->reqWaitId));
        pReq->pArg2     = NULL;
        /************************************/
        block           = TRUE;
    }
    
    // setting alarm timer for cnl request.
    if ((block) &&  
        (pReq->type != CNL_REQ_TYPE_SEND_REQ) &&  
        (pReq->type != CNL_REQ_TYPE_RECEIVE_REQ)) {
        g_cnlToutArg.pCnlDev = pCnlDev;
        g_cnlToutArg.pReq = pReq;
        retval = CMN_createAlarmTim(CNL_CMD_TOUT_TIM_ID, 0, NULL, CNL_cmdToutCallback);
        if (retval != SUCCESS) {
            DBG_ERR("CMN_createAlarmTim failed. retval:%d\n", retval);
        } else {
            retval = CMN_startAlarmTim(CNL_CMD_TOUT_TIM_ID, CNL_CMD_TOUT_VAL_MS);
            if (retval != SUCCESS) {
                DBG_ERR("CMN_startAlarmTim failed. retval:%d\n", retval);
                retval = CMN_deleteAlarmTim(CNL_CMD_TOUT_TIM_ID);
                if (retval != SUCCESS) {
                    DBG_ERR("CMN_deleteAlarmTim failed. retval:%d\n", retval);
                }
            }
        }
        DBG_INFO("CNL command timeout start\n");
    }

    //
    // 3.add request to the request depended queue and set device signaled.
    //
    retval = CNL_addRequest(pCnlDev, pReq);
    if(retval != SUCCESS) {
        if(block){
            if((pReq->type != CNL_REQ_TYPE_SEND_REQ) &&
               (pReq->type != CNL_REQ_TYPE_RECEIVE_REQ)) {
                // stop alarm timer for cnl request.
                retval = CMN_stopAlarmTim(CNL_CMD_TOUT_TIM_ID);
                if (retval != SUCCESS) {
                    DBG_ERR("CMN_stopAlarmTim failed. retval:%d\n", retval);
                }
                retval = CMN_deleteAlarmTim(CNL_CMD_TOUT_TIM_ID);
                if (retval != SUCCESS) {
                    DBG_ERR("CMN_deleteAlarmTim failed. retval:%d\n", retval);
                }
                g_cnlToutArg.pCnlDev = NULL;
                g_cnlToutArg.pReq = NULL;
            }
            CMN_UNLOCK_MUTEX(pCnlDev->reqMtxId);
        }
        pReq->status = CNL_ERR_QOVR;
        return SUCCESS;
    }

    if(block) {
        CMN_WAIT(pCnlDev->reqWaitId);
        if((pReq->type != CNL_REQ_TYPE_SEND_REQ) &&
           (pReq->type != CNL_REQ_TYPE_RECEIVE_REQ)) {
            // stop alarm timer for cnl request.
            retval = CMN_stopAlarmTim(CNL_CMD_TOUT_TIM_ID);
            if (retval != SUCCESS) {
                DBG_ERR("CMN_stopAlarmTim failed. retval:%d\n", retval);
            }
            retval = CMN_deleteAlarmTim(CNL_CMD_TOUT_TIM_ID);
            if (retval != SUCCESS) {
                DBG_ERR("CMN_deleteAlarmTim failed. retval:%d\n", retval);
            }
            g_cnlToutArg.pCnlDev = NULL;
            g_cnlToutArg.pReq = NULL;
        }
        retval = SUCCESS;
        CMN_UNLOCK_MUTEX(pCnlDev->reqMtxId);
    } else {
        pReq->status = CNL_SUCCESS;
    }

    return retval;
}


// utilities.
/*-------------------------------------------------------------------
 * Function : CNL_cancel
 *-----------------------------------------------------------------*/
/**
 * cancel the request pending in CNL
 * @param   pDev  : the pointer to the CNL device.
 * @param   reqId : request ID of target request.
 * @return  SUCCESS   (normally completion)
 * @return  ERR_NOOBJ (not found target request)
 * @return  ERR_NOMEM (memory or resource is depleted)
 * @return  ERR_INVSTAT (add dicard request failed)
 * @return  ERR_TIMEOUT (the timeout occured)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNL_cancel(void         *pDev,
           T_CNL_REQ_ID  reqId)
{

    T_CMN_ERR      retval = SUCCESS;
    S_CNL_DEV     *pCnlDev = (S_CNL_DEV *)pDev;
    S_CNL_CMN_REQ *pReq = NULL;
    S_CNL_CMN_REQ *pReqTmp = NULL;
    void          *pBuf = NULL;

    if ((reqId == CNL_DISCARD_RECVDATA_0) || (reqId == CNL_DISCARD_RECVDATA_1)) {
        retval = CMN_allocMem((void **)&pReq, sizeof(S_CNL_CMN_REQ));
        if ((retval != SUCCESS) || (pReq == NULL)) {
            DBG_ERR("CMN_allocMem failed. retval:%d, pReq:%p\n", retval, pReq);
            return ERR_NOMEM;
        }
        CMN_MEMSET(pReq, 0, sizeof(S_CNL_CMN_REQ));

        CMN_lockCpu(pCnlDev->mngLockId);
        switch(reqId) {
        case CNL_DISCARD_RECVDATA_0:
            /* check discard request in RX0 queue */
            CMN_LIST_FOR(&pCnlDev->rx0Queue, pReqTmp, S_CNL_CMN_REQ, list) {
                if (pReqTmp->id == CNL_DISCARD_RECVDATA_0) {
                    CMN_unlockCpu(pCnlDev->mngLockId);
                    DBG_INFO("Already exist Discard Request in RX0 queue.\n");
                    CMN_releaseMem(pReq);
                    return SUCCESS;
                }
            }
            pReq->dataReq.profileId = CNL_PROFILE_ID_0; 
            break;
        case CNL_DISCARD_RECVDATA_1:
            /* check discard request in RX1 queue */
            CMN_LIST_FOR(&pCnlDev->rx1Queue, pReqTmp, S_CNL_CMN_REQ, list) {
                if (pReqTmp->id == CNL_DISCARD_RECVDATA_1) {
                    CMN_unlockCpu(pCnlDev->mngLockId);
                    DBG_INFO("Already exist Discard Request in RX1 queue.\n");
                    CMN_releaseMem(pReq);
                    return SUCCESS;
                }
            }
            pReq->dataReq.profileId = CNL_PROFILE_ID_1; 
            break;
        default:
            /* NOP */
            break;
        } 
        CMN_unlockCpu(pCnlDev->mngLockId);


        retval = CMN_allocMem(&pBuf, CNL_DISCARD_DATA_BUF_SIZE);
        if ((retval != SUCCESS) || (pBuf == NULL)) {
            DBG_ERR("CMN_allocMem failed. retval:%d pBuf:%p, CMN_releaseMem(pReq:%p)\n", retval, pBuf, pReq);
            CMN_releaseMem(pReq);
            return ERR_NOMEM;
        }

        pReq->type = CNL_REQ_TYPE_RECEIVE_REQ;
        pReq->id = reqId;
        pReq->dataReq.length = CNL_DISCARD_DATA_BUF_SIZE; 
        pReq->dataReq.fragmented = CNL_NOT_FRAGMENTED_DATA; 
        pReq->dataReq.pData = pBuf; 
        pReq->pComplete = CNL_releaseDiscardBuffer;

        CMN_lockCpu(pCnlDev->mngLockId);
        retval = CNL_addRequestToRxQueue(pCnlDev, pReq);
        CMN_unlockCpu(pCnlDev->mngLockId);
        if (retval != CNL_SUCCESS) {
            CMN_releaseMem(pBuf);
            CMN_releaseMem(pReq);
            retval = ERR_INVSTAT;
        }
    } else {
        /* Normal CANCEL process */
        retval = CNL_cancelRequest(pCnlDev, reqId);
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_getState
 *-----------------------------------------------------------------*/
/**
 * get CNL internal state.
 * @param   pDev : the pointer to the CNL device.
 * @return  CNL STATE.
 * @note   
 */
/*-----------------------------------------------------------------*/
extern T_CNL_STATE
CNL_getState(void *pDev)
{

    return ((S_CNL_DEV *)pDev)->cnlState;

}

/*-------------------------------------------------------------------
 * Function : CNL_releaseDiscardBuffer
 *-----------------------------------------------------------------*/
/**
 * release discard buffer.
 * @param   pReq  : the pointer to the S_CNL_CMN_REQ.
 * @param   pArg1 : not used.
 * @param   pArg2 : not used.
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_releaseDiscardBuffer(S_CNL_CMN_REQ *pReq,
                         void          *pArg1,
                         void          *pArg2)
{

    if (pReq == NULL) {
        return;
    }
    if (pReq->dataReq.pData != NULL) {
        DBG_INFO("CMN_releaseMem(pData:%p)\n", pReq->dataReq.pData);
        CMN_releaseMem(pReq->dataReq.pData);
        pReq->dataReq.pData = NULL;
    }

    DBG_INFO("CMN_release(pReq:%p)\n", pReq);
    CMN_releaseMem(pReq);

    return;
}

/*-------------------------------------------------------------------
 * Function : CNL_cmdToutCallback
 *-----------------------------------------------------------------*/
/**
 * CNL command timeout callback function
 * @param   arg     :not used.
 * @return  nothing.
 * @note    use g_cnlToutArg 
 */
/*-----------------------------------------------------------------*/
static void CNL_cmdToutCallback(unsigned long arg)
{
    DBG_INFO("CNL_cmdToutCallback Called.\n");

    if ((g_cnlToutArg.pReq == NULL) || (g_cnlToutArg.pCnlDev == NULL)) {
        return;
    } 

    CMN_lockCpu(g_cnlToutArg.pCnlDev->mngLockId);
    if (CMN_IS_IN_LIST(&(g_cnlToutArg.pCnlDev->ctrlQueue), g_cnlToutArg.pReq, S_CNL_CMN_REQ, list)) {
        CNL_removeRequestFromCtrlQueue(g_cnlToutArg.pCnlDev, g_cnlToutArg.pReq);
        CMN_unlockCpu(g_cnlToutArg.pCnlDev->mngLockId);

        g_cnlToutArg.pReq->status = CNL_ERR_TIMEOUT;
        // release WAIT
        g_cnlToutArg.pReq->pComplete(g_cnlToutArg.pReq, g_cnlToutArg.pReq->pArg1, g_cnlToutArg.pReq->pArg2);
    } else { 
        CMN_unlockCpu(g_cnlToutArg.pCnlDev->mngLockId);
    }

    return;
}
