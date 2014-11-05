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
 *  @file     cnlwrap_if.h
 *  @brief    CNL wrapper driver I/F definitions.
 *
 *
 *  @note
 */
/*==================================================================*/

#if !defined(__CNLWRAP_IF_H__)
#define __CNLWRAP_IF_H__

#include <linux/ioctl.h>

/*-------------------------------------------------------------------
 * Macro definitions
 *-----------------------------------------------------------------*/
/*
 * @brief device information
 */
#define CNLWRAP_DEVICENAME             "CnlFitCtrl"
#define CNLWRAP_DEVFILE                "/dev/"CNLWRAP_DEVICENAME

#define CNLWRAPADPT_DEVICENAME         "CnlFitAdpt"
#define CNLWRAPADPT_SCSIDEVICENAME     "CnlFitAdpt"
#define CNLWRAPADPT_OBEXDEVICENAME     "CnlFitAdpt"
#define CNLWRAPADPT_SCSIDEVFILE        "/dev/"CNLWRAPADPT_SCSIDEVICENAME
#define CNLWRAPADPT_OBEXDEVFILE        "/dev/"CNLWRAPADPT_OBEXDEVICENAME

/**
 * @brief CNL wrapper event type definition.
 */
typedef enum tagE_CNLWRAP_EVENT_TYPE {
    CNLWRAP_EVENT_ERROR_IND,
    CNLWRAP_EVENT_CONNECT_IND,
    CNLWRAP_EVENT_ACCEPT_IND,
    CNLWRAP_EVENT_ACCEPT_CNF,
    CNLWRAP_EVENT_RELEASE_IND,

    CNLWRAP_EVENT_DATA_REQ_COMP,

    CNLWRAP_EVENT_SUSPEND_PORT,        // additional (USE_ADPT_PORT only)
    CNLWRAP_EVENT_SUSPEND_PORT_COMP,   // additional (USE_ADPT_PORT only)

    // obsolete, unused event types.
    CNLWRAP_EVENT_POWERSAVE_IND,
    CNLWRAP_EVENT_POWERSAVE_CNF,
    CNLWRAP_EVENT_WAKE_IND,
    CNLWRAP_EVENT_WAKE_CNF,
    CNLWRAP_EVENT_DATA_IND,
    
    CNLWRAP_EVENT_UNKNOWN              = 0xff, // unknown
} E_CNLWRAP_EVENT_TYPE;

//
// parameter definitions for event(CONNECT_IND)
//
#define UID_SIZE                       8
#define PCL_PARAM_SIZE                 20

//
// parameter definitions for event(RELEASE)
//
#define RELEASE_ORIGIN_CNL             1
#define RELEASE_ORIGIN_PCL             2
#define RELEASE_CAUSE_CONNECT_TIMEOUT  1
#define RELEASE_CAUSE_ACCEPT_TIMEOUT   2
#define RELEASE_CAUSE_RETRY_TIMEOUT    3


//
// parameter definitions for ioctl(SEND_DATA/RECEIVE_DATA).
//
#define PROFILE_ID_0                   0
#define PROFILE_ID_1                   1
#define NOT_FRAGMENTED                 0
#define FRAGMENTED                     1
#define TARGET_SPECIFIED               0
#define TARGET_UNSPECIFIED             1

#define ASYNC_REQUEST                  0
#define SYNC_REQUEST                   1

#define DATA_DIRECTION_OUT             0
#define DATA_DIRECTION_IN              1


//
// parameter definitions for ioctl(ENABLE_PORT).
//
typedef enum tagE_CNLWRAP_ADPT_TYPE {
    CNLWRAP_SCSI_PORT                  = 0,
    CNLWRAP_OBEX_PORT,

    CNLWRAP_NO_PORT                    = 0xFF,
    // add more adapter type if needed
}E_CNLWRAP_ADPT_TYPE;

#define SINGLE_USE                     0
#define MULTI_USE                      1



#define CNLWRAP_REQ_UNSUPPORTED        0xFFFFFFFF
#define CNLWRAP_REQ_CANCELLED          0x00000002
#define CNLWRAP_REQ_PENDING            0x00000001
#define CNLWRAP_REQ_SUCCESS            0x00000000
#define CNLWRAP_REQ_CNL_ERR_INVSTAT    (-2)
#define CNLWRAP_REQ_CNL_ERR_BADPARAM   (-3)
#define CNLWRAP_REQ_CNL_ERR_HOST_IO    (-7)
#define CNLWRAP_REQ_CNL_ERR_CANCELLED  (-9)

