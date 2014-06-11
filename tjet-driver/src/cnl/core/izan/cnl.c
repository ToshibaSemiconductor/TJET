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
 *  @file     cnl.c
 *
 *  @brief    CNL core driver main source.
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

#include "cnlup_if.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define WAIT_CNLTASKSTOP_MAX_NUM    50
#define WAIT_CNLTASKSTOP_SLEEP_TIME  5


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static T_CMN_ERR  CNL_getCmnResources(void);
static T_CMN_ERR  CNL_putCmnResources(void);
static void       CNL_initDeviceParam(S_CNL_DEV *);

static T_CMN_ERR  CNL_registerCnlOps(S_CNL_DEV *);
static T_CMN_ERR  CNL_unregisterCnlOps(S_CNL_DEV *);

/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static u8  g_cnlTaskId       = CNL_TASK_ID;
static u8  g_cnlDevMtxId     = CNL_DEV_MTX_ID;
static u8  g_cnlDevMemPoolId = CNL_DEV_MPL_ID;
S_CNL_DEV *g_pCnlDevArray[CNL_DEV_MAX_NUM];         // management unnecessary 

static u8     g_sigDevLockId = CNL_DEV_SIG_LOC_ID;
static S_LIST g_sigDevList;

static S_CNL_OPS g_cnlOps = {
    .pOpen           = CNL_open,
    .pClose          = CNL_close,
    .pRequest        = CNL_request,
    .pCancel         = CNL_cancel,
    .pGetState       = CNL_getState,
};

static S_CNL_DEV *g_pCurrentCnlDev = NULL;

/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : CNL_registerCnlOps
 *-----------------------------------------------------------------*/
/**
 * register CNL operations to upper layer driver.
 * @param  pCnlDev : return pointer to the CNL device.
 * @return SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNL_registerCnlOps(S_CNL_DEV *pCnlDev)
{
    return CNLUP_registerCNL(pCnlDev, &g_cnlOps);
}


/*-------------------------------------------------------------------
 * Function : CNL_unregisterCnlOps
 *-----------------------------------------------------------------*/
/**
 * unregister CNL operations from upper layer driver.
 * @param  pCnlDev : return pointer to the CNL device.
 * @return SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNL_unregisterCnlOps(S_CNL_DEV *pCnlDev)
{
    return CNLUP_unregisterCNL(pCnlDev);
}


/*-------------------------------------------------------------------
 * Function : CNL_getCmnResources
 *-----------------------------------------------------------------*/
/**
 * get CNL common resources
 * @param  nothing.
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (state is invalid)
 * @return ERR_BADPARM (invalid parameter)
 * @return ERR_NOID    (invalid resource ID)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNL_getCmnResources()
{

    T_CMN_ERR retval;
    
    // create CNL task object.
    retval = CMN_createTask(g_cnlTaskId, 0, NULL, CNL_task, 0, 0, NULL);
    if(retval != SUCCESS) {
        DBG_ERR("create CNL task object failed\n");
        goto EXIT;
    }

    // create device management semaphore object.
    retval = CMN_INIT_MUTEX(g_cnlDevMtxId);
    if(retval != SUCCESS) {
        DBG_ERR("create CNL semaphore object failed\n");
        goto EXIT_1;
    }

    // create device mem pool
    retval = CMN_createFixedMemPool(g_cnlDevMemPoolId,
                                    0,
                                    CNL_DEV_MPL_CNT,
                                    CNL_DEV_MPL_SIZE);
    if(retval != SUCCESS) {
        DBG_ERR("create CNL Fixed memory pool object failed\n");
        goto EXIT_2;
    }

    // create signal device mutex.
    retval = CMN_createCpuLock(g_sigDevLockId);
    if(retval != SUCCESS) {
        DBG_ERR("create signal device mutex object failed.\n");
        goto EXIT_3;
    }

    return SUCCESS;

EXIT_3:
    CMN_deleteFixedMemPool(g_cnlDevMemPoolId);
EXIT_2:
    CMN_deleteSem(g_cnlDevMtxId);
EXIT_1:
    CMN_deleteTask(g_cnlTaskId);
EXIT:
    return retval;
}


/*-------------------------------------------------------------------
 * Function : CNL_putCmnResources
 *-----------------------------------------------------------------*/
