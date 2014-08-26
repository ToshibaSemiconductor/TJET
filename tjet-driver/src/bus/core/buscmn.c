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
 *  @file     buscmn.c
 *
 *  @brief    common bus interface function
 *
 *
 *  @note
 */
/*=================================================================*/
/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"
#include "cmn_dbg.h"

#include <linux/slab.h>

#include "buscmn.h"
#include "buscmn_dev.h"
#include "bus_sdio.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.1";
#define DRIVER_DESC "CNL Bus Abstraction Driver";

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static S_BUSCMN_DEV *g_pCmnDevArray[BUSCMN_DEV_MAX_NUM] = {};

static S_BUS_OPS g_sdioBusOps = {
    .pSetIrqHandler = BUS_sdioSetIrqHandler,
    .pIoctl         = BUS_sdioIoctl,
    .pRead          = BUS_sdioRead,
    .pWrite         = BUS_sdioWrite,
};

/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/
/*-------------------------------------------------------------------
 * Function : BUSCMN_allocDev
 *-----------------------------------------------------------------*/
/**
 * allocate bus common device.
 * @param  busType  : bus type (SDIO or PCI)
 * @param  privSize : size of private data field.
 * @return pointer to the allocated bus common device.
 * @note   
 */
/*-----------------------------------------------------------------*/
S_BUSCMN_DEV *
BUSCMN_allocDev(T_BUSCMN_TYPE busType, 
                u32 privSize) 
{
    S_BUSCMN_DEV *pCmnDev;
    S_BUS_OPS    *pBusOps = NULL;

    switch(busType) {
    case BUSCMN_TYPE_SDIO : 
        pBusOps = &g_sdioBusOps;
        break;
    case BUSCMN_TYPE_PCI :
    case BUSCMN_TYPE_USB :
        // PCI and USB bus is not supported yet.
    default :
        return NULL;
    }
    pCmnDev = (S_BUSCMN_DEV *)kmalloc(sizeof(S_BUSCMN_DEV) + privSize, GFP_KERNEL);
    if(pCmnDev == NULL) {
        return NULL;
    }

    pCmnDev->pBusOps = pBusOps;
    return pCmnDev;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_releaseDev
 *-----------------------------------------------------------------*/
/**
 * release bus common device.
 * @param  
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
void
BUSCMN_releaseDev(S_BUSCMN_DEV *pCmnDev)
{
    kfree(pCmnDev);
    return;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_registerDev
 *-----------------------------------------------------------------*/
/**
 * register bus common device.
 * @param  pCmnDev : pointer to the bus common device.
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (not found empty array to store)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_registerDev(S_BUSCMN_DEV *pCmnDev) 
{

    u8 devnum;

    for(devnum=0; devnum<BUSCMN_DEV_MAX_NUM; devnum++) {
        if(g_pCmnDevArray[devnum] == NULL) {
            // empty found.
            DBG_INFO("found empty array box\n");
            g_pCmnDevArray[devnum] = pCmnDev;
            return SUCCESS;
        }
    }
    return ERR_NOOBJ;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_unregisterDev
 *-----------------------------------------------------------------*/
/**
 * unregister bus common device.
 * @param  pCmnDev : pointer to the bus common device.
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (device not found)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_unregisterDev(S_BUSCMN_DEV *pCmnDev) 
{

    u8 devnum;

    for(devnum=0; devnum<BUSCMN_DEV_MAX_NUM; devnum++) {
        if(g_pCmnDevArray[devnum] == pCmnDev) {
            // empty found.
            DBG_INFO("found cmn device\n");
            g_pCmnDevArray[devnum] = NULL;
            return SUCCESS;
        }
    }
    return ERR_NOOBJ;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_devToCmnDev
 *-----------------------------------------------------------------*/
/**
 * search bus common device by host device.
 * @param  pDev : pointer to the host device.
 * @return SUCCESS   (normally completion)
 * @return ERR_NOOBJ (device not found)
 * @note   
 */
/*-----------------------------------------------------------------*/
S_BUSCMN_DEV *
BUSCMN_devToCmnDev(void *pDev) 
{

    u8 devnum;

    for(devnum=0; devnum<BUSCMN_DEV_MAX_NUM; devnum++) {
        if((g_pCmnDevArray[devnum] != NULL) && 
           (g_pCmnDevArray[devnum]->pDev == pDev))  {
            // empty found.
            DBG_INFO("found cmn device related dev.\n");
            return g_pCmnDevArray[devnum];
        }
    }
    return NULL;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_registerDriver
 *-----------------------------------------------------------------*/
/**
 * register SDIO card driver driver.
 * @param  pDriver : the pointer to the common bus driver.
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (driver is already registerd)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_registerDriver(S_BUSCMN_DRIVER *pDriver) 
{

    T_CMN_ERR retval;

    switch(pDriver->busType) {
    case BUSCMN_TYPE_SDIO : 
        retval = BUS_sdioRegisterDriver(pDriver);
        break;
    case BUSCMN_TYPE_PCI :
    case BUSCMN_TYPE_USB :
        // PCI and USB bus is not supported yet.
    default :
        DBG_ERR("unkown bus type 0x%x\n", pDriver->busType);
        return ERR_BADPARM;
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_unregisterDriver
 *-----------------------------------------------------------------*/
/**
 * unregister driver.
 * @param  pDriver : the pointer to the common bus driver.
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (driver is already registerd)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_unregisterDriver(S_BUSCMN_DRIVER *pDriver) 
{

    T_CMN_ERR retval;
    
    switch(pDriver->busType) {
    case BUSCMN_TYPE_SDIO : 
        retval = BUS_sdioUnregisterDriver(pDriver);
        break;
    case BUSCMN_TYPE_PCI :
    case BUSCMN_TYPE_USB :
        // PCI and USB bus is not supported yet.
    default :
        DBG_ERR("unkown bus type 0x%x\n", pDriver->busType);
        return ERR_BADPARM; 
    }

    return retval;

}


/*-------------------------------------------------------------------
 * Function : BUSCMN_setIrqHandler
 *-----------------------------------------------------------------*/
/**
 * set IRQ handler.
 * @param  pPtr        : pointer to the bus common device.
 * @param  pIrqHandler : pointer to the IRQ handler function.
 * @param  pArg        : pointer to the IRQ handler argument.
 * @return SUCCESS     (normally completion)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_setIrqHandler(void          *pPtr,
                     T_IRQ_HANDLER  pIrqHandler,
                     void          *pArg) 
{

    S_BUSCMN_DEV *pCmnDev = (S_BUSCMN_DEV *)pPtr;
    return pCmnDev->pBusOps->pSetIrqHandler(pCmnDev, pIrqHandler, pArg);

}


/*-------------------------------------------------------------------
 * Function : BUSCMN_ioctl
 *-----------------------------------------------------------------*/
/**
 * execute I/O to the device(read/write register etc..)
 * @param  pPtr        : pointer to the bus common device.
 * @param  ioType      : I/O type to the device.
 * @param  pArg        : pointer to the argument of I/O
 * @return SUCCESS     (normally completion)
 * @return ERR_BADPARM (bad parameter error)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUSCMN_ioctl(void            *pPtr,
             T_BUSCMN_IOTYPE  ioType,
             void            *pArg) 
{

    S_BUSCMN_DEV *pCmnDev = (S_BUSCMN_DEV *)pPtr;
    if ( pCmnDev != NULL ) {
        if ( pCmnDev->pBusOps != NULL ) {
            if ( pCmnDev->pBusOps->pIoctl != NULL) {
                if ( pArg != NULL ) {
                    return pCmnDev->pBusOps->pIoctl(pCmnDev, ioType, pArg);
                }
            }
        }
    }
    return ERR_SYSTEM;
}


/*-------------------------------------------------------------------
 * Function : BUSCMN_read
 *-----------------------------------------------------------------*/
/**
 * read chunk of data from device.
 * @param  pPtr        : pointer to the bus common device
 * @param  addr        : data address of device.
 * @param  length      : data length to read.
 * @param  pData       : pointer to the data buffer.
 * @param  pStatus     : return pointer of status
 * @return SUCCESS     (normally completion)
 * @return ERR_BADPARM (bad parameter error)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUSCMN_read(void *pPtr, 
            u32   addr,
            u32   length,
            void *pData,
            void *pStatus)
{

    S_BUSCMN_DEV *pCmnDev = (S_BUSCMN_DEV *)pPtr;
    return pCmnDev->pBusOps->pRead(pCmnDev, addr, length, pData, pStatus);

}


/*-------------------------------------------------------------------
 * Function : BUSCMN_write
 *-----------------------------------------------------------------*/
/**
 * write chunk of data to device.
 * @param  pPtr        : pointer to the host device(S_BUSCMN_DEV) 
 * @param  addr        : address of the device.
 * @param  length      : data length to write.
 * @param  pData       : pointer to the data buffer.
 * @param  pStatus     : return pointer of status
 * @return SUCCESS     (normally completion)
 * @return ERR_BADPARM (bad parameter error)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUSCMN_write(void *pPtr, 
             u32   addr,
             u32   length,
             void *pData,
             void *pStatus)
{

    S_BUSCMN_DEV *pCmnDev = (S_BUSCMN_DEV *)pPtr;
    return pCmnDev->pBusOps->pWrite(pCmnDev, addr, length, pData, pStatus);

}


/**
 * Module Initailize/Cleanup functions.
 * called when module installed or rmoved.
 */
/*-------------------------------------------------------------------
 * Function : BUSCMN_initModule
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
BUSCMN_initModule(void) 
{
    return 0;
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
BUSCMN_exitModule(void) 
{
    return;
}

module_init(BUSCMN_initModule);
module_exit(BUSCMN_exitModule);

/* Module information */
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_VERSION( DRIVER_VERSION );

//
// EXPORT_SYMBOLS
//
EXPORT_SYMBOL(BUSCMN_registerDriver);
EXPORT_SYMBOL(BUSCMN_unregisterDriver);
EXPORT_SYMBOL(BUSCMN_setIrqHandler);
EXPORT_SYMBOL(BUSCMN_ioctl);
EXPORT_SYMBOL(BUSCMN_read);
EXPORT_SYMBOL(BUSCMN_write);
