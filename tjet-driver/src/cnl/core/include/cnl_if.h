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
 *  @file     cnl_if.h
 *
 *  @brief    definitions of CNL interface structures for upper layer.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNL_IF_H__)
#define __CNL_IF_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"

#include "cnl_type.h"
#include "cnl_err.h"

#include "oscmn.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define CMN_REQ_EXT_SIZE 32

/**
 * @brief CNL cancel request discardRequestID values 
 */
#define CNL_DISCARD_RECVDATA_0  (0UL)       /* Discard ProfileID:0 receive data */
#define CNL_DISCARD_RECVDATA_1  (~0UL)      /* Discard ProfileID:1 receive data */

/**
 * @brief CNL request types
 */
typedef enum tagE_CNL_REQ_TYPE {
    CNL_REQ_TYPE_INIT_REQ = 1,
    CNL_REQ_TYPE_CLOSE_REQ,
    CNL_REQ_TYPE_CONNECT_REQ,
    CNL_REQ_TYPE_WAIT_CONNECT_REQ,
    CNL_REQ_TYPE_ACCEPT_REQ,
    CNL_REQ_TYPE_ACCEPT_RES,
    CNL_REQ_TYPE_RELEASE_REQ,
    CNL_REQ_TYPE_SEND_REQ,
    CNL_REQ_TYPE_RECEIVE_REQ,
    CNL_REQ_TYPE_REG_DATA_IND,
    CNL_REQ_TYPE_UNREG_DATA_IND,

    CNL_REQ_TYPE_GET_STATS,
    CNL_REQ_TYPE_REG_PASSTHROUGH,

    //  CNL driver internal use only
    CNL_REQ_TYPE_POWERSAVE_REQ,
    CNL_REQ_TYPE_WAKE_REQ,
} E_CNL_REQ_TYPE;
typedef u8 T_CNL_REQ_TYPE;


/**
 * @brief CNL event types
 */
typedef enum tagE_CNL_EVT_TYPE {
    CNL_EVT_TYPE_ERROR_IND = 1, // additional
    CNL_EVT_TYPE_CONNECT_IND,
    CNL_EVT_TYPE_ACCEPT_IND,
    CNL_EVT_TYPE_ACCEPT_CNF,
    CNL_EVT_TYPE_RELEASE_IND,
    CNL_EVT_TYPE_POWERSAVE_IND,
} E_CNL_EVT_TYPE;
typedef u8 T_CNL_EVT_TYPE;

typedef unsigned long T_CNL_REQ_ID;


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
/**
 * @brief CNL_SAP Wake.request parameter
 *        internal use only.
 */
typedef struct tagS_CNL_WAKE_REQ {
    u8 dormantPeriod;
    u8 awakePeriod;
}S_CNL_WAKE_REQ;


/**
 * @brief CNL data received callback type.
 */
typedef void(*T_CNL_RECV_CBK)(void *, u8);


/**
 * @brief CNL register Data.indication callback parameter structure.
 */
typedef struct tagS_CNL_REG_DATA_IND {
    u8              profileId;
    u8              reserved;
    u16             reserved2;
    T_CNL_RECV_CBK  pCbk;
    void           *pArg;
}S_CNL_REG_DATA_IND;


/**
 * @brief CNL unregister Data.indication callback parameter structure.
 */
typedef struct tagS_CNL_UNREG_DATA_IND {
    u8 profileId;
}S_CNL_UNREG_DATA_IND;


/**
 * @brief CNL common request.
 */
typedef struct tagS_CNL_CMN_REQ S_CNL_CMN_REQ;
struct tagS_CNL_CMN_REQ {

    T_CNL_ERR status;

