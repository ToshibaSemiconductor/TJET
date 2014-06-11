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
 *  @file     cnl_type.h
 *
 *  @brief    CNL dependent types.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNL_TYPE_H__)
#define __CNL_TYPE_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
//
// sizes defined by CNL specification.
//
#define CNL_UID_SIZE                       8
#define CNL_LICC_INFO_SIZE                20
#define CNL_PCL_PARAM_SIZE                20
#define CNL_CSDU_SIZE                   4096


//
// connect request dependent value
//
#define CNL_SPECIFIED_UID                  0
#define CNL_UNSPECIFIED_UID                1


//
// release indication dependent value
//
#define CNL_RELEASE_ORIGIN_CNL             1
#define CNL_RELEASE_ORIGIN_PCL             2
#define CNL_RELEASE_CAUSE_CONNECT_TIMEOUT  1
#define CNL_RELEASE_CAUSE_ACCEPT_TIMEOUT   2
#define CNL_RELEASE_CAUSE_RETRY_TIMEOUT    3


//
// data request(send/receive) dependent value
//
#define CNL_PROFILE_ID_0                   0
#define CNL_PROFILE_ID_1                   1
#define CNL_NOT_FRAGMENTED_DATA            0
#define CNL_FRAGMENTED_DATA	               1


/**
 * @brief CNL state.(see CNL spec 6.5)
 */
typedef enum tagE_CNL_MAIN_STATE {
    CNL_STATE_NULL                = 0x00,
    CNL_STATE_CLOSE               = 0x01,
    CNL_STATE_SEARCH              = 0x02,
    CNL_STATE_CONNECTION_REQUEST  = 0x04,
    CNL_STATE_ACCEPT_WAITING      = 0x08,
    CNL_STATE_RESPONSE_WAITING    = 0x10,
    CNL_STATE_RESPONDER_RESPONSE  = 0x20,
    CNL_STATE_INITIATOR_CONNECTED = 0x40,
    CNL_STATE_RESPONDER_CONNECTED = 0x80,
}E_CNL_MAIN_STATE;


/**
 * @brief CNL connected substate.(see CNL spec 6.5)
 */
typedef enum tagE_CNL_SUBSTATE {
    CNL_SUBSTATE_NULL            = 0x00,
    CNL_SUBSTATE_CONNECTED       = 0x01,
    CNL_SUBSTATE_TARGET_SLEEP    = 0x02,
    CNL_SUBSTATE_LOCAL_HIBERNATE = 0x04,
}E_CNL_SUBSTATE;
typedef u16 T_CNL_STATE; // 8-15bit : main state, 0-8bit : substate.

#define CNLSTATE_TO_MAINSTATE(state)   ((state >> 8) & 0xFF)
#define CNLSTATE_TO_SUBSTATE(state)    (state & 0xFF)

/**
 * @brief Power save parameters valid range value. (see CNL spec 7.2.5)
 */
#define CNL_POWERSAVE_TKEEPALIVE_RANGE_MIN     1    // minimum KeepAlive (unit 1ms:H/W dependency)
#define CNL_POWERSAVE_TKEEPALIVE_RANGE_MAX  1000    // maximum KeepAlive 
#define CNL_POWERSAVE_TDC_RANGE_MIN            1    // minimum Dormant period (unit 5ms)
#define CNL_POWERSAVE_TDC_RANGE_MAX          200    // maximum Dormant period
#define CNL_POWERSAVE_TAC_RANGE_MIN            0    // minimum Awake period (unit 100us)
#define CNL_POWERSAVE_TAC_RANGE_MAX          255    // maximum Awake period

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
//
// CNL request/response parameter structures.
//
/**
 * @brief CNL SAP Init.request parameter structure.
 */
typedef struct tagS_CNL_INIT_REQ {
    u8  liccVersion;
    u8  mux;
    u16 reserved;
    u8  ownUID[CNL_UID_SIZE]; // return parameter.
}S_CNL_INIT_REQ;


/**
 * @brief CNL SAP Init.request parameter structure.
 */
typedef struct tagS_CNL_CLOSE_REQ {
}S_CNL_CLOSE_REQ;


/**
 * @brief CNL SAP Connect.request parameter structure.
 */
