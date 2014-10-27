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
 *  @file     cnlio_fit.c
 *
 *  @brief    Describe I/O to access/control CNL fitting driver for IZAN.
 *
 *
 *  @note
 */
/*=================================================================*/

#include <linux/types.h>
#include <linux/version.h>

#include <linux/module.h>
#include <linux/init.h>

#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/compat.h>

#include <linux/poll.h>
#include <linux/wait.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list.h>


#include "cmn_type.h"
#include "cmn_dbg.h"
#include "cmn_err.h"
#include "cnlfit.h"
#include "cnlfit_upif.h"
#include "cnlwrap_if.h"


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief the character device setting
 */
#define CNLIO_CTRL_MAJOR_NO            0
#define CNLIO_CTRL_MINOR_NO            0
#define CNLIO_ADPT_MAJOR_NO            0
#define CNLIO_ADPT_MINOR_NO            0

#define CNLIO_CTRL_MAX_CHANNEL         1
#define CNLIO_ADPT_MAX_CHANNEL         1

#define PADDING_4B(x)                  ((x + 3) & ~0x03)

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.1";


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/
/**
 * @brief CNL IO argument bucket management box
 */
typedef struct tag_S_CNLIO_ARG_BOX     {
    // list head
    struct list_head                   elm;
    // indicator to be able to free or not to
    uint                               free;
    // command relative to this box
    uint                               cmd;
    // the pointer of user argument
    void                              *puarg;
    // the pointer of user buffer
    void                              *pusr;

    // the pointer to arg included in this box
    void                              *parg;

    // argument itself
    S_CNLIO_ARG_BUCKET                 arg;

    struct semaphore                   sema;

} S_CNLIO_ARG_BOX;


/**
 * @brief CNL IO structure for control
 */
typedef struct tagS_CNLIO_FIT_PRIV    {
    // device information
    int                                id;
    int                                type;

    // lock
    struct semaphore                   fitSem;
    // poll method
    wait_queue_head_t                  waitEvt;

    // list head
    struct list_head                   async;

    // CNL control information for fitting module.
    void                              *pInfo;

} S_CNLIO_FIT_PRIV;


/*-------------------------------------------------------------------
 * Global Definitions
 *-----------------------------------------------------------------*/


static dev_t                           g_ctrlDevNo;
static dev_t                           g_adptDevNo;

static struct cdev                     g_ctrlChar;
static struct cdev                     g_adptChar;

/* auto node creation */
struct class				*g_ctrlClass;
struct class				*g_adptClass;

/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/
/**
 * @brief control exclusively
 */
static inline void
ioctl_lock(struct semaphore *sem)
{
    while(1)
    {
        if (down_interruptible(sem))
            continue;
        break;
    }
    return;
}

static inline void
ioctl_unlock(struct semaphore *sem)
{
    up(sem);

    return;
}

static inline void
CNLIO_FIT_LOCK(S_CNLIO_FIT_PRIV *pPriv)
{
    while(1)
    {
        if (down_interruptible(&pPriv->fitSem))
            continue;

        break;
    }


    return;
}


static inline void
CNLIO_FIT_UNLOCK(S_CNLIO_FIT_PRIV *pPriv)
{
    up(&pPriv->fitSem);


    return;
}


static inline int
CNLIO_cmnErrToSysErr(T_CMN_ERR cmnErr) {
    switch(cmnErr) {
    case SUCCESS :
        return 0;

    case ERR_NOMEM :
        return -ENOMEM;

    case ERR_BADPARM :
    case ERR_INVSTAT :
    case ERR_NOOBJ :
    case ERR_QOVR : 
    default :
        return -EINVAL;
    }
}


static inline S_CNLWRAP_STATUS
CNLIO_fitArgToStatus(int cmd, S_CNLIO_ARG_BUCKET *pArg)
{
    switch(cmd) {
    case CNLWRAPIOC_INIT :
        return pArg->req.init.status;
    case CNLWRAPIOC_CONNECT :
        return pArg->req.connect.status;
    case CNLWRAPIOC_ACCEPT :
        return pArg->req.accept.status;
    case CNLWRAPIOC_RELEASE :
        return pArg->req.release.status;
    case CNLWRAPIOC_SENDDATA :
    case CNLWRAPIOC_RECVDATA :
        return pArg->req.data.status;
    case CNLWRAPIOC_CANCEL:
        return pArg->req.cancel.status;
    case CNLWRAPIOC_CLOSE :
    case CNLWRAPIOC_WAIT_CONNECT :
    case CNLWRAPIOC_CONFIRM :
        return pArg->req.status;
    case CNLWRAPIOC_GETSTATS :
        return pArg->req.stats.status;

    case CNLWRAPIOC_POWERSAVE:
        return pArg->req.powersave.status;

    case CNLWRAPIOC_GETEVENT :
    case CNLWRAPIOC_SYNCRECV :
    case CNLWRAPIOC_STOP_EVENT :
    default :
        return 0;
    }
}


