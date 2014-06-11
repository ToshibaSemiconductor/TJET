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
 *  @file     buscmn_dev.h
 *
 *  @brief    bus common device definitions.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__BUSCMN_DEV_H__)
#define __BUSCMN_DEV_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"

#include "buscmn.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define BUSCMN_DEV_MAX_NUM 1

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
typedef struct tagS_BUSCMN_DEV S_BUSCMN_DEV;

typedef struct S_BUS_OPS {
    T_CMN_ERR (*pSetIrqHandler)(S_BUSCMN_DEV *, T_IRQ_HANDLER, void *);
    T_CMN_ERR (*pIoctl)(S_BUSCMN_DEV *, T_BUSCMN_IOTYPE, void *);
    T_CMN_ERR (*pRead)(S_BUSCMN_DEV *, u32, u32, void *, void *);
    T_CMN_ERR (*pWrite)(S_BUSCMN_DEV *, u32, u32, void *, void *);
}S_BUS_OPS;


/**
 * @common device structure.
 */
struct tagS_BUSCMN_DEV {
    void          *pDev;     // pointer to the real device.
    S_BUS_OPS     *pBusOps;  // bus operations.(bus depended)
    unsigned long  priv[0];  // device private data field.(bus depended)
};


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
S_BUSCMN_DEV *BUSCMN_allocDev(T_BUSCMN_TYPE, u32);
void          BUSCMN_releaseDev(S_BUSCMN_DEV *);
T_CMN_ERR     BUSCMN_registerDev(S_BUSCMN_DEV *);
T_CMN_ERR     BUSCMN_unregisterDev(S_BUSCMN_DEV *);
S_BUSCMN_DEV *BUSCMN_devToCmnDev(void *);

#endif /* __BUSCMN_DEV_H__ */
