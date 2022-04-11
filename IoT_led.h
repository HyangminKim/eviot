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
**   $FILNAME   : IoT_led.h                                                             **
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

#ifndef IOT_LED_H_
#define IOT_LED_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define IOT_LED_PATH_SZ 128
#define POWER_LIMIT 12800

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _IoTLedMsgType
{
    IOT_LEDMSG_ALL         = 0x00,
    IOT_LEDMSG_BATLEVEL    = 0x01,
    IOT_LEDMSG_BMSFAULT    = 0x02,
	IOT_LEDMSG_IOTFAULT    = 0x03,
	IOT_LEDMSG_TERMINATE   = 0x04,
	IOT_LEDMSG_FAULTINJECT = 0x05
} IoTLedMsgType;

typedef struct _IoTLedMsg
{
    long msgtype;
    int len;
    int IOT_status;
    int BMS_status;
    int BMS_AvailPwr;
    char path[IOT_LED_PATH_SZ];
} IoTLedMsg;


/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

#endif /* IOT_LED_H_ */
