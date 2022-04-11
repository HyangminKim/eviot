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
**   $FILNAME   : IoT_main.c                                                            **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 18, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains main control implementation for all processes.     **
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
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <net/if.h>

#include <config/libconfig.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_dc.h"
#include "IoT_stor.h"
#include "IoT_fota.h"

#include "IoT_main.h"
#include "IoT_mq.h"
#include "IoT_gps.h"
#include "IoT_led.h"
#include "IoT_debug.h"
// #include "IoT_hello.h"   // Hello Process Template

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define MAX_NORSP_LIMIT     2

#define MAX_PROC_PATH       30
#define MAX_PROC_PATH_EXE   50
#define MAX_WAIT_STAT_UPD   10 /* 10 seconds */
#define MAX_GLBSYNC_TIMECNT 60 /* 60 minutes */

#define PROC_PATH_FOLDER    "/home/debian/eviot/"

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/
typedef enum _IoTState
{
    /* g_s32IoTLevel = -1 */
    IOT_STATE_NOT_STARTED       = 0x00,
    IOT_STATE_INIT              = 0x01,

    /* g_s32IoTLevel = 0 */
    IOT_STATE_RUNNING           = 0x02,
    IOT_STATE_SYS_RECOV_FAIL    = 0x03,

    /* g_s32IoTLevel = 1 */
    IOT_STATE_CAN_FAIL          = 0x04,
    IOT_STATE_SBMS_FAIL         = 0x05,
    IOT_STATE_MQ_FAIL           = 0x06,

    /* g_s32IoTLevel = 2 */
    IOT_STATE_STOR_FAIL         = 0x07,
    IOT_STATE_MULTI_FAIL        = 0x08,

    /* g_s32IoTLevel = 4 */
    IOT_STATE_SHUTDOWN          = 0x09,
    IOT_STATE_SYS_FAULT         = 0x0A,
    IOT_STATE_MAX               = 0x0B
} IoTState;

typedef enum _IoTTermType
{
    IOT_TERM_TYPE_APP       = 0x01,
    IOT_TERM_TYPE_RESET     = 0x02,
    IOT_TERM_TYPE_SHUTDOWN  = 0x03,
    IOT_TERM_TYPE_MAX       = 0x04
} IoTTermType;

typedef struct _ProcState
{
    pid_t pid[PROC_TYPE_MAX];
    char exeFile[PROC_TYPE_MAX][MAX_PROC_PATH];
    bool runSkip[PROC_TYPE_MAX];
    uint8_t runStat[PROC_TYPE_MAX];
    uint8_t failIdx[PROC_TYPE_MAX];
    uint8_t counter[PROC_TYPE_MAX];
} ProcState;

typedef struct _CommQos
{
    int canErrCnt;
    int sbmsDisconnCnt;
    int mqDisconnCnt;
} CommQos;

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/
static const char *g_strProcName[PROC_TYPE_MAX] =
{
    "PROC_TYPE_MAIN",
    "PROC_TYPE_DC",
    "PROC_TYPE_STOR",
    "PROC_TYPE_FOTA",
    "PROC_TYPE_GPS",
    "PROC_TYPE_LED",
    "PROC_TYPE_MQ",
    "PROC_TYPE_SOH"
    // "PROC_TYPE_HELLO"   // Hello Process Template
};

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
extern char **environ;

static struct timeval g_stGblSyncTime;
static int g_s32FaultInjNum;

static int g_s32MainMsgID;
static int g_s32DcMsgID;
static int g_s32StorMsgID;
static int g_s32FotaMsgID;
static int g_s32GpsMsgID;
static int g_s32LedMsgID;
static int g_s32MqMsgID;
// static int g_s32HelloMsgID;    // Hello Process Template

static IoTMainMsg   g_stMainMsg;
static IoTDcMsg     g_stDcMsg;
static IoTStorMsg   g_stStorMsg;
static IoTFotaMsg   g_stFotaMsg;
static IoTGpsMsg    g_stGpsMsg;
static IoTLedMsg    g_stLedMsg;
static IoTMqMsg    g_stMqMsg;
// static IoTHelloMsg  g_stHelloMsg;   // Hello Process Template

static ProcState g_stProcStat = // @suppress("Invalid arguments")
{
    .exeFile = {"IoTMain"
//                ,"IoTDC",
//                "IoTStor",
//                "IoTFota",
//                "IoTGps",
//                "IoTLed",
//                "IoTMq",
//                "IoTSoh",
                // "IoTHello"  // Hello Process Template
                },
};

static CommQos g_stIoTCommQos;
static int g_s32IoTStat, g_s32IoTLevel;

static IoTTermType g_enSafeExitFlag;
static int g_s32ChldSigPid;
static bool g_bThreadStopFlag;

static char g_strWanIP[20];

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

/*
 * NTP를 통해 외부 시간 서버에서 시간을 읽어와서
 * OS시간을 설정한다. 실패하는 경우 1초 단위로
 * 100회 재시도 한다.
 * 성공하면 0을 리턴.
 */
