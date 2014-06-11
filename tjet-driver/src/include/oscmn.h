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
 *  @file     oscomn.h
 *
 *  @breif    Describes the data structures and the functions exported
 *            from OS common library.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__OSCMN_H__)
#define __OSCMN_H__

#include "cmn_type.h"
#include "cmn_err.h"

#if defined(USE_OS_LINUX)
#include "../os/linux/include/sys_base.h"
#endif



/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/**
 * @breif the macros to define the message priority
 */
#define  CMN_MSG_PRI_HIGH              1
#define  CMN_MSG_PRI_NORM              2
#define  CMN_MSG_PRI_LOW               3


/**
 * @breif the macros to define the special time
 */
#define  CMN_TIME_POLL                 0
#define  CMN_TIME_FEVR            0xFFFF


/*===================================================================
 * macros related to Semaphore
 *==================================================================*/
/**
 * @breif the macros to operate the semaphore as mutex
 */
#define CMN_INIT_MUTEX(id)             CMN_createSem(id, 0, 1, 1)
#define CMN_LOCK_MUTEX(id)             CMN_waitSem(id, CMN_TIME_FEVR)
#define CMN_UNLOCK_MUTEX(id)           CMN_signalSem(id)


/*
 * @breif the macros to operate the semaphore as syncronization betweeen tasks
 */
#define CMN_INIT_WAIT(id)              CMN_createSem(id, 0, 0, 1)
#define CMN_WAIT(id)                   CMN_waitSem(id, CMN_TIME_FEVR)
#define CMN_WAIT_TO(id, to)            CMN_waitSem(id, to)
#define CMN_REL_WAIT(id)               CMN_signalSem(id)


/*
 * @breif the macros to operate the semaphore as general semaphore
 */
#define CMN_INIT_SEM(id, init, max)    CMN_createSem(id, 0, init, max)
#define CMN_WAIT_SEM(id)               CMN_waitSem(id, CMN_TIME_FEVR)
#define CMN_POST_SEM(id)               CMN_signalSem(id)


/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/


/*===================================================================
 * the structures related to "Synchronization"
 *==================================================================*/
/**
 * @breif the structure to refer the status of semaphore
 */
typedef struct tagS_CMN_REF_SEM {
    u16                                semCount;
} S_CMN_REF_SEM;


/*===================================================================
 * the structures related to "Message"
 *==================================================================*/
/**
 * @breif message header
 */
typedef struct tagS_CMN_MSG {
    S_OS_MSG                           msgHdr;    // uITRON message header (and list structure for linux)
    u8                                 srcModID;  // the module ID of the sender
    u8                                 reserved;  // reserved
    u16                                type;      // the type of message
    u32                                length;    // the length of message
    u32                                userData;  // the user data
    void                              *pData;     // the pointer to user data
} S_CMN_MSG;

/**
 * @breif the structure to refer the status of message box
 */
typedef struct tagS_CMN_REF_MBX {
    S_CMN_MSG                         *pNextMsg;
} S_CMN_REF_MBX;


/*===================================================================
 * the structures related to "Memory"
 *==================================================================*/
/**
 * @breif the structure to refer the status of memory pool
 */
typedef struct tagS_CMN_REF_MPL {
    u16                                availMemCount;
    u16                                reserved;
} S_CMN_REF_MPL;


/*===================================================================
 * the structures related to "Task/Thread"
 *==================================================================*/
/**
 * @breif the structure to refer the status of task
 */
typedef struct tagS_CMN_REF_TSK {
    u8                                 tskPri;
    u8                                 reserved;
    u16                                wupCount;
    int                                sysUse;
} S_CMN_REF_TSK;


/*===================================================================
 * the structures related to "Timer"
 *==================================================================*/
/**
 * @breif the structure to refer the status of timer
 */
typedef struct tagS_CMN_REF_TIM {
    int                                stillWork;
} S_CMN_REF_TIM;


/*===================================================================
 * the structures related to "List"
 *==================================================================*/
/**
 * @breif the structure to define list
 */
typedef struct tagS_LIST {
    void  *pNext;
    void  *pPrev;
}S_LIST;


