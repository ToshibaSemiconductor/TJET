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
 *  @file     bus_sdio.h
 *
 *  @brief    SDIO bus interface functions.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__BUS_SDIO_H__)
#define __BUS_SDIO_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"

#include "buscmn.h"
#include "buscmn_dev.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
extern T_CMN_ERR BUS_sdioRegisterDriver(S_BUSCMN_DRIVER *);
extern T_CMN_ERR BUS_sdioUnregisterDriver(S_BUSCMN_DRIVER *);

extern T_CMN_ERR BUS_sdioSetIrqHandler(S_BUSCMN_DEV *, T_IRQ_HANDLER, void *);
extern T_CMN_ERR BUS_sdioIoctl(S_BUSCMN_DEV *, T_BUSCMN_IOTYPE, void *);
extern T_CMN_ERR BUS_sdioRead(S_BUSCMN_DEV *, u32, u32, void *, void *);
extern T_CMN_ERR BUS_sdioWrite(S_BUSCMN_DEV *, u32, u32, void *, void *);

#endif /* __BUS_SDIO_H__ */