static int initOsTimeFromNTP(void)
{
    int i;
//    int sockfd = 0;
    int sysret = -1;
//    struct ifreq ifr;

//    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (FAIL == sockfd)
//    {
//        perror("socket error: ");
//        return (-1);
//    }

//    ifr.ifr_addr.sa_family = AF_INET;
//    snprintf(ifr.ifr_name, sizeof(IOT_DEV_ETH), IOT_DEV_ETH);
    for (i=0; i<300; i++)
    {
//        if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0)
//        {
//            /* 네트워크 주소 할당 안된 경우, 재시도 */
//            printf("Main Proc : initOsTimeFromNTP(%d) : No Network \n", i);
//        }
//        else
        {
            /* 네트워크에서 시간을 읽어서 OS시간 설정 */
            sysret = system((const char*) "rdate -s time.bora.net");
            if (sysret == 0 ) // 설정 성공
            {
                //LOG_INFO("Main Proc : initOsTimeFromNTP(%d) : rdate Success", i);
                DBGPRT("Main Proc : initOsTimeFromNTP(%d) : rdate Success \n", i);
                break;
            }
            //LOG_INFO("Main Proc : initOsTimeFromNTP(%d) : rdate failed !!", i);
            DBGPRT("Main Proc : initOsTimeFromNTP(%d) : rdate failed !! \n", i);
        }

        // sleep 1 seconds.
        usleep(1000*1000);
    }

//    close(sockfd);
    return sysret;
}

static void getFaultInjectNum(void)
{
    int fdscr = -1;
    char fbuf[4] = {0, };

    fdscr = open(IOT_FAULTNM_FILE_PATH, O_RDONLY);
    if(fdscr < 0)
    {
        //LOG_INFO("No fault injection");
        close(fdscr);

        return;
    }

    if(read(fdscr, fbuf, sizeof(fbuf) - 1) > 0)
    {
        g_s32FaultInjNum = atoi((const char *)fbuf);
        //LOG_INFO("IoTMain Proc Fault Injection Number: %d", g_s32FaultInjNum);
    }

    close(fdscr);
}