/*===================================================================
 * the structures for RF setting
 *==================================================================*/
typedef struct tagS_RFPARAM_PLIST {
    int    *prmArrayMax;
    int    *prmRegAddr;
    int    *prmRegValue;
}S_RFPARAM_PLIST;

/*
 * @brief macros to operate the list
 */
#define CMN_LIST_INIT(hd)                        { \
    (hd)->pNext = (void *)(hd);                    \
    (hd)->pPrev = (void *)(hd);                    \
}

#define CMN_LIST_ADD_TAIL(hd, elm, type, field)  {            \
    if ((hd)->pNext == (hd))                                  \
    {                                                         \
        (hd)->pNext = (void *)(elm);                          \
        (hd)->pPrev = (void *)(elm);                          \
        ((type *)(elm))->field.pNext = (void *)(hd);          \
        ((type *)(elm))->field.pPrev = (void *)(hd);          \
    }                                                         \
    else                                                      \
    {                                                         \
        ((type *)(elm))->field.pNext = (void *)(hd);          \
        ((type *)(elm))->field.pPrev = (hd)->pPrev;           \
        ((type *)((hd)->pPrev))->field.pNext = (void *)(elm); \
        (hd)->pPrev                          = (void *)(elm); \
    }                                                         \
}

#define CMN_LIST_REMOVE(hd, elm, type, field)    { \
    if (((hd)->pNext == (void *)(elm)) &&          \
        ((hd)->pPrev == (void *)(elm)))            \
    {                                              \
        (hd)->pNext = (void *)(hd);                \
        (hd)->pPrev = (void *)(hd);                \
    }                                              \
    else                                           \
    if ((hd)->pNext == (void *)(elm))              \
    {                                              \
        (hd)->pNext = ((type *)(elm))->field.pNext;                           \
        ((type *)(((type *)(elm))->field.pNext))->field.pPrev = (void *)(hd); \
    }                                              \
    else                                           \
    if ((hd)->pPrev == (void *)(elm))              \
    {                                              \
        ((type *)(((type *)(elm))->field.pPrev))->field.pNext = (void *)(hd); \
        (hd)->pPrev = ((type *)(elm))->field.pPrev;                           \
    }                                              \
    else                                           \
    {                                              \
        ((type *)(((type *)(elm))->field.pPrev))->field.pNext =       \
            ((type *)(elm))->field.pNext;                             \
        ((type *)(((type *)(elm))->field.pNext))->field.pPrev =       \
            ((type *)(elm))->field.pPrev;                             \
    }                                              \
}

#define CMN_LIST_IS_EMPTY(hd)          (((hd)->pNext == (void *)(hd)) ? TRUE : FALSE)
#define CMN_LIST_LOOKUP(hd)            (((hd)->pNext == (void *)(hd)) ? NULL : (hd)->pNext)

#define CMN_LIST_REMOVE_HEAD(hd, type, field)                           \
    ({                                                                  \
        void *_ptr = NULL;                                              \
        if((hd)->pNext != (void *)(hd)) {                               \
            _ptr = (hd)->pNext;                                         \
            (hd)->pNext = (((type *)_ptr)->field).pNext;                \
            if((hd)->pNext == (void *)(hd)) {                           \
                (hd)->pPrev = (void *)(hd);                             \
            } else {                                                    \
                ((type *)(((type *)_ptr)->field).pNext)->field.pPrev = (hd); \
            }                                                           \
        }                                                               \
        (type *)_ptr;                                                   \
    })


#define CMN_IS_IN_LIST(hd, elm, type, field)                            \
    ({                                                                  \
        T_CMN_ERR  _ret = FALSE;                                        \
        void *_ptr;                                                     \
        for(_ptr=(hd)->pNext;                                           \
            _ptr != (hd);                                               \
            _ptr=((type*)_ptr)->field.pNext) {                          \
            if(_ptr == elm) {                                           \
                _ret=TRUE;                                              \
                break;                                                  \
            }                                                           \
        }                                                               \
        _ret;                                                           \
    })


#define CMN_LIST_FOR(hd, elm, type, field)                            \
    for(elm=(type *)((hd)->pNext);                                    \
        (void *)elm != (hd);                                          \
        elm=(type*)(elm->field.pNext))

