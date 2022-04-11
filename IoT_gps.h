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
**   $FILNAME   : IoT_lte.h                                                             **
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

#ifndef IOT_GPS_H_
#define IOT_GPS_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define IOT_GPS_PATH_SZ 128

// Error codes and return values
#define UART_FUNCTION_SUCCESSFUL        0
#define UART_NUMBER_INCORRECT           1
#define UART_BAUDRATE_INCORRECT         2
#define UART_FIFO_ERROR                 3
#define UART_INCORRECT_PATH             4


//#define DEV_SER             "/dev/ttyO4" //beaglebone uart4
#define DEV_SER             "/dev/ttyO2" //IoT Board uart2
#define RX_BUFFER_SIZE      255
#define SECONDS(t)          t
#define MINUTES(t)          t*60
#define UART_1_INIT_CMD     "devc-seromap -u2 -F -b115200 -c48000000/16 0x48022000^2,74"

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _IoTGpsMsgType
{
    IOT_GPSMSG_ALL         = 0x00,
    IOT_GPSMSG_BATLEVEL    = 0x01,
    IOT_GPSMSG_TERMINATE   = 0x02,
    IOT_GPSMSG_FAULTINJECT = 0x03
} IoTGpsMsgType;

typedef struct _IoTGpsMsg
{
    long msgtype;
    int len;
    char path[IOT_GPS_PATH_SZ];
} IoTGpsMsg;

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/
int g_s32GpsFaultInjNum;



/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/


#endif /* IOT_GPS_H_ */
