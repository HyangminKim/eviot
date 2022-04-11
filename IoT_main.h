/*****************************************************************************************
**                                                                                      **
** Copyright (H) SK holdings C&C (2021)                                                 **
**                                                                                      **
** All rights reserved.                                                                 **
**                                                                                      **
** This document contains proprietary information belonging to SK holdings C&C.         **
** Passing on and copying of this document, and communication of its contents is not    **
** permitted without prior written authorization.                                       **
**                                                                                      **
******************************************************************************************
**                                                                                      **
**   $FILNAME   : IoT_main.h                                                            **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 18, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_MAIN_H_
#define IOT_MAIN_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define IOT_STAT_VER_SZ       10
#define IOT_STAT_STATE_SZ      8
/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _ProcType /* Proc Type */
{
    PROC_TYPE_MAIN  = 0x00,
    PROC_TYPE_DC    = 0x01,
    PROC_TYPE_STOR  = 0x02,
    PROC_TYPE_FOTA  = 0x03,
    PROC_TYPE_GPS   = 0x04,
    PROC_TYPE_LED   = 0x05,
    PROC_TYPE_MQ    = 0x06,
    PROC_TYPE_SOH   = 0x07,
    // PROC_TYPE_HELLO = 0x0A, // Hello Process Template
    PROC_TYPE_MAX   = 0x08
} ProcType;

/* Proc Status */
typedef enum _ProcStat
{
    PROC_STAT_TERMINATE = 0x00,
    PROC_STAT_INIT      = 0x01,
    PROC_STAT_RUN       = 0x02,
    PROC_STAT_MAX       = 0x03
} ProcStat;

typedef enum _DataType
{
    DATA_TYPE_EVENT     = 0x00,
    DATA_TYPE_FAULT_IDX = 0x01,
    DATA_TYPE_MAX       = 0x02
} DataType;

/* Main Proc Message Type */
typedef enum _IoTMainMsgType
{
    IOT_MAINMSG_ALL             = 0x00,
    IOT_MAINMSG_PROCRSP         = 0x01,
    IOT_MAINMSG_STATUS          = 0x02,
    IOT_MAINMSG_TERMRSP         = 0x03,
    IOT_MAINMSG_DOWNRSP         = 0x04,
    IOT_MAINMSG_ROOTFSUPDRSP    = 0x05,
    IOT_MAINMSG_GTIMERSP        = 0x06,
    IOT_MAINMSG_COMMQOS         = 0x07
} IoTMainMsgType;

/* Main Proc Message */
typedef struct _IoTMainMsg
{
    long msgtype;
    ProcType proc;
    char data[DATA_TYPE_MAX];
} IoTMainMsg;


typedef struct _IoTStat
{
    uint64_t msgCnt;
    uint64_t ts;
    int gTime;
    uint64_t bootTime;
    int freeMemMb;
    int freeDiskMb;
    int cpuUsagePercent;
    char wanIP[16];
    char iotVer[IOT_STAT_VER_SZ];
    uint8_t state[IOT_STAT_STATE_SZ];
} IoTStat;


/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

#endif /* IOT_MAIN_H_ */
