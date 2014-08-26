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
 *  @file     cnlfit.c
 *
 *  @brief    CNL fitting driver main source.
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

#include "cmn_rsc.h"
#include "cmn_rsc2.h"

#include "cnl_type.h"
#include "cnl_if.h"
#include "cnl_err.h"

#include "cnlwrap_if.h"
#include "cnlfit_upif.h"
#include "cnlfit.h"

#include "cnlup_if.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.1";


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static T_CMN_ERR    CNLFIT_openCtrl(void **, int,  S_CNLIO_FIT_CBKS *);
static T_CMN_ERR    CNLFIT_openAdpt(void **, int,  S_CNLIO_FIT_CBKS *);
static T_CMN_ERR    CNLFIT_closeCtrl(S_CTRL_MGR *, int);
static T_CMN_ERR    CNLFIT_closeAdpt(S_ADPT_MGR *, int);

/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static S_CTRL_MGR *g_pCtrlTable[CNLFIT_DEV_NUM];
static u8          g_ctrlDevMtxId    = CNLFIT_DEV_MTX_ID;
static u8          g_ctrlWaitAdptId  = CNLFIT_WAIT_ADPT_ID;
static u8          g_ctrlDevMplId    = CNLFIT_DEV_MPL_ID;
static u8          g_ctrlIocontMplId = CNLFIT_IOCONT_MPL_ID;
static u8          g_ctrlLockId      = CNLFIT_CTRL_LOCK_ID;
static u8          g_adptLockId      = CNLFIT_ADPT_LOCK_ID;

/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : CNLFIT_openCtrl
 *-----------------------------------------------------------------*/
/**
 * open CNLFIT device controller side.
 * @param     pMgr  : the pointer to store the pointer of S_CTRL_MGR.
 * @param     id    : device index.
 * @param     pCbks : I/O layer callback pointer.
 * @return    SUCCESS (normally completion)
 * @return    ERR_INVSTAT(invalid state error)
 * @return    ERR_NOOBJ(not found the object to open)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_openCtrl(void             **pMgr,
                int                id,
                S_CNLIO_FIT_CBKS  *pCbks)
{

    T_CMN_ERR   retval;
    S_CTRL_MGR *pCtrlMgr;

    DBG_INFO("OpenCtrl : id = %d, g_pCtrlTable[%d] = %p\n", 
             id, id, g_pCtrlTable[id]);

    DBG_ASSERT((0<=id) && (id<CNLFIT_DEV_NUM));

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    if(g_pCtrlTable[id] == NULL) {
        DBG_ERR("OpenCtrl : CNL Device has not been registered yet.\n");
        retval = ERR_NOOBJ;
        goto EXIT;
    }

    pCtrlMgr = g_pCtrlTable[id];
    if(pCtrlMgr->state != CTRL_DEV_READY) {
        DBG_ERR("OpenCtrl : Invalid state[0x%x] to open\n", pCtrlMgr->state);
        retval = ERR_INVSTAT;
        goto EXIT;
    }

    // device is registered, open CNL device.
    pCtrlMgr->pclCbks.pArg      = pCtrlMgr;
    pCtrlMgr->pclCbks.pCnlEvent = CNLFIT_eventCbk;

    retval = pCtrlMgr->cnlOps.pOpen(&pCtrlMgr->pCnlPtr, &pCtrlMgr->pclCbks);
    if(retval != SUCCESS) {
        DBG_ERR("OpenCtrl : open CNL device failed[%d].\n", retval);
        goto EXIT;
    }

    pCtrlMgr->ioMgr.stopped = FALSE;
    CMN_LIST_INIT(&pCtrlMgr->ioMgr.requestList);
    CMN_LIST_INIT(&pCtrlMgr->ioMgr.eventList);

    // now completed preparing to access CNL, initialize parameters.
    pCtrlMgr->state = CTRL_DEV_ACTIVE;
    CMN_MEMCPY(&pCtrlMgr->ioMgr.ioCbks, pCbks, sizeof(S_CNLIO_FIT_CBKS));

    // add more if needed

    *pMgr = (void *)pCtrlMgr;
    DBG_INFO("OpenCtrl : Open Controller success.\n");

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : CNLFIT_openAdpt
 *-----------------------------------------------------------------*/
