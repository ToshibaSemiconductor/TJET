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
 *  @file     cnl.h
 *
 *  @brief    CNL local header.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNL_H__)
#define __CNL_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"

#include "oscmn.h"
#include "cmn_rsc.h"

#include "cnl_type.h"
#include "cnl_err.h"
#include "cnl_if.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/* REG_TXDATAINFO RATE[27:24] (0:auto 1:32 2:65 3:130 4:261 5:522) */
#define LINK_ADAP_RATE32     1
#define LINK_ADAP_RATE65     2
#define LINK_ADAP_RATE130    3  /* default */
#define LINK_ADAP_RATE261    4
#define LINK_ADAP_RATE522    5

/* link adaptation option */
/* REG_TXDATAINFO */
#define IZAN_TX_RATE      LINK_ADAP_RATE522
/* REG_RATE*/
#define IZAN_DEFAULT_RATE_REG_VALUE      (RATE_LC_AUTO | RATE_INIT_522 | \
                                          RATE_DOWN_ALG1 |RATE_DOWN_1RANK | \
                                          RATE_UP_ALG1)

/**
 * Configurations. these are moved to config file or Makefile etc..
 */
#define CNL_DEV_MAX_NUM         1 // shall not set over 31.
#define CNL_CTRL_QUEUE_SIZE     1 // control request queue depth.
#define CNL_TX_QUEUE_SIZE       5 // TX request queue depth.
#define CNL_RX_QUEUE0_SIZE      2 // RX request queue depth.
#define CNL_RX_QUEUE1_SIZE      5 // RX request queue depth.
#define CNL_MAX_DEVICE_PRIV   128 // max size of device private data.
#define CNL_ACTION_LIST_NUM    1

// config end.

#define CNL_LICC_VERSION_1     1

/**
 * Internal type definitions
 */
/**
 * @brief CNL device state.
 */
typedef enum tagE_DEV_STATE {
    CNL_DEV_READY = 0x01,
    CNL_DEV_ACTIVE,
    CNL_DEV_GONE,
}E_DEV_STATE;
typedef u8 T_DEV_STATE;


/**
 * @brief Power State
 */
typedef enum tagE_PWR_STATE {
    CNL_PWR_STATE_AWAKE              = 0x01,
    CNL_PWR_STATE_AWAKE_TO_HIBERNATE,
    CNL_PWR_STATE_HIBERNATE,
    CNL_PWR_STATE_HIBERNATE_TO_AWAKE,
}E_PWR_STATE;
typedef u8 T_CNL_PWR_STATE;


/**
 * @brief CNL Action type
 */
typedef enum tagE_CNL_ACTION {
    CNL_ACTION_NOP              = 0x00,
    CNL_ACTION_INIT             = 0x01,
    CNL_ACTION_CLOSE,
    CNL_ACTION_WAKE,
    CNL_ACTION_SLEEP,
    CNL_ACTION_SEND_MNG_FRAME,
    CNL_ACTION_SEND_DATA,
    CNL_ACTION_RECEIVE_DATA,
    CNL_ACTION_HANDLE_COMPLETE   = 0x81, // 0x80 bit is 1, state machine
    CNL_ACTION_HANDLE_MNG_FRAME,
    CNL_ACTION_HANDLE_TIMEOUT,
    CNL_ACTION_HANDLE_ERROR,
    CNL_ACTION_HANDLE_UPPER_ORDER,
    CNL_ACTION_HANDLE_EXTRA_EVENT,
}E_CNL_ACTION;
typedef u8 T_CNL_ACTION;
#define IS_HANDLER_ACTION(x) (x & 0x80)


/**
 * @brief CNL internal Event type(bit)
 */
typedef enum tagE_CNL_EVENT {
    CNL_EVENT_DEVICE_REMOVED  = 0x0001,
    CNL_EVENT_WAKE_REQUIRED   = 0x0002,
    CNL_EVENT_ERROR_OCCURRED  = 0x0004,
    CNL_EVENT_COMP_REQUEST    = 0x0008,
    CNL_EVENT_TIMEOUT         = 0x0010,
    CNL_EVENT_RECVD_MNG_FRAME = 0x0020,
    CNL_EVENT_TX_READY        = 0x0040,
    CNL_EVENT_TX_COMP         = 0x0080,
    CNL_EVENT_RX_READY        = 0x0100,
}E_CNL_EVENT;
typedef u16 T_CNL_EVENT;


/**
 * @brief timer type.
 */
