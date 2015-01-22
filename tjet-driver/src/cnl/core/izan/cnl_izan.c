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
 *  @file     cnl_izan.c
 *
 *  @brief    CNL device interface for IZAN.
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
#include "cnl_izan.h"
#include "izan_sdio_reg.h"
#include "izan_cnf.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
// register access utility.
static T_CNL_ERR IZAN_readRegister(void *, u32, u32, void *);
static T_CNL_ERR IZAN_writeRegister(void *, u32, u32, void *);
static T_CNL_ERR IZAN_pollU8Register(void *, u32, u8, u8, u16, u8);
static T_CNL_ERR IZAN_pollU32Register(void *, u32, u32, u32, u16, u8);
static T_CNL_ERR IZAN_readDMA(void *, u32,  u32, void *);
static T_CNL_ERR IZAN_writeDMA(void *, u32,  u32, void *);
static T_CNL_ERR IZAN_readRxvgagain(S_CNL_DEV *);
static T_CNL_ERR IZAN_resetRxvgagain(S_CNL_DEV *);

// HW access function for action handler
static T_CNL_ERR IZAN_getDeviceParam(S_CNL_DEV *, S_CNL_DEVICE_PARAM *);
static T_CNL_ERR IZAN_init(S_CNL_DEV *);
static T_CNL_ERR IZAN_close(S_CNL_DEV *);
static T_CNL_ERR IZAN_wake(S_CNL_DEV *);
static T_CNL_ERR IZAN_sleep(S_CNL_DEV *);
static T_CNL_ERR IZAN_sendMngFrame(S_CNL_DEV *, u16, void *, void *);
static T_CNL_ERR IZAN_sendData(S_CNL_DEV *, u8, u8, u32, void *);
static T_CNL_ERR IZAN_sendDataIntUnmask(S_CNL_DEV *);
static T_CNL_ERR IZAN_readReadyTxBuffer(S_CNL_DEV *, u32 *);
static T_CNL_ERR IZAN_receiveData(S_CNL_DEV *, u8, u8 *, u32 *, void *);
static T_CNL_ERR IZAN_receiveDataIntUnmask(S_CNL_DEV *);
static T_CNL_ERR IZAN_readReadyRxBuffer(S_CNL_DEV *, u32 *, u8);

// HW access function for scheduler and state machine 
static T_CNL_ERR IZAN_readReadyPid(S_CNL_DEV *, u8 *);
static T_CNL_ERR IZAN_readReadyBuffer(S_CNL_DEV *, u8, u32 *);
static T_CNL_ERR IZAN_readMngBody(S_CNL_DEV *, u16 *, void *, void *);
static T_CNL_ERR IZAN_changeState(S_CNL_DEV *, T_CNL_STATE, T_CNL_STATE);

static T_CNL_ERR IZAN_waitConnect(S_CNL_DEV *);
static T_CNL_ERR IZAN_cancelWaitConnect(S_CNL_DEV *pCnlDev);

// HW access function for other case.
static void      IZAN_releaseDeviceData(S_CNL_DEV *);
static T_CNL_ERR IZAN_getStats(S_CNL_DEV *, S_CNL_STATS *);
static T_CNL_ERR IZAN_regPassthrough(S_CNL_DEV *, S_CNL_REG_PASSTHROUGH *);

// reset for state change.
static T_CNL_ERR IZAN_closeReset(S_CNL_DEV *);
static T_CNL_ERR IZAN_releaseReset(S_CNL_DEV *);

// IZAN sequence
static T_CNL_ERR IZAN_sleepToAwake(S_CNL_DEV *);
static T_CNL_ERR IZAN_awakeToSleep(S_CNL_DEV *);
static T_CNL_ERR IZAN_sleepToDeepsleep(S_CNL_DEV *);
static T_CNL_ERR IZAN_resetTxFifo(S_CNL_DEV *);
static T_CNL_ERR IZAN_resetRxFifo(S_CNL_DEV *);
static T_CNL_ERR IZAN_disableCnlInt(S_CNL_DEV *);
static T_CNL_ERR IZAN_enableCnlInt(S_CNL_DEV *);
static T_CNL_ERR IZAN_stopTxMngFrame(S_CNL_DEV *);
static T_CNL_ERR IZAN_stopTxDataFrame(S_CNL_DEV *);
static T_CNL_ERR IZAN_clearTxRxBank(S_CNL_DEV *);
static T_CNL_ERR IZAN_preProcCS(S_CNL_DEV *, T_CNL_STATE, T_CNL_STATE);
static T_CNL_ERR IZAN_postProcCS(S_CNL_DEV *, T_CNL_STATE, T_CNL_STATE);

// interrupt utility.
static void      IZAN_intToEvent(S_CNL_DEVICE_EVENT *, u32);
static T_CNL_ERR IZAN_addIntUnmask(S_CNL_DEV *, u32);