/**
 * put CNL common resources
 * @param  nothing.
 * @return SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CMN_ERR
CNL_putCmnResources()
{

    // ignore errors.
    CMN_deleteCpuLock(g_sigDevLockId);
    CMN_deleteFixedMemPool(g_cnlDevMemPoolId);
    CMN_deleteSem(g_cnlDevMtxId);
    CMN_deleteTask(g_cnlTaskId);

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_initDeviceParam
 *-----------------------------------------------------------------*/
/**
 * initialize CNL device parameters.
 * @param  pCnlDev  : pointer to the CNL device structure.
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
CNL_initDeviceParam(S_CNL_DEV *pCnlDev)
{

    //
    // set initial parameters.
    //

    // device state.
    pCnlDev->devState = CNL_DEV_READY;
    CMN_LIST_INIT(&pCnlDev->devList);

    pCnlDev->cnlState    = MAKE_CNLSTATE(CNL_STATE_CLOSE, CNL_SUBSTATE_NULL);
    pCnlDev->liccVersion = CNL_LICC_VERSION_1;

    // request queue params.
    CMN_LIST_INIT(&pCnlDev->txQueue);
    CMN_LIST_INIT(&pCnlDev->rx0Queue);
    CMN_LIST_INIT(&pCnlDev->rx1Queue);
    CMN_LIST_INIT(&pCnlDev->ctrlQueue);
    pCnlDev->txReqCnt   = 0;
    pCnlDev->rx0ReqCnt  = 0;
    pCnlDev->rx1ReqCnt  = 0;
    pCnlDev->ctrlReqCnt = 0;

    CMN_LIST_INIT(&pCnlDev->cancelQueue);

    pCnlDev->waitConnect = FALSE;
    pCnlDev->crossover   = FALSE;
    pCnlDev->missCaccAck = FALSE;

    // interrupt informations.
    CMN_MEMSET(&pCnlDev->event, 0x00, sizeof(S_CNL_DEVICE_EVENT));

    // pass through all event.
    CNL_clearEventFilter(pCnlDev);

    pCnlDev->procAction    = CNL_ACTION_NOP;
    pCnlDev->txReady       = FALSE;
    pCnlDev->txComp        = FALSE;
    pCnlDev->txSendingCsdu = 0; //
    pCnlDev->rxReady       = FALSE;
    pCnlDev->rxReadyPid    = CNL_EMPTY_PID;

    pCnlDev->pwrState = CNL_PWR_STATE_AWAKE;
    pCnlDev->pPclCbks = NULL;
    pCnlDev->recvCbk[0].pCbk = NULL;
    pCnlDev->recvCbk[0].pArg = NULL;
    pCnlDev->recvCbk[0].call = FALSE;
    pCnlDev->recvCbk[1].pCbk = NULL;
    pCnlDev->recvCbk[1].pArg = NULL;
    pCnlDev->recvCbk[1].call = FALSE;

    return;
}


/**
 * CNL device management functions.
 * alloc/release 
 * register/unregister is called by Bus driver.
 * open/close is called by upper layer driver.
 */
/*-------------------------------------------------------------------
 * Function : CNL_signalDev
 *-----------------------------------------------------------------*/
/**
 * set CNL device signaled to wakeup task.
 * @param  pCnlDev : return pointer to the CNL device.
 * @return SUCCESS (normally completion)
 * @return ERR_INVID   (the ID is invalid)
 * @return ERR_NOOBJ   (the object don't exist)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_signalDev(S_CNL_DEV *pCnlDev)
{

    T_CMN_ERR retval;

    //
    // add event to signaled device queue.
    // if already queued, do nothing.
    // 
    CMN_lockCpu(g_sigDevLockId);
    if(!CMN_IS_IN_LIST(&g_sigDevList, pCnlDev, S_CNL_DEV, devList)) {
        CMN_LIST_ADD_TAIL(&g_sigDevList, pCnlDev, S_CNL_DEV, devList);
    }else{
        DBG_INFO("this device is already signaled.\n");
    }
    CMN_unlockCpu(g_sigDevLockId);

    retval = CMN_wakeupTask(g_cnlTaskId);
    if(retval != SUCCESS) {
        DBG_ERR("wakeupTask failed.\n");
        return retval;
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_getSignaledDev
 *-----------------------------------------------------------------*/
