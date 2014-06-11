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
 *  @file     cnlfit_km.c
 *
 *  @brief    Describes functions to make objects modularized.
 *
 *
 *  @note
 */
/*=================================================================*/

#include <linux/types.h>
#include <linux/version.h>

#include <linux/module.h>
#include <linux/init.h>

#include "cmn_type.h"
#include "cmn_err.h"

#include "cnlwrap_if.h"

#include "cnlfit_upif.h"

#include "cnlup_if.h"


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Extern Functions
 *-----------------------------------------------------------------*/
extern void  CNLFIT_initMod(void);
extern void  CNLFIT_cleanMod(void);




/*-------------------------------------------------------------------
 * Function   : CNLFIT_init
 *-----------------------------------------------------------------*/
/**
 * function to initialize kernel module.
 * @param     nothing
 * @return    alwayse return 0.
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int __init
CNLFIT_init(void)
{
    CNLFIT_initMod();

    return 0;
}


/*-------------------------------------------------------------------
 * Function   : CNLFIT_exit
 *-----------------------------------------------------------------*/
/**
 * function to cleanup kernel module.
 * @param     nothing
 * @return    nothing
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static void __exit
CNLFIT_exit(void)
{
    CNLFIT_cleanMod();

    return;
}


module_init(CNLFIT_init);
module_exit(CNLFIT_exit);


EXPORT_SYMBOL(CNLFIT_open);
EXPORT_SYMBOL(CNLFIT_close);
EXPORT_SYMBOL(CNLFIT_searchEvent);
EXPORT_SYMBOL(CNLFIT_ctrl);
EXPORT_SYMBOL(CNLUP_registerCNL);
EXPORT_SYMBOL(CNLUP_unregisterCNL);