// initialize sequence.
static T_CNL_ERR IZAN_initializePMU(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializePHY(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializeRF(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializeSPI(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializeCNL(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializeDevice(S_CNL_DEV *);
static T_CNL_ERR IZAN_setupTimer(S_CNL_DEV *);
static T_CNL_ERR IZAN_initializeOptional(S_CNL_DEV *);
static T_CNL_ERR IZAN_setCorrectionValueCROSC(S_CNL_DEV *);
static T_CNL_ERR IZAN_clearMSRReq(S_CNL_DEV *);
static T_CNL_ERR IZAN_setCROSCTrim(S_CNL_DEV *, u16);
static T_CNL_ERR IZAN_pollMSRResult(S_CNL_DEV *, u16 *);

// close sequence.

// calibration sequence.
static T_CNL_ERR IZAN_execCalibration(S_CNL_DEV *);

// functions to be registered to SD host.
static void      IZAN_irqHandler(void *);
static int       IZAN_probe(void *, S_BUSCMN_IDS *);
static int       IZAN_remove(void *);

static T_CNL_ERR IZAN_setTimerforPowerSave(S_CNL_DEV *);

/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
/**
 * @brief IZAN SDIO profile.
 */
static S_BUSSDIO_PROFILE g_sdioProfile = {
    .blockSize   = 512,  // 0 : use card default, else : set new value.
    .dmaAddrMode = 0,    // 0 : use fixed address at Cmd53 of read/write.
                         // 1 : use incrementing address at Cmd53 of read/write.
};


/**
 * @brief IZAN SDIO driver information.
 */
static S_BUSCMN_DRIVER g_cnlDriver = {
    .pName    = "Toshiba SD for TransferJet",
    .busType  = BUSCMN_TYPE_SDIO,
    .pProbe   = IZAN_probe,
    .pRemove  = IZAN_remove,
    .pProfile = &g_sdioProfile,

    // set valid device match ids
    .ids.vendorId    = BUSCMN_ANY_ID,
    .ids.deviceId    = BUSCMN_ANY_ID,
    .ids.subVendorId = BUSCMN_ANY_ID,
    .ids.subDeviceId = BUSCMN_ANY_ID,
    .ids.classId     = BUSCMN_ANY_ID,
};

// device interface function
static S_CNL_DEVICE_OPS g_cnlDeviceOps = {
    .pGetDeviceParam    = IZAN_getDeviceParam,
    .pInit              = IZAN_init,
    .pClose             = IZAN_close,
    .pWake              = IZAN_wake,
    .pSleep             = IZAN_sleep,
    .pSendMngFrame      = IZAN_sendMngFrame,
    .pSendData          = IZAN_sendData,
    .pSendDataIntUnmask = IZAN_sendDataIntUnmask,
    .pReadReadyTxBuffer = IZAN_readReadyTxBuffer,
    .pReceiveData       = IZAN_receiveData,
    .pReceiveDataIntUnmask = IZAN_receiveDataIntUnmask,
    .pReadReadyRxBuffer = IZAN_readReadyRxBuffer,

    // call from scheduler or statemachine functions.
    .pReadReadyPid      = IZAN_readReadyPid,
    .pReadReadyBuffer   = IZAN_readReadyBuffer,
    .pReadMngBody       = IZAN_readMngBody,
    .pChangeState       = IZAN_changeState,

    // 
    .pWaitConnect       = IZAN_waitConnect,
    .pCancelWaitConnect = IZAN_cancelWaitConnect,
    .pGetStats          = IZAN_getStats,
    .pRegPassthrough    = IZAN_regPassthrough,

    // 
    .pReleaseDeviceData = IZAN_releaseDeviceData,

};


// for IZAN_readMonitor()
static u32 g_monaddr[] =
{
// TX read monitor registers.
    REG_TXNUM,                      // 0x14300
    REG_TXRETNUM,                   // 0x14304
    REG_TXMNNUM,                    // 0x14310
    REG_TXR32NUM,                   // 0x14388
    REG_TXRETR32NUM,                // 0x1438C
    REG_TXR65NUM,                   // 0x14390
    REG_TXRETR65NUM,                // 0x14394
    REG_TXR130NUM,                  // 0x14398
    REG_TXRETR130NUM,               // 0x1439C
    REG_TXR261NUM,                  // 0x143A0
    REG_TXRETR261NUM,               // 0x143A4
    REG_TXR522NUM,                  // 0x143A8
    REG_TXRETR522NUM,               // 0x143AC
// RX read monitor registers.
    REG_RATEOUT,                    // 0x140E8
    REG_RXCNLHEADERRNUM,            // 0x14334
    REG_RXCNLBODYERRNUM,            // 0x14338
    REG_RXACKOKNUM,                 // 0x1433C
    REG_RXSDOKNUM,                  // 0x14340
    REG_RXMD1OKNUM,                 // 0x14344
    REG_RXMD2OKNUM,                 // 0x14348
    REG_RXMNOKNUM,                  // 0x1434C
    REG_RX32OKNUM,                  // 0x14350
    REG_RX32NGNUM,                  // 0x14354
    REG_RX65OKNUM,                  // 0x14358
    REG_RX65NGNUM,                  // 0x1435C
    REG_RX130OKNUM,                 // 0x14360
    REG_RX130NGNUM,                 // 0x14364
    REG_RX261OKNUM,                 // 0x14368
    REG_RX261NGNUM,                 // 0x1436C
    REG_RX522OKNUM,                 // 0x14370
    REG_RX522NGNUM,                 // 0x14374
    REG_RXVGAGAIN,                  // 0x14378
    REG_RXEVM,                      // 0x1437C
    REG_TXRATEDOWNNUM,              // 0x143B0
    REG_TXRATEUPNUM,                // 0x143B4

    REG_ZA_0x14480,                 // 0x14480
    REG_ZA_0x14484,                 // 0x14484
    REG_ZA_0x14488,                 // 0x14488
    REG_ZA_0x1448C,                 // 0x1448C
    REG_ZA_0x14490,                 // 0x14490
    REG_ZA_0x14494,                 // 0x14494
    REG_ZA_0x14498,                 // 0x14498
    REG_ZA_0x1449C,                 // 0x1449C
    REG_ZA_0x144A0,                 // 0x144A0
    REG_ZA_0x144A4,                 // 0x144A4

// rx info read monitor registers.
    REG_PHY_RXCOUNT_EDGEDET,                // 0x15044
    REG_PHY_RXCOUNT_HEADDET,                // 0x15048
    REG_PHY_RXCOUNT_HEADERR,                // 0x1504C
    REG_PHYRX_HEAD_ERR,                     // 0x15524
    REG_PHY_RXCOUNT_PYLDERR,                // 0x15050
    REG_PHY_RX_PYLDERR,                     // 0x15058
    REG_PHY_RXCOUNT_PYLDOK,                 // 0x15054
    REG_PHYRX_EVM_I,                        // 0x15514
    REG_PHYRX_EVM_Q,                        // 0x15518
    REG_PHYRFCTRL_AGCRSSI_DB,               // 0x15274
    REG_PHYRXEQ_CALCULATED_PHASEOFST,       // 0x15430
    REG_PHYRXEQ_CALCULATED_ANGLERATE,       // 0x15434
    REG_PHYRXEQ_FCANCEL_POSITIVE_COUNTER,   // 0x15438
    REG_PHYRXEQ_FCANCEL_NEGATIVE_COUNTER,   // 0x1543C
    REG_PHYRXEQ_ELCOUNT,                    // 0x15640
    REG_PHYRFCTRL_DCOFSET_I,                // 0x15294
    REG_PHYRFCTRL_DCOFSET_Q,                // 0x15298
    REG_PHYRX_HEAD_DATA,                    // 0x15520
    REG_PHYRFCTRL_RXVGAGAIN,                // 0x152B8
    0xFFFFFFFF                              // end.
};

static int g_TxRxBankStaRead_N = CNL_FREQUPDN_MAX;
static int g_freqUpdN          = CNL_DEFAULT_FREQUPDN_VALUE;
u8         g_rssi              = CNL_RSSI_VALUE_MIN;

static S_CNL_DEV *stored_dev = NULL;

/*-------------------------------------------------------------------
 * Inline function definition
 *-----------------------------------------------------------------*/
static inline void
IZAN_initDeviceData(S_IZAN_DEVICE_DATA *pDeviceData)
{
    CMN_MEMSET(pDeviceData, 0x00, sizeof(S_IZAN_DEVICE_DATA));
    pDeviceData->currIntEnable = IZAN_INTEN_NONE;
    pDeviceData->pmuState      = PMU_SLEEP;
    pDeviceData->needCal       = TRUE;

    // initial value is 0xFFFF-FF
    CMN_MEMSET(pDeviceData->targetUID, 0xFF, CNL_UID_SIZE);

    pDeviceData->rxRemain      = 0;
    pDeviceData->rxFragment    = CNL_FRAGMENTED_DATA;
    pDeviceData->rxNeedReset   = FALSE;
    pDeviceData->rxBankHeadPos = 0;
    pDeviceData->txNeedResend  = FALSE;
    pDeviceData->discardCreq   = FALSE;

    return;
}

static inline u16
IZAN_origLiccToExtLicc(u8 licc)
{
    switch(licc) {
    case CNL_LICC_C_REQ :
        return CNL_EXT_LICC_C_REQ;
    case CNL_LICC_C_ACC :
        return CNL_EXT_LICC_C_ACC;
    case CNL_LICC_C_RLS :
        return CNL_EXT_LICC_C_RLS;
    case CNL_LICC_C_SLEEP :
        return CNL_EXT_LICC_C_SLEEP;
    case CNL_LICC_C_WAKE :
        return CNL_EXT_LICC_C_WAKE;
    case CNL_LICC_C_PROBE :
        return CNL_EXT_LICC_C_PROBE;
    default :
        DBG_ASSERT(0);
        return 0;
    }
}


static inline u8
IZAN_extLiccToOrigLicc(u16 extLicc)
{
    switch(extLicc) {
    case CNL_EXT_LICC_C_REQ :
        return CNL_LICC_C_REQ;
    case CNL_EXT_LICC_C_ACC :
        return CNL_LICC_C_ACC;
    case CNL_EXT_LICC_C_RLS :
        return CNL_LICC_C_RLS;
    case CNL_EXT_LICC_C_SLEEP :
        return CNL_LICC_C_SLEEP;
    case CNL_EXT_LICC_C_WAKE :
        return CNL_LICC_C_WAKE; 
    case CNL_EXT_LICC_C_PROBE :
        return CNL_LICC_C_PROBE;
    default :
        DBG_ASSERT(0);
        return 0;
    }
}


static inline S_IZAN_DEVICE_DATA *
IZAN_cnlDevToDeviceData(S_CNL_DEV *pCnlDev)
{
    return (S_IZAN_DEVICE_DATA *)&(pCnlDev->devicePriv[0]);
}


static inline void
IZAN_setupMngFrame(S_IZAN_MNGFRM *pMngFrm, 
                   u8             liccVersion,
                   u16            licc,
                   void          *pOwnUID,
                   void          *pLiccInfo)
{
    //
    // setup Management frame body (be careful endian)
    // 
    pMngFrm->liccVersion            = liccVersion;
    pMngFrm->licc                   = IZAN_extLiccToOrigLicc(licc);
    pMngFrm->reserved               = 0;
    *((u32 *)pMngFrm->ownUID)       = CMN_H2BE32(*(((u32 *)pOwnUID)+1));
    *(((u32 *)pMngFrm->ownUID)+1)   = CMN_H2BE32(*((u32 *)pOwnUID));
    *((u32 *)pMngFrm->liccInfo)     = *((u32 *)pLiccInfo);
    *(((u32 *)pMngFrm->liccInfo)+1) = *(((u32 *)pLiccInfo)+1);
    *(((u32 *)pMngFrm->liccInfo)+2) = *(((u32 *)pLiccInfo)+2);
    *(((u32 *)pMngFrm->liccInfo)+3) = *(((u32 *)pLiccInfo)+3);
    *(((u32 *)pMngFrm->liccInfo)+4) = *(((u32 *)pLiccInfo)+4);
    return;
}

static inline void
IZAN_anlysMngFrame(S_IZAN_MNGFRM *pMngFrm, u16 *pLicc, void *pOwnUID, void *pLiccInfo)
{
    //
    // analyse Management frame body (be careful endian)
    //
    *pLicc                  = IZAN_origLiccToExtLicc(pMngFrm->licc);
    *((u32 *)pOwnUID)       = CMN_BE2H32(*(((u32 *)pMngFrm->ownUID)+1));
    *(((u32 *)pOwnUID)+1)   = CMN_BE2H32(*((u32 *)pMngFrm->ownUID));
    *((u32 *)pLiccInfo)     = *((u32 *)pMngFrm->liccInfo);
    *(((u32 *)pLiccInfo)+1) = *(((u32 *)pMngFrm->liccInfo)+1);
    *(((u32 *)pLiccInfo)+2) = *(((u32 *)pMngFrm->liccInfo)+2);
    *(((u32 *)pLiccInfo)+3) = *(((u32 *)pMngFrm->liccInfo)+3);
    *(((u32 *)pLiccInfo)+4) = *(((u32 *)pMngFrm->liccInfo)+4);
    return;
}


static inline void
IZAN_setupTxInfo(u32 *pTxInfo, u32 length, u8 profileId, u8 fragment)
{
    u8  i, cnt;
    u32 rest;
    cnt = (u8)LENGTH_TO_CSDU(length);

    DBG_ASSERT(cnt <= IZAN_TX_CSDU_NUM);

    rest = length;
    for(i=0; i<cnt; i++) {
        if(i == cnt-1) {
            // last frame
            pTxInfo[i] = IZAN_MAKE_DATAINFO(rest, profileId, fragment, IZAN_TX_RATE);
        } else {
            pTxInfo[i] = IZAN_MAKE_DATAINFO(CNL_CSDU_SIZE, profileId, CNL_FRAGMENTED_DATA, IZAN_TX_RATE);
            rest             -= CNL_CSDU_SIZE;
        }
        DBG_INFO("Setup TxInfo[%d] = 0x%x\n", i, *((u32 *)(&pTxInfo[i])));
    }
    return;
}


static inline u32
IZAN_stateToIntEnable(T_CNL_STATE state)
{

    u8 mainState = CNLSTATE_TO_MAINSTATE(state);
    u8 subState  = CNLSTATE_TO_SUBSTATE(state);

    switch(mainState) {
    case CNL_STATE_CLOSE :
        return IZAN_INTEN_NONE;
    case CNL_STATE_SEARCH :
        return IZAN_INTEN_SEARCH;
    case CNL_STATE_CONNECTION_REQUEST :
        return IZAN_INTEN_CONNECTION_REQUEST;
    case CNL_STATE_ACCEPT_WAITING :
        return IZAN_INTEN_ACCEPT_WAITING;
    case CNL_STATE_RESPONSE_WAITING :
        return IZAN_INTEN_RESPONSE_WAITING;
    case CNL_STATE_RESPONDER_RESPONSE :
        return IZAN_INTEN_RESPONDER_RESPONSE;
    case CNL_STATE_INITIATOR_CONNECTED :
    case CNL_STATE_RESPONDER_CONNECTED :
        switch(subState) {
        case CNL_SUBSTATE_CONNECTED :
            return IZAN_INTEN_CONNECTED | INT_CPROBETX;
        case CNL_SUBSTATE_TARGET_SLEEP :
        case CNL_SUBSTATE_LOCAL_HIBERNATE :
            return IZAN_INTEN_CONNECTED;
        default :
            DBG_ASSERT(0);
            return 0;
        }
    default :
        DBG_ASSERT(0);
        return 0;
    }
}

/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*=================================================================*/
/* Register Access Utility functions                               */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_readRegister
 *-----------------------------------------------------------------*/
/**
 * read register utility.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  length   : length of the register(only 1 or 4 is valid)
 * @param  pData    : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readRegister(void *pDev,
                  u32   regAddr,
                  u32   length,
                  void *pData)
{

    T_CMN_ERR         retval;
    S_BUSCMN_REG_CTRL ctrl;
    u8                status = 0;
    u8                cnt;

    // setup read register params.
    ctrl.addr    = regAddr;
    ctrl.length  = length;
    ctrl.pData   = pData;
    ctrl.pStatus = &status;

    cnt = 0;
    while(cnt <= IZAN_RW_RETRY) {
        retval = BUSCMN_ioctl(pDev, BUSCMN_IOTYPE_READ_REG, &ctrl);
        if(retval != SUCCESS) {
            DBG_ERR("read register failed[%d].\n", retval);
            return CNL_ERR_HOST_IO;
        }
        // command success , check response flag.
        status &= BUSSDIO_STATUS_MASK;
        if(status == 0) {
            // success.
            return CNL_SUCCESS;
        }
        else if(status == BUSSDIO_STATUS_CRC_ERR) {
            DBG_WARN("read register CRC error occured, retry cmd.\n");
            cnt++;
            continue;
        } else {
            DBG_ERR("read register status is not success[%x].\n", status);
            return CNL_ERR_HOST_IO;
        }
    }
    return CNL_ERR_HOST_IO;
}


/*-------------------------------------------------------------------
 * Function : IZAN_writeRegister
 *-----------------------------------------------------------------*/
/**
 * write register utility.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  length   : length of the register(only 1 or 4 is valid)
 * @param  pData    : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_writeRegister(void *pDev,
                   u32   regAddr,
                   u32   length,
                   void *pData)

{

    T_CMN_ERR         retval;
    S_BUSCMN_REG_CTRL ctrl;
    u8                status = 0;
    u8                cnt;

    // setup write register params.
    ctrl.addr    = regAddr;
    ctrl.length  = length;
    ctrl.pData   = pData;
    ctrl.pStatus = &status;

    cnt = 0;
    while(cnt <= IZAN_RW_RETRY) {
        retval = BUSCMN_ioctl(pDev, BUSCMN_IOTYPE_WRITE_REG, &ctrl);
        if(retval != SUCCESS) {
            DBG_ERR("write register failed[%d].\n", retval);
            return CNL_ERR_HOST_IO;
        }
        status &= BUSSDIO_STATUS_MASK;
        if(status == 0) {
            // success.
            return CNL_SUCCESS;
        }
        else if(status == BUSSDIO_STATUS_CRC_ERR) {
            DBG_WARN("write register CRC error occured, retry\n");
            cnt++;
            continue;
        } else {
            DBG_ERR("write register status is not success[%x].\n", status);
            return CNL_ERR_HOST_IO;
        }
    }
    return CNL_ERR_HOST_IO;
}


/*-------------------------------------------------------------------
 * Function : IZAN_pollU8Register
 *-----------------------------------------------------------------*/
/**
 * poll to be valid value of a u8 register.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  value    : valid value.
 * @param  mask     : compare mask.
 * @param  delay    : delay for retry (ms).
 * @param  retry    : retry count.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_pollU8Register(void *pDev,
                    u32   regAddr,
                    u8    value,
                    u8    mask,
                    u16   delay,
                    u8    retry)
{

    T_CNL_ERR         retval;
    u8                readValue = 0;
    u8                cnt = 0;

    do {
        if(delay > 0) {
            CMN_delayTask(delay);
        }
        retval = IZAN_readRegister(pDev, regAddr, 1, &readValue);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PollU8Register : read register failed[%d].\n", retval);
            return retval;
        }
        
        if(value == (readValue & mask)) {
            return CNL_SUCCESS;
        }
    } while(cnt++ < retry);

    DBG_ERR("PollU8Register : register poll failed.[valid:0x%x, actual:0x%x].\n", 
            value, (readValue & mask));

    return CNL_ERR_HW_PROT;
}


/*-------------------------------------------------------------------
 * Function : IZAN_pollU32Register
 *-----------------------------------------------------------------*/
/**
 * poll to be valid value of a u32 register.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  value    : valid value.
 * @param  mask     : compare mask.
 * @param  delay    : delay for retry (ms).
 * @param  retry    : retry count.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_pollU32Register(void *pDev,
                     u32   regAddr,
                     u32   value,
                     u32   mask,
                     u16   delay,
                     u8    retry)
{

    T_CNL_ERR         retval;
    u32               readValue = 0;
    u8                cnt = 0;

    do {
        if(delay > 0) {
            CMN_delayTask(delay);
        }
        retval = IZAN_readRegister(pDev, regAddr, 4, &readValue);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PollU32Register : read register failed[%d].\n", retval);
            return retval;
        }

        if(value == (readValue & mask)) {
            return CNL_SUCCESS;
        }
    } while(cnt++ < retry);

    DBG_ERR("PollU32Register : register poll failed.[valid:0x%x, actual:0x%x].\n", 
            value, (readValue & mask));

    return CNL_ERR_HW_PROT;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readDMA
 *-----------------------------------------------------------------*/
/**
 * read DMA utility.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  length   : length of the register(only 1 or 4 is valid)
 * @param  pData    : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readDMA(void *pDev,
             u32   regAddr, 
             u32   length,
             void *pData)
{
    
    T_CMN_ERR         retval;
    u8                status = 0;

    retval = BUSCMN_read(pDev, regAddr, length, pData, &status);
    if(retval != SUCCESS) {
        DBG_ERR("read DMA failed[%d].\n", retval);
        return CNL_ERR_HOST_IO;
    }
    if((status & BUSSDIO_STATUS_MASK) == 0) {
        // success.
        return CNL_SUCCESS;
    } else if((status & BUSSDIO_STATUS_MASK) == BUSSDIO_STATUS_CRC_ERR) {
        // caller should enter REWIND routine.
        DBG_WARN("read DMA CRC error occured.\n");
    } 

    return CNL_ERR_HOST_IO;
}


/*-------------------------------------------------------------------
 * Function : IZAN_writeDMA
 *-----------------------------------------------------------------*/
/**
 * write DMA utility.
 * @param  pDev     : the pointer to the device.
 * @param  regAddr  : register address
 * @param  length   : length of the register(only 1 or 4 is valid)
 * @param  pData    : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_writeDMA(void *pDev,
              u32   regAddr, 
              u32   length,
              void *pData)
{
    
    T_CMN_ERR         retval;
    u8                status = 0;

    retval = BUSCMN_write(pDev, regAddr, length, pData, &status);
    if(retval != SUCCESS) {
        DBG_ERR("write register failed[%d].\n", retval);
        return CNL_ERR_HOST_IO;
    }
    if((status & BUSSDIO_STATUS_MASK) == 0) {
        // success.
        return CNL_SUCCESS;
    } else if((status & BUSSDIO_STATUS_MASK) == BUSSDIO_STATUS_CRC_ERR) {
        DBG_WARN("write DMA CRC error occured.\n");
        return CNL_ERR_HOST_IO;
    } 

    return CNL_ERR_HOST_IO;
}

/*-------------------------------------------------------------------
 * Function : IZAN_readRxvgagain
 *-----------------------------------------------------------------*/
/**
 * execute read RXVGAGAIN 
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_BADPARM  (bad paramter)
 * @note   use g_rssi g_TxRxBankStRead_N 
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_readRxvgagain(S_CNL_DEV *pCnlDev)
{
    u32       rxvgagain;
    u8        mainState;
    T_CNL_ERR retval;

    if (pCnlDev == NULL) {
        DBG_ERR("Bad Parameter pCnlDev:%p.\n", pCnlDev);
        return CNL_ERR_BADPARM; 
    }


    mainState = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);
    if (( mainState == CNL_STATE_INITIATOR_CONNECTED) ||
        ( mainState == CNL_STATE_RESPONDER_CONNECTED)) {
        g_TxRxBankStaRead_N++;
        if ( g_TxRxBankStaRead_N >= g_freqUpdN) { 
            DBG_INFO("g_TxRxBankStaRead_N:%d, g_rssi:%u\n", g_TxRxBankStaRead_N, g_rssi);
            g_TxRxBankStaRead_N = 0; 
            retval = IZAN_readRegister(pCnlDev->pDev, REG_RXVGAGAIN, 4, &rxvgagain); 
            if(retval != CNL_SUCCESS) {
                DBG_ERR("read RXVGAGAIN failed[%d].\n", retval);
                return retval;
            }
            g_rssi = IZAN_RXVGAGAIN_TO_RSSI(CMN_LE2H32(rxvgagain));
        }
    }

    return CNL_SUCCESS;

}

/*-------------------------------------------------------------------
 * Function : IZAN_resetRxvgagain
 *-----------------------------------------------------------------*/
/**
 * execute reset rxvgagain
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_BADPARM  (bad parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_resetRxvgagain(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR retval = CNL_SUCCESS;
    u32       staten;
    u32       vgacoef;
    void      *pDev;
    
    if (pCnlDev == NULL) {
        DBG_ERR("read RXSTATEN register failed[%d]\n", retval);
        return CNL_ERR_BADPARM;
    }

    pDev = pCnlDev->pDev;

    /* VGACOEF setting */
    retval = IZAN_readRegister(pDev, REG_VGAEVMCOEF, 4, &vgacoef);
    if (retval != CNL_SUCCESS) {
        DBG_ERR("read VGACOEF register failed[%d]\n", retval);
        return retval;
    }
    vgacoef &= CMN_H2LE32(~VGAEVMCOEF_VGACOEF_MASK);
    vgacoef |= CMN_H2LE32(VGAEVMCOEF_VGACOEF_10);
    retval = IZAN_writeRegister(pDev, REG_VGAEVMCOEF, 4, &vgacoef);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("write VGACOEF register failed[%d].\n", retval);
        return retval;
    }

    /* Reset RXVGAGAIN counter */
    retval = IZAN_readRegister(pDev, REG_RXSTATEN, 4, &staten);
    if (retval != CNL_SUCCESS) {
        DBG_ERR("read RXSTATEN register failed[%d]\n", retval);
        return retval;
    }
    staten &= CMN_H2LE32(~RXSTAT_RXVGAGAIN_EN);
    retval = IZAN_writeRegister(pDev, REG_RXSTATEN, 4, &staten);
    if (retval != CNL_SUCCESS) {
        DBG_ERR("write RXSTAEN register failed[%d]\n", retval);
        return retval;
    }
    staten |= CMN_H2LE32(RXSTAT_RXVGAGAIN_EN);
    retval = IZAN_writeRegister(pDev, REG_RXSTATEN, 4, &staten);
    if (retval != CNL_SUCCESS) {
        DBG_ERR("write RXSTAEN register failed[%d]\n", retval);
        return retval;
    }

    /* Reset counter */
    g_TxRxBankStaRead_N = CNL_FREQUPDN_MAX;

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_closeReset
 *-----------------------------------------------------------------*/
/**
 * reset sequence of CNL is closed.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_closeReset(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    S_IZAN_DEVICE_DATA *pDeviceData;

    // 
    // close reset sequence.(after Search->Close)
    //
    // 1. reset parameters(same as release reset) (already executed.)
    // 2. clear all interrupt and mask.
    // 3. change PMU state Awake to Sleep.
    // 4. change PMU state Sleep to Deepsleep.
    //

    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    // 3.
    retval = IZAN_awakeToSleep(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Close reset : Awake to Sleep failed[%d].\n", retval);
        CMN_unlockPower();
        return retval;
    }

    // 4. deepsleep
    retval = IZAN_sleepToDeepsleep(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Close reset : Sleep to DeepSleep failed[%d].\n", retval);
        CMN_unlockPower();
        return retval;
    }

    pDeviceData->pmuState     = PMU_DEEP_SLEEP;
    pDeviceData->rxRemain     = 0;
    pDeviceData->rxFragment   = CNL_FRAGMENTED_DATA;
    pDeviceData->rxNeedReset  = FALSE;
    pDeviceData->txNeedResend = FALSE;
    pDeviceData->discardCreq  = FALSE;
    
    // unlock WakeLock
    CMN_unlockPower();

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_releaseReset
 *-----------------------------------------------------------------*/
/**
 * reset sequence of CNL link is released.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_releaseReset(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 tuid;
    u32                 mfreaddone;

    //
    // reset sequence for link down.
    //
    // 1. reset TARGETUID.
    // 2. reset ACK flag. - already done.
    // 3. clear Tx/Rx buffer(bank) and disable Tx.
    // 4. reset RX fifo.
    // 5. clear MF buffer(do MFREADDONE)
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    
    // 1.
    CMN_MEMSET(pDeviceData->targetUID, 0xFF, CNL_UID_SIZE);

    tuid   = CMN_H2LE32(*(((u32 *)pDeviceData->targetUID)+1));
    retval = IZAN_writeRegister(pDev, REG_TARGETUID1, 4, &tuid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReleaseReset : set TARGETUID[1] failed[%d].\n", retval);
        return retval;
    }
    tuid   = CMN_H2LE32(*((u32 *)pDeviceData->targetUID));
    retval = IZAN_writeRegister(pDev, REG_TARGETUID2, 4, &tuid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReleaseReset : set TARGETUID[2] failed[%d].\n", retval);
        return retval;
    }

    // 3.
    // clear Tx/Rx and TX disable.
    retval = IZAN_clearTxRxBank(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReleaseReset : clear Tx/Rx Bank failed[%d].\n", retval);
        return retval;
    }

    retval = IZAN_resetRxFifo(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReleaseReset : reset RxFIFO failed[%d].\n", retval);
        return retval;
    }

    mfreaddone = CMN_H2LE32(MFDATAREADDONE_ON);
    retval = IZAN_writeRegister(pDev, REG_MFDATAREADDONE, 4, &mfreaddone);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReleaseReset : MFDATAREADDONE failed[%d].\n", retval);
        return retval;
    }

    // reset device parameter
    pDeviceData->rxRemain     = 0;
    pDeviceData->rxFragment   = CNL_FRAGMENTED_DATA;
    pDeviceData->rxNeedReset  = FALSE;
    pDeviceData->txNeedResend = FALSE;
    pDeviceData->discardCreq  = FALSE;
    return CNL_SUCCESS;

}


/*=================================================================*/
/* IZAN card Initialize related functions                          */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_initializeDevice
 *-----------------------------------------------------------------*/
/**
 * execute initialize CNL  device sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializeDevice(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    // initialize PMU.
    retval = IZAN_initializePMU(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // initialize PHY.
    retval = IZAN_initializePHY(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // initialize RF.
    retval = IZAN_initializeRF(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // initialize SPI.
    retval = IZAN_initializeSPI(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // initialize CNL.
    retval = IZAN_initializeCNL(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // initialize RF and Others (evaluation option)
    retval = IZAN_initializeOptional(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // set CR OSC correction value.
    retval = IZAN_setCorrectionValueCROSC(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    // sleep.
    retval = IZAN_awakeToSleep(pCnlDev);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_initializePMU.
 *-----------------------------------------------------------------*/
/**
 * initialize PMU sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializePMU(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR    retval;
    void        *pDev;

    pDev = pCnlDev->pDev;

    //
    // PMU initialize sequence.
    //
    // 1. change state from DeepSleep to Awake.(same as Sleep to Awake)
    // 2. poll to wait eFuse loaded
    // 3. disable bus clock for eFuse(default enabled)
    // 4. SW reset for eFuse(default enabled)
    //

    // 1.
    retval = IZAN_sleepToAwake(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in PMU Init : change PMU DeepSleep to Awake failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_initializePHY.
 *-----------------------------------------------------------------*/
/**
 * initialize PHY sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializePHY(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR    retval;
    void        *pDev;
    u32          clkgate;
    u32          ctrltm;
	u32          cmfset;

    pDev = pCnlDev->pDev;

    //
    // PHY initialize sequence.
    //
    // 1. set PHY_TEST_MODULE_CLKGATE register to 0x00000001
    // 2. set PHY_CONTROL_TESTMODE register to 0x00000002
    // 3. set PHY_RXSYNC_MF_SETTING register to 512tap
    //


    // 1.
    clkgate = CMN_H2LE32(0x00000000);
    if(CMN_getMonitorSwitch() == MONSW_ON){
       clkgate = CMN_H2LE32(0x00000001);
    }
    retval = IZAN_writeRegister(pDev, REG_TESTMODULECLKGATE, 4, &clkgate);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in PHY Init : write TEST_MODULE_CLKGATE(0x%x) failed[%d].\n", clkgate, retval);
        return retval;
    }


    // 2.
    if(CMN_getMonitorSwitch() == MONSW_ON){
        ctrltm = CMN_H2LE32(0x00000002);
        retval = IZAN_writeRegister(pDev, REG_CONTROLTESTMODE, 4, &ctrltm);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("in PHY Init : write CONTROL_TEST_MODE(0x%x) failed[%d].\n", ctrltm, retval);
            return retval;
        }
    }

    // 3.
    cmfset = CMN_H2LE32(RXSYNC_MF_SETTING_512TAP);
    retval = IZAN_writeRegister(pDev, REG_PHY_RXSYNC_MF_SETTING, 4, &cmfset);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in PHY Init : write RXSYNC_MF_SETTING(0x%x) failed[%d].\n", cmfset, retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_initializeRF.
 *-----------------------------------------------------------------*/
/**
 * initialize RF sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializeRF(S_CNL_DEV *pCnlDev)
{
    /*
    RF initialization is not processed here. 
    Instead, it is processed by IZAN_initializeOptional(). 
    */
    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_initializeOptional
 *-----------------------------------------------------------------*/
/**
 * initialize RF and Others sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializeOptional(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;
    S_RFPARAM_PLIST  tmpPrm;
    u32          rfAddr;
    u32          rfVal;
    int         *pMaxcnt;
    int         *pAddr;
    int         *pVal;
    int          i = 0;

    pDev = pCnlDev->pDev;

    //
    // Optional RF and Others initial setting.
    //
    CMN_getRfParam(&tmpPrm);
    pMaxcnt = tmpPrm.prmArrayMax;
    pAddr   = tmpPrm.prmRegAddr;
    pVal    = tmpPrm.prmRegValue;

    for(i=0; i<*(pMaxcnt); i++) {
        rfAddr = CMN_H2LE32(*(pAddr+i));
        rfVal  = CMN_H2LE32(*(pVal+i));
        if(rfAddr == 0) {
            continue;
        }
        retval = IZAN_writeRegister(pDev, rfAddr, 4, &rfVal);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("in Optional Init : write Address(0x%x).value(0x%x) failed[%d].\n", rfAddr, rfVal, retval);
            return retval;
        }
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_initializeSPI.
 *-----------------------------------------------------------------*/
/**
 * initialize SPI sequence. read information from E2PROM.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializeSPI(S_CNL_DEV *pCnlDev)
{
    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_setCorrectionValueCROSC
 *-----------------------------------------------------------------*/
/**
 * set CR OSC correction value.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR IZAN_setCorrectionValueCROSC(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;

    u16          trm = 0;
    u16          paramA = 0;
    u16          paramB = 0;
    u8           crmsrctl = 0;

    pDev = pCnlDev->pDev;

    //
    // get CR OSC correction value sequence
    //
    // 1. Set measurement period = 1 clock
    // 2. Clear CRMSRREQ
    // 3. CROSC trimming level setting
    // 4. start measurement
    // 5. wait CRMSRFIN & get CRMSRRSLT param
    // 6. adjust CR OSC measurement value
    // 7. Postprocessing (->Clear CRMSRREQ)
    //

    // 1.
    crmsrctl = 0x00;
    retval = IZAN_writeRegister(pDev, REG_CRMSRCTL, 1, &crmsrctl);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in set CROSC correction value : write CRMSRCTL(0x%x) failed[%d].\n", crmsrctl, retval);
        return retval;
    }
    DBG_INFO("[DEBUG] clear CRMSRVAL. -> reg_w = 0x%x\n", crmsrctl);

    while (trm < 16) {

        // 2.
        retval = IZAN_clearMSRReq(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("in set CROSC correction value : IZAN_clearMSRReq() failed[%d].\n", retval);
            return retval;
        }

        // 3.
        retval = IZAN_setCROSCTrim(pCnlDev, trm);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("in set CROSC correction value : IZAN_setCROSCTrim() failed[%d].\n", retval);
            return retval;
        }

        // 4.
        crmsrctl = 0x01;
        retval = IZAN_writeRegister(pDev, REG_CRMSRCTL, 1, &crmsrctl);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("in set CROSC correction value : write CRMSRCTL(0x%x) failed[%d].\n", crmsrctl, retval);
            return retval;
        }
        DBG_INFO("[DEBUG] set CRMSRREQ. -> reg_w = 0x%x\n", crmsrctl);

        // 5.
        retval = IZAN_pollMSRResult(pCnlDev, &paramA);
        if(retval == CNL_ERR_TIMEOUT) {
            DBG_ERR("TRM=%d IZAN_pollMSRResult() returned CNL_ERR_TIMEOUT\n", trm);
            trm++;
            continue;
        }
        else if(retval != CNL_SUCCESS) {
            DBG_ERR("in set CROSC correction value : IZAN_pollMSRResult() failed[%d].\n", retval);
            return retval;
        }

        // 6.
        if(paramA <= IZAN_CRMSRSTAT_STANDARD){
            trm++;
            DBG_INFO("[DEBUG] CRMSRSTAT <= 0x36B0 -> TRM=%d, A=0x%x, B=0x%x.\n", trm, paramA, paramB);
            paramB = paramA;
        }
        else{
            if(paramA-IZAN_CRMSRSTAT_STANDARD > IZAN_CRMSRSTAT_STANDARD-paramB){
                trm--;

                // 3.
                retval = IZAN_setCROSCTrim(pCnlDev, trm);
                if(retval != CNL_SUCCESS) {
                    DBG_ERR("in set CROSC correction value : IZAN_setCROSCTrim() failed[%d].\n", retval);
                    return retval;
                }
                DBG_INFO("[DEBUG] CRMSRSTAT > prev.CRMSRSTAT -> set TRM=%d, A=0x%x, B=0x%x.\n", trm, paramA, paramB);
            }
            else {
                DBG_INFO("[DEBUG] CRMSRSTAT <= prev.CRMSRSTAT -> A=0x%x, B=0x%x.\n", trm, paramA, paramB);
            }
            break;
        }
    }

    // 7.
    retval = IZAN_clearMSRReq(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in set CROSC correction value : IZAN_clearMSRReq() failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}

/*-------------------------------------------------------------------
 * Function : IZAN_clearMSRReq
 *-----------------------------------------------------------------*/
/**
 * clear CRMSRREQ reg.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return else             (failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR IZAN_clearMSRReq(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;
    u8           readValue = 0;

    pDev = pCnlDev->pDev;

    // clear CRMSRREQ !!
    retval = IZAN_readRegister(pDev, REG_CRMSRCTL, 1, &readValue);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_clearMSRReq : read CRMSRCTL failed[%d].\n", retval);
    }
    else{
        readValue &= 0xFE;
        retval = IZAN_writeRegister(pDev, REG_CRMSRCTL, 1, &readValue);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_clearMSRReq : write CRMSRCTL(0x%x) failed[%d].\n", readValue, retval);
        }
        DBG_INFO("[DEBUG] clear CRMSRREQ. -> reg_w = 0x%x\n", readValue);
    }
    return retval;
}


static T_CNL_ERR IZAN_setCROSCTrim(S_CNL_DEV *pCnlDev, u16 trm)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;
    u8           writeValue = 0;

    pDev = pCnlDev->pDev;

    writeValue = (u8)(trm & 0x000F);
    retval = IZAN_writeRegister(pDev, REG_ZA_0x01915, 1, &writeValue);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    writeValue = 0x00;
    retval = IZAN_writeRegister(pDev, REG_ZA_0x01916, 1, &writeValue);
    if(retval != CNL_SUCCESS) {
        return retval;
    }
    DBG_INFO("[DEBUG] set TRM=%d.\n", trm);

    CMN_delayTask(1);

    writeValue = 0x01;
    retval = IZAN_writeRegister(pDev, REG_ZA_0x01914, 1, &writeValue);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    return CNL_SUCCESS;
}

/*-------------------------------------------------------------------
 * Function : IZAN_pollMSRResult
 *-----------------------------------------------------------------*/
/**
 * polling CRMSRFIN & get CRMSRRSLT
 * @param  pCnlDev      : the pointer to the CNL device.
 * @param  pResultValue : CRMSRRSLT parameter.
 * @return CNL_SUCCESS      (normally completion)
 * @return else             (failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR IZAN_pollMSRResult(S_CNL_DEV *pCnlDev, u16 *pResultValue)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;
    u8           readValue = 0;
    u8           retry = 0;

    pDev = pCnlDev->pDev;
    retry = 0;

    // polling CRMSRFIN !!
    do{
        retval = IZAN_readRegister(pDev, REG_CRMSRSTAT, 1, &readValue);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_pollMSRResult : read register failed[%d].\n", retval);
            return retval;
        }
        retry++;
        if(retry > IZAN_EXIT_CRMSR_POLL) {
            DBG_ERR("IZAN_pollMSRResult : CNL_ERR_TIMEOUT retry:%d\n", retry);
            return CNL_ERR_TIMEOUT;
        }
    } while ((readValue & 0x01) == 0x00);

    // get CRMSRRSLT
    retval = IZAN_readRegister(pDev, REG_CRMSRRSLT1, 1, &readValue);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_pollMSRResult : read register failed[%d].\n", retval);
        return retval;
    }
    *pResultValue = (u16)readValue;
    *pResultValue <<= 8;

    retval = IZAN_readRegister(pDev, REG_CRMSRRSLT0, 1, &readValue);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_pollMSRResult : read register failed[%d].\n", retval);
        return retval;
    }
    *pResultValue += (u16)readValue;

    DBG_INFO("[DEBUG] get CRMSRRSLT -> 0x%x\n", *pResultValue);

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_execCalibration
 *-----------------------------------------------------------------*/
/**
 * execute Calibration sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_execCalibration(S_CNL_DEV *pCnlDev)
{
    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_setMonitorSwitch
 *-----------------------------------------------------------------*/
/**
 * set monitor switch registers
 * @param  pCnlDev : the pointer to the CNL device.
 * @param  flag    : TRUE: enable FALSE: disable.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_setMonitorSwitch(S_CNL_DEV *pCnlDev, int flag)
{
    T_CNL_ERR    retval;
    void        *pDev;
    u32          tmpval  = 0;
    u32          clkgate = 0;
    u32          ctrltm  = 0;

    pDev        = pCnlDev->pDev;

    //
    // monitor switch initialize sequence.
    //
    // 1. set 0x000140D0(REG_RATECNTQ) register to 0x03040004
    // 2. set 0x00014380(REG_TXSTATEN) register to 0x000FFC13
    // 3. set 0x00014384(REG_RXSTATEN) register to 0x00000001
    // 4. set 0x00015528(REG_PHYRX_EVM_ENABLE) register to 0x00000001
    // 5. set 0x0001563C(REG_PHYRXEQ_ELCOUNT_EN) register to 0x00000001
    // 6. set 0x0001527C(REG_PHYRFCTRL_DCAMP_EN) register to 0x00000001
    // 7. set 0x00015004(REG_TESTMODULECLKGATE) register to 0x00000001
    // 8. set 0x00015020(REG_CONTROLTESTMODE) register to 0x00000002
    // 9. PHY receive infomation clear (set 1 and 0 to reg.0x00015040)

    if(CMN_getMonitorSwitch() != MONSW_ON){
        // monitor off or other
        return CNL_SUCCESS;
    }

    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x000FFC13);
    }
    retval = IZAN_writeRegister(pDev, REG_TXSTATEN, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set REG_TXSTATEN(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x003FFFF8);
    }
    retval = IZAN_writeRegister(pDev, REG_RXSTATEN, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set REG_RXSTATEN(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x00000001);
    }
    retval = IZAN_writeRegister(pDev, REG_PHYRX_EVM_ENABLE, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set REG_PHYRX_EVM_ENABLE(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x00000001);
    }
    retval = IZAN_writeRegister(pDev, REG_PHYRXEQ_ELCOUNT_EN, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set REG_PHYRXEQ_ELCOUNT_EN(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x00000001);
    }
    retval = IZAN_writeRegister(pDev, REG_PHYRFCTRL_DCAMP_EN, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set REG_PHYRFCTRL_DCAMP_EN(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    if(flag == TRUE){
        clkgate = CMN_H2LE32(0x00000001);
    }
    retval = IZAN_writeRegister(pDev, REG_TESTMODULECLKGATE, 4, &clkgate);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set TEST_MODULE_CLKGATE(0x%x) failed[%d].\n", clkgate, retval);
        return retval;
    }

    if(flag == TRUE){
        ctrltm = CMN_H2LE32(0x00000002);
    }
    retval = IZAN_writeRegister(pDev, REG_CONTROLTESTMODE, 4, &ctrltm);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setMonitorSwitch : set CONTROL_TEST_MODE(0x%x) failed[%d].\n", ctrltm, retval);
        return retval;
    }

    // 9. PHY receive infomation clear
    if(flag == TRUE){
        tmpval = CMN_H2LE32(0x00000001);
        retval = IZAN_writeRegister(pDev, REG_PHY_RXCOUNT_CLEAR, 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_setMonitorSwitch : set REG_PHY_RXCOUNT_CLEAR(0x%x) failed[%d].\n", tmpval, retval);
            return retval;
        }
    
        tmpval = 0;
        retval = IZAN_writeRegister(pDev, REG_PHY_RXCOUNT_CLEAR, 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_setMonitorSwitch : set REG_PHY_RXCOUNT_CLEAR(0x%x) failed[%d].\n", tmpval, retval);
            return retval;
        }
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_setModeSelect
 *-----------------------------------------------------------------*/
/**
 * set rate control registers
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_setModeSelect(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR    retval;
    void        *pDev;
    u32          tmpval = 0;

    pDev        = pCnlDev->pDev;

    //
    // rate control setting
    //
    // 1. set 0x000140D0(REG_RATECNTQ) register to 0x03040004
    // 2. set 0x000140D4(REG_RATECNTS) register to 0x10201020
    // 3. set 0x000140DC(REG_RATEOFF)  register to 0x00000000
    //   (1.-3. is invalid when fixed rate.)
    //
    // 4. set 0x000140D8(REG_RATECTRLINIT) register to 0x00000001
    // 5. set 0x000140E0(REG_RATE) register to 0x43000000
    //   (When fixed rate, set register to 0x03000000.
    //    Using transmission information, set rate in hosts.)
    // 6. set 0x000140D8(REG_RATECTRLINIT) register to 0x00000000

    if((CMN_getModeSelect() != MD_LINKADPT) && 
       (CMN_getModeSelect() != MD_FIXDRATE))
    {
        return CNL_SUCCESS;
    }

    if(CMN_getModeSelect() == MD_LINKADPT){
        //link adaptation
        tmpval = CMN_H2LE32(IZAN_RATECNTQ);
        retval = IZAN_writeRegister(pDev, REG_RATECNTQ, 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_setModeSelect : set REG_RATECNTQ(0x%x) failed[%d].\n", tmpval, retval);
            return retval;
        }
    
        tmpval = CMN_H2LE32(IZAN_RATECNTS);
        retval = IZAN_writeRegister(pDev, REG_RATECNTS, 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_setModeSelect : set REG_RATECNTS(0x%x) failed[%d].\n", tmpval, retval);
            return retval;
        }
    
        tmpval = CMN_H2LE32(IZAN_RATEOFF);
        retval = IZAN_writeRegister(pDev, REG_RATEOFF, 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_setModeSelect : set REG_RATEOFF(0x%x) failed[%d].\n", tmpval, retval);
            return retval;
        }
    }

    tmpval = CMN_H2LE32(0x000000001);
    retval = IZAN_writeRegister(pDev, REG_RATECTRLINIT, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setModeSelect : set REG_RATECTRLINIT(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }


    tmpval = IZAN_DEFAULT_RATE_REG_VALUE;  // Manual(fixed rate)
    if(CMN_getModeSelect() == MD_LINKADPT){
        tmpval = (tmpval & 0x3FFFFFFF) | RATE_LC_AUTO; // link adaptation
    } else {
        tmpval = (tmpval & 0x3FFFFFFF) | RATE_MANUAL; // Manual rate
    }

    //tmpval = (tmpval & 0x3FFFFFFF) | RATE_MANUAL; // Manual rate
    tmpval = CMN_H2LE32(tmpval);
    retval = IZAN_writeRegister(pDev, REG_RATE, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setModeSelect : set REG_RATE(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    tmpval = CMN_H2LE32(0x000000000);
    retval = IZAN_writeRegister(pDev, REG_RATECTRLINIT, 4, &tmpval);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setModeSelect : set REG_RATECTRLINIT(0x%x) failed[%d].\n", tmpval, retval);
        return retval;
    }

    return CNL_SUCCESS;
}

/*-------------------------------------------------------------------
 * Function : IZAN_readMonitor
 *-----------------------------------------------------------------*/
/**
 * output monitor registers.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readMonitor(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR    retval = CNL_SUCCESS;
    void        *pDev;
    int          i;
    u32          tmpval  = 0;

    pDev        = pCnlDev->pDev;

    if(CMN_getMonitorSwitch() != MONSW_ON){
        // monitor off or other
        return CNL_SUCCESS;
    }
    // register read and outout
    i = 0;
    while(g_monaddr[i] != 0xFFFFFFFF){
        retval = IZAN_readRegister(pDev, g_monaddr[i], 4, &tmpval);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN_readMonitor : read addr[0x%08X] [failed[%d].\n", g_monaddr[i], retval);
            return retval;
        }
        DBG_MON("monitor addr[0x%08X] = 0x%08X\n",g_monaddr[i], CMN_LE2H32(tmpval));
        i++;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_initializeCNL.
 *-----------------------------------------------------------------*/
/**
 * initialize CNL sequence.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_initializeCNL(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 mask;
    u32                 config;
    u32                 rate;
    u32                 uid;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    //
    // CNL initialize.
    //
    // 1. mask all CNL interrupt (use poll in initialize)
    // 2. enable interrupt
    // -- need other settings(use default value) --
    // 3. Bank number.
    // 4. OWN UID
    // 5. set TargetUID (0xFFFF-FF)
    // 6. initialize TxFIFO current address - deleted.
    // 7. initialize RxFIFO current address - deleted.
    // 8. set RATE auto.
    // 9. set BANKCLR TXDISABLE=0
    // 10.get FreqUpdN
    //

    // 1. unncessary
    mask = CMN_H2LE32(IZAN_INTEN_TO_MASK(IZAN_INTEN_NONE));
    retval = IZAN_writeRegister(pDev, REG_INTMASK, 4, &mask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in CNL Init : Mask interrupt failed[%d].\n", retval);
        return retval;
    }

    // 2.
    retval = IZAN_enableCnlInt(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : enable CNL int failed(0x%x)[%d].\n", retval);
        return retval;
    }

    // 3.
    config = CMN_H2LE32(IZAN_DEFAULT_CONFIG_REG_VALUE | (0x0F & IZAN_TX_CSDU_NUM));
    retval = IZAN_writeRegister(pDev, REG_CONFIG, 4, &config);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : write Config(0x%x)[%d].\n", config, retval);
        return retval;
    }

    // 4.-- UID register is big-endian but register value is little endian
    uid    = CMN_H2LE32(*(((u32 *)pDeviceData->ownUID)+1));
    retval = IZAN_writeRegister(pDev, REG_OWNUID1, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set OWNUID[1] failed[%d].\n", retval);
        return retval;
    }
    uid    = CMN_H2LE32(*((u32 *)pDeviceData->ownUID));
    retval = IZAN_writeRegister(pDev, REG_OWNUID2, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set OWNUID[2] failed[%d].\n", retval);
        return retval;
    }

    // 5. -- UID register is big-endian but register value is little endian
    uid    = CMN_H2LE32(*(((u32 *)pDeviceData->targetUID)+1));
    retval = IZAN_writeRegister(pDev, REG_TARGETUID1, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set TARGETUID[1] failed[%d].\n", retval);
        return retval;
    }
    uid    = CMN_H2LE32(*((u32 *)pDeviceData->targetUID));
    retval = IZAN_writeRegister(pDev, REG_TARGETUID2, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set TARGETUID[2] failed[%d].\n", retval);
        return retval;
    }

    // Timer Setting
    retval = IZAN_setupTimer(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : setup timer failed[%d].\n", retval);
        return retval;
    }

    // 8.
    rate = CMN_H2LE32(IZAN_DEFAULT_RATE_REG_VALUE);
    retval = IZAN_writeRegister(pDev, REG_RATE, 4, &rate);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set RATE failed[%d].\n", retval);
        return retval;
    }

    // 10.
    g_freqUpdN = CMN_getFreqUpdN();
    if ((g_freqUpdN > CNL_FREQUPDN_MAX) || (g_freqUpdN < CNL_FREQUPDN_MIN)) {
        DBG_WARN("g_freqUpdN is default(%d) setting because FreqUpdN:%d. is invalid value. \n", CNL_DEFAULT_FREQUPDN_VALUE, g_freqUpdN);
        g_freqUpdN = CNL_DEFAULT_FREQUPDN_VALUE; 
    } 

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_setupTimer.
 *-----------------------------------------------------------------*/
/**
 * setup timer.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_setupTimer(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR  retval;
    void      *pDev;

    u32        cnlTimer;
    u8         pmuTimer;

    pDev  = pCnlDev->pDev;

    //
    // setup timers. 
    // 1. set T_CONNECT timer
    // 2. set T_ACCEPT timer
    // 3. set T_RETRY timer
    // 4. set T_RESEND timer
    // 5. set T_KEEPALIVE timer
    // 6. set TAS timer
    // 7. set TDS timer
    // 8. set SEARCH_DORMANT timer(PMU block)
    // 9. set HIBENRATE_DORMANT timer(PMU block)
    // 10-1. set REG_TRIFS register
    // 10-2. set REG_TCACCIFS register
    //

    // 1.
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TCONNECT_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TCONNECT, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set T_CONNECT timer failed[%d].\n", retval);
        return retval;
    }

    // 2.
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TACCEPT_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TACCEPT, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set T_ACCEPT timer failed[%d].\n", retval);
        return retval;
    }

    // 3.
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TRETRY_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TRETRY, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set T_RETRY timer failed[%d].\n", retval);
        return retval;
    }

    // 4.
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TRESEND_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TRESEND, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set T_RESEND timer failed[%d].\n", retval);
        return retval;
    }

    // 5.
    cnlTimer = (u32)IZAN_PMUBLK_TIM_TO_CLK(IZAN_TKEEPALIVE_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TKEEPALIVE, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set T_KEEPALIVE timer failed[%d].\n", retval);
        return retval;
    }

    // 6. 
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TAS_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TAS, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set TAS timer failed[%d].\n", retval);
        return retval;
    }

    // 7. 
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(IZAN_TAC_TIMER);
    retval   = IZAN_writeRegister(pDev, REG_TAC, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set TAC timer failed[%d].\n", retval);
        return retval;
    }

    // 8
    pmuTimer = (u8)(IZAN_PMUBLK_TIM_TO_CLK(IZAN_SRCHDMT_TIMER) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_SRCHDMTTIM0, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set SRCHDMTTIM(low) timer failed[%d].\n", retval);
        return retval;
    }
    pmuTimer = (u8)((IZAN_PMUBLK_TIM_TO_CLK(IZAN_SRCHDMT_TIMER) >> 8) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_SRCHDMTTIM1, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set SRCHDMTTIM(high) timer failed[%d].\n", retval);
        return retval;
    }


    // 9
    pmuTimer = (u8)(IZAN_PMUBLK_TIM_TO_CLK(IZAN_HBNTDMT_TIMER) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_HBNTDMTTIM0, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set HBNTDMTTIM(low) timer failed[%d].\n", retval);
        return retval;
    }
    pmuTimer = (u8)((IZAN_PMUBLK_TIM_TO_CLK(IZAN_HBNTDMT_TIMER) >> 8) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_HBNTDMTTIM1, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set HBNTDMTTIM(high) timer failed[%d].\n", retval);
        return retval;
    }

    // 10-1
    cnlTimer = IZAN_TRIFS;
    retval   = IZAN_writeRegister(pDev, REG_TRIFS, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set IZAN_TRIFS failed[%d].\n", retval);
        return retval;
    }

    // 10-2
    cnlTimer = IZAN_TCACCIFS;
    retval   = IZAN_writeRegister(pDev, REG_TCACCIFS, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SetupTimer : set IZAN_TCACCIFS failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*=================================================================*/
/* IZAN internal sequence functions                                */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_sleepToAwake
 *-----------------------------------------------------------------*/
/**
 * change PMU state Sleep to Awake
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sleepToAwake(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  pmuchg;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    //
    // PMU sleep(or deepsleep) to awake sequence
    //
    // 1. change state from Sleep to Awake.
    // 2. poll to be Awake.
    //


    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 1.
    pmuchg = PMUMCHG_TO_AWK;
    retval = IZAN_writeRegister(pDev, REG_PMUMCHG, 1, &pmuchg);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("write PMUMCHG(0x%x) failed[%d].\n", pmuchg, retval);
        goto EXIT;
    }

    // 2.
    retval = IZAN_pollU8Register(pDev,
                                 REG_PMUSTATE,
                                 PMUSTATE_AWK,
                                 0xFF,
                                 IZAN_PMUCHG_AWK_DELAY,
                                 IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("poll PMUSTATE  failed[%d].\n",retval);
        goto EXIT;
    }

    pDeviceData->pmuState = PMU_AWAKE;

EXIT:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}
                               

/*-------------------------------------------------------------------
 * Function : IZAN_awakeToSleep.
 *-----------------------------------------------------------------*/
/**
 * change PMU state awake to Sleep
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_awakeToSleep(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  pmuchg;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    //
    // PMU initialize sequence.
    //
    // 1. change state from Awake to Sleep.
    // 2. poll to be Sleep.
    //

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 1.
    pmuchg = PMUMCHG_TO_SLP;
    retval = IZAN_writeRegister(pDev, REG_PMUMCHG, 1, &pmuchg);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in Init Done : write PMUMCHG(0x%x) failed[%d].\n", pmuchg, retval);
        goto EXIT;
    }


    // 2.
    retval = IZAN_pollU8Register(pDev,
                                 REG_PMUSTATE,
                                 PMUSTATE_SLP,
                                 0xFF,
                                 IZAN_PMUCHG_SLP_DELAY,
                                 IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in Init Done : PMUSTATE can't sleep[%d].\n", retval);
        goto EXIT;
    }

    pDeviceData->pmuState = PMU_SLEEP;

EXIT:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : IZAN_sleepToDeepsleep.
 *-----------------------------------------------------------------*/
/**
 * change PMU state sleep to Deepsleep
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_INVSTAT  (invalid pmuState)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sleepToDeepsleep(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  pmuchg;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    if(pDeviceData->pmuState != PMU_SLEEP) {
        DBG_ERR("in CloseReset Done: PMUSTATE failed[%d].\n", pDeviceData->pmuState);
        return CNL_ERR_INVSTAT;
    }

    //
    // PMU sleep to deepsleep sequence.
    //
    // 1. change state from Sleep to Deepsleep.
    // 2. poll to be Deepsleep.
    //

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 1.
    pmuchg = PMUMCHG_TO_DPSLP;
    retval = IZAN_writeRegister(pDev, REG_PMUMCHG, 1, &pmuchg);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in CloseReset Done : write PMUMCHG(0x%x) failed[%d].\n", pmuchg, retval);
        goto EXIT;
    }


    // 2.
    retval = IZAN_pollU8Register(pDev,
                                 REG_PMUSTATE,
                                 PMUSTATE_DPSLP,
                                 0xFF,
                                 IZAN_PMUCHG_SLP_DELAY,
                                 IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("in CloseReset Done : PMUSTATE can't sleep[%d].\n", retval);
        goto EXIT;
    }

    pDeviceData->pmuState = PMU_DEEP_SLEEP;

EXIT:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : IZAN_resetTxFifo
 *-----------------------------------------------------------------*/
/**
 * reset TXFIFO bus address.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_resetTxFifo(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR    retval;
    void        *pDev;

    u8           txFifoAddr;

    // 
    // reset TxFIFO (AHB buf address) 
    //
    // 1. reset TxFIFO address to head(SDIO register)
    //

    pDev  = pCnlDev->pDev;

    // HostIF is not allowed CMD53, 

    // 1.
    txFifoAddr = 0;
    retval = IZAN_writeRegister(pDev, REG_TXFIFO_CUR_ADR_REG0, 1, &txFifoAddr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Reset TxFifo : write REG_TXFIFO_CUR_ADR_REG0 failed[%d].\n", retval);
        return retval;
    }

    txFifoAddr = 0;
    retval = IZAN_writeRegister(pDev, REG_TXFIFO_CUR_ADR_REG1, 1, &txFifoAddr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Reset TxFifo : write REG_TXFIFO_CUR_ADR_REG0 failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_resetRxFifo
 *-----------------------------------------------------------------*/
/**
 * reset RXFIFO bus address.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_resetRxFifo(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR    retval;
    void        *pDev;

    u8           rxFifoAddr;

    // 
    // reset RxFIFO (AHB buf address) 
    //
    // 1. reset RxFIFO address to head(SDIO register)
    //

    pDev  = pCnlDev->pDev;

    // 1.
    rxFifoAddr = 0;
    retval = IZAN_writeRegister(pDev, REG_RXFIFO_CUR_ADR_REG0, 1, &rxFifoAddr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Reset RxFifo : write REG_RXFIFO_CUR_ADR_REG0 failed[%d].\n", retval);
        return retval;
    }

    rxFifoAddr = 0;
    retval = IZAN_writeRegister(pDev, REG_RXFIFO_CUR_ADR_REG1, 1, &rxFifoAddr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Reset RxFifo : write REG_RXFIFO_CUR_ADR_REG0 failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_disableCnlInt
 *-----------------------------------------------------------------*/
/**
 * disable all interrupt from CNL Block(use SDIOIP register).
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_disableCnlInt(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR  retval;
    u8         mask;
    void      *pDev;

    //.
    // 1. use REG_CARDINTMASKREG1
    //

    pDev = pCnlDev->pDev;
    // 1.
    mask = 0;
    retval = IZAN_writeRegister(pDev, REG_CARD_INT_MASK_REG1, 1, &mask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("disable CNL Int : write REG_CARD_INT_MASK_REG1 failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_enableCnlInt
 *-----------------------------------------------------------------*/
/**
 * enable all interrupt from CNL Block(use SDIOIP register).
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_enableCnlInt(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR  retval;
    u8         mask;
    void      *pDev;

    //.
    // 1. use REG_CARDINTMASKREG1(AMSK is CNL interrupt)
    //

    pDev = pCnlDev->pDev;

    // 1.
    mask = 0x01;
    retval = IZAN_writeRegister(pDev, REG_CARD_INT_MASK_REG1, 1, &mask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("enable CNL Int : write REG_CARD_INT_MASK_REG1 failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_stopTxMngFrame
 *-----------------------------------------------------------------*/
/**
 * stop Tx Management frame.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_stopTxMngFrame(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 txmfstop;
    u32                 intst;


    //
    // 1. clear TXMFSTOPCONF interrupt 
    // 2. set TXMFSTOP 
    // 3. poll TXMFSTOPCONF is raised.
    // 4. clear TXMFSTOPCONF interrupt.
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 2.
    txmfstop = CMN_H2LE32(TXMFSTOP_ON);
    retval = IZAN_writeRegister(pDev, REG_TXMFSTOP, 4, &txmfstop);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxMngFrame : stop TXMF failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 3.
    retval = IZAN_pollU32Register(pDev,
                                  REG_INT,
                                  CMN_H2LE32(INT_TXMFSTOPCONF),
                                  CMN_H2LE32(INT_TXMFSTOPCONF),
                                  IZAN_STOP_TXMF_DELAY,
                                  IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxMngFrame : poll TXMFSTOPCONF interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 4.
    intst = CMN_H2LE32(INT_TXMFSTOPCONF);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxMngFrame : clear TXMFSTOPCONF interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : IZAN_stopTxDataFrame
 *-----------------------------------------------------------------*/
/**
 * stop Tx Data frame.
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_stopTxDataFrame(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 bankclr;
    u32                 intst;


    //.
    // 1. clear TXDATASTOPCONF interrupt 
    // 2. set TXDATASTOP(in BANKCLR)
    // 3. poll TXDATASTOPCONF is raised.
    // 4. set TXDISABLE(in BANKCLR)
    // 5. clear TXDATASTOPCONF interrupt
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 2.
    bankclr = CMN_H2LE32(BANKCLR_TXDATASTOP);
    retval = IZAN_writeRegister(pDev, REG_BANKCLR, 4, &bankclr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxDataFrame : stop TXDATA failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 3.
    retval = IZAN_pollU32Register(pDev,
                                  REG_INT,
                                  CMN_H2LE32(INT_TXDATASTOPCONF),
                                  CMN_H2LE32(INT_TXDATASTOPCONF),
                                  IZAN_STOP_TXDATA_DELAY,
                                  IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxDataFrame : poll TXDATASTOPCONF interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 4.
    bankclr = CMN_H2LE32(BANKCLR_TXDISABLE);
    retval = IZAN_writeRegister(pDev, REG_BANKCLR, 4, &bankclr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxDataFrame : stop TXDATA failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 5.
    intst  = CMN_H2LE32(INT_TXDATASTOPCONF);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("StopTxDataFrame : clear TXDATASTOPCONF interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : IZAN_clearTxRxBank
 *-----------------------------------------------------------------*/
/**
 * clear Tx/Rx bank
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   this function must be called in SEARCH state and PMU awake.
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_clearTxRxBank(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 bankclr;
    u32                 intst;
    u32                 command;

    //
    // 1. Search -> Close
    // 2. clear Tx/Rx bank
    // 3. Close  -> Search
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 1-1.
    command = CMN_H2LE32(CLMSTATE_CLOSE | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE);
    retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : set STMODE(0x%x) failed[%d].\n", command, retval);
        goto COMPLETE;
    }

    retval = IZAN_pollU32Register(pDev,
                                  REG_INT,
                                  CMN_H2LE32(INT_AWAKETOSLEEP),
                                  CMN_H2LE32(INT_AWAKETOSLEEP),
                                  IZAN_S2C_DELAY,
                                  IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : AWAKETOSLEEP interrupt not raised.\n");
        goto COMPLETE;
    }

    // 1-3
    intst = CMN_H2LE32(INT_AWAKETOSLEEP);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : clear AWAKETOSLEEP interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 2.
    bankclr = CMN_H2LE32(BANKCLR_TXBANK | BANKCLR_RXBANK | BANKCLR_TXDISABLE);
    retval = IZAN_writeRegister(pDev, REG_BANKCLR, 4, &bankclr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : clear Tx/Rx Bank failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 3-1.
    command = CMN_H2LE32(CLMSTATE_SEARCH | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE);
    retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : set STMODE(0x%x) failed[%d].\n", command, retval);
        goto COMPLETE;
    }

    retval = IZAN_pollU32Register(pDev,
                                  REG_INT,
                                  CMN_H2LE32(INT_SLEEPTOAWAKE | INT_AWAKEPRIOD),
                                  CMN_H2LE32(INT_SLEEPTOAWAKE | INT_AWAKEPRIOD),
                                  IZAN_C2S_DELAY,
                                  IZAN_POLL_RETRY);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : SLEEPTOAWAKE interrupt not raised.\n");
        goto COMPLETE;
    }

    // 3-3.
    intst = CMN_H2LE32(INT_SLEEPTOAWAKE | INT_AWAKEPRIOD);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ClearTxRxBank : clear SLEEPTOAWAKE interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;
}


/*-------------------------------------------------------------------
 * Function : IZAN_preProcCS
 *-----------------------------------------------------------------*/
/**
 * pre process of change state
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  state   : the CNL state.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_preProcCS(S_CNL_DEV   *pCnlDev, 
               T_CNL_STATE  prevState,
               T_CNL_STATE  newState)
{
    
    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;

    u8                  prevMain;
    u8                  prevSub;
    u8                  newMain;
    u8                  newSub;
    u32                 timerctl;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    prevMain = CNLSTATE_TO_MAINSTATE(prevState);
    prevSub  = CNLSTATE_TO_MAINSTATE(prevState);
    newMain  = CNLSTATE_TO_MAINSTATE(newState);
    newSub   = CNLSTATE_TO_SUBSTATE(newState);


    //
    // *(Not CLOSE) -> SEARCH
    // stop TX DataFrame and MngFrame.
    //
    if((newMain == CNL_STATE_SEARCH) && (prevMain != CNL_STATE_CLOSE)) {
        timerctl = CMN_H2LE32(TIMERCTL_DISABLE_KEEPALIVE);
        retval = IZAN_writeRegister(pDev, REG_TIMERCONTROL, 4, &timerctl);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PreProcCS : Disable KeepAlive timer failed[%d].\n", retval);
            return retval;
        }

        retval = IZAN_stopTxMngFrame(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PreProcCS : stop TX MngFrame failed[%d].\n", retval);
            return retval;
        }

        retval = IZAN_stopTxDataFrame(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PreProcCS : stop Tx DataFrame failed[%d].\n", retval);
            return retval;
        }

    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_postProcCS
 *-----------------------------------------------------------------*/
/**
 * post process of change state
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  state   : the CNL state.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_postProcCS(S_CNL_DEV   *pCnlDev, 
                T_CNL_STATE  prevState,
                T_CNL_STATE  newState)
{
    
    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;

    u8                  prevMain;
    u8                  prevSub;
    u8                  newMain;
    u8                  newSub;
    u32                 bankclr;
    u32                 intst;
    u32                 timerctl;

    S_CNL_DEVICE_EVENT  event;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    prevMain    = CNLSTATE_TO_MAINSTATE(prevState);
    prevSub     = CNLSTATE_TO_MAINSTATE(prevState);
    newMain     = CNLSTATE_TO_MAINSTATE(newState);
    newSub      = CNLSTATE_TO_SUBSTATE(newState);


    //
    // * -> CLOSE exec close reset.
    //
    if(newMain == CNL_STATE_CLOSE) {
        retval = IZAN_closeReset(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PostProcCS : closeReset failed[%d].\n", retval);
            return retval;
        }
    }


    //
    // *     -> SEARCH, exec release reset and Enable TXBANKEMPT intrrupt
    // CLOSE -> SEARCH  Enable TXBANKEMPT intrrupt
    //
    if(newMain == CNL_STATE_SEARCH) {
        if(prevMain != CNL_STATE_CLOSE){
            // monitor
            retval = IZAN_readMonitor(pCnlDev);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("PostProcCS : readMonitor failed[%d].\n", retval);
                return retval;
            }

            retval = IZAN_releaseReset(pCnlDev);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("PostProcCS : releaseReset failed[%d].\n", retval);
                return retval;
            }

            timerctl = CMN_H2LE32(0x00000000);
            retval = IZAN_writeRegister(pDev, REG_TIMERCONTROL, 4, &timerctl);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("PostProcCS : Enable KeepAlive timer failed[%d].\n", retval);
                return retval;
            }
        }

        //
        // bank clear(TXENABLE). and unmask TXBANKEMPT.
        //
        bankclr = CMN_H2LE32(BANKCLR_TXENABLE);
        retval = IZAN_writeRegister(pDev, REG_BANKCLR, 4, &bankclr);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("Connect Reset : clear Tx/Rx buffer failed[%d].\n", retval);
            return retval;
        }

        retval = IZAN_pollU32Register(pDev,
                                      REG_INT,
                                      CMN_H2LE32(INT_TXBANKEMPT),
                                      CMN_H2LE32(INT_TXBANKEMPT),
                                      0,
                                      IZAN_POLL_RETRY);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PostProcCS : poll TXBANKEMPT interrupt failed[%d].\n", retval);
            return retval;
        }
        intst = CMN_H2LE32(INT_TXBANKEMPT);
        retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PostProcCS : clear TXBANKEMPT interrupt failed[%d].\n", retval);
            return retval;
        }

        CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
        event.type |= CNL_EVENT_TX_READY;
        CNL_addEvent(pCnlDev, &event); 
    }

    //
    // CONNECTION_REQUEST -> RESPONDER_RESPONSE
    // RESPONDER_RESPONSE -> RESPONDER_CONNECTED and missing the ACK for C-Acc.
    // exec stopTxMngFrame(for C-Req Crossover)
    //
    if(((prevMain == CNL_STATE_CONNECTION_REQUEST) && (newMain == CNL_STATE_RESPONDER_RESPONSE)) ||
       ((prevMain == CNL_STATE_RESPONDER_RESPONSE) && (newMain == CNL_STATE_RESPONDER_CONNECTED) &&
        (pCnlDev->missCaccAck == TRUE))) {
        retval = IZAN_stopTxMngFrame(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("PostProcCS : stopTxMngFrame failed[%d].\n", retval);
            return retval;
        }
    }

    {
        u32               txmngbody1;

        //
        // CONNECTION_REQUEST -> RESPONSE_WAITING
        //
        if((prevMain == CNL_STATE_CONNECTION_REQUEST) && (newMain == CNL_STATE_RESPONSE_WAITING)) {
            txmngbody1 = 0;
            retval = IZAN_writeRegister(pDev, REG_TXMNGBODY1, 4, &txmngbody1);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("PostProcCS : write TXMNGBODY1 failed[%d].\n", retval);
                return retval;
            }
        }
    }

    return CNL_SUCCESS;
}


/*=================================================================*/
/* IZAN interrupt related functions                                */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_intToEvent
 *-----------------------------------------------------------------*/
/**
 * convert interrupt status to CNL event.
 * @param  pEvent : the pointer to the S_CNL_DEVICE_EVENT
 * @param  intst  : interrupt status bitmap
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
IZAN_intToEvent(S_CNL_DEVICE_EVENT *pEvent,
                u32                 intst)
{

    u32 bit, bitmap;
    u32 phybits = 0;

    bitmap = intst;

    do{
        bit = GET_PRIOR_BIT(bitmap);
        switch(bit) {
        //
        // management frame send complete interrupt.
        //
        case INT_CPROBETX : // for manual mode.
            pEvent->type    |= CNL_EVENT_COMP_REQUEST;
            pEvent->compReq |= CNL_COMP_PROBE_REQ;
            break;
        case INT_CSLEEPTX :
            pEvent->type    |= CNL_EVENT_COMP_REQUEST;
            pEvent->compReq |= CNL_COMP_SLEEP_REQ;
            break;
        case INT_CWAKETX :
            pEvent->type    |= CNL_EVENT_COMP_REQUEST;
            pEvent->compReq |= CNL_COMP_WAKE_REQ;
            break;
        case INT_CACCTX :
            // extended, notify event as ACK for C-Acc received.
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_ACK_FOR_C_ACC;
            break;
        case INT_CRLSTX :
            pEvent->type    |= CNL_EVENT_COMP_REQUEST;
            pEvent->compReq |= CNL_COMP_RELEASE_REQ;
            break;
        case INT_CACCACKTX :
            pEvent->type    |= CNL_EVENT_COMP_REQUEST;
            pEvent->compReq |= CNL_COMP_ACCEPT_RES;
            break;
            
        //
        // management frame received interrupt.
        // 
        case INT_CPROBERCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_PROBE;
            break;
        case INT_CSLEEPRCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_SLEEP;
            break;
        case INT_CWAKERCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_WAKE;
            break;
        case INT_CRLSRCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_RLS;
            break;
        case INT_CACCRCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_ACC;
            break;
        case INT_CREQRCV :
            pEvent->type      |= CNL_EVENT_RECVD_MNG_FRAME;
            pEvent->recvdLicc |= CNL_EXT_LICC_C_REQ;
            break;
            
        //
        // internal error interrupt.
        // 
        case INT_LCFATALERR :
            pEvent->type      |= CNL_EVENT_ERROR_OCCURRED;
            pEvent->error      = CNL_ERR_HW_ERROR1; // need to PowerOnReset
            break;
        case INT_IRRDFSN :
            pEvent->type      |= CNL_EVENT_ERROR_OCCURRED;
            pEvent->error      = CNL_ERR_HW_ERROR3; // need to release.
            break;

        //
        // PHY state change related interrupt.
        // 

        case INT_AWAKEPRIOD :
        case INT_SLEEPTOAWAKE :
        case INT_AWAKETOSLEEP :
            phybits |= bit;
            break;
        //
        // timer expired interrupt
        // 
        case INT_TKEEPATOUT :
            pEvent->type    |= CNL_EVENT_TIMEOUT;
            pEvent->timer   |= CNL_T_KEEPALIVE;
            break;
        case INT_TRESENDTOUT :
            pEvent->type    |= CNL_EVENT_TIMEOUT;
            pEvent->timer   |= CNL_T_RESEND;
            break;
        case INT_TRETRYTOUT :
            pEvent->type    |= CNL_EVENT_TIMEOUT;
            pEvent->timer   |= CNL_T_RETRY;
            break;
        case INT_TACCTOUT :
            pEvent->type    |= CNL_EVENT_TIMEOUT;
            pEvent->timer   |= CNL_T_ACCEPT;
            break;
        case INT_TCONNTOUT :
            pEvent->type    |= CNL_EVENT_TIMEOUT;
            pEvent->timer   |= CNL_T_CONNECT;
            break;

        //
        // RX data frame interrupt.
        // 
        case INT_RXBANKNOTEMPT :
            pEvent->type    |= CNL_EVENT_RX_READY;
            break;

        //
        // TX data frame interrupt.
        // 
        case INT_TXDFRAME :
            pEvent->type    |= CNL_EVENT_TX_COMP | CNL_EVENT_TX_READY;
            break;
        case INT_TXBANKEMPT :
            pEvent->type    |= CNL_EVENT_TX_READY;
            break;

        case INT_FCFATALERR :
            pEvent->type      |= CNL_EVENT_ERROR_OCCURRED;
            pEvent->error      = CNL_ERR_HW_ERROR2;  // need to close.
            break;
            
        //
        // unuse interrupt.
        // 
        case INT_FTX :
        case INT_TXBANKFULL :
        case INT_TXBANKONEEMPT :
            // unused interrupt.
//            DBG_WARN("unsupported interrupt[0x%08x].\n", bit);
//            DBG_ASSERT(0);
            break;
        default : 
//            DBG_WARN("unknown interrupt[0x%08x]\n", bit);
//            DBG_ASSERT(0);
            break;
        }
        CLEAR_BIT(bitmap, bit);
    }while(bitmap != 0);

    switch(phybits) {
    case INT_AWAKEPRIOD :
        phybits |= bit;
        pEvent->type    |= CNL_EVENT_COMP_REQUEST;
        pEvent->compReq |= CNL_COMP_WAKE;
        break;
    case INT_SLEEPTOAWAKE | INT_AWAKEPRIOD :
        pEvent->type    |= CNL_EVENT_COMP_REQUEST;
        pEvent->compReq |= CNL_COMP_INIT;
        break;
    case INT_AWAKETOSLEEP :
        pEvent->type    |= CNL_EVENT_COMP_REQUEST;
        pEvent->compReq |= CNL_COMP_CLOSE;
        break;
    }

    return;
}


/*-------------------------------------------------------------------
 * Function : IZAN_addIntUnmask
 *-----------------------------------------------------------------*/
/**
 * add unmask interrupt.
 * @param  pCnlDev : the pointer to the CNL device.
 * @param  unmask  : bitmap to be unmask.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_addIntUnmask(S_CNL_DEV *pCnlDev,
                  u32        unmask)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 newmask;
    u32                 intmask;

    //
    // 1. set INTMASK 
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    newmask = IZAN_INTEN_TO_MASK(pDeviceData->currIntEnable | unmask);

    intmask = CMN_H2LE32(newmask);
    
    // 1.
    retval = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("AddIntUnmask : write INTMASK failed[%d].\n", retval);
        goto COMPLETE;
    }

    DBG_INFO("AddIntUnmask : INTMASK modified [Enable(0x%08x)] -> [Enable(0x%08x)].\n",
             pDeviceData->currIntEnable, (pDeviceData->currIntEnable | unmask));

    pDeviceData->currIntEnable = pDeviceData->currIntEnable | unmask;

COMPLETE :
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;

}


/*=================================================================*/
/* IZAN Common Bus interface related functions                     */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_irqHandler
 *-----------------------------------------------------------------*/
/**
 * default IRQ handler.
 * @param  pArg  : the pointer to the S_CNL_DEV
 * @return nothing.
 * @note
 */
/*-----------------------------------------------------------------*/
static void
IZAN_irqHandler(void *pArg)
{

    T_CNL_ERR           retval;
    S_CNL_DEV          *pCnlDev;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    S_CNL_DEVICE_EVENT  event;

    u32                 newmask;
    u32                 intmask;
    u32                 intst = 0;
    u32                 bits;

    pCnlDev     = (S_CNL_DEV *)pArg;
    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));

    //
    // normal IRQ Handler sequence.
    //
    // 1. read interrupt status.
    // 2. clear interrupt
    // 3. mask interrupt
    // 4. convert interrupt status to event.
    // 5. notify event.
    //

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    if(pDeviceData->pmuState == PMU_POWERSAVE) {
        //
        // interrupt raised while powersave state.
        // disable CNL interrupt and notify event.
        //
        DBG_INFO("Interrupt raised while POWERSAVE sate.\n");

        retval = IZAN_disableCnlInt(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IRQ Handler : disable CNL interrupt failed[%d].\n", retval);
            goto EXIT;
        }

        event.type = CNL_EVENT_WAKE_REQUIRED;
        goto EXIT;
    }

    // 1.
    retval = IZAN_readRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
        DBG_ERR("IRQ Handler : read REG_INT failed[%d].\n", retval);
        return;
    }
    intst = CMN_LE2H32(intst);

    // bits interrupt bitmap.
    bits = pDeviceData->currIntEnable & intst;

    // 2. write interrupt status(clear interrupt)
    intst = CMN_H2LE32(bits);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
        DBG_ERR("IRQ Handler : write REG_INT[0x%08x] failed[%d].\n", intst, retval);
        return;
    }

    {
        u32  rxbanksta;
        u32  rxbankcnt;

        if (bits & INT_RXBANKNOTEMPT)
        {
            retval = IZAN_readRegister(pDev, REG_RXBANKSTA, 4, &rxbanksta);
            if(retval != CNL_SUCCESS) {
                CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
                DBG_ERR("IRQ Handler : read RXBANKSTA failed[%d].\n", retval);
                return;
            }

            rxbankcnt = CMN_LE2H32(rxbanksta) & 0x0000000F;

            if(rxbankcnt == 0) { 
                DBG_INFO("=======> RXBANKEMPTINT should be ignored because RXBANKCNT = %d \n",
                          rxbankcnt);
                bits &= ~INT_RXBANKNOTEMPT;
            }
            if (IZAN_readRxvgagain(pCnlDev) != CNL_SUCCESS) {
                DBG_ERR("IRQ Handler : read RXVGAGAIN failed.\n");
            }
        }
    }

    newmask = IZAN_INTEN_TO_MASK(pDeviceData->currIntEnable & ~bits);

    // 3. write interrupt mask(if necessary)
    intmask = CMN_H2LE32(newmask);
    retval  = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
        DBG_ERR("IRQ Handler : write REG_INT[0x%08x] failed[%d].\n", intmask, retval);
        return;
    }

    DBG_INFO("IRQ Handler : INTMASK modified [Enable(0x%08x)] -> [Enable(0x%08x)].\n",
             pDeviceData->currIntEnable, pDeviceData->currIntEnable & ~bits);

    pDeviceData->currIntEnable = pDeviceData->currIntEnable & ~bits;

    // 4.convert interrupt status to Event type.
    IZAN_intToEvent(&event, bits);

    DBG_INFO("IRQ Handler : converted INT[0x%08x] to Event[0x%x]\n", 
             bits, event.type);


    if(bits & INT_TRESENDTOUT) {
        //
        // if T_Resend timeouted, need to TX resend when
        // state is changed to CONNECTED.
        //
        pDeviceData->txNeedResend = TRUE;

    }

