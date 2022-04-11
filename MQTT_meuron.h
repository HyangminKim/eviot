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
**   $FILNAME   : MQTT_meuron.h                                                         **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 24, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef MQTT_MEURON_H_
#define MQTT_MEURON_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/


/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

int MeuronMqtt_Init(char *clientID, char *user, char *pass);
bool MeuronMqtt_IsConnected(void);
int MeuronMqtt_Connect(char *host, int port);
int MeuronMqtt_Disconnect(void);
int MeuronMqtt_Loop(void);
int MeuronMqtt_Subscribe(char *topic_prefix, const char* topic_name, void *cbfunc);
int MeuronMqtt_Publish(char *topic_prefix, const char* topic_name, const char *data, int len);
int MeuronMqtt_Deinit(void);

int BecomMqtt_Publish(char *topic_prefix, const char* topic_name, const char *data, int len);

#endif /* MQTT_MEURON_H_ */
