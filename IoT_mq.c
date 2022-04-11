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
**   $FILNAME   : IoT_mq.c                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 23, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains MQ process implementation.                        **
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
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <net/if.h>

#include <config/libconfig.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_rbuf.h"
#include "IoT_main.h"
#include "IoT_debug.h"

#include "IoT_mq.h"
#include "MQTT_client.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define MQ_MSGQ_FULL_CNT   56000
#undef OLDDATA

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Macro Definitions                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/
static const char *TOPIC_NAME_DATA       = "data";
static const char *TOPIC_NAME_STATUS     = "status";
static const char *TOPIC_NAME_LOCATION   = "loc";
static const char *TOPIC_NAME_COMMAND    = "cmd";
static const char *TOPIC_NAME_SOH        = "soh";

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
//static int g_s32FaultInjNum;

static int g_s32MainMsgID, g_s32MqMsgID;
static IoTMainMsg g_stMainMsg;
static IoTMqMsg g_stMqMsg;

static pthread_mutex_t g_mtxMqMainMsg;

static char g_s8MqTxFileBuf[MAX_RBUF_RBMS_DATA_SZ];

static MqConfig g_stMqCfg =
{
    .serverType = 4,
    .addr = {IOT_CFG_MQTT_ADDR},
    .port = IOT_CFG_MQTT_PORT,
    .user = {IOT_CFG_MQTT_USER},
    .pass = {IOT_CFG_MQTT_PASS},
    .topicPrefix = {IOT_CFG_MQTT_TOPIC_PREFIX},
    .clientID = {IOT_CFG_MQTT_CLIENTID}
};

/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/


static void reboot_system(void)
{
    int fdscr = -1, rbCnt = 0;
    char fbuf = 0;

    sleep(1); /* Sleep 1 sec for message print time */
    fdscr = open(IOT_REBOOT_FILE_PATH, O_RDWR | O_CREAT | O_SYNC, 0644);
    if(fdscr < 0)
    {
        //LOG_INFO("### Reboot count file open failed. reboot now...");
        //LogClose();

        /* hard reset */
        reboot(RB_AUTOBOOT);
    }

    if(read(fdscr, &fbuf, 1) > 0)
    {
        rbCnt = fbuf - '0';
        if(rbCnt > IOT_REBOOT_LIMIT) /* Go to shutdown if reboot has performed more than 3 */
        {
            fbuf = '0';
            lseek(fdscr, 0, SEEK_SET);
            write(fdscr, &fbuf, 1);
            close(fdscr);

            /* Power OFF */
            //LOG_INFO("### SHUTDOWN now...");
            //LogClose();
            reboot(RB_POWER_OFF);
        }
        else
        {
            fbuf = rbCnt + '1';
            lseek(fdscr, 0, SEEK_SET);
            write(fdscr, &fbuf, 1);
        }
    }
    else /* First reboot */
    {
        fbuf = '1';
        lseek(fdscr, 0, SEEK_SET);
        write(fdscr, &fbuf, 1);
    }

    close(fdscr);
    //LOG_INFO("### Reboot now...");
    //LogClose();

    /* hard reset */
    reboot(RB_AUTOBOOT);
}

static void procMqStatusUpdate(int evt, int fail_idx)
{
    int rtn = 0;

    pthread_mutex_lock(&g_mtxMqMainMsg);
    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
    g_stMainMsg.proc = PROC_TYPE_MQ;
    g_stMainMsg.data[DATA_TYPE_EVENT] = evt;
    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = fail_idx;
    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg)-sizeof(long), 0);
    pthread_mutex_unlock(&g_mtxMqMainMsg);

    if(rtn < 0) /* Fault inject num: 302 */
    {
        //LOG_WARN("Failed send message to MAIN proc message queue");
        reboot_system();
    }
}

static void initMsgQueue(void)
{
    /* 1. Initialize MAIN Proc message queue - fault num: 304 */
    if(FAIL == (g_s32MainMsgID = msgget((key_t)MSGQ_TYPE_MAIN, IPC_CREAT | 0666)))
    {
        //LOG_WARN("Failed create MAIN Proc message queue");
        reboot_system();
    }

    /* 2. Initialize MQ Proc message queue - fault num: 305 */
    if(FAIL == (g_s32MqMsgID = msgget((key_t)MSGQ_TYPE_MQ, IPC_CREAT | 0666)))
    {
        //LOG_WARN("Failed create MQ Proc message queue");
        reboot_system();
    }
}