#define CMN_LIST_LOOKUP_NEXT(hd, elm, type, field)    \
    (((type *)(elm))->field.pNext == (void *)(hd)) ?  \
    NULL : ((type *)(((type *)(elm))->field.pNext))


#define CMN_LIST_SPLICE(hd, newhd, type, field) {                   \
    if ((newhd)->pNext == (newhd)) {                                \
        if((hd)->pNext != (hd)) {                                   \
            (newhd)->pNext = (hd)->pNext;                           \
            (newhd)->pPrev = (hd)->pPrev;                           \
            ((type *)((hd)->pNext))->field.pPrev = (void *)(newhd); \
            ((type *)((hd)->pPrev))->field.pNext = (void *)(newhd); \
            (hd)->pNext = (void *)(hd);                             \
            (hd)->pPrev = (void *)(hd);                             \
        }                                                           \
    } else {                                                        \
        if((hd)->pNext != (hd)) {                                   \
            ((type *)((newhd)->pPrev))->field.pNext = (hd)->pNext;  \
            ((type *)((hd)->pNext))->field.pPrev = (newhd)->pPrev;  \
            (newhd)->pPrev = (hd)->pPrev;                           \
            ((type *)((hd)->pPrev))->field.pNext = (void *)(newhd); \
            (hd)->pNext = (void *)(hd);                             \
            (hd)->pPrev = (void *)(hd);                             \
        }                                                           \
    }                                                               \
}

#define CMN_LIST_SWAP_ELEM(hd, old, new, type, field) {             \
    ((type *)(new))->field.pNext = ((type *)(old))->field.pNext;    \
    ((type *)(new))->field.pPrev = ((type *)(old))->field.pPrev;    \
    if (((hd)->pNext == (void *)(old)) &&                           \
        ((hd)->pPrev == (void *)(old))){                            \
        (hd)->pNext = (void *)(new);                                \
        (hd)->pPrev = (void *)(new);                                \
    }                                                               \
    else if((hd)->pNext == (void *)(old)) {                         \
        (hd)->pNext = (void *)(new);                                \
        ((type *)(((type *)(old))->field.pNext))->field.pPrev =     \
            (void *)(new);                                          \
    }                                                               \
    else if((hd)->pPrev == (void *)(old)) {                         \
        (hd)->pPrev = (void *)(new);                                \
        ((type *)(((type *)(old))->field.pPrev))->field.pNext =     \
            (void *)(new);                                          \
    }                                                               \
    else {                                                          \
        ((type *)(((type *)(old))->field.pPrev))->field.pNext =     \
            (void *)(new);                                          \
        ((type *)(((type *)(old))->field.pNext))->field.pPrev =     \
            (void *)(new);                                          \
    }                                                               \
}

/*-------------------------------------------------------------------
 * External Functions
 *-----------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

/*===================================================================
 * the functions related to "Synchronization"
 *=================================================================*/
extern T_CMN_ERR   CMN_createSem(u8, u32, u16, u16);
extern T_CMN_ERR   CMN_acreateSem(u32, u16, u16);
extern T_CMN_ERR   CMN_deleteSem(u8);
extern T_CMN_ERR   CMN_signalSem(u8);
extern T_CMN_ERR   CMN_waitSem(u8, u16);
extern T_CMN_ERR   CMN_referSem(u8, S_CMN_REF_SEM *); // not supported


/*===================================================================
 * the functions related to "CPU lock"
 *=================================================================*/
extern T_CMN_ERR   CMN_createCpuLock(u8);
extern T_CMN_ERR   CMN_acreateCpuLock(void);
extern T_CMN_ERR   CMN_deleteCpuLock(u8);
extern T_CMN_ERR   CMN_lockCpu(u8);
extern T_CMN_ERR   CMN_unlockCpu(u8);


/*===================================================================
 * the functions related to "Message"
 *=================================================================*/