static inline int
CNLIO_fitGetInParamLength(int cmd)
{
    switch(cmd) {
    case CNLWRAPIOC_INIT :
        return sizeof(S_CNLWRAP_REQ_INIT);

    case CNLWRAPIOC_CONNECT :
        return sizeof(S_CNLWRAP_REQ_CONNECT);

    case CNLWRAPIOC_ACCEPT :
        return sizeof(S_CNLWRAP_REQ_ACCEPT);
        break;

    case CNLWRAPIOC_RELEASE :
        return sizeof(S_CNLWRAP_REQ_RELEASE);
        break;

    case CNLWRAPIOC_SENDDATA :
    case CNLWRAPIOC_RECVDATA :
        return sizeof(S_CNLWRAP_REQ_DATA);
        break;

    case CNLWRAPIOC_CANCEL:
        return sizeof(S_CNLWRAP_REQ_CANCEL);
        break;

    case CNLWRAPIOC_POWERSAVE:
        return sizeof(S_CNLWRAP_REQ_POWERSAVE);
        break;

    case CNLWRAPIOC_CLOSE :
    case CNLWRAPIOC_WAIT_CONNECT :
    case CNLWRAPIOC_CONFIRM :
    case CNLWRAPIOC_GETEVENT :
    case CNLWRAPIOC_SYNCRECV :
    case CNLWRAPIOC_STOP_EVENT :
    case CNLWRAPIOC_ENABLE_PORT :
    case CNLWRAPIOC_DISABLE_PORT :
    case CNLWRAPIOC_SUSPEND_CONF :
    case CNLWRAPIOC_GET_ADPT_ID :
    case CNLWRAPIOC_GETSTATS :
    default :
        return 0;
    }
}


static inline int
CNLIO_fitGetOutParamLength(int cmd)
{
    switch (cmd) {
    case CNLWRAPIOC_INIT :
        return sizeof(S_CNLWRAP_REQ_INIT);

    case CNLWRAPIOC_CONNECT :
        return sizeof(S_CNLWRAP_REQ_CONNECT);

    case CNLWRAPIOC_ACCEPT :
        return sizeof(S_CNLWRAP_REQ_ACCEPT);

    case CNLWRAPIOC_RELEASE :
        return sizeof(S_CNLWRAP_REQ_RELEASE);

    case CNLWRAPIOC_SENDDATA:
    case CNLWRAPIOC_RECVDATA:
        return sizeof(S_CNLWRAP_REQ_DATA);

    case CNLWRAPIOC_CANCEL:
        return sizeof(S_CNLWRAP_REQ_CANCEL);

    case CNLWRAPIOC_GETEVENT:
        return sizeof(S_CNLWRAP_EVENT);

    case CNLWRAPIOC_CLOSE :
    case CNLWRAPIOC_WAIT_CONNECT :
    case CNLWRAPIOC_CONFIRM :
        // no OUT parameter expect status.
        return sizeof(S_CNLWRAP_STATUS);

    case CNLWRAPIOC_GET_ADPT_ID :
        return sizeof(u32);

    case CNLWRAPIOC_GETSTATS :
        return sizeof(S_CNLWRAP_STATS);

    case CNLWRAPIOC_POWERSAVE:
        return sizeof(S_CNLWRAP_REQ_POWERSAVE);

    case CNLWRAPIOC_ENABLE_PORT :
    case CNLWRAPIOC_DISABLE_PORT :
    case CNLWRAPIOC_SYNCRECV :
    case CNLWRAPIOC_STOP_EVENT :
    case CNLWRAPIOC_SUSPEND_CONF :
    default :
        // no OUT parameter.
        return 0;
    }
}


/*-------------------------------------------------------------------
 * Prototype Functions
 *-----------------------------------------------------------------*/
static int              CNLIO_fitOpen(struct inode *, struct file  *);
static int              CNLIO_fitRelease(struct inode *, struct file *);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int              CNLIO_fitIoctl(struct inode *, struct file *, uint, ulong);
#else
static long             CNLIO_fitIoctl(struct file *, uint, ulong);
#if defined(_LP64) == 1
static long             CNLIO_compat_fitIoctl(struct file *, uint, ulong);
#endif
#endif
static uint             CNLIO_fitPoll(struct file *, struct poll_table_struct *);

static void             CNLIO_fitNotifyEvent   (void *);
static int              CNLIO_fitIoctlPreProc  (S_CNLIO_FIT_PRIV *, uint, void *, S_CNLIO_ARG_BOX *);
static int              CNLIO_fitIoctlPostProc (S_CNLIO_FIT_PRIV *, S_CNLIO_ARG_BOX *, int);
static int              CNLIO_fitCmdInitializer(S_CNLIO_FIT_PRIV *, S_CNLIO_ARG_BOX *);
static int              CNLIO_fitCmdFinisher   (S_CNLIO_FIT_PRIV *, S_CNLIO_ARG_BOX *, int);
static S_CNLIO_ARG_BOX *CNLIO_fitSearchAsyncBox(S_CNLIO_FIT_PRIV *, ulong);

static int              CNLIO_fitArgBucketPurge(S_CNLIO_FIT_PRIV *);