// Remove message queue
static void removeMsgQueue(int msg_queue_id)
{
    // 메시지 큐 삭제
    if(FAIL != msgctl(msg_queue_id, IPC_RMID, NULL))
    {
        //LOG_INFO("Failed to delete message queue : id = %d ", msg_queue_id);
    }
    else
    {
        //LOG_INFO("Succeed to delete message queue : id = %d ", msg_queue_id);
    }
}

void loadMqCfgString(config_setting_t *setting, const char *name, char *dest, int dest_len, int errno)
{
    const char *strcfg = NULL;
    if(0 != config_setting_lookup_string(setting, name, &strcfg))
    {
        DBGPRT("config_setting_lookup_string : %s \n", strcfg);
        strncpy(dest, strcfg, dest_len);
    }
    else
    {
        //LOG_WARN("Can't find %s ", name);
        procMqStatusUpdate(IOT_EVT_INIT_UNRECOVER, errno);
    }
}

void loadMqCfgInt(config_setting_t *setting, const char *name, int *dest, int errno)
{
    int cfgint;
    if(0 != config_setting_lookup_int(setting, name, &cfgint))
    {
        *dest = cfgint;
    }
    else
    {
        //LOG_WARN("Can't find %s ", name);
        procMqStatusUpdate(IOT_EVT_INIT_UNRECOVER, errno);
    }
}

static void loadMqCfgUUID(char* dest_uuid, int dest_len)
{
    int t = 0;
    char *szTemp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    char *szHex = "0123456789abcdef-";
    int nLen = strlen (szTemp);

    srand (clock());

    for (t=0; t<nLen; t++)
    {
      int r = rand () % 16;
      char c = ' ';
      switch (szTemp[t])
      {
        case 'x' :
          c = szHex [r];
          break;
        case 'y' :
          c = szHex [(r & 0x03) | 0x08];
          break;
        case '-' :
          c = '-';
          break;
        case '4' :
          c = '4';
          break;
        default :
          break;
      }
      dest_uuid[t] = c;
    }
    dest_uuid[t] = 0;
}

