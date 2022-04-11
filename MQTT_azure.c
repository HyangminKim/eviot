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
**   $FILNAME   : MQTT_azure.c                                                         **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 24, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains MQTT client implementation.                        **
**                                                                                      **
**                                                                                      **
******************************************************************************************/


/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <mosquitto/mosquitto.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_debug.h"
#include "MQTT_client.h"
#include "MQTT_azure.h"


/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define MQTT_TOPIC_TO_PUB_SZ    50
// Refer to link for azure keep live timeout
// https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#default-keep-alive-timeout
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    (60)
#define MQTT_PROCESS_LOOP_TIMEOUT_MS        (50)

// Refer to https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#receiving-cloud-to-device-messages
#define AZURE_SUB_TOPIC "devices/%s/messages/devicebound/#"
// Refer to https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#sending-device-to-cloud-messages
#define AZURE_PUB_TOPIC "devices/%s/messages/events/"
#define AZUZR_PUG_MSG   "{\"devId\":\"%s\", \"msgType\":\"%s\", \"msgBody\":%s}"
/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/

typedef struct _MQTT_SubInfo
{
    char subscribe_topic[MQTT_TOPIC_TO_PUB_SZ];
    MQTT_On_Message on_message;
} MQTT_SubInfo;


typedef struct _MQTT_CbStat
{
    int msg_id;
    volatile int  callback_rtn;
    volatile bool callbacked;
} MQTT_CbStat;
/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static struct mosquitto *g_pstMosqInst;

static int g_intMsgId;
static bool g_bMqttConnected;
static MQTT_SubInfo  g_stSubInfo;
static MQTT_CbStat   g_stConnStat, g_stDisconnStat, g_stSubStat, g_stPubStat;


/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/

/*
 * Callback Parameters
 *  mosq    the mosquitto instance making the callback.
 *  obj the user data provided in mosquitto_new
 *  rtn  the return code of the connection response.
 *      The values are defined by the MQTT protocol version in use.
 *      For MQTT v5.0, look at section 3.2.2.2 Connect Reason code: https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html
 *      For MQTT v3.1.1, look at section 3.2.2.3 Connect Return code: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 * flags   the connect flags.
 */
static void mosquitto_connect_v5_callback(struct mosquitto *mosq, void *obj, int rtn, int flags, const mosquitto_property *prop)
{
    // callback 된 메시지 id가 보낸 메시지 id와 같은지 확인.
    DBGPRT("mosquitto_connect_v5_callback: mid=%d, rtn=%d, flags=%d \n",g_stConnStat.msg_id, rtn, flags);

    g_stConnStat.callbacked = true;
    g_stConnStat.callback_rtn = rtn;

    if (!rtn)
    {
        g_bMqttConnected = true;
    }
}

/*
 * Callback Parameters
 *  mosq    the mosquitto instance making the callback.
 *  obj the user data provided in mosquitto_new
 *  rc  integer value indicating the reason for the disconnect.
 *     A value of 0 means the client has called mosquitto_disconnect.
 *     Any other value indicates that the disconnect is unexpected.
 */
static void mosquitto_disconnect_v5_callback(struct mosquitto *mosq, void *obj, int rtn, const mosquitto_property *prop)
{
    // rtn이 0이면 mosquitto_disconnect를 호출에 대한 콜백을 의미
    // 다른 값인 경우, 다른 이유로 연결이 끊김을 의미
    if (!rtn)
    {
        g_stDisconnStat.callbacked = true;
        g_stDisconnStat.callback_rtn = rtn;
    }

    g_bMqttConnected = false;

}

/*
 * Callback Parameters
 *  mosq : the mosquitto instance making the callback.
 *  obj  : the user data provided in mosquitto_new
 *  mid  : the message id of the sent message.
 *  qos_count   the number of granted subscriptions (size of granted_qos).
 *  granted_qos an array of integers indicating the granted QoS for each of the subscriptions.
 *  props : list of MQTT 5 properties, or NULL
 */

