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
 *  @file     cnl_izan.h
 *
 *  @brief    IZAN depended header file.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNL_IZAN_H__)
#define __CNL_IZAN_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"
#include "cnl_type.h"
#include "izan_cnf.h"

#include "oscmn.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * Sampleup flags.
 */
#define IZAN_E2PROM_CLK        5    // MHz      
#define IZAN_E2PROM_START_ADDR 0    // 0-511    
#define IZAN_E2PROM_LOAD_SIZE  3    // 3  Byte  
#define IZAN_E2PROM_LOAD_NUM   24   // 24 times 

/**
 * internal delay definitions
 */
#define IZAN_PMUCHG_AWK_DELAY  5 // 5ms to awake.
#define IZAN_PMUCHG_SLP_DELAY  1 // 1ms to sleep.
#define IZAN_EP2ROM_READ_DELAY 1 // ?ms to awake.
#define IZAN_STOP_TXMF_DELAY   1 // 1ms to stop TX MF.
#define IZAN_STOP_TXDATA_DELAY 1 // 1ms to stop TX DataFrame.
#define IZAN_EXIT_DMT_DELAY    1 // 1ms to dormant exit delay
#define IZAN_S2C_DELAY         5 // 5ms to Close -> Search
#define IZAN_C2S_DELAY         5 // 5ms to Search -> Close
#define IZAN_E2PROM_LOAD_DELAY 10 // 10ms to complete loading E2PROM

#define IZAN_RW_RETRY          2 // retry count for register access(CRC error).
#define IZAN_REWIND_RETRY      2 // retry count for rewind.
#define IZAN_POLL_RETRY        5 // retry count for status polling.
#define IZAN_EXIT_DMT_POLL     5 // retry count for polling dormant exit.
#define IZAN_EXIT_CRMSR_POLL  30 // retry count for polling CRMSR exit.
#define IZAN_CRLS_RESEND_DELAY 1 // ?ms
#define IZAN_CRLS_RESEND_RETRY 3 // ?ms

/**
 * @brief IZAN PMU state.
 */
typedef enum tagE_PMU_STATE {
    PMU_DEEP_SLEEP = 0x01,
    PMU_SLEEP,
    PMU_AWAKE,
    PMU_POWERSAVE,
} E_PMU_STATE;
typedef u8 T_IZAN_PMU_STATE;


//
// IZAN interrupt mask definition
//
#define IZAN_INTEN_ALL                   (0xFFFFFFFF)
#define IZAN_INTEN_NONE                  (0x00000000)
#define IZAN_INTEN_SEARCH                (0x00000000)
#define IZAN_INTEN_CONNECTION_REQUEST    (INT_CREQRCV | INT_CACCRCV | INT_CRLSRCV)
#define IZAN_INTEN_RESPONSE_WAITING      (INT_CRLSRCV | INT_CSLEEPRCV)
#define IZAN_INTEN_ACCEPT_WAITING        (INT_CRLSRCV)
#define IZAN_INTEN_RESPONDER_RESPONSE    (INT_CRLSRCV | INT_CSLEEPRCV | INT_CWAKERCV | INT_RXBANKNOTEMPT)

#define IZAN_INTEN_CONNECTED             (INT_CSLEEPRCV | INT_CWAKERCV | INT_CRLSRCV | \
                                          INT_TRESENDTOUT | INT_TRETRYTOUT | \
                                          INT_IRRDFSN | INT_LCFATALERR | INT_FCFATALERR)
#define IZAN_INTEN_DATA_BITS             (INT_RXBANKNOTEMPT | INT_TXDFRAME | INT_TXBANKFULL | \
                                          INT_TXBANKONEEMPT | INT_TXBANKEMPT)

#define IZAN_INTEN_MFTX_BITS             (INT_CSLEEPTX | INT_CWAKETX | INT_CACCTX | \
                                          INT_CRLSTX | INT_CACCACKTX)
                                          


// IZAN default register value for registers
#define IZAN_DEFAULT_CONFIG_REG_VALUE    (CONFIG_TUID_MANUAL_CLEAR | \
                                          CONFIG_ACK_BUFFER_FULL |   \
                                          CONFIG_NO_FILTER_TUID)

