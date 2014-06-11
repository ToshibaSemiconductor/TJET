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
 *  @file     bus_sdio.c
 *
 *  @brief    SDIO bus interface function
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

#include <asm/atomic.h>
#include <asm/bitops.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#include "buscmn.h"
#include "buscmn_dev.h"
#include "bus_sdio.h"
#include "sdcard_func_if.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
#define SDIO_DIR_IN            0
#define SDIO_DIR_OUT           1
#define SDIO_BLKMODE_OFF       0
#define SDIO_BLKMODE_ON        1
#define SDIO_OPCODE_FIXED_ADDR 0
#define SDIO_OPCODE_INC_ADDR   1


/*-------------------------------------------------------------------
 * Structure definition
 *-----------------------------------------------------------------*/
typedef struct tagS_SDIO_PRIV {
    // define SDIO device private data if necessary
    u8  function;
    u16 blockSize;
    u8  dmaAddrMode;
}S_SDIO_PRIV;

#define DEV_TO_PRIV(x) ((S_SDIO_PRIV *)((x)->priv))

/*-------------------------------------------------------------------
 * Prototypes
 *-----------------------------------------------------------------*/
static void      BUS_sdioProbe(struct sdcard_device *);  // temporary 
static void      BUS_sdioRemove(struct sdcard_device *); // temporary
static int       BUS_sdioSetBlockSize(struct sdcard_device *, u8, u16 *);


/*-------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------*/
static S_BUSCMN_DRIVER      *g_pCmnDrv = NULL;
static unsigned long         g_used    = 0;
static struct sdcard_driver  g_sdioDrv;


/*-------------------------------------------------------------------
 * Inline functions definition
 *-----------------------------------------------------------------*/
static inline int
BUS_sdioExecCmd52(struct sdcard_device *pSdDev, u8 dir, u8 raw, 
                  u32 addr, u8 *pData, u8 *pStatus)
{

    int                 retval;
    struct sdcard_cmd52 cmd52 = {0};

    cmd52.direction  = (unsigned int)dir;
    cmd52.raw        = (unsigned int)raw;
    cmd52.regaddr    = (unsigned int)addr;
    cmd52.data       = (dir == SDIO_DIR_OUT) ? (unsigned int)*pData : 0;


    if (dir == SDIO_DIR_OUT) {
        DBG_INFO2("cmd52 p:d=%d, raw=%d, ad=%08x, d(O): %02x\n",
             dir, raw, addr, cmd52.data);
    }

    /* down_interruptible(&pSdDev->sem); */
    retval = sdcard_cmd52(pSdDev, &cmd52);
    /* up(&pSdDev->sem); */

    if (retval == 0) {
        DBG_INFO2("cmd52 returns %d\n", retval);
    }

    if ((retval == TRUE) && (dir == SDIO_DIR_IN)) {
        DBG_INFO2("cmd52 p:d=%d, raw=%d, ad=%08x, d(O): %02x\n",
             dir, raw, addr, cmd52.data);
    }

    // return back data.
    *pData   = (u8)cmd52.data;
    *pStatus = (u8)cmd52.resp_flags;


    return retval;
}


static inline int
BUS_sdioExecCmd52Func0(struct sdcard_device *pSdDev, u8 dir, u8 raw, 
                       u32 addr, u8 *pData, u8 *pStatus)
{

    int                 retval;
    struct sdcard_cmd52 cmd52 = {0};

    cmd52.direction  = (unsigned int)dir;
    cmd52.raw        = (unsigned int)raw;
    cmd52.regaddr    = (unsigned int)addr;
    cmd52.data       = (dir == SDIO_DIR_OUT) ? (unsigned int)*pData : 0;


    if (dir == SDIO_DIR_OUT) {
        DBG_INFO2("cmd52f p:d=%d, raw=%d, ad=%08x, d(O): %02x\n",
             dir, raw, addr, cmd52.data);
    }

    /* down_interruptible(&pSdDev->sem); */
    retval = sdcard_cmd52_funcnum(pSdDev, &cmd52, 0);
    /* up(&pSdDev->sem); */

    if (retval == 0) {
        DBG_INFO2("cmd52f returns %d\n", retval);
    }

    if ((retval == TRUE) && (dir == SDIO_DIR_IN)) {
        DBG_INFO2("cmd52f p:d=%d, raw=%d, ad=%08x, d(I): %02x\n",
             dir, raw, addr, cmd52.data);
    }

    // return back data.
    *pData   = (u8)cmd52.data;
    *pStatus = (u8)cmd52.resp_flags;


    return retval;
}


