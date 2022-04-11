
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
**   $FILNAME   : MQTT_awsiot.c                                                         **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 24, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains MQTT client for AWS IoT                            **
**                                                                                      **
**                                                                                      **
******************************************************************************************/


/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/* Standard includes. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* POSIX includes. */
#include <unistd.h>

/* Include Demo Config as the first non-system header. */
// #include "demo_config.h"

/* MQTT API headers. */
#include <awsiot/core_mqtt.h>
#include <awsiot/core_mqtt_state.h>

/* OpenSSL sockets transport implementation. */
#include <awsiot/openssl_posix.h>

/*Include backoff algorithm header for retry logic.*/
#include <awsiot/backoff_algorithm.h>

/* Clock for timer. */
#include <awsiot/clock.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_debug.h"
#include "MQTT_client.h"
#include "MQTT_awsiot.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define NETWORK_BUFFER_SIZE     1024
#define MQTT_TOPIC_SIZE         20
/**
 * @brief Timeout for receiving CONNACK packet in milli seconds.
 */
#define SUBACK_RECV_TIMEOUT_MS                  ( 2000U )

/**
 * @brief Timeout for receiving CONNACK packet in milli seconds.
 */
#define PUBACK_RECV_TIMEOUT_MS                  ( 2000U )

/**
 * @brief Timeout for receiving CONNACK packet in milli seconds.
 */
#define CONNACK_RECV_TIMEOUT_MS                  ( 2000U )

/**
 * @brief Timeout for MQTT_ProcessLoop function in milliseconds.
 */
#define MQTT_PROCESS_LOOP_TIMEOUT_MS        ( 50U )
/**
 * @brief The maximum time interval in seconds which is allowed to elapse
 *  between two Control Packets.
 *
 *  It is the responsibility of the Client to ensure that the interval between
 *  Control Packets being sent does not exceed the this Keep Alive value. In the
 *  absence of sending any other Control Packets, the Client MUST send a
 *  PINGREQ Packet.
 */
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    ( 30U )
/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    OpensslParams_t * pParams;
};

/**
 * @brief Structure to keep the MQTT publish packets until an ack is received
 * for QoS1 publishes.
 */
typedef struct SubscribeInfo
{
    char subscribe_topic[MQTT_TOPIC_SIZE];

    MQTTSubscribeInfo_t subInfo;

    MQTT_On_Message on_message;
} SubscribeInfo_t;


typedef struct MessageStat
{
    uint16_t msg_id;
    volatile bool callbacked;
    volatile int callback_rtn;
} MessageStat_t;
/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static bool g_bAwsMqttConnected;
static MQTTContext_t g_stMqttContext = {0};
static NetworkContext_t g_stNetworkContext = {0};
static OpensslParams_t g_stOpensslParams = {0};

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context.
 */
static uint8_t g_arNwBuffer[ NETWORK_BUFFER_SIZE ];

/**
 * @brief Array to keep subscription topics.
 * Used to re-subscribe to topics that failed initial subscription attempts.
 */
static SubscribeInfo_t g_stSubInfo = { 0 };
static MessageStat_t g_stSubMsgStat, g_stPubMsgStat;

/**
 * @brief Struct to keep the outgoing publish messages.
 * This outgoing publish message are kept until a successful ack is received.
 */
//static PublishPackets_t g_stPubMsgInfo = { 0 };

/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/


/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/

static uint32_t generateRandomNumber()
{
    return( rand() );
}

/* Attempt to connect to the MQTT broker. If connection fails, retry after
 * a timeout. Timeout value will be exponentially increased till the maximum
 * attempts are reached or maximum timeout value is reached. The function
 * returns EXIT_FAILURE if the TCP connection cannot be established to
 * broker after configured number of attempts. */