/**
 * get signaled CNL device.
 * @param  pCnlDev : return pointer to the CNL device.
 * @return SUCCESS     (normally completion)
 * @return ERR_RLWAIT  (force release during wait)
 * @return ERR_SYSTEM  (internal error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_getSignaledDev(S_CNL_DEV **pCnlDev)
{

    int           retval;
    S_CNL_DEV    *pTmpCnlDev;

    // clear current CnlDev
    CMN_lockCpu(g_sigDevLockId);
    g_pCurrentCnlDev = NULL;
    CMN_unlockCpu(g_sigDevLockId);

    //
    // get CNL device operation required.
    //
RETRY:
    retval = CMN_sleepTask(0);
    if(retval != 0) {
        DBG_ERR("terminateTask is called[%d]\n", retval);
        return retval; // exit task.
    }

    DBG_INFO("normally wakeup\n");

    CMN_lockCpu(g_sigDevLockId);
    pTmpCnlDev = CMN_LIST_REMOVE_HEAD(&g_sigDevList,
                                      S_CNL_DEV,
                                      devList);

    if (pTmpCnlDev != NULL) {
        // Set current CnlDev
        g_pCurrentCnlDev = pTmpCnlDev;
    }

    CMN_unlockCpu(g_sigDevLockId);

    if(pTmpCnlDev == NULL) {
        DBG_INFO("signaled same time.\n");
        goto RETRY;
    }

    *pCnlDev = pTmpCnlDev;

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_allocDevice
 *-----------------------------------------------------------------*/
/**
 * allocate and init CNL device.
 * @param  nothing.
 * @return the pointer to the allocated and initialized S_CNL_DEV
 * @note   
 */
/*-----------------------------------------------------------------*/
S_CNL_DEV *
CNL_allocDevice()
{

    T_CMN_ERR  retval;
    S_CNL_DEV *pCnlDev = NULL;

    //
    // allocate CNL device structure.
    //
    retval = CMN_getFixedMemPool(g_cnlDevMemPoolId, (void **)(&pCnlDev), CMN_TIME_FEVR);
    if((retval != SUCCESS) || (pCnlDev == NULL)) {
        DBG_ERR("allocate CNL device structure failed[%d].\n", retval);
        goto EXIT;
    }

    CMN_MEMSET(pCnlDev, 0x00, sizeof(S_CNL_DEV));

    //
    // get device dependent resources.
    //
    pCnlDev->mngLockId = CNL_DEV_MNG_LOC_ID;
    retval = CMN_createCpuLock(pCnlDev->mngLockId);
    if(retval != SUCCESS) {
        DBG_ERR("create device management lock object failed[%d].\n", retval);
        goto EXIT_1;
    }

    pCnlDev->reqMtxId = CNL_REQ_MTX_ID;
    retval = CMN_INIT_MUTEX(pCnlDev->reqMtxId);
    if(retval != SUCCESS) {
        DBG_ERR("create block request mutex object failed[%d].\n", retval);
        goto EXIT_2;
    }

    pCnlDev->reqWaitId = CNL_REQ_WAIT_ID;
    retval = CMN_INIT_WAIT(pCnlDev->reqWaitId);
    if(retval != SUCCESS) {
        DBG_ERR("create block request wait object failed[%d].\n", retval);
        goto EXIT_3;
    }

    pCnlDev->cancelWaitId = CNL_CANCEL_WAIT_ID;
    retval = CMN_INIT_WAIT(pCnlDev->cancelWaitId);
    if(retval != SUCCESS) {
        DBG_ERR("create block request wait object failed[%d].\n", retval);
        goto EXIT_4;
    }

    // create device mem pool
    pCnlDev->dummyReqMplId = CNL_DUMMY_REQ_MPL_ID;
    retval = CMN_createFixedMemPool(pCnlDev->dummyReqMplId,
                                    0,
                                    CNL_DUMMY_REQ_MPL_CNT,
                                    CNL_DUMMY_REQ_MPL_SIZE);
    if(retval != SUCCESS) {
        DBG_ERR("create DummyReq Fixed memory pool object failed\n");
        goto EXIT_5;
    }

    CNL_initDeviceParam(pCnlDev);

    return pCnlDev;

EXIT_5:
    CMN_deleteSem(pCnlDev->cancelWaitId);
EXIT_4:
    CMN_deleteSem(pCnlDev->reqWaitId);
EXIT_3:
    CMN_deleteCpuLock(pCnlDev->reqMtxId);
EXIT_2:
    CMN_deleteCpuLock(pCnlDev->mngLockId);
EXIT_1:
    CMN_releaseFixedMemPool(g_cnlDevMemPoolId, pCnlDev);
EXIT:

    DBG_ERR("alloc CNL device failed.\n");

    return NULL;

}