EXIT:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    // 5.  
    CNL_addEvent(pCnlDev, &event);

    return;

}

/*-------------------------------------------------------------------
 * Function : IZAN_suspend
 *-----------------------------------------------------------------*/
/**
 * This function add event for supend/resume notify
 * @param  type  (0:Resumed  1:Suspended)
 * @return nothing
 * @note
 */
/*-----------------------------------------------------------------*/
static void IZAN_suspend(int type)
{
    if( stored_dev ) {
        if( stored_dev->devState == CNL_DEV_ACTIVE ) {
            S_CNL_DEVICE_EVENT event={0};
            event.type |= CNL_EVENT_ERROR_OCCURRED;
            if( type ) {
                event.error = CNL_ERR_HW_SUSPEND;
            }else{
                event.error = CNL_ERR_HW_RESUME;
            }
            CNL_addEvent(stored_dev, &event);
        }
    }
}

/*-------------------------------------------------------------------
 * Function : IZAN_probe
 *-----------------------------------------------------------------*/
/**
 * callback function for IZAN device inserted.
 * @param  pDev : access handler to the device.
 * @param  pIds  : the pointer to the S_BUSCMN_IDS.
 * @return 0  (normally completion)
 * @return -1 (failed dureing probe sequence)
 * @note   
 */