/*-----------------------------------------------------------*/
static int connectToServerWithBackoffRetries( NetworkContext_t * pNetworkContext, char *host, int port, char *rca_path, char *cert_path, char *prk_path )
{
    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    OpensslStatus_t opensslStatus = OPENSSL_SUCCESS;
    BackoffAlgorithmContext_t reconnectParams;
    ServerInfo_t serverInfo;
    OpensslCredentials_t opensslCredentials;
    uint16_t nextRetryBackOff;

    /* Initialize information to connect to the MQTT broker. */
    serverInfo.pHostName = host;
    serverInfo.hostNameLength = strlen(host);
    serverInfo.port = port;

    /* Initialize credentials for establishing TLS session. */
    memset( &opensslCredentials, 0, sizeof( OpensslCredentials_t ) );
    opensslCredentials.pRootCaPath = rca_path;
    opensslCredentials.pClientCertPath = cert_path;
    opensslCredentials.pPrivateKeyPath = prk_path;

    /* AWS IoT requires devices to send the Server Name Indication (SNI)
     * extension to the Transport Layer Security (TLS) protocol and provide
     * the complete endpoint address in the host_name field. Details about
     * SNI for AWS IoT can be found in the link below.
     * https://docs.aws.amazon.com/iot/latest/developerguide/transport-security.html */
    opensslCredentials.sniHostName = host;

    /* Pass the ALPN protocol name depending on the port being used.
     * Please see more details about the ALPN protocol for the AWS IoT MQTT endpoint in the link below.
     * https://aws.amazon.com/blogs/iot/mqtt-with-tls-client-authentication-on-port-443-why-it-is-useful-and-how-it-works/
     */
    if( port == 443 )
    {
        opensslCredentials.pAlpnProtos = AWS_IOT_MQTT_ALPN;
        opensslCredentials.alpnProtosLen = AWS_IOT_MQTT_ALPN_LENGTH;
    }

    /* Initialize reconnect attempts and interval */
    BackoffAlgorithm_InitializeParams( &reconnectParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase until maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects
         * to the MQTT broker as specified in AWS_IOT_ENDPOINT and AWS_MQTT_PORT
         * at the demo config header. */
        LOG_INFO("Establishing a TLS session to %.*s:%d.", strlen(host), host, port );
        opensslStatus = Openssl_Connect( pNetworkContext,
                                         &serverInfo,
                                         &opensslCredentials,
                                         TRANSPORT_SEND_RECV_TIMEOUT_MS,
                                         TRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( opensslStatus != OPENSSL_SUCCESS )
        {
            /* Generate a random number and get back-off value (in milliseconds)
             * for the next connection retry. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff(
                                        &reconnectParams,
                                        generateRandomNumber(),
                                        &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LOG_WARN( "Connection to the broker failed, all attempts exhausted.");
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LOG_WARN( "Connection to the broker failed. Retrying connection "
                           "after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff );
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( opensslStatus != OPENSSL_SUCCESS ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent,
                                 char * clientId)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo = { 0 };

    /* Establish MQTT session by sending a CONNECT packet. */

    /* If #createCleanSession is true, start with a clean session
     * i.e. direct the MQTT broker to discard any previous session data.
     * If #createCleanSession is false, directs the broker to attempt to
     * reestablish a session which was already present. */
    connectInfo.cleanSession = createCleanSession;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    connectInfo.pClientIdentifier = clientId;
    connectInfo.clientIdentifierLength = strlen(clientId);

    /* The maximum time interval in seconds which is allowed to elapse
     * between two Control Packets.
     * It is the responsibility of the Client to ensure that the interval between
     * Control Packets being sent does not exceed the this Keep Alive value. In the
     * absence of sending any other Control Packets, the Client MUST send a
     * PINGREQ Packet. */
    connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    /* Use the username and password for authentication, if they are defined.
     * Refer to the AWS IoT documentation below for details regarding client
     * authentication with a username and password.
     * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
     * An authorizer setup needs to be done, as mentioned in the above link, to use
     * username/password based client authentication.
     *
     * The username field is populated with voluntary metrics to AWS IoT.
     * The metrics collected by AWS IoT are the operating system, the operating
     * system's version, the hardware platform, and the MQTT Client library
     * information. These metrics help AWS IoT improve security and provide
     * better technical support.
     *
     * If client authentication is based on username/password in AWS IoT,
     * the metrics string is appended to the username to support both client
     * authentication and metrics collection. */

    connectInfo.pUserName = METRICS_STRING;
    connectInfo.userNameLength = METRICS_STRING_LENGTH;
    /* Password for authentication is not used. */
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;


    /* Send MQTT CONNECT packet to broker. */
    mqttStatus = MQTT_Connect( pMqttContext, &connectInfo, NULL, CONNACK_RECV_TIMEOUT_MS, pSessionPresent );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        LOG_WARN( "Connection with MQTT broker failed with status %s.",
                    MQTT_Status_strerror( mqttStatus ));
    }
    else
    {
        LOG_INFO( "MQTT connection successfully established with broker.\n\n");
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void handleIncomingPublish( MQTTPublishInfo_t * pPublishInfo,
                                   uint16_t packetIdentifier )
{
    assert( pPublishInfo != NULL );

    /* Process incoming Publish. */
    LOG_INFO( "Incoming Publish: Id(%d), Topic(%.*s), Payload(%.*s.) \n",
               packetIdentifier,
               pPublishInfo->topicNameLength,
               pPublishInfo->pTopicName,
               ( int ) pPublishInfo->payloadLength,
               ( const char * ) pPublishInfo->pPayload );

    DBGPRT( "Incoming Publish: Id(%d), Topic(%.*s), Payload(%.*s.) \n",
               packetIdentifier,
               pPublishInfo->topicNameLength,
               pPublishInfo->pTopicName,
               ( int ) pPublishInfo->payloadLength,
               ( const char * ) pPublishInfo->pPayload );

    if (pPublishInfo->payloadLength && g_stSubInfo.on_message)
    {
        g_stSubInfo.on_message(pPublishInfo->pTopicName,
                               pPublishInfo->topicNameLength,
                               pPublishInfo->pPayload,
                               pPublishInfo->payloadLength);
    }

}


// Callback function for receiving packets.
static void AwsMqtt_EventCallback(MQTTContext_t * pMqttContext,
                                  MQTTPacketInfo_t * pPacketInfo,
                                  MQTTDeserializedInfo_t * pDeserializedInfo )
{
    uint16_t packetIdentifier;
    uint8_t * pPayload = NULL;
    size_t pSize = 0;

    /* Suppress unused parameter warning when asserts are disabled in build. */
    ( void ) pMqttContext;

    packetIdentifier = pDeserializedInfo->packetIdentifier;

    /* Handle incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if( ( pPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
//        assert( pDeserializedInfo->pPublishInfo != NULL );
//        /* Handle incoming publish. */
        handleIncomingPublish( pDeserializedInfo->pPublishInfo, packetIdentifier );
    }
    else
    {
        /* Handle other packets. */
        switch( pPacketInfo->type )
        {
            case MQTT_PACKET_TYPE_SUBACK:
                /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
                 * It contains the status code indicating server approval/rejection for the subscription to the single topic
                 * requested. The SUBACK will be parsed to obtain the status code */
                DBGPRT("AwsMqtt_EventCallback :  SUB[%u] => SUBACK[%u]  \n", g_stSubMsgStat.msg_id, packetIdentifier);
                if (packetIdentifier == g_stSubMsgStat.msg_id)
                {
                    g_stSubMsgStat.callbacked = true;
                    MQTT_GetSubAckStatusCodes( pPacketInfo, &pPayload, &pSize );
                    if( pPayload[ 0 ] != MQTTSubAckFailure )
                        g_stSubMsgStat.callback_rtn = EXIT_SUCCESS;
                    else
                        g_stSubMsgStat.callback_rtn = EXIT_FAILURE;
                }
                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
                /* Make sure ACK packet identifier matches with Request packet identifier. */
                break;

            case MQTT_PACKET_TYPE_PINGRESP:
                /* Nothing to be done from application as library handles PINGRESP. */
                break;

            case MQTT_PACKET_TYPE_PUBACK:
                // callback 된 메시지 id가 보낸 메시지 id와 같은지 확인.
                DBGPRT("AwsMqtt_EventCallback :  PUB[%u] => PUBACK[%u]  \n", g_stPubMsgStat.msg_id, packetIdentifier);
                if (packetIdentifier == g_stPubMsgStat.msg_id)
                {
                    g_stPubMsgStat.callbacked = true;
                    g_stPubMsgStat.callback_rtn = EXIT_SUCCESS;
                }
                break;

            default:
                /* Any other packet type is invalid. */
                break;
        }
    }
}

int AwsMqtt_Init(void)
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t networkBuffer;
    TransportInterface_t transport;

    g_bAwsMqttConnected = false;

//    struct timespec tp;
//    /* Get current time to seed pseudo random number generator. */
//    ( void ) clock_gettime( CLOCK_REALTIME, &tp );
//    /* Seed pseudo random number generator with nanoseconds. */
//    srand( tp.tv_nsec );

    /* Set the pParams member of the network context with desired transport. */
    g_stNetworkContext.pParams = &g_stOpensslParams;

    /* Fill in TransportInterface send and receive function pointers.
     * For this demo, TCP sockets are used to send and receive data
     * from network. Network context is SSL context for OpenSSL.*/
    transport.pNetworkContext = &g_stNetworkContext;
    transport.send = Openssl_Send;
    transport.recv = Openssl_Recv;

    /* Fill the values for network buffer. */
    networkBuffer.pBuffer = g_arNwBuffer;
    networkBuffer.size = NETWORK_BUFFER_SIZE;

    /* Initialize MQTT library. */
    mqttStatus = MQTT_Init( &g_stMqttContext,
                            &transport,
                            Clock_GetTimeMs,
                            AwsMqtt_EventCallback,
                            &networkBuffer );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        LOG_WARN("MQTT init failed: Status = %s. \n", MQTT_Status_strerror( mqttStatus ));
    }

    return returnStatus;
}

bool AwsMqtt_IsConnected()
{
    return g_bAwsMqttConnected;
}

int AwsMqtt_Connect(char *host, int port, char *cid, char *rca_path, char *cert_path, char *prk_path)
{
    int returnStatus = EXIT_SUCCESS;
    bool brokerSessionPresent;

    /* Attempt to connect to the MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will be exponentially increased till the maximum
     * attempts are reached or maximum timeout value is reached. The function
     * returns EXIT_FAILURE if the TCP connection cannot be established to
     * broker after configured number of attempts. */
    returnStatus = connectToServerWithBackoffRetries( &g_stNetworkContext, host, port, rca_path, cert_path, prk_path);
    if( returnStatus == EXIT_FAILURE )
    {
        /* Log error to indicate connection failure after all
         * reconnect attempts are over. */
        LOG_WARN( "AwsMqtt_Connect: Failed to make TTL connection to MQTT broker %.*s.", strlen(host), host );
    }

    /* Sends an MQTT Connect packet using the established TLS session,
     * then waits for connection acknowledgment (CONNACK) packet. */
    returnStatus = establishMqttSession( &g_stMqttContext, true, &brokerSessionPresent, cid);
    if( returnStatus != EXIT_SUCCESS )
    {
        g_bAwsMqttConnected = false;
        /* Log error to indicate connection failure after all
         * reconnect attempts are over. */
        /* End TLS session, then close TCP connection. */
        ( void ) Openssl_Disconnect(&g_stNetworkContext);
        LOG_WARN( "AwsMqtt_Connect: Failed to make MQTT session to MQTT broker %.*s.", strlen(host), host );
    }
    else
    {
        g_bAwsMqttConnected = true;
        LOG_INFO( "AwsMqtt_Connect: Success %.*s.", strlen(host), host );
    }

    return returnStatus;
}

int AwsMqtt_Disconnect(void)
{
    MQTTStatus_t mqttStatus = MQTTSuccess;
    int returnStatus = EXIT_SUCCESS;

    /* Send DISCONNECT. */
    if( MQTTSuccess != MQTT_Disconnect( &g_stMqttContext ))
    {
        LOG_WARN( "AwsMqtt_Disconnect: Failed with status=%s.",
                    MQTT_Status_strerror( mqttStatus ));
        returnStatus = EXIT_FAILURE;
    }

    if (returnStatus == EXIT_SUCCESS)
    {
        ( void ) Openssl_Disconnect( &g_stNetworkContext );
    }

    return returnStatus;
}


int AwsMqtt_Loop(void)
{
    MQTTStatus_t mqttStatus;
    mqttStatus = MQTT_ProcessLoop( &g_stMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );
    if (mqttStatus != MQTTSuccess)
    {
        LOG_WARN("AwsMqtt_Loop Error : %s", MQTT_Status_strerror( mqttStatus ));
        if ((mqttStatus==MQTTKeepAliveTimeout) ||
            (mqttStatus==MQTTNoDataAvailable) ||
            (mqttStatus==MQTTSendFailed) ||
            (mqttStatus==MQTTRecvFailed) )
        {
            g_bAwsMqttConnected = false;
        }
    }

    return (int)mqttStatus;
}

/*-----------------------------------------------------------*/

int AwsMqtt_Subscribe( char *topic_prefix, const char* topic_name, void *cbfunc )
{
    int processLoopTimeMs = 0;
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;

    // topic name 길이 검사
    if ((strlen(topic_prefix) + strlen(topic_name) + 2) <= MQTT_TOPIC_SIZE)
    {
        snprintf(g_stSubInfo.subscribe_topic, MQTT_TOPIC_SIZE, "%s/%s", topic_prefix, topic_name);
    }
    else
    {
        LOG_WARN("Publish topic name length is larger than %d ", MQTT_TOPIC_SIZE);
        return EXIT_FAILURE;
    }

    /* write subscribe info  */
    g_stSubInfo.subInfo.qos = MQTTQoS1;
    g_stSubInfo.subInfo.pTopicFilter = g_stSubInfo.subscribe_topic;
    g_stSubInfo.subInfo.topicFilterLength = strlen(g_stSubInfo.subscribe_topic);
    g_stSubInfo.on_message = cbfunc;

    /* Generate packet identifier for the SUBSCRIBE packet. */
    g_stSubMsgStat.msg_id = MQTT_GetPacketId(&g_stMqttContext);
    g_stSubMsgStat.callbacked = false;
    g_stSubMsgStat.callback_rtn = EXIT_SUCCESS;

    /* Send SUBSCRIBE packet. */
    mqttStatus = MQTT_Subscribe( &g_stMqttContext,
                                 &g_stSubInfo.subInfo,
                                 1, g_stSubMsgStat.msg_id );

    if( mqttStatus != MQTTSuccess )
    {
        LOG_WARN( "AwsMqtt_Subscribe : error = %s.", MQTT_Status_strerror( mqttStatus ));
        memset(&g_stSubMsgStat, 0, sizeof(g_stSubMsgStat));

        if ((mqttStatus==MQTTKeepAliveTimeout) ||
            (mqttStatus==MQTTNoDataAvailable) ||
            (mqttStatus==MQTTSendFailed))
        {
            g_bAwsMqttConnected = false;
        }

        return EXIT_FAILURE;
    }

    // QoS=1, SUBACK 응답 대기 루프.
    // 2초를 넘거나, 그안에 PUBACK를 받거나, ProcessLoop 에러 발생시 루프 종료.
    processLoopTimeMs = 0;
    do {
        mqttStatus = MQTT_ProcessLoop( &g_stMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );
        processLoopTimeMs += MQTT_PROCESS_LOOP_TIMEOUT_MS;
    } while (processLoopTimeMs < SUBACK_RECV_TIMEOUT_MS
            && g_stSubMsgStat.callbacked == false
            && mqttStatus == MQTTSuccess);

    // MQTT_ProcessLoop 루프 결과 처리,

    // MQTT_ProcessLoop 리턴값이 MQTTSuccess 아닌 경우,
    // 이 경우 MQTT 상태를 DISCONNECTED로 변경한다
    if (mqttStatus != MQTTSuccess )
    {
        LOG_WARN( "AwsMqtt_Publish: MQTT_ProcessLoop error = %s.", MQTT_Status_strerror( mqttStatus ));
        if ((mqttStatus==MQTTKeepAliveTimeout) ||
            (mqttStatus==MQTTNoDataAvailable) ||
            (mqttStatus==MQTTSendFailed) ||
            (mqttStatus==MQTTRecvFailed) )
        {
            g_bAwsMqttConnected = false;
        }
        returnStatus = EXIT_FAILURE;
    }
    // 대기 시간이 2초 이상인 경우, 실패 처리
    else if (processLoopTimeMs > SUBACK_RECV_TIMEOUT_MS)
    {
        LOG_WARN( "AwsMqtt_Subscribe: No SUBACK in %d msec", SUBACK_RECV_TIMEOUT_MS);
        returnStatus = EXIT_FAILURE;
    }
    // SUBACK를 받은 경우,
    else if (true == g_stSubMsgStat.callbacked)
    {
        returnStatus = g_stSubMsgStat.callback_rtn;
        if (returnStatus != EXIT_SUCCESS)
        {
            LOG_WARN("AwsMqtt_Subscribe : Callback error => %d ", returnStatus);
        }
    }

    memset(&g_stSubMsgStat, 0, sizeof(g_stSubMsgStat));
    return returnStatus;
}

/*-----------------------------------------------------------*/

int AwsMqtt_Publish(char *topic_prefix, const char* topic_name, const char *topic_data, int topic_data_len)
{
    static char publish_topic[MQTT_TOPIC_SIZE];
    static MQTTPublishInfo_t pubInfo;
    int processLoopTimeMs = 0;
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;

    // topic name 길이 검사
    if ((strlen(topic_prefix) + strlen(topic_name) + 2) <= MQTT_TOPIC_SIZE)
    {
        snprintf(publish_topic, MQTT_TOPIC_SIZE, "%s/%s", topic_prefix, topic_name);
    }
    else
    {
        LOG_WARN("Publish topic name length is larger than %d ", MQTT_TOPIC_SIZE);
        return EXIT_FAILURE;
    }

    /* This example publishes to only one topic and uses QOS1. */
    pubInfo.qos = MQTTQoS1;
    pubInfo.pTopicName = publish_topic;
    pubInfo.topicNameLength = strlen(publish_topic);
    pubInfo.pPayload = topic_data;
    pubInfo.payloadLength = topic_data_len;

    /* Get a new packet id. */
    g_stPubMsgStat.msg_id = MQTT_GetPacketId(&g_stMqttContext);
    g_stPubMsgStat.callbacked = false;
    g_stPubMsgStat.callback_rtn = EXIT_SUCCESS;

    /* Send PUBLISH packet. */
    mqttStatus = MQTT_Publish( &g_stMqttContext,
                               &pubInfo,
                               g_stPubMsgStat.msg_id );

    if( mqttStatus != MQTTSuccess )
    {
        LOG_WARN( "AwsMqtt_Publish: error = %s.", MQTT_Status_strerror( mqttStatus ));
        memset(&g_stPubMsgStat, 0, sizeof(g_stPubMsgStat));

        if ((mqttStatus==MQTTKeepAliveTimeout) ||
            (mqttStatus==MQTTNoDataAvailable) ||
            (mqttStatus==MQTTSendFailed))
        {
            g_bAwsMqttConnected = false;
        }

        return EXIT_FAILURE;
    }

    // QoS=1, PUBACK 응답 대기 루프.
    // 2초를 넘거나, 그안에 PUBACK를 받거나, ProcessLoop 에러 발생시 루프 종료.
    processLoopTimeMs = 0;
    do {
        mqttStatus = MQTT_ProcessLoop( &g_stMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );
        processLoopTimeMs += MQTT_PROCESS_LOOP_TIMEOUT_MS;
    } while (processLoopTimeMs < PUBACK_RECV_TIMEOUT_MS
            && g_stPubMsgStat.callbacked == false
            && mqttStatus == MQTTSuccess);

    // MQTT_ProcessLoop 루프 결과 처리,

    // MQTT_ProcessLoop 리턴값이 MQTTSuccess 아닌 경우,
    // 이 경우 MQTT 상태를 DISCONNECTED로 변경한다
    if (mqttStatus != MQTTSuccess )
    {
        LOG_WARN( "AwsMqtt_Publish: MQTT_ProcessLoop error = %s.", MQTT_Status_strerror( mqttStatus ));
        if ((mqttStatus==MQTTKeepAliveTimeout) ||
            (mqttStatus==MQTTNoDataAvailable) ||
            (mqttStatus==MQTTSendFailed) ||
            (mqttStatus==MQTTRecvFailed) )
        {
            g_bAwsMqttConnected = false;
        }
        returnStatus = EXIT_FAILURE;
    }
    // 대기 시간이 2초 이상인 경우, 실패 처리
    else if (processLoopTimeMs > PUBACK_RECV_TIMEOUT_MS)
    {
        LOG_WARN( "AwsMqtt_Publish: No PUBACK in %d msec", PUBACK_RECV_TIMEOUT_MS);
        returnStatus = EXIT_FAILURE;
    }
    // PUBACK를 받은 경우,
    else if (true == g_stPubMsgStat.callbacked)
    {
        returnStatus = g_stPubMsgStat.callback_rtn;
        if (returnStatus != EXIT_SUCCESS)
        {
            LOG_WARN("AwsMqtt_Publish: Callback error => %d ", returnStatus);
        }
    }

    memset(&g_stPubMsgStat, 0, sizeof(g_stPubMsgStat));
    return returnStatus;
}

int AwsMqtt_Deinit(void)
{
    return EXIT_SUCCESS;
}