/*-------------------------------------------------------------------
 * Function : CNL_releaseDevice
 *-----------------------------------------------------------------*/
/**
 * release and init CNL device.
 * @return the pointer to the allocated and initialized S_CNL_DEV
 * @note   
 */
/*-----------------------------------------------------------------*/
void 
CNL_releaseDevice(S_CNL_DEV *pCnlDev)
{

    //
    // release CNL device structure.
    //
    DBG_ASSERT(pCnlDev != NULL);

    CMN_deleteFixedMemPool(pCnlDev->dummyReqMplId);
    CMN_deleteSem(pCnlDev->cancelWaitId);
    CMN_deleteSem(pCnlDev->reqWaitId);
    CMN_deleteSem(pCnlDev->reqMtxId);
    CMN_deleteCpuLock(pCnlDev->mngLockId);

    pCnlDev->pDeviceOps->pReleaseDeviceData(pCnlDev);

    CMN_releaseFixedMemPool(g_cnlDevMemPoolId, pCnlDev);

    return;

}


/*-------------------------------------------------------------------
 * Function : CNL_registerDevice
 *-----------------------------------------------------------------*/
/**
 * unregister CNL device
 * @param  pCnlDev : the pointer to the CNL device.
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (expire max devices)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_registerDevice(S_CNL_DEV *pCnlDev)  
{

    T_CMN_ERR retval;
    u8        devnum;

    CMN_LOCK_MUTEX(g_cnlDevMtxId);

    for(devnum=0; devnum<CNL_DEV_MAX_NUM; devnum++) {
        if(g_pCnlDevArray[devnum] == NULL) {
            // empty found.
            g_pCnlDevArray[devnum] = pCnlDev;
            pCnlDev->devnum = devnum;
            goto FOUND;
        }
    }
    CMN_UNLOCK_MUTEX(g_cnlDevMtxId);
    DBG_ERR("can't handle more device.\n");
    return ERR_NOOBJ;

FOUND:
    retval = CNL_registerCnlOps(pCnlDev);
    if(retval != SUCCESS) {
        DBG_ERR("register CNL operation to upper layer failed[%d].\n",
                retval);
    }

    CMN_UNLOCK_MUTEX(g_cnlDevMtxId);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_unregisterDevice
 *-----------------------------------------------------------------*/
