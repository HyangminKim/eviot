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
**   $FILNAME   : IoT_mq.h                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 15, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_MQ_H_
#define IOT_MQ_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define IOT_MQ_PATH_SZ 128
#define IOT_MQ_JSON_STRING_SZ 1024

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _IoTMqMsgType
{
    IOT_MQMSG_ALL         = 0x00,
    IOT_MQMSG_TERMINATE   = 0x01,
    IOT_MQMSG_FAULTINJECT = 0x02,
    IOT_MQMSG_SENDSTAT    = 0x03,
    IOT_MQMSG_OLDSTAT     = 0x04,
    IOT_MQMSG_SENDDATA    = 0x05,
    IOT_MQMSG_OLDDATA     = 0x06,
    IOT_MQMSG_SENDLOC     = 0x07,
    IOT_MQMSG_OLDLOC      = 0x08,
    IOT_MQMSG_SENDSOH     = 0x09
} IoTMqMsgType;

typedef struct _IoTMqMsg
{
    long msgtype;
    int  len;
    union {
        char path[IOT_MQ_PATH_SZ];
        char json_string[IOT_MQ_JSON_STRING_SZ];
    } data;
} IoTMqMsg;

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/
int mq_main(void);

#endif /* IOT_MQ_H_ */