typedef enum tagE_CNL_TIMER_TYPE{
    CNL_T_CONNECT   = 0x01,
    CNL_T_ACCEPT    = 0x02,
    CNL_T_RETRY     = 0x04,
    CNL_T_RESEND    = 0x08,
    CNL_T_KEEPALIVE = 0x10,
}E_CNL_TIMER_TYPE;


/**
 * @brief completed request type
 */
typedef enum tagE_CNL_COMP_REQ_TYPE{
    CNL_COMP_NULL        = 0x0000, // 
    CNL_COMP_INIT        = 0x0001,
    CNL_COMP_CLOSE       = 0x0002,
    CNL_COMP_CONNECT_REQ = 0x0004, // dummy completion : sending C-Req.
    CNL_COMP_ACCEPT_REQ  = 0x0008, // dummy completion : sending C-Acc.
    CNL_COMP_ACCEPT_RES  = 0x0010, // dummy completion : sent ACK for C-Acc.
    CNL_COMP_RELEASE_REQ = 0x0020, // dummy completion : sent C-Rls
    CNL_COMP_SLEEP_REQ   = 0x0040, // C-Sleep completed.(ACK received)
    CNL_COMP_WAKE_REQ    = 0x0080, // C-Wake completed.(ACK received)
    CNL_COMP_PROBE_REQ   = 0x0100, // C-Probe completed.(ACK received)

    CNL_COMP_WAKE        = 0x0200, // Wake completed.(device depended?)
    CNL_COMP_SLEEP       = 0x0400, // Sleep completed.(device depended?)

    CNL_COMP_DATA_REQ    = 0x0800,
}E_CNL_COMP_REQ_TYPE;

// POWERSAVE.request valid
#define IS_INTERNAL_REQ(x) (x & (CNL_COMP_WAKE | CNL_COMP_SLEEP | \
                                 CNL_COMP_WAKE_REQ | CNL_COMP_PROBE_REQ))

/**
 * @brief LiCC type defintion(CNL specification 6.3.5)
 */
typedef enum tagE_CNL_LICC_TYPE{
    CNL_LICC_C_REQ   = 0x01,
    CNL_LICC_C_ACC   = 0x02,
    CNL_LICC_C_RLS   = 0x03,
    CNL_LICC_C_SLEEP = 0x08,
    CNL_LICC_C_WAKE  = 0x09,
    CNL_LICC_C_PROBE = 0x0A,
}E_CNL_LICC_TYPE;


/**
 * @brief extended LiCC type definition for CNL driver.
 *        priority ordered.
 */
typedef enum tagE_CNL_EXT_LICC_TYPE{
    CNL_EXT_LICC_C_RLS         = 0x01,
    CNL_EXT_LICC_C_REQ         = 0x02,
    CNL_EXT_LICC_C_ACC         = 0x04,
    CNL_EXT_LICC_ACK_FOR_C_ACC = 0x08,
    CNL_EXT_LICC_C_SLEEP       = 0x10,
    CNL_EXT_LICC_C_WAKE        = 0x20,
    CNL_EXT_LICC_C_PROBE       = 0x40,
}E_CNL_EXT_LICC_TYPE;


/**
 * @brief extra event type.
 */
typedef enum tagE_CNL_EXTRA_EVENT_TYPE {
    CNL_EXTRA_DEVICE_REMOVED  = 0x01,
    CNL_EXTRA_DATA_INDICATION,
}E_CNL_EXTRA_EVENT_TYPE;


// request state.
typedef enum tagE_CNL_REQ_STATE {
    CNL_REQ_QUEUED = 0x01,
    CNL_REQ_PROCESSING,
    CNL_REQ_COMPLETED,
    CNL_REQ_CANCELLING,
    CNL_REQ_CANCELLED,
}E_CNL_REQ_STATE;

#define CNL_TX_CSDU      0
#define CNL_RX_CSDU      1
#define CNL_EMPTY_PID 0xFF

#define MAKE_CNLSTATE(main, sub)       ((main << 8) | (sub))
#define SET_MAINSTATE(state, main)     (state = ((main << 8) | (state & 0xFF)))
#define SET_SUBSTATE(state, sub)       (state = ((state & 0xFF00) | (sub)))
#define GET_MAINSTATE(state)           ((state >> 8) & 0xFF)
#define GET_SUBSTATE(state)            (state & 0xFF)

// use NTZ algorithm to get most prior bit.
#define GET_PRIOR_BIT(x)       ((x) & (-(x))) 
#define CLEAR_BIT(bitmap, bit) ((bitmap) &= ~(bit))

