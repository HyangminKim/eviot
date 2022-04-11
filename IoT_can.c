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
**   $FILNAME   : IoT_can.c                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 17, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains CAN Handler for Rack BMS implementation.           **
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
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>

#include <config/libconfig.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_dc.h"

#include "IoT_can.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define CAN_RACKID_CNT  0x10

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/
enum RACK_CAN_MSG_IDX
{
    CAN_PERIOD_MSG_IDX  = 0, /* ID - 0x12 */
    CAN_SYNC01_MSG_IDX  = 1, /* ID - 0x14 */
    CAN_SYNC02_MSG_IDX  = 2, /* ID - 0x15 */
    CAN_SYNC03_MSG_IDX  = 3, /* ID - 0x16 */
    CAN_SYNC04_MSG_IDX  = 4, /* ID - 0x17 */

    CAN_ONDEM01_MSG_IDX = 5, /* ID - 0x1B */
    CAN_ONDEM02_MSG_IDX = 6, /* ID - 0x1C */
    CAN_ONDEM03_MSG_IDX = 7, /* ID - 0x1D */
    CAN_ONDEM04_MSG_IDX = 8, /* ID - 0x1E */

    CAN_ONDEM06_MSG_IDX = 9  /* ID - 0x19 */
};

typedef void (*IoTCan_ReportEvt_CBFunc)(int, int);

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/
//static const unsigned int g_u32RackFiltedCanID[CAN_MAX_MSGID_CNT] = {0x12, 0x14, 0x15, 0x16, 0x17, 0x1B, 0x1C, 0x1D, 0x1E, 0x19};
//static const unsigned int g_u32RfiltedCanID[CAN_MAX_ID_CNT] = { 0x4D0, 0x4D1,
//                                                                0x4D2, 0x4D3,
//                                                                0x4D4, 0x4D5,
//                                                                0x4D6, 0x4D7,
//                                                                0x4D8, 0x4D9,
//                                                                0x4DA, 0x4DB,
//                                                                0x4DC, 0x4DD,
//                                                                0x4DE, 0x4DF,
//                                                                0x4E0, 0x4E1,
//                                                                0x4E2, 0x4E3,
//                                                                0x4E4, 0x4E5,
//                                                                0x4E6, 0x4E7,
//                                                                0x4E8, 0x4E9,
//                                                                0x4EA, 0x4EB,
//                                                                0x4EC, 0x4ED,
//                                                                0x4EE, 0x4EF,
//                                                                0x4F0, 0x4F1,
//                                                                0x4F2 };
static const unsigned int g_u32RfiltedCanID[CAN_MAX_ID_CNT] = { 0x4D0, 0x4D1,
                                                                0x4D2, 0x4D3,
                                                                0x4D4, 0x4D5,
                                                                0x4D6, 0x4D7,
                                                                0x4D8, 0x4D9,
                                                                0x4DA, 0x4DB,
                                                                0x4DC, 0x4DD,
                                                                0x4DE, 0x4DF,
                                                                0x4E7, 0x4E8,
                                                                0x4EB, 0x4EC,
                                                                0x4ED, 0x4EE,
                                                                0x4EF, 0x4F0,
                                                                0x4F1, 0x4F2 };
static const unsigned int g_u32RackMaxCntCanID[CAN_MAX_MSGID_CNT] = {1, 1, 1, 1, 1, 1,
                                                                     1, 1, 1, 1, 1, 1,
                                                                     1, 1, 1, 1, 1, 1,
                                                                     1, 1, 1, 1, 1, 1,
                                                                     1, 1};

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static RespBmsCanInfo g_stCanInfo;
static IoTCan_ReportEvt_CBFunc g_funcCanReportEvtCB;


/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/
static uint64_t getMSecTimeStamp(void)
{
    uint64_t nowMS = 0;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    nowMS = ((uint64_t)tv.tv_sec) * 1000 + ((uint64_t)tv.tv_usec) / 1000;

    return nowMS;
}

static void reportCanEvt(int evt, int idx)
{
    if(NULL != g_funcCanReportEvtCB)
    {
        g_funcCanReportEvtCB(evt, idx);
    }
}