/*===================================================================
 *                   Character Device interface
 *==================================================================*/
/*-------------------------------------------------------------------
 * Function   : CNLIO_fitOpen
 *-----------------------------------------------------------------*/
/**
 * method that opens the file of character device.
 * @param     pInode    : the pointer to the node structure
 * @param     pFile     : the pointer to the file structure
 * @return    0           (sucess)
 * @return    -ENODEV     (there is no device)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitOpen(struct inode *pInode,
              struct file  *pFile)
{

    int                 retval = 0;
    int                 id;
    int                 type;
    S_CNLIO_FIT_PRIV   *pfitPriv = NULL;
    S_CNLIO_FIT_CBKS    cbks;

    int                 major;
    int                 minor;

    major = MAJOR(pInode->i_rdev);
    minor = MINOR(pInode->i_rdev);

    if(major == MAJOR(g_ctrlDevNo)) {
        DBG_INFO("Open Controller module.\n");
        type = CNLFIT_DEVTYPE_CTRL;
        id   = minor - CNLIO_CTRL_MINOR_NO;
    } else {
        DBG_INFO("Open Adapter module.\n");
        type = CNLFIT_DEVTYPE_ADPT;
        id   = minor - CNLIO_ADPT_MINOR_NO;
    }

    DBG_INFO("IOFIT open : major=%d, minor=%d.\n", major, minor);

    // allocate the private data of the CNL IO
    pfitPriv = kmalloc(sizeof(S_CNLIO_FIT_PRIV), GFP_KERNEL);
    if(pfitPriv == NULL) {
        DBG_ERR("allocate FitPriv failed.\n");
        return -ENOMEM;
    }

    //
    // 1. initialize S_CNL_PRIV structure
    //
    {
        // zero clear
        memset(pfitPriv, 0, sizeof(S_CNLIO_FIT_PRIV));

        // initialize parameters
        pfitPriv->id   = id;
        pfitPriv->type = type;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36) 
        init_MUTEX(&pfitPriv->fitSem);
#else 
        sema_init(&pfitPriv->fitSem,1);
#endif
        init_waitqueue_head(&pfitPriv->waitEvt);

        INIT_LIST_HEAD(&pfitPriv->async);

        // prepare for callbacks
        cbks.event.pioCbk = CNLIO_fitNotifyEvent;
        cbks.event.pioArg = pfitPriv;

        // open device
        retval = CNLFIT_open(type, id, &pfitPriv->pInfo, &cbks);
        if(retval != SUCCESS) {
            kfree(pfitPriv);

            retval = -ENODEV;
            goto EXIT;
        }
    }


    //
    // 2. each setting to open
    //

    // set the private data to file
    pFile->private_data = pfitPriv;


EXIT:
    return retval;
}

/*-------------------------------------------------------------------
 * Function   : CNLIO_fitRelease
 *-----------------------------------------------------------------*/
/**
 * method that releases the file of character device.
 * @param     pInode    : the pointer to the node structure
 * @param     pFile     : the pointer to the file structure
 * @return    0           (success)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitRelease(struct inode *pInode,
                 struct file  *pFile)
{
    int                 retval = 0;
    S_CNLIO_FIT_PRIV  *pfitPriv = NULL;

    DBG_INFO("CNLIO_fitRelaese called.\n");

    // get the private data of the CNL IO
    pfitPriv = (S_CNLIO_FIT_PRIV *)pFile->private_data;

    // deallocate arg box
    retval = CNLIO_fitArgBucketPurge(pfitPriv);
    if(retval != SUCCESS){
        goto EXIT;
    }

    retval = CNLFIT_close(pfitPriv->type, pfitPriv->id, pfitPriv->pInfo);

    if(retval != SUCCESS) {
        retval = -EIO;
        goto EXIT;
    }

    // reset the private data to file
    pFile->private_data = NULL;

    // free the private data of the CNL IO
    kfree(pfitPriv);

EXIT:
    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitPoll
 *-----------------------------------------------------------------*/
/**
 * method that notifies whether there is event or not
 * @param     pFile     : the pointer to the file structure
 * @param     pWait     : the pointer to the poll_table_struct structure
 * @return    bit mask for poll
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static uint
CNLIO_fitPoll(struct file              *pFile,
              struct poll_table_struct *pWait)
{

    T_CMN_ERR ret = 0;
    int       pollBitMask = 0;

    S_CNLIO_FIT_PRIV  *pfitPriv = NULL;


    // get the private data of the TJet driver
    pfitPriv = (S_CNLIO_FIT_PRIV *)pFile->private_data;

    // wait event
    poll_wait(pFile, &pfitPriv->waitEvt, pWait);

    // if event is found, set bit to mask
    ret = CNLFIT_searchEvent(pfitPriv->type, pfitPriv->pInfo);
    switch(ret) {
    case SUCCESS :
        pollBitMask = (POLLIN | POLLRDNORM);
        break;
    case ERR_INVSTAT :
        pollBitMask = (POLLHUP);
        break;
    case ERR_NOOBJ :
    default :
        break;
    }

    return pollBitMask;
}

/*-------------------------------------------------------------------
 * Function   : CNLIO_fitIoCtl
 *-----------------------------------------------------------------*/
