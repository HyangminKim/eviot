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
**   $FILNAME   : MQTT_client.c                                                         **
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


/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_debug.h"
#include "MQTT_client.h"
#include "MQTT_meuron.h"
#include "MQTT_awsiot.h"
#include "MQTT_azure.h"
//#include "MQTT_google.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/


/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/

// MQTT Client 초기화 및 설정
int mqtt_init(MqConfig *pCfg)
{
    int rtn;
    if (pCfg == NULL)
        return -1;

    // Meuron MQTT Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        rtn = MeuronMqtt_Init(pCfg->clientID, pCfg->user, pCfg->pass);
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        rtn = MeuronMqtt_Init(pCfg->clientID, pCfg->user, pCfg->pass);
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        rtn = AwsMqtt_Init();
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        rtn = AzureMqtt_Init(pCfg->clientID, pCfg->user, pCfg->pass, pCfg->clientRootCaPath);
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        rtn = GcpMqtt_Init();
//    }
    else
    {
        LOG_WARN("Invalid MQTT server type => %d", pCfg->serverType);
        return -1;
    }

    if (0 != rtn)
    {
        LOG_WARN("Failed to initialize MQTT : errCode = %d", rtn);
        return MQ_RET_ERROR;
    }
    else
    {
        return MQ_RET_SUCCESS;
    }
}

int mqtt_deinit(MqConfig *pCfg)
{
//    int rtn;
    if (pCfg == NULL)
        return -1;

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Deinit();
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_Deinit();
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Deinit();
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Deinit();
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_Deinit();
//    }
    else
    {
        return -1;
    }

    return 0;
}
int mqtt_connect(MqConfig *pCfg)
{
    // int rtn;
    if (pCfg == NULL)
        return -1;

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Connect(pCfg->addr, pCfg->port);
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_Connect(pCfg->addr, pCfg->port);
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Connect(pCfg->addr, pCfg->port,
                pCfg->clientID,
                pCfg->clientRootCaPath,
                pCfg->clientCertPath,
                pCfg->clientPriKeyPath);
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Connect(pCfg->addr, pCfg->port);
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_Connect(pCfg->addr, // project_id
//                    pCfg->user, // client id prefix
//                    pCfg->clientID, // client id (device id)
//                    pCfg->clientPriKeyPath); // client private key
//    }
    else
    {
        return -1;
    }

    return 0;
}

bool mqtt_connected(MqConfig *pCfg)
{
    if (pCfg == NULL)
        return false;

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_IsConnected();
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_IsConnected();
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_IsConnected();
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_IsConnected();
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_IsConnected();
//    }
    else
    {
        return false;
    }

    return false;

}

int mqtt_subscribe(MqConfig *pCfg, const char* topic_name, MQTT_On_Message on_msg)
{
    if (pCfg == NULL)
        return (-1);

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Subscribe(pCfg->topicPrefix, topic_name, on_msg);
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_Subscribe(pCfg->topicPrefix, topic_name, on_msg);
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Subscribe(pCfg->topicPrefix, topic_name, on_msg);
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Subscribe(pCfg->topicPrefix, topic_name, on_msg);
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_Subscribe(pCfg->topicPrefix, topic_name, on_msg);
//    }
    else
    {
        return -1;
    }

    return (int)(0);
}

int mqtt_publish(MqConfig *pCfg, const char* topic_name, const char *data, int len)
{
    if (pCfg == NULL)
        return (-1);

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Publish(pCfg->topicPrefix, topic_name, data, len);
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return BecomMqtt_Publish(pCfg->topicPrefix, topic_name, data, len);
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Publish(pCfg->topicPrefix, topic_name, data, len);
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Publish(pCfg->topicPrefix, topic_name, data, len);
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_Publish(pCfg->topicPrefix, topic_name, data, len);
//    }
    else
    {
        return -1;
    }

    return (int)(0);
}

int mqtt_loop(MqConfig *pCfg)
{
    // int rtn;
    if (pCfg == NULL)
        return -1;

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Loop();
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_Loop();
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Loop();
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Loop();
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        // return GcpMqtt_Loop();
//    }
    else
    {
        return -1;
    }

    return 0;
}

int mqtt_disconnect(MqConfig *pCfg)
{
    if (pCfg == NULL)
        return -1;

    // Meuron Mqtt Server (MQTT 3.1.1)
    if (pCfg->serverType == IOT_MQ_SERVER_MEURON)
    {
        return MeuronMqtt_Disconnect();
    }
    // BECOM MQTT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_BECOM)
    {
        return MeuronMqtt_Disconnect();
    }
    // AWS IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AWS)
    {
        return AwsMqtt_Disconnect();
    }
    // Azure IoT Server
    else if (pCfg->serverType == IOT_MQ_SERVER_AZURE)
    {
        return AzureMqtt_Disconnect();
    }
    // Google IoT Server
//    else if (pCfg->serverType == IOT_MQ_SERVER_GOOGLE)
//    {
//        return GcpMqtt_Disconnect();
//    }
    else
    {
        return -1;
    }

    return 0;
}