typedef struct tagS_CNL_CONNECT_REQ {
    u8 targetUID[CNL_UID_SIZE];
    u8 pclParam[CNL_PCL_PARAM_SIZE];
    u8 specified;
}S_CNL_CONNECT_REQ;


/**
 * @brief CNL SAP waitConnect.request parameter structure.
 */
typedef struct tagS_CNL_WAIT_CONNECT_REQ {
}S_CNL_WAIT_CONNECT_REQ;


/**
 * @brief CNL SAP Accept.request parameter structure.
 */
typedef struct tagS_CNL_ACCEPT_REQ {
    u8 targetUID[CNL_UID_SIZE];
    u8 pclParam[CNL_PCL_PARAM_SIZE];
}S_CNL_ACCEPT_REQ;


/**
 * @brief CNL SAP Accept.response parameter structure.
 */
typedef struct tagS_CNL_ACCEPT_RES {
    u8 targetUID[CNL_UID_SIZE];
}S_CNL_ACCEPT_RES;


/**
 * @brief CNL SAP Release.request parameter structure.
 */
typedef struct tagS_CNL_RELEASE_REQ {
    u8 pclParam[CNL_PCL_PARAM_SIZE];
}S_CNL_RELEASE_REQ;


/**
 * @brief CNL SAP Powersave.request parameter structure.
 */
typedef struct tagS_CNL_POWERSAVE_REQ{
    u8    dormantPeriod;
    u8    awakePeriod;
    u16   keepAlive;
}S_CNL_POWERSAVE_REQ;


/**
 * @brief CNL SAP Data.request parameter structure.
 */
typedef struct tagS_CNL_DATA_REQ {
    u32   length;
    u8    profileId;
    u8    fragmented;
    u16   reserved;
    void *pData;
}S_CNL_DATA_REQ;


//
// CNL indication/confirmation parameter structures.
//
/**
 * @brief CNL SAP Connect.indication parameter structure.
 */
typedef struct tagS_CNL_CONNECT_IND{
    u8 targetUID[CNL_UID_SIZE];
    u8 pclParam[CNL_PCL_PARAM_SIZE];
}S_CNL_CONNECT_IND;


/**
 * @brief CNL Accept.indication parameter structures.
 */
typedef struct tagS_CNL_ACCEPT_IND{
    u8 targetUID[CNL_UID_SIZE];
    u8 pclParam[CNL_PCL_PARAM_SIZE];
}S_CNL_ACCEPT_IND;


/**
 * @brief CNL SAP Accept.confirmation parameter structures.
 */
typedef struct tagS_CNL_ACCEPT_CNF{
    u8 targetUID[CNL_UID_SIZE];
}S_CNL_ACCEPT_CNF;


/**
 * @brief CNL SAP Release.indication parameter structures.
 */
typedef struct tagS_CNL_RELEASE_IND{
    u8  causeOrigin;
    u8  causeCode;
    u16 reserved;
    u8  pclParam[CNL_PCL_PARAM_SIZE];
}S_CNL_RELEASE_IND;


/**
 * @brief data received indication(only profileId).
 */
typedef struct tagS_CNL_DATA_IND{
    u8 profileId;
} S_CNL_DATA_IND;


/**
 * @brief CNL GET_STATS parameter structure.
 */
typedef struct tagS_CNL_STATS {
    u8 RSSI;
}S_CNL_STATS;


/**
 * @brief CNL SAP Powersave.indication parameter structures.
 */
typedef struct tagS_CNL_POWERSAVE_IND{
    u8  dormantPeriod;
    u8  awakePeriod;
}S_CNL_POWERSAVE_IND;


/**
 * @brief CNL REG_PASSTHROUGH parameter structure.
 */

#define CNL_REG_PASSTHROUGH_READ        0
#define CNL_REG_PASSTHROUGH_WRITE       1
#define CNLIO_RSV_BYTE_3                3

typedef struct tagS_CNL_REG_PASSTHROUGH {
    u32   regAddr;
    u32   length;
    u8    dir;
    u8    reserved[CNLIO_RSV_BYTE_3];
    void *pData;
}S_CNL_REG_PASSTHROUGH;

/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/


#endif /* __CNL_TYPE_H__ */
