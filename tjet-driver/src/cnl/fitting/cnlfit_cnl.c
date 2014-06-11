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
 *  @file     cnlfit_cnl.c
 *
 *  @brief    CNL fitting command/event handler.
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

#include "cnl_type.h"
#include "cnl_if.h"

#include "cnlwrap_if.h"
#include "cnlfit_upif.h"
#include "cnlfit.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define TYPE_TO_DIRECTION(x) (x == CNL_REQ_TYPE_SEND_REQ) ? \
    DATA_DIRECTION_OUT : DATA_DIRECTION_IN;


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static S_IO_CONTAINER *CNLFIT_allocIoContainer(S_CTRL_MGR *);
static void            CNLFIT_freeIoContainer(S_CTRL_MGR *, S_IO_CONTAINER *);

static T_CMN_ERR       CNLFIT_checkCmd(uint, int);
static void            CNLFIT_convertEvent(S_CTRL_MGR *, S_CNLWRAP_EVENT *, S_IO_CONTAINER *);

// CNL request completion callback related functions
static void            CNLFIT_asyncCbk(S_CNL_CMN_REQ *, void *, void *);
static T_CMN_ERR       CNLFIT_cnlInit(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlClose(S_CTRL_MGR *,S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlConnect(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlWaitConnect(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlAccept(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlConfirm(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlRelease(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlSendData(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *, int);
static T_CMN_ERR       CNLFIT_cnlRecvData(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *, int);
static T_CMN_ERR       CNLFIT_cnlCancel(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *, int);
static T_CMN_ERR       CNLFIT_getEvent(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *, int);
static T_CMN_ERR       CNLFIT_stopEvent(S_CTRL_MGR *, int);
static T_CMN_ERR       CNLFIT_enablePort(S_CTRL_MGR *);
static T_CMN_ERR       CNLFIT_disablePort(S_CTRL_MGR *);
static T_CMN_ERR       CNLFIT_disableComp(S_CTRL_MGR *);
static T_CMN_ERR       CNLFIT_cnlGetStats(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);
static T_CMN_ERR       CNLFIT_cnlPowerSave(S_CTRL_MGR *, S_CNLIO_ARG_BUCKET *);

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
 * Function   : CNLFIT_allocIoContainer
 *-----------------------------------------------------------------*/
/**
 * allocate IO Container.
 * @param   pCtrlMgr : the pointer to S_CTRL_MGR.
 * @return  pointer to allocated IO Container.
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
static S_IO_CONTAINER *
CNLFIT_allocIoContainer(S_CTRL_MGR *pCtrlMgr)
{

    T_CMN_ERR       retval;
    S_IO_CONTAINER *pIoCont;

    retval = CMN_getFixedMemPool(pCtrlMgr->iocontMplId, (void **)(&pIoCont), CMN_TIME_FEVR);
    if(retval != SUCCESS) {
        return NULL;
    }
    CMN_MEMSET(pIoCont, 0x00, sizeof(S_IO_CONTAINER));

    return pIoCont;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_freeIoContainer
 *-----------------------------------------------------------------*/
/**
 * free IO Container.
 * @param   pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param   pIoCont  : the pointer to S_IO_CONTAINER to free.
 * @return  nothing.
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
static void
CNLFIT_freeIoContainer(S_CTRL_MGR     *pCtrlMgr,
                       S_IO_CONTAINER *pIoCont)
{

    CMN_releaseFixedMemPool(pCtrlMgr->iocontMplId, pIoCont);
    return;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_checkCmd
 *-----------------------------------------------------------------*/
/**
 * check cmd acceptable for device type.
 * @param   cmd  : I/O control command.
 * @param   type : device type.
 * @return  SUCCESS (normally completion)
 * @return  ERR_BADPARM (invalid cmd)
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_checkCmd(uint cmd,
                int  type)
{

    switch(cmd) {
    case CNLWRAPIOC_INIT :
    case CNLWRAPIOC_CLOSE :
    case CNLWRAPIOC_CONNECT :
    case CNLWRAPIOC_WAIT_CONNECT : 
    case CNLWRAPIOC_ACCEPT :
    case CNLWRAPIOC_CONFIRM :
    case CNLWRAPIOC_RELEASE :
    case CNLWRAPIOC_ENABLE_PORT :
    case CNLWRAPIOC_DISABLE_PORT :
    case CNLWRAPIOC_GETSTATS :
    case CNLWRAPIOC_POWERSAVE :
        // command for ctrl side.
        if(type != CNLFIT_DEVTYPE_CTRL) {
            return ERR_BADPARM;
        }
        break;

    case CNLWRAPIOC_SUSPEND_CONF :
    case CNLWRAPIOC_GET_ADPT_ID :
        // command for adpt side.
        if(type != CNLFIT_DEVTYPE_ADPT) {
            return ERR_BADPARM;
        }
        break;

    case CNLWRAPIOC_GETEVENT :
    case CNLWRAPIOC_SYNCRECV :
    case CNLWRAPIOC_STOP_EVENT : 
    case CNLWRAPIOC_SENDDATA :
    case CNLWRAPIOC_RECVDATA :
    case CNLWRAPIOC_CANCEL :
        // common command
        break;
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_asyncCbk
 *-----------------------------------------------------------------*/
/**
 * CNL request completion callback function.
 * this callback is used for asynchronous(data) request.
 * @param     pCnlReq : the pointer to S_CNL_CMN_REQ
 * @param     pMgr    : the pointer to S_IO_MGR
 * @param     pCont   : the pointer to S_IO_CONTAINER
 * @return    nothing.
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static void
CNLFIT_asyncCbk(S_CNL_CMN_REQ *pCnlReq,
                void          *pMgr,
                void          *pCont)
{
  
    S_IO_MGR        *pIoMgr;
    S_IO_CONTAINER  *pIoCont;

    pIoMgr  = (S_IO_MGR *)pMgr;
    pIoCont = (S_IO_CONTAINER *)pCont;

    DBG_ASSERT((pCnlReq->type == CNL_REQ_TYPE_SEND_REQ) ||
               (pCnlReq->type == CNL_REQ_TYPE_RECEIVE_REQ));

    // re-use IO_CONTAINER.
    pIoCont->ioType = CNLFIT_IO_TYPE_REQ_COMP;
    CMN_lockCpu(pIoMgr->lockId);

    CMN_LIST_ADD_TAIL(&pIoMgr->eventList, pIoCont, S_IO_CONTAINER, list);
    CMN_unlockCpu(pIoMgr->lockId);
    pIoMgr->ioCbks.event.pioCbk(pIoMgr->ioCbks.event.pioArg);

    return;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_convertEvent
 *-----------------------------------------------------------------*/
/**
 * convert CNL and Wrapper event to I/O event.
 * @param  pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param  pEvent   : the pointer to the S_CNLWRAP_EVENT 
 * @param  pIoCont  : the pointer to the S_IO_CONTAINER
 * @return nothing.
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static void
CNLFIT_convertEvent(S_CTRL_MGR      *pCtrlMgr, 
                    S_CNLWRAP_EVENT *pEvent,
                    S_IO_CONTAINER  *pIoCont)
{

    S_CNL_CMN_EVT *pCnlEvt;
    S_CNL_CMN_REQ *pCnlReq;

    switch(pIoCont->ioType) {
    case CNLFIT_IO_TYPE_REQ_COMP :
        //
        // CNL (data)request completion
        //
        pCnlReq = &pIoCont->cnlReq;
        switch(pCnlReq->type) {
        case CNL_REQ_TYPE_SEND_REQ :
        case CNL_REQ_TYPE_RECEIVE_REQ :
            pEvent->type   = CNLWRAP_EVENT_DATA_REQ_COMP;
            pEvent->length = sizeof(S_CNLWRAP_DATA_REQ_COMP);

            pEvent->dataReqComp.status     = pCnlReq->status;
            pEvent->dataReqComp.length     = pCnlReq->dataReq.length;
            pEvent->dataReqComp.profileId  = pCnlReq->dataReq.profileId;
            pEvent->dataReqComp.fragmented = pCnlReq->dataReq.fragmented;
            pEvent->dataReqComp.direction  = TYPE_TO_DIRECTION(pCnlReq->type);
            pEvent->dataReqComp.requestId  = pCnlReq->id;
            break;
        default :
            DBG_ERR("Invalid request completion[0x%02x]\n", pCnlReq->type);
            pEvent->type   = CNLWRAP_EVENT_UNKNOWN;
            pEvent->length = 0;
            break;
        }

        DBG_INFO("convertEvent : ReqComp to EventReqComp.\n");
        break;

    case CNLFIT_IO_TYPE_CNL_EVENT :
        //
        // CNL event.
        //
        pCnlEvt = &pIoCont->cnlEvt;
        switch(pCnlEvt->type) {
        case CNL_EVT_TYPE_ERROR_IND :
            pEvent->type            = CNLWRAP_EVENT_ERROR_IND;
            pEvent->length          = sizeof(S_CNLWRAP_ERROR_IND);
            pEvent->errorInd.status = pCnlEvt->error;
            break;
        case CNL_EVT_TYPE_CONNECT_IND : 
            pEvent->type   = CNLWRAP_EVENT_CONNECT_IND;
            pEvent->length = sizeof(S_CNLWRAP_CONNECT_IND);
            CMN_MEMCPY(pEvent->connectInd.targetUID,
                       pCnlEvt->connectInd.targetUID,
                       CNL_UID_SIZE);
            CMN_MEMCPY(pEvent->connectInd.pclParam,
                       pCnlEvt->connectInd.pclParam,
                       CNL_PCL_PARAM_SIZE);

            CMN_MEMCPY(&pEvent->connectInd, &pCnlEvt->connectInd, pEvent->length);

            // store targetUID
            CMN_MEMCPY(pCtrlMgr->targetUID, pCnlEvt->connectInd.targetUID, CNL_UID_SIZE);
            break;
        case CNL_EVT_TYPE_ACCEPT_IND :
            pEvent->type   = CNLWRAP_EVENT_ACCEPT_IND;
            pEvent->length = sizeof(S_CNLWRAP_ACCEPT_IND);
            CMN_MEMCPY(pEvent->acceptInd.targetUID,
                       pCnlEvt->acceptInd.targetUID,
                       CNL_UID_SIZE);
            CMN_MEMCPY(pEvent->acceptInd.pclParam,
                       pCnlEvt->acceptInd.pclParam,
                       CNL_PCL_PARAM_SIZE);
            CMN_MEMCPY(pCtrlMgr->targetUID, pCnlEvt->acceptInd.targetUID, CNL_UID_SIZE);
            break;
        case CNL_EVT_TYPE_ACCEPT_CNF :
            pEvent->type   = CNLWRAP_EVENT_ACCEPT_CNF;
            pEvent->length = sizeof(S_CNLWRAP_ACCEPT_CNF);
            CMN_MEMCPY(&pEvent->acceptCnf.targetUID,
                       &pCnlEvt->acceptCnf.targetUID,
                       CNL_UID_SIZE);
            break;
        case CNL_EVT_TYPE_RELEASE_IND :
            pEvent->type   = CNLWRAP_EVENT_RELEASE_IND;
            pEvent->length = sizeof(S_CNLWRAP_RELEASE_IND);

            pEvent->releaseInd.causeOrigin = pCnlEvt->releaseInd.causeOrigin;
            pEvent->releaseInd.causeCode = pCnlEvt->releaseInd.causeCode;
            CMN_MEMCPY(&pEvent->releaseInd.pclParam,
                       &pCnlEvt->releaseInd.pclParam,
                       CNL_PCL_PARAM_SIZE);

            break;
        case CNL_EVT_TYPE_POWERSAVE_IND:
            pEvent->type   = CNLWRAP_EVENT_POWERSAVE_IND;
            pEvent->length = sizeof(S_CNLWRAP_POWERSAVE_IND);
            pEvent->powersaveInd.dormantPeriod = pCnlEvt->powersaveInd.dormantPeriod;
            pEvent->powersaveInd.awakePeriod = pCnlEvt->powersaveInd.awakePeriod;
        break;
        default : 
            DBG_ERR("Unknown CNL event[0x%02x]\n", pCnlEvt->type);
            pEvent->type   = CNLWRAP_EVENT_UNKNOWN;
            pEvent->length = 0;
            break;
        }

        DBG_INFO("convertEvent : CNLEvt[0x%x] to WRAPEvt[0x%x].\n",
                 pCnlEvt->type, pEvent->type);

        break;

    case CNLFIT_IO_TYPE_PORT_EVENT :
        //
        // Adapter Port event.
        //
        pEvent->type   = pIoCont->portEvt;
        pEvent->length = 0;

        DBG_INFO("convertEvent : PortEvent to PortEvent.\n");

        break;
    default :
        DBG_ERR("Unknown IO_Type[0x%02x]\n", pIoCont->ioType);
        pEvent->type   = CNLWRAP_EVENT_UNKNOWN;
        pEvent->length = 0;
        break;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlInit
 *-----------------------------------------------------------------*/
/**
 * CNL INIT command(CNLWRAPIOC_INIT) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlInit(S_CTRL_MGR         *pCtrlMgr,
               S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR       retval = SUCCESS;
    S_IO_CONTAINER *pIoCont;

    S_CNL_INIT_REQ     *pCnlInit;
    S_CNLWRAP_REQ_INIT *pWrapInit;

    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlInit : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlInit  = &pIoCont->cnlReq.initReq;
    pWrapInit = &pArg->req.init;

    // create INIT request
    pIoCont->cnlReq.type      = CNL_REQ_TYPE_INIT_REQ;
    pIoCont->cnlReq.pComplete = NULL; // SyncRequest.
    pIoCont->cnlReq.status    = CNL_SUCCESS;

    pCnlInit->liccVersion     = 0;   // ignored
    pCnlInit->mux             = 0;   // ignored
    // copy Request ownUID
    CMN_MEMCPY(pCnlInit->ownUID, pWrapInit->ownUID, CNL_UID_SIZE);
    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlInit : CNL_INIT.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pWrapInit->status = pIoCont->cnlReq.status;
    CMN_MEMCPY(pWrapInit->ownUID, pCnlInit->ownUID, CNL_UID_SIZE);

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlClose
 *-----------------------------------------------------------------*/
/**
 * CNL CLOSE command(CNLWRAPIOC_CLOSE) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlClose(S_CTRL_MGR         *pCtrlMgr,
                S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR       retval = SUCCESS;
    S_IO_CONTAINER *pIoCont;
    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlClose : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    // create CLOSE request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_CLOSE_REQ;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;
    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlClose : CNL_CLOSE.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pArg->req.status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlConnect
 *-----------------------------------------------------------------*/
/**
 * CNL CONNECT command(CNLWRAPIOC_CONNECT) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlConnect(S_CTRL_MGR         *pCtrlMgr,
                  S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR              retval = SUCCESS;
    S_IO_CONTAINER        *pIoCont;

    S_CNL_CONNECT_REQ     *pCnlConnect;
    S_CNLWRAP_REQ_CONNECT *pWrapConnect;

    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlConnect : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlConnect  = &pIoCont->cnlReq.connectReq;
    pWrapConnect = &pArg->req.connect;

    // create CONNECT request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_CONNECT_REQ;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;

    // convert Wrap req to CNL req.
    pCnlConnect->specified = pWrapConnect->targetSpecified;
    CMN_MEMCPY(pCnlConnect->targetUID, pWrapConnect->targetUID, CNL_UID_SIZE);
    CMN_MEMCPY(pCnlConnect->pclParam, pWrapConnect->pclParam, CNL_PCL_PARAM_SIZE);

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlConnect : CNL_CONNECT.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pArg->req.connect.status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}

/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlWaitConnect
 *-----------------------------------------------------------------*/
/**
 * CNL WAIT_CONNECT(CNLWRAPIOC_WCONNECT) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlWaitConnect(S_CTRL_MGR         *pCtrlMgr,
                      S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR       retval = SUCCESS;
    S_IO_CONTAINER *pIoCont;
    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlWconnect : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    // create WAIT CONNECT request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_WAIT_CONNECT_REQ;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;
    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlWaitConnect : CNL_WAITCONNECT.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pArg->req.status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlAccept
 *-----------------------------------------------------------------*/
/**
 * CNL ACCEPT command (CNLWRAPIOC_ACCEPT) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlAccept(S_CTRL_MGR         *pCtrlMgr,
                 S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR              retval = SUCCESS;
    S_IO_CONTAINER        *pIoCont;
    S_CNL_ACCEPT_REQ      *pCnlAccept;
    S_CNLWRAP_REQ_ACCEPT  *pWrapAccept;

    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlAccept : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlAccept  = &pIoCont->cnlReq.acceptReq;
    pWrapAccept = &pArg->req.accept;

    // create ACCEPT request
    pIoCont->cnlReq.type      = CNL_REQ_TYPE_ACCEPT_REQ;
    pIoCont->cnlReq.pComplete = NULL; // SyncRequest.
    pIoCont->cnlReq.status    = CNL_SUCCESS;

    CMN_MEMCPY(pCnlAccept->targetUID, pCtrlMgr->targetUID, CNL_UID_SIZE);
    CMN_MEMCPY(pCnlAccept->pclParam, pWrapAccept->pclParam, CNL_PCL_PARAM_SIZE);

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlAccept : CNL_ACCEPT.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pWrapAccept->status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlConfirm
 *-----------------------------------------------------------------*/
/**
 * CNL ACCEPTRES command(CNLWRAPIOC_CONFIRM) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlConfirm(S_CTRL_MGR         *pCtrlMgr,
                  S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR       retval = SUCCESS;
    S_IO_CONTAINER *pIoCont;
    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlConfirm : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    // create ACCEPT RESPONSE request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_ACCEPT_RES;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;

    // wrap I/O has no parameter of data.
    CMN_MEMCPY(&pIoCont->cnlReq.acceptRes.targetUID,
               pCtrlMgr->targetUID, CNL_UID_SIZE);
    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlConfirm : CNL_ACCEPT.response failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pArg->req.status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlRelease
 *-----------------------------------------------------------------*/
/**
 * CNL RELEASE command(CNLWRAPIOC_RELEASE) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlRelease(S_CTRL_MGR         *pCtrlMgr,
                  S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR              retval = SUCCESS;
    S_IO_CONTAINER        *pIoCont;
    S_CNL_RELEASE_REQ     *pCnlRelease;
    S_CNLWRAP_REQ_RELEASE *pWrapRelease;
    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlRelease : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlRelease  = &pIoCont->cnlReq.releaseReq;
    pWrapRelease = &pArg->req.release;

    // create RELEASE request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_RELEASE_REQ;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;

    CMN_MEMCPY(pCnlRelease->pclParam, pWrapRelease->pclParam, CNL_PCL_PARAM_SIZE);

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlRelease : CNL_RELEASE.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pWrapRelease->status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlPowerSave
 *-----------------------------------------------------------------*/
/**
 * CNL POWERSAVE command(CNLWRAPIOC_POWERSAVE) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlPowerSave(S_CTRL_MGR         *pCtrlMgr,
                    S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR                retval = SUCCESS;
    S_IO_CONTAINER          *pIoCont;
    S_CNL_POWERSAVE_REQ     *pCnlPowersave;
    S_CNLWRAP_REQ_POWERSAVE *pWrapPowersave;
    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlPowersave : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlPowersave  = &pIoCont->cnlReq.powersaveReq;
    pWrapPowersave = &pArg->req.powersave;

    // create POWERSAVE request
    pIoCont->cnlReq.type                = CNL_REQ_TYPE_POWERSAVE_REQ;
    pIoCont->cnlReq.pComplete           = NULL; // SyncRequest.
    pIoCont->cnlReq.status              = CNL_SUCCESS;

    // convert Wrap req to CNL req.
    pCnlPowersave->dormantPeriod = pWrapPowersave->dormantPeriod;
    pCnlPowersave->awakePeriod   = pWrapPowersave->awakePeriod;
    pCnlPowersave->keepAlive     = pWrapPowersave->keepAlive;

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlPowersave : CNL_POWERSAVE.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pWrapPowersave->status = pIoCont->cnlReq.status;

EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlSendData
 *-----------------------------------------------------------------*/
/**
 * CNL SENDDATA command (CNLWRAPIOC_SENDDATA) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @param  type     : deveice type
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlSendData(S_CTRL_MGR         *pCtrlMgr,
                   S_CNLIO_ARG_BUCKET *pArg,
                   int                 type)
{

    T_CMN_ERR           retval = SUCCESS;
    S_IO_CONTAINER     *pIoCont;
    S_IO_MGR           *pIoMgr;
    S_CNL_DATA_REQ     *pCnlData;
    S_CNLWRAP_REQ_DATA *pWrapData;

    if(type == CNLFIT_DEVTYPE_CTRL) {
        pIoMgr = &pCtrlMgr->ioMgr;
    } else {
        pIoMgr = &pCtrlMgr->adptMgr.ioMgr;
    }

    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlSendData : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlData  = &pIoCont->cnlReq.dataReq;
    pWrapData = &pArg->req.data;

    // create SENDDATA request
    pIoCont->ioType            = CNLFIT_IO_TYPE_REQ;
    pIoCont->cnlReq.type       = CNL_REQ_TYPE_SEND_REQ;
    pIoCont->cnlReq.pComplete  = CNLFIT_asyncCbk; // AsyncRequest.
    pIoCont->cnlReq.pArg1      = (void *)pIoMgr;
    pIoCont->cnlReq.pArg2      = (void *)pIoCont;
    pIoCont->cnlReq.status     = CNL_SUCCESS;

    // convert request.
    pCnlData->profileId   = pWrapData->profileId;
    pCnlData->fragmented  = pWrapData->fragmented;
    pCnlData->length      = pWrapData->length;
    pCnlData->pData       = (void *)pWrapData->userBufAddr;

    pIoCont->cnlReq.id    = pWrapData->requestId;

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if((retval != SUCCESS) || (pIoCont->cnlReq.status != CNL_SUCCESS)) {
        DBG_ERR("cnlSendData : CNL_DATA.request failed[%d]\n", retval);

        CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

        goto EXIT;
    }

    // return back parameter
    pWrapData->status = pIoCont->cnlReq.status;

EXIT :
    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlRecvData
 *-----------------------------------------------------------------*/
/**
 * CNL RECVDATA command(CNLWRAPIOC_RECVDATA) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @param  type     : deveice type
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlRecvData(S_CTRL_MGR         *pCtrlMgr,
                   S_CNLIO_ARG_BUCKET *pArg,
                   int                 type)
{

    T_CMN_ERR           retval = SUCCESS;
    S_IO_CONTAINER     *pIoCont;
    S_IO_MGR           *pIoMgr;
    S_CNL_DATA_REQ     *pCnlData;
    S_CNLWRAP_REQ_DATA *pWrapData;
    
    if(type == CNLFIT_DEVTYPE_CTRL) {
        pIoMgr = &pCtrlMgr->ioMgr;
    } else {
        pIoMgr = &pCtrlMgr->adptMgr.ioMgr;
    }

    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlRecvData : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlData  = &pIoCont->cnlReq.dataReq;
    pWrapData = &pArg->req.data;

    // create RECVDATA request
    pIoCont->ioType            = CNLFIT_IO_TYPE_REQ;
    pIoCont->cnlReq.type       = CNL_REQ_TYPE_RECEIVE_REQ;
    pIoCont->cnlReq.pComplete  = CNLFIT_asyncCbk; // AsyncRequest.
    pIoCont->cnlReq.pArg1      = (void *)pIoMgr;
    pIoCont->cnlReq.pArg2      = (void *)pIoCont;
    pIoCont->cnlReq.status     = CNL_SUCCESS;

    // convert request.
    pCnlData->profileId   = pWrapData->profileId;
    pCnlData->fragmented  = pWrapData->fragmented;
    pCnlData->length      = pWrapData->length;
    pCnlData->pData       = (void *)pWrapData->userBufAddr;

    pIoCont->cnlReq.id    = pWrapData->requestId;

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if((retval != SUCCESS) || (pIoCont->cnlReq.status != CNL_SUCCESS)) {
        DBG_ERR("cnlRecvData : CNL_DATA.request failed[%d]\n", retval);

        CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

        goto EXIT;
    }

    // return back parameter
    pWrapData->status = pIoCont->cnlReq.status;

EXIT :
    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlCancel
 *-----------------------------------------------------------------*/
/**
 * CNL CANCEL command(CNLWRAPIOC_CANCEL) handler.
 * @param  pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @param  type     : data request to WRAP or ADPT
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (not found pending request)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlCancel(S_CTRL_MGR         *pCtrlMgr,
                 S_CNLIO_ARG_BUCKET *pArg,
                 int                 type)
{

    T_CMN_ERR          retval;
    T_CNL_REQ_ID       requestId;
    
    requestId = (ulong)(pArg->req.cancel.requestId);

    retval = pCtrlMgr->cnlOps.pCancel(pCtrlMgr->pCnlPtr, requestId);
    if(retval == SUCCESS) {
        pArg->req.cancel.status = CNL_SUCCESS;
    } else {
        pArg->req.cancel.status = CNL_ERR_NOOBJ;
    }
    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_getEvent
 *-----------------------------------------------------------------*/
/**
 * GETEVENT command(CNLWRAPIOC_GET_EVENT) handler.
 * @param  pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @param  type     : request to WRAP or ADPT
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (no event has occured)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_getEvent(S_CTRL_MGR         *pCtrlMgr,
                S_CNLIO_ARG_BUCKET *pArg,
                int                 type)
{

    S_IO_MGR        *pIoMgr;
    S_IO_CONTAINER  *pIoCont;

    if(type == CNLFIT_DEVTYPE_CTRL) {
        pIoMgr = &pCtrlMgr->ioMgr;
    } else {
        pIoMgr = &pCtrlMgr->adptMgr.ioMgr;
    }

    //
    // get from event queue.
    //
    CMN_lockCpu(pIoMgr->lockId);
    pIoCont = (S_IO_CONTAINER *)
        CMN_LIST_REMOVE_HEAD(&pIoMgr->eventList, S_IO_CONTAINER, list);
    CMN_unlockCpu(pIoMgr->lockId);

    if(pIoCont == NULL) {
        DBG_ERR("no event is queued.\n");
        return ERR_NOOBJ;
    }

    //
    // convert local event to I/F event.
    //
    CNLFIT_convertEvent(pCtrlMgr, &pArg->req.event, pIoCont);

    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_stopEvent
 *-----------------------------------------------------------------*/
/**
 * STOPEVENT command (CNLWRAPIOC_STOP_EVENT) handler.
 * @param  pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param  type     : request to WRAP or ADPT
 * @return SUCCESS (normally completion)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_stopEvent(S_CTRL_MGR    *pCtrlMgr,
                 int            type)
{

    S_IO_MGR        *pIoMgr;

    if(type == CNLFIT_DEVTYPE_CTRL) {
        pIoMgr = &pCtrlMgr->ioMgr;
    } else {
        pIoMgr = &pCtrlMgr->adptMgr.ioMgr;
    }

    pIoMgr->stopped = TRUE;
    pIoMgr->ioCbks.event.pioCbk(pIoMgr->ioCbks.event.pioArg);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_enablePort
 *-----------------------------------------------------------------*/
/**
 * ENABLE PORT command(CNLWRAPIOC_ENABLE_PORT)handler
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (already enabled adapter port or invalid type)
 * @note   assumed to with holding mutex.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_enablePort(S_CTRL_MGR *pCtrlMgr)
{

    T_CMN_ERR retval = SUCCESS;

    // check type.
    if(pCtrlMgr->adptMgr.state == ADPT_PORT_DISABLED) {
        // set adapter state enabled
        pCtrlMgr->adptMgr.state = ADPT_PORT_READY;
    } else {
        DBG_ERR("invalid adapter state[%d]\n", pCtrlMgr->adptMgr.state);
        retval = ERR_INVSTAT;
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_disablePort
 *-----------------------------------------------------------------*/
/**
 * DISABLEPORT command(CNLWRAPIOC_DISABLE_PORT) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR
 * @return SUCCESS     (normally completion)
 * @return ERR_NOMEM   (memory or resource is depleted)
 * @return ERR_INVSTAT (invalid state to disable adapter port)
 * @note   assumed to with holding mutex.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_disablePort(S_CTRL_MGR *pCtrlMgr)
{

    S_IO_CONTAINER *pIoCont;
    int             retval = SUCCESS;

    DBG_INFO("DisablePort called AdptPortState is [0x%x].\n", pCtrlMgr->adptMgr.state);

    switch(pCtrlMgr->adptMgr.state) {
    case ADPT_PORT_READY :
        //
        // adapter is not active. add DISABLE_COMP to self event queue
        //
        pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
        if(pIoCont == NULL) {
            DBG_ERR("DisablePort : alloc IoContainer failed\n");
            return ERR_NOMEM;
        }

        pIoCont->ioType  = CNLFIT_IO_TYPE_PORT_EVENT;
        pIoCont->portEvt = CNLWRAP_EVENT_SUSPEND_PORT_COMP;
        
        CMN_lockCpu(pCtrlMgr->ioMgr.lockId);
        CMN_LIST_ADD_TAIL(&pCtrlMgr->ioMgr.eventList, pIoCont, S_IO_CONTAINER, list);
        CMN_unlockCpu(pCtrlMgr->ioMgr.lockId);

        pCtrlMgr->ioMgr.ioCbks.event.pioCbk(pCtrlMgr->ioMgr.ioCbks.event.pioArg);

        pCtrlMgr->adptMgr.state = ADPT_PORT_DISABLED;

        break;

    case ADPT_PORT_ACTIVE :
        //
        // adapter is active. add DISABLE to self event queue
        //
        pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
        if(pIoCont == NULL) {
            DBG_ERR("DisablePort : alloc IoContainer failed\n");
            return ERR_NOMEM;
        }

        pIoCont->ioType  = CNLFIT_IO_TYPE_PORT_EVENT;
        pIoCont->portEvt = CNLWRAP_EVENT_SUSPEND_PORT;

        CMN_lockCpu(pCtrlMgr->adptMgr.ioMgr.lockId);
        CMN_LIST_ADD_TAIL(&pCtrlMgr->adptMgr.ioMgr.eventList, pIoCont, S_IO_CONTAINER, list);
        CMN_unlockCpu(pCtrlMgr->adptMgr.ioMgr.lockId);

        pCtrlMgr->adptMgr.ioMgr.ioCbks.event.pioCbk(pCtrlMgr->adptMgr.ioMgr.ioCbks.event.pioArg);

        pCtrlMgr->adptMgr.state = ADPT_PORT_SUSPENDING;

        break;

    // case ADPT_DISABLED :
    // case ADPT_SUSPENDING :
    default :
        // invalid state.
        DBG_ERR("invalid adapter state[%d]\n", pCtrlMgr->adptMgr.state);
        retval = ERR_INVSTAT;
        break;
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_disableComp
 *-----------------------------------------------------------------*/
/**
 * DISABLECOMP command(CNLWRAPIOC_DISABLE_COMP) handler.
 * @param  pCtrlMgr  : the pointer to S_CTRL_MGR
 * @return SUCCESS     (normally completion)
 * @return ERR_NOMEM   (memory or resource is depleted)
 * @return ERR_INVSTAT (invalid state to disable adapter port)
 * @note   assumed to with holding mutex.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_disableComp(S_CTRL_MGR *pCtrlMgr)
{

    T_CMN_ERR       retval = SUCCESS;
    S_IO_CONTAINER *pIoCont;

    if(pCtrlMgr->adptMgr.state != ADPT_PORT_SUSPENDING) {
        DBG_ERR("invalid adapter state[%d]\n", pCtrlMgr->adptMgr.state);
        return ERR_INVSTAT;
    }

    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("disableComp : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pIoCont->ioType  = CNLFIT_IO_TYPE_PORT_EVENT;
    pIoCont->portEvt = CNLWRAP_EVENT_SUSPEND_PORT_COMP;
        
    CMN_lockCpu(pCtrlMgr->ioMgr.lockId);
    CMN_LIST_ADD_TAIL(&pCtrlMgr->ioMgr.eventList, pIoCont, S_IO_CONTAINER, list);
    CMN_unlockCpu(pCtrlMgr->ioMgr.lockId);

    pCtrlMgr->ioMgr.ioCbks.event.pioCbk(pCtrlMgr->ioMgr.ioCbks.event.pioArg);

    pCtrlMgr->adptMgr.state = ADPT_PORT_DISABLED;

    return retval;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlGetStats
 *-----------------------------------------------------------------*/
/**
 * CNL GETSTATS command (CNLWRAPIOC_GETSTATS) handler.
 * @param  pCtrlMgr : the pointer to S_CTRL_MGR.
 * @param  pArg     : the pointer to S_CNLIO_ARG_BUCKET.
 * @return SUCCESS (normally completion)
 * @return ERR_NOMEM (memory or resource is depleted)
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_cnlGetStats(S_CTRL_MGR         *pCtrlMgr,
                   S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR        retval = SUCCESS;
    S_IO_CONTAINER  *pIoCont;
    S_CNL_STATS     *pCnlStats;
    S_CNLWRAP_STATS *pWrapStats;

    
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("cnlGetStats : alloc IoContainer failed\n");
        return ERR_NOMEM;
    }

    pCnlStats  = &pIoCont->cnlReq.stats;
    pWrapStats = &pArg->req.stats;

    // create GET STATS request
    pIoCont->cnlReq.type      = CNL_REQ_TYPE_GET_STATS;
    pIoCont->cnlReq.pComplete = NULL; // SyncRequest.
    pIoCont->cnlReq.status    = CNL_SUCCESS;

    retval = pCtrlMgr->cnlOps.pRequest(pCtrlMgr->pCnlPtr, &pIoCont->cnlReq);
    if(retval != SUCCESS) {
        DBG_ERR("cnlGetStats : CNL_ACCEPT.request failed[%d]\n", retval);
        goto EXIT;
    }

    // return back parameter
    pWrapStats->status = pIoCont->cnlReq.status;
    pWrapStats->RSSI   = pIoCont->cnlReq.stats.RSSI;


EXIT :
    CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);

    return retval;

}


/////////////////////////////////////////////////////////////////////
// CNLFIT external functions definition                            //
/////////////////////////////////////////////////////////////////////
/*-------------------------------------------------------------------
 * Function   : CNLFIT_eventCbk
 *-----------------------------------------------------------------*/
/**
 * Event handler from CNL layer.
 * @param   pMgr    : the pointer to S_CTRL_MGR
 * @param   pCnlEvt : the pointer to S_CNL_CMN_EVT
 * @return  SUCCESS (normally completion)
 * @return  -EINVAL (invalid cmd)
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
void
CNLFIT_eventCbk(void          *pMgr,
                S_CNL_CMN_EVT *pCnlEvt)
{

    // event handler.
    S_CTRL_MGR     *pCtrlMgr;
    S_IO_CONTAINER *pIoCont;

    DBG_INFO("EventCbk : new event occurred[0x%x]\n", pCnlEvt->type);

    pCtrlMgr = (S_CTRL_MGR *)pMgr;

    // allocate IoContainer
    pIoCont = CNLFIT_allocIoContainer(pCtrlMgr);
    if(pIoCont == NULL) {
        DBG_ERR("EventCbk : alloc IoContainer failed\n");
        return;
    }

    // convert CNL Evt to WrapEvt.
    CMN_MEMCPY(&pIoCont->cnlEvt, pCnlEvt, sizeof(S_CNL_CMN_EVT));

    // add EventQueue
    pIoCont->ioType = CNLFIT_IO_TYPE_CNL_EVENT;
    
    CMN_lockCpu(pCtrlMgr->ioMgr.lockId);
    CMN_LIST_ADD_TAIL(&pCtrlMgr->ioMgr.eventList, pIoCont, S_IO_CONTAINER, list);
    CMN_unlockCpu(pCtrlMgr->ioMgr.lockId);

    pCtrlMgr->ioMgr.ioCbks.event.pioCbk(pCtrlMgr->ioMgr.ioCbks.event.pioArg);

    DBG_INFO("EventCbk : Done.\n");

    return;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_clearAllEvent
 *-----------------------------------------------------------------*/
/**
 * clear events.
 * @param  pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param  pIoMgr   : the pointer to the S_IO_MGR
 * @return nothing.
 * @note   nothing.
 */
/*-----------------------------------------------------------------*/
void
CNLFIT_clearAllEvent(S_CTRL_MGR *pCtrlMgr,
                     S_IO_MGR   *pIoMgr)
{

    S_IO_CONTAINER *pIoCont;

    CMN_lockCpu(pIoMgr->lockId);
    while(1) {
        pIoCont = 
            (S_IO_CONTAINER *)CMN_LIST_REMOVE_HEAD(&pIoMgr->eventList,
                                                   S_IO_CONTAINER,
                                                   list);
        if(pIoCont == NULL) {
            break;
        }
        CNLFIT_freeIoContainer(pCtrlMgr, pIoCont);
    }                                       
    CMN_unlockCpu(pIoMgr->lockId);

    return;

}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cnlRequest
 *-----------------------------------------------------------------*/
/**
 * CNLFIT I/O Control handler
 * @param   type : device type(CTRL or ADPT)
 * @param   pMgr : the pointer to S_CTRL_MGR
 * @param   cmd  : control command
 * @param   pArg : the pointer to S_CNLIO_ARG_BUCKET
 * @return  SUCCESS   (some event exists)
 * @return  ERR_NOOBJ (no event exists)
 * @return  ERR_INVSTAT (event is stopped)
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNLFIT_cnlRequest(int                 type,
                  S_CTRL_MGR         *pCtrlMgr,
                  uint                cmd,
                  S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR retval;

    retval = CNLFIT_checkCmd(cmd, type);
    if(retval != SUCCESS) {
        DBG_ERR("Ctrl : cmd[%s] is not allowd by[0x%x] side.\n",
                CNLFIT_cmdToString(cmd), type);
        return ERR_BADPARM;
    }

    switch(cmd) {
    case CNLWRAPIOC_INIT :
        retval = CNLFIT_cnlInit(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_CLOSE :
        retval = CNLFIT_cnlClose(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_CONNECT :
        retval = CNLFIT_cnlConnect(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_WAIT_CONNECT :
        retval = CNLFIT_cnlWaitConnect(pCtrlMgr, pArg);
        break;
        
    case CNLWRAPIOC_ACCEPT :
        retval = CNLFIT_cnlAccept(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_CONFIRM :
        retval = CNLFIT_cnlConfirm(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_RELEASE :
        retval = CNLFIT_cnlRelease(pCtrlMgr, pArg);
        break;

    case CNLWRAPIOC_SENDDATA :
        retval = CNLFIT_cnlSendData(pCtrlMgr, pArg, type);
        break;

    case CNLWRAPIOC_RECVDATA :
        retval = CNLFIT_cnlRecvData(pCtrlMgr, pArg, type);
        break;

    case CNLWRAPIOC_POWERSAVE :
        retval = CNLFIT_cnlPowerSave(pCtrlMgr, pArg);
        break;
    //
    // utility I/O control.
    //
    case CNLWRAPIOC_CANCEL :
        retval = CNLFIT_cnlCancel(pCtrlMgr, pArg, type);
        break;

    case CNLWRAPIOC_GETEVENT :
        retval = CNLFIT_getEvent(pCtrlMgr, pArg, type);
        break;

    case CNLWRAPIOC_STOP_EVENT :
        retval = CNLFIT_stopEvent(pCtrlMgr, type);
        break;

    //
    // adapter port management I/O control.
    // need to lock mutex.
    //
    case CNLWRAPIOC_ENABLE_PORT :
        CMN_LOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        retval = CNLFIT_enablePort(pCtrlMgr);
        CMN_UNLOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        break;

    case CNLWRAPIOC_DISABLE_PORT :
        CMN_LOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        retval = CNLFIT_disablePort(pCtrlMgr);
        CMN_UNLOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        break;

    case CNLWRAPIOC_SUSPEND_CONF :
        //
        // pArg is NULL indicates local-call
        // in this case mutex is already locked.
        //
        if(pArg != NULL) {
            CMN_LOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        }
        retval = CNLFIT_disableComp(pCtrlMgr);
        if(pArg != NULL) {
            CMN_UNLOCK_MUTEX(pCtrlMgr->ctrlDevMtxId);
        }
        break;

    case CNLWRAPIOC_GET_ADPT_ID : // obsolete.
        pArg->req.adptId = 1;
        retval = SUCCESS;
        break;

    case CNLWRAPIOC_SYNCRECV : // obsolete.
        retval = SUCCESS;
        break;

    case CNLWRAPIOC_GETSTATS :
        retval = CNLFIT_cnlGetStats(pCtrlMgr, pArg);
        break;

    default :
        DBG_ERR("unknown I/O control command[0x%x]\n", cmd);
        retval = ERR_BADPARM;
        break;
    }

    return retval;
}