static void initMsgQueue(void)
{
    /* 1. Initialize MAIN Proc message queue - Fault inject num: 12 */
    if(FAIL == (g_s32MainMsgID = msgget((key_t)MSGQ_TYPE_MAIN, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create MAIN Proc message queue");
        reboot_system();
    }

    /* 2. Initialize DC Proc message queue - Fault inject num: 13 */
    if(FAIL == (g_s32DcMsgID = msgget((key_t)MSGQ_TYPE_DC, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create DC Proc message queue");
        reboot_system();
    }

    /* 5. Initialize STOR Proc message queue - Fault inject num: 16 */
    if(FAIL == (g_s32StorMsgID = msgget((key_t)MSGQ_TYPE_STOR, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create STOR Proc message queue");
        reboot_system();
    }

    /* 6. Initialize FOTA Proc message queue - Fault inject num: 17 */
    if(FAIL == (g_s32FotaMsgID = msgget((key_t)MSGQ_TYPE_FOTA, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create FOTA Proc message queue");
        reboot_system();
    }

    /* Initialize GPS Proc message queue - Fault inject num: 17 */
    if(FAIL == (g_s32GpsMsgID = msgget((key_t)MSGQ_TYPE_GPS, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create GPS Proc message queue");
        reboot_system();
    }

    /* Initialize LED Proc message queue - Fault inject num: 17 */
    if(FAIL == (g_s32LedMsgID = msgget((key_t)MSGQ_TYPE_LED, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create LED Proc message queue");
        reboot_system();
    }

    /* Initialize MQ Proc message queue - Fault inject num: 17 */
    if(FAIL == (g_s32MqMsgID = msgget((key_t)MSGQ_TYPE_MQ, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create MQ Proc message queue");
        reboot_system();
    }

#if 0 // Hello Process Template
    /* Initialize HELLO Proc message queue - Fault inject num: 17 */
    if(FAIL == (g_s32HelloMsgID = msgget((key_t)MSGQ_TYPE_HELLO, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create HELLO Proc message queue");
        reboot_system();
    }
#endif
}

static void procQosUpdate(int proc, int errCnt1, int errCnt2)
{
    switch(proc)
    {
        case PROC_TYPE_DC :
            g_stIoTCommQos.canErrCnt += errCnt1;
            g_stIoTCommQos.sbmsDisconnCnt += errCnt2;
            break;

        case PROC_TYPE_MQ :
            g_stIoTCommQos.mqDisconnCnt += errCnt1;
            break;
    }
}

static int composeStatJsonString(IoTStat *pStatData, char *strbuf, int maxlen)
{
    int jsonStringLength;
    const char *iotStatMsgFmt =
              "{\"ts\": %lld,"
              "\"cnt\": %d,"
              "\"gtime\": %d,"
              "\"boot\": %d,"
              "\"cpuUsePercent\": %d,"
              "\"freeDiskMb\": %d,"
              "\"freeRamMb\": %d,"
              "\"wanip\": \"%s\","
              "\"ver\": \"%s\","
              "\"state\":[%d,%d,%d,%d]}";

    // DBGPRT("composeStatJsonString [%d] => %s \n", maxlen, iotStatMsgFmt);

    jsonStringLength = snprintf(strbuf, maxlen, iotStatMsgFmt,
            pStatData->ts,
            (int)pStatData->msgCnt,
            pStatData->gTime,
            (int)pStatData->bootTime,
            pStatData->cpuUsagePercent,
            pStatData->freeDiskMb,
            pStatData->freeMemMb,
            pStatData->wanIP,
            pStatData->iotVer,
            pStatData->state[0],
            pStatData->state[1],
            pStatData->state[2],
            pStatData->state[3]
            );

    return jsonStringLength;
}

static void findWanIP(char *str_ipv4, int str_max)
{
    FILE *fp;

    fp = popen("curl icanhazip.com", "r");
    if (fp == NULL) {
        DBGPRT("findWanIP: popen failed! \n");
        //LOG_WARN("findWanIP: popen failed!");
        return;
    }

    if (fgets(str_ipv4, str_max, fp) != NULL)
    {
        str_ipv4[strlen(str_ipv4) - 1] = '\0';
        DBGPRT("findWanIP: WAN IP = %s \n", str_ipv4);
        //LOG_INFO("findWanIP: WAN IP = %s", str_ipv4);
    }
    else
    {
        DBGPRT("findWanIP: WAN IP invalid \n", str_ipv4);
        //LOG_INFO("findWanIP: WAN IP invalid ", str_ipv4);
    }

    pclose(fp);
    return;
}

static double getCpuUsage(){
    double percent;
    FILE* statfile;
    static unsigned long long lastUser=0, lastNice=0, lastSys=0, lastIdle=0;
    unsigned long long currUser, currNice, currSys, currIdle, total;

    statfile = fopen("/proc/stat", "r");
    fscanf(statfile, "cpu %llu %llu %llu %llu",
            &currUser, &currNice, &currSys, &currIdle);
    fclose(statfile);

    if (currUser < lastUser || currNice < lastNice ||
        currSys < lastSys || currIdle < lastIdle || lastIdle == 0 )
    {
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else
    {
        total = (currUser - lastUser) + (currNice - lastNice) +
                (currSys - lastSys);
        percent = total;
        total += (currIdle - lastIdle);
        percent /= total;
        percent *= 100;
    }

    lastUser = currUser;
    lastNice = currNice;
    lastSys = currSys;
    lastIdle = currIdle;

    return percent;
}

static void procStatusSend(void)
{
    static uint64_t statMsgCnt = 0;

    int rtn = 0;
    IoTMqMsg msgMq;
    IoTStat iotStat;
    struct timeval now;
    struct sysinfo sinfo;
    struct statfs  vfs;

    sysinfo(&sinfo);
    statfs("/", &vfs);

    // iotStat(Status data) 생성
    iotStat.msgCnt = statMsgCnt;
    iotStat.ts = getMSecTimeStamp();

    gettimeofday(&now, NULL);
    iotStat.gTime = (g_stGblSyncTime.tv_sec != 0) ? (now.tv_sec - g_stGblSyncTime.tv_sec) : -1;
    iotStat.bootTime = now.tv_sec - sinfo.uptime;
    snprintf(iotStat.iotVer, IOT_STAT_VER_SZ, IOT_SW_VERSION);

    iotStat.state[0] = g_s32IoTLevel;
    iotStat.state[1] = g_s32IoTStat;
    memcpy(&iotStat.state[2], &g_stProcStat.failIdx[0], (IOT_STAT_STATE_SZ - 2));

    snprintf(iotStat.wanIP, sizeof(iotStat.wanIP), g_strWanIP);

    iotStat.freeDiskMb = (int)((vfs.f_bavail * vfs.f_bsize)/(1024*1024));
    iotStat.freeMemMb = (int)(((sinfo.freeram+sinfo.freeswap)*sinfo.mem_unit)/(1024*1024));
    iotStat.cpuUsagePercent = (int)getCpuUsage();

    // struct -> json string 변환
    msgMq.msgtype = IOT_MQMSG_SENDSTAT;
    msgMq.len = composeStatJsonString(&iotStat, msgMq.data.json_string, sizeof(msgMq.data.json_string) );
    DBGPRT("MAIN: procStatusSend: msg_len=>%d, msg=>%s \n", msgMq.len, msgMq.data.json_string);

    // MQ process로 status 메시지를 보냄.
    rtn = msgsnd(g_s32MqMsgID, &msgMq, sizeof(IoTMqMsg) - sizeof(long), 0);
    if(rtn < 0) /* fault num: 10 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send message to DC proc");
        reboot_system();
    }

    statMsgCnt++;
}
/**
 * @ Name : static void iotStatusSend(void)
 * @ Author : SK
 * @ Date : 2021.06.4
 *
 * @ Parameters[in]  : -
 * @ Parameters[out] : -
 * @ Return Value : -
 *
 * @ brief :
 *    Send IOT status data to LED process when error occurred.
**/
static void iotStatusSend(void)
{
    IoTLedMsg msgLED;

	/* Send message to LED process */
	msgLED.msgtype = IOT_LEDMSG_IOTFAULT;
	msgLED.IOT_status = 1;
	msgsnd(g_s32LedMsgID, &msgLED, sizeof(IoTLedMsg) - sizeof(long),0);
}
/*
 * Main에서 fork한 모든 프로세스가 자신의 초기화를 완료한 후
 * IOT_MAINMSG_STATUS 메시지를 통해 IOT_EVT_INIT_SUCCESS 이벤트를 보내면,
 * Main proccess의 초기화가 최종으로 완료되고, 아래 함수를 호출하여
 * 필요한 프로세스에게 xxMSG_INIT_SUCCESS_ALL 형태의 메시지를 보낸다.
 */
static void procStatusSendInitSuccessAll(void)
{
    // CAN process에게 IOT_DCMSG_INIT_SUCCESS_ALL 메시지를 보낸다
    IoTDcMsg msgDC;

    msgDC.msgtype = IOT_DCMSG_INIT_SUCCESS_ALL;
    if(FAIL == msgsnd(g_s32DcMsgID, &msgDC, sizeof(IoTDcMsg) - sizeof(long), 0)) /* fault inject num: 9 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed to send IOT_DCMSG_INIT_SUCCESS_ALL message to DC proc ");
        // reboot_system();
    }

}

static void procStatusUpdate(int proc, int evt, int fail_idx)
{
    int idx = 0;
    struct timeval tv;
    struct tm *now;

    //LOG_INFO("================== IoT Status Update Proc:%s, Event:%d, FailIdx:%d)", g_strProcName[proc], evt, fail_idx);

    g_stProcStat.counter[proc] = 0;
    g_stProcStat.failIdx[proc] = 0;

    gettimeofday(&tv, NULL);
    now = localtime(&tv.tv_sec);

    if((PROC_TYPE_STOR == proc) && (now->tm_hour == 23) && (now->tm_min == 59))
    {
        //LOG_INFO("CAN Error count:%d, SBMS Disconnect count:%d", g_stIoTCommQos.canErrCnt, g_stIoTCommQos.sbmsDisconnCnt);
        //LOG_INFO("ETH Disconnect count:%d", g_stIoTCommQos.mqDisconnCnt);

        memset(&g_stIoTCommQos, 0x00, sizeof(CommQos));
    }

    /* In case of alive update or FOTA task */
    if((0 == evt) || (PROC_TYPE_FOTA == proc))
    {
//     	// For Test
//		/* Send message to LED process */
//		msgLED.msgtype = IOT_LEDMSG_IOTFAULT;
//		msgLED.IOT_status = 1;
//		msgsnd(g_s32LedMsgID, &msgLED, sizeof(IoTLedMsg) - sizeof(long),0);

        return;
    }

    switch(evt)
    {
        case IOT_EVT_LAUNCH :           /* event#1 - launch */
            g_s32IoTStat = IOT_STATE_INIT;
            g_stProcStat.runStat[proc] = PROC_STAT_INIT;
            break;

        case IOT_EVT_INIT_UNRECOVER :   /* event#2 - Init unrecoverable failure */
            g_s32IoTStat = IOT_STATE_SYS_FAULT;
            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_INIT_SUCCESS :     /* event#3 - Init success */
            g_stProcStat.runStat[proc] = PROC_STAT_RUN;
            if(PROC_TYPE_MAIN == proc)
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
                // 모든 자식 프로세스의 초기화가 완료되면,
                // 필요한 프로세스에게 해당 상태를 알린다.
                procStatusSendInitSuccessAll();
            }
            break;

        case IOT_EVT_SYS_IGNORE :       /* event#4 - System ignore */
            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_SYS_FAIL :         /* event#5 - System failure */
            if(0 == g_s32IoTLevel)
            {
                g_s32IoTStat = IOT_STATE_SYS_RECOV_FAIL;
            }
            else
            {
                if(g_s32IoTLevel > 0)
                {
                    g_s32IoTStat = IOT_STATE_MULTI_FAIL;
                }
            }

            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_SYS_RECOVERED :    /* event#6 - System recover */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_SYS_RECOV_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
            }
            break;

        case IOT_EVT_SYS_RECOV_FAIL :   /* event#7 - System recover failure */
        case IOT_EVT_SYS_UNRECOVER :    /* event#8 - System unrecoverable failure */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_SYS_RECOV_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_SYS_FAULT;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            break;

        case IOT_EVT_GTSYNC_FAIL :      /* event#9 - Global time synchronization failure */
            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_CAN_FAIL :         /* event#10 - CAN failure */
            if(IOT_STATE_RUNNING == g_s32IoTStat)
            {
                g_s32IoTStat = IOT_STATE_CAN_FAIL;
            }
            else
            {
                if((g_s32IoTLevel > 0) && (IOT_STATE_CAN_FAIL != g_s32IoTStat))
                {
                    g_s32IoTStat = IOT_STATE_MULTI_FAIL;
                }
            }
            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_CAN_RECOVERED :    /* event#11 - CAN recovered */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_CAN_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
            }
            break;

        case IOT_EVT_CAN_RECOV_FAIL :   /* event#12 - CAN recover failure */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_CAN_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_SYS_FAULT;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            break;

        case IOT_EVT_SBMS_FAIL :        /* event#13 - SBMS failure */
            if(IOT_STATE_RUNNING == g_s32IoTStat)
            {
                g_s32IoTStat = IOT_STATE_SBMS_FAIL;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            else
            {
                if((g_s32IoTLevel > 0) && (IOT_STATE_SBMS_FAIL != g_s32IoTStat))
                {
                    g_s32IoTStat = IOT_STATE_MULTI_FAIL;
                    g_stProcStat.failIdx[proc] = fail_idx;
                }
            }
            break;

        case IOT_EVT_SBMS_RECOVERED :   /* event#14 - SBMS recovered */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_SBMS_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
            }
            break;

        case IOT_EVT_SBMS_RECOV_FAIL :  /* event#15 - SBMS recover failure */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_SBMS_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_SYS_FAULT;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            break;

        case IOT_EVT_MQ_FAIL :         /* event#19 - MQ failure */
            if(IOT_STATE_RUNNING == g_s32IoTStat)
            {
                g_s32IoTStat = IOT_STATE_MQ_FAIL;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            else
            {
                if((g_s32IoTLevel > 0) && (IOT_STATE_MQ_FAIL != g_s32IoTStat))
                {
                    g_s32IoTStat = IOT_STATE_MULTI_FAIL;
                    g_stProcStat.failIdx[proc] = fail_idx;
                }
            }
            break;

        case IOT_EVT_MQ_RECOVERED :    /* event#20 - MQ recovered */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_MQ_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
            }
            break;

        case IOT_EVT_MQ_RECOV_FAIL :   /* event#21 - MQ recover failure */
            if((0x01 == g_s32IoTLevel) && (IOT_STATE_MQ_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_MQ_FAIL;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            break;

        case IOT_EVT_STOR_FAIL :        /* event#22 - STOR failure */
            if(IOT_STATE_RUNNING == g_s32IoTStat)
            {
                g_s32IoTStat = IOT_STATE_STOR_FAIL;
            }
            else
            {
                if((g_s32IoTLevel > 0) && (IOT_STATE_STOR_FAIL != g_s32IoTStat))
                {
                    g_s32IoTStat = IOT_STATE_MULTI_FAIL;
                }
            }
            g_stProcStat.failIdx[proc] = fail_idx;
            break;

        case IOT_EVT_STOR_RECOVERED :   /* event#23 - STOR recovered */
            if((0x02 == g_s32IoTLevel) && (IOT_STATE_STOR_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_RUNNING;
            }
            break;

        case IOT_EVT_STOR_RECOV_FAIL :  /* event#24 - STOR recover failure */
            if((0x02 == g_s32IoTLevel) && (IOT_STATE_STOR_FAIL == g_s32IoTStat))
            {
                g_s32IoTStat = IOT_STATE_SYS_FAULT;
                g_stProcStat.failIdx[proc] = fail_idx;
            }
            break;

        case IOT_EVT_OVER_NONNET :      /* event#25 - None network over 24 hours */
        case IOT_EVT_POWER_OFF :        /* event#26 - Power off evevnt */
            g_s32IoTStat = IOT_STATE_SHUTDOWN;
            break;
    }

    /* IoT Leveling */
    if(g_s32IoTStat < IOT_STATE_RUNNING) g_s32IoTLevel = -1;
    else if(g_s32IoTStat < IOT_STATE_SYS_RECOV_FAIL) g_s32IoTLevel = 0;
    else if(g_s32IoTStat < IOT_STATE_STOR_FAIL) g_s32IoTLevel = 1;
    else if(g_s32IoTStat < IOT_STATE_SHUTDOWN) g_s32IoTLevel = 2;
    else g_s32IoTLevel = 4;

    /* IOT Fault, when g_s32IoTLevel > 0 */
// 	if(g_s32IoTLevel > 0)
// 	{
// 		/* Send message to LED process */
// 		iotStatusSend();
// 	}
// 	else ;

    //LOG_INFO("================== IoT Now Level: %d, State: %d", g_s32IoTLevel, g_s32IoTStat);
    procStatusSend();

    switch(g_s32IoTStat)
    {
        case IOT_STATE_INIT :

            for(idx = PROC_TYPE_DC; idx < PROC_TYPE_MAX; idx++)
            {
            	// no check for FOTA
            	if(idx == PROC_TYPE_FOTA)
                    continue;
                // skip status check
                if (true == g_stProcStat.runSkip[idx])
                    continue;

                if(PROC_STAT_RUN != g_stProcStat.runStat[idx])
                {
                    return;
                }
            }


            g_stProcStat.runStat[PROC_TYPE_MAIN] = PROC_STAT_RUN;
            g_s32IoTStat = IOT_STATE_RUNNING;

            procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_INIT_SUCCESS, 0);
            //LOG_INFO("### IOT_STATE_INIT");
            break;

        case IOT_STATE_SYS_FAULT :
            sleep(MAX_WAIT_STAT_UPD);
            g_enSafeExitFlag = IOT_TERM_TYPE_RESET;
            //LOG_INFO("### IOT_STATE_SYS_FAULT");
            break;

        case IOT_STATE_MULTI_FAIL :
        case IOT_STATE_SHUTDOWN :
            sleep(MAX_WAIT_STAT_UPD);
            g_enSafeExitFlag = IOT_TERM_TYPE_SHUTDOWN;
            //LOG_INFO("### TERMINATION_TYPE_SHUTDOWN");
            break;

        default :
            break;
    }
}

static void getGlbTimeReqForSync(void)
{
    /* Send message to network for time synchronization */

#if 0
    g_stWifiMsg.msgtype = WIFI_MSG_GBLTIME;
    if(FAIL == msgsnd(g_s32WifiMsgID, &g_stWifiMsg, sizeof(IoTWifiMsg) - sizeof(long), 0)) /* fault inject num: 9 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send message to WIFI proc message queue");
        procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_SYS_IGNORE, 9);
    }

    g_stEthMsg.msgtype = ETH_MSG_GBLTIME;
    if(FAIL == msgsnd(g_s32EthMsgID, &g_stEthMsg, sizeof(IoTEthMsg) - sizeof(long), 0)) /* fault inject num: 10 ? */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send message to WIFI proc message queue");
        procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_SYS_IGNORE, 10);
    }
#endif
}

static int create_proc(ProcType pTyp)
{
    IoTMainMsg msg;
    pid_t pid;

    char tt1, tt2;
    char tmp[MAX_PROC_PATH_EXE] = {0, };

    pid = fork(); /* fault inject num: 8 */
    if(pid > 0)
    {
        g_stProcStat.pid[pTyp] = pid;
        g_stProcStat.runStat[pTyp] = PROC_STAT_INIT;
        //LOG_INFO("Proc Create -- Proc Name :%s, PID:%d", g_strProcName[pTyp], g_stProcStat.pid[pTyp]);

        /* Waiting for the result of creating proc */
        if(FAIL == msgrcv(g_s32MainMsgID, &msg, sizeof(IoTMainMsg) - sizeof(long), IOT_MAINMSG_PROCRSP, 0)) /* fault inject num: 5 */
        {
            g_s32FaultInjNum = 0;
            //LOG_WARN("Failed receive message Main Proc :%s(%d)", strerror(errno), errno);
            reboot_system();
        }

        tt1 = msg.data[0];
        tt2 = msg.data[1];
        //LOG_WARN("Value check - msg.data[0]:%d, msg.data[1]:%d, Proc Name :%s", tt1, tt2, g_strProcName[msg.proc]);

        /* Check result */
        if((msg.proc != pTyp) || (SUCCESS != msg.data[0])) /* fault inject num: 6 */
        {
            g_s32FaultInjNum = 0;
            //LOG_WARN("Failed create : %s Proc", g_strProcName[pTyp]);
            reboot_system();
        }

    }
    else if(pid == 0)
    {
        snprintf(tmp, MAX_PROC_PATH_EXE, PROC_PATH_FOLDER"%s", (char *)&g_stProcStat.exeFile[pTyp][0]);
        if(FAIL == execle(tmp, tmp, NULL, environ)) /* fault inject num: 7 */
        {
            //LOG_WARN("Failed execle: %s", g_stProcStat.exeFile[pTyp]);
            reboot_system();
        }
    }
    else
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("create_proc fork error!");
        reboot_system();
    }

    return 0;
}

static void terminate_proc(ProcType termProc)
{
    int msgQueueId;
    const void *msgPtr;
    size_t msgSize;
    int fail_idx;
    g_s32ChldSigPid = 0;

    switch(termProc)
    {
        case PROC_TYPE_DC :
            g_stDcMsg.msgtype = IOT_DCMSG_TERMINATE;
            msgQueueId = g_s32DcMsgID;
            msgPtr = &g_stDcMsg;
            msgSize = sizeof(IoTDcMsg) - sizeof(long);
            fail_idx = 39;
            break;
        case PROC_TYPE_STOR :
            g_stStorMsg.msgtype = IOT_STORMSG_TERMINATE;
            msgPtr = &g_stStorMsg;
            msgQueueId = g_s32StorMsgID;
            msgSize = sizeof(IoTStorMsg) - sizeof(long);
            fail_idx = 42;
            break;
        case PROC_TYPE_FOTA :
            g_stFotaMsg.msgtype = IOT_FOTAMSG_TERMINATE;
            msgPtr = &g_stFotaMsg;
            msgQueueId = g_s32FotaMsgID;
            msgSize = sizeof(g_stFotaMsg) - sizeof(long);
            fail_idx = 43;
            break;
        case PROC_TYPE_GPS :
            g_stGpsMsg.msgtype = IOT_GPSMSG_TERMINATE;
            msgPtr = &g_stGpsMsg;
            msgQueueId = g_s32GpsMsgID;
            msgSize = sizeof(g_stGpsMsg) - sizeof(long);
            fail_idx = 44;
            break;
        case PROC_TYPE_LED :
            g_stLedMsg.msgtype = IOT_LEDMSG_TERMINATE;
            msgPtr = &g_stLedMsg;
            msgQueueId = g_s32LedMsgID;
            msgSize = sizeof(g_stLedMsg) - sizeof(long);
            fail_idx = 45;
            break;
        case PROC_TYPE_MQ :
            g_stMqMsg.msgtype = IOT_MQMSG_TERMINATE;
            msgPtr = &g_stMqMsg;
            msgQueueId = g_s32MqMsgID;
            msgSize = sizeof(g_stMqMsg) - sizeof(long);
            fail_idx = 46;
            break;
#if 0 // Hello Process Template
        case PROC_TYPE_HELLO :
            g_stHelloMsg.msgtype = IOT_HELLOMSG_TERMINATE;
            msgPtr = &g_stHelloMsg;
            msgQueueId = g_s32HelloMsgID;
            msgSize = sizeof(g_stHelloMsg) - sizeof(long);
            fail_idx = 47;
            break;
#endif
        default:
        	break;
    }

    if(FAIL == msgsnd(msgQueueId, msgPtr, msgSize, 0))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send message to %s Proc message queue", g_stProcStat.exeFile[termProc]);
        procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_SYS_IGNORE, fail_idx);
    }
    else
    {
        while(g_s32ChldSigPid != g_stProcStat.pid[termProc])
        {
            usleep(10 * USEC_TO_MILISEC);
        }
    }
    //LOG_INFO("%s Proc Terminated",g_stProcStat.exeFile[termProc]);
}

static void defunc_handler(int sig)
{
    g_s32ChldSigPid = wait(NULL);
    //LOG_INFO("Child Signal PID:%d", g_s32ChldSigPid);
}

static void *proc_alive_check_thread(void *arg)
{
    static int threadCnt = 0;
    static int syncGblTimeCnt = 0;

    int idx = 0;
    int nonNetCnt = 0;
    char tmp[MAX_PROC_PATH_EXE] = {0, };

    g_bThreadStopFlag = false;
    while(false == g_bThreadStopFlag)
    {
        if(IOT_ALIVE_TIME == threadCnt) /* 1 minute */
        {
            procStatusSend();
            syncGblTimeCnt++;

            if(MAX_GLBSYNC_TIMECNT == syncGblTimeCnt) /* 1 hour: 60 minutes */
            {
                syncGblTimeCnt = 0;
                getGlbTimeReqForSync();
            }

            /* Kill proc and then re-create the proc when there is no response over 2 minutes from the proc */
            for(idx = PROC_TYPE_DC; idx < PROC_TYPE_MAX; idx++)
            {
                // skip alive check when runSkip flag is ture
                if(g_stProcStat.runSkip[idx] == true)
                    continue;

                g_stProcStat.counter[idx]++;

                if(g_stProcStat.counter[idx] > MAX_NORSP_LIMIT) /* fault inject num: 11 */
                {
                    g_s32FaultInjNum = 0;
                    //LOG_WARN("=================== No response %s. It will be restarted", g_strProcName[idx]);
                    procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_SYS_FAIL, 11);
                    g_stProcStat.runStat[idx] = PROC_STAT_TERMINATE;
                    snprintf(tmp, MAX_PROC_PATH_EXE, "/proc/%d", g_stProcStat.pid[idx]);

                    if(0 == access(tmp, F_OK))
                    {
                        //LOG_WARN("KILL %s", tmp);
                        g_s32ChldSigPid = 0;
                        kill(g_stProcStat.pid[idx], SIGKILL);
                        while(g_s32ChldSigPid != g_stProcStat.pid[idx])
                        {
                            usleep(10 * USEC_TO_MILISEC);
                        }
                    }

                    //LOG_WARN("================ Re-Create proc : %d", idx);
                    create_proc((ProcType)idx);
                    procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_SYS_RECOVERED, 11);
                    g_stProcStat.counter[idx] = 0;
                }
            }

            if(IOT_EVT_MQ_FAIL == g_s32IoTStat)
            {
                nonNetCnt++;
                if(IOT_MIN_24HOURS == nonNetCnt) /* 24 hours */
                {
                    procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_OVER_NONNET, 0);
                }
            }
            else
            {
                nonNetCnt = 0;
            }

            threadCnt = 0;
        }

        usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
        threadCnt++;
    }

    return NULL;
}

static void *proc_exit_trigger_thread(void *arg)
{
    int nfds = 2;
    char buf[5] = {0, };
    struct pollfd fdset;
    int faultNum = 0, procNum = 0;

    fdset.fd = STDIN_FILENO;
    fdset.events = POLLIN;

    while(1)
    {
        if(poll(&fdset, nfds, 1000) >= 0)
        {
            memset(buf, 0x00, sizeof(buf));
            if(0 != (fdset.revents & POLLIN))
            {
                if(read(fdset.fd, buf, 3))
                {
                    if('x' == buf[0])
                    {
                        //LOG_INFO("=========================== IOT Terminate");
                        g_enSafeExitFlag = IOT_TERM_TYPE_APP;
                        return NULL;
                    }
                    else
                    {
                        faultNum = atoi((const char *)buf);
                        procNum = faultNum / 100;

                        //LOG_INFO("============ Fault injection num : %d, proc : %s", faultNum, g_strProcName[procNum]);

                        switch(procNum)
                        {
                            case PROC_TYPE_MAIN :
                                g_s32FaultInjNum = faultNum;
                                break;

                            case PROC_TYPE_DC :
                                g_stDcMsg.msgtype = IOT_DCMSG_FAULTINJECT;
                                g_stDcMsg.faultNum = faultNum;
                                if(FAIL == msgsnd(g_s32DcMsgID, &g_stDcMsg, sizeof(IoTDcMsg) - sizeof(long), 0))
                                {
                                    //LOG_INFO("Failed send message to DC proc message queue");
                                }
                                break;

                            case PROC_TYPE_MQ :
                                g_stMqMsg.msgtype = IOT_MQMSG_FAULTINJECT;
                                g_stMqMsg.len = faultNum;
                                if(FAIL == msgsnd(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), 0))
                                {
                                    //LOG_INFO("Failed send message to MQ proc message queue");
                                }
                                break;

                            case PROC_TYPE_STOR :
                                g_stStorMsg.msgtype = IOT_STORMSG_FAULTINJECT;
                                g_stStorMsg.idx = faultNum;
                                if(FAIL == msgsnd(g_s32StorMsgID, &g_stStorMsg, sizeof(IoTStorMsg) - sizeof(long), 0))
                                {
                                    //LOG_INFO("Failed send message to STOR proc message queue");
                                }
                                break;

                            case PROC_TYPE_FOTA :
                                break;
                        } /* switch */
                    } /* else */
                } /* if(read(fdset.fd, buf, 3)) */
            } /* if(0 != (fdset.revents & POLLIN)) */
        } /* if(poll(&fdset, nfds, 1000) >= 0) */

        usleep(10 * USEC_TO_MILISEC); /* sleep 10ms */
    } /* while */

    return NULL;
}

/* Get fork list from config file & initialize g_stProcStat.runSkip[] */
static void loadForkConfigure(ProcState *pProcStat)
{
    config_t cfg;
    config_setting_t *setting;

    int idx;
    const char *strcfg = NULL;

    for(idx = 0; idx < PROC_TYPE_MAX; idx++)
    {
        g_stProcStat.runSkip[idx] = false;
    }

    config_init(&cfg);
    if(CONFIG_FALSE == config_read_file(&cfg, IOT_CONFIG_FILE_PATH)) /* Fault inject num: 206 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Main Proc - config file read error(%s:%d - %s)",
//                 config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        reboot_system();
    }
    else
    {
        setting = config_lookup(&cfg, "mainfork");
        if(NULL != setting)
        {
            for(idx = 0; idx < PROC_TYPE_MAX; idx++)
            {
            	int fork = 0;
            	config_setting_lookup_int(setting, g_stProcStat.exeFile[idx], &fork);
            	//LOG_INFO("### Fork Config : proc[%s] ==> %d ",g_stProcStat.exeFile[idx], fork);
            	// set runSkip false when fork is non-zero value
            	g_stProcStat.runSkip[idx] = fork ? false: true;
            }
        }
        config_destroy(&cfg);
    }
}

int main(void)
{
    int idx = 0, rtn = 0;
    pthread_t aliveThreadID;
    /* pthread_t safeExitThreadID; */

    LogSetInfo(LOG_FILE_PATH, "IoTMain"); /* Initialize logging */
    //LOG_INFO("Start IoT Main Proc");

    /* 시스템 시간 설정 초기화에 실패한 경우 리부팅 한다. */
    if (0 != initOsTimeFromNTP())
    {
        //LOG_WARN("Failed to initialize system time ");
        // 개발시에는 아래 리부팅하는 부분을 마스킹함.
        // reboot_system();
    }

    // WAN IP 주소를 찾는다.
    findWanIP(g_strWanIP, sizeof(g_strWanIP));

    gettimeofday(&g_stGblSyncTime, NULL);

    getFaultInjectNum(); /* Get fault injection number */
    initMsgQueue(); /* Initialize message queue */
    procStatusUpdate(PROC_TYPE_MAIN, IOT_EVT_LAUNCH, 0); /* Main Proc launch event */

    /* Get fork list from config file & initialize g_stProcStat.runSkip[] */
    loadForkConfigure(&g_stProcStat);

    /* Set Process Information */
    for(idx = 0; idx < PROC_TYPE_MAX; idx++)
    {
        g_stProcStat.pid[idx] = -1;
        g_stProcStat.runStat[idx] = PROC_STAT_TERMINATE;
    }
    g_stProcStat.pid[PROC_TYPE_MAIN] = getpid();
    g_stProcStat.runStat[PROC_TYPE_MAIN] = PROC_STAT_INIT;

    g_enSafeExitFlag = 0;

    for(idx = PROC_TYPE_DC; idx < PROC_TYPE_MAX; idx++)
    {
        // skip create process when runSkip flag is ture
        if(g_stProcStat.runSkip[idx] == true)
            continue;

        create_proc((ProcType)idx);
    }
    /* Create alive thread */
    pthread_create(&aliveThreadID, 0, proc_alive_check_thread, NULL);

    mq_main();
    dc_main();
    stor_main();
    /* Create exit thread: comment out to run at background mode */
    /* pthread_create(&safeExitThreadID, 0, proc_exit_trigger_thread, NULL); */
    signal(SIGCHLD, defunc_handler);

    memset(&g_stIoTCommQos, 0x00, sizeof(CommQos));

    printf("Main Proc Created !! \n");

    while(1)
    {
        /* waiting for result of creating proc */
        rtn = msgrcv(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), IOT_MAINMSG_ALL, IPC_NOWAIT);
        if(FAIL == rtn)
        {
            /* No error in case of interrupt from timer */
            if(EINTR == errno) continue;
        }
        else
        {
            switch(g_stMainMsg.msgtype)
            {
                case IOT_MAINMSG_PROCRSP :
                    break;

                case IOT_MAINMSG_STATUS :
                    procStatusUpdate(g_stMainMsg.proc, g_stMainMsg.data[0], g_stMainMsg.data[1]);
                    break;

                case IOT_MAINMSG_GTIMERSP :
                    if(SUCCESS != g_stMainMsg.data[0])
                    {
                        // 네트워크 시간 응답 실패인 경우, RTC HW 시간을 읽어서 OS시간 설정
                        // EV IoT 보드에는 RTC가 없어서, 아래 라인은 마스킹처리함.
                        // system((const char *)"hwclock --hctosys -f /dev/rtc0");
                    }
                    // OS 시간을 읽어서, g_stGblSyncTime 전역변수에 설정
                    gettimeofday(&g_stGblSyncTime, NULL);
                    break;

                case IOT_MAINMSG_COMMQOS :
                    procQosUpdate(g_stMainMsg.proc, g_stMainMsg.data[0], g_stMainMsg.data[1]);
                    break;

                default :
                    break;
            }
        }

        /* Invoke FOTA proc to update root file system */

        /* All proc termination */
        if(g_enSafeExitFlag)
        {
            /* Main proc alive thread stop */
            g_bThreadStopFlag = true;

            for(idx = PROC_TYPE_DC; idx < PROC_TYPE_MAX; idx++)
            {
                // skip create process when runSkip flag is ture
                if(g_stProcStat.runSkip[idx] == true)
                    continue;

                terminate_proc((ProcType)idx);
            }

            switch(g_enSafeExitFlag)
            {
                case IOT_TERM_TYPE_APP :
                    //LOG_INFO("### TERMINATION_TYPE_APP");
                    //LogClose();
                    return 0;

                case IOT_TERM_TYPE_RESET :
                    //LOG_INFO("### TERMINATION_TYPE_RESET");
                    //LogClose();
                    reboot_system();
                    break;

                case IOT_TERM_TYPE_SHUTDOWN :
                    //LOG_INFO("### TERMINATION_TYPE_SHUTDOWN");
                    //LogClose();
                    reboot(RB_POWER_OFF);
                    break;

                default :
                    break;
            }
        }

        usleep(10 * USEC_TO_MILISEC);
    }
}