#define LENGTH_TO_CSDU(len) ((len + CNL_CSDU_SIZE - 1) / CNL_CSDU_SIZE)
#define PADDING_4B(x)       ((x + 3) & ~0x03)
#define IS_MULTI_OF_4K(x)   (!(x & 0x0FFF))
#define IS_MULTI_OF_4B(x)   (!(x & 0x03))

#define CNL_TAC_TO_AWAKE_PERIOD(usec)   (u8)((usec) / 100)
#define CNL_TDC_TO_DORMANT_PERIOD(usec) (u8)((usec) / 5000)

#define CNL_POWERSAVE_KEEPALIVE_TO_TKEEPALIVE(msec) (u32)((msec) * 1000)
#define CNL_POWERSAVE_AWAKE_TO_TAC(usec)            ((usec) == 0) ? (u32)(256 * 100) : (u32)((usec)*100)
#define CNL_POWERSAVE_DORMANT_TO_TDC(usec)          (u32)((usec) * 5000)

#define CNL_FREQUPDN_MAX                (100) 
#define CNL_FREQUPDN_MIN                (1) 
#define CNL_RSSI_VALUE_MIN              (0xFF) 
#define CNL_RSSI_VALUE_MAX              (0x00) 
#define CNL_DEFAULT_FREQUPDN_VALUE      (10)


/* Discard Data Buffer Size */
#define CNL_DISCARD_DATA_BUF_SIZE 4096 /* Default Buffer Size is IZAN 1bank = 4096byte */

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
typedef struct tagS_CNL_DEV S_CNL_DEV;

/**
 * @brief S_CNL_DATA_REQ extension data.
 */
typedef struct tagS_CNL_DATA_REQ_EXT {
    u32 position;   // start position of next send/receive.
    u32 sendingLen; // current sending length.
    u32 compLen;    // sent/received data length.
    u32 startTime;  // use test mode only
}S_DATA_REQ_EXT;

/*
 * @brief data received callback management.
 */
typedef struct tagS_CNL_RECV_CBK_MGR {
    T_CNL_RECV_CBK  pCbk;
    void           *pArg;
    u8              call;
}S_CNL_RECV_CBK_MGR;


/**
 * @brief CNL Action parameter.
 */
typedef struct tagS_CNL_ACTION {
    T_CNL_ACTION   type;
    S_CNL_CMN_REQ *pReq;      // current target request.

    //
    // action dependent field.
    //
    // for HANDLE_COMPLETE
    u16            compReq;
    S_LIST         compQueue; // data request complete queue.

    // for HANDLE_TIMEOUT
    u16            timer;

    // for HANDLE_MNGFRAME
    u16            licc;
    u8             targetUID[CNL_UID_SIZE];
    u8             liccInfo[CNL_LICC_INFO_SIZE];

    // for HANDLE_ERROR
    T_CNL_ERR      status;

    // for HANDLE_ORDER
    T_CNL_REQ_TYPE order;

    // for HANDLE_EXTRA_EVENT
    u16            extra;
    u8             profileId;

    // for SENDDATA/RECEIVEDATA
    u32            readyLength;
}S_CNL_ACTION;


/*
 * CNL device related structures.
 */
/**
 * @brief CNL Device depend parameters.
 */
typedef struct tagS_CNL_DEVICE_PARAM {
    u8  ownUID[CNL_UID_SIZE]; // own UID.
    u8  txCsduNum;            // max number of available TX CSDU.
    u8  rxCsduNum;            // max number of available RX CSDU.
    u32 tConnect;             // T_Connect timer (usec order)
    u32 tAccept;              // T_Accept timer (usec order)
    u32 tRetry;               // T_Retry timer (usec order)
    u32 tResend;              // T_Resend timer (usec order)
    u32 tKeepAlive;           // T_Keepalive timer (usec order)
    u32 tas;                  // AwakePeriod for Search state(usec order)
    u32 tds;                  // DormantPeriod for Search state(usec order)
    u32 tac;                  // AwakePeriod for LocalHibernate state(usec order)
    u32 tdc;                  // DormantPeriod for LocalHibernate state(usec order)
    // add more device depend parameters.
}S_CNL_DEVICE_PARAM;


/**
 * @brief CNL event structure.
 */
typedef struct tagS_CNL_DEVICE_EVENT {
    T_CNL_EVENT    type;
    u16            recvdLicc; // licc of reveived management frame.
    u16            compReq;   // type of request completed
    u16            timer;     // expired timer
    T_CNL_ERR      error;     // error code.
}S_CNL_DEVICE_EVENT;