/**
 * open CNLFIT device adapter side.
 * @param     pMgr  : the pointer to store the pointer of S_ADPT_MGR.
 * @param     id    : device index.
 * @param     pCbks : I/O layer callback pointer.
 * @return    SUCCESS (normally completion)
 * @return    ERR_INVSTAT(invalid state error)
 * @return    ERR_NOOBJ(not found the object to open)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_openAdpt(void             **pMgr,
                int                id,
                S_CNLIO_FIT_CBKS  *pCbks)
{

    T_CMN_ERR   retval = SUCCESS;
    S_CTRL_MGR *pCtrlMgr;
    S_ADPT_MGR *pAdptMgr;

    DBG_INFO("OpenAdpt : id = %d, g_pCtrlTable[%d] = %p\n", 
             id, id, g_pCtrlTable[id]);

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    if(g_pCtrlTable[id] == NULL) {
        DBG_ERR("OpenAdpt : CNL Device has not been registered yet.\n");
        retval = ERR_NOOBJ;
        goto EXIT;
    }

    pCtrlMgr = g_pCtrlTable[id];
    if(pCtrlMgr->state != CTRL_DEV_ACTIVE) {
        DBG_ERR("OpenAdpt : Invalid state[0x%x] of CtrlMgr to open\n", pCtrlMgr->state);
        retval = ERR_INVSTAT;
        goto EXIT;
    }
    
    pAdptMgr = &pCtrlMgr->adptMgr;
    if(pAdptMgr->state != ADPT_PORT_READY) {
        DBG_ERR("OpenAdpt : Invalid state[0x%x] of AdptMgr ot open\n", pAdptMgr->state);
        retval = ERR_INVSTAT;
        goto EXIT;
    }

    // now completed preparing to access Adapter.
    pAdptMgr->state   = ADPT_PORT_ACTIVE;
    pAdptMgr->pParent = pCtrlMgr;

    pAdptMgr->ioMgr.stopped = FALSE;
    CMN_LIST_INIT(&pAdptMgr->ioMgr.requestList);
    CMN_LIST_INIT(&pAdptMgr->ioMgr.eventList);

    // add more if needed

    *pMgr = (void *)pAdptMgr;
    CMN_MEMCPY(&pAdptMgr->ioMgr.ioCbks, pCbks, sizeof(S_CNLIO_FIT_CBKS));

    DBG_INFO("OpenAdpt : Open AdapterPort success.\n");

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    return retval;
}




/*-------------------------------------------------------------------
 * Function : CNLFIT_closeCtrl
 *-----------------------------------------------------------------*/
/**
 * close CNLFIT device controller side.
 * @param     pCtrlMgr : the pointer to the S_CTRL_MGR
 * @param     id       : device index.
 * @return    SUCCESS (normally completion)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_closeCtrl(S_CTRL_MGR *pCtrlMgr,
                 int         id)
{

    T_CMN_ERR          retval = SUCCESS;
    S_CNLIO_ARG_BUCKET arg;
    u8                 free = FALSE;

    DBG_ASSERT((0<=id) && (id<CNLFIT_DEV_NUM));
    DBG_ASSERT(pCtrlMgr == g_pCtrlTable[id]);

    // force close and no-care error.
    CNLFIT_cnlRequest(CNLFIT_DEVTYPE_CTRL, pCtrlMgr, CNLWRAPIOC_CLOSE, &arg);

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    // check adapter state.
    if((pCtrlMgr->adptMgr.state == ADPT_PORT_ACTIVE) && 
       (pCtrlMgr->adptMgr.state == ADPT_PORT_SUSPENDING)) {
        // wait for closing Adapter.
        pCtrlMgr->adptMgr.state = ADPT_PORT_FORCE_CLOSE;
        CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);
        CMN_WAIT(pCtrlMgr->waitAdptId);
        CMN_LOCK_MUTEX(g_ctrlDevMtxId);
    }

    switch(pCtrlMgr->state) {
    case CTRL_DEV_ACTIVE :
        // set devstate to READY
        pCtrlMgr->state = CTRL_DEV_READY;
        break;
    case CTRL_DEV_GONE :
        // arleady unregistered, no more access.
        g_pCtrlTable[id] = NULL;
        free = TRUE;
        break;
    case CTRL_DEV_READY :
    default : 
        DBG_ERR("CloseCtrl : Invalid state[%d] of CtrlMgr to close\n", pCtrlMgr->state);
        retval = ERR_INVSTAT;
        goto EXIT;
    }

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    if(retval != SUCCESS) {
        return retval;
    }

    DBG_INFO("Call CNL Close.\n");
    pCtrlMgr->cnlOps.pClose(pCtrlMgr->pCnlPtr);

    if(free) {
        CMN_releaseFixedMemPool(g_ctrlDevMplId, pCtrlMgr);
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function : CNLFIT_closeAdpt
 *-----------------------------------------------------------------*/