#define IZAN_SPICTL_DEFAULT              (SPICTL_FSDOFLT_CMD | SPICTL_CSMODE_AUTO | SPICTL_RESERVED)
#define IZAN_SPIINTST_DEFAULT            (SPIINTST_TXFUL | SPIINTST_STX)
#define IZAN_SPITXDATA_READCMD_DEFAULT   (SPITXDATA_CONT_TRANS | SPITXDATA_BYTECNT_2BYTE | \
                                          SPITXDATA_TXREADCMD)
#define IZAN_SPITXDATA_READ_DEFAULT      (SPITXDATA_CMD_READ)
//
// register value utilities.
//
#define IZAN_INTEN_TO_MASK(inten)              (~(inten))
#define IZAN_RXBANKSTA_TO_READY_CSDU(banksta)  ((u8)(CMN_LE2H32(banksta) & 0x0F))
#define IZAN_TXBANKSTA_TO_READY_CSDU(banksta)  (IZAN_TX_CSDU_NUM - (u8)(CMN_LE2H32(banksta) & 0x0F))

#define IZAN_DATAINFO_TO_RATE(info)      ((u8)((CMN_LE2H32(info) & 0x0F000000) >> 24))
#define IZAN_DATAINFO_TO_PID(info)       ((u8)((CMN_LE2H32(info) & 0x00800000) >> 23))
#define IZAN_DATAINFO_TO_FRAG(info)      ((u8)((CMN_LE2H32(info) & 0x00400000) >> 22))
#define IZAN_DATAINFO_TO_LENGTH(info)    ((u16)(CMN_LE2H32(info) & 0xFFFF))

#define IZAN_MAKE_DATAINFO(length, pid, frag, rate)                     \
    CMN_H2LE32(((u16)length) |                                          \
               ((pid & 0x01) << 23) |                                   \
               ((frag & 0x01) << 22) |                                  \
               ((rate & 0x0F) << 24))

//
// calculate timer registers value from absolute time value
//
#define IZAN_CNLBLK_TIM_TO_CLK(usec)     ((usec) * 70)
#define IZAN_PMUBLK_TIM_TO_CLK(usec)     ((usec) / 100)

#define IZAN_TA_TIM_TO_POLL(usec)        (((usec) / 1000) + 1)


//
// calculate BASE value of SPI_TIMINGCONTROL register.
// prescaler assumed to be set 0.
//
#define IZAN_CALC_SPITIMBASE(clk)        (((70 % clk) == 0) ?      \
                                          (((70 / clk) + 2) >> 1) : \
                                          (((70 / clk) + 1) >> 1))

//
// calculate ReadAddr value of SPI_TXDATA register.
//
#define IZAN_CALC_E2PROM_READADDR(start) ((0xFF & start) | \
                                          ((0x100 & start) << 3))

//
// return lower 8-bit value.
//
#define IZAN_RXVGAGAIN_TO_RSSI(gain)     (0xFF & (gain))

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
/**
 * @brief IZAN dependent structure.
 */
typedef struct tagS_IZAN_DEVICE_DATA {
    // interrupt data
    u8                  intLockId;
    u32                 currIntMask;
    u32                 currIntEnable;

    // pmu state
    T_IZAN_PMU_STATE    pmuState;
    u8                  dummy[3];

    // should be aligned
    u8                  ownUID[CNL_UID_SIZE];
    u8                  targetUID[CNL_UID_SIZE];

    u8                  needCal;

    // 
    u32                 rxRemain;
    u8                  rxFragment;
    u8                  rxNeedReset;
    u32                 rxBankHeadPos;

    //
    u8                  txNeedResend;
    u8                  discardCreq;
    S_CNL_DEVICE_EVENT  eventFilter;

}S_IZAN_DEVICE_DATA;


#pragma pack(1)
/**
 * @brief IZAN management frame body structure
 */
typedef struct tagS_IZAN_MNGFRM {
    u8  liccVersion;
    u8  licc;
    u16 reserved;
    u8  ownUID[CNL_UID_SIZE];
    u8  liccInfo[CNL_PCL_PARAM_SIZE];
}S_IZAN_MNGFRM;

#pragma pack()


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
#endif /* __CNL_IZAN_H__ */