static void initRespData(RxCanDataInfo *pRxCanData)
{
    memset(&pRxCanData->respData, 0x00, sizeof(RespDataInfo));
    pRxCanData->rxMsgCnt = 0;
    pRxCanData->bDoneRxData = false;
    pRxCanData->bFirstFlg = true;

}
static void initReqData(TxCanDataInfo *pTxCanData)
{
    memset(&pTxCanData->reqData, 0x00, sizeof(ReqDataInfo));
    pTxCanData->txMsgCnt = 0;
}

static void initCanConfigure(void)
{
    CanSettingInfo *pCanSetting = NULL;

    memset(&g_stCanInfo, 0x00, sizeof(RespBmsCanInfo));

    pCanSetting = &g_stCanInfo.canSetting;

    pCanSetting->can_s = -1;
    pCanSetting->rack_total_cnt = RBMS_MAX_RACK_CNT;
    pCanSetting->rack_msg_max_cnt = CAN_MAX_MSGNO_CNT;

    memset(pCanSetting->devInfo.devName, 0x00, CAN_DEV_NAME_SZ);
    strncpy(pCanSetting->devInfo.devName, IOT_DEV_CAN, strlen(IOT_DEV_CAN) + 1);
    pCanSetting->devInfo.bitrate = 500000;

    pCanSetting->bChkBufOff = false;
    pCanSetting->nOvfCnt = 0;

    initRespData(&g_stCanInfo.rxCanData);
    initReqData(&g_stCanInfo.txCanData);

    pthread_mutex_init(&g_stCanInfo.dataMutex, NULL);
}

static int loadCanConfigure(void)
{
    config_t cfg;
    config_setting_t *setting = NULL;

    const char *strcfg = NULL;
    char cansyscmd[CAN_SYS_CMD_SZ] = {0, };

    config_init(&cfg);
    if(CONFIG_FALSE == config_read_file(&cfg, IOT_CONFIG_FILE_PATH)) /* Fault inject num: 131 */
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("DC CAN Proc - config file read error(%s:%d - %s)",
//                 config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 31);
        config_destroy(&cfg);

        return FAIL;
    }
    else
    {
        setting = config_lookup(&cfg, "bank");
        if(NULL != setting)
        {
            /* configure: count */
            if(0 == config_setting_lookup_int(setting, "count", &g_stCanInfo.canSetting.rack_total_cnt)) /* Fault inject num: 132 */
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Can't find rack count");
                reportCanEvt(IOT_EVT_INIT_UNRECOVER, 32);
            }

            /* configure: message count */
            if(0 == config_setting_lookup_int(setting, "msg_cnt", &g_stCanInfo.canSetting.rack_msg_max_cnt)) /* Fault inject num: 133 */
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Can't find rack message count");
                reportCanEvt(IOT_EVT_INIT_UNRECOVER, 33);
            }
        }

        setting = config_lookup(&cfg, "can");
        if(NULL != setting)
        {
            /* configure: dev */
            if(0 != config_setting_lookup_string(setting, "dev", &strcfg)) /* Fault inject num: 134 */
            {
                snprintf(g_stCanInfo.canSetting.devInfo.devName, CAN_DEV_NAME_SZ, strcfg);
            }
            else
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Can't find CAN channel");
                reportCanEvt(IOT_EVT_INIT_UNRECOVER, 34);
            }

            /* configure: baudrate */
            if(0 == config_setting_lookup_int(setting, "bitrate", &g_stCanInfo.canSetting.devInfo.bitrate)) /* Fault inject num: 135 */
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Can't find CAN baudrate");
                reportCanEvt(IOT_EVT_INIT_UNRECOVER, 35);
            }
        }

        config_destroy(&cfg);