/**
 * close CNLFIT device adapter side.
 * @param     pAdptMgr : the pointer to the S_ADPT_MGR
 * @param     id       : device index.
 * @return    SUCCESS (normally completion)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNLFIT_closeAdpt(S_ADPT_MGR *pAdptMgr,
                 int         id)
{

    T_CMN_ERR           retval = SUCCESS;
    S_CTRL_MGR         *pCtrlMgr;

    DBG_ASSERT(pAdptMgr != NULL);

    pCtrlMgr = pAdptMgr->pParent;

    DBG_ASSERT(pCtrlMgr == g_pCtrlTable[id]);

    // force clear all data request.

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    switch(pAdptMgr->state) {
    case ADPT_PORT_FORCE_CLOSE :
        // only release wainting.
        CMN_REL_WAIT(pCtrlMgr->waitAdptId);
        break;
    case ADPT_PORT_ACTIVE :
        pAdptMgr->state = ADPT_PORT_READY;
        break;
    case ADPT_PORT_SUSPENDING :
        // close adapter immediately after disabled.
        CNLFIT_cnlRequest(CNLFIT_DEVTYPE_ADPT, pCtrlMgr, CNLWRAPIOC_SUSPEND_CONF, NULL);
        break;
        
    case ADPT_PORT_READY :
    case ADPT_PORT_DISABLED :
    default :
        DBG_ERR("CloseAdpt : Invalid state[%d] of AdptMgr to close\n", pAdptMgr->state);
        retval = ERR_INVSTAT;
        goto EXIT;
    }

    CNLFIT_clearAllEvent(pCtrlMgr, &pAdptMgr->ioMgr);

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    return retval;
}


/*=================================================================*/
/* external functions for local use                                */
/*=================================================================*/
/*=================================================================*/
/* Interface functions for Upper layer(I/O Fitting)                */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : CNLFIT_open
 *-----------------------------------------------------------------*/
/**
 * CNLFIT open routine.
 * @param     type  : device type (ctrl or adpt)
 * @param     id    : device number.
 * @param     pMgr  : the pointer to store the pointer of manager.
 * @param     pCbks : I/O layer callback pointer.
 * @return    SUCCESS (normally completion)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNLFIT_open(int                type,
            int                id,
            void             **pMgr,
            S_CNLIO_FIT_CBKS  *pCbks)

{

    T_CMN_ERR  retval = SUCCESS;

    //
    // open ctrl/adpt
    //
    if(type == CNLFIT_DEVTYPE_CTRL) {
        retval = CNLFIT_openCtrl(pMgr, id, pCbks);
    } else {
        retval = CNLFIT_openAdpt(pMgr, id, pCbks);
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function : CNLFIT_close
 *-----------------------------------------------------------------*/
/**
 * CNLFIT close routine.
 * @param     type  : device type (ctrl or adpt)
 * @param     id    : device number.
 * @param     pMgr  : the pointer to the S_CTRL_MGR.
 * @return    SUCCESS (normally completion)
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNLFIT_close(int   type,
             int   id,
             void *pMgr)
{

    T_CMN_ERR   retval = SUCCESS;

    if(type == CNLFIT_DEVTYPE_CTRL) {
        retval = CNLFIT_closeCtrl((S_CTRL_MGR *)pMgr, id);
    } else {
        retval = CNLFIT_closeAdpt((S_ADPT_MGR *)pMgr, id);
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_searchEvent
 *-----------------------------------------------------------------*/