static void mosquitto_subscribe_v5_callback(struct mosquitto *mosq, void *obj, int msgID, int qos_count, const int* grant_qos, const mosquitto_property *props)
{
    // callback 된 메시지 id가 보낸 메시지 id와 같은지 확인.
    DBGPRT("mosquitto_subscribe_callback: mid=%d, cb_mid=%d, count=%d, qos=%d \n",g_stSubStat.msg_id, msgID, qos_count, grant_qos[0]);

    if (msgID != g_stSubStat.msg_id)
        return;

    if (qos_count == 1 && grant_qos[0] < 128)
    {
        g_stSubStat.callbacked = true;
        g_stSubStat.callback_rtn = MOSQ_ERR_SUCCESS;
    }
    else
    {
        g_stSubStat.callbacked = true;
        g_stSubStat.callback_rtn = -1;
    }
}

/*
 * Callback Parameters
 *  mosq : the mosquitto instance making the callback.
 *  obj  : the user data provided in mosquitto_new
 *  msg : the message data.  This variable and associated memory will be freed by the library
 *        after the callback completes.  The client should make copies of any of the data it requires.
 *  props :   list of MQTT 5 properties, or NULL
 */
static void mosquitto_message_v5_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg, const mosquitto_property *props)
{
    // callback 된 메시지 id가 보낸 메시지 id와 같은지 확인.
    DBGPRT("mosquitto_message_v5_callback: mid=%d ,topic=%s ,payload= \n%.*s \n",
            msg->mid, msg->topic, msg->payloadlen, (char*)msg->payload);

    LOG_INFO("mosquitto_message_v5_callback: mid=%d ,topic=%s ,payload= \n%.*s \n",
            msg->mid, msg->topic, msg->payloadlen, (char*)msg->payload);

    if (msg->payloadlen && g_stSubInfo.on_message)
    {
        g_stSubInfo.on_message(msg->topic, strlen(msg->topic), msg->payload, msg->payloadlen);
    }
}

/*
 * Callback Parameters
 *  mosq : the mosquitto instance making the callback.
 *  obj  : the user data provided in mosquitto_new
 *  mid  : the message id of the sent message.
 *  rtn  : the MQTT 5 reason code (https://mosquitto.org/api/files/mqtt_protocol-h.html#mqtt5_return_codes)
 *  props : list of MQTT 5 properties, or NULL
 */
static void mosquitto_publish_v5_callback(struct mosquitto *mosq, void *obj, int msgID, int rtn, const mosquitto_property *prop)
{
    // callback 된 메시지 id가 보낸 메시지 id와 같은지 확인.
    DBGPRT("mosquitto_publish_v5_callback: mid=%d, cb_mid=%d, rtn=%d \n",g_stPubStat.msg_id, msgID, rtn);

    if (msgID != g_stPubStat.msg_id)
        return;

    g_stPubStat.callbacked = true;
    g_stPubStat.callback_rtn = rtn;

}

/*
 * Callback Parameters
 *  mosq    the mosquitto instance making the callback.
 *  obj the user data provided in mosquitto_new
 *  level   the log message level from the values:
 *          MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
 *  str the message string.
 */
static void mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{

}


bool AzureMqtt_IsConnected(void)
{
    return (g_bMqttConnected==true);
}