/*-----------------------------------------------------------------*/
static int
IZAN_probe(void         *pDev,
           S_BUSCMN_IDS *pIds)
{

    T_CMN_ERR           retval;
    T_CNL_ERR           status;
    S_CNL_DEV          *pCnlDev;
    S_IZAN_DEVICE_DATA *pDeviceData;

    //
    // allocate and set S_CNL_DEV
    //
    pCnlDev = CNL_allocDevice();
    if(pCnlDev == NULL) {
        return -1;
    }

    //
    // set device dependent data and function.
    //
    pCnlDev->pDev       = pDev;
    pCnlDev->pDeviceOps = &g_cnlDeviceOps;

    //
    // setup device data.
    //
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    IZAN_initDeviceData(pDeviceData);

    pDeviceData->intLockId = CNL_INT_MTX_ID;
    retval = CMN_INIT_MUTEX(pDeviceData->intLockId);
    if(retval != SUCCESS) {
        DBG_ERR("create interrupt mutex object failed[%d].\n", retval);
        CNL_releaseDevice(pCnlDev);
        return -1;
    }

    // 2. card initialize sequence.
    status = IZAN_initializeDevice(pCnlDev);
    if(status != CNL_SUCCESS) {
        CMN_deleteSem(pDeviceData->intLockId);
        CNL_releaseDevice(pCnlDev);
        return -1;
    }

    // 3. set irq handler.
    retval = BUSCMN_setIrqHandler(pDev,
                                  IZAN_irqHandler, 
                                  (void *)pCnlDev);
    if(retval != SUCCESS) {
        CMN_deleteSem(pDeviceData->intLockId);
        CNL_releaseDevice(pCnlDev);
        return -1;
    }

    retval = CNL_registerDevice(pCnlDev);
    if(retval != SUCCESS) {
        DBG_ERR("register device failed.\n");
        CMN_deleteSem(pDeviceData->intLockId);
        CNL_releaseDevice(pCnlDev);
        return -1;
    }

    // create Lock Mng.
    CMN_createPowerLock();

    stored_dev = pCnlDev;
    CMN_setSuspendEvent(IZAN_suspend);

    return 0;
}