#define CNLWRAP_STS_CNL_ERR_DEVICE_REMOVED (-11)
#define CNLWRAP_STS_CNL_ERR_HW_SUSPEND     (-17)
#define CNLWRAP_STS_CNL_ERR_HW_RESUME      (-18)

/*-------------------------------------------------------------------
 * structure definition.
 *-----------------------------------------------------------------*/
#pragma pack(push, 4)

/**
 * @brief cnl wrapper ioctl status for no-parameter request.
 */
typedef int S_CNLWRAP_STATUS;


/**
 * @brief cnl wrapper event error indication
 */
typedef struct tagS_CNLWRAP_ERROR_IND{
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_ERROR_IND;


/**
 * @brief cnl wrapper event connect indication
 */
typedef struct tagS_CNLWRAP_CONNECT_IND{
    u8                                 targetUID[UID_SIZE];
    u8                                 pclParam[PCL_PARAM_SIZE];
}S_CNLWRAP_CONNECT_IND;


/**
 * @brief cnl wrapper event accept indication
 */
typedef struct tagS_CNLWRAP_ACCEPT_IND{
    u8                                 targetUID[UID_SIZE];
    u8                                 pclParam[PCL_PARAM_SIZE];
}S_CNLWRAP_ACCEPT_IND;


/**
 * @brief cnl wrapper event accept confirmation
 */
typedef struct tagS_CNLWRAP_ACCEPT_CNF{
    u8                                 targetUID[UID_SIZE];
}S_CNLWRAP_ACCEPT_CNF;


/**
 * @brief cnl wrapper event release indication.
 */
typedef struct tagS_CNLWRAP_RELEASE_IND{
    u8                                 causeOrigin;
    u8                                 causeCode;
    u8                                 pclParam[PCL_PARAM_SIZE];
}S_CNLWRAP_RELEASE_IND;


/**
 * @brief cnl wrapper event powersave indication
 */
typedef struct tagS_CNLWRAP_POWERSAVE_IND{
    u8                                 dormantPeriod;
    u8                                 awakePeriod;
}S_CNLWRAP_POWERSAVE_IND;


/**
 * @brief cnl wrapper event data indication(only profileId)
 */
typedef struct tagS_CNLWRAP_DATA_IND{
    u8                                 profileId;
}S_CNLWRAP_DATA_IND;


/**
 * @brief cnl wrapper event data request completed.
 */
typedef struct tagS_CNLWRAP_DATA_REQ_COMP{
    S_CNLWRAP_STATUS                   status;
    unsigned long                      requestId; //unsigned long is 32bit/64bit compatible id(pointer size).
    u8                                 profileId;
    u8                                 direction;
    u8                                 fragmented;
    u32                                length;    
}S_CNLWRAP_DATA_REQ_COMP;


/**
 * @brief cnl wrapper event data request completed.  (32bit compatible)
 */
typedef struct {
    S_CNLWRAP_STATUS                   status;
    u32                                requestId;
    u8                                 profileId;
    u8                                 direction;
    u8                                 fragmented;
    u32                                length;    
} S_CNLWRAP32_DATA_REQ_COMP;


#define PIPE_MSG_BUFSIZE               32
#define PIPE_MSG_HDRSIZE               (sizeof(u8) * 2 + sizeof(u16) + sizeof(u32) * 2 + sizeof(void*) * 3)

/**
 * @brief adapter pipe message.
 */
typedef struct tagS_CNLWRAP_PIPE_MSG {
    void                              *pReserved1;
    void                              *pReserved2;
    u8                                 srcModID;
    u8                                 reserved;
    u16                                type;
    u32                                length;
    u32                                userData;
    void                              *pReserved3;
    u8                                 buffer[PIPE_MSG_BUFSIZE];
}S_CNLWRAP_PIPE_MSG;

typedef struct {
    u32                                pReserved1;
    u32                                pReserved2;
    u8                                 srcModID;
    u8                                 reserved;
    u16                                type;
    u32                                length;
    u32                                userData;
    u32                                pReserved3;
    u8                                 buffer[PIPE_MSG_BUFSIZE];
} S_CNLWRAP32_PIPE_MSG;

/**
 * @brief cnl wrapper event..
 */
typedef struct tagS_CNLWRAP_EVENT {
    u8                                 type;
    u16                                length;
    u8                                 reserved;
    union {
        S_CNLWRAP_ERROR_IND            errorInd;
        S_CNLWRAP_CONNECT_IND          connectInd;
        S_CNLWRAP_ACCEPT_IND           acceptInd;
        S_CNLWRAP_ACCEPT_CNF           acceptCnf;
        S_CNLWRAP_RELEASE_IND          releaseInd;
        S_CNLWRAP_POWERSAVE_IND        powersaveInd;
        S_CNLWRAP_DATA_IND             dataInd;
        S_CNLWRAP_DATA_REQ_COMP        dataReqComp;
        S_CNLWRAP_PIPE_MSG             pipeMsg;
    };
}S_CNLWRAP_EVENT;


/**
 * @brief cnl wrapper event..  (32bit compatible)
 */
typedef struct {
    u8                                 type;
    u16                                length;
    u8                                 reserved;
    union {
        S_CNLWRAP_ERROR_IND            errorInd;
        S_CNLWRAP_CONNECT_IND          connectInd;
        S_CNLWRAP_ACCEPT_IND           acceptInd;
        S_CNLWRAP_ACCEPT_CNF           acceptCnf;
        S_CNLWRAP_RELEASE_IND          releaseInd;
        S_CNLWRAP_POWERSAVE_IND        powersaveInd;
        S_CNLWRAP_DATA_IND             dataInd;
        S_CNLWRAP32_DATA_REQ_COMP      dataReqComp;
        S_CNLWRAP32_PIPE_MSG           pipeMsg;
    };
} S_CNLWRAP32_EVENT;


/**
 * @brief cnl wrapper ioctl init request.
 */
typedef struct tagS_CNLWRAP_REQ_INIT{
    u8                                 ownUID[UID_SIZE];
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_INIT;


/**
 * @brief cnl wrapper ioctl connect request.
 */
typedef struct tagS_CNLWRAP_REQ_CONNECT{
    u8                                 targetSpecified;
    u8                                 targetUID[UID_SIZE];
    u8                                 pclParam[PCL_PARAM_SIZE];
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_CONNECT;


/**
 * @brief cnl wrapper ioctl accept request.
 */
typedef struct tagS_CNLWRAP_REQ_ACCEPT{
    u8                                 pclParam[PCL_PARAM_SIZE];
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_ACCEPT;


/**
 * @brief cnl wrapper ioctl release request.
 */
typedef struct tagS_CNLWRAP_REQ_RELEASE{
    u8                                 pclParam[PCL_PARAM_SIZE];
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_RELEASE;


/**
 * @brief cnl wrapper ioctl powersave request.
 */
typedef struct tagS_CNLWRAP_REQ_POWERSAVE{
    u8                                 dormantPeriod;
    u8                                 awakePeriod;
    u16                                keepAlive;
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_POWERSAVE;


/**
 * @brief cnl wrapper ioctl data request.
 */
typedef struct tagS_CNLWRAP_REQ_DATA{
    u8                                 profileId;
    u8                                 fragmented;
    u32                                length;
    void *                             userBufAddr;
    u8                                 sync;
    unsigned long                      requestId;//unsigned long is 32bit/64bit compatible id(pointer size).
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_DATA;


/**
 * @brief cnl wrapper ioctl data request. (32bit compatible)
 */
typedef struct {
    u8                                 profileId;
    u8                                 fragmented;
    u32                                length;
    u32                                userBufAddr;
    u8                                 sync;
    u32                                requestId;
    S_CNLWRAP_STATUS                   status;
} S_CNLWRAP32_REQ_DATA;


/**
 * @brief cnl wrapper ioctl register cbk
 */
typedef struct tagS_CNLWRAP_REQ_REGCBK{
    u8                                 profileId;
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_REGCBK;


/**
 * @brief cnl wrapper ioctl cancel
 */
typedef struct tagS_CNLWRAP_REQ_CANCEL{
    unsigned long                      requestId;//unsigned long is 32bit/64bit compatible id(pointer size).
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_CANCEL;


/**
 * @brief cnl wrapper ioctl cancel  (32bit compatible)
 */
typedef struct {
    u32                                requestId;
    S_CNLWRAP_STATUS                   status;
} S_CNLWRAP32_REQ_CANCEL;


/**
 * @brief cnl wrapper ioctl get state
 */
typedef struct tagS_CNLWRAP_REQ_GETSTATE{
    u8                                 mainState;
    u8                                 subState;
}S_CNLWRAP_REQ_GETSTATE;


/**
 * @brief enable/disable port request
 */
typedef struct tagS_CNLWRAP_REQ_PORT {
    u8                                 type;
    u8                                 multi;
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_REQ_PORT;


/**
 * @brief cnl wrapper ioctl get state
 */
typedef struct tagS_CNLWRAP_STATS{
    u8                                 RSSI;
    S_CNLWRAP_STATUS                   status;
}S_CNLWRAP_STATS;

#pragma pack(pop)

/* CNLWRAP I/O control number definitions */
/* ioctl-magic number 'W','w' are already used all range. */
/* ioctl-magic number 'c' is alreday used 0x00-0x7F. */
#define CNLWRAPIOC_MAGIC               'c'
#define CNLWRAPIOC_INIT                _IOWR(CNLWRAPIOC_MAGIC, 0x80, S_CNLWRAP_REQ_INIT)
#define CNLWRAPIOC_CLOSE               _IOR(CNLWRAPIOC_MAGIC,  0x81, S_CNLWRAP_STATUS)
#define CNLWRAPIOC_CONNECT             _IOWR(CNLWRAPIOC_MAGIC, 0x82, S_CNLWRAP_REQ_CONNECT)
#define CNLWRAPIOC_WAIT_CONNECT        _IOR(CNLWRAPIOC_MAGIC,  0x83, S_CNLWRAP_STATUS)
#define CNLWRAPIOC_ACCEPT              _IOWR(CNLWRAPIOC_MAGIC, 0x84, S_CNLWRAP_REQ_ACCEPT)
#define CNLWRAPIOC_CONFIRM             _IOR(CNLWRAPIOC_MAGIC,  0x85, S_CNLWRAP_STATUS)
#define CNLWRAPIOC_RELEASE             _IOWR(CNLWRAPIOC_MAGIC, 0x86, S_CNLWRAP_REQ_RELEASE)
#define CNLWRAPIOC_SENDDATA            _IOWR(CNLWRAPIOC_MAGIC, 0x87, S_CNLWRAP32_REQ_DATA)
#define CNLWRAPIOC_RECVDATA            _IOWR(CNLWRAPIOC_MAGIC, 0x88, S_CNLWRAP32_REQ_DATA)
#define CNLWRAPIOC_CANCEL              _IOWR(CNLWRAPIOC_MAGIC, 0x89, S_CNLWRAP32_REQ_CANCEL)
#define CNLWRAPIOC_GETEVENT            _IOR(CNLWRAPIOC_MAGIC,  0x8a, S_CNLWRAP32_EVENT)
#define CNLWRAPIOC_SYNCRECV            _IOW(CNLWRAPIOC_MAGIC,  0x8b, u32)
#define CNLWRAPIOC_STOP_EVENT          _IO(CNLWRAPIOC_MAGIC,   0x8c)

// below ioctl cmd definitions are USE_ADPT_PORT only.
#define CNLWRAPIOC_ENABLE_PORT         _IOR(CNLWRAPIOC_MAGIC, 0x8d, S_CNLWRAP_REQ_PORT)
#define CNLWRAPIOC_DISABLE_PORT        _IOR(CNLWRAPIOC_MAGIC, 0x8e, S_CNLWRAP_REQ_PORT)
#define CNLWRAPIOC_SUSPEND_CONF        _IO(CNLWRAPIOC_MAGIC,  0x8f)
#define CNLWRAPIOC_GET_ADPT_ID         _IOR(CNLWRAPIOC_MAGIC, 0x90, u32)

#define CNLWRAPIOC_GETSTATS            _IOR(CNLWRAPIOC_MAGIC,  0x91, S_CNLWRAP_STATS)
// optional ioctl
#define CNLWRAPIOC_POWERSAVE           _IOWR(CNLWRAPIOC_MAGIC, 0x92, S_CNLWRAP_REQ_POWERSAVE)

#endif /* __CNLWRAP_IF_H__ */