static inline int
BUS_sdioExecCmd53(struct sdcard_device *pSdDev, u8 dir, u8 blockMode,
                  u8 opCode, u32 addr, u16 count, void *pData, u8 *pStatus)
{

    int                 retval;
    struct sdcard_cmd53 cmd53 = {0};

    cmd53.direction  = (unsigned int)dir;
    cmd53.bm         = blockMode;
    cmd53.op         = opCode;
    cmd53.regaddr    = (unsigned int)addr;
    cmd53.bcount     = (unsigned int)count;
    cmd53.dbuf       = (unsigned char *)pData;

    if (dir == SDIO_DIR_OUT) {
        DBG_INFO2("cmd53 p:d=%d, bm=%d, oc=%d, ad=%08x, c=%d, d(O): %02x, %02x, %02x, %02x, %02x\n",
             dir, blockMode, opCode, addr, count, cmd53.dbuf[0], cmd53.dbuf[1], cmd53.dbuf[2], cmd53.dbuf[3], cmd53.dbuf[4]);
    }

    /* down_interruptible(&pSdDev->sem); */
    retval = sdcard_cmd53(pSdDev, &cmd53);
    /* up(&pSdDev->sem); */

    if (retval == 0) {
        DBG_INFO2("cmd53 returns %d\n", retval);
    }

    if ((retval == TRUE) && (dir == SDIO_DIR_IN)) {
        DBG_INFO2("cmd53 p:d=%d, bm=%d, oc=%d, ad=%08x, c=%d, d(I): %02x, %02x, %02x, %02x, %02x\n",
             dir, blockMode, opCode, addr, count, cmd53.dbuf[0], cmd53.dbuf[1], cmd53.dbuf[2], cmd53.dbuf[3], cmd53.dbuf[4]);
    }

    // return back data.
    *pStatus = (u8)cmd53.resp_flags;


    return retval;
}


/*-------------------------------------------------------------------
 * Function declarations
 *-----------------------------------------------------------------*/

/*-------------------------------------------------------------------
 * Function : BUS_sdioSetBlockSize
 *-----------------------------------------------------------------*/
/**
 * set block size for this function
 * @param  pSdDev     : pointer to the sdcard_device.
 * @param  function   : function number.
 * @param  pBlockSize : block size.
 * @return  0 (normally completion)
 * @return -1 (error)
 * @note   
 */
/*-----------------------------------------------------------------*/
static int
BUS_sdioSetBlockSize(struct sdcard_device *pSdDev,
                     u8                    function,
                     u16                  *pBlockSize)
{

    int                 retval;
    u32                 addr;
    u8                  status;
    u8                  data;
    u8                  dir;
    u16                 blockSize;

    DBG_ASSERT(pBlockSize != NULL);
    DBG_ASSERT(*pBlockSize <= 2048);

    if(*pBlockSize == 0) {
        dir = SDIO_DIR_IN;
        blockSize = 0;
    } else {
        dir = SDIO_DIR_OUT;
        blockSize = *pBlockSize;
    }

    addr = function * 0x100 + 0x10; // Function N's block size register.
    data = (u8)(cpu_to_le16(blockSize) & 0xFF);  // little-endian.
    retval = BUS_sdioExecCmd52Func0(pSdDev, dir, 0, addr, &data, &status);
    if((retval == FALSE) || ((status & 0xCB) != 0)) {
        DBG_ERR("cmd52 for set block size[L] failed.(retval=%d, status=0x%x)\n",
                retval, status);
    }
    blockSize = (blockSize & 0xFF00) | data;

    addr = function * 0x100 + 0x11;
    data = (u8)((cpu_to_le16(blockSize) >> 8) & 0xFF); // little-endian.
    retval = BUS_sdioExecCmd52Func0(pSdDev, dir, 0, addr, &data, &status);
    if((retval == FALSE) || ((status & 0xCB) != 0)) {
        DBG_ERR("cmd52 for set block size[H] failed.(retval=%d, status=0x%x)\n",
                retval, status);
        return -1;
    }
    blockSize = (blockSize & 0x00FF) | (data << 8);

    if(dir == SDIO_DIR_IN) {
        *pBlockSize = blockSize;
    }

    DBG_ASSERT(blockSize != 0);

    return 0;
}


