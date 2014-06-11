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
 *  @file     cnl_util.c
 *
 *  @brief    CNL utility functions.
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


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*=================================================================*/
/* Queue management utility                                        */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNL_addRequestToCtrlQueue
 *-----------------------------------------------------------------*/
/**
 * add common request to CTRL queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return CNL_SUCCESS  (normally completion)
 * @return CNL_ERR_QOVR (queue overflow)
 * @note
 */
/*-----------------------------------------------------------------*/
T_CNL_ERR
CNL_addRequestToCtrlQueue(S_CNL_DEV     *pCnlDev,
                          S_CNL_CMN_REQ *pReq)
{

    T_CMN_ERR retval;

    if(pCnlDev->ctrlReqCnt >= CNL_CTRL_QUEUE_SIZE) {
        DBG_ERR("CTRL queue overflow(current[%u]:max[%u])\n", 
                pCnlDev->ctrlReqCnt, CNL_CTRL_QUEUE_SIZE);
        retval = CNL_ERR_QOVR;
    } else {
        pReq->state = CNL_REQ_QUEUED;
        pCnlDev->ctrlReqCnt++;
        CMN_LIST_ADD_TAIL(&pCnlDev->ctrlQueue, pReq, S_CNL_CMN_REQ, list);
        retval = CNL_SUCCESS;
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_addRequestToTxQueue
 *-----------------------------------------------------------------*/
/**
 * add common request to TX queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return CNL_SUCCESS  (normally completion)
 * @return CNL_ERR_QOVR (queue overflow)
 * @note
 */
/*-----------------------------------------------------------------*/
T_CNL_ERR
CNL_addRequestToTxQueue(S_CNL_DEV     *pCnlDev,
                        S_CNL_CMN_REQ *pReq)
{

    T_CMN_ERR retval;

    if(pCnlDev->txReqCnt >= CNL_TX_QUEUE_SIZE) {
        DBG_ERR("TX queue overflow(current[%u]:max[%u])\n", 
                pCnlDev->txReqCnt, CNL_TX_QUEUE_SIZE);
        retval = CNL_ERR_QOVR;
    } else {
        pReq->state = CNL_REQ_QUEUED;
        pCnlDev->txReqCnt++;
        CMN_LIST_ADD_TAIL(&pCnlDev->txQueue, pReq, S_CNL_CMN_REQ, list);
        retval = CNL_SUCCESS;
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_addRequestToRxQueue
 *-----------------------------------------------------------------*/
/**
 * add common request to RX queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return CNL_SUCCESS  (normally completion)
 * @return CNL_ERR_QOVR (queue overflow)
 * @note
 */
/*-----------------------------------------------------------------*/
T_CNL_ERR
CNL_addRequestToRxQueue(S_CNL_DEV     *pCnlDev,
                        S_CNL_CMN_REQ *pReq)
{

    T_CMN_ERR retval;

    if(pReq->dataReq.profileId == CNL_PROFILE_ID_0) {
        if(pCnlDev->rx0ReqCnt >= CNL_RX_QUEUE0_SIZE) {
            DBG_ERR("RX0 queue overflow(current[%u]:max[%u])\n", 
                    pCnlDev->rx0ReqCnt, CNL_RX_QUEUE0_SIZE);
            retval = CNL_ERR_QOVR;
        } else {
            pReq->state = CNL_REQ_QUEUED;
            pCnlDev->rx0ReqCnt++;
            CMN_LIST_ADD_TAIL(&pCnlDev->rx0Queue, pReq, S_CNL_CMN_REQ, list);
            retval = CNL_SUCCESS;
        }
    } else {
        if(pCnlDev->rx1ReqCnt >= CNL_RX_QUEUE1_SIZE) {
            DBG_ERR("RX1 queue overflow(current[%u]:max[%u])\n", 
                    pCnlDev->rx1ReqCnt, CNL_RX_QUEUE1_SIZE);
            retval = CNL_ERR_QOVR;
        } else {
            pReq->state = CNL_REQ_QUEUED;
            pCnlDev->rx1ReqCnt++;
            CMN_LIST_ADD_TAIL(&pCnlDev->rx1Queue, pReq, S_CNL_CMN_REQ, list);
            retval = CNL_SUCCESS;
        }
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : CNL_removeRequestFromCtrlQueue
 *-----------------------------------------------------------------*/
/**
 * remove common request from CTRL queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return nothing.
 * @note
 */
/*-----------------------------------------------------------------*/
void
CNL_removeRequestFromCtrlQueue(S_CNL_DEV     *pCnlDev,
                               S_CNL_CMN_REQ *pReq)
{

    CMN_LIST_REMOVE(&pCnlDev->ctrlQueue, pReq, S_CNL_CMN_REQ, list);
    pCnlDev->ctrlReqCnt--;

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_removeRequestFromTxQueue
 *-----------------------------------------------------------------*/
/**
 * remove common request from TX queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return nothing.
 * @note
 */
/*-----------------------------------------------------------------*/
void
CNL_removeRequestFromTxQueue(S_CNL_DEV     *pCnlDev,
                             S_CNL_CMN_REQ *pReq)
{

    CMN_LIST_REMOVE(&pCnlDev->txQueue, pReq, S_CNL_CMN_REQ, list);
    pCnlDev->txReqCnt--;

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_removeRequestFromRxQueue
 *-----------------------------------------------------------------*/
/**
 * remove common request from RX queue.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pReq    : the pointer to the S_CNL_CMN_REQ
 * @return nothing.
 * @note
 */
/*-----------------------------------------------------------------*/
void
CNL_removeRequestFromRxQueue(S_CNL_DEV     *pCnlDev,
                             S_CNL_CMN_REQ *pReq)
{

    if(pReq->dataReq.profileId == CNL_PROFILE_ID_0) {
        CMN_LIST_REMOVE(&pCnlDev->rx0Queue, pReq, S_CNL_CMN_REQ, list);
        pCnlDev->rx0ReqCnt--;
    } else {
        CMN_LIST_REMOVE(&pCnlDev->rx1Queue, pReq, S_CNL_CMN_REQ, list);
        pCnlDev->rx1ReqCnt--;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_searchNextTxRequest
 *-----------------------------------------------------------------*/
/**
 * search next TX request to send.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return pointer to the Next Send request.
 * @note   
 */
/*-----------------------------------------------------------------*/
S_CNL_CMN_REQ *
CNL_searchNextSendRequest(S_CNL_DEV *pCnlDev)
{

    S_CNL_CMN_REQ  *pReq;
    S_DATA_REQ_EXT *pExt;

    CMN_LIST_FOR(&pCnlDev->txQueue, pReq, S_CNL_CMN_REQ, list) {
        pExt = (S_DATA_REQ_EXT *)(&pReq->extData);
        if(pReq->dataReq.length > pExt->position) {
            return pReq;
        }
    }

    return NULL;
}


/*-------------------------------------------------------------------
 * Function : CNL_searchCancelRequest
 *-----------------------------------------------------------------*/
/**
 * search next TX request to send.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  reqId   : request Id to indicate target request.
 * @return pointer to the cancel target request.
 * @note   
 */
/*-----------------------------------------------------------------*/
S_CNL_CMN_REQ *
CNL_searchCancelRequest(S_CNL_DEV     *pCnlDev,
                        T_CNL_REQ_ID   reqId)
{
    S_CNL_CMN_REQ *pReq;

    CMN_LIST_FOR(&pCnlDev->rx1Queue, pReq, S_CNL_CMN_REQ, list) {
        if(pReq->id == reqId) {
            return pReq;
        }
    }

    CMN_LIST_FOR(&pCnlDev->rx0Queue, pReq, S_CNL_CMN_REQ, list) {
        if(pReq->id == reqId) {
            return pReq;
        }
    }

    CMN_LIST_FOR(&pCnlDev->txQueue, pReq, S_CNL_CMN_REQ, list) {
        if(pReq->id == reqId) {
            return pReq;
        }
    }

    CMN_LIST_FOR(&pCnlDev->ctrlQueue, pReq, S_CNL_CMN_REQ, list) {
        if(pReq->id == reqId) {
            return pReq;
        }
    }

    return NULL;
}