/*-------------------------------------------------------------------
 * Function : IZAN_remove
 *-----------------------------------------------------------------*/
/**
 * callbak function for IZAN device rejected.
 * @param  pDev : access handler to device.
 * @return 0  (normally completion)
 * @return -1 (failed during remove sequence)
 * @note   
 */
/*-----------------------------------------------------------------*/
static int
IZAN_remove(void *pDev)
{

    S_CNL_DEV          *pCnlDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  lockId;
    
    DBG_INFO("CNL device is removed.\n");
    pCnlDev     = CNL_devToCnlDev(pDev);

    if(pCnlDev == NULL) {
        DBG_ERR("not found CNL device related device.\n");
        return -1;
    }
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    lockId = pDeviceData->intLockId;
    CNL_unregisterDevice(pCnlDev);
    CMN_deleteSem(lockId);

    CMN_clearSuspendEvent();
    stored_dev = NULL;

    // unlock WakeLock & delete Lock Mng.
    CMN_unlockPower();
    CMN_deletePowerLock();
    return 0;
}


/*-------------------------------------------------------------------
 * Function : IZAN_releaseDeviceData
 *-----------------------------------------------------------------*/
/**
 * device data field cleanup function
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
IZAN_releaseDeviceData(S_CNL_DEV *pCnlDev)
{

    S_IZAN_DEVICE_DATA *pDeviceData;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    
    CMN_deleteSem(pDeviceData->intLockId);

    return;
}


/*-------------------------------------------------------------------
 * Function : CNL_registerDriver
 *-----------------------------------------------------------------*/
/**
 * register CNL device driver to common bus driver
 * @param  nothing.
 * @return SUCCESS    (normally completion)
 * @return ERR_SYSTEM (unregister failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_registerDriver() 
{
    T_CMN_ERR retval;

    retval = BUSCMN_registerDriver(&g_cnlDriver);
    if(retval != SUCCESS) {
        DBG_ERR("register CNL device driver failed.\n");
        return ERR_SYSTEM;
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : CNL_unregisterDriver
 *-----------------------------------------------------------------*/
/**
 * unregister CNL device driver from common bus driver
 * @return SUCCESS    (normally completion)
 * @return ERR_SYSTEM (unregister failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
CNL_unregisterDriver() 
{
    int retval;

    retval = BUSCMN_unregisterDriver(&g_cnlDriver);
    if(retval != 0) {
       DBG_ERR("unregister CNL IZAN device driver failed.\n");
        return ERR_SYSTEM;
    }

    return SUCCESS;
}


/*=================================================================*/
/* External Device Operations                                      */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Function : IZAN_getDeviceParm
 *-----------------------------------------------------------------*/
/**
 * get device dependent CNL parameters.
 * @param  pCnlDev     : the pointer to the S_CNL_DEV
 * @param  pDeviceParm : the poniter to the S_CNL_DEVICE_PARM
 * @return SUCCESS (normally completion)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_getDeviceParam(S_CNL_DEV          *pCnlDev,
                    S_CNL_DEVICE_PARAM *pDeviceParam)
{

    // own UID is arleady read
    S_IZAN_DEVICE_DATA *pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_MEMCPY(pDeviceParam->ownUID, pDeviceData->ownUID, CNL_UID_SIZE);
    pDeviceParam->txCsduNum   = IZAN_TX_CSDU_NUM;
    pDeviceParam->rxCsduNum   = IZAN_RX_CSDU_NUM;
    pDeviceParam->tConnect    = IZAN_TCONNECT_TIMER;
    pDeviceParam->tAccept     = IZAN_TACCEPT_TIMER;
    pDeviceParam->tRetry      = IZAN_TRETRY_TIMER;
    pDeviceParam->tResend     = IZAN_TRESEND_TIMER;
    pDeviceParam->tKeepAlive  = IZAN_TKEEPALIVE_TIMER;
    pDeviceParam->tas         = IZAN_TAS_TIMER;
    pDeviceParam->tds         = IZAN_SRCHDMT_TIMER;
    pDeviceParam->tac         = IZAN_TAC_TIMER;
    pDeviceParam->tdc         = IZAN_HBNTDMT_TIMER;

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_init
 *-----------------------------------------------------------------*/
