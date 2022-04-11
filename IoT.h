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
**   $FILNAME   : IoT.h                                                                 **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 10, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_H_
#define IOT_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define IOT_SW_VERSION          "1.0.3"
//#define IOT_SSL_DIR_PATH        "/home/root/ssl/"
//#define IOT_CONFIG_FILE_PATH    "/home/root/config/iot.cfg"
//#define IOT_REBOOT_FILE_PATH    "/home/root/config/rebootcnt"
//#define IOT_SYSUPD_FILE_PATH    "/home/root/config/sysupd"
//#define IOT_FAULTNM_FILE_PATH   "/home/root/config/faultnum"
#define IOT_SSL_DIR_PATH        "/home/debian/eviot/config/"
#define IOT_CONFIG_FILE_PATH    "/home/debian/eviot/config/iot.cfg"
#define IOT_REBOOT_FILE_PATH    "/home/debian/eviot/config/rebootcnt"
#define IOT_SYSUPD_FILE_PATH    "/home/debian/eviot/config/sysupd"
#define IOT_FAULTNM_FILE_PATH   "/home/debian/eviot/config/faultnum"

#define IOT_FILE_PATH_MAXSZ     128

/* Definitions for default configuration */
#define IOT_CFG_STR_SZ          50
#define IOT_CFG_BANKID          "BANK1"

#define IOT_CFG_MQTT_USER       "eviot"
#define IOT_CFG_MQTT_PASS       "abcd1234"
#define IOT_CFG_MQTT_ADDR       "mqtt.meuron.io"
#define IOT_CFG_MQTT_PORT       1883
#define IOT_CFG_MQTT_TOPIC_PREFIX "ev1"
#define IOT_CFG_MQTT_CLIENTID   "cid#mq#ev1"
#define IOT_CFG_ETH_CLIENTID    "cid#eth#ev1"
#define IOT_CFG_WIFI_CLIENTID   "cid#wifi#ev1"

/* Definitions for device name */
#define IOT_DEV_CAN             "can0"

#define IOT_DEV_ETH             "eth0"
#define IOT_DEV_SBMS            "eth1"

#define IOT_DEV_WIFI            "wlan0"

/* Definitions for time */
#define IOT_REBOOT_LIMIT        2
#define IOT_ALIVE_TIME          60
#define IOT_RETRY_CNT           3

#define IOT_SEC_24HOURS         (24 * 60 * 60) /* 86400 seconds */
#define IOT_MIN_24HOURS         (24 * 60) /* 1440 minutes */
#define USEC_TO_MILISEC         1000
#define MILISEC_TO_SEC          1000

/* Definitions for shared memory */
#define SHM_KEY_STATUS          10001
#define SHM_KEY_RBMS_DATA       10002
#define SHM_KEY_SBMS_DATA       10003
#define SHM_KEY_LOC_DATA        10004

/* Definitions for IOT HW architecture */
#define RBMS_MAX_RACK_CNT       10 /* Installed rack BMS count */

#define FAIL            -1
#define SUCCESS         0

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _MsgqType
{
    MSGQ_TYPE_MAIN      = 0x01,
    MSGQ_TYPE_DC        = 0x02,
    MSGQ_TYPE_STOR      = 0x03,
    MSGQ_TYPE_FOTA      = 0x04,
    MSGQ_TYPE_GPS       = 0x05,
    MSGQ_TYPE_LED       = 0x06,
    MSGQ_TYPE_MQ        = 0x07,
    MSGQ_TYPE_SOH       = 0x08,
    // MSGQ_TYPE_HELLO     = 0x0A,  // Hello Process Template
    MSGQ_TYPE_MAX       = 0x09
} MsgqType;

typedef enum _IoTEvt
{
    IOT_EVT_LAUNCH          = 0x01,
    IOT_EVT_INIT_UNRECOVER  = 0x02,
    IOT_EVT_INIT_SUCCESS    = 0x03,
    IOT_EVT_SYS_IGNORE      = 0x04,
    IOT_EVT_SYS_FAIL        = 0x05,
    IOT_EVT_SYS_RECOVERED   = 0x06,
    IOT_EVT_SYS_RECOV_FAIL  = 0x07,
    IOT_EVT_SYS_UNRECOVER   = 0x08,

    IOT_EVT_GTSYNC_FAIL     = 0x09,
    IOT_EVT_CAN_FAIL        = 0x0A,
    IOT_EVT_CAN_RECOVERED   = 0x0B,
    IOT_EVT_CAN_RECOV_FAIL  = 0x0C,
    IOT_EVT_SBMS_FAIL       = 0x0D,
    IOT_EVT_SBMS_RECOVERED  = 0x0E,
    IOT_EVT_SBMS_RECOV_FAIL = 0x0F,

    IOT_EVT_STOR_FAIL       = 0x10,
    IOT_EVT_STOR_RECOVERED  = 0x11,
    IOT_EVT_STOR_RECOV_FAIL = 0x12,

    IOT_EVT_MQ_FAIL         = 0x13,
    IOT_EVT_MQ_RECOVERED    = 0x14,
    IOT_EVT_MQ_RECOV_FAIL   = 0x15,

    IOT_EVT_OVER_NONNET     = 0x16,
    IOT_EVT_POWER_OFF       = 0x17,

    IOT_EVT_LED1_ON         = 0x18,
    IOT_EVT_LED2_ON         = 0x19,

    IOT_EVT_MAX             = 0x1A
} IoTEvt;

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

#endif /* IOT_H_ */