static void loadMqConfigure(MqConfig *pCfg)
{
    config_t cfg;
    config_setting_t *setting;

    config_init(&cfg);
    if(CONFIG_FALSE == config_read_file(&cfg, IOT_CONFIG_FILE_PATH)) /* Fault inject num: 306 */
    {
        //LOG_WARN("MQ Proc - config file read error(%s:%d - %s)",
//                 config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        procMqStatusUpdate(IOT_EVT_INIT_UNRECOVER, 6);
        config_destroy(&cfg);
    }
    else
    {
        setting = config_lookup(&cfg, "server");
        // Common
        if(NULL != setting)
        {
            /* configure: serverType */
            loadMqCfgInt(setting, "server_type", &(pCfg->serverType), 7);
            DBGPRT("# loadMqConfigure: server_type : %d \n", pCfg->serverType);

            /* configure: topic prefix */
            loadMqCfgString(setting, "topic_prefix", pCfg->topicPrefix, sizeof(pCfg->topicPrefix), 8);
            DBGPRT("# loadMqConfigure: topic_prefix : %s \n", pCfg->topicPrefix);

            /* configure: mq client ID */
            /*
            loadMqCfgString(setting, "clientid", pCfg->clientID, sizeof(pCfg->clientID), 9);
            DBGPRT("# loadMqConfigure: clientid : %s \n", pCfg->clientID);
            */

            loadMqCfgUUID(pCfg->clientID, sizeof(pCfg->clientID));
            DBGPRT("# loadMqConfigure: clientid : %s \n", pCfg->clientID);
        }

        // Config for Meuron IoT
        if ((NULL != setting) && (pCfg->serverType == IOT_MQ_SERVER_MEURON))
        {
            /* configure: address */
            loadMqCfgString(setting, "meuron_address", pCfg->addr, sizeof(pCfg->addr), 10);
            DBGPRT("# loadMqConfigure: meuron_address : %s \n", pCfg->addr);

            /* configure: port */
            loadMqCfgInt(setting, "meuron_port", &(pCfg->port), 11);
            DBGPRT("# loadMqConfigure: meuron_port : %d \n", pCfg->port);

            /* configure: user */
            loadMqCfgString(setting, "meuron_user", pCfg->user, sizeof(pCfg->user), 12);
            DBGPRT("# loadMqConfigure: meuron_user : %s \n", pCfg->user);

            /* configure: password */
            loadMqCfgString(setting, "meuron_pass", pCfg->pass, sizeof(pCfg->pass), 13);
            DBGPRT("# loadMqConfigure: meuron_pass : %s \n", pCfg->pass);
        }

        // Config for Becom IoT
        if ((NULL != setting) && (pCfg->serverType == IOT_MQ_SERVER_BECOM))
        {
            /* configure: address */
            loadMqCfgString(setting, "becom_address", pCfg->addr, sizeof(pCfg->addr), 10);
            DBGPRT("# loadMqConfigure: becom_address : %s \n", pCfg->addr);

            /* configure: port */
            loadMqCfgInt(setting, "becom_port", &(pCfg->port), 11);
            DBGPRT("# loadMqConfigure: becom_port : %d \n", pCfg->port);

            /* configure: user */
            loadMqCfgString(setting, "becom_user", pCfg->user, sizeof(pCfg->user), 12);
            DBGPRT("# loadMqConfigure: becom_user : %s \n", pCfg->user);

            /* configure: password */
            loadMqCfgString(setting, "becom_pass", pCfg->pass, sizeof(pCfg->pass), 13);
            DBGPRT("# loadMqConfigure: becom_pass : %s \n", pCfg->pass);
        }

        // Config for AWS IoT
        if ((NULL != setting) && (pCfg->serverType == IOT_MQ_SERVER_AWS))
        {
            /* configure: address */
            loadMqCfgString(setting, "awsiot_address", pCfg->addr, sizeof(pCfg->addr), 14);
            DBGPRT("# loadMqConfigure: awsiot_address : %s \n", pCfg->addr);

            /* configure: port */
            loadMqCfgInt(setting, "awsiot_port", &(pCfg->port), 15);
            DBGPRT("# loadMqConfigure: awsiot_port : %d \n", pCfg->port);

            /* configure: root ca */
            loadMqCfgString(setting, "awsiot_root_ca_cert_path", pCfg->clientRootCaPath, sizeof(pCfg->clientRootCaPath), 16);
            DBGPRT("# loadMqConfigure: awsiot_root_ca_cert_path : %s \n", pCfg->clientRootCaPath);

            /* configure: client cert */
            loadMqCfgString(setting, "awsiot_client_cert_path", pCfg->clientCertPath, sizeof(pCfg->clientCertPath), 17);
            DBGPRT("# loadMqConfigure: awsiot_client_cert_path : %s \n", pCfg->clientCertPath);

            /* configure: client private key */
            loadMqCfgString(setting, "awsiot_client_private_key_path", pCfg->clientPriKeyPath, sizeof(pCfg->clientPriKeyPath), 18);
            DBGPRT("# loadMqConfigure: awsiot_client_private_key_path : %s \n", pCfg->clientPriKeyPath);
        }

        // Config for Azure IoT
        if ((NULL != setting) && (pCfg->serverType == IOT_MQ_SERVER_AZURE))
        {
            /* configure: address */
            loadMqCfgString(setting, "azure_address", pCfg->addr, sizeof(pCfg->addr), 19);
            DBGPRT("# loadMqConfigure: azure_address : %s \n", pCfg->addr);

            /* configure: port */
            loadMqCfgInt(setting, "azure_port", &(pCfg->port), 20);
            DBGPRT("# loadMqConfigure: azure_port : %d \n", pCfg->port);

            /* configure: user */
//            loadMqCfgString(setting, "azure_user", pCfg->user, sizeof(pCfg->user), 21);
            snprintf(pCfg->user, sizeof(pCfg->user), "%s/%s/%s", pCfg->addr,pCfg->clientID,"?api-version=2018-06-30");
            DBGPRT("# loadMqConfigure: azure_user : %s \n", pCfg->user);

            /* configure: password */
            loadMqCfgString(setting, "azure_pass", pCfg->pass, sizeof(pCfg->pass), 22);
            DBGPRT("# loadMqConfigure: azure_pass : %s \n", pCfg->pass);

            /* configure: root ca */
            loadMqCfgString(setting, "azure_root_ca_cert_path", pCfg->clientRootCaPath, sizeof(pCfg->clientRootCaPath), 23);
            DBGPRT("# loadMqConfigure: azure_root_ca_cert_path : %s \n", pCfg->clientRootCaPath);
        }

        // Config for Google IoT
        if ((NULL != setting) && (pCfg->serverType == IOT_MQ_SERVER_GOOGLE))
        {
            /* configure: project id prefix , copy to addr */
            loadMqCfgString(setting, "gcpiot_project_id", pCfg->addr, sizeof(pCfg->addr), 24);
            DBGPRT("# loadMqConfigure: gcpiot_project_id : %s \n", pCfg->addr);

            /* configure: client id prefix , copy to user */
            loadMqCfgString(setting, "gcpiot_client_id_prefix", pCfg->user, sizeof(pCfg->user), 25);
            DBGPRT("# loadMqConfigure: gcpiot_client_id_prefix : %s \n", pCfg->user);

            /* configure: client private key */
            loadMqCfgString(setting, "gcpiot_client_private_key_path", pCfg->clientPriKeyPath, sizeof(pCfg->clientPriKeyPath), 26);
            DBGPRT("# loadMqConfigure: gcpiot_client_private_key_path : %s \n", pCfg->clientPriKeyPath);
        }

        config_destroy(&cfg);
        //LOG_INFO("MQ configuration [serverType: %d, addr: %s, port: %d, prefix: %s, clientID: %s]",
//                 pCfg->serverType, pCfg->addr, pCfg->port, pCfg->topicPrefix, pCfg->clientID);
    }
}