/**
 * initialize IZAN device.
 * @param  pCnlDev     : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_init(S_CNL_DEV *pCnlDev)

{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 command;
    u8                  dmtoff;
    u8                  i;
    u32                 uid;
    u32                 tmp_uid1 = 0;
    u32                 tmp_uid2 = 0;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    // 
    // IZAN init(CNL_INIT.request) sequence.
    //
    // 0. card initialize sequence.
    // 1. chang PMU state Sleep to Awake
    // 2. execute calibration(first time only)
    // 3. set DMTOFF to 1(disable dormant)
    // 4. set STMODE search.
    // 5. set int mask(SLEEPTOAWAKEINT) or polling.
    //

    // lock WakeLock. 
    CMN_lockPower();

    // 0. card initialize sequence.
    if (pDeviceData->pmuState ==  PMU_DEEP_SLEEP) {
        retval = IZAN_initializeDevice(pCnlDev);
        if (retval != CNL_SUCCESS) {
            DBG_ERR("card re-initialize sequence failed.\n");
            // unlock WakeLock. 
            CMN_unlockPower();
            return retval;
        }
    }

    retval = IZAN_sleepToAwake(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("change PMU sleep to awake failed.\n");
        // unlock WakeLock. 
        CMN_unlockPower();
        return retval;
    }

    // 2.
    if(pDeviceData->needCal) {
        retval = IZAN_execCalibration(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("calibration failed[%d].\n", retval);
            // unlock WakeLock. 
            CMN_unlockPower();
            return retval;
        }
        pDeviceData->needCal = FALSE;
    }

    /* monitor function set to enable */
    retval = IZAN_setMonitorSwitch(pCnlDev,TRUE);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN Init : set monitor switch failed[%d].\n", retval);
        // unlock WakeLock.
        CMN_unlockPower();
        return retval;
    }

    /* mode select (fixed rate or link adaptation) */
    retval = IZAN_setModeSelect(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN Init : set mode select failed[%d].\n", retval);
        // unlock WakeLock.
        CMN_unlockPower();
        return retval;
    }

    // 3. disable Dormant
    dmtoff = DMT_DISABLE;
    retval = IZAN_writeRegister(pDev, REG_DMTOFF, 1, &dmtoff);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN Init : set DMTOFF(0x%x) failed[%d].\n", dmtoff, retval);
        // unlock WakeLock. 
        CMN_unlockPower();
        return retval;
    }

    // set ownUID
    // pCnlDev->deviceParam.ownUID check
    for (i=0; i < CNL_UID_SIZE; i++) {
        if (pCnlDev->deviceParam.ownUID[i] != 0x00) {
            break;
        }
    }
    // All ZERO?
    if (i == CNL_UID_SIZE) {
        // Yes:use eFuse data
        retval = IZAN_readRegister(pDev, REG_SYS_UID_D, 4, &uid);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("Initialize CNL : read SYS_UID_D failed[%d].\n", retval);
            // unlock WakeLock. 
            CMN_unlockPower();
            return retval;
        }
        // Vendor Code [19:12],[11:4]
        tmp_uid1 = (uid << 16);
        
        retval = IZAN_readRegister(pDev, REG_SYS_UID_C, 4, &uid);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("Initialize CNL : read SYS_UID_C failed[%d].\n", retval);
            // unlock WakeLock. 
            CMN_unlockPower();
            return retval;
        }
        // Vendor Code [3:0]
        tmp_uid1 |= (uid & 0xF000);
        // Extension Identifier [27:24],[23:16]
        tmp_uid2 = ((uid & 0x0FFF) << 16);
        
        retval = IZAN_readRegister(pDev, REG_SYS_UID_B, 4, &uid);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("Initialize CNL : read SYS_UID_B failed[%d].\n", retval);
            // unlock WakeLock. 
            CMN_unlockPower();
            return retval;
        }
        // Extension Identifier [15:8],[7:0]
        tmp_uid2 |= (uid & 0xFFFF);

        // set pDeviceData->ownUID
        *(((u32 *)pDeviceData->ownUID)+1) = CMN_LE2H32(tmp_uid1);
        *((u32 *)pDeviceData->ownUID) = CMN_LE2H32(tmp_uid2);
        // update pCnlDev->deviceParam.ownUID
        CMN_MEMCPY(pCnlDev->deviceParam.ownUID, pDeviceData->ownUID, CNL_UID_SIZE);
    } else {
        // No:use Request Parameter data
        // set pDeviceData->ownUID
        CMN_MEMCPY(pDeviceData->ownUID, pCnlDev->deviceParam.ownUID, CNL_UID_SIZE);
    }

    // UID register is big-endian but register value is little endian
    uid    = CMN_H2LE32(*(((u32 *)pDeviceData->ownUID)+1));
    retval = IZAN_writeRegister(pDev, REG_OWNUID1, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set OWNUID[1] failed[%d].\n", retval);
        return retval;
    }
    uid    = CMN_H2LE32(*((u32 *)pDeviceData->ownUID));
    retval = IZAN_writeRegister(pDev, REG_OWNUID2, 4, &uid);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("Initialize CNL : set OWNUID[2] failed[%d].\n", retval);
        return retval;
    }

    // 4.
    command = CLMSTATE_SEARCH | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
    retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN Init : set STMODE(0x%x) failed[%d].\n", command, retval);
        // unlock WakeLock. 
        CMN_unlockPower();
        return retval;
    }

    // 5. 
    retval = IZAN_addIntUnmask(pCnlDev, INT_SLEEPTOAWAKE | INT_AWAKEPRIOD);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN Init : set INTMASK(0x%x) failed[%d].\n", INT_SLEEPTOAWAKE, retval);
        // unlock WakeLock. 
        CMN_unlockPower();
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_close
 *-----------------------------------------------------------------*/
/**
 * close IZAN device.
 * @param  pCnlDev : the pointer to the CNL device
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_close(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    u8                  mainState;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 command;
    u32                 bankclr;

    //
    // IZAN close(CNL_CLOSE.request) sequence.
    //
    // 1. reset to SEARCH state.
    // 2. change STMODE close.
    // 3. enable AWAKETOSLEEPINT
    //

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    mainState   = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    // 1.
    if(mainState != CNL_STATE_SEARCH) {
        retval = IZAN_changeState(pCnlDev, pCnlDev->cnlState,
                                  MAKE_CNLSTATE(CNL_STATE_SEARCH, CNL_SUBSTATE_NULL));
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN close : ChangeState to Search failed[%d].\n", retval);
            return retval;
        }
    }

    /* monitor function set to disable */
    retval = IZAN_setMonitorSwitch(pCnlDev,FALSE);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN close : set monitor switch failed[%d].\n", retval);
        return retval;
    }

    bankclr = CMN_H2LE32(BANKCLR_TXDISABLE); // disable TX.
    retval = IZAN_writeRegister(pDev, REG_BANKCLR, 4, &bankclr);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN close : TX disable failed[%d].\n", retval);
        return retval;
    }

    // 2.
    command = CLMSTATE_CLOSE | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE; // ACK disable and AUTO Probe.
    retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN close : set STMODE(0x%x) failed[%d].\n", command, retval);
        return retval;
    }

    // 3. 
    retval = IZAN_addIntUnmask(pCnlDev, INT_AWAKETOSLEEP);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN close : set unmask(AWAKETOSLEEP) failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_wake
 *-----------------------------------------------------------------*/
/**
 * wakeup IZAN device.(powersave -> awake)
 * @param  pCnlDev : the pointer to the CNL device
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_wake(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  dmtoff;
    u8                  dmtexit;
    u8                  pmustate = 0;
    u8                  ok, retry;
    u32                 intst;
    u32                 intmask;
    u16                 delay;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    
    delay = (CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_SEARCH) ? \
        IZAN_TA_TIM_TO_POLL(IZAN_TAS_TIMER) : IZAN_TA_TIM_TO_POLL(IZAN_TAC_TIMER);

    DBG_INFO("Awake Polling Interval is %u msec\n", delay);
    //
    // Dormant to Awake sequence.
    // -- see IZAN system specification.
    //
    // this sequence is a little tricky.
    // 1. disable all CNL interrupt.
    // 2. set DMTOFF  0x01
    // 3. set DMTEXIT 0x01
    // 4. poll wakeup (msec order).
    // 5. reset DMTEXIT 0x00
    // 6. unmask AWAKEPRIOD interrupt
    // 7. enable all CNL interrupt after wakeup
    //

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    // 1.
    retval = IZAN_disableCnlInt(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : disable All CNL interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 2.
    dmtoff = DMT_DISABLE;
    retval = IZAN_writeRegister(pDev, REG_DMTOFF, 1, &dmtoff);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : set DMTOFF(0x%x) failed[%d].\n", dmtoff, retval);
        goto COMPLETE;
    }

    // 3.
    dmtexit = DMTEXIT_ON;
    retval = IZAN_writeRegister(pDev, REG_DMTEXIT, 1, &dmtexit);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : set DMTEXIT(0x%x) failed[%d].\n", dmtexit, retval);
        goto COMPLETE;
    }
    
    // 4.
    ok = 0;
    retry = 0;
    do {
        CMN_delayTask(delay);
        retval = IZAN_readRegister(pDev, REG_PMUSTATE, 1, &pmustate);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("IZAN wake : read REG_PMUSTATE failed[%d].\n", retval);
            goto COMPLETE;
        }
        if(pmustate == PMUSTATE_AWK) {
            ok++;
        } else {
            ok = 0;
        }
        if(retry++ > IZAN_EXIT_DMT_POLL) {
            DBG_ERR("IZAN wake : can't wake up from dormant.\n");
            retval = CNL_ERR_HW_PROT;
            goto COMPLETE;
        }
    } while(ok < 2);


    // 5.
    dmtexit = DMTEXIT_OFF;
    retval  = IZAN_writeRegister(pDev, REG_DMTEXIT, 1, &dmtexit);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : set DMTEXIT(0x%x) failed[%d].\n", dmtexit, retval);
        goto COMPLETE;
    }
    pDeviceData->pmuState = PMU_AWAKE;
    intst  = CMN_H2LE32(INT_SLEEPTOAWAKE|INT_AWAKETOSLEEP);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : clear SLEEPTOAWAKE|AWAKETOSLEEP int failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 6.
    intmask = CMN_H2LE32(IZAN_INTEN_TO_MASK(pDeviceData->currIntEnable | INT_AWAKEPRIOD));
    retval = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : write INTMASK failed[%d].\n", retval);
        goto COMPLETE;
    }
    
    DBG_INFO("IZAN wake : INTMASK modified [Enable(0x%08x)] -> [Enable(0x%08x)].\n",
             pDeviceData->currIntEnable, (pDeviceData->currIntEnable | INT_AWAKEPRIOD));

    pDeviceData->currIntEnable |= INT_AWAKEPRIOD;

    // 7.
    retval = IZAN_enableCnlInt(pCnlDev);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN wake : enable CNL interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

COMPLETE :
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;

}


/*-------------------------------------------------------------------
 * Function : IZAN_sleep
 *-----------------------------------------------------------------*/
/**
 * sleep IZAN device.(awake -> powersave)
 * @param  pCnlDev : the pointer to the CNL device
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sleep(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u8                  dmtoff;
    u32                 intst;
    S_CNL_DEVICE_EVENT  event;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);


    //
    // dummy read from CNL register.
    //
    IZAN_readRegister(pDev, REG_INT, 4, &intst);

    // 1.
    dmtoff = 0;
    retval = IZAN_writeRegister(pDev, REG_DMTOFF, 1, &dmtoff);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN sleep : set DMNTOFF(0x%x) failed[%d].\n", dmtoff, retval);
        goto COMPLETE;
    }

    //
    //
    DBG_INFO("IZAN sleep : get SRCHDMTTIM0(0x%x)\n", REG_SRCHDMTTIM0);
    retval = IZAN_readRegister(pDev, REG_SRCHDMTTIM0, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN sleep : get SRCHHDMTTIM0(0x%x) failed[%d].\n", REG_SRCHDMTTIM0, retval);
        goto COMPLETE;
    }

    // change to powersave immediately,
    pDeviceData->pmuState = PMU_POWERSAVE;

    // notify dummy completion to shceduler.
    CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
    event.type    = CNL_EVENT_COMP_REQUEST;
    event.compReq = CNL_COMP_SLEEP;
    CNL_addEvent(pCnlDev, &event);

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
    
    return retval;

}


/*-------------------------------------------------------------------
 * Function : IZAN_sendMngFrame
 *-----------------------------------------------------------------*/
/**
 * read available CSDU count from HW.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  licc       : which LiCC(extended LiCC type)
 * @param  pTargetUID : target UID
 * @param  pLiccInfo  : LiCC information
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sendMngFrame(S_CNL_DEV *pCnlDev,
                  u16        licc,
                  void      *pTargetUID,
                  void      *pLiccInfo)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 uid;
    u32                 mftxreq;
    S_IZAN_MNGFRM       mngFrame;
    S_CNL_DEVICE_EVENT  event;
    int                 resend;
    u16                 delay;
    u32                 unmask;
    u32                 command;
    void               *pSetUID;
    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    //
    // send management frame sequence. 
    //
    // 1. set TargetUID if necessary.
    // 2. set ACK flag if necessary.
    // 3. setup ManagementFrame.
    // 4. fill TXMNGBODY1-8 
    // 5. set MFTXREQ(when send C-Rls, retry some times)
    // 6. enable interrupt for complete or 
    //    notify dummy completion to scheduler
    //
    // -- extra case, when LiCC is CNL_EXT_LICC_ACK_FOR_C_ACC,
    //    do not exec above sequence. only enable ACKFLAG.
    //    when LiCC is CNL_EXT_LICC_C_ACC,
    //    enable ACKFLAG before exec above sequence.
    // 

    command    = 0;
    resend     = 0;
    delay      = 0;
    unmask     = 0;
    pSetUID    = NULL;

    CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));

    //
    // pre-check LiCC
    //
    switch(licc) {
    case CNL_EXT_LICC_C_RLS :
        //
        // send C-Rls frame
        // - set retry for C-Rls TX.
        // - unmask CRLSTX
        //
        resend        = IZAN_CRLS_RESEND_RETRY;
        delay         = IZAN_CRLS_RESEND_DELAY;
        unmask        = INT_CRLSTX;
        if((CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_ACCEPT_WAITING) ||
           (CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_RESPONSE_WAITING) ||
           (CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState) == CNL_STATE_CONNECTION_REQUEST)) {
            DBG_INFO("Send C-Rls called in ACCEPT_WAITING or CONNECTION_REQUST state, need to set TargetUID.\n");
            pSetUID = (void *)pDeviceData->targetUID;
        }
        break;

    case CNL_EXT_LICC_C_REQ :
        //
        // send C-Req
        // - notify dummy completion(COMP_CONNECT_REQ)
        // - unmask CACCRCV,CRLSRCV,TCONNTOUT
        //
        event.type    = CNL_EVENT_COMP_REQUEST;
        event.compReq = CNL_COMP_CONNECT_REQ;
        unmask        = INT_TCONNTOUT;

        if(pTargetUID != NULL) {
            DBG_INFO("Send C-Req as specified UID.\n");
            CMN_MEMCPY(pDeviceData->targetUID, pTargetUID, CNL_UID_SIZE);
            pSetUID = pTargetUID;
        }
        break;

    case CNL_EXT_LICC_C_ACC :
        //
        // send C-Acc frame.
        //
        // - set ackflag enable,
        // - notify dummy completion(COMP_ACCEPT_REQ)
        // - unmask CACCTX | TACCTOUT
        // -- note --
        // additional spec, unmask CSLEEPRCV,CWAKERCV,RXBANKNOTEMPT too
        // for missing ACK for C-Acc from Initiator.
        //
        command       = CLMSTATE_RESPONDER_RESPONSE | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
        event.type    = CNL_EVENT_COMP_REQUEST;
        event.compReq = CNL_COMP_ACCEPT_REQ;
        unmask        = INT_CACCTX | INT_TACCTOUT;

        pSetUID       = pTargetUID;
        break;

    case CNL_EXT_LICC_ACK_FOR_C_ACC :
        //
        // send ACK for C-Acc frame,
        // - set ackflag enbale
        // - unmask CACCACKTX
        //
        command       = CLMSTATE_RESPONSE_WAITING | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
        pSetUID       = (void *)pTargetUID;
        event.type    = CNL_EVENT_COMP_REQUEST;
        event.compReq = CNL_COMP_ACCEPT_RES;
        break;

    case CNL_EXT_LICC_C_SLEEP :
        //
        // send C-Sleep frame,
        // unmask CSLEEPTX
        //
        unmask        = INT_CSLEEPTX;
        // set keepalive / tdc / tac
        retval = IZAN_setTimerforPowerSave(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : set Keepalive/Tdc/Tac failed[%d].\n", retval);
            return retval;
        }
        break;

    case CNL_EXT_LICC_C_WAKE :
        //
        // send C-Wake frame,
        // unmask CWAKETX
        //
        unmask        = INT_CWAKETX;
        break;

    case CNL_EXT_LICC_C_PROBE :
    default :
        DBG_ASSERT(0);
        return CNL_ERR_BADPARM;
        break;
    }


    // 1.
    if(pSetUID != NULL) {
        uid = CMN_H2LE32(*(((u32 *)pSetUID)+1));
        retval = IZAN_writeRegister(pDev, REG_TARGETUID1, 4, &uid);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : set TARGETUID[1] failed[%d].\n", retval);
            return retval;
        }
        uid = CMN_H2LE32(*((u32 *)pSetUID));
        retval = IZAN_writeRegister(pDev, REG_TARGETUID2, 4, &uid);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : set TARGETUID[2] failed[%d].\n", retval);
            return retval;
        }
    }


    //
    // exec sequence for extra case.
    //
    if(command != 0) {

        if(pDeviceData->discardCreq == TRUE) {
            command |= COMMAND_CREQ_RCVDIS;
        }
        command = CMN_H2LE32(command);
        retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : set COMMAND(0x%x) failed[%d].\n", command, retval);
            return retval;
        }
        if(licc == CNL_EXT_LICC_ACK_FOR_C_ACC) {
            // ACK for C-Acc, send MngFrame is unncessary
            goto COMPLETE;
        }
    }

    // 3. 
    IZAN_setupMngFrame(&mngFrame, pCnlDev->liccVersion, licc, pDeviceData->ownUID, pLiccInfo);

    // 4.
    retval = IZAN_writeRegister(pDev, REG_TXMNGBODY1, 32, &mngFrame);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SendMngFrame : write TXMNGBODY1-8 failed[%d].\n", retval);
        return retval;
    }

    // 3.
    mftxreq = CMN_H2LE32(MFTXREQ_START);
    do {
        retval = IZAN_writeRegister(pDev, REG_MFTXREQ, 4, &mftxreq);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : write MFTXREQ failed[%d].\n", retval);
            return retval;
        }
        resend--;
        if(delay > 0)
            CMN_delayTask(delay);
    }while(resend >= 0);

COMPLETE :
    if(unmask != 0) {

        retval = IZAN_addIntUnmask(pCnlDev, unmask);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendMngFrame : add IntUnmask failed[%d].\n", retval);
            return retval;
        }
    }

    if(event.type != 0) {
        CNL_addEvent(pCnlDev, &event);
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_sendData
 *-----------------------------------------------------------------*/
/**
 * send data frame.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  profileId  : profileId (0 or 1)
 * @param  fragment   : more fragment or not
 * @param  length     : data length to be write
 * @param  pData      : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sendData(S_CNL_DEV *pCnlDev,
              u8         profileId,
              u8         fragment,
              u32        length,
              void      *pData)
{

    T_CNL_ERR  retval;
    void      *pDev;
    u8         cnt;
    u32        txInfo[IZAN_TX_CSDU_NUM];

    pDev  = pCnlDev->pDev;

    DBG_ASSERT(pData  != NULL);
    DBG_ASSERT((length > 0) && (length <= IZAN_TX_CSDU_NUM * CNL_CSDU_SIZE));

    cnt = (u8)LENGTH_TO_CSDU(length);

    // 
    // send data frame sequence.
    //
    // 0. setup TX info data.
    // 1. write TXDATAINFO.
    // 2. write data to TXFIFO.(writeDMA)
    // 3. if write data is not 4096 * n, put TxFIFO address to head.
    //

    // 0.
    IZAN_setupTxInfo(txInfo, length, profileId, fragment);

    // 1.
    retval = IZAN_writeRegister(pDev, REG_TXDATAINFO, 4 * cnt, (void *)txInfo);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SendData : set TXINFO[1-%d] failed[%d].\n", cnt, retval);
        return retval;
    }

    // 2.
    retval = IZAN_writeDMA(pDev, REG_TXRXFIFO, PADDING_4B(length), pData);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SendData : write DATA to TXFIFO failed[%d].\n", retval);
        return retval;
    }

    // 3. 
    if(!IS_MULTI_OF_4K(PADDING_4B(length))) {
        // last data frame is not 4K byte, reset TxFIFO internal address.
        retval = IZAN_resetTxFifo(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("SendData : resetTxFifo failed[%d].\n", retval);
            return retval;
        }
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_sendDataIntUnmask
 *-----------------------------------------------------------------*/
/**
 * enable send interrupt.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_sendDataIntUnmask(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR  retval;
    u32        unmask;


    // enable interrupt.
    unmask = INT_TXBANKEMPT | INT_TXDFRAME;
    retval = IZAN_addIntUnmask(pCnlDev, unmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("SendDataIntUnmask : add IntUnmask failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_receiveData.
 *-----------------------------------------------------------------*/