/**
 * method that controls the device specific functions.
 * @param     pInode    : the pointer to the node structure
 * @param     pFile     : the pointer to the file structure
 * @param     cmd       : the command  of ioctl
 * @param     arg       : the argument of ioctl
 * @return    0           (success)
 * @return    -EBUSY      (device busy)
 * @return    -ENOMEM     (out of memory)
 * @return    -EFALUT     (bad address)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int
CNLIO_fitIoctl(struct inode *pInode,
               struct file  *pFile,
               uint          cmd,
               ulong         arg)
#else
static long
CNLIO_fitIoctl(struct file  *pFile,
               uint          cmd,
               ulong         arg)
#endif
{
    int                 retval   = 0;
    T_CMN_ERR           status;

    S_CNLIO_FIT_PRIV  *pfitPriv = NULL;
    S_CNLIO_ARG_BOX    *pBox     = NULL;
    int                 free;


    // get the private data of the Jet driver
    pfitPriv = (S_CNLIO_FIT_PRIV *)pFile->private_data;

    //
    // allocate the memory for the argument
    //
    pBox = kmalloc(sizeof(S_CNLIO_ARG_BOX), GFP_KERNEL);

    if(pBox == NULL) {
        retval = -ENOMEM;
        goto EXIT;
    }


    //
    // ioctl command pre-processing function
    //
    retval = CNLIO_fitIoctlPreProc(pfitPriv, cmd,  (void *)arg, pBox);
    if(retval != 0) {
        pBox->free = TRUE;
        goto EXIT;
    }


    //
    // handle ioctl command
    //
    status = CNLFIT_ctrl(pfitPriv->type, pfitPriv->pInfo, cmd, pBox->parg);
    if(status != SUCCESS) {
        retval = CNLIO_cmnErrToSysErr(status);
    }

    //
    // ioctl command post processing function
    //
    retval = CNLIO_fitIoctlPostProc(pfitPriv, pBox, retval);
    if(retval != 0) {
        goto EXIT;
    }

    free = pBox->free;
    ioctl_unlock(&pBox->sema);
    if (free == TRUE) {
        kfree(pBox);
    }

    return 0;

EXIT:
    // if it is possible to deallocate the buffer, free it.
    if(pBox) {
        if(pBox->free)  kfree(pBox);
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitNotifyEvent
 *-----------------------------------------------------------------*/
/**
 * function that release the poll wait.
 * @param     pArg  : the argument to input to this function
 * @return    nothing
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static void
CNLIO_fitNotifyEvent(void *pArg)
{
    S_CNLIO_FIT_PRIV  *pfitPriv = NULL;

    DBG_INFO("NotifyEvent : called.\n");

    pfitPriv = (S_CNLIO_FIT_PRIV *)pArg;

    // wake poll
    wake_up_interruptible(&pfitPriv->waitEvt);

    return;
}

/*-------------------------------------------------------------------
 * Function   : CNLIO_fitIoctlPreProc
 *-----------------------------------------------------------------*/
/**
 * helper function that pre-processes ioctl command.
 * @param     pcioPriv  : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @param     cmd       : the command of ioctl
 * @param     puArg     : the pointer to the S_CNLIO_ARG_BUCKET structure
 * @param     pBox      : the pointer to the S_CNLIO_ARG_BOX structure
 * @return    0           (success)
 * @return    -EFALUT     (bad address)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitIoctlPreProc(S_CNLIO_FIT_PRIV *pfitPriv,
                      uint              cmd,
                      void             *puArg,
                      S_CNLIO_ARG_BOX  *pBox)
{
    int                 retval  = 0;
    int                 length  = 0;
    S_CNLIO_ARG_BUCKET *pArg = NULL;


    //
    // copy the user argument to the kernel argument in the BOX
    //
    memset(pBox, 0, sizeof(S_CNLIO_ARG_BOX));

    sema_init(&pBox->sema, 1);
    ioctl_lock(&pBox->sema);

    // check IN parameter length.
    length = CNLIO_fitGetInParamLength(cmd);

    if(length > 0) {
        if(copy_from_user(&pBox->arg.req, (void *)puArg, length)) {
            retval = -EFAULT;
            goto EXIT;
        }
    }


    //
    // initilize S_CNLIO_ARG_BOX member
    //
    pArg = &pBox->arg;

    INIT_LIST_HEAD(&pBox->elm);
    pBox->cmd    = cmd;
    pBox->puarg  = puArg;
    pBox->parg   = (void *)pArg;

    retval = CNLIO_fitCmdInitializer(pfitPriv, pBox);


EXIT:
    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitIoctlPostProc
 *-----------------------------------------------------------------*/