extern T_CMN_ERR   CMN_createMsgBox(u8, u32);
extern T_CMN_ERR   CMN_deleteMsgBox(u8);
extern T_CMN_ERR   CMN_sendMsgBox(u8, u8, S_CMN_MSG *);
extern T_CMN_ERR   CMN_receiveMsgBox(u8, S_CMN_MSG **, u16);
extern T_CMN_ERR   CMN_referMsgBox(u8, S_CMN_REF_MBX *); // not supported


/*===================================================================
 * the functions related to "Memory"
 *=================================================================*/
extern T_CMN_ERR   CMN_createFixedMemPool(u8, u32, u16, uint);
extern T_CMN_ERR   CMN_acreateFixedMemPool(u32, u16, uint);
extern T_CMN_ERR   CMN_deleteFixedMemPool(u8);
extern T_CMN_ERR   CMN_getFixedMemPool(u8, void **, u16);
extern T_CMN_ERR   CMN_releaseFixedMemPool(u8, void *);
extern T_CMN_ERR   CMN_referFixedMemPool(u8, S_CMN_REF_MPL *); // not supported

extern T_CMN_ERR   CMN_allocMem(void **, uint);
extern void        CMN_releaseMem(void *);

/*===================================================================
 * the functions related to "Task/Thread"
 *=================================================================*/
extern T_CMN_ERR   CMN_createTask(u8, u32, void *, void *, u8, u32, void *);
extern T_CMN_ERR   CMN_acreateTask(u32, void *, void *, u8, u32, void *);
extern T_CMN_ERR   CMN_deleteTask(u8);
extern T_CMN_ERR   CMN_startTask(u8, void *);
extern void        CMN_exitTask(void);
extern T_CMN_ERR   CMN_terminateTask(u8);
extern T_CMN_ERR   CMN_sleepTask(u16);
extern T_CMN_ERR   CMN_wakeupTask(u8);
extern T_CMN_ERR   CMN_delayTask(u16);
extern T_CMN_ERR   CMN_referTask(u8, S_CMN_REF_TSK *); // not supported

/*===================================================================
 * the functions related to "Timer"
 *=================================================================*/
extern T_CMN_ERR   CMN_createAlarmTim(u8, u32, void *, void *);
extern T_CMN_ERR   CMN_acreateAlarmTim(u32, void *, void *);
extern T_CMN_ERR   CMN_deleteAlarmTim(u8);
extern T_CMN_ERR   CMN_startAlarmTim(u8, u16);
extern T_CMN_ERR   CMN_stopAlarmTim(u8);
extern T_CMN_ERR   CMN_getTime(u32 *);
extern T_CMN_ERR   CMN_referAlarmTim(u8, S_CMN_REF_TIM *); // not supported

/*===================================================================
 * the functions related to "Byte Order Convert"
 *=================================================================*/
// **caution**
// Please not use these functions directly, but use macros (CMN_XXX)
extern u16   CMN_byteSwap16(u16);
extern u32   CMN_byteSwap32(u32);

/*===================================================================
 * the functions related to "print"
 *=================================================================*/
extern void  CMN_print(const char *, ...);

/*===================================================================
 * the functions related to monitor and mode
 *=================================================================*/
#define MONSW_OFF   0
#define MONSW_ON    1
extern int   CMN_getMonitorSwitch(void);
#define MD_FIXDRATE 0
#define MD_LINKADPT 1
extern int   CMN_getModeSelect(void);

/*===================================================================
 * the functions related to suspend event.
 *=================================================================*/
extern int   CMN_getSuspendState(void);
extern void  CMN_setSuspendEvent(void *);
extern void  CMN_clearSuspendEvent(void);

/*===================================================================
 * the functions related to RF regs.
 *=================================================================*/
extern T_CMN_ERR   CMN_getRfParam(S_RFPARAM_PLIST *);

/*===================================================================
 * the functions related to monitor RSSI  
 *=================================================================*/
extern int CMN_getFreqUpdN(void);

/*===================================================================
 * the functions related to "WakeLock"
 *=================================================================*/
extern void CMN_createPowerLock(void);
extern void CMN_deletePowerLock(void);
extern T_CMN_ERR CMN_lockPower(void);
extern T_CMN_ERR CMN_unlockPower(void);

#if defined(__cplusplus)
}
#endif

#endif //__OSCMN_H__