/**
 * unregister CNL device.
 * @param  pCnlDev  : the pointer to the CNL device.
 * @return SUCCESS   (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_unregisterDevice(S_CNL_DEV *pCnlDev)  
{

    T_CMN_ERR          retval;
    S_CNL_DEVICE_EVENT event;

    int RetryCount = 0;

    CMN_LOCK_MUTEX(g_cnlDevMtxId);

    if(pCnlDev->devState == CNL_DEV_READY) {
        //
        // upper driver is not using this device.
        // unregister CNL operations from upper layer driver.
        // 
        retval = CNL_unregisterCnlOps((void *)pCnlDev);
        if(retval == SUCCESS) {
            g_pCnlDevArray[pCnlDev->devnum] = NULL;
            CNL_releaseDevice(pCnlDev);
        } else {
            DBG_ERR("unregister CNL operation from upper layer failed[%d].\n",
                    retval);
        }
    } else {
        // devstate should be DEV_ACTIVE.
        DBG_ASSERT(pCnlDev->devState == CNL_DEV_ACTIVE);

        // notify device is removed.
        CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
        event.type  = CNL_EVENT_DEVICE_REMOVED;
        CNL_addEvent(pCnlDev, &event);

        pCnlDev->devState = CNL_DEV_GONE;
    }

    CMN_UNLOCK_MUTEX(g_cnlDevMtxId);

    // wait for CNL task idle state.
    do {
        S_CNL_DEV   *pTempCnlDev = NULL;

        CMN_lockCpu(g_sigDevLockId);
        pTempCnlDev = (S_CNL_DEV*)CMN_LIST_LOOKUP(&g_sigDevList);
        if (g_pCurrentCnlDev == NULL && pTempCnlDev == NULL) {
            DBG_INFO("No events. Cnl task is idle state.\n");
            CMN_unlockCpu(g_sigDevLockId);
            break;
        }
        else {
            // do nothing.
            DBG_INFO("Cnl task is running. pCurrentCnlDev=%p, pTempCnlDev=%p\n", 
                g_pCurrentCnlDev, pTempCnlDev);
        }
        CMN_unlockCpu(g_sigDevLockId);

        // retry MaxRetryCount times.
        if(++RetryCount > WAIT_CNLTASKSTOP_MAX_NUM) {
            break;
        }

        DBG_INFO("Wait for CNL task.\n");
        CMN_delayTask(WAIT_CNLTASKSTOP_SLEEP_TIME);

    } while (TRUE);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : CNL_devToCnlDev
 *-----------------------------------------------------------------*/
/**
 * search CNL device by device pointer.
 * @param  pDev : the pointer to device.
 * @return pointer to the CNL device
 * @note   
 */
/*-----------------------------------------------------------------*/
S_CNL_DEV *
CNL_devToCnlDev(void *pDev)  
{

    u8        devnum;

    for(devnum=0; devnum<CNL_DEV_MAX_NUM; devnum++) {
        if((g_pCnlDevArray[devnum] != NULL) &&
           (g_pCnlDevArray[devnum]->pDev == pDev)) {
            return g_pCnlDevArray[devnum];
        }
    }
    return NULL;

}


/*-------------------------------------------------------------------
 * Function : CNL_open
 *-----------------------------------------------------------------*/
/**
 * open CNL device.
 * @param  pDev    : the pointer of pointer to the CNL device.
 * @param  pPclCbk : the pointer to the PCL Callback functions.
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (invalid device state)
 * @return ERR_NOOBJ   (the object don't exist)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_open(void **pDev, S_PCL_CBKS *pPclCbks) 
{

    T_CMN_ERR  retval;
    u8         devnum;
    S_CNL_DEV *pCnlDev;

    pCnlDev = (S_CNL_DEV *)(*pDev);

    CMN_LOCK_MUTEX(g_cnlDevMtxId);

    // check if device is present or not.
    if(pCnlDev == NULL) {
        for(devnum=0; devnum<CNL_DEV_MAX_NUM; devnum++) {
            if((g_pCnlDevArray[devnum] != NULL) &&
               (g_pCnlDevArray[devnum]->devState == CNL_DEV_READY)) {
                // unused device found.
                pCnlDev = g_pCnlDevArray[devnum];
                break;
            }
        }
        if(pCnlDev == NULL) {
            // empty device not found.
            DBG_ERR("unused CNL device is not found.\n");
            retval = ERR_NOOBJ; // no object error.
            goto EXIT;
        }
    }

    // check device state.
    if(pCnlDev->devState != CNL_DEV_READY) {
        DBG_ERR("the target device is in invalid state to open[%d].\n",
                pCnlDev->devState);
        retval = ERR_INVSTAT; // invalid state error.
    } else {
        // dev state READY --> ACTIVE
        // store PCL callback function pointers.
        pCnlDev->devState = CNL_DEV_ACTIVE;
        pCnlDev->pPclCbks = pPclCbks;
        retval = SUCCESS;
    }

EXIT:
    CMN_UNLOCK_MUTEX(g_cnlDevMtxId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : CNL_close
 *-----------------------------------------------------------------*/