/*-------------------------------------------------------------------
 * Function : BUS_sdioRegisterDriver
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
BUS_sdioRegisterDriver(S_BUSCMN_DRIVER *pDriver) 
{

    void  *pRetval = NULL;

    if(test_and_set_bit(0, &g_used) != 0) {
        DBG_ERR("SDIO card driver is already registered.\n");
        return ERR_INVSTAT;
    }

    g_pCmnDrv = pDriver;

    //
    // convert ID table. S_BUSCMN_DRIVER to sdcard_driver
    // (ID table).
    //
    g_sdioDrv.Name       = pDriver->pName;
    g_sdioDrv.Probe      = BUS_sdioProbe;
    g_sdioDrv.Disconnect = BUS_sdioRemove;

    pRetval = sdcard_register_driver(&g_sdioDrv);

    DBG_INFO("sdcard_register_driver returns %d\n", (pRetval == NULL)?0:pRetval);

    if(pRetval == NULL) {
        return ERR_SYSTEM;
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : BUS_unregisterDriver
 *-----------------------------------------------------------------*/
/**
 * unregister driver.
 * @param  pDriver : the pointer to the common bus driver.
 * @return SUCCESS     (normally completion)
 * @return ERR_INVSTAT (driver is not already registerd)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR
BUS_sdioUnregisterDriver(S_BUSCMN_DRIVER *pDriver) 
{

    int retval;

    if(test_bit(0, &g_used) == 0) {
        DBG_ERR("SDIO card driver is not registerd.\n");
        return ERR_INVSTAT;
    }

    retval = sdcard_unregister_driver(&g_sdioDrv);

    DBG_INFO("sdcard_unregister_driver returns %d\n", retval);

    if(retval == FALSE) {
        return ERR_SYSTEM;
    }

    g_pCmnDrv = NULL;

    clear_bit(0, &g_used);

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : BUS_sdioProbe
 *-----------------------------------------------------------------*/
/**
 * probe function for SDCARD driver.
 * @param  pSdDev : the pointer to the sdcard device.
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
BUS_sdioProbe(struct sdcard_device *pSdDev)
{

    int               retval;
    S_BUSCMN_DEV      *pCmnDev;
    S_SDIO_PRIV       *pPriv;
    S_BUSSDIO_PROFILE *pProfile;

    DBG_INFO("bus probe called.\n");
    DBG_ASSERT(g_pCmnDrv != NULL);

    pCmnDev = BUSCMN_allocDev(BUSCMN_TYPE_SDIO, sizeof(S_SDIO_PRIV));
    if(pCmnDev == NULL) {
        DBG_ERR("allocate bus common device failed.\n");
        return;
    }

    // setup private field if necessary.
    pPriv = DEV_TO_PRIV(pCmnDev);
    pProfile = (S_BUSSDIO_PROFILE *)g_pCmnDrv->pProfile;

    // setup common device operation.
    pCmnDev->pDev      = (void *)pSdDev;
    pPriv->function    = 1;
    pPriv->blockSize   = pProfile->blockSize;
    pPriv->dmaAddrMode = pProfile->dmaAddrMode;

    retval = BUS_sdioSetBlockSize(pSdDev, pPriv->function, &pPriv->blockSize);
    if(retval != 0) {
        BUSCMN_releaseDev(pCmnDev);
        return;
    }
    DBG_ASSERT(pPriv->blockSize != 0);

    BUSCMN_registerDev(pCmnDev);

    // call upper layer probe function.
    retval = g_pCmnDrv->pProbe((void *)pCmnDev, &g_pCmnDrv->ids);
    if(retval != 0) {
        DBG_ERR("call upper layer probe function failed.\n");
        BUSCMN_unregisterDev(pCmnDev);
        BUSCMN_releaseDev(pCmnDev);
        return;
    }

    DBG_INFO("probe device completed successfully.\n");

    return;

}


/*-------------------------------------------------------------------
 * Function : BUS_sdioRemove
 *-----------------------------------------------------------------*/
/**
 * disconnect function for SDCARD driver.
 * @param  pSdDev : the pointer to the sdcard device.
 * @return nothing.
 * @note   
 */
/*-----------------------------------------------------------------*/
static void
BUS_sdioRemove(struct sdcard_device *pSdDev)
{

    int           retval;
    S_BUSCMN_DEV *pCmnDev;

    pCmnDev = BUSCMN_devToCmnDev(pSdDev);
    if(pCmnDev == NULL) {
        DBG_ERR("not found device to remove..\n");
        goto EXIT;
    }

    // call upper layer probe function.
    retval = g_pCmnDrv->pRemove((void *)pCmnDev);
    if(retval != 0) {
        DBG_ERR("call upper layer remove function failed.\n");
    }

    BUSCMN_unregisterDev(pCmnDev);
    BUSCMN_releaseDev(pCmnDev);

    DBG_INFO("disconnect device completed successfully.\n");

EXIT:
    return;
}


/*-------------------------------------------------------------------
 * Function : BUS_sdioSetIrqHandler
 *-----------------------------------------------------------------*/
