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
**   $FILNAME   : MQTT_awsiot.h                                                         **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : May 25, 2021                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef MQTT_AWSIOT_H_
#define MQTT_AWSIOT_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/

// AWS IoT client authentication.
// Refer to https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
//#ifndef AWS_MQTT_PORT
//    #define AWS_MQTT_PORT    ( 8883 )
//#endif

//#define AWS_IOT_ENDPOINT         "a2bbicagredycv-ats.iot.ap-northeast-2.amazonaws.com"
//#define ROOT_CA_CERT_PATH        "/home/debian/eviot/credentials/AmazonRootCA1.crt"
//#define CLIENT_CERT_PATH         "/home/debian/eviot/credentials/d2fff1b90e-certificate.pem.crt"
//#define CLIENT_PRIVATE_KEY_PATH  "/home/debian/eviot/credentials/d2fff1b90e-private.pem.key"
//#define CLIENT_IDENTIFIER        "testclient"
//
//#define AWS_IOT_ENDPOINT_LENGTH         ( ( uint16_t ) ( sizeof( AWS_IOT_ENDPOINT ) - 1 ) )
//#define CLIENT_IDENTIFIER_LENGTH        ( ( uint16_t ) ( sizeof( CLIENT_IDENTIFIER ) - 1 ) )

/**
 * @brief ALPN (Application-Layer Protocol Negotiation) protocol name for AWS IoT MQTT.
 *
 * This will be used if the AWS_MQTT_PORT is configured as 443 for AWS IoT MQTT broker.
 * Please see more details about the ALPN protocol for AWS IoT MQTT endpoint in the link below.
 * https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/
 */
#define AWS_IOT_MQTT_ALPN               "\x0ex-amzn-mqtt-ca"
#define AWS_IOT_MQTT_ALPN_LENGTH        ( ( uint16_t ) ( sizeof( AWS_IOT_MQTT_ALPN ) - 1 ) )

/**
 * @brief The maximum number of retries for connecting to server.
 */
#define CONNECTION_RETRY_MAX_ATTEMPTS            ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying connection to server.
 */
#define CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS    ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for connection retry attempts.
 */
#define CONNECTION_RETRY_BACKOFF_BASE_MS         ( 500U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
//#define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 500 )
#define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 1000 )


/**
 * @brief The MQTT metrics string expected by AWS IoT.
 */
#define OS_NAME                   "Debian"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define OS_VERSION                "10.3"

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define HARDWARE_PLATFORM_NAME    "Meuron"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#define MQTT_LIB    "core-mqtt@" MQTT_LIBRARY_VERSION
/**
 * @brief The MQTT metrics string expected by AWS IoT.
 */
#define METRICS_STRING  "?SDK=" OS_NAME "&Version=" OS_VERSION "&Platform=" HARDWARE_PLATFORM_NAME "&MQTTLib=" MQTT_LIB

/**
 * @brief The length of the MQTT metrics string expected by AWS IoT.
 */
#define METRICS_STRING_LENGTH               ( ( uint16_t ) ( sizeof( METRICS_STRING ) - 1 ) )

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

int AwsMqtt_Init(void);
int AwsMqtt_Connect(char *host, int port, char *cid, char *rca_path, char *cert_path, char *prk_path);
int AwsMqtt_Disconnect(void);
bool AwsMqtt_IsConnected();
int AwsMqtt_Loop(void);
int AwsMqtt_Publish(char *topic_prefix, const char* topic_name, const char *topic_data, int topic_data_len);
int AwsMqtt_Subscribe( char *topic_prefix, const char* topic_name, void *cbfunc );
int AwsMqtt_Deinit(void);

#endif /* MQTT_AWSIOT_H_ */