/**
 * receive data frame.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  profileId  : profileId (0 or 1) (not effect)
 * @param  pFragment  : more fragment or not(OUT)
 * @param  pLength    : pointer to the data buffer length.(IN/OUT)
 * @param  pData      : pointer to the data buffer.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_receiveData(S_CNL_DEV *pCnlDev,
                 u8         profileId,
                 u8        *pFragment,
                 u32       *pLength,
                 void      *pData)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 length, remain;
    u32                 readdone, rewind;
    u8                  frag;
    u8                  reset;
    int                 retry;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    //
    // receive data frame seuqence. 
    //
    // 1. read data from RXFIFO.(if failed, do REWIND)
    // 2. if read data is completed, set READDONE
    //
    
    remain = pDeviceData->rxRemain;
    reset  = pDeviceData->rxNeedReset;

    DBG_ASSERT(pData != NULL);
    DBG_ASSERT(*pLength > 0);
    DBG_ASSERT(remain >= *pLength);


    //
    // compare SW buffer size and remained data length.
    //
    if(*pLength < remain) {
        // SW buffer is smaller than remained data. force FRAGMENTED.
        length  = *pLength;
        remain -= length;
        frag    = CNL_FRAGMENTED_DATA;
    } else {
        // SW buffer is enough to read remained data.
        length  = remain;
        remain  = 0;
        frag    = pDeviceData->rxFragment;
    }


    // 1. read data from RXFIFO.
    retry = 0;
    while(retry <= IZAN_REWIND_RETRY) {
        retval = IZAN_readDMA(pDev, REG_TXRXFIFO, PADDING_4B(length), pData);
        if(retval == SUCCESS) {
            break;
        }

        //
        // read bank data failed, do REWIND.
        //
        DBG_WARN("read DMA failed, write REG_REWIND to reset Bank.\n");

        rewind  = CMN_H2LE32(REWIND_ON);
        retval = IZAN_writeRegister(pDev, REG_REWIND, 4, &rewind);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReceiveData : write REWIND failed[%d].\n", retval);
            return retval;
        }
        retry++;
    }


    // 2.
    readdone = CMN_H2LE32(READDONE_ON);
    retval = IZAN_writeRegister(pDev, REG_READDONE, 4, &readdone);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReceiveData : write READDONE failed[%d].\n", retval);
        return retval;
    }

    // store return parameters.
    *pLength   = length;
    *pFragment = frag;

    // update information.
    pDeviceData->rxRemain      = remain;
    if(remain == 0) {
        pDeviceData->rxBankHeadPos = 0;
    } else {
        pDeviceData->rxBankHeadPos = (pDeviceData->rxBankHeadPos + length) % CNL_CSDU_SIZE;
    }

    //
    // if completed reading a data chunk(not fragment DataFrame)
    // and the data is not 4K aligned, reset RxFIFO.
    //
    if((remain == 0) && (reset == TRUE)) {
        // reset RXFIFO address.
        retval = IZAN_resetRxFifo(pCnlDev);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReceiveData : reset RxFIFO failed[%d].\n", retval);
            return retval;
        }
        pDeviceData->rxNeedReset = FALSE;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_receiveDataIntUnmask.
 *-----------------------------------------------------------------*/
/**
 * enable receive interrupt.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_receiveDataIntUnmask(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    u32                 unmask;

    pDev = pCnlDev->pDev;

    {
        u32                rxbanksta;
        u32                rxbankcnt;
        S_CNL_DEVICE_EVENT event;

        retval = IZAN_readRegister(pDev, REG_RXBANKSTA, 4, &rxbanksta);
        if(retval != CNL_SUCCESS) {
            DBG_ERR(" : read RXBANKSTA failed[%d].\n", retval);
            return retval;
        }
        if (IZAN_readRxvgagain(pCnlDev) != CNL_SUCCESS) {
            DBG_ERR("ReceiveDataIntUnmask : read RXVGAGAIN failed.\n");
        }

        rxbankcnt = CMN_LE2H32(rxbanksta) & 0x0000000F;

        if(rxbankcnt > 0) {
            DBG_INFO("=======> RXBANKCNT = %d \n", rxbankcnt);
            CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
            event.type |= CNL_EVENT_RX_READY;
            CNL_addEvent(pCnlDev, &event); 
        } else {

            CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
            event.type |= CNL_EVENT_RX_READY;
            CNL_clearEvent(pCnlDev, &event); 

            // enable interrupt.
            unmask = INT_RXBANKNOTEMPT;
            retval = IZAN_addIntUnmask(pCnlDev, unmask);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("ReceiveDataIntUnmask : add IntUnmask failed[%d].\n", retval);
                return retval;
            }
        }
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readReadyPid
 *-----------------------------------------------------------------*/
/**
 * check which ProfileID's data is ready.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  pProfileId : which profileId head of stored CSDU.(only effect RX_CSDU)
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readReadyPid(S_CNL_DEV *pCnlDev, 
                  u8        *pProfileId)

{


    T_CNL_ERR  retval;
    void      *pDev;
    u32        rxInfo = 0;

    pDev = pCnlDev->pDev;


    //
    // check ready CSDU count sequence.
    //
    // 1. read RXDATEINFO[1] to check PID of the head data of RX bank.
    //
    retval = IZAN_readRegister(pDev, REG_RXDATAINFO, 4, &rxInfo);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReadReadyPid : read RXDATAINFO[1] failed[%d].\n", retval);
        return retval;
    }
    
    *pProfileId = IZAN_DATAINFO_TO_PID(rxInfo);
    
    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readReadyBuffer
 *-----------------------------------------------------------------*/
/**
 * read ready buffer length.
 * TX : number of empty BANK count * CSDU_SIZE.
 * RX : number of CSDU same PID * CSDU_SIZE - position of RX head bank.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  which      : which CSDU (CNL_TX_CSDU or CNL_RX_CSDU)
 * @param  pLength    : ready buffer length.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note 
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readReadyBuffer(S_CNL_DEV *pCnlDev, 
                     u8         which,
                     u32       *pLength)


{


    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 banksta = 0;
    u32                 rxInfo[IZAN_RX_CSDU_NUM] = {0};

    u8                  cnt, i;
    u32                 remain;
    u8                  pid;
    u8                  frag;
    

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);


    //
    // check ready CSDU count sequence.
    //
    // if TX
    // 1. read TXBANKSTA
    //    -- return empty Bank count * CSDU_SIZE
    // 2. if Sending Frame is remained, UNMASK TXDFRAME interrupt again
    //
    // if RX
    // 1. read RXBANKSTA
    // 2. read RXDATEINFO to calculate continous data size.
    //

    if(which == CNL_TX_CSDU) {
        // TX 1.
        retval = IZAN_readRegister(pDev, REG_TXBANKSTA, 4, &banksta);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadReadyBuffer(TX) : read TXBANKSTA failed[%d].\n", retval);
            return retval;
        }

        *pLength = (u32)(IZAN_TXBANKSTA_TO_READY_CSDU(banksta) * CNL_CSDU_SIZE);

        // TX 2.
        if(IZAN_TXBANKSTA_TO_READY_CSDU(banksta) != IZAN_TX_CSDU_NUM) {
            retval = IZAN_addIntUnmask(pCnlDev, INT_TXDFRAME);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("ReadReadyBuffer(TX) : re-unmask TXDFRAME failed[%d].\n", retval);
                return retval;
            }
        }
    }
    else {
        //
        // RX : check ready bank count and calculate continous DataFrame length.
        //      continuous means same PID and not fragmented here.
        //
        if((pDeviceData->rxRemain != 0) &&
           (pDeviceData->rxFragment == CNL_NOT_FRAGMENTED_DATA)) {
            // already read and calculated, but not received by upper,
            // return cached value.
            *pLength = pDeviceData->rxRemain;
            return CNL_SUCCESS;
        }

        // RX 1.
        retval = IZAN_readRegister(pDev, REG_RXBANKSTA, 4, &banksta);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadReadyBuffer : read RXBANKSTA failed[%d].\n", retval);
            return retval;
        }

        // RX 2.
        cnt    = IZAN_RXBANKSTA_TO_READY_CSDU(banksta);
        retval = IZAN_readRegister(pDev, REG_RXDATAINFO, 4 * cnt, &rxInfo);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadReadyBuffer : read RXDATAINFO failed[%d].\n", retval);
            return retval;
        }

        remain = 0;
        frag   = IZAN_DATAINFO_TO_FRAG(rxInfo[0]);
        pid    = IZAN_DATAINFO_TO_PID(rxInfo[0]);
        for(i=0; i<cnt; i++) {

            if(pid != IZAN_DATAINFO_TO_PID(rxInfo[i])) {
                // different PID data. do not add length.
                break;
            }
            
            frag    = IZAN_DATAINFO_TO_FRAG(rxInfo[i]);

            remain += IZAN_DATAINFO_TO_LENGTH(rxInfo[i]);
            if(frag == CNL_NOT_FRAGMENTED_DATA) {
                // terminal data of a data chunk, exit loop
                break;
            }
        }
        
        if((!IS_MULTI_OF_4K(remain)) && (frag == CNL_NOT_FRAGMENTED_DATA)) {
            //
            // when lenght of this data chunk is not 4K aligned and not fragmented,
            // need to reset RXFIFO address when completed reading all of this data chunk.
            //
            pDeviceData->rxNeedReset = TRUE;
        } else {
            pDeviceData->rxNeedReset = FALSE;
        }
        
        // cache these RX data information.
        pDeviceData->rxRemain   = remain - pDeviceData->rxBankHeadPos;
        pDeviceData->rxFragment = frag;
        
        remain = pDeviceData->rxRemain;
        DBG_INFO("continous data left in IZAN[pid=%u, frag=%u, length=%u].\n", pid, frag, remain);
        *pLength = remain;

    }

    if (IZAN_readRxvgagain(pCnlDev) != CNL_SUCCESS) {
        DBG_ERR("ReadReadyBuffer : read RXVGAGAIN failed.\n");
    }


    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readReadyTxBuffer
 *-----------------------------------------------------------------*/
/**
 * read ready TX buffer length.
 * TX : number of empty BANK count * CSDU_SIZE.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  pLength    : ready buffer length.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note 
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readReadyTxBuffer(S_CNL_DEV *pCnlDev,
                       u32       *pLength)
{

    T_CNL_ERR           retval;
    void               *pDev;
    u32                 banksta = 0;


    pDev = pCnlDev->pDev;

    //
    // check ready CSDU count sequence.
    //
    retval = IZAN_readRegister(pDev, REG_TXBANKSTA, 4, &banksta);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReadyTxBuffer : read TXBANKSTA failed[%d].\n", retval);
        return retval;
    }

    *pLength = (u32)(IZAN_TXBANKSTA_TO_READY_CSDU(banksta) * CNL_CSDU_SIZE);

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readReadyRxBuffer
 *-----------------------------------------------------------------*/
/**
 * read ready RX buffer length.
 * RX : number of CSDU same PID * CSDU_SIZE - position of RX head bank.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  pLength    : ready buffer length.
 * @param  pid        : profile id.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note 
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readReadyRxBuffer(S_CNL_DEV *pCnlDev, 
                       u32       *pLength, 
                       u8         pid)
{
    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 rxInfo[IZAN_RX_CSDU_NUM] = {0};

    u8                  i;
    u32                 remain;
    u8                  frag;


    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    // read RXDATAINFO.
    retval = IZAN_readRegister(pDev, REG_RXDATAINFO, 4 * IZAN_RX_CSDU_NUM, &rxInfo);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReadyRxBuffer : read RXDATAINFO failed[%d].\n", retval);
        return retval;
    }

    // check profile id.
    if (pid != IZAN_DATAINFO_TO_PID(rxInfo[0])) {
        DBG_ERR("ReadyRxBuffer : [current pid=%u] != [buffer pid=%u]\n", 
            pid, IZAN_DATAINFO_TO_PID(rxInfo[0]));

        *pLength = 0;
        return CNL_SUCCESS;
    }

    remain = 0;
    pid    = IZAN_DATAINFO_TO_PID(rxInfo[0]);
    frag    = IZAN_DATAINFO_TO_FRAG(rxInfo[0]);

    for(i=0; i<IZAN_RX_CSDU_NUM; i++) {
        if(((CMN_LE2H32(rxInfo[i]) & 0x80000000) >> 31) == 1) {
            if(pid != IZAN_DATAINFO_TO_PID(rxInfo[i])) {
                // different PID data. do not add length.
                break;
            }
            frag    = IZAN_DATAINFO_TO_FRAG(rxInfo[i]);
            remain += IZAN_DATAINFO_TO_LENGTH(rxInfo[i]);
            if(frag == CNL_NOT_FRAGMENTED_DATA) {
                // terminal data of a data chunk, exit loop
                break;
            }
        } else {
            break;
        }
    }

    if((!IS_MULTI_OF_4K(remain)) && (frag == CNL_NOT_FRAGMENTED_DATA)) {
        pDeviceData->rxNeedReset = TRUE;
    } else {
        pDeviceData->rxNeedReset = FALSE;
    }

    // cache these RX data information.
    pDeviceData->rxRemain   = remain - pDeviceData->rxBankHeadPos;
    pDeviceData->rxFragment = frag;

    remain = pDeviceData->rxRemain;
    DBG_INFO("continous data left in IZAN[pid=%u, frag=%u, length=%u].\n", pid, frag, remain);
    *pLength = remain;

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_readMngBody
 *-----------------------------------------------------------------*/
/**
 * read management frame data body.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @param  pLicc      : which LiCC(extended LiCC type)
 * @param  pTargetUID : the pointer to the target UID.
 * @param  pLiccInfo  : the pointer to the liccInfo.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_readMngBody(S_CNL_DEV *pCnlDev, 
                  u16      *pLicc,
                  void     *pTargetUID,
                  void     *pLiccInfo)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 addr;
    u32                 mfreaddone;
    u8                  needDone;
    u16                 rxLicc;
    S_IZAN_MNGFRM       mngFrame;
    u8                  state;
    u32                 command;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);
    state       = CNLSTATE_TO_MAINSTATE(pCnlDev->cnlState);

    DBG_ASSERT(*pLicc != 0);

    //
    // 1. read management frame body sequence.
    //
    // 1. read management frame body 
    // C-Rls                 : RXCRLSBODY
    // C-Req, C-Acc, C-Sleep : RXMNGBODY
    // else                  : none
    // 
    switch(*pLicc) {
    case CNL_EXT_LICC_C_RLS :
        addr     = REG_RXCRLSBODY1;
        needDone = 0;
        break;

    case CNL_EXT_LICC_C_REQ :
        if(state == CNL_STATE_SEARCH) {
            command  = CLMSTATE_SEARCH | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE | COMMAND_CREQ_RCVDIS;
        } else if(state == CNL_STATE_CONNECTION_REQUEST) {
            command  = CLMSTATE_CONNECTION_REQUEST | COMMAND_ACK_DISABLE | \
                COMMAND_AUTO_CPROBE | COMMAND_CREQ_RCVDIS;
        } else {
            DBG_ASSERT(0);
            return CNL_ERR_HW_PROT;
        }

        DBG_INFO("ReadMngBody : Received C-Req, set CREQRCVDIS not to receive C-Req more over.\n");

        // discard C-Req.
        command = CMN_H2LE32(command);
        retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &command);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadMngBody : write COMMAND register to discard C-Req failed[%d]\n", retval);
            return retval;
        }
        
        pDeviceData->discardCreq = TRUE; // mark discardCreq.
        addr     = REG_RXMNGBODY1;
        needDone = 1;
        break;

    case CNL_EXT_LICC_C_ACC :
    case CNL_EXT_LICC_C_SLEEP :
        addr     = REG_RXMNGBODY1;
        needDone = 1;
        break;

    default :
        // do nothing for C_WAKE/C_PROBE
        return CNL_SUCCESS;
    }

    // 1. 
    retval = IZAN_readRegister(pDev, addr, 32, &mngFrame);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ReadMngBody : read RXMNGBODY failed[%d].\n", retval);
        return retval;
    }

    // 2. if read from RXMNGBODY, set MFDATAREADDONE.
    if(needDone) {
        mfreaddone = CMN_H2LE32(MFDATAREADDONE_ON);
        retval = IZAN_writeRegister(pDev, REG_MFDATAREADDONE, 4, &mfreaddone);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadMngBody : MFDATAREADDONE failed[%d].\n", retval);
            return retval;
        }
    }

    // store them.
    IZAN_anlysMngFrame(&mngFrame, &rxLicc, pTargetUID, pLiccInfo);
    DBG_ASSERT(rxLicc == *pLicc);

    // store TargetUID when C-Req or C-Acc received)
    switch(*pLicc) {
    case CNL_EXT_LICC_C_REQ :
        CMN_MEMCPY(pDeviceData->targetUID, pTargetUID, CNL_UID_SIZE);
        break;
    case CNL_EXT_LICC_C_ACC :
        CMN_MEMCPY(pDeviceData->targetUID, pTargetUID, CNL_UID_SIZE);
        if(state == CNL_STATE_INITIATOR_CONNECTED) {
            //
            // if state is INITIATOR_CONNECTED,
            // some C-Acc frame may be received, but at most 1 or 2 times.
            //
            retval = IZAN_addIntUnmask(pCnlDev, INT_CACCRCV);
            if(retval != CNL_SUCCESS) {
                DBG_ERR("ReadMngBody : unmask CACCRCV failed[%d].\n", retval);
                return retval;
            }
        }
        break;
    case CNL_EXT_LICC_C_SLEEP :
        retval = IZAN_addIntUnmask(pCnlDev, INT_CSLEEPRCV);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ReadMngBody : unmask CSLEEPRCV failed[%d].\n", retval);
            return retval;
        }
        break;
    default :
        break;
    }

    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_changeState
 *-----------------------------------------------------------------*/
