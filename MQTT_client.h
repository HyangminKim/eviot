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
**   $FILNAME   : MQTT_client.h                                                         **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : April, 21,  2021                                                      **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains MQTT wrapper for multiple MQTT server type         **
**                                                                                      **
**                                                                                      **
******************************************************************************************/

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

typedef void (*MQTT_Client_Connect_CB)(int rtn, const char *err_str);
typedef void (*MQTT_Client_Disconnect_CB)(int rtn, const char *err_str);
typedef void (*MQTT_Client_Publish_CB)(int rtn, int msg_id);
typedef void (*MQTT_Client_Log_CB)(const char *log_str);
typedef void (*MQTT_Client_Published_CB)(const char *topic, int topic_len, const char *data, int data_len);

typedef void (*MQTT_On_Connect)(int rtn, const char *err_str);
typedef void (*MQTT_On_Disconnect)(int rtn, const char *err_str);
typedef void (*MQTT_On_Publish)(int rtn, int msg_id);
typedef void (*MQTT_On_Log)(const char *log_str);
typedef void (*MQTT_On_Message)(const char *topic, int topic_len, const char *data, int data_len);

typedef enum _MqServerType
{
    IOT_MQ_SERVER_NONE    = 0x00,
    IOT_MQ_SERVER_AWS     = 0x01,
    IOT_MQ_SERVER_AZURE   = 0x02,
    IOT_MQ_SERVER_GOOGLE  = 0x03,
    IOT_MQ_SERVER_MEURON  = 0x04,
    IOT_MQ_SERVER_BECOM   = 0x05
} MqServerType;

typedef struct _MqConfig
{
    int  serverType; //  AWSIoT:1, AzureIoT:2, GoogleIoT: 3 Custom:4,
    char addr[100];
    int  port;
    char user[100];
    char pass[150];
    char topicPrefix[50];
    char clientID[50];
    char clientRootCaPath[100]; // Client root CA file path
    char clientCertPath[100];   // Client Certificate file path
    char clientPriKeyPath[100]; // Client private key file path
} MqConfig;

typedef enum _MqRetType
{
    MQ_RET_SUCCESS = 0,
    MQ_RET_ERROR = 1
} MqRetType;

int  mqtt_init(MqConfig *pCfg);
int  mqtt_deinit(MqConfig *pCfg);
int  mqtt_connect(MqConfig *pCfg);
bool mqtt_connected(MqConfig *pCfg);
int  mqtt_subscribe(MqConfig *pCfg, const char* topic_name, MQTT_On_Message on_msg);
int  mqtt_publish(MqConfig *pCfg, const char *topic_name, const char *data, int len);
int  mqtt_loop(MqConfig *pCfg);
int  mqtt_disconnect(MqConfig *pCfg);

#endif /* MQTT_CLIENT_H_ */
