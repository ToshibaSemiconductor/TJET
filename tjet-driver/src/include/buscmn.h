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
 *  @file     buscmn.h
 *
 *  @brief    common bus module interface definition.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__BUSCMN_H__)
#define __BUSCMN_H__

/*-------------------------------------------------------------------
 * Header section
 *-----------------------------------------------------------------*/
#include "cmn_type.h"
#include "cmn_err.h"


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief bus types.
 */
typedef enum tagE_BUSCMN_TYPE {
    BUSCMN_TYPE_PCI = 0x01,
    BUSCMN_TYPE_SDIO,
    BUSCMN_TYPE_USB,
}E_BUSCMN_TYPE;
typedef u8 T_BUSCMN_TYPE;


/**
 * @brief bus ioctl types.
 */
typedef enum tagE_BUSCMN_IOTYPE {
    BUSCMN_IOTYPE_READ_REG = 0x01,
    BUSCMN_IOTYPE_WRITE_REG,
}E_BUSCMN_IOTYPE;
typedef u8 T_BUSCMN_IOTYPE;

#define BUSCMN_ANY_ID 0xFFFFFFFF


/**
 * status code for ioctl and read/write.
 */
#define BUSSDIO_STATUS_OUT_OF_RANGE 0x01
#define BUSSDIO_STATUS_FUNC_NUM     0x02
#define BUSSDIO_STATUS_ERR          0x08
#define BUSSDIO_STATUS_ILL_CMD      0x40
#define BUSSDIO_STATUS_CRC_ERR      0x80
#define BUSSDIO_STATUS_MASK         0xCB

/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
/**
 * @brief common IDs for detecting device.
 */
typedef struct tagS_BUSCMN_IDS {
    // 
    // set BUSCMN_ANY_ID if unnecessary to match device.
    //
    u32 vendorId;    // manf code if bus type is SDIO
    u32 deviceId;    // manf info if bus type is SDIO
    u32 subVendorId; // use only PCI
    u32 subDeviceId; // use only PCI
    u32 classId;     // class code.
}S_BUSCMN_IDS;


/**
 * @brief Register access params
 * use for ioctl(READ_REG/WRITE_REG)
 */
typedef struct tagS_BUSCMN_REG_CTRL {
    u32   addr;     // register address.
    u32   length;   // rw length.
    void *pData;    // data buffer
    void *pStatus;  // 
}S_BUSCMN_REG_CTRL;


/**
 * @brief common IDs
 */
typedef struct tagS_BUSCMN_DRIVER {
    T_BUSCMN_TYPE  busType;
    S_BUSCMN_IDS   ids;
    char          *pName;
    int            (*pProbe)(void *, S_BUSCMN_IDS *);
    int            (*pRemove)(void *);
    void          *pProfile;
}S_BUSCMN_DRIVER;


typedef void (*T_IRQ_HANDLER)(void *);


/**
 * @brief profile for SDIO bus.
 */
typedef struct tagS_BUSSDIO_PROFILE {
    u16 blockSize;   // 1 - 2048 is valid, recommended value is 512.
    u8  dmaAddrMode; // address mode(OP code of CMD53) for read/write.
                     // 0 : fixed address, 1 : incrementing address
}S_BUSSDIO_PROFILE;


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * External functions/variables
 *-----------------------------------------------------------------*/
// device driver registration.
extern T_CMN_ERR BUSCMN_registerDriver(S_BUSCMN_DRIVER *);
extern T_CMN_ERR BUSCMN_unregisterDriver(S_BUSCMN_DRIVER *);

// common function.
extern T_CMN_ERR BUSCMN_setIrqHandler(void *, T_IRQ_HANDLER, void *);
extern T_CMN_ERR BUSCMN_ioctl(void *, T_BUSCMN_IOTYPE, void *);
extern T_CMN_ERR BUSCMN_read(void *, u32, u32, void *, void *);
extern T_CMN_ERR BUSCMN_write(void *, u32, u32, void *, void *);

// bus dependent function.
extern int BUSCMN_busRequest(u8, void *);

#endif /* __BUSCMN_H__ */