/**
 * helper function that post-processes ioctl command.
 * @param     pcioPriv  : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @param     pBox      : the pointer to the S_CNLIO_ARG_BOX structure
 * @param     errFlag   : the error status about the command to finish
 * @return    0           (success)
 * @return    -EFALUT     (bad address)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitIoctlPostProc(S_CNLIO_FIT_PRIV *pfitPriv,
                       S_CNLIO_ARG_BOX  *pBox,
                       int               errFlag)
{
    int   retval  = 0;

    S_CNLIO_ARG_BUCKET  *pArg       = NULL;

    S_CNLWRAP_STATUS     status;
    
    pArg = pBox->parg;

    status = CNLIO_fitArgToStatus(pBox->cmd, pArg);

    if(errFlag) {
        retval = CNLIO_fitCmdFinisher(pfitPriv, pBox, errFlag);
        retval = errFlag;
        goto EXIT;
    }

    if(status != CNL_SUCCESS) {
        retval = CNLIO_fitCmdFinisher(pfitPriv, pBox, status);
        goto EXIT;
    }

    if((pBox->cmd == CNLWRAPIOC_SENDDATA) || (pBox->cmd == CNLWRAPIOC_RECVDATA)) {
        //
        // SENDDATA/RECVDATA request is Async request.
        // return only status.
        //
        pArg->req.data.status = CNLWRAP_REQ_PENDING;
    }

    //
    // if the event is the completion of the async request
    // it is necessary to finish the async request at first.
    //
    if(pBox->cmd == CNLWRAPIOC_GETEVENT) {

        S_CNLWRAP_EVENT  *pevent;
        S_CNLIO_ARG_BOX  *pListedBox = NULL;

        pevent = &pBox->arg.req.event;

        if(pevent->type == CNLWRAP_EVENT_DATA_REQ_COMP) {

            // search the async command
            pListedBox =
                CNLIO_fitSearchAsyncBox(pfitPriv, 
                                         pevent->dataReqComp.requestId);
        }

        // if found
        if(pListedBox) {
            void *puBuf;
            void *pnBuf;

            ioctl_lock(&pListedBox->sema);

            puBuf = (void *)pListedBox->pusr;
            pnBuf = (void *)pListedBox->arg.req.data.userBufAddr;

            if((pListedBox->cmd == CNLWRAPIOC_RECVDATA) &&
               (pevent->dataReqComp.status == CNL_SUCCESS)) {
                //
                // Async receive request normally completed.
                // copy back the received data to user buffer.
                //
                DBG_ASSERT((ulong)pListedBox->pusr == pevent->dataReqComp.requestId);
                if(copy_to_user(puBuf, pnBuf, pevent->dataReqComp.length)) {
                    DBG_ASSERT(0);
                    retval = -EFAULT;
                }
            }
            if(pnBuf)
                kfree(pnBuf);

            ioctl_unlock(&pListedBox->sema);

            kfree(pListedBox);
        }
    }
    
    retval = CNLIO_fitCmdFinisher(pfitPriv, pBox, status);

EXIT:
    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitCmdInitializer
 *-----------------------------------------------------------------*/
/**
 * utility function that initializes the command.
 * @param     pcioPriv  : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @param     pBox      : the pointer to the S_CNLIO_ARG_BOX structure
 * @return    0           (success)
 * @return    -EFALUT     (bad address)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitCmdInitializer(S_CNLIO_FIT_PRIV *pfitPriv,
                        S_CNLIO_ARG_BOX  *pBox)
{
    int  retval  = 0;
    u32  ubufLen = 0;
    u32  copyLen = 0;

    S_CNLIO_ARG_BUCKET  *pArg   = NULL;
    S_CNLWRAP_REQ_DATA  *pdataReq;
    void                *puBuf  = NULL;
    void                *pnBuf  = NULL;
    void               **ppData = NULL;


    pArg = pBox->parg;


    //
    // execute pre-processing if needed.
    // this prosess depends on each command.
    //
    switch (pBox->cmd) {
    case CNLWRAPIOC_SENDDATA:
    case CNLWRAPIOC_RECVDATA:
        pdataReq = &pArg->req.data;

        if((pdataReq->length == 0) || ((void *)pdataReq->userBufAddr == NULL))
           break;  // because these are checked in lower moudule.

        puBuf   = (void *)pdataReq->userBufAddr;
        ppData  = (void **)(&pdataReq->userBufAddr);

        ubufLen =  PADDING_4B(pdataReq->length);

        if(pBox->cmd == CNLWRAPIOC_SENDDATA)
            copyLen = pdataReq->length;

        break;
    default:
        break;
    }


    if(puBuf) {
        pnBuf = kmalloc(ubufLen, GFP_KERNEL);

        if(pnBuf == NULL) {
            retval = -ENOMEM;
            goto EXIT;
        }


        if(copyLen) {
            if(copy_from_user(pnBuf, puBuf, copyLen)) {
                retval = -EFAULT;
                goto EXIT;
            }
        }

        // exchange the pointer to the memroy between user and kernel
        pBox->pusr = puBuf;
        *ppData    = pnBuf;

         CNLIO_FIT_LOCK(pfitPriv);
         list_add_tail(&pBox->elm, &pfitPriv->async);
         CNLIO_FIT_UNLOCK(pfitPriv);
    }

EXIT:
    if(retval != 0) {
        if(pnBuf)  kfree(pnBuf);
    }

    return retval;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitCmdFinisher
 *-----------------------------------------------------------------*/