static void *mq_alive_check_thread(void *arg)
{
    static int threadCnt = 0;
    int rtn = 0, retryCnt = 0;

    while(1)
    {
        if(IOT_ALIVE_TIME == threadCnt) /* 1 minute */
        {
            /* Tx alive message to MAIN proc */
            pthread_mutex_lock(&g_mtxMqMainMsg);
            g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
            g_stMainMsg.proc = PROC_TYPE_MQ;
            g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
            g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
            rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0);
            printf("mq!!!\n");
            pthread_mutex_unlock(&g_mtxMqMainMsg);

            if(rtn < 0) /* Fault inject num: 301 */
            {
                //LOG_WARN("Failed send message to MAIN proc message queue");
                procMqStatusUpdate(IOT_EVT_SYS_FAIL, 1);
                retryCnt = 0;

                do
                {
                    pthread_mutex_lock(&g_mtxMqMainMsg);
                    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
                    g_stMainMsg.proc = PROC_TYPE_MQ;
                    g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
                    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
                    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0);
                    pthread_mutex_unlock(&g_mtxMqMainMsg);

                    retryCnt++;
                    //LOG_INFO("Retry to tx MAIN proc message queue (return:%d, retry count:%d)", rtn, retryCnt);
                } while((rtn < 0) && (retryCnt < IOT_RETRY_CNT));

                (rtn < 0) ? procMqStatusUpdate(IOT_EVT_SYS_RECOV_FAIL, 1) : procMqStatusUpdate(IOT_EVT_SYS_RECOVERED, 1);
            }

            threadCnt = 0;
        }

        usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
        threadCnt++;
    }

    return NULL;
}

int readFileForMq(char *path, int len, char* pbuf)
{
    int rtn = 0, retryCnt = 0;
    int fdscr = 0;

    fdscr = open(path, O_RDONLY);
    if(fdscr < 0) /* Fault inject num: 312 */
    {
        //LOG_WARN("MQ proc file open error : %s", path);
        procMqStatusUpdate(IOT_EVT_SYS_FAIL, 12);

        do
        {
            fdscr = open(path, O_RDONLY);
            retryCnt++;
        } while((fdscr < 0) && (retryCnt < IOT_RETRY_CNT));

        (fdscr < 0) ? procMqStatusUpdate(IOT_EVT_SYS_RECOV_FAIL, 12) : procMqStatusUpdate(IOT_EVT_SYS_RECOVERED, 12);
    }

    pbuf[len] = 0x20; /* space */
    pbuf[len + 1] = 0x00; /* NULL */

    rtn = read(fdscr, pbuf, len);
    close(fdscr);

    return rtn;
}