/**
 * set IRQ handler
 * @param  pCmnDev    : pointer to the bus common device.
 * @param  pIrqHandle : pointer to the IRQ handler function.
 * @param  pArg       : pointer to the IRQ handler argument..
 * @return SUCCESS     (normally completion)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUS_sdioSetIrqHandler(S_BUSCMN_DEV  *pCmnDev,
                      T_IRQ_HANDLER  pIrqHandler,
                      void          *pArg)
{

    int           retval;
    S_SDIO_PRIV  *pPriv;

    pPriv = DEV_TO_PRIV(pCmnDev);

    retval = sdcard_register_irq_handler(pIrqHandler, pArg);

    DBG_INFO("sdcard_register_irq_handler returns %d.\n", retval);

    if(retval == FALSE) {
        return ERR_SYSTEM;
    }

    return SUCCESS;
}


/*-------------------------------------------------------------------
 * Function : BUS_sdioIoctl
 *-----------------------------------------------------------------*/
/**
 * I/O access to SDIO device(currently register read/write is enabled)
 * @param  pCmnDev    : pointer to the bus common device.
 * @param  ioType     : I/O type.
 * @param  pArg       : pointer to the IRQ handler argument..
 * @return SUCCESS     (normally completion)
 * @return ERR_BADPARM (bad parameter error)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUS_sdioIoctl(S_BUSCMN_DEV    *pCmnDev, 
              T_BUSCMN_IOTYPE  ioType,
              void            *pArg)
{

    int                   retval = 0;
    S_BUSCMN_REG_CTRL    *pRegCtrl;
    S_SDIO_PRIV          *pPriv;

    struct sdcard_device *pSdDev;
    u8                    dir;

    u32                   length;
    u32                   rest;
    u32                   pos;
    u16                   count;
    u8                    blockMode;

    pSdDev = (struct sdcard_device *)pCmnDev->pDev;
    pPriv  = DEV_TO_PRIV(pCmnDev);

    switch(ioType) {
    case BUSCMN_IOTYPE_READ_REG :
        pRegCtrl = (S_BUSCMN_REG_CTRL *)pArg;
        dir = SDIO_DIR_IN;
        break;
    case BUSCMN_IOTYPE_WRITE_REG :
        pRegCtrl = (S_BUSCMN_REG_CTRL *)pArg;
        dir = SDIO_DIR_OUT;
        break;
    default : 
        DBG_ERR("sdio bus is not support ioType[%d].\n", ioType);
        return ERR_BADPARM;
    }

    //
    // convert REG_CTRL to cmd52/cmd53
    // if length is 1 then use cmd52, else use cmd53.
    // and detail informations are given from profile of S_BUSCMN_DRIVER.
    //
    if(pRegCtrl->length == 1) {
        retval = BUS_sdioExecCmd52(pSdDev,
                                   dir,
                                   0,
                                   pRegCtrl->addr,
                                   (u8 *)pRegCtrl->pData,
                                   (u8 *)pRegCtrl->pStatus);
        if(retval == FALSE) {
            DBG_ERR("exec cmd52 failed.\n");
        }
    } else {
        rest = pRegCtrl->length;
        pos  = 0;
        while(rest > 0) {
            length = rest / pPriv->blockSize;
            if(length != 0) {
                // at least 1block.
                blockMode = SDIO_BLKMODE_ON;
                count     = (u16)MIN(0x1FF, length);
                length    = count * pPriv->blockSize;
            } else {
                blockMode = SDIO_BLKMODE_OFF;
                count     = (u16)MIN(512, (rest % pPriv->blockSize));
                length    = count;
                count     = count % 512; // 0 means 512 byte.
            }
            retval = BUS_sdioExecCmd53(pSdDev,
                                       dir,
                                       blockMode,
                                       SDIO_OPCODE_INC_ADDR,
                                       pRegCtrl->addr + pos,
                                       count,
                                       (void *)((u8 *)pRegCtrl->pData + pos),
                                       (u8 *)pRegCtrl->pStatus);
            if(retval == FALSE) {
                DBG_ERR("exec cmd53 failed.\n");
                break;
            }
            pos  += length;
            rest -= length;
        }
    }

    if(retval == FALSE) {
        return ERR_SYSTEM;
    }

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : BUS_sdioRead
 *-----------------------------------------------------------------*/