/**
 * utility function that completes the command.
 * @param     pcioPriv  : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @param     pBox      : the pointer to the S_CNLIO_ARG_BOX structure
 * @param     errFlag   : the error status about the command to finish
 * @return    0           (success)
 * @return    -EFALUT     (bad address)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitCmdFinisher(S_CNLIO_FIT_PRIV *pfitPriv,
                     S_CNLIO_ARG_BOX  *pBox,
                     int               errFlag)
{
    int  retval  = 0;
    int  length;

    S_CNLIO_ARG_BUCKET  *pArg   = NULL;
    void                *pnBuf  = NULL;

    pArg = pBox->parg;

    //
    // execute post-processing if needed.
    // this prosess depends on each command.
    //
    length = CNLIO_fitGetOutParamLength(pBox->cmd);

    if(length > 0) {
        if(copy_to_user(pBox->puarg, (void *)&(pArg->req), length)) {
            retval = -EFAULT;
        }
    }

    if((pBox->cmd == CNLWRAPIOC_SENDDATA) || (pBox->cmd == CNLWRAPIOC_RECVDATA)) {
        if(errFlag) {
            DBG_INFO("DataRequest failed, remove from async queue and free data buffer.\n");

            CNLIO_FIT_LOCK(pfitPriv);
            list_del(&pBox->elm);
            CNLIO_FIT_UNLOCK(pfitPriv);

            pnBuf = (void *)pBox->arg.req.data.userBufAddr;

            kfree(pnBuf);
            pBox->free = TRUE;
        } else {
            // success to request.
            pBox->free = FALSE;
        }
    } else {
        // 
        // except DataRequest, always free pBox.
        //
        pBox->free = TRUE;
    }


    return retval;
}

#if (KERNEL_VERSION(2,6,36) <= LINUX_VERSION_CODE) && (defined(_LP64) == 1)
/*-------------------------------------------------------------------
 * Function   : CNLIO_compat_fitIoctl
 *-----------------------------------------------------------------*/
static long
CNLIO_compat_fitIoctl(struct file * pFile,
                      uint          cmd,
                      ulong         arg)
{
    long retval = -EINVAL;
    void __user * arg32 = compat_ptr(arg);

    DBG_INFO("CNLIO_compat_fitIoctl(cmd=%s)+\n", CNLFIT_cmdToString(cmd));
    DBG_INFO("#####CNLIO_compat_fitIoctl(cmd=%x)+\n", cmd);

    switch (cmd) {
    case CNLWRAPIOC_SENDDATA:
    case CNLWRAPIOC_RECVDATA: {
        S_CNLWRAP32_REQ_DATA req32;
        S_CNLWRAP_REQ_DATA req64;
        S_CNLWRAP_REQ_DATA __user * arg64;

        arg64 = compat_alloc_user_space(sizeof *arg64);

        if (copy_from_user(&req32, arg32, sizeof req32)) {
            retval = -EFAULT;
            break;
        }

        req64.profileId = req32.profileId;
        req64.fragmented = req32.fragmented;
        req64.length = req32.length;
        req64.userBufAddr = compat_ptr(req32.userBufAddr);
        req64.sync = req32.sync;
        req64.requestId = (ulong)compat_ptr(req32.requestId);

        if (copy_to_user(arg64, &req64, sizeof req64)) {
            retval = -EFAULT;
            break;
        }

        retval = CNLIO_fitIoctl(pFile, cmd, (ulong)arg64);

        if (copy_from_user(&req64, arg64, sizeof req64)) {
            retval = -EFAULT;
            break;
        }

        req32.status = req64.status;

        if (copy_to_user(arg32, &req32, sizeof req32)) {
            retval = -EFAULT;
            break;
        }

        }
        break;

    case CNLWRAPIOC_GETEVENT: {
        void * src32;
        S_CNLWRAP32_EVENT req32;
        S_CNLWRAP_EVENT req64;
        S_CNLWRAP_EVENT __user * arg64;

        arg64 = compat_alloc_user_space(sizeof *arg64);

        retval = CNLIO_fitIoctl(pFile, cmd, (ulong)arg64);

        if (copy_from_user(&req64, arg64, sizeof req64)) {
            retval = -EFAULT;
            break;
        }

        if (CNLWRAP_EVENT_DATA_REQ_COMP == req64.type) {
            req32.type = req64.type;
            req32.length = req64.length;
            req32.dataReqComp.status = req64.dataReqComp.status;
            req32.dataReqComp.requestId = (u32)req64.dataReqComp.requestId;
            req32.dataReqComp.profileId = req64.dataReqComp.profileId;
            req32.dataReqComp.direction = req64.dataReqComp.direction;
            req32.dataReqComp.fragmented = req64.dataReqComp.fragmented;
            req32.dataReqComp.length = req64.dataReqComp.length;

            src32 = &req32;
        } else
            src32 = &req64;

        if (copy_to_user(arg32, src32, sizeof req32)) {
            retval = -EFAULT;
            break;
        }

        }
        break;

    case CNLWRAPIOC_CANCEL: {
        S_CNLWRAP32_REQ_CANCEL req32;
        S_CNLWRAP_REQ_CANCEL req64;
        S_CNLWRAP_REQ_CANCEL __user * arg64;

        arg64 = compat_alloc_user_space(sizeof *arg64);

        if (copy_from_user(&req32, arg32, sizeof req32)) {
            retval = -EFAULT;
            break;
        }

        req64.requestId = (req32.requestId == (u32)CNL_DISCARD_RECVDATA_1)
                        ? CNL_DISCARD_RECVDATA_1
                        : (ulong)compat_ptr(req32.requestId);

        if (copy_to_user(arg64, &req64, sizeof req64)) {
            retval = -EFAULT;
            break;
        }

        retval = CNLIO_fitIoctl(pFile, cmd, (ulong)arg64);

        if (copy_from_user(&req64, arg64, sizeof req64)) {
            retval = -EFAULT;
            break;
        }

        req32.status = req64.status;

        if (copy_to_user(arg32, &req32, sizeof req32)) {
            retval = -EFAULT;
            break;
        }

        }
        break;

    default:
        retval = CNLIO_fitIoctl(pFile, cmd, (ulong)arg32);
        break;
    }

    DBG_INFO("CNLIO_compat_fitIoctl(cmd=%s)- %s\n", CNLFIT_cmdToString(cmd)
        , retval ? "failed" : "success");

    return retval;
}
#endif