#ifdef OLDDATA
static void readFileAndMqttPublish(MqConfig *pMqCfg, IoTMqMsg *pMqMsg)
{
    static const char *TOPIC_NAME_DATA       = "data";
    static const char *TOPIC_NAME_OLD_DATA   = "arcdata";
    static const char *TOPIC_NAME_STATUS     = "status";
    static const char *TOPIC_NAME_OLD_STATUS = "arcstatus";
    static const char *TOPIC_NAME_LOCATION     = "loc";
    static const char *TOPIC_NAME_OLD_LOCATION = "arcloc";
    int len, rtn;
    const char *topic_name;

    long failBackupMsgType; // mqtt publish 실패시 메시지큐에 다시 백업한다

    switch (pMqMsg->msgtype)
    {
        case IOT_MQMSG_SENDDATA:
            topic_name = TOPIC_NAME_DATA;
            failBackupMsgType = IOT_MQMSG_OLDDATA;
            break;
        case IOT_MQMSG_OLDDATA:
            topic_name = TOPIC_NAME_OLD_DATA;
            failBackupMsgType = IOT_MQMSG_OLDDATA;
            break;
        case IOT_MQMSG_SENDSTAT:
            topic_name = TOPIC_NAME_STATUS;
            failBackupMsgType = IOT_MQMSG_OLDSTAT;
            break;
        case IOT_MQMSG_OLDSTAT:
            topic_name = TOPIC_NAME_OLD_STATUS;
            failBackupMsgType = IOT_MQMSG_OLDSTAT;
            break;
        case IOT_MQMSG_SENDLOC:
            topic_name = TOPIC_NAME_LOCATION;
            failBackupMsgType = IOT_MQMSG_OLDLOC;
            break;
        case IOT_MQMSG_OLDLOC:
            topic_name = TOPIC_NAME_OLD_LOCATION;
            failBackupMsgType = IOT_MQMSG_OLDLOC;
            break;
        default:
            return;
    }

    // message로 전달된 file path를 읽어서, g_s8MqTxFileBuf에 옮긴다.
    len = readFileForMq(pMqMsg->path, pMqMsg->len, g_s8MqTxFileBuf);
    if(len > 0)
    {
        // g_s8MqTxFileBuf의 데이터를 mqttTopicType의 topic으로 mqtt server에 배포한다.
        rtn = mqtt_publish(pMqCfg, topic_name, g_s8MqTxFileBuf, len);

        if(MQ_RET_SUCCESS != rtn)
        {
            // publish 실패시,
            //LOG_WARN("MQTT publish failed: topic = %s, err = %d", topic_name, rtn);
            // 메시지큐에 failBackupMsgType 메시지키로 메시지를 다시 넣고,
            pMqMsg->msgtype = failBackupMsgType;
            msgsnd(g_s32MqMsgID, pMqMsg, sizeof(IoTMqMsg) - sizeof(long), IPC_NOWAIT);
            // MQ 프로세스 상태를 Main 프로세스로 보내서 업데이트.
            procMqStatusUpdate(IOT_EVT_MQ_FAIL, 17);
        }
        else
        {
            /*
            //LOG_INFO("MQ proc state count: %d, data count: %d, old state count: %d, old data count: %d",
                     g_u32MqStatCnt, g_u32MqDataCnt, g_u32MqOldStatCnt, g_u32MqOldDataCnt);
            */
        }
    }
}
#else
// publish 실패한 경우, 같은 메시지 키로 다시 메시지큐에 넣는다.
static void readFileAndMqttPublish(MqConfig *pMqCfg, IoTMqMsg *pMqMsg)
{
    int len, rtn;
    const char *topic_name;

    memset(g_s8MqTxFileBuf, 0, sizeof(g_s8MqTxFileBuf));

    switch (pMqMsg->msgtype)
    {
        case IOT_MQMSG_SENDSTAT:
            topic_name = TOPIC_NAME_STATUS;
            len = pMqMsg->len;
            memcpy(g_s8MqTxFileBuf, pMqMsg->data.json_string ,len);
            break;

        case IOT_MQMSG_SENDDATA:
            topic_name = TOPIC_NAME_DATA;
            // message로 전달된 file path를 읽어서, g_s8MqTxFileBuf에 옮긴다.
            len = readFileForMq(pMqMsg->data.path, pMqMsg->len, g_s8MqTxFileBuf);
            break;

        case IOT_MQMSG_SENDLOC:
            topic_name = TOPIC_NAME_LOCATION;
            // message로 전달된 file path를 읽어서, g_s8MqTxFileBuf에 옮긴다.
            len = readFileForMq(pMqMsg->data.path, pMqMsg->len, g_s8MqTxFileBuf);
            break;

        case IOT_MQMSG_SENDSOH:
            topic_name = TOPIC_NAME_SOH;
            // message로 전달된 file path를 읽어서, g_s8MqTxFileBuf에 옮긴다.
            len = readFileForMq(pMqMsg->data.path, pMqMsg->len, g_s8MqTxFileBuf);
            break;

        default:
            return;
    }

    if(len > 0)
    {
        // g_s8MqTxFileBuf의 데이터를 mqttTopicType의 topic으로 mqtt server에 배포한다.
        rtn = mqtt_publish(pMqCfg, topic_name, g_s8MqTxFileBuf, len);

        if(MQ_RET_SUCCESS != rtn)
        {
            // publish 실패시,
            //LOG_WARN("MQTT publish failed: topic = %s, err = %d", topic_name, rtn);
            // 메시지큐에 메시지를 다시 넣고(맨 뒤로 가게됨)
            msgsnd(g_s32MqMsgID, pMqMsg, sizeof(IoTMqMsg) - sizeof(long), IPC_NOWAIT);
            // MQ 프로세스 상태를 Main 프로세스로 보내서 업데이트.
            procMqStatusUpdate(IOT_EVT_MQ_FAIL, 17);
        }
        else
        {
            /*
            //LOG_INFO("MQ proc state count: %d, data count: %d, old state count: %d, old data count: %d",
                     g_u32MqStatCnt, g_u32MqDataCnt, g_u32MqOldStatCnt, g_u32MqOldDataCnt);
            */
        }
    }
}
#endif // OLDDATA

