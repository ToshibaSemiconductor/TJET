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
 *  @file     cmn_rsc.h
 *
 *  @breif    Manages the resorces used in core modules.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_RSC_H__)
#define __CMN_RSC_H__

#include "cmn_type.h"
#include "cmn_err.h"


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief Fixed memory pool object resource IDs
 */
enum tagE_CMN_MPL_RSC_ID {
    // toscnl
    CNL_DEV_MPL_ID                   = 1,
    CNL_DUMMY_REQ_MPL_ID,

    // toscnlev
    CNLEV_DEV_MPL_ID,

    // toscnlctl
    CNLCTL_CNLREQ_MPL_ID,
    CNLCTL_CNLEVT_MPL_ID,

    CMN_MPL_RSC_ID_MAX,
};


/**
 * @brief Fixed memory pool object size
 */
enum tagE_CMN_MPL_SIZE {
    // toscnl
    CNL_DEV_MPL_SIZE                 = 640, // It is actual 512B, when a 64-bit data model is LP64.
    CNL_DUMMY_REQ_MPL_SIZE           = 144, // It is actual 136B, when a 64-bit data model is LP64.

    // toscnlev
    CNLEV_DEV_MPL_SIZE               = 128, // actual 100

    // toscnlctl
    CNLCTL_CNLREQ_MPL_SIZE           = 128, // actual 100
    CNLCTL_CNLEVT_MPL_SIZE           = 64,  // actual 40
};


/**
 * @brief Fixed memory pool object max count
 */
enum tagE_CMN_MPL_CNT {
    // toscnl
    CNL_DEV_MPL_CNT                  = 1,
    CNL_DUMMY_REQ_MPL_CNT            = 10,

    // toscnlev
    CNLEV_DEV_MPL_CNT                = 10,

    // toscnlctl
    CNLCTL_CNLREQ_MPL_CNT            = 10,
    CNLCTL_CNLEVT_MPL_CNT            = 10,
};


/**
 * @brief Semaphore object resource IDs
 */
enum tagE_CMN_SEM_RSC_IDS {
    // toscnl
    CNL_DEV_MTX_ID                   = 1,
    CNL_REQ_MTX_ID,
    CNL_REQ_WAIT_ID,
    CNL_CANCEL_WAIT_ID,
    CNL_INT_MTX_ID,

    // toscnlev
    CNLEV_DEV_MTX_ID,
    CNLEV_DB_MTX_ID,
    CNLEV_REG_INT_WAIT_ID,
    CNLEV_PERF_REQ_WAIT_ID,

    // toscnlctl
    CNLCTL_DEV_MTX_ID,
    CNLCTL_DB_MTX_ID,

    CMN_SEM_RSC_ID_MAX,
};


/**
 * @brief CPU lock object resource IDs (original extension)
 */
enum tagE_CMN_LOC_RSC_ID {
    // toscnl
    CNL_DEV_SIG_LOC_ID               = 1,
    CNL_DEV_MNG_LOC_ID,

    CMN_LOC_RSC_ID_MAX,
};


/**
 * @brief Task object resource IDs
 */
enum tagE_CMN_TASK_RSC_IDS {
    // toscnl
    CNL_TASK_ID                      = 1,

    CMN_TASK_RSC_ID_MAX,
};


/**
 * @brief Timer object resource IDs
 */
enum tagE_CMN_TIMER_RSC_IDS {
    // toscnl
    CNL_CMD_TOUT_TIM_ID              = 1,
    CNL_CANCEL_TOUT_TIM_ID,

    CNL_TIM_ID_MAX,
};

// resources end.


#endif //__CMN_RSC_H__