/*-------------------------------------------------------------------
 * Function   : CNLIO_fitSearchAsyncBox
 *-----------------------------------------------------------------*/
/**
 * utility function that searches the specifed argument box, and if the
 * specified argument box is found, removes it from list.
 * @param     pcioPriv   : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @param     requestId  : requestId
 * @return    the pointer to the S_CNLIO_ARG_BOX structure
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static S_CNLIO_ARG_BOX *
CNLIO_fitSearchAsyncBox(S_CNLIO_FIT_PRIV *pfitPriv,
                        ulong             requestId)
{
    u8                  find = FALSE;
    S_CNLIO_ARG_BOX    *pBox = NULL;
    S_CNLWRAP_REQ_DATA *pdataReq;


    CNLIO_FIT_LOCK(pfitPriv);


    list_for_each_entry(pBox, &pfitPriv->async, elm)
    {
        pdataReq = &pBox->arg.req.data;
        if(pdataReq->requestId == requestId) {
            find = TRUE;
            break;
        }
    }


    if(find) {
        list_del(&pBox->elm);
    } else {
        pBox = NULL;
    }


    CNLIO_FIT_UNLOCK(pfitPriv);


    return pBox;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitArgBucketPurge
 *-----------------------------------------------------------------*/
/**
 * utility function that searches the specifed argument box, and if the
 * specified argument box is found, delete it from list, and dealloate
 * argument box.
 * @param     pcioPriv   : the pointer to the S_CNLIO_CTRL_PRIV structure
 * @return    SUCCESS     (success)
 * @return    -ENOMEM     (error)
 * @note      nothing
 */
/*-----------------------------------------------------------------*/
static int
CNLIO_fitArgBucketPurge(S_CNLIO_FIT_PRIV *pfitPriv)
{
    int                 retval   = SUCCESS;
    S_CNLIO_ARG_BOX    *pBox     = NULL;
    S_CNLIO_ARG_BUCKET *pArg     = NULL;
    S_CNLWRAP_REQ_DATA *pdataReq = NULL;
    void               *pData    = NULL;


    if(pfitPriv == NULL){
        retval = -ENOMEM;
        goto EXIT;
    }

    CNLIO_FIT_LOCK(pfitPriv);

    while(!list_empty(&pfitPriv->async)){
        pBox = list_entry(pfitPriv->async.next, S_CNLIO_ARG_BOX, elm);
        pArg = pBox->parg;
        pdataReq = &pArg->req.data;
        pData = (void *)(pdataReq->userBufAddr);
        if(pData != NULL) {
            // deallocate data buffer
            DBG_INFO("(rel)kfree pData[%p]\n",pData);
            kfree(pData);
        }
        // deallocate arg box
        list_del(&pBox->elm);
        DBG_INFO("(rel)kfree pBox[%p]\n", pBox);
        kfree(pBox);
    }

    CNLIO_FIT_UNLOCK(pfitPriv);

EXIT:
    return retval;

}


/*
 * @brief I/O methods structure global
 */
static const struct file_operations  g_ctrlOps = {
    .owner                             = THIS_MODULE,
    .read                              = NULL,
    .write                             = NULL,
    .poll                              = CNLIO_fitPoll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
    .ioctl                             = CNLIO_fitIoctl,
#else
    .unlocked_ioctl                    = CNLIO_fitIoctl,
#if defined(_LP64) == 1
    .compat_ioctl                      = CNLIO_compat_fitIoctl,
#endif
#endif
    .open                              = CNLIO_fitOpen,
    .release                           = CNLIO_fitRelease,
};