    //
    // request dependent data field.
    //
    // shows request type and union field table as below.
    // INIT_REQ         : initReq
    // CLOSE_REQ        : closeReq
    // CONNECT_REQ      : connectReq
    // WAIT_CONNECT_REQ : wconnectReq
    // ACCEPT_REQ       : acceptReq
    // ACCEPT_RES       : acceptRes
    // RELEASE_REQ      : releaseReq
    // SEND_REQ         : dataReq;
    // RECEIVE_REQ      : dataReq;
    // REG_DATA_IND     : regDataInd
    // UNREG_DATA_IND   : unregDataInd
    // GET_STATS        : stats
    // 
    // test mode only.
    // INITIALIZE_CARD  : initCard;
    // SET_IRQHANDLER   : setIrqHandler;
    // DIRECT_CMD       : directCmd;
    // BURST_SEND_REQ   : dataReq;
    // BURST_RECEIVE_REQ: dataReq;
    // GET_STATISTICCS  : getStatistics;
    // START            : start;
    // STOP             : stop;
    // RESET            : reset;
    //
    union {
        S_CNL_INIT_REQ         initReq;
        S_CNL_CLOSE_REQ        closeReq;       // no param.
        S_CNL_CONNECT_REQ      connectReq;
        S_CNL_WAIT_CONNECT_REQ wconnectReq;    // no param.
        S_CNL_ACCEPT_REQ       acceptReq;
        S_CNL_ACCEPT_RES       acceptRes;
        S_CNL_RELEASE_REQ      releaseReq;
        S_CNL_DATA_REQ         dataReq;
        S_CNL_REG_DATA_IND     regDataInd;
        S_CNL_UNREG_DATA_IND   unregDataInd;

        S_CNL_STATS            stats;
        S_CNL_REG_PASSTHROUGH  regPT;

        // internal use only.
        S_CNL_POWERSAVE_REQ    powersaveReq;
        S_CNL_WAKE_REQ         wakeReq;
    };

    /******* below fields are this structure depended ******/

    //
    // list head element
    // used for queue management by CNL internally.
    //
    S_LIST         list;

    //
    // request type.
    //
    T_CNL_REQ_TYPE type;

    //
    // request identifier.
    // this Id is used to cancel itself.
    //
    T_CNL_REQ_ID   id;


    //
    // request state and extention data.
    // these field are used by CNL internally.
    //
    u8 state;
    u8 dummy[3];
    // should be aligned
    u8 extData[CMN_REQ_EXT_SIZE];



    //
    // completion callback.
    // completion callback is called when request is completed in CNL,
    // except for initReq and closeReq request.
    // these two requests are defined as block-request.
    // 1st argument is pointer to the S_CNL_CMN_REQ itself,
    // 2nd argument is given by pArg1.(set NULL if unnecessary)
    // 3nd argument is given by pArg2.(set NULL if unnecessary)
    //
    void           (*pComplete)(S_CNL_CMN_REQ *, void *, void *);
    void           *pArg1;
    void           *pArg2;

};


/**
 * @brief CNL common event.
 */
typedef struct tagS_CNL_CMN_EVT {
    //
    // request type.
    //
    T_CNL_EVT_TYPE type;

    //
    // common data field.
    // data format is event dependent.
    //
    // shows request-format table as below.
    // ERROR_IND   : error;
    // CONNECT_IND : connectInd
    // ACCEPT_IND  : acceptInd
    // ACCEPT_CNF  : acceptCnf
    // RELEASE_IND : releaseInd
    // 
    union {
        T_CNL_ERR         error;
        S_CNL_CONNECT_IND connectInd;
        S_CNL_ACCEPT_IND  acceptInd;
        S_CNL_ACCEPT_CNF  acceptCnf;
        S_CNL_RELEASE_IND releaseInd;
        S_CNL_POWERSAVE_IND powersaveInd;
    };
} S_CNL_CMN_EVT;


/**
 * @brief PCL callbacks
 */
typedef struct tagS_PCL_CBKS {
    // callback argument.
    // used as 1st parameter of below indication/confirmation
    void *pArg;
    void (*pCnlEvent)(void *, S_CNL_CMN_EVT *);
}S_PCL_CBKS;


/**
 * @brief CNL Operations structure for registration to upper.
 *        these functions are same as below external functions.
 */
typedef struct tagS_CNL_OPS {
    // CNL function pointers.
    T_CMN_ERR (*pOpen)(void **, S_PCL_CBKS *);
    T_CMN_ERR (*pClose)(void *);

    T_CMN_ERR   (*pRequest)(void *, S_CNL_CMN_REQ *);

    // utilities.
    T_CMN_ERR   (*pCancel)(void *, T_CNL_REQ_ID);
    T_CNL_STATE (*pGetState)(void *);
}S_CNL_OPS;


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
// these function are used for direct I/O driver.
extern T_CMN_ERR    CNL_open(void **, S_PCL_CBKS *);
extern T_CMN_ERR    CNL_close(void *);
extern T_CMN_ERR    CNL_request(void *, S_CNL_CMN_REQ *);
extern T_CMN_ERR    CNL_cancel(void *, T_CNL_REQ_ID);
extern T_CNL_STATE  CNL_getState(void *);

#endif /* __CNL_IF_H__ */