//        LOG_INFO("CAN configuration [CAN dev: %s, baudrate: %d, rack count: %d, message count: %d]",
//                 g_stCanInfo.canSetting.devInfo.devName, g_stCanInfo.canSetting.devInfo.bitrate, g_stCanInfo.canSetting.rack_total_cnt, g_stCanInfo.canSetting.rack_msg_max_cnt);
    }

    // snprintf(cansyscmd, CAN_SYS_CMD_SZ, "sudo ip link set %s type can bitrate %d", g_stCanInfo.canSetting.devInfo.devName, g_stCanInfo.canSetting.devInfo.bitrate);
    snprintf(cansyscmd, CAN_SYS_CMD_SZ, "ip link set %s type can bitrate %d", g_stCanInfo.canSetting.devInfo.devName, g_stCanInfo.canSetting.devInfo.bitrate);
    //snprintf(cansyscmd, CAN_SYS_CMD_SZ,"tmppwd");
    system((const char *)cansyscmd);

    return SUCCESS;
}

static void *check_can_busoff_thread(void *data)
{
    int chkCnt = (int)(CAN_CHK_BUS_OFF_TIME / 10); /* check 500ms */

    while(chkCnt >= 0)
    {
        if(false == g_stCanInfo.canSetting.bChkBufOff) /* change bus-off to restart */
        {
            break;
        }

        usleep(10 * USEC_TO_MILISEC); /* 10ms */
        chkCnt--;
    }

    if(chkCnt < 0)
    {
//        LOG_ERR("CAN BUS OFF-Failed recovery sequence. Reset");
        reportCanEvt(IOT_EVT_CAN_RECOV_FAIL, 23);
        g_stCanInfo.bRunning = false;
//        g_stCanInfo.bTxRunning = false;

        IoTCan_Close();
    }
    else
    {
//        LOG_INFO("CAN BUS OFF-Recovery sequence Success!!");
        reportCanEvt(IOT_EVT_CAN_RECOVERED, 23);
    }

    return NULL;
}

static void *check_can_overflow_thread(void *data)
{
    int ovfCntPrv = 0;
    int chkCnt = (int)(CAN_CHK_OVERFLOW_TIME / 10); /* check 500ms */

    while(chkCnt >= 0)
    {
        ovfCntPrv = g_stCanInfo.canSetting.nOvfCnt;
        usleep(10 * USEC_TO_MILISEC); /* 10ms */

        if(ovfCntPrv == g_stCanInfo.canSetting.nOvfCnt) /* if overflow count is not changed then it will recover */
        {
            break;
        }

        chkCnt--;
    }

    if(chkCnt < 0)
    {
//        LOG_ERR("CAN OVERFLOW-Failed recovery sequence. Reset");
        reportCanEvt(IOT_EVT_CAN_RECOV_FAIL, 24);
        g_stCanInfo.bRunning = false;
//        g_stCanInfo.bTxRunning = false;

        IoTCan_Close();
    }
    else
    {
//        LOG_INFO("CAN OVERFLOW-Recovery sequence Success!!");
        reportCanEvt(IOT_EVT_CAN_RECOVERED, 24);
        g_stCanInfo.canSetting.nOvfCnt = 0;
    }

    return NULL;
}

static bool chkSkipCanData(int rx_msg_cnt, unsigned int can_id)
{
    static int rxCanMsgCnt[CAN_MAX_MSGID_CNT] = {0, };

    bool bSkip = false;
    int idx = 0;

    /* Ignore all received CAN message until rx_msg_cnt is zero */
    if(0 == rx_msg_cnt)
    {
        memset(rxCanMsgCnt, 0x00, sizeof(rxCanMsgCnt));
        /* LOG_INFO("Initialize received CAN message count"); */
    }

    for(idx = 0; idx < CAN_MAX_MSGID_CNT; idx++)
    {
        if((can_id & 0xFFF) == (g_u32RfiltedCanID[idx] & 0xFFF))
        {
//            printf("%x %x %x %x\n", can_id,g_u32RfiltedCanID[idx] , (can_id & 0x0F) ,(g_u32RfiltedCanID[idx] & 0x0F));
            rxCanMsgCnt[idx]++;
            bSkip = (rxCanMsgCnt[idx] > g_u32RackMaxCntCanID[idx]) ? true : false;
//            printf("11skip : %d\n", bSkip);
            break;
        }
    }

    return bSkip;
}