/*===================================================================
 *                     load/unload CNLIO module
 *==================================================================*/

/*-------------------------------------------------------------------
 * Function   : CNLIO_fitInit
 *-----------------------------------------------------------------*/
/**
 * function to initialize called when module is loaded.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static int __init
CNLIO_fitInit(void)
{

    int  retval = 0;

    g_ctrlDevNo = CNLIO_CTRL_MAJOR_NO;
    retval = alloc_chrdev_region(&g_ctrlDevNo,
                                 CNLIO_CTRL_MINOR_NO,
                                 CNLIO_CTRL_MAX_CHANNEL,
                                 CNLWRAP_DEVICENAME);
    if(retval != 0){
        return -1;
    }

    cdev_init(&g_ctrlChar, &g_ctrlOps);
    g_ctrlChar.owner = THIS_MODULE;
    g_ctrlChar.ops   = &g_ctrlOps;

	g_ctrlClass = class_create(THIS_MODULE, "CnlFitCtrl");
	if (g_ctrlClass == NULL)
		goto ctrl_class_exit;

	/* auto udev node creation */
	if (device_create(g_ctrlClass, NULL, g_ctrlDevNo,
				NULL, "CnlFitCtrl0") == NULL) {
		goto ctrl_device_exit;
    }

    retval = cdev_add(&g_ctrlChar, g_ctrlDevNo, CNLIO_CTRL_MAX_CHANNEL);
    if(retval != 0){
		goto ctrl_exit;
    }

    DBG_INFO("add CTRL character device completed[major = %d].\n",
             MAJOR(g_ctrlDevNo));

    g_adptDevNo = CNLIO_ADPT_MAJOR_NO;
    retval = alloc_chrdev_region(&g_adptDevNo,
                                 CNLIO_ADPT_MINOR_NO,
                                 CNLIO_ADPT_MAX_CHANNEL,
                                 CNLWRAPADPT_DEVICENAME);
    if(retval != 0){
		goto adpt_cdev_exit;
    }

    cdev_init(&g_adptChar, &g_ctrlOps);
    g_adptChar.owner = THIS_MODULE;
    g_adptChar.ops   = &g_ctrlOps;

	g_adptClass = class_create(THIS_MODULE, "CnlFitAdpt");
	if (g_adptClass == NULL)
		goto adpt_class_exit;

	/* auto udev node creation */
	if (device_create(g_adptClass, NULL, g_adptDevNo,
				NULL, "CnlFitAdpt0") == NULL) {
		goto adpt_device_exit;
	}

    retval = cdev_add(&g_adptChar,
                      g_adptDevNo,
                      CNLIO_ADPT_MAX_CHANNEL);
    if(retval != 0){
		goto adpt_exit;
    }

    DBG_INFO("add ADPT character device completed[major = %d].\n",
             MAJOR(g_adptDevNo));

    return 0;

adpt_exit:
	device_destroy(g_adptClass, g_adptDevNo);
adpt_device_exit:
	class_destroy(g_adptClass);
adpt_class_exit:
	unregister_chrdev_region(g_adptDevNo, CNLIO_ADPT_MAX_CHANNEL);
adpt_cdev_exit:
	cdev_del(&g_ctrlChar);
ctrl_exit:
	device_destroy(g_ctrlClass, g_ctrlDevNo);
ctrl_device_exit:
	class_destroy(g_ctrlClass);
ctrl_class_exit:
	unregister_chrdev_region(g_ctrlDevNo, CNLIO_CTRL_MAX_CHANNEL);
	return -ENODEV;
}


/*-------------------------------------------------------------------
 * Function   : CNLIO_fitExit
 *-----------------------------------------------------------------*/
/**
 * function to cleanup callied when module is unloaded.
 * @param     nothing.
 * @return    nothing.
 * @note      nothing.
 */
/*-----------------------------------------------------------------*/
static void __exit
CNLIO_fitExit(void)
{

    // destroy node and class
	device_destroy(g_adptClass, g_adptDevNo);
	class_destroy(g_adptClass);
    DBG_INFO("destroy ADPT class and device completed.\n");

	device_destroy(g_ctrlClass, g_ctrlDevNo);
	class_destroy(g_ctrlClass);
    DBG_INFO("destroy CTRL class and device completed.\n");

    // unregister character device and driver
    cdev_del(&g_adptChar);
    unregister_chrdev_region(g_adptDevNo, CNLIO_ADPT_MAX_CHANNEL);
    DBG_INFO("delete ADPT character device completed.\n");

    cdev_del(&g_ctrlChar);
    unregister_chrdev_region(g_ctrlDevNo, CNLIO_CTRL_MAX_CHANNEL);
    DBG_INFO("delete CTRL character device completed.\n");

    return;
}


module_init( CNLIO_fitInit );
module_exit( CNLIO_fitExit );

MODULE_LICENSE("GPL v2");
MODULE_VERSION( DRIVER_VERSION );