/**
 * @brief CNL Device operations.
 */
typedef struct tagS_CNL_DEVICE_OPS {
    // 
    T_CNL_ERR (*pGetDeviceParam)(S_CNL_DEV *, S_CNL_DEVICE_PARAM *);

    // call from action handler.
    T_CNL_ERR (*pInit)(S_CNL_DEV *);
    T_CNL_ERR (*pClose)(S_CNL_DEV *);
    T_CNL_ERR (*pWake)(S_CNL_DEV *);
    T_CNL_ERR (*pSleep)(S_CNL_DEV *);
    T_CNL_ERR (*pSendMngFrame)(S_CNL_DEV *, u16, void *, void *);
    T_CNL_ERR (*pSendData)(S_CNL_DEV *, u8, u8, u32, void *);
    T_CNL_ERR (*pSendDataIntUnmask)(S_CNL_DEV *);
    T_CNL_ERR (*pReadReadyTxBuffer)(S_CNL_DEV *, u32 *);
    T_CNL_ERR (*pReceiveData)(S_CNL_DEV *, u8, u8 *, u32 *, void *);
    T_CNL_ERR (*pReceiveDataIntUnmask)(S_CNL_DEV *);
    T_CNL_ERR (*pReadReadyRxBuffer)(S_CNL_DEV *, u32 *, u8);

    // call from scheduler or statemachine functions.
    T_CNL_ERR (*pReadReadyPid)(S_CNL_DEV *, u8 *);
    T_CNL_ERR (*pReadReadyBuffer)(S_CNL_DEV *, u8, u32 *);
    T_CNL_ERR (*pReadMngBody)(S_CNL_DEV *, u16 *, void *, void *);
    T_CNL_ERR (*pChangeState)(S_CNL_DEV *, T_CNL_STATE, T_CNL_STATE);

    T_CNL_ERR (*pWaitConnect)(S_CNL_DEV *);
    T_CNL_ERR (*pCancelWaitConnect)(S_CNL_DEV *);
    T_CNL_ERR (*pGetStats)(S_CNL_DEV *, S_CNL_STATS *);

    T_CNL_ERR (*pRegPassthrough)(S_CNL_DEV *, S_CNL_REG_PASSTHROUGH *);
    
    // device cleanup function.
    void      (*pReleaseDeviceData)(S_CNL_DEV *);

    // add more interface if necessary.
}S_CNL_DEVICE_OPS;


/**
 * @brief CNL device information structure.
 */
struct tagS_CNL_DEV {
    //
    // device management informations.
    //
    u8                  devnum;        // device number.
    T_DEV_STATE         devState;      // device state.
    S_LIST              devList;       // use for signaling.


    //
    // CNL management informations.
    //
    u8                  mngLockId;     // device lock.

    T_CNL_STATE         cnlState;      // CNL state
    T_CNL_PWR_STATE     pwrState;      // 
    u8                  liccVersion;   // LiCC version.

    // reqeust information.
    u8                  reqMtxId;      // sync request mutex.
    u8                  reqWaitId;     // wait for block request.
    S_LIST              ctrlQueue;     // CTRL request queue head.
    u8                  ctrlReqCnt;    // current CTRL request count.
    S_LIST              txQueue;       // TX request queue head.
    u8                  txReqCnt;      // current TX request count.
    S_LIST              rx0Queue;      // RX request queue head for pid 0.
    u8                  rx0ReqCnt;     // current RX request for pid 0 count.
    S_LIST              rx1Queue;      // RX request queue head for pid 1
    u8                  rx1ReqCnt;     // current RX request for pid 1 count.

    u8                  dummyReqMplId; // dummy request memory pool Id for cancel.
    u8                  cancelWaitId;  // cancel request wait Id
    S_LIST              cancelQueue;   // cancel request queue.
    T_CMN_ERR           cancelStatus;  // cancel status
    
    u8                  waitConnect;   // WAIT_CONNECT is called or not.
    S_CNL_CMN_REQ       pwrReq;        // use for internal request(Powersave/Wake)
    u8                  crossover;     // indicates C-Req crossover occurred.
    u8                  missCaccAck;   // missing ACK for C-Acc but connected.

    // device event informations.
    S_CNL_DEVICE_EVENT  event;

    // expect event filter.
    S_CNL_DEVICE_EVENT  eventFilter;