static void handle_recevied_canerror(struct can_frame *pframe)
{
    CanSettingInfo *pCanSetting = NULL;
    pCanSetting = &g_stCanInfo.canSetting;

    if(pframe->can_id & CAN_ERR_BUSOFF) /* Fault inject num: 123, 125 */
    {
        g_s32DcFaultInjNum = 0;
//        LOG_ERR("Error: CAN BUS OFF");
        reportCanEvt(IOT_EVT_CAN_FAIL, 25);

        if(false == pCanSetting->bChkBufOff)
        {
            pthread_mutex_lock(&g_stCanInfo.dataMutex);

            pCanSetting->bChkBufOff = true;

            pthread_mutex_unlock(&g_stCanInfo.dataMutex);
            pthread_create(&pCanSetting->busoff_chk_threadID, NULL, check_can_busoff_thread, NULL);
        }
    }

    if(pframe->can_id & CAN_ERR_RESTARTED)
    {
        pCanSetting->bChkBufOff = false;
//        LOG_INFO("CAN Restarted!");
    }

    if(pframe->can_id & CAN_ERR_CRTL) /* Fault inject num: 124, 126, 127 */
    {
        if(pframe->data[1] & CAN_ERR_CRTL_RX_OVERFLOW) /* Fault inject num: 124, 126 */
        {
            g_s32DcFaultInjNum = 0;
//            LOG_ERR("Error: CAN Rx Buffer Overflow");
            reportCanEvt(IOT_EVT_CAN_FAIL, 26);

            if(0 == pCanSetting->nOvfCnt)
            {
                pthread_mutex_lock(&g_stCanInfo.dataMutex);

                pCanSetting->nOvfCnt++;

                pthread_mutex_unlock(&g_stCanInfo.dataMutex);
                pthread_create(&pCanSetting->ovf_chk_threadID, NULL, check_can_overflow_thread, NULL);
            }
            else
            {
                pCanSetting->nOvfCnt++;
            }
        }

        if(pframe->data[1] & CAN_ERR_CRTL_TX_OVERFLOW) /* Fault inject num: 127 */
        {
            g_s32DcFaultInjNum = 0;
//            LOG_ERR("Error: CAN Tx Buffer Overflow");
            reportCanEvt(IOT_EVT_CAN_FAIL, 27);

            if(0 == pCanSetting->nOvfCnt)
            {
                pthread_mutex_lock(&g_stCanInfo.dataMutex);

                pCanSetting->nOvfCnt++;

                pthread_mutex_unlock(&g_stCanInfo.dataMutex);
                pthread_create(&pCanSetting->ovf_chk_threadID, NULL, check_can_overflow_thread, NULL);
            }
            else
            {
                pCanSetting->nOvfCnt++;
            }
        }

        if(pframe->data[1] & CAN_ERR_CRTL_RX_PASSIVE)
        {
//            LOG_DBG("CAN RX Passive");
        }

        if(pframe->data[1] & CAN_ERR_CRTL_TX_PASSIVE)
        {
//            LOG_DBG("CAN TX Passive");
        }
    }
}
static void handle_recevied_candata(struct can_frame *pframe)
{
    static bool bFirstMsg = false;
    static struct timeval stTimes, edTimes;

    int i = 0, data_idx = 0;
    int rack_align_data_idx = 0;
    bool bSkipCanData = false;

    RxCanDataInfo *pRcvCanData = NULL;
    RespDataInfo *pRespData = NULL;
    unsigned char *pData = NULL;

    if (false == bFirstMsg)
    {
        /* First CAN Message --> 0x4D0 */
        bFirstMsg = (pframe->can_id == g_u32RfiltedCanID[0]) ? true : false;
        if (false == bFirstMsg)
        {
            return;
        }
        gettimeofday(&stTimes, NULL);
    }

    data_idx = (pframe->can_id - g_u32RfiltedCanID[0]);

    pthread_mutex_lock(&g_stCanInfo.dataMutex);

    pRcvCanData = &g_stCanInfo.rxCanData;
    pRcvCanData->bFirstFlg = true;

    if (true == pRcvCanData->bDoneRxData)
    {
        //LOG_INFO("Not yet send rack data to server [%d:%d], (%d)", g_stCanInfo.rackSendFifoCnt[rack_idx], rack_fifo_idx, rack_idx + 1);
        pthread_mutex_unlock(&g_stCanInfo.dataMutex);
        return;
    }

//    bSkipCanData = chkSkipCanData(pRcvCanData->rxMsgCnt, pframe->can_id);
//    if (true == bSkipCanData)
//    {
//        //LOG_INFO("Skip CAN ID: 0x%04X", pframe->can_id);
//        printf("Skip CAN ID: 0x%04X\n", pframe->can_id);
//        pthread_mutex_unlock(&g_stCanInfo.dataMutex);
//        return;
//    }

    rack_align_data_idx = data_idx * CAN_DATA_ALIGN_SZ;
    pRespData = &pRcvCanData->respData;

    pData = (unsigned char *) ((unsigned char *) pRespData + rack_align_data_idx);

    for (i = 0; i < pframe->can_dlc; i++)
    {
        pData[i] = pframe->data[i];
    }

    pRcvCanData->rxMsgCnt++;

    if (pRcvCanData->rxMsgCnt >= CAN_MAX_ID_CNT)
    {
        pRcvCanData->bDoneRxData = true;
        pRcvCanData->dataDoneTime = getMSecTimeStamp();

        gettimeofday(&edTimes, NULL);
    }

    pthread_mutex_unlock(&g_stCanInfo.dataMutex);

    if (0x01 == pRcvCanData->rxMsgCnt) /* First CAN message is received */
    {
        gettimeofday(&stTimes, NULL);
        memcpy(&pRcvCanData->startTime, &stTimes, sizeof(struct timeval));
    }
}

