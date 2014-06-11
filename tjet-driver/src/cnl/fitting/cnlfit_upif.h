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
 *  @file     cnlfit_upif.h
 *
 *  @brief    Describes macros, structures and fuctions as interface
 *            to show the upper layer.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__CNLFIT_UPIF_H__)
#define __CNLFIT_UPIF_H__

#include "cmn_type.h"
#include "cmn_err.h"
#include "cnl_type.h"
#include "cnl_err.h"
#include "cnlwrap_if.h"

/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
#define CNLFIT_DEVTYPE_CTRL                  1
#define CNLFIT_DEVTYPE_ADPT                  2


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief CNL IO argument bucket management box
 */
typedef struct tag_S_CNLIO_ARG_BUCKET  {
    union {
        S_CNLWRAP_STATUS       status;
        S_CNLWRAP_REQ_INIT     init;
        S_CNLWRAP_REQ_CONNECT  connect;
        S_CNLWRAP_REQ_ACCEPT   accept;
        S_CNLWRAP_REQ_RELEASE  release;
        S_CNLWRAP_REQ_DATA     data;
        S_CNLWRAP_REQ_CANCEL   cancel;
        S_CNLWRAP_REQ_PORT     port;
        S_CNLWRAP_EVENT        event;
        u32                    adptId; // obsolete.
        S_CNLWRAP_STATS        stats;
        S_CNLWRAP_REQ_POWERSAVE powersave;
    }req;
}S_CNLIO_ARG_BUCKET;


/*
 * @brief the structure that packages the callback function and that argument.
 */
typedef struct tagS_CNLIO_CBK_PACK {
    void (*pioCbk)(void *);
    void  *pioArg;
} S_CNLIO_CBK_PACK;


/*
 * @brief the structure for CNLIO to register the callback function.
 */
typedef struct tagS_CNLIO_FIT_CBKS  {
    S_CNLIO_CBK_PACK event;
} S_CNLIO_FIT_CBKS;


/*-------------------------------------------------------------------
 * External Functions
 *-----------------------------------------------------------------*/
//
// for CNLIO module
//
extern T_CMN_ERR CNLFIT_open       (int, int, void **, S_CNLIO_FIT_CBKS *);
extern T_CMN_ERR CNLFIT_close      (int, int, void *);
extern T_CMN_ERR CNLFIT_searchEvent(int, void *);
extern T_CMN_ERR CNLFIT_ctrl       (int, void *, uint, S_CNLIO_ARG_BUCKET *);

#endif /* __CNLFIT_UPIF_H__ */