/**
 * indicates state change to HW
 * @param  pCnlDev   : the pointer to the S_CNL_DEV
 * @param  prevState : the Current CNL state.
 * @param  newState  : the New CNL state to be changed.
 * @return CNL_SUCCESS     (normally completion)
 * @return CNL_ERR_HOST_IO (HostI/O failed)
 * @return CNL_ERR_HW_PROT (HW protocol error)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_changeState(S_CNL_DEV   *pCnlDev,
                 T_CNL_STATE  prevState,
                 T_CNL_STATE  newState)
{
    
    T_CNL_ERR           retval = CNL_SUCCESS;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    S_CNL_DEVICE_EVENT  event;

    u8                  newMain;
    u8                  newSub;
    u8                  prevMain;
    u8                  prevSub;
    u8                  needDone = FALSE;
    u8                  skipCmd  = FALSE;
    u32                 intst;
    u32                 prevInten;
    u32                 newInten;
    u32                 currInten;
    u32                 clearInt;
    u32                 intmask;
    u32                 command;
    u32                 wcommand;
    u32                 mfreaddone;
    u32                 banksta  = 0; /* for TXBANKSTA */

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    prevMain    = CNLSTATE_TO_MAINSTATE(prevState);
    prevSub     = CNLSTATE_TO_MAINSTATE(prevState);
    newMain     = CNLSTATE_TO_MAINSTATE(newState);
    newSub      = CNLSTATE_TO_SUBSTATE(newState);


    retval = IZAN_preProcCS(pCnlDev, prevState, newState);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ChangeState : pre process failed[%d].\n", retval);
        return retval;
    }

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    prevInten = IZAN_stateToIntEnable(prevState);
    newInten  = IZAN_stateToIntEnable(newState);
    currInten = pDeviceData->currIntEnable;

    switch(newMain) {
    case CNL_STATE_CLOSE :
        //
        // change * -> CLOSE
        // only call close reset.
        //
        command  = CLMSTATE_CLOSE | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        skipCmd  = TRUE;
        break;
    case CNL_STATE_SEARCH :
        //
        // change CLOSE -> SEARCH
        // or     *-> SEARCH
        //
        // if previous state is CLOSE, STMODE is already CLMSTATE_SERCH,
        // unnecessary to re-write to COMMAND register.
        //
        // else(previous state is not CLOSE), set STMODE and,
        // necessary to call releaseRest.
        //
        command  = CLMSTATE_SEARCH | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        if(prevMain == CNL_STATE_CLOSE) {
            skipCmd = TRUE;
        }
        break;

    case CNL_STATE_CONNECTION_REQUEST :
        //
        // change SEARCH -> CONNECTION_REQUEST
        //
        // unmask CREQRCV, CRLSRCV, CACCRCV interrupt.
        // other interrupt(TCONNTOUT) will be unmasked 
        // when start sending C-Req.
        //
        command  = CLMSTATE_CONNECTION_REQUEST | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        break;

    case CNL_STATE_ACCEPT_WAITING :
        //
        // change SEARCH -> ACCEPT_WAITING
        //
        // unmask CRLSRCV interuupt
        // other interrupt(CACCTX,TACCTOUT) will be unmasked
        // when start sending C-Acc
        //
        command  = CLMSTATE_ACCEPT_WAITING | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        break;

    case CNL_STATE_RESPONSE_WAITING :
        //
        // change CONNECTION_REQUEST -> RESPONSE_WAITING
        //
        // unmask no interrupt
        // other interrupt(CACCACKTX) will be unmasked
        // when start sending ACK for C-Acc
        //
        command  = CLMSTATE_RESPONSE_WAITING | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        break;

    case CNL_STATE_RESPONDER_RESPONSE :
        //
        // change CONNECTION_REQUEST -> RESPONDER_RESPONSE
        // or     ACCEPT_WAITING     -> RESPONDER_RESPONSE
        //
        // unmask no interrupt
        // other interrupt(CACCTX, TACCTOUT) will be unmasked
        // when start sending C-Acc
        //
        // !! Note.
        // C-Req is remained in MF buffer, so can't receive direct C-Sleep.
        // receiving C-Sleep will be enabled when C-Req filetr is implemented.
        //
        command  = CLMSTATE_RESPONDER_RESPONSE | COMMAND_ACK_DISABLE | COMMAND_AUTO_CPROBE;
        break;

    case CNL_STATE_INITIATOR_CONNECTED :
        if (prevMain != CNL_STATE_INITIATOR_CONNECTED) {
            if (IZAN_resetRxvgagain(pCnlDev) != CNL_SUCCESS) {
                DBG_ERR("ChangeState : IZAN_resetRxvgagain failed.\n");
            }
        }
        switch(newSub) {
        case CNL_SUBSTATE_CONNECTED :
            //
            // change RESPONSE_WAITING -> CONNECTED(ACK for C-Acc completed)
            // or     TARGET_SLEEP     -> CONNECTED(C-Wake completed, C-Wake received)
            // or     LOCAL_HIBERNATE  -> CONNECTED(C-Wake received)
            //
            // unmask CONNECTED default and
            // - CPROBETX for criteria of Sleep.
            // if change from RESPONSE_WAITING,
            // - CACCRECV      for clearing MF buffer.
            // - RXBANKNOETMPT for 
            //
            // do not mask Data related interrupt bits if arleady unmasked.
            //
            if(prevMain == CNL_STATE_RESPONSE_WAITING) {
                newInten |= INT_RXBANKNOTEMPT | INT_CACCRCV; // to clear excess C-Acc 
            } else {
                newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            }
            command  = CLMSTATE_INIT_CONNECTED | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;

        case CNL_SUBSTATE_TARGET_SLEEP :
            //
            // change CONNECTED       -> TARGET_SLEEP(T_Resend or C-Sleep recvd)
            // or     LOCAL_HIBERNATE -> TARGET_SLEEP(Order Wake)
            //
            newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            command  = CLMSTATE_INIT_TARGETSLEEP | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;

        case CNL_SUBSTATE_LOCAL_HIBERNATE :
            //
            // change CONNECTED    -> LOCAL_HIBERNATE(C-Sleep completed)
            // or     TARGET_SLEEP -> LOCAL_HIBERNATE(Order Powersave)
            //
            newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            command  = CLMSTATE_INIT_LOCALHIBERNATE | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;
        default :
            retval = CNL_ERR_BADPARM;
            goto COMPLETE;
        }
        break;

    case CNL_STATE_RESPONDER_CONNECTED :
        if (prevMain != CNL_STATE_RESPONDER_CONNECTED) {
            if (IZAN_resetRxvgagain(pCnlDev) != CNL_SUCCESS) {
                DBG_ERR("ChangeState : IZAN_resetRxvgagain failed.\n");
            }
        }
        switch(newSub) {
        case CNL_SUBSTATE_CONNECTED :
            //
            // change RESPONDER_RESPONSE -> CONNECTED(ACK recvd)
            // or     TARGET_SLEEP       -> CONNECTED(C-Wake completed)
            //
            // unmask CONNECTED default mask and CPROBETX for criteria of Sleep
            //
            // Note.
            // RXBANKNOTEMPT is already unmasked in RESPONDER_RESPONSE state.
            // 
            newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            command   = CLMSTATE_RESP_CONNECTED | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;

        case CNL_SUBSTATE_TARGET_SLEEP :
            //
            // change CONNECTED       -> TARGET_SLEEP(T_Resend or C-Wake recvd)
            // or     LOCAL_HIBERNATE -> TARGET_SLEEP(Order Wake)
            //
            newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            command  = CLMSTATE_RESP_TARGETSLEEP | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;

        case CNL_SUBSTATE_LOCAL_HIBERNATE :
            //
            // change CONNECTED    -> LOCAL_HIBERNATE(C-Sleep completed)
            // or     TARGET_SLEEP -> LOCAL_HIBERNATE(Order Powersave)
            //
            newInten |= (currInten & (IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS)); // take over DATA and MFTX bits.
            command  = CLMSTATE_RESP_LOCALHIBERNATE | COMMAND_ACK_ENABLE | COMMAND_AUTO_CPROBE;
            break;
        default :
            retval = CNL_ERR_BADPARM;
            goto COMPLETE;
        }
        break;
    default :
        retval = CNL_ERR_BADPARM;
        goto COMPLETE;
    }

    //
    // Clear Interrupt is Mask of previous state and new state, but
    // do not clear DATA BITS and MFTXBITS when state is in CONNECTED,
    //
    clearInt  = IZAN_INTEN_TO_MASK(newInten) | IZAN_INTEN_TO_MASK(prevInten);
    if((newMain == CNL_STATE_INITIATOR_CONNECTED) ||
       (newMain == CNL_STATE_RESPONDER_CONNECTED)) {
        // take over DATA and MFTX bits.
        clearInt &= ~(IZAN_INTEN_DATA_BITS | IZAN_INTEN_MFTX_BITS);
    }
    //
    // When changing a state from SERACH to CONNECTION_REQUEST, 
    // the status of C-ACC Receive INT(INT_CACCRCV) is not cleared. 
    //
    if(newMain == CNL_STATE_CONNECTION_REQUEST){
        clearInt &= ~(INT_CACCRCV);
    }

    // do not clear C-RLS TX Interrupt in all state
    clearInt &= ~INT_CRLSTX;

    // take over C-RLS TX mask bit in all state
    newInten |= (currInten & INT_CRLSTX);

    DBG_INFO("ChangeState : Masks[Prev:0x%08x, New:0x%08x, Curr:0x%08x, Clear:0x%08x].\n",
             prevInten, newInten, currInten, clearInt);

    CMN_MEMSET(&event, 0x00, sizeof(S_CNL_DEVICE_EVENT));
    IZAN_intToEvent(&event, clearInt);

    // 1.
    intmask = CMN_H2LE32(IZAN_INTEN_TO_MASK(IZAN_INTEN_NONE));
    retval  = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ChangeState : write INTMASK[0x%08x] failed[%d].\n",
                IZAN_INTEN_TO_MASK(IZAN_INTEN_NONE), retval);
        goto COMPLETE;
    }
    pDeviceData->currIntEnable = IZAN_INTEN_NONE;


    if(skipCmd == FALSE) {
        // 2.
        if((newMain != CNL_STATE_SEARCH) && (pDeviceData->discardCreq == TRUE)) {
            command |= COMMAND_CREQ_RCVDIS;
        }

        wcommand = CMN_H2LE32(command);
        retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &wcommand);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ChangeState : write COMMAND register failed[%d]\n", retval);
            goto COMPLETE;
        }

        // 3. read back
        retval = IZAN_pollU32Register(pDev,
                                      REG_COMMAND,
                                      CMN_H2LE32(command & ~COMMAND_RETXDATAREQ),
                                      CMN_H2LE32(~(CLMSTATE_DORMANT | CLMSTATE_PLL_ON | COMMAND_RETXDATAREQ)),
                                      0,
                                      IZAN_POLL_RETRY);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ChangeState : readback COMMAND register failed[%d]\n", retval);
            goto COMPLETE;
        }

        if (((newSub == CNL_SUBSTATE_CONNECTED) && (pDeviceData->txNeedResend)) ||
            ((newSub == CNL_SUBSTATE_CONNECTED) && (CNLSTATE_TO_SUBSTATE(prevState) == CNL_SUBSTATE_TARGET_SLEEP))) {
            /* check TXBANKCNT */
            retval = IZAN_readRegister(pDev, REG_TXBANKSTA, 4, &banksta);
            if (retval != CNL_SUCCESS) {
                DBG_ERR("ChangeState : read TXBANKSTA failed[%d].\n", retval);
                goto COMPLETE;
            }
            if ((banksta & 0x000F) != 0) {  /* TXBANKCNT != 0 */
                wcommand = CMN_H2LE32(command | COMMAND_RETXDATAREQ);
                retval = IZAN_writeRegister(pDev, REG_COMMAND, 4, &wcommand);
                if(retval != CNL_SUCCESS) {
                    DBG_ERR("ChangeState : write COMMAND[RETXDATAREQ] register failed[%d]\n", retval);
                    goto COMPLETE;
                }
            }
            pDeviceData->txNeedResend = FALSE;
        }
    }

    if(clearInt & (INT_CREQRCV | INT_CACCRCV | INT_CSLEEPRCV)) {
        DBG_INFO("ChangeState : MFREADDONE is necessary(ClearedInterrupt)[clearInt(0x%x)]\n",
                 clearInt);
        needDone = TRUE;
    }

    // 4.
    intst = CMN_H2LE32(clearInt);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ChangeState : clear interrupt failed[%d].\n", retval);
        goto COMPLETE;
    }

    // clear event related interrupt.
    CNL_clearEvent(pCnlDev, &event);

    if(event.recvdLicc & (CNL_EXT_LICC_C_REQ | CNL_EXT_LICC_C_ACC | CNL_EXT_LICC_C_SLEEP)) {
        DBG_INFO("ChangeState : MFREADDONE is necessary(ClearedEvent)[type:0x%x,timer:0x%x,comp:0x%x,licc[0x%x].\n",
                 event.type, event.timer, event.compReq, event.recvdLicc);
        needDone = TRUE;
    }

    // exec MFREADDONE.
    if(needDone == TRUE) {
        mfreaddone = MFDATAREADDONE_ON;
        retval = IZAN_writeRegister(pDev, REG_MFDATAREADDONE, 4, &mfreaddone);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("ChangeState : MFDATAREADDONE failed[%d].\n", retval);
            goto COMPLETE;
        }
    }

    // 5.
    intmask = CMN_H2LE32(IZAN_INTEN_TO_MASK(newInten));
    retval  = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ChangeState : write INTMASK[0x%08x] failed[%d].\n", IZAN_INTEN_TO_MASK(newInten), retval);
        goto COMPLETE;
    }
    pDeviceData->currIntEnable = newInten;

    DBG_INFO("ChangeState : INTMASK modified[Enable(0x%x)] -> [Enable(0x%x)].\n",
             prevInten, newInten);

COMPLETE :
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);
    if(retval != CNL_SUCCESS) {
        return retval;
    }

    //
    //  special case post process.
    //
    retval = IZAN_postProcCS(pCnlDev, prevState, newState);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("ChnageState : post process failed[%d].\n", retval);
        return retval;
    }

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_waitConnect
 *-----------------------------------------------------------------*/
/**
 * wait connect.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_waitConnect(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 intst;
    u32                 intmask;
    u32                 mfreaddone;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    //
    // 1. clear all interrupt
    // 2. clear MF buffer (do MFREADDONE).
    // 3. unmask CREQRCV interrupt.
    //
    intst = CMN_H2LE32(IZAN_INTEN_ALL);
    retval = IZAN_writeRegister(pDev, REG_INT, 4, &intst);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("WaitConnect : write REG_INT[0x%08x] failed[%d].\n", intst, retval);
        goto COMPLETE;
    }

    mfreaddone = CMN_H2LE32(MFDATAREADDONE_ON);
    retval = IZAN_writeRegister(pDev, REG_MFDATAREADDONE, 4, &mfreaddone);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("WaitConnect : MFDATAREADDONE failed[%d].\n", retval);
        goto COMPLETE;
    }

    // 2.
    intmask = CMN_H2LE32(IZAN_INTEN_TO_MASK(pDeviceData->currIntEnable | INT_CREQRCV));
    retval = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("WaitConnect : write INTMASK failed[%d].\n", retval);
        goto COMPLETE;
    }

    DBG_INFO("WaitConnect : INTMASK modified [Enable(0x%08x)] -> [Enable(0x%08x)].\n",
             pDeviceData->currIntEnable, (pDeviceData->currIntEnable | INT_CREQRCV));

    pDeviceData->currIntEnable |= INT_CREQRCV;

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;

}

/*-------------------------------------------------------------------
 * Function : IZAN_cancelWaitConnect
 *-----------------------------------------------------------------*/
/**
 * cancelWait connect.
 * @param  pCnlDev    : the pointer to the S_CNL_DEV
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR
IZAN_cancelWaitConnect(S_CNL_DEV *pCnlDev)
{

    T_CNL_ERR           retval;
    void               *pDev;
    S_IZAN_DEVICE_DATA *pDeviceData;
    u32                 intmask;

    pDev        = pCnlDev->pDev;
    pDeviceData = IZAN_cnlDevToDeviceData(pCnlDev);

    CMN_LOCK_MUTEX(pDeviceData->intLockId);

    //
    // 1. mask CREQRCV interrup.
    //
    // 1.
    intmask = CMN_H2LE32(IZAN_INTEN_TO_MASK(pDeviceData->currIntEnable & ~INT_CREQRCV));
    retval = IZAN_writeRegister(pDev, REG_INTMASK, 4, &intmask);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("WaitConnect : write INTMASK failed[%d].\n", retval);
        goto COMPLETE;
    }

    DBG_INFO("CancelWaitConnect : INTMASK modified [Enable(0x%08x)] -> [Enable(0x%08x)].\n",
             pDeviceData->currIntEnable, (pDeviceData->currIntEnable & ~INT_CREQRCV));

    pDeviceData->currIntEnable &= ~INT_CREQRCV;

COMPLETE:
    CMN_UNLOCK_MUTEX(pDeviceData->intLockId);

    return retval;

}

/*-------------------------------------------------------------------
 * Function : IZAN_getStats
 *-----------------------------------------------------------------*/
/**
 * get current statistics value.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pStats  : the poniter to store RSSI value.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_getStats(S_CNL_DEV   *pCnlDev,
              S_CNL_STATS *pStats)
{

    T_CNL_ERR         retval;
    void             *pDev;
    u32               rxvgagain = 0;

    pDev  = pCnlDev->pDev;

    //
    // 1. read RXVGAGAIN for RSSI.
    // add more if necessary
    //

    // 1.
    retval = IZAN_readRegister(pDev, REG_RXVGAGAIN, 4, &rxvgagain);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("GetStats : read REG_RXVGAGAIN failed[%d].\n", retval);
        return retval;
    }

    // convert RXVGAGAIN value to RSSI.
    pStats->RSSI = IZAN_RXVGAGAIN_TO_RSSI(CMN_LE2H32(rxvgagain));

    return CNL_SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : IZAN_regPassthrough
 *-----------------------------------------------------------------*/
/**
 * get current reg value and set value to reg.
 * @param  pCnlDev : the pointer to the S_CNL_DEV
 * @param  pregPT  : the poniter to the S_CNL_REG_PASSTHROUGH
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_BADPARM (invalid parameter)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_regPassthrough(S_CNL_DEV             *pCnlDev,
                    S_CNL_REG_PASSTHROUGH *pregPT)
{

    T_CNL_ERR         retval;
    void             *pDev;
    u32               regAddr = 0;
    u32               length  = 0;
    u8                direction;

    pDev  = pCnlDev->pDev;

    DBG_ASSERT(pregPT  != NULL);

    if(pregPT->pData == NULL){
        return CNL_ERR_BADPARM;
    }
    regAddr   = pregPT->regAddr;
    length    = pregPT->length;
    direction = pregPT->dir;

    switch(direction) {
    case CNL_REG_PASSTHROUGH_READ :
        // read 
        retval = IZAN_readRegister(pDev, regAddr, length, pregPT->pData);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("regPassthrough : read register failed[%d].\n", retval);
            return retval;
        }
        break;
    case CNL_REG_PASSTHROUGH_WRITE :
        // write
        retval = IZAN_writeRegister(pDev, regAddr, length, pregPT->pData);
        if(retval != CNL_SUCCESS) {
            DBG_ERR("regPassthrough : write register failed[%d].\n", retval);
            return retval;
        }
        break;
    default :
        DBG_ERR("regPassthrough : direction parameter failed.\n");
        return CNL_ERR_BADPARM;
    }
    return CNL_SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : IZAN_setTimerforPowerSave
 *-----------------------------------------------------------------*/
/**
 * set timer (keepalive/tdc/tac) for POWERSAVE.request
 * @param  pCnlDev : the pointer to the CNL device.
 * @return CNL_SUCCESS      (normally completion)
 * @return CNL_ERR_HOST_IO  (HostI/O failed)
 * @return CNL_ERR_HW_PROT  (HW protocol error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static T_CNL_ERR 
IZAN_setTimerforPowerSave(S_CNL_DEV *pCnlDev)
{
    T_CNL_ERR           retval = CNL_SUCCESS;
    void               *pDev;
    S_CNL_DEVICE_PARAM *pDevParam;
    u32                 tmptimer;
    u32                 cnlTimer;
    u8                  pmuTimer;

    pDev  = pCnlDev->pDev;
    pDevParam = (S_CNL_DEVICE_PARAM *)&(pCnlDev->deviceParam);

    /* set TKEEPALIVE register (unit:ms) */
    tmptimer = pDevParam->tKeepAlive;
    cnlTimer = IZAN_PMUBLK_TIM_TO_CLK(tmptimer);
    if(cnlTimer > IZAN_MAX_TKEEPALIVE) {
        DBG_ERR("IZAN_setTimerforPowerSave : tKeepAlive parameter error [%d].\n", cnlTimer);
        return CNL_ERR_BADPARM;
    }
    retval = IZAN_writeRegister(pDev, REG_TKEEPALIVE, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setTimerforPowerSave : set T_KEEPALIVE(0x%x) failed[%d].\n", cnlTimer, retval);
        return retval;
    }

    // awake period
    tmptimer = pDevParam->tac;
    cnlTimer = IZAN_CNLBLK_TIM_TO_CLK(tmptimer);
    retval   = IZAN_writeRegister(pDev, REG_TAC, 4, &cnlTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setTimerforPowerSave : set TAC timer failed[%d].\n", retval);
        return retval;
    }

    // dormant period
    tmptimer = pDevParam->tdc;
    if(IZAN_PMUBLK_TIM_TO_CLK(tmptimer) > IZAN_MAX_HBNTDMTTIM) {
        DBG_ERR("IZAN_setTimerforPowerSave : tdc parameter error [%d].\n", tmptimer);
        return CNL_ERR_BADPARM;
    }
    pmuTimer = (u8)(IZAN_PMUBLK_TIM_TO_CLK(tmptimer) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_HBNTDMTTIM0, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setTimerforPowerSave : set HBNTDMTTIM(low) timer failed[%d].\n", retval);
        return retval;
    }
    pmuTimer = (u8)((IZAN_PMUBLK_TIM_TO_CLK(tmptimer) >> 8) & 0xFF);
    retval   = IZAN_writeRegister(pDev, REG_HBNTDMTTIM1, 1, &pmuTimer);
    if(retval != CNL_SUCCESS) {
        DBG_ERR("IZAN_setTimerforPowerSave : set HBNTDMTTIM(high) timer failed[%d].\n", retval);
        return retval;
    }

    return retval;
}