static void *can_rx_data_thread(void *data)
{
    int nBytes = 0, retryCnt = 0;
    bool bReceived = false, bFirstWait = false;
    struct can_frame frame;

    while(false == g_stCanInfo.bRunning)
    {
        /* wait.. */
        usleep(100 * USEC_TO_MILISEC);
    }

    while(true == g_stCanInfo.bRunning)
    {
        nBytes = read(g_stCanInfo.canSetting.can_s, &frame, sizeof(struct can_frame));

        if(nBytes != sizeof(struct can_frame)) /* Fault inject num: 128, 129, 130 */
        {
            perror("Read");
            bReceived = false;
            if((false == bFirstWait) && (errno == ENETDOWN))
            {
                bFirstWait = true;
                usleep(10 * USEC_TO_MILISEC);
                continue;
            }

            reportCanEvt(IOT_EVT_CAN_FAIL, 28);
            retryCnt = 0;

            do
            {
                if(nBytes < 0) /* Fault inject num: 128, 129 */
                {
                    g_s32DcFaultInjNum = 0;

                    if(errno == ENETDOWN) /* Fault inject num: 128 */
                    {
                        LOG_ERR("CAN network is down, errno: %d(%s)", errno, strerror(errno));
                    }
                    else /* Fault inject num: 129 */
                    {
                        LOG_ERR("Failed read CAN frame errno: %d (%s)", errno, strerror(errno));
                    }
                }
                else
                {
                    if(nBytes != sizeof(struct can_frame)) /* Fault inject num: 130 */
                    {
                        g_s32DcFaultInjNum = 0;
                        LOG_ERR("Failed incomplete CAN frame errno: %d (%s)", errno, strerror(errno));
                    }

                    retryCnt++;
                    usleep(USEC_TO_MILISEC);

                    nBytes = read(g_stCanInfo.canSetting.can_s, &frame, sizeof(struct can_frame));
                }

            } while((nBytes != sizeof(struct can_frame)) && (retryCnt < IOT_RETRY_CNT));

            if(nBytes < 0) /* CAN network is down */
            {
                (errno == ENETDOWN) ? reportCanEvt(IOT_EVT_CAN_RECOV_FAIL, 28) : reportCanEvt(IOT_EVT_CAN_RECOV_FAIL, 29);
            }
            else if(nBytes != sizeof(struct can_frame)) /* CAM message size is mismatched */
            {
                reportCanEvt(IOT_EVT_CAN_RECOV_FAIL, 30);
            }
            else /* CAN network is recovered */
            {
                reportCanEvt(IOT_EVT_CAN_RECOVERED, 28);
                bReceived = true;
            }
        }
        else
        {
            bReceived = true;
        }

        if(true == bReceived)
        {
            if(frame.can_id & CAN_ERR_FLAG) /* Fault inject num: 123, 124, 125, 126, 127 */
            {
                handle_recevied_canerror(&frame);
            }
            else
            {
                handle_recevied_candata(&frame);
            }
        }
    }

    g_stCanInfo.bRunning = false;

    return NULL;
}