/**
 * close CNL device.
 * @param  pDev : the pointer to the CNL device..
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (invalid device state)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
CNL_close(void *pDev)
{

    T_CMN_ERR  retval;
    S_CNL_DEV *pCnlDev;

    pCnlDev = (S_CNL_DEV *)(pDev);

    CMN_LOCK_MUTEX(g_cnlDevMtxId);

    if(pCnlDev->devState == CNL_DEV_ACTIVE) {
        // device state ACTIVE --> READY
        pCnlDev->devState = CNL_DEV_READY;
        retval = SUCCESS;
    } else if(pCnlDev->devState == CNL_DEV_GONE) {
        //
        // no more access to this device.
        // unregister CNL operations from upper layer.
        //
        retval = CNL_unregisterCnlOps((void *)pCnlDev);
        if(retval == SUCCESS) {
            g_pCnlDevArray[pCnlDev->devnum] = NULL;
            CNL_releaseDevice(pCnlDev);
        } else {
            DBG_ERR("unregister CNL operation from upper layer failed[%d].\n",
                    retval);
        }

        retval = SUCCESS;
    } else {
        DBG_ERR("the target device is in invalid state to close[%d].\n",
                pCnlDev->devState);
        retval = ERR_INVSTAT; // invalid state error.
    }

    CMN_UNLOCK_MUTEX(g_cnlDevMtxId);

    return retval;
}


/**
 * Module Initailize/Cleanup functions.
 * called when module installed or rmoved.
 */
/*-------------------------------------------------------------------
 * Function : CNL_init
 *-----------------------------------------------------------------*/
/**
 * Initialize routine called when module inserted to kernel.
 * @param   nothing.
 * @return  0  (normally completion)
 * @return  -1 (error occurred)
 * @note   
 */
/*-----------------------------------------------------------------*/
int
CNL_init(void) 
{

    T_CMN_ERR retval;
    u8        devnum;
    
    for(devnum=0; devnum<CNL_DEV_MAX_NUM; devnum++) {
        g_pCnlDevArray[devnum] = NULL;
    }
    CMN_LIST_INIT(&g_sigDevList);

    // get common resources.
    retval = CNL_getCmnResources();
    if(retval != SUCCESS) {
        goto EXIT;
    }

    // 1. start CNL task.
    retval = CMN_startTask(g_cnlTaskId, NULL);
    if(retval != SUCCESS) {
        goto EXIT_1;
    }

    // 2. register driver(s)
    retval = CNL_registerDriver();
    if(retval != SUCCESS) {
        goto EXIT_2;
    }

    DBG_INFO("CNL core driver module is inserted to the kernel.\n");

    return SUCCESS;

EXIT_2:
    CMN_terminateTask(g_cnlTaskId);
EXIT_1:
    CNL_putCmnResources();
EXIT:
    DBG_ERR("install CNL core driver module failed.\n");

    return -1;
}


/*-------------------------------------------------------------------
 * Function : CNL_exit
 *-----------------------------------------------------------------*/
/**
 * Initialize routine called when module inserted to kernel.
 * @param   nothing.
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
void
CNL_exit(void) 
{

    // be careful to unregister
    CNL_unregisterDriver();
    CMN_terminateTask(g_cnlTaskId);
    CNL_putCmnResources();

    DBG_INFO("CNL core driver module is removed from the kernel.\n");

    return;
}
