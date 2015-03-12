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
 *  @file     cmn_rsc2.h
 *
 *  @breif    Manages the resorces used in core modules.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CMN_RSC2_H__)
#define __CMN_RSC2_H__

#include "cmn_type.h"
#include "cmn_err.h"


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief Fixed memory pool object resource IDs
 */
enum tagE_CMN_MPL_RSC_ID_EXT {
    // toscnlfit
    CNLFIT_DEV_MPL_ID                =    CMN_MPL_RSC_ID_MAX,

    CNLFIT_IOCONT_MPL_ID,

    // sipipe
    SIPIPE_MPL_INFO_ID,
    SIPIPE_MPL_CMD_ID,
};


/**
 * @brief Fixed memory pool object size
 */
enum tagE_CMN_MPL_SIZE_EXT {
    // toscnlfit
    CNLFIT_DEV_MPL_SIZE             = 256, // It is actual 248B, when a 64-bit data model is LP64.
    CNLFIT_IOCONT_MPL_SIZE          = 160, // It is actual 160B, when a 64-bit data model is LP64.
    CNLFIT_TXRX_MPL_SIZE            = 65536, // SEND/RECEIVE Buffer size

    // sipipe
    SIPIPE_MPL_INFO_SIZE             = 64, // temporary(acturl 38)
    SIPIPE_MPL_CMD_SIZE              = 32, // acturl 17
};


/**
 * @brief Fixed memory pool object max count
 */
enum tagE_CMN_MPL_CNT_EXT {
    // toscnlfit
    CNLFIT_DEV_MPL_CNT              = 1,
    CNLFIT_IOCONT_MPL_CNT           = 10,

    // sipipe
    SIPIPE_MPL_INFO_CNT              = 1,
    SIPIPE_MPL_CMD_CNT               = 10, 
    CNLFIT_TXRX_MPL_CNT              = 12,
};


/**
 * @brief Semaphore object resource IDs
 */
enum tagE_CMN_SEM_RSC_IDS_EXT {
    // toscnlfit
    CNLFIT_DEV_MTX_ID                = CMN_SEM_RSC_ID_MAX,
    CNLFIT_WAIT_ADPT_ID,
};


/**
 * @brief CPU lock object resource IDs (original extension)
 */
enum tagE_CMN_LOC_RSC_ID_EXT {
    // toscnlfit
    CNLFIT_CTRL_LOCK_ID              = CMN_LOC_RSC_ID_MAX,
    CNLFIT_ADPT_LOCK_ID,

    // sipipe(temporary)
    SIPIPE_CMD_LOC_ID,
};


// resources end.


#endif /* __CMN_RSC2_H__ */
