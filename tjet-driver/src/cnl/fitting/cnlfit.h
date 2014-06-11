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
 *  @file     cnlfit.h
 *
 *  @brief    Describes macros, structures and fuctions as interface
 *            to use cnlfit internally.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNLFIT_H__)
#define __CNLFIT_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_dbg.h"

#include "oscmn.h"

#include "cnl_type.h"
#include "cnl_if.h"

#include "cnlwrap_if.h"
#include "cnlfit_upif.h"

#include "cnlup_if.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define CNLFIT_DEV_NUM                       1 // same as CNLFIT_DEV_MPL_CNT


#define DEVTYPE_CTRL                         0
#define DEVTYPE_ADPT                         1

#define IO_CONTAINER_NUM                     10


/**
 * @brief I/O container I/O type
 */
typedef enum tagE_CNLFIT_IO_TYPE {
    CNLFIT_IO_TYPE_REQ                       = 1,
    CNLFIT_IO_TYPE_REQ_COMP,
    CNLFIT_IO_TYPE_CNL_EVENT,
    CNLFIT_IO_TYPE_PORT_EVENT,
}E_CNLFIT_IO_TYPE;
typedef u8 T_CNLFIT_IO_TYPE;


/**
 * @brief Ctrl device state
 */
typedef enum tagE_CTRLDEV_STATE {
    CTRL_DEV_READY                           = 1,
    CTRL_DEV_ACTIVE,
    CTRL_DEV_GONE,
}E_CTRLDEV_STATE;
typedef u8 T_CTRLDEV_STATE;

/**
 * @brief Adapter port state
 */
typedef enum tagE_ADPTPORT_STATE {
    ADPT_PORT_DISABLED                       = 1,
    ADPT_PORT_READY,
    ADPT_PORT_ACTIVE,
    ADPT_PORT_SUSPENDING,
    ADPT_PORT_FORCE_CLOSE
}E_ADPTPORT_STATE;
typedef u8 T_ADPTPORT_STATE;

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
/**
 * @brief CNLFIT I/O container.
 */
typedef struct tagS_IO_CONTAINER {
    // management parameter.
    S_LIST                                   list;
    T_CNLFIT_IO_TYPE                         ioType;
    union {
        S_CNL_CMN_REQ                        cnlReq;
        S_CNL_CMN_EVT                        cnlEvt;
        u8                                   portEvt;
    };
}S_IO_CONTAINER;


/**
 * @brief CNLFIT I/O manager
 */
typedef struct tagS_IO_MGR {
    // async request queue member.
    u8                                       lockId;
    S_LIST                                   requestList;
    S_LIST                                   eventList;

    // event queue member.
    u8                                       stopped;     // event stop flag
    S_CNLIO_FIT_CBKS                         ioCbks;
}S_IO_MGR;


typedef struct tagS_CTRL_MGR S_CTRL_MGR;
/**
 * @brief adapter port manager.
 */
typedef struct tagS_ADPT_MGR {
    S_CTRL_MGR                              *pParent; // back pointer to S_CTRL_MGR
    E_ADPTPORT_STATE                         state;   // adapter state
    S_IO_MGR                                 ioMgr;   // I/O manager of ADPT side.
}S_ADPT_MGR;


/** 
 * @brief cnlfit controller manager.
 */ 
struct tagS_CTRL_MGR {
    // device dependent member;
    int                                      index;     // device index

    // access to CNL
    void                                    *pCnlPtr;   // lower layer CNL pointer
    S_CNL_OPS                                cnlOps;   // CNL operations.
    S_PCL_CBKS                               pclCbks;
    u8                                       targetUID[CNL_UID_SIZE];

    T_CTRLDEV_STATE                          state;
    S_IO_MGR                                 ioMgr;     // I/O manager of Ctrl side.

    E_ADPTPORT_STATE                         adptState; // adapter state
    S_ADPT_MGR                               adptMgr;

    u8                                       iocontMplId;
    u8                                       waitAdptId;
    u8                                       ctrlDevMtxId;
};


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/
static inline char *
CNLFIT_cmdToString(uint cmd) {
    switch(cmd) {
    case CNLWRAPIOC_INIT :
        return "CNLWRAPIOC_INIT";
    case CNLWRAPIOC_CLOSE :
        return "CNLWRAPIOC_CLOSE";
    case CNLWRAPIOC_CONNECT :
        return "CNLWRAPIOC_CONNECT";
    case CNLWRAPIOC_WAIT_CONNECT :
        return "CNLWRAPIOC_WAIT_CONNECT";
    case CNLWRAPIOC_ACCEPT :
        return "CNLWRAPIOC_ACCEPT";
    case CNLWRAPIOC_CONFIRM :
        return "CNLWRAPIOC_CONFIRM";
    case CNLWRAPIOC_RELEASE :
        return "CNLWRAPIOC_RELEASE";
    case CNLWRAPIOC_SENDDATA :
        return "CNLWRAPIOC_SENDDATA";
    case CNLWRAPIOC_RECVDATA :
        return "CNLWRAPIOC_RECVDATA";
    case CNLWRAPIOC_CANCEL :
        return "CNLWRAPIOC_CANCEL";
    case CNLWRAPIOC_GETEVENT :
        return "CNLWRAPIOC_GETEVENT";
    case CNLWRAPIOC_STOP_EVENT :
        return "CNLWRAPIOC_STOP_EVENT";
    case CNLWRAPIOC_ENABLE_PORT :
        return "CNLWRAPIOC_ENABLE_PORT";
    case CNLWRAPIOC_DISABLE_PORT :
        return "CNLWRAPIOC_DISABLE_PORT";
    case CNLWRAPIOC_SUSPEND_CONF :
        return "CNLWRAPIOC_SUSPEND_CONF";
    case CNLWRAPIOC_GET_ADPT_ID :
        return "CNLWRAPIOC_GET_ADPT_ID";
    case CNLWRAPIOC_SYNCRECV :
        return "CNLWRAPIOC_SYNCRECV";
    case CNLWRAPIOC_POWERSAVE :
        return "CNLWRAPIOC_POWERSAVE";
    default :
        return "unknown";
    }
}

/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
extern void      CNLFIT_eventCbk(void *, S_CNL_CMN_EVT *);

// I/O control functions.

extern void      CNLFIT_clearAllEvent(S_CTRL_MGR *, S_IO_MGR *);
extern T_CMN_ERR CNLFIT_cnlRequest(int, S_CTRL_MGR *, uint, S_CNLIO_ARG_BUCKET *);

#endif /* __CNLFIT_H__ */
