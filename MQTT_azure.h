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
**   $FILNAME   : MQTT_azure.h                                                         **
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

#ifndef MQTT_AZURE_H_
#define MQTT_AZURE_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/

/*****************************************************************************************
**                      Global Type efinitions                                         **
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

int AzureMqtt_Init(char *clientID, char *user, char *pass, char *rca_path);
bool AzureMqtt_IsConnected(void);
int AzureMqtt_Connect(char *host, int port);
int AzureMqtt_Disconnect(void);
int AzureMqtt_Loop(void);
int AzureMqtt_Subscribe(char *topic_prefix, const char* topic_name, void *cbfunc);
int AzureMqtt_Publish(char *topic_prefix, const char* topic_name, const char *data, int len);
int AzureMqtt_Deinit(void);

#endif /* MQTT_AZURE_H_ */