/**
 * search event functions.
 * @param   pMgr    : the pointer to S_CTRL_MGR.
 * @param   pCnlEvt : the pointer to S_CNL_CMN_EVT
 * @return  SUCCESS   (some event exists)
 * @return  ERR_NOOBJ (no event exists)
 * @return  ERR_INVSTAT (event is stopped)
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNLFIT_searchEvent(int   type,
                   void *pMgr)
{
  
    S_IO_MGR *pIoMgr ;

    if(type == CNLFIT_DEVTYPE_CTRL) {
        pIoMgr = &((S_CTRL_MGR *)pMgr)->ioMgr;
    } else {
        pIoMgr = &((S_ADPT_MGR *)pMgr)->ioMgr;
    }

    if(pIoMgr->stopped == TRUE) {
        return ERR_INVSTAT;
    }
    
    if(CMN_LIST_IS_EMPTY(&pIoMgr->eventList)) {
        return ERR_NOOBJ;
    }

    DBG_INFO("SearchEvent : Done.\n");

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_ctrl
 *-----------------------------------------------------------------*/
/**
 * CNLFIT I/O Control handler
 * @param   type : device type(CTRL or ADPT)
 * @param   pMgr : the pointer to S_CTRL_MGR.
 * @param   cmd  : control command.
 * @param   pArg : the pointer to S_CNLIO_ARG_BUCKET
 * @return  SUCCESS   (some event exists)
 * @return  ERR_NOOBJ (no event exists)
 * @return  ERR_INVSTAT (event is stopped)
 * @note    nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNLFIT_ctrl(int                 type,
            void               *pMgr,
            uint                cmd,
            S_CNLIO_ARG_BUCKET *pArg)
{

    T_CMN_ERR retval =  SUCCESS;
    S_CTRL_MGR *pCtrlMgr;
    S_ADPT_MGR *pAdptMgr;

    DBG_INFO("Ctrl : called.[%s][0x%x]\n", CNLFIT_cmdToString(cmd), type);

    if(type == CNLFIT_DEVTYPE_CTRL) {
        pCtrlMgr = (S_CTRL_MGR *)pMgr;
    } else {
        pAdptMgr = (S_ADPT_MGR *)pMgr;
        pCtrlMgr = pAdptMgr->pParent;
    }

    retval = CNLFIT_cnlRequest(type, pCtrlMgr, cmd, pArg);
    if(retval != SUCCESS) {
        DBG_ERR("Ctrl : cnlRequest for command[%s][0x%x] failed[%d].\n",
                CNLFIT_cmdToString(cmd), type, retval);
    }

    return retval;
}


/*=================================================================*/
/* Interface functions for Lower layer(CNL)                        */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function   : CNLUP_registerCNL
 *-----------------------------------------------------------------*/
/**
 * register CNL device to fitting module.
 * @param     pDev : the pointer to the CNL device.
 * @param     pOps : the pointer to the CNL Operations.
 * @return    SUCCESS    (normally completion) 
 * @return    ERR_NOMEM  (memory or resource is depleted.)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNLUP_registerCNL(void      *pDev,
                  S_CNL_OPS *pOps)
{

    T_CMN_ERR   retval = SUCCESS;
    int         i;
    u8          found = FALSE;
    S_CTRL_MGR *pCtrlMgr;

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    for(i=0; i<CNLFIT_DEV_NUM; i++) {
        if(g_pCtrlTable[i] == NULL) {
            found = TRUE;
            break;
        }
    }

    if(!found) {
        retval = ERR_NOMEM;
        goto EXIT;
    }
    
    retval = CMN_getFixedMemPool(g_ctrlDevMplId, (void **)(&pCtrlMgr), CMN_TIME_FEVR);
    if(retval != SUCCESS) {
        goto EXIT;
    }
    
    pCtrlMgr->pCnlPtr = pDev;
    CMN_MEMCPY(&pCtrlMgr->cnlOps, pOps, sizeof(S_CNL_OPS));
    
    pCtrlMgr->state                 = CTRL_DEV_READY;
    pCtrlMgr->iocontMplId           = g_ctrlIocontMplId;
    pCtrlMgr->ioMgr.lockId          = g_ctrlLockId;
    pCtrlMgr->adptMgr.ioMgr.lockId  = g_adptLockId;
    pCtrlMgr->waitAdptId            = g_ctrlWaitAdptId;
    pCtrlMgr->ctrlDevMtxId          = g_ctrlDevMtxId;

    pCtrlMgr->adptMgr.state         = ADPT_PORT_DISABLED;
    g_pCtrlTable[i]                 = pCtrlMgr;
    
    DBG_INFO("CNLFIT RegisterCNL success.\n");

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLUP_unregisterCNL
 *-----------------------------------------------------------------*/