int AzureMqtt_Init(char *clientID, char *user, char *pass, char *rca_path)
{
    int rtn = 0;
    g_intMsgId = 0;
    g_bMqttConnected = false;

    if(NULL == clientID)
    {
        LOG_INFO("Client ID is NULL");
        return MOSQ_ERR_INVAL;
    }

    rtn = mosquitto_lib_init();
    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_INFO("mosquitto_lib_init:%d, %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    g_pstMosqInst = mosquitto_new(clientID, false, NULL);
    if(NULL == g_pstMosqInst)
    {
        LOG_INFO("mosquitto_new error");
        return MOSQ_ERR_NOMEM;
    }

    rtn = mosquitto_username_pw_set(g_pstMosqInst, user, pass);
    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_INFO("mosquitto_username_pw_set:%d, %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    rtn = mosquitto_tls_set(g_pstMosqInst, rca_path, NULL, NULL, NULL, NULL);
    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_INFO("mosquitto_tls_set:%d, %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    // specify the mqtt version to use
    rtn = mosquitto_int_option(g_pstMosqInst, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V311);
    if (rtn != MOSQ_ERR_SUCCESS)
    {
        LOG_INFO("mosquitto_int_option: %d, %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    mosquitto_connect_v5_callback_set(g_pstMosqInst , mosquitto_connect_v5_callback);
    mosquitto_disconnect_v5_callback_set(g_pstMosqInst , mosquitto_disconnect_v5_callback);
    mosquitto_publish_v5_callback_set(g_pstMosqInst , mosquitto_publish_v5_callback);
    mosquitto_subscribe_v5_callback_set(g_pstMosqInst , mosquitto_subscribe_v5_callback);
    mosquitto_message_v5_callback_set(g_pstMosqInst , mosquitto_message_v5_callback);
    mosquitto_log_callback_set(g_pstMosqInst, mosquitto_log_callback);

    return MOSQ_ERR_SUCCESS;
}

int AzureMqtt_Connect(char *host, int port)
{
    int rtn = 0;
    if((NULL == host) || (0 == port))
    {
        return MOSQ_ERR_INVAL;
    }

    g_stConnStat.callbacked = false;
    g_stConnStat.callback_rtn = MOSQ_ERR_SUCCESS;

    // Keep alive : 60 sec
    rtn = mosquitto_connect(g_pstMosqInst, host, port, MQTT_KEEP_ALIVE_INTERVAL_SECONDS);
    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_WARN("MQTT connect failed : %d - %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    // mosquitto_connect_v5_callback 콜백 함수 결과 대기
    do
    {
        rtn = mosquitto_loop(g_pstMosqInst, MQTT_PROCESS_LOOP_TIMEOUT_MS, 1);
        usleep(10 * USEC_TO_MILISEC);
    } while((MOSQ_ERR_SUCCESS == rtn) && (false == g_stConnStat.callbacked));

    return (true == g_stConnStat.callbacked)? g_stConnStat.callback_rtn: rtn;
}

int AzureMqtt_Disconnect(void)
{
    int rtn = 0;

    g_stDisconnStat.callbacked = false;
    g_stDisconnStat.callback_rtn = MOSQ_ERR_SUCCESS;

    rtn =  mosquitto_disconnect(g_pstMosqInst);
    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_WARN("MQTT disconnect failed : %d - %s", rtn, mosquitto_strerror(rtn));
        return rtn;
    }

    // mosquitto_disconnect_v5_callback 콜백 함수 결과 대기
    do
    {
        rtn = mosquitto_loop(g_pstMosqInst, MQTT_PROCESS_LOOP_TIMEOUT_MS, 1);
        usleep(10 * USEC_TO_MILISEC);
    } while((MOSQ_ERR_SUCCESS == rtn) && (false == g_stDisconnStat.callbacked));

    return (true == g_stDisconnStat.callbacked)? g_stDisconnStat.callback_rtn: rtn;
}

int AzureMqtt_Loop(void)
{
    return mosquitto_loop(g_pstMosqInst, MQTT_PROCESS_LOOP_TIMEOUT_MS, 1);
}

int AzureMqtt_Subscribe(char *topic_prefix, const char* topic_name, void *cbfunc)
{
    int rtn = 0;

    if(false == g_bMqttConnected)
    {
        return MOSQ_ERR_NO_CONN;
    }

    memset(&g_stSubInfo, 0, sizeof(g_stSubInfo));
    g_stSubInfo.on_message = cbfunc;

    // Refer to https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-mqtt-support#receiving-cloud-to-device-messages
    // Azure : devices/{device_id}/messages/devicebound/#
    snprintf(g_stSubInfo.subscribe_topic, sizeof(g_stSubInfo.subscribe_topic), AZURE_SUB_TOPIC, topic_prefix);

    g_stSubStat.msg_id = g_intMsgId++;
    g_stSubStat.callbacked = false;
    g_stSubStat.callback_rtn = MOSQ_ERR_SUCCESS;

    // MQTT Publish, QOS = 1, Retain = false
    rtn = mosquitto_subscribe(g_pstMosqInst, &g_stSubStat.msg_id, g_stSubInfo.subscribe_topic, 1);

    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_WARN("mosquitto_subscribe error : %d", rtn);
        return rtn;
    }

    // mosquitto_subscribe_v5_callback 함수 호출 대기
    do
    {
        rtn = mosquitto_loop(g_pstMosqInst, MQTT_PROCESS_LOOP_TIMEOUT_MS, 1);
        usleep(10 * USEC_TO_MILISEC);
    } while((MOSQ_ERR_SUCCESS == rtn) && (false == g_stSubStat.callbacked));

    // 네트워크 연결이 유실된 경우, mosquitto_loop는 MOSQ_ERR_NO_CONN(4)리턴
    // 이 경우 MQTT 상태를 DISCONNECTED로 변경한다.
    if (rtn == MOSQ_ERR_NO_CONN )
    {
        g_bMqttConnected = false;
    }

    return (true == g_stSubStat.callbacked)? g_stSubStat.callback_rtn: rtn;
}



int AzureMqtt_Publish(char *topic_prefix, const char* topic_name, const char *data, int len)
{
    int rtn = 0;
    int payload_len = 0;
    static char publish_topic[50];
    // ToDo : publish msg의 최대 크기 확인
    static char payload_data[3*1024];

    if(false == g_bMqttConnected)
    {
        return MOSQ_ERR_NO_CONN;
    }

    g_stPubStat.msg_id = g_intMsgId++;
    g_stPubStat.callbacked = false;
    g_stPubStat.callback_rtn = MOSQ_ERR_SUCCESS;

    memset(payload_data, 0, sizeof(payload_data));
    memset(publish_topic, 0, sizeof(publish_topic));
    snprintf(publish_topic, sizeof(publish_topic), AZURE_PUB_TOPIC, topic_prefix);
    payload_len = snprintf(payload_data, sizeof(payload_data), AZUZR_PUG_MSG, topic_prefix, topic_name, data);

    // MQTT Publish, QOS = 1, Retain = false
    rtn = mosquitto_publish(g_pstMosqInst, &g_stPubStat.msg_id,
            publish_topic,
            payload_len,
            payload_data,
            1, false);

    if(MOSQ_ERR_SUCCESS != rtn)
    {
        LOG_WARN("mosquitto_publish error : %d", rtn);
        return rtn;
    }

    // mosquitto_publish_v5_callback 함수 호출 대기
    do
    {
        rtn = mosquitto_loop(g_pstMosqInst, MQTT_PROCESS_LOOP_TIMEOUT_MS, 1);
        usleep(10 * USEC_TO_MILISEC);
    } while((MOSQ_ERR_SUCCESS == rtn) && (false == g_stPubStat.callbacked));

    // 네트워크 연결이 유실된 경우, mosquitto_loop는 MOSQ_ERR_NO_CONN(4)리턴
    // 이 경우 MQTT 상태를 DISCONNECTED로 변경한다.
    if (rtn == MOSQ_ERR_NO_CONN )
    {
        g_bMqttConnected = false;
    }

    return (true == g_stPubStat.callbacked)? g_stPubStat.callback_rtn: rtn;
}

int AzureMqtt_Deinit(void)
{
    mosquitto_destroy(g_pstMosqInst);
    return mosquitto_lib_cleanup();
}