/**
 * read chunk of data from SDIO device
 * @param  pCmnDev    : pointer to the bus common device.
 * @param  addr       : address of the access point of device(REGADDR).
 * @param  length     : data length.
 * @param  pData      : pointer to the data buffer.
 * @param  pStatus    : return pointer to the status
 * @return SUCCESS     (normally completion)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUS_sdioRead(S_BUSCMN_DEV *pCmnDev,
             u32           addr,
             u32           length,
             void         *pData,
             void         *pStatus)
{

    int                   retval = 0;

    S_SDIO_PRIV          *pPriv;
    struct sdcard_device *pSdDev;

    u32                   len;
    u32                   rest;
    u32                   addrPos;
    u32                   bufPos;
    u16                   count;
    u8                    blockMode;
    u8                    opCode;

    pSdDev  = (struct sdcard_device *)pCmnDev->pDev;
    pPriv   = DEV_TO_PRIV(pCmnDev);

    opCode  = (pPriv->dmaAddrMode == 0) ? SDIO_OPCODE_FIXED_ADDR : SDIO_OPCODE_INC_ADDR;
    rest    = length;
    addrPos = 0;
    bufPos  = 0;

    while(rest > 0) {
        len = rest / pPriv->blockSize;
        if(len != 0) {
            // at least 1block.
            blockMode = SDIO_BLKMODE_ON;
            count     = (u16)MIN(0x1FF, len);
            len       = count * pPriv->blockSize;
        } else {
            blockMode = SDIO_BLKMODE_OFF;
            count     = (u16)MIN(512, (rest % pPriv->blockSize));
            len       = count;
            count     = count % 512; // 0 means 512 byte.
        }
        retval = BUS_sdioExecCmd53(pSdDev,
                                   SDIO_DIR_IN,
                                   blockMode,
                                   opCode,
                                   addr + addrPos,
                                   count,
                                   (void *)((u8 *)pData + bufPos),
                                   (u8 *)pStatus);
        if(retval == FALSE) {
            DBG_ERR("exec cmd53 failed.\n");
            break;
        }
        if(opCode != SDIO_OPCODE_FIXED_ADDR) {
            addrPos  += len;
        }
        bufPos += len;
        rest   -= len;
    }

    if(retval == FALSE) {
        return ERR_SYSTEM;
    }

    return SUCCESS;

}


/*-------------------------------------------------------------------
 * Function : BUS_sdioWrite
 *-----------------------------------------------------------------*/
/**
 * write chunk of data to SDIO device
 * @param  pCmnDev    : pointer to the bus common device.
 * @param  addr       : address of the access point of device(REGADDR).
 * @param  length     : data length.
 * @param  pData      : pointer to the data buffer.
 * @param  pStatus    : return pointer to the status
 * @return SUCCESS     (normally completion)
 * @return ERR_SYSTEM  (system error)
 * @note   
 */
/*-----------------------------------------------------------------*/
T_CMN_ERR 
BUS_sdioWrite(S_BUSCMN_DEV *pCmnDev,
              u32           addr,
              u32           length,
              void         *pData,
              void         *pStatus)
{

    int                   retval = 0;

    S_SDIO_PRIV          *pPriv;
    struct sdcard_device *pSdDev;

    u32                   len;
    u32                   rest;
    u32                   addrPos;
    u32                   bufPos;
    u16                   count;
    u8                    blockMode;
    u8                    opCode;

    pSdDev  = (struct sdcard_device *)pCmnDev->pDev;
    pPriv   = DEV_TO_PRIV(pCmnDev);

    opCode  = (pPriv->dmaAddrMode == 0) ? SDIO_OPCODE_FIXED_ADDR : SDIO_OPCODE_INC_ADDR;
    rest    = length;
    addrPos = 0;
    bufPos  = 0;

    while(rest > 0) {
        len = rest / pPriv->blockSize;
        if(len != 0) {
            // at least 1block.
            blockMode = SDIO_BLKMODE_ON;
            count     = (u16)MIN(0x1FF, len);
            len       = count * pPriv->blockSize;
        } else {
            blockMode = SDIO_BLKMODE_OFF;
            count     = (u16)MIN(512, (rest % pPriv->blockSize));
            len       = count;
            count     = count % 512; // 0 means 512 byte.
        }
        retval = BUS_sdioExecCmd53(pSdDev,
                                   SDIO_DIR_OUT,
                                   blockMode,
                                   opCode,
                                   addr + addrPos,
                                   count,
                                   (void *)((u8 *)pData + bufPos),
                                   (u8 *)pStatus);
        if(retval == FALSE) {
            DBG_ERR("exec cmd53 failed.\n");
            break;
        }
        if(opCode != SDIO_OPCODE_FIXED_ADDR) {
            addrPos  += len;
        }
        bufPos += len;
        rest   -= len;
    }

    if(retval == FALSE) {
        return ERR_SYSTEM;
    }

    return SUCCESS;

}