/**
 * unregister CNL device to fitting module.
 * @param     pDev : the pointer to the CNL device.
 * @return    SUCCESS     (normally completion) 
 * @return    ERR_NOOBJ   (not found the object to unregister)
 * @return    ERR_INVSTAT (invalid state to unregister)
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNLUP_unregisterCNL(void  *pDev)
{

    T_CMN_ERR   retval = SUCCESS;
    int         i;
    u8          found = FALSE;
    S_CTRL_MGR *pCtrlMgr = NULL;

    CMN_LOCK_MUTEX(g_ctrlDevMtxId);

    for(i=0; i<CNLFIT_DEV_NUM; i++) {
        if(g_pCtrlTable[i]->pCnlPtr == pDev) {
            // found device.
            found = TRUE;
            pCtrlMgr = g_pCtrlTable[i];
            break;
        }
    }

    if(!found) {
        DBG_ERR("Not found CtrlDev to unregister.\n");
        retval = ERR_NOOBJ;
        goto EXIT;
    }

    switch(pCtrlMgr->state) {
    case CTRL_DEV_READY :
        // this device is not used, so clear it.
        CMN_releaseFixedMemPool(g_ctrlDevMplId, pCtrlMgr);
        g_pCtrlTable[i] = NULL;
        break;
    case CTRL_DEV_ACTIVE :
        // this device is used, so only change device state.
        pCtrlMgr->state = CTRL_DEV_GONE;
        break;
    case CTRL_DEV_GONE :
    default : 
      DBG_ERR("Invalid CtrlDev state[%d] to unregister\n", pCtrlMgr->state);
      retval = ERR_INVSTAT;
      goto EXIT;
    }

EXIT :
    CMN_UNLOCK_MUTEX(g_ctrlDevMtxId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_initMod
 *-----------------------------------------------------------------*/
/**
 * module initialization function.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
void 
CNLFIT_initMod(void)
{

    int i;
    // fill them.

    for(i=0; i<CNLFIT_DEV_NUM; i++) {
        g_pCtrlTable[i] = NULL;
    }

    CMN_INIT_MUTEX(g_ctrlDevMtxId);
    CMN_INIT_WAIT(g_ctrlWaitAdptId);

    CMN_createFixedMemPool(g_ctrlDevMplId,
                           0, 
                           CNLFIT_DEV_MPL_CNT,
                           CNLFIT_DEV_MPL_SIZE);

    CMN_createFixedMemPool(g_ctrlIocontMplId,
                           0, 
                           CNLFIT_IOCONT_MPL_CNT,
                           CNLFIT_IOCONT_MPL_SIZE);

    CMN_createCpuLock(g_ctrlLockId);
    CMN_createCpuLock(g_adptLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_cleanMod
 *-----------------------------------------------------------------*/
/**
 * module cleanup function.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
void
CNLFIT_cleanMod(void)
{

    // fill them.
    CMN_deleteSem(g_ctrlDevMtxId);
    CMN_deleteSem(g_ctrlWaitAdptId);
    CMN_deleteFixedMemPool(g_ctrlDevMplId);
    CMN_deleteFixedMemPool(g_ctrlIocontMplId);
    CMN_deleteCpuLock(g_ctrlLockId);
    CMN_deleteCpuLock(g_adptLockId);

    return;
}

MODULE_LICENSE("GPL v2");
MODULE_VERSION( DRIVER_VERSION );
