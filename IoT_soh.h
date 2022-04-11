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
**   $FILNAME   : IoT_soh.h                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : June 2, 2021                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_SOH_H_
#define IOT_SOH_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define SOH_MAX_FEAT_NUM    53
#define SOH_VOLT_FEAT_NUM   51
#define SOH_MAX_MEAS_NUM    64 /* 1 hours by 10 seconds */

#define BOUND_START_CVOLT   3360    /* 3.36 [V] */
#define BOUND_END_CVOLT     3380    /* 3.38 [V] */
#define BOUND_STEP_CVOLT    0.4     /* 0.0004 [V] */

#define SEL_PACK_CURR       500     /* 50 [A] = 0.1 * 500 */
#define SEL_STD_GAPTM       10      /* 10 [sec] */

#define MIN_TO_SEC          60
#define HOUR_TO_SEC         3600
#define MEAS_VOLT_SCALE     0.001
#define PACK_CURR_SCALE     0.1

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _IoTSohMsgType
{
    IOT_SOHMSG_ALL             = 0x00,
    IOT_SOHMSG_CHKVALID        = 0x01,
    IOT_SOHMSG_TERMINATE       = 0x02,
    IOT_SOHMSG_FAULTINJECT     = 0x03
} IoTSohMsgType;

typedef enum _MeasType
{
    MEAS_TYPE_AVGCELLV  = 0x00,
    MEAS_TYPE_PACKCURR  = 0x01,
    MEAS_TYPE_MAX       = 0x02
} MeasType;

typedef struct _IoTSohMsg
{
    long msgtype;
    int idx;
    unsigned long measTimeStamp;
    signed short meas[MEAS_TYPE_MAX];
} IoTSohMsg;

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

#endif /* IOT_SOH_H_ */