void onMqMessage(const char *topic, int topic_len, const char *data, int data_len)
{
    DBGPRT("onMqMessage: topic= %.*s ,payload= \n%.*s \n", topic_len, topic, data_len, data);
}


static void *mq_proc_thread(void *arg)
{
    int rtn = 0;
    bool mqttConnected = false;

    while(1)
    {
        // TERMINATE 메시지 우선 처리
        rtn = msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(0), IOT_MQMSG_TERMINATE, IPC_NOWAIT);
        if(FAIL != rtn)
        {
            // removeMsgQueue(g_s32MqMsgID);
            // mqtt_disconnect(&g_stMqCfg);

            g_stMainMsg.msgtype = IOT_MAINMSG_TERMRSP;
            g_stMainMsg.proc = PROC_TYPE_MQ;
            if(FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0))
            {
                //LOG_INFO("Failed send message to MAIN proc message queue");
            }

            //LogClose();
            return 0;
        }

        // MQTT 서버와 연결 상태 확인 및 상태 변경시 처리
        if (mqttConnected != mqtt_connected(&g_stMqCfg))
        {
            mqttConnected = !mqttConnected;
            if (mqttConnected == true)
            {
                //LOG_INFO("MQ MQTT Connected");
                DBGPRT("MQ Proc : changed to connected !! \n");
                procMqStatusUpdate(IOT_EVT_MQ_RECOVERED, 3);
            }
            else
            {
                //LOG_INFO("MQ MQTT Disconnected");
                DBGPRT("MQ Proc : changed to DISconnected !! \n");
                procMqStatusUpdate(IOT_EVT_MQ_FAIL, 3);

                pthread_mutex_lock(&g_mtxMqMainMsg);
                g_stMainMsg.msgtype = IOT_MAINMSG_COMMQOS;
                g_stMainMsg.proc = PROC_TYPE_MQ;
                g_stMainMsg.data[DATA_TYPE_EVENT] = 1;
                g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;

                if(msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0) < 0)
                {
                    //LOG_INFO("Failed send message to main task message queue");
                }
                pthread_mutex_unlock(&g_mtxMqMainMsg);
            }
        }

        // MQTT 서버와 끊김 상태 처리
        if(mqttConnected == false)
        {
            // mqtt_connect가 1초정도 주기로 호출되도록 함.
            usleep(1000 * USEC_TO_MILISEC);
            DBGPRT("MQ Proc : MQ server connect call !! \n");

            // mqtt_connect blocking 호출 (연결 성공 혹은 실패 결과 후 리턴됨)
            rtn = mqtt_connect(&g_stMqCfg);
            if(SUCCESS != rtn)
            {
                DBGPRT("MQ Proc : MQ server connect failed !! \n");
                //LOG_WARN("mqtt_connect failed: %d", rtn);
            }

#if 0
            // command topic을 subscribe한다.
            rtn = mqtt_subscribe(&g_stMqCfg, TOPIC_NAME_COMMAND, onMqMessage);
            if(SUCCESS != rtn)
            {
                DBGPRT("MQ Proc : MQ server subscribe failed !! \n");
                //LOG_WARN("mqtt_subscribe failed: %d", rtn);
            }
#endif
        }
        // MQTT 서버와 연결 상태 처리
        else
        {
            /* 1. Send Status to MQTT server*/
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg)-sizeof(long), IOT_MQMSG_SENDSTAT, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }

            /* 2. Send Data to MQTT server*/
            if(mqtt_connected(&g_stMqCfg))
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IOT_MQMSG_SENDDATA, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }

            /* 3. Send Location to MQTT server*/
            if(mqtt_connected(&g_stMqCfg))
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IOT_MQMSG_SENDLOC, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }

#ifdef OLDDATA
            /* 4. Send Old Status to MQTT server*/
            if(mqtt_connected(&g_stMqCfg))
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IOT_MQMSG_OLDSTAT, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }

            /* 5. Send Old Data to MQTT server*/
            if(mqtt_connected(&g_stMqCfg))
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IOT_MQMSG_OLDDATA, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }

            /* 6. Send Old Data to MQTT server*/
            if(mqtt_connected(&g_stMqCfg))
            if(FAIL != msgrcv(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IOT_MQMSG_OLDLOC, IPC_NOWAIT))
            {
                readFileAndMqttPublish(&g_stMqCfg, &g_stMqMsg);
            }
#endif
        }

        // mqtt_loop를 호출하여 mqtt 콜백 함수 등 남은 mqtt 작업들이 처리되도록 함.
        mqtt_loop(&g_stMqCfg);

        // 너무 빠른 루프 실행을 방지하기 위해 sleep 호출
        usleep(10 * USEC_TO_MILISEC);
    }
    return NULL;
}
int mq_main(void)
{
    int rtn = 0;
    bool mqttConnected = false;
    pthread_t aliveThreadID, procThreadID;

//    LogSetInfo(LOG_FILE_PATH, "IoTMq"); /* Initialize logging */
    //LOG_INFO("Start IoT MQ Proc");

    initMsgQueue(); /* Initialize message queue */

    /* Send result message to Main Proc */
    g_stMainMsg.msgtype = IOT_MAINMSG_PROCRSP;
    g_stMainMsg.proc = PROC_TYPE_MQ;
    g_stMainMsg.data[0] = SUCCESS;

    if(FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg)-sizeof(long), 0)) /* Fault inject num: 313 */
    {
        //LOG_WARN("Failed send proc resp message to main proc message queue");
        reboot_system();
    }

    pthread_mutex_init(&g_mtxMqMainMsg, NULL);
    pthread_create(&aliveThreadID, 0, mq_alive_check_thread, NULL); /* Alive thread to report status */

    // MQ 설정 불러오기
    loadMqConfigure(&g_stMqCfg);

    // MQTT Client 초기화 및 설정
    rtn = mqtt_init(&g_stMqCfg);
    if(0 != rtn) /* Fault inject num: 314 */
    {
        procMqStatusUpdate(IOT_EVT_INIT_UNRECOVER, 14);
    }

    // Main 프로세스에게 초기화 완료 알림
    procMqStatusUpdate(IOT_EVT_INIT_SUCCESS, 0);
    pthread_create(&procThreadID, 0, mq_proc_thread, NULL);

}