    u8                  txReady;       // indicate TX is ready.
    u8                  txComp;        // indicate some TX request is completed.
    u8                  txSendingCsdu; // current using TX CSDU count.
    u8                  rxReady;       // indicate RX is ready.
    u8                  rxReadyPid;    // remaind RX.



    T_CNL_ACTION        procAction;    // current processing `CTRL' Action.

    //
    // upper driver interface.
    //
    S_PCL_CBKS         *pPclCbks;      // PCL callbacks.
    S_CNL_RECV_CBK_MGR  recvCbk[2];    // data received callback.


    //
    // device related interface.
    //
    void               *pDev;          // pointer to the device.
    S_CNL_DEVICE_OPS   *pDeviceOps;    // device interface functions for CNL core.
    S_CNL_DEVICE_PARAM  deviceParam;   // device dependent CNL parameters.
    u8                  devicePriv[0]; // device private data field.
                                       // maximum size is defined as CNL_MAX_DEVICE_PRIV
};


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/
static inline void
CNL_completeRequest(S_CNL_DEV *pCnlDev, S_CNL_CMN_REQ *pReq) {
    if(pReq->state == CNL_REQ_CANCELLING) {
        // this is dummy request, do not call completion but free it.
        CMN_releaseFixedMemPool(pCnlDev->dummyReqMplId, pReq);
    } else {
        pReq->pComplete(pReq, pReq->pArg1, pReq->pArg2);
    }
    return;
}

static inline void
CNL_clearEventFilter(S_CNL_DEV *pCnlDev) {
    pCnlDev->eventFilter.type      = 0x0000;
    pCnlDev->eventFilter.compReq   = 0x0000;
    pCnlDev->eventFilter.recvdLicc = 0x0000;
    pCnlDev->eventFilter.timer     = 0x0000;
    return;
}

static inline void
CNL_setEventFilter(S_CNL_DEV *pCnlDev, S_CNL_DEVICE_EVENT *pEvent) {
    // add expect event.
    pCnlDev->eventFilter.type      |= pEvent->type;
    pCnlDev->eventFilter.compReq   |= pEvent->compReq;
    pCnlDev->eventFilter.timer     |= pEvent->timer;
    pCnlDev->eventFilter.recvdLicc |= pEvent->recvdLicc;
    return;
}


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
// cnl.c
extern int        CNL_init(void);
extern void       CNL_exit(void);
extern S_CNL_DEV *CNL_allocDevice(void);
extern void       CNL_releaseDevice(S_CNL_DEV *);
extern T_CMN_ERR  CNL_registerDevice(S_CNL_DEV *);
extern T_CMN_ERR  CNL_unregisterDevice(S_CNL_DEV *);
extern S_CNL_DEV *CNL_devToCnlDev(void *);
extern T_CMN_ERR  CNL_signalDev(S_CNL_DEV *);
extern T_CMN_ERR  CNL_getSignaledDev(S_CNL_DEV **);

// cnl_task.c
extern void       CNL_task(void *);

// cnl_schd.c
extern T_CMN_ERR  CNL_addRequest(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern T_CMN_ERR  CNL_addEvent(S_CNL_DEV *, S_CNL_DEVICE_EVENT *);
extern T_CMN_ERR  CNL_clearEvent(S_CNL_DEV *, S_CNL_DEVICE_EVENT *);
extern T_CMN_ERR  CNL_cancelRequest(S_CNL_DEV *, T_CNL_REQ_ID);
extern T_CMN_ERR  CNL_getAction(S_CNL_DEV *, S_CNL_ACTION *);

extern T_CNL_ERR  CNL_stateMachine(S_CNL_DEV *, S_CNL_ACTION *);

// cnl_util.c
extern T_CNL_ERR  CNL_addRequestToCtrlQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern T_CNL_ERR  CNL_addRequestToTxQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern T_CNL_ERR  CNL_addRequestToRxQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern void       CNL_removeRequestFromCtrlQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern void       CNL_removeRequestFromTxQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern void       CNL_removeRequestFromRxQueue(S_CNL_DEV *, S_CNL_CMN_REQ *);
extern S_CNL_CMN_REQ *CNL_searchNextSendRequest(S_CNL_DEV *);
extern S_CNL_CMN_REQ *CNL_searchCancelRequest(S_CNL_DEV *, T_CNL_REQ_ID);


// cnl_izan.c
extern T_CMN_ERR  CNL_registerDriver(void);
extern T_CMN_ERR  CNL_unregisterDriver(void) ;

#endif /* __CNL_H__ */
