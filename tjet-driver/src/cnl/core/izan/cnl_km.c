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
 *  @file     cnl_km.c
 *
 *  @brief    kernel module related functions for linux
 *
 *
 *  @note
 */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include <linux/module.h>
#include "cnl.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.1";
#define DRIVER_DESC "CNL Core Driver";


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
/**
 * Module Initailize/Cleanup functions.
 * called when module installed or rmoved.
 */
/*-------------------------------------------------------------------
 * Function : CNL_initModule
 *-----------------------------------------------------------------*/
/**
 * Initialize routine called when module inserted to kernel.
 * @param   nothing.
 * @return  0  (normally completion)
 * @return  -1 (error occurred)
 * @note   
 */
/*-----------------------------------------------------------------*/
static int __init
CNL_initModule(void) 
{
    return CNL_init();
}


/*-------------------------------------------------------------------
 * Function : CNL_exitModule
 *-----------------------------------------------------------------*/
/**
 * cleanup routine called when module removed from kernel.
 * @param   nothing.
 * @return  nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void __exit
CNL_exitModule(void) 
{
    CNL_exit();
    return;
}


module_init(CNL_initModule);
module_exit(CNL_exitModule);

/* Module information */
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_VERSION( DRIVER_VERSION );