static void *can_tx_data_thread(void *data)
{
    struct can_frame frame;

    int i = 0;
    int date[8] = { 0, };
    g_stCanInfo.stopDaq  = 0x0000;

    while(false == g_stCanInfo.bTxRunning)
     {
         /* wait.. */
         usleep(100 * USEC_TO_MILISEC);
     }

    while(true == g_stCanInfo.bTxRunning)
    {
        time_t now = time(NULL);
        struct tm *tm_struct = localtime(&now);

        date[0]  = g_stCanInfo.stopDaq; //0x0010;
        date[1] = tm_struct->tm_sec;
        date[2] = tm_struct->tm_min;
        date[3] = tm_struct->tm_hour;
        date[4] = tm_struct->tm_mday;
        date[5] = tm_struct->tm_wday;
        date[6] = tm_struct->tm_mon + 1;
        date[7] = tm_struct->tm_year + 1900 - 2000;

        frame.can_id = 0x4C0;
        frame.can_dlc = 8;

        for (i = 0; i < frame.can_dlc; i++)
        {
            frame.data[i] = (date[i]) ;
        }

        if (write(g_stCanInfo.canSetting.can_s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame))
        {
            perror("Write");
        }
        else
        {
            usleep(1000 * USEC_TO_MILISEC);
    #if true
            if(!g_stCanInfo.rxCanData.bFirstFlg)
            {
                g_stCanInfo.stopDaq = 0x0003;
                printf("##POWER_OFF : FAIL GET BMS DATA 1SEC AFTER REQUEST\n");
//                LOG_INFO("##POWER_OFF : FAIL GET BMS DATA 1SEC AFTER REQUEST");
                procDCStatusUpdate_Extern(IOT_EVT_POWER_OFF, 0);
            }
    #endif
            g_stCanInfo.stopDaq +=  0x0010;
            if( g_stCanInfo.stopDaq > 0x00F0)
                g_stCanInfo.stopDaq = 0x0000;
        }

    }

    g_stCanInfo.bTxRunning = false;

    return NULL;
}

int IoTCan_SetCallBackFunc(void *cbfunc)
{
    g_funcCanReportEvtCB = cbfunc;

    return SUCCESS;
}

RespBmsCanInfo *IoTCan_GetRespBmsCanInfo(void)
{
    return &g_stCanInfo;
}

int IoTCan_UpDownLink(bool up)
{
    char syscmd[CAN_SYS_CMD_SZ] = {0, };

    if(true == up)
    {
        // snprintf(syscmd, CAN_SYS_CMD_SZ, "sudo ip link set %s up", g_stCanInfo.canSetting.devInfo.devName);
        snprintf(syscmd, CAN_SYS_CMD_SZ, "ip link set %s up", g_stCanInfo.canSetting.devInfo.devName);
    }
    else
    {
        // snprintf(syscmd, CAN_SYS_CMD_SZ, "sudo ip link set %s down", g_stCanInfo.canSetting.devInfo.devName);
        snprintf(syscmd, CAN_SYS_CMD_SZ, "ip link set %s down", g_stCanInfo.canSetting.devInfo.devName);
    }
    system((const char *)syscmd);

    return SUCCESS;
}

int IoTCan_Open(void)
{
    int respID = 0;
    int idx = 0, respIdx = 0;

    CanSettingInfo *pCanSetting = NULL;

    struct ifreq ifr;
    struct sockaddr_can addr;

    struct can_filter rfilter[RBMS_MAX_RACK_CNT * CAN_MAX_MSGID_CNT];
    can_err_mask_t err_mask = CAN_ERR_MASK;
    int loopback = 0;

    initCanConfigure();
    if(FAIL == loadCanConfigure()) /* Fault inject num: 146 */
    {
        g_s32DcFaultInjNum = 0;
        LOG_WARN("Failed load configuration for CAN");
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 46);

        return FAIL;
    }

    IoTCan_UpDownLink(false);

    pCanSetting = &g_stCanInfo.canSetting;
    if((pCanSetting->can_s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) /* Fault inject num: 148 */
    {
        g_s32DcFaultInjNum = 0;
        LOG_ERR("Failed create CAN socket errno: %d (%s)", errno, strerror(errno));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 48);

        return FAIL;
    }

    memset(&addr, 0x00, sizeof(addr));
    memset(&ifr.ifr_name, 0x00, sizeof(ifr.ifr_name));

    strncpy(ifr.ifr_name, pCanSetting->devInfo.devName, strlen(pCanSetting->devInfo.devName));
    if(ioctl(pCanSetting->can_s, SIOCGIFINDEX, &ifr) < 0) /* Fault inject num: 149 */
    {
        g_s32DcFaultInjNum = 0;
        close(pCanSetting->can_s);

        LOG_ERR("Faild get ifr index errno: %d (%s)", errno, strerror(errno));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 49);
        return FAIL;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    /* Filter CAN ID to take receiving */
    respID = 0x00;
    for(respIdx = 0; respIdx < RBMS_MAX_RACK_CNT; respIdx++)
    {
        for (idx = 0; idx < CAN_MAX_MSGID_CNT; idx++)
        {
            rfilter[(respIdx * CAN_MAX_MSGID_CNT) + idx].can_id = g_u32RfiltedCanID[idx] + respID;
            rfilter[(respIdx * CAN_MAX_MSGID_CNT) + idx].can_mask = CAN_SFF_MASK;
        }

        respID += CAN_RACKID_CNT;
    }

    setsockopt(pCanSetting->can_s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
    setsockopt(pCanSetting->can_s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask));

    /* Disable loopback */
    setsockopt(pCanSetting->can_s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));

    if(bind(pCanSetting->can_s, (struct sockaddr *)&addr, sizeof(addr)) < 0) /* Fault inject num: 150 */
    {
        g_s32DcFaultInjNum = 0;
        close(pCanSetting->can_s);
        LOG_ERR("bind error. errno: %d (%s)", errno, strerror(errno));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 50);

        return FAIL;
    }

    g_stCanInfo.bRunning = false;
    g_stCanInfo.bTxRunning = false;
    if(pthread_create(&g_stCanInfo.rx_can_threadID, NULL, can_rx_data_thread, NULL) < 0) /* Fault inject num: 151 */
    {
        g_s32DcFaultInjNum = 0;
        close(pCanSetting->can_s);
        LOG_ERR("Failed to create can_recv thread: %d (%s)", errno, strerror(errno));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 51);

        return FAIL;
    }

    if(pthread_create(&g_stCanInfo.tx_can_threadID, NULL, can_tx_data_thread, NULL) < 0) /* Fault inject num: 151 */
    {
        g_s32DcFaultInjNum = 0;
        close(pCanSetting->can_s);
        LOG_ERR("Failed to create can_recv thread: %d (%s)", errno, strerror(errno));
        reportCanEvt(IOT_EVT_INIT_UNRECOVER, 51);

        return FAIL;
    }

    return SUCCESS;
}

int IoTCan_Start(void)
{
    IoTCan_UpDownLink(true);
    g_stCanInfo.bRunning = true;
    g_stCanInfo.bTxRunning = true;
    return SUCCESS;
}

int IoTCan_Stop(void)
{
    IoTCan_UpDownLink(false);
    g_stCanInfo.bRunning = false;
    g_stCanInfo.bTxRunning = false;

    return SUCCESS;
}

int IoTCan_Close(void)
{
    IoTCan_Stop();
    usleep(200 * USEC_TO_MILISEC); /* wait for finishing thread */

    if(g_stCanInfo.canSetting.can_s >= 0)
    {
        close(g_stCanInfo.canSetting.can_s);
    }

    return SUCCESS;
}
