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
 **   $FILNAME   : IoT_dc.c                                                              **
 **                                                                                      **
 **   $VERSION   : v0.1                                                                  **
 **                                                                                      **
 **   $Date      : Dec 17, 2020                                                          **
 **                                                                                      **
 **   AUTHOR     : SK Dev Engineer                                                       **
 **                                                                                      **
 **   PROJECT    : SK IoT Gateway                                                        **
 **                                                                                      **
 **   DESCRIPTION: This file contains Data Collection process implementation.            **
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
#include <math.h>
#include <termios.h>
#include <signal.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include <config/libconfig.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_rbuf.h"
#include "IoT_main.h"
#include "IoT_stor.h"
#include "IoT_can.h"
#include "IoT_led.h"
#include "IoT_soh.h"
#include "Sensor.h"

#include "IoT_dc.h"

#include <netinet/in.h>
#include <netdb.h>

/*****************************************************************************************
 **                      Definitions                                                     **
 ******************************************************************************************/
#define CSV_LINE_LEN    128
#define CSV_LINE_COL    6

#define TIME_GAP_STEP   10 /* 10 seconds */

volatile int done;

//#define DEFAULT_PORT "/dev/ttyO4" //beaglebone uart4
#define DEFAULT_PORT "/dev/ttyO2"


#define DEFAULT_SPEED 9600
// we will use I2C2 which is enumerated as 1 on the BBB
#define I2CBUS 1

// set to 1 to print out the intermediate calculations
#define DBG_PRINT 0
// set to 1 to use the example values in the datasheet
// rather than the values retrieved via i2c
#define ALG_TEST 0

void register_sig_handler();
void sigint_handler(int sig);
int open_port(const char *device, int speed, struct termios *oldopts);
int map_speed_to_unix(int speed);
void run_test(int fd, unsigned char byte);
/*****************************************************************************************
 **                      Type Definitions                                                **
 ******************************************************************************************/
typedef enum _CsvColumn
{
    CSV_COL_TIME    = 0x00, /* Time[s] */
    CSV_COL_CYCLE   = 0x01, /* Charge/Discharge Cycle Count */
    CSV_COL_ICURR   = 0x02, /* Ideal current [A] */
    CSV_COL_FCURR   = 0x03, /* Filtered current [A] */
    CSV_COL_INTGRQ  = 0x04, /* Integrated capacity [Ah] */
    CSV_COL_CVOLT   = 0x05  /* Simulated cell voltage [V] */
} CsvColumn;

typedef struct _RespObj
{
    json_object *obj;
    json_object *timeStamp;
    json_object *bankId;
    json_object *canId;

    /* 0x4D0 */
    json_object *rCell01;
    json_object *rCell02;
    json_object *rCell03;
    json_object *rCell04;

    json_object *rCell05;
    json_object *rCell06;
    json_object *rCell07;
    json_object *rCell08;

    json_object *rCell09;
    json_object *rCell10;
    json_object *rCell11;
    json_object *rCell12;

    json_object *rCell13;
    json_object *rCell14;
    json_object *rCell15;
    json_object *rCell16;

    json_object *rCell17;
    json_object *rCell18;
    json_object *rCell19;
    json_object *rCell20;

    json_object *rCell21;
    json_object *rCell22;
    json_object *rCell23;
    json_object *rCell24;

    json_object *rCell25;
    json_object *rCell26;
    json_object *rCell27;
    json_object *rCell28;

    json_object *rCell29;
    json_object *rCell30;
    json_object *rCell31;
    json_object *rCell32;

    json_object *rCell33;
    json_object *rCell34;
    json_object *rCell35;
    json_object *rCell36;

    json_object *rCell37;
    json_object *rCell38;
    json_object *rCell39;
    json_object *rCell40;

    json_object *rCell41;
    json_object *rCell42;
    json_object *rCell43;
    json_object *rCell44;

    json_object *rCell45;
    json_object *rCell46;
    json_object *rCell47;
    json_object *rCell48;

    json_object *rCell49;
    json_object *rCell50;
    json_object *rCell51;
    json_object *rCell52;

    json_object *rCell53;
    json_object *rCell54;
    json_object *rCell55;
    json_object *rCell56;

    json_object *rCell57;
    json_object *rCell58;
    json_object *rCell59;
    json_object *rCell60;

    json_object *rCell61;
    json_object *rCell62;
    json_object *rCell63;
    json_object *rCell64;

    /*  0x4E7  */
    json_object *rTemp01;
    json_object *rTemp02;
    json_object *rTemp03;
    json_object *rTemp04;
    json_object *rTemp05;
    json_object *rTemp06;
    json_object *rTemp07;
    json_object *rTemp08;

    json_object *rTemp09;
    json_object *rTemp10;
    json_object *rTemp11;
    json_object *rTemp12;
    json_object *rTemp13;
    json_object *rTemp14;
    json_object *rTemp15;
    json_object *rTemp16;

    json_object *rTemp17;
    json_object *rTemp18;
    json_object *rTemp19;
    json_object *rTemp20;
    json_object *rTemp21;
    json_object *rTemp22;
    json_object *rTemp23;
    json_object *rTemp24;

    /*  0x4EB  */
    json_object *rModVol01;
    json_object *rModVol02;
    json_object *rModVol03;
    json_object *rModVol04;

    json_object *rModVol05;
    json_object *rModVol06;
    json_object *rModVol07;
    json_object *rModVol08;

    /*0x4ED*/
    json_object *rPackVol;
    json_object *rInvVol;
    json_object *rPackSOC;
    json_object *rPackSOH;

    json_object *rPackCurr;
    json_object *rBmsStat;
    json_object *rBmsFltLvl;
    json_object *rRlyStat;

    json_object *rMaxCellVol;
    json_object *rAvgCellVol;
    json_object *rMinCellVol;
    json_object *rMaxCellNo;
    json_object *rMinCellNo;

    json_object *rMaxTemp;
    json_object *rAvgTemp;
    json_object *rMinTemp;
    json_object *rMaxTempNo;
    json_object *rMinTempNo;

    json_object *rMaxChgPwr;
    json_object *rMaxDChgPwr;
    json_object *rRealPwr;
    json_object *rAvailCapa;

    /*0x4F2*/
    json_object *rWrnCurrOver;
    json_object *rWrnCurrUnder;

    json_object *rWrnSocOver;
    json_object *rWrnSocUnder;
    json_object *rWrnCellOver;
    json_object *rWrnCellUnder;
    json_object *rWrnCellDiff;
    json_object *rWrnTempOver;
    json_object *rWrnTempUnder;
    json_object *rWrnTempDiff;
    json_object *rWrnPackOver;
    json_object *rWrnPackUnder;

    json_object *rFltRlyDrv;
    json_object *rFltAuxPwr;
    json_object *rFltCsm;
    json_object *rFltAfe;
    json_object *rFltMcu;
    json_object *rFltCurrOver;
    json_object *rFltCurrUnder;

    json_object *rFltSocUnder;
    json_object *rFltSohUnder;
    json_object *rFltRlyAge;
    json_object *rFltRlyOpen;
    json_object *rFltRlyWeld;
    json_object *rFltCellOver;
    json_object *rFltCellUnder;
    json_object *rFltCellDiff;
    json_object *rFltTempOver;
    json_object *rFltTempUnder;
    json_object *rFltTempDiff;
    json_object *rFltPackOver;
    json_object *rFltPackUnder;
} RespObj;

typedef struct _RespsObj
{
    json_object *obj;
    json_object *msgCnt;
    json_object *timeStamp;
    json_object *respCnt;
    json_object *respAry;
    RespObj resp;
} RespsObj;


#if true
typedef struct _ImusObj
{
    json_object *obj;
    json_object *timeStamp;

    json_object *rLatitude;
    json_object *rLongitude;

} ImusObj;

//typedef struct _ImusObj
//{
//    json_object *obj;
//    json_object *msgCnt;
//    json_object *timeStamp;
//    ImuObj sen;
//} ImusObj;

//typedef struct _IoTImuData
//{
//    json_object *obj;
//    ImusObj sens;
//} IoTImuData;
#endif

typedef struct _IoTRespBmsData
{
    json_object *obj;
    RespsObj resps;
    ImusObj sens;
} IoTRespBmsData;



/*****************************************************************************************
 **                      Constants                                                       **
 ******************************************************************************************/

/*****************************************************************************************
 **                      Variables                                                       **
 ******************************************************************************************/
static int g_s32MainMsgID, g_s32DcMsgID, g_s32StorMsgID, g_s32LedMsgID;
static IoTMainMsg g_stMainMsg;
static IoTDcMsg   g_stDcMsg;
static IoTLedMsg  g_stLedMsg;
static int g_s32SohMsgID;
static IoTSohMsg g_stSohMsg;

static RespBmsCanInfo g_stCanInfo;
static SenInfo g_stSenInfo;
uint64_t tfTimeStamp;

static IoTRespBmsData g_stJsonObjs;

static pthread_mutex_t g_mtxDcMainMsg;
static pthread_mutex_t g_mtxEvtCB;

float gps[2] ;
int killCnt = 0;

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

static void getFaultInjectNum(void)
{
    int fdscr = -1;
    char fbuf[4] = { 0, };

    fdscr = open(IOT_FAULTNM_FILE_PATH, O_RDONLY);
    if (fdscr < 0)
    {
//        LOG_INFO("No fault injection");
        close(fdscr);

        return;
    }

    if (read(fdscr, fbuf, sizeof(fbuf) - 1) > 0)
    {
        g_s32DcFaultInjNum = atoi((const char *) fbuf);
//        LOG_INFO("IoTDc Proc Fault Injection Number: %d", g_s32DcFaultInjNum);
    }

    close(fdscr);
}

static void initSensData(SenInfo *pRxSenData)
{
    memset(&pRxSenData->pSenData, 0x00, sizeof(SenInfo));
    gps[0]=0;
    gps[1]=0;
}

static void reboot_system(void)
{
    int fdscr = -1, rbCnt = 0;
    char fbuf = 0;

    sleep(1); /* Sleep 1 sec for message print time */
    fdscr = open(IOT_REBOOT_FILE_PATH, O_RDWR | O_CREAT | O_SYNC, 0644);
    if (fdscr < 0)
    {
//        LOG_INFO("### Reboot count file open failed. reboot now...");
        LogClose();

        /* hard reset */
        reboot(RB_AUTOBOOT);
    }

    if (read(fdscr, &fbuf, 1) > 0)
    {
        rbCnt = fbuf - '0';
        if (rbCnt > IOT_REBOOT_LIMIT) /* Go to shutdown if reboot has performed more than 3 */
        {
            fbuf = '0';
            lseek(fdscr, 0, SEEK_SET);
            write(fdscr, &fbuf, 1);
            close(fdscr);

            /* Power OFF */
//            LOG_INFO("### SHUTDOWN now...");
            LogClose();
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
//    LOG_INFO("### Reboot now...");
    LogClose();

    /* hard reset */
    reboot(RB_AUTOBOOT);
}

static void initMsgQueue(void)
{
    /* 1. Initialize MAIN Proc message queue - Fault inject num: 108 */
    if (FAIL
            == (g_s32MainMsgID = msgget((key_t) MSGQ_TYPE_MAIN,
                                        IPC_CREAT | 0666)))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed create MAIN Proc message queue");
        reboot_system();
    }

    /* 2. Initialize DC Proc message queue - Fault inject num: 109 */
    if (FAIL == (g_s32DcMsgID = msgget((key_t) MSGQ_TYPE_DC, IPC_CREAT | 0666)))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed create DC Proc message queue");
        reboot_system();
    }

    /* 3. Initialize STOR Proc message queue - Fault inject num: 110 */
    if (FAIL
            == (g_s32StorMsgID = msgget((key_t) MSGQ_TYPE_STOR,
                                        IPC_CREAT | 0666)))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed create STOR Proc message queue");
        reboot_system();
    }

    /* 4. Initialize LED Proc message queue */
    if (FAIL == (g_s32LedMsgID = msgget((key_t) MSGQ_TYPE_LED, IPC_CREAT | 0666)))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed create LED Proc message queue");
        reboot_system();
    }

    /* 5. Initialize SOH Proc message queue */
    if(FAIL == (g_s32SohMsgID = msgget((key_t)MSGQ_TYPE_SOH, IPC_CREAT | 0666)))
    {
        printf("Failed create SOH Proc message queue\n");
//        LOG_WARN("Failed create SOH Proc message queue");
        reboot_system();
    }
}

static void initJsonObjs(IoTRespBmsData *pRData, const char *bankID)
{
    /* IoT Data obj */
    pRData->obj = json_object_new_object();

    /* Resps obj */
    pRData->resps.obj = json_object_new_object();

    /* Create GPS data for JSON format */
    pRData->sens.obj = json_object_new_object();

    json_object_object_add(pRData->resps.obj, "GPS", pRData->sens.obj);

    /*timestamp*/
    pRData->sens.timeStamp = json_object_new_int64(-1);
    json_object_object_add(pRData->sens.obj, "Created", pRData->sens.timeStamp);

    pRData->sens.rLatitude = json_object_new_double(-1);
    json_object_object_add(pRData->sens.obj, "Latitude",pRData->sens.rLatitude);

    pRData->sens.rLongitude = json_object_new_double(-1);
    json_object_object_add(pRData->sens.obj, "Longitude",pRData->sens.rLongitude);


    /* Create CAN data for JSON format */
    pRData->resps.resp.obj = json_object_new_object();

    json_object_object_add(pRData->resps.obj, "BMS", pRData->resps.resp.obj);

    /*timestamp*/
    pRData->resps.resp.timeStamp = json_object_new_int64(-1);
    json_object_object_add(pRData->resps.resp.obj, "Created",
                           pRData->resps.resp.timeStamp);
    /* rCell01 */
    pRData->resps.resp.rCell01 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell01",
                           pRData->resps.resp.rCell01);

    pRData->resps.resp.rCell02 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell02",
                           pRData->resps.resp.rCell02);
    pRData->resps.resp.rCell03 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell03",
                           pRData->resps.resp.rCell03);
    pRData->resps.resp.rCell04 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell04",
                           pRData->resps.resp.rCell04);
    pRData->resps.resp.rCell05 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell05",
                           pRData->resps.resp.rCell05);
    pRData->resps.resp.rCell06 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell06",
                           pRData->resps.resp.rCell06);
    pRData->resps.resp.rCell07 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell07",
                           pRData->resps.resp.rCell07);
    pRData->resps.resp.rCell08 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell08",
                           pRData->resps.resp.rCell08);
    pRData->resps.resp.rCell09 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell09",
                           pRData->resps.resp.rCell09);
    pRData->resps.resp.rCell10 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell10",
                           pRData->resps.resp.rCell10);
    pRData->resps.resp.rCell11 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell11",
                           pRData->resps.resp.rCell11);
    pRData->resps.resp.rCell12 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell12",
                           pRData->resps.resp.rCell12);
    pRData->resps.resp.rCell13 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell13",
                           pRData->resps.resp.rCell13);
    pRData->resps.resp.rCell14 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell14",
                           pRData->resps.resp.rCell14);
    pRData->resps.resp.rCell15 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell15",
                           pRData->resps.resp.rCell15);
    pRData->resps.resp.rCell16 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell16",
                           pRData->resps.resp.rCell16);
    pRData->resps.resp.rCell17 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell17",
                           pRData->resps.resp.rCell17);
    pRData->resps.resp.rCell18 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell18",
                           pRData->resps.resp.rCell18);
    pRData->resps.resp.rCell19 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell19",
                           pRData->resps.resp.rCell19);
    pRData->resps.resp.rCell20 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell20",
                           pRData->resps.resp.rCell20);
    pRData->resps.resp.rCell21 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell21",
                           pRData->resps.resp.rCell21);
    pRData->resps.resp.rCell22 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell22",
                           pRData->resps.resp.rCell22);
    pRData->resps.resp.rCell23 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell23",
                           pRData->resps.resp.rCell23);
    pRData->resps.resp.rCell24 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell24",
                           pRData->resps.resp.rCell24);
    pRData->resps.resp.rCell25 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell25",
                           pRData->resps.resp.rCell25);
    pRData->resps.resp.rCell26 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell26",
                           pRData->resps.resp.rCell26);
    pRData->resps.resp.rCell27 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell27",
                           pRData->resps.resp.rCell27);
    pRData->resps.resp.rCell28 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell28",
                           pRData->resps.resp.rCell28);
    pRData->resps.resp.rCell29 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell29",
                           pRData->resps.resp.rCell29);
    pRData->resps.resp.rCell30 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell30",
                           pRData->resps.resp.rCell30);
    pRData->resps.resp.rCell31 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell31",
                           pRData->resps.resp.rCell31);
    pRData->resps.resp.rCell32 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell32",
                           pRData->resps.resp.rCell32);
    pRData->resps.resp.rCell33 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell33",
                           pRData->resps.resp.rCell33);
    pRData->resps.resp.rCell34 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell34",
                           pRData->resps.resp.rCell34);
    pRData->resps.resp.rCell35 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell35",
                           pRData->resps.resp.rCell35);
    pRData->resps.resp.rCell36 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell36",
                           pRData->resps.resp.rCell36);
    pRData->resps.resp.rCell37 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell37",
                           pRData->resps.resp.rCell37);
    pRData->resps.resp.rCell38 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell38",
                           pRData->resps.resp.rCell38);
    pRData->resps.resp.rCell39 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell39",
                           pRData->resps.resp.rCell39);
    pRData->resps.resp.rCell40 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell40",
                           pRData->resps.resp.rCell40);
    pRData->resps.resp.rCell41 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell41",
                           pRData->resps.resp.rCell41);
    pRData->resps.resp.rCell42 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell42",
                           pRData->resps.resp.rCell42);
    pRData->resps.resp.rCell43 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell43",
                           pRData->resps.resp.rCell43);
    pRData->resps.resp.rCell44 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell44",
                           pRData->resps.resp.rCell44);
    pRData->resps.resp.rCell45 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell45",
                           pRData->resps.resp.rCell45);
    pRData->resps.resp.rCell46 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell46",
                           pRData->resps.resp.rCell46);
    pRData->resps.resp.rCell47 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell47",
                           pRData->resps.resp.rCell47);
    pRData->resps.resp.rCell48 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell48",
                           pRData->resps.resp.rCell48);
    pRData->resps.resp.rCell49 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell49",
                           pRData->resps.resp.rCell49);
    pRData->resps.resp.rCell50 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell50",
                           pRData->resps.resp.rCell50);
    pRData->resps.resp.rCell51 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell51",
                           pRData->resps.resp.rCell51);
    pRData->resps.resp.rCell52 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell52",
                           pRData->resps.resp.rCell52);
    pRData->resps.resp.rCell53 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell53",
                           pRData->resps.resp.rCell53);
    pRData->resps.resp.rCell54 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell54",
                           pRData->resps.resp.rCell54);
    pRData->resps.resp.rCell55 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell55",
                           pRData->resps.resp.rCell55);
    pRData->resps.resp.rCell56 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell56",
                           pRData->resps.resp.rCell56);
    pRData->resps.resp.rCell57 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell57",
                           pRData->resps.resp.rCell57);
    pRData->resps.resp.rCell58 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell58",
                           pRData->resps.resp.rCell58);
    pRData->resps.resp.rCell59 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell59",
                           pRData->resps.resp.rCell59);
    pRData->resps.resp.rCell60 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell60",
                           pRData->resps.resp.rCell60);
    pRData->resps.resp.rCell61 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell61",
                           pRData->resps.resp.rCell61);
    pRData->resps.resp.rCell62 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell62",
                           pRData->resps.resp.rCell62);
    pRData->resps.resp.rCell63 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell63",
                           pRData->resps.resp.rCell63);
    pRData->resps.resp.rCell64 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Cell64",
                           pRData->resps.resp.rCell64);

    pRData->resps.resp.rTemp01 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp01",
                           pRData->resps.resp.rTemp01);
    pRData->resps.resp.rTemp02 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp02",
                           pRData->resps.resp.rTemp02);
    pRData->resps.resp.rTemp03 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp03",
                           pRData->resps.resp.rTemp03);
    pRData->resps.resp.rTemp04 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp04",
                           pRData->resps.resp.rTemp04);
    pRData->resps.resp.rTemp05 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp05",
                           pRData->resps.resp.rTemp05);
    pRData->resps.resp.rTemp06 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp06",
                           pRData->resps.resp.rTemp06);
    pRData->resps.resp.rTemp07 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp07",
                           pRData->resps.resp.rTemp07);
    pRData->resps.resp.rTemp08 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp08",
                           pRData->resps.resp.rTemp08);
    pRData->resps.resp.rTemp09 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp09",
                           pRData->resps.resp.rTemp09);
    pRData->resps.resp.rTemp10 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp10",
                           pRData->resps.resp.rTemp10);
    pRData->resps.resp.rTemp11 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp11",
                           pRData->resps.resp.rTemp11);
    pRData->resps.resp.rTemp12 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp12",
                           pRData->resps.resp.rTemp12);
    pRData->resps.resp.rTemp13 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp13",
                           pRData->resps.resp.rTemp13);
    pRData->resps.resp.rTemp14 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp14",
                           pRData->resps.resp.rTemp14);
    pRData->resps.resp.rTemp15 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp15",
                           pRData->resps.resp.rTemp15);
    pRData->resps.resp.rTemp16 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp16",
                           pRData->resps.resp.rTemp16);
    pRData->resps.resp.rTemp17 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp17",
                           pRData->resps.resp.rTemp17);
    pRData->resps.resp.rTemp18 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp18",
                           pRData->resps.resp.rTemp18);
    pRData->resps.resp.rTemp19 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp19",
                           pRData->resps.resp.rTemp19);
    pRData->resps.resp.rTemp20 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp20",
                           pRData->resps.resp.rTemp20);
    pRData->resps.resp.rTemp21 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp21",
                           pRData->resps.resp.rTemp21);
    pRData->resps.resp.rTemp22 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp22",
                           pRData->resps.resp.rTemp22);
    pRData->resps.resp.rTemp23 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp23",
                           pRData->resps.resp.rTemp23);
    pRData->resps.resp.rTemp24 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "Temp24",
                           pRData->resps.resp.rTemp24);

    pRData->resps.resp.rModVol01 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol01",
                           pRData->resps.resp.rModVol01);
    pRData->resps.resp.rModVol02 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol02",
                           pRData->resps.resp.rModVol02);
    pRData->resps.resp.rModVol03 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol03",
                           pRData->resps.resp.rModVol03);
    pRData->resps.resp.rModVol04 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol04",
                           pRData->resps.resp.rModVol04);
    pRData->resps.resp.rModVol05 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol05",
                           pRData->resps.resp.rModVol05);
    pRData->resps.resp.rModVol06 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol06",
                           pRData->resps.resp.rModVol06);
    pRData->resps.resp.rModVol07 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol07",
                           pRData->resps.resp.rModVol07);
    pRData->resps.resp.rModVol08 = json_object_new_int(-1);
    json_object_object_add(pRData->resps.resp.obj, "ModVol08",
                           pRData->resps.resp.rModVol08);

    /*etc*/
    pRData->resps.resp.rPackVol = json_object_new_int(-1);
    pRData->resps.resp.rInvVol = json_object_new_int(-1);
    pRData->resps.resp.rPackSOC = json_object_new_int(-1);
    pRData->resps.resp.rPackSOH = json_object_new_int(-1);

    pRData->resps.resp.rPackCurr = json_object_new_int(-1);
    pRData->resps.resp.rBmsStat = json_object_new_int(-1);
    pRData->resps.resp.rBmsFltLvl = json_object_new_int(-1);
    pRData->resps.resp.rRlyStat = json_object_new_int(-1);

    pRData->resps.resp.rMaxCellVol = json_object_new_int(-1);
    pRData->resps.resp.rAvgCellVol = json_object_new_int(-1);
    pRData->resps.resp.rMinCellVol = json_object_new_int(-1);
    pRData->resps.resp.rMaxCellNo = json_object_new_int(-1);
    pRData->resps.resp.rMinCellNo = json_object_new_int(-1);

    pRData->resps.resp.rMaxTemp = json_object_new_int(-1);
    pRData->resps.resp.rAvgTemp = json_object_new_int(-1);
    pRData->resps.resp.rMinTemp = json_object_new_int(-1);
    pRData->resps.resp.rMaxTempNo = json_object_new_int(-1);
    pRData->resps.resp.rMinTempNo = json_object_new_int(-1);

    pRData->resps.resp.rMaxChgPwr = json_object_new_int(-1);
    pRData->resps.resp.rMaxDChgPwr = json_object_new_int(-1);
    pRData->resps.resp.rRealPwr = json_object_new_int(-1);
    pRData->resps.resp.rAvailCapa = json_object_new_int(-1);

    pRData->resps.resp.rWrnCurrOver = json_object_new_int(-1);
    pRData->resps.resp.rWrnCurrUnder = json_object_new_int(-1);

    pRData->resps.resp.rWrnSocOver = json_object_new_int(-1);
    pRData->resps.resp.rWrnSocUnder = json_object_new_int(-1);
    pRData->resps.resp.rWrnCellOver = json_object_new_int(-1);
    pRData->resps.resp.rWrnCellUnder = json_object_new_int(-1);
    pRData->resps.resp.rWrnCellDiff = json_object_new_int(-1);
    pRData->resps.resp.rWrnTempOver = json_object_new_int(-1);
    pRData->resps.resp.rWrnTempUnder = json_object_new_int(-1);
    pRData->resps.resp.rWrnTempDiff = json_object_new_int(-1);
    pRData->resps.resp.rWrnPackOver = json_object_new_int(-1);
    pRData->resps.resp.rWrnPackUnder = json_object_new_int(-1);

    pRData->resps.resp.rFltRlyDrv = json_object_new_int(-1);
    pRData->resps.resp.rFltAuxPwr = json_object_new_int(-1);
    pRData->resps.resp.rFltCsm = json_object_new_int(-1);
    pRData->resps.resp.rFltAfe = json_object_new_int(-1);
    pRData->resps.resp.rFltMcu = json_object_new_int(-1);
    pRData->resps.resp.rFltCurrOver = json_object_new_int(-1);
    pRData->resps.resp.rFltCurrUnder = json_object_new_int(-1);

    pRData->resps.resp.rFltSocUnder = json_object_new_int(-1);
    pRData->resps.resp.rFltSohUnder = json_object_new_int(-1);
    pRData->resps.resp.rFltRlyAge = json_object_new_int(-1);
    pRData->resps.resp.rFltRlyOpen = json_object_new_int(-1);
    pRData->resps.resp.rFltRlyWeld = json_object_new_int(-1);
    pRData->resps.resp.rFltCellOver = json_object_new_int(-1);
    pRData->resps.resp.rFltCellUnder = json_object_new_int(-1);
    pRData->resps.resp.rFltCellDiff = json_object_new_int(-1);
    pRData->resps.resp.rFltTempOver = json_object_new_int(-1);
    pRData->resps.resp.rFltTempUnder = json_object_new_int(-1);
    pRData->resps.resp.rFltTempDiff = json_object_new_int(-1);
    pRData->resps.resp.rFltPackOver = json_object_new_int(-1);
    pRData->resps.resp.rFltPackUnder = json_object_new_int(-1);

    json_object_object_add(pRData->resps.resp.obj, "PackVol",
                           pRData->resps.resp.rPackVol);
    json_object_object_add(pRData->resps.resp.obj, "InvVol",
                           pRData->resps.resp.rInvVol);
    json_object_object_add(pRData->resps.resp.obj, "PackSOC",
                           pRData->resps.resp.rPackSOC);
    json_object_object_add(pRData->resps.resp.obj, "PackSOH",
                           pRData->resps.resp.rPackSOH);

    json_object_object_add(pRData->resps.resp.obj, "PackCurr",
                           pRData->resps.resp.rPackCurr);
    json_object_object_add(pRData->resps.resp.obj, "BmsStat",
                           pRData->resps.resp.rBmsStat);
    json_object_object_add(pRData->resps.resp.obj, "BmsFltLvl",
                           pRData->resps.resp.rBmsFltLvl);
    json_object_object_add(pRData->resps.resp.obj, "RlyStat",
                           pRData->resps.resp.rRlyStat);

    json_object_object_add(pRData->resps.resp.obj, "MaxCellVol",
                           pRData->resps.resp.rMaxCellVol);
    json_object_object_add(pRData->resps.resp.obj, "AvgCellVol",
                           pRData->resps.resp.rAvgCellVol);
    json_object_object_add(pRData->resps.resp.obj, "MinCellVol",
                           pRData->resps.resp.rMinCellVol);
    json_object_object_add(pRData->resps.resp.obj, "MaxCellNo",
                           pRData->resps.resp.rMaxCellNo);
    json_object_object_add(pRData->resps.resp.obj, "MinCellNo",
                           pRData->resps.resp.rMinCellNo);

    json_object_object_add(pRData->resps.resp.obj, "MaxTemp",
                           pRData->resps.resp.rMaxTemp);
    json_object_object_add(pRData->resps.resp.obj, "AvgTemp",
                           pRData->resps.resp.rAvgTemp);
    json_object_object_add(pRData->resps.resp.obj, "MinTemp",
                           pRData->resps.resp.rMinTemp);
    json_object_object_add(pRData->resps.resp.obj, "MaxTempNo",
                           pRData->resps.resp.rMaxTempNo);
    json_object_object_add(pRData->resps.resp.obj, "MinTempNo",
                           pRData->resps.resp.rMinTempNo);

    json_object_object_add(pRData->resps.resp.obj, "MaxChgPwr",
                           pRData->resps.resp.rMaxChgPwr);
    json_object_object_add(pRData->resps.resp.obj, "MaxDChgPwr",
                           pRData->resps.resp.rMaxDChgPwr);
    json_object_object_add(pRData->resps.resp.obj, "RealPwr",
                           pRData->resps.resp.rRealPwr);
    json_object_object_add(pRData->resps.resp.obj, "AvailCapa",
                           pRData->resps.resp.rAvailCapa);

    json_object_object_add(pRData->resps.resp.obj, "WrnCurrOver",
                           pRData->resps.resp.rWrnCurrOver);
    json_object_object_add(pRData->resps.resp.obj, "WrnCurrUnder",
                           pRData->resps.resp.rWrnCurrUnder);

    json_object_object_add(pRData->resps.resp.obj, "WrnSocOver",
                           pRData->resps.resp.rWrnSocOver);
    json_object_object_add(pRData->resps.resp.obj, "WrnSocUnder",
                           pRData->resps.resp.rWrnSocUnder);
    json_object_object_add(pRData->resps.resp.obj, "WrnCellOver",
                           pRData->resps.resp.rWrnCellOver);
    json_object_object_add(pRData->resps.resp.obj, "WrnCellUnder",
                           pRData->resps.resp.rWrnCellUnder);
    json_object_object_add(pRData->resps.resp.obj, "WrnCellDiff",
                           pRData->resps.resp.rWrnCellDiff);
    json_object_object_add(pRData->resps.resp.obj, "WrnTempOver",
                           pRData->resps.resp.rWrnTempOver);
    json_object_object_add(pRData->resps.resp.obj, "WrnTempUnder",
                           pRData->resps.resp.rWrnTempUnder);
    json_object_object_add(pRData->resps.resp.obj, "WrnTempDiff",
                           pRData->resps.resp.rWrnTempDiff);
    json_object_object_add(pRData->resps.resp.obj, "WrnPackOver",
                           pRData->resps.resp.rWrnPackOver);
    json_object_object_add(pRData->resps.resp.obj, "WrnPackUnder",
                           pRData->resps.resp.rWrnPackUnder);

    json_object_object_add(pRData->resps.resp.obj, "FltRlyDrv",
                           pRData->resps.resp.rFltRlyDrv);
    json_object_object_add(pRData->resps.resp.obj, "FltAuxPwr",
                           pRData->resps.resp.rFltAuxPwr);
    json_object_object_add(pRData->resps.resp.obj, "FltCsm",
                           pRData->resps.resp.rFltCsm);
    json_object_object_add(pRData->resps.resp.obj, "FltAfe",
                           pRData->resps.resp.rFltAfe);
    json_object_object_add(pRData->resps.resp.obj, "FltMcu",
                           pRData->resps.resp.rFltMcu);
    json_object_object_add(pRData->resps.resp.obj, "FltCurrOver",
                           pRData->resps.resp.rFltCurrOver);
    json_object_object_add(pRData->resps.resp.obj, "FltCurrUnder",
                           pRData->resps.resp.rFltCurrUnder);

    json_object_object_add(pRData->resps.resp.obj, "FltSocUnder",
                           pRData->resps.resp.rFltSocUnder);
    json_object_object_add(pRData->resps.resp.obj, "FltSohUnder",
                           pRData->resps.resp.rFltSohUnder);
    json_object_object_add(pRData->resps.resp.obj, "FltRlyAge",
                           pRData->resps.resp.rFltRlyAge);
    json_object_object_add(pRData->resps.resp.obj, "FltRlyOpen",
                           pRData->resps.resp.rFltRlyOpen);
    json_object_object_add(pRData->resps.resp.obj, "FltRlyWeld",
                           pRData->resps.resp.rFltRlyWeld);
    json_object_object_add(pRData->resps.resp.obj, "FltCellOver",
                           pRData->resps.resp.rFltCellOver);
    json_object_object_add(pRData->resps.resp.obj, "FltCellUnder",
                           pRData->resps.resp.rFltCellUnder);
    json_object_object_add(pRData->resps.resp.obj, "FltCellDiff",
                           pRData->resps.resp.rFltCellDiff);
    json_object_object_add(pRData->resps.resp.obj, "FltTempOver",
                           pRData->resps.resp.rFltTempOver);
    json_object_object_add(pRData->resps.resp.obj, "FltTempUnder",
                           pRData->resps.resp.rFltTempUnder);
    json_object_object_add(pRData->resps.resp.obj, "FltTempDiff",
                           pRData->resps.resp.rFltTempDiff);
    json_object_object_add(pRData->resps.resp.obj, "FltPackOver",
                           pRData->resps.resp.rFltPackOver);
    json_object_object_add(pRData->resps.resp.obj, "FltPackUnder",
                           pRData->resps.resp.rFltPackUnder);
}

#if true
static void composeEachJsonObj(IoTRespBmsData *pRData, RespDataInfo *pRespData, SenDataInfo *pSenData, uint64_t nRTime)
{
    /* GPS Data */
    json_object_set_int64(pRData->sens.timeStamp, nRTime);
    json_object_set_double(pRData->sens.rLatitude, pSenData->rLatitude);
    json_object_set_double(pRData->sens.rLongitude, pSenData->rLongitude);

    /* CAN Message */
    json_object_set_int64(pRData->resps.resp.timeStamp, nRTime);
    json_object_set_int(pRData->resps.resp.rCell01,
                        htons(pRespData->rResp00.st_daq_resp_00.sCell01));
    json_object_set_int(pRData->resps.resp.rCell02,
                        htons(pRespData->rResp00.st_daq_resp_00.sCell02));
    json_object_set_int(pRData->resps.resp.rCell03,
                        htons(pRespData->rResp00.st_daq_resp_00.sCell03));
    json_object_set_int(pRData->resps.resp.rCell04,
                        htons(pRespData->rResp00.st_daq_resp_00.sCell04));
    json_object_set_int(pRData->resps.resp.rCell05,
                        htons(pRespData->rResp01.st_daq_resp_01.sCell05));
    json_object_set_int(pRData->resps.resp.rCell06,
                        htons(pRespData->rResp01.st_daq_resp_01.sCell06));
    json_object_set_int(pRData->resps.resp.rCell07,
                        htons(pRespData->rResp01.st_daq_resp_01.sCell07));
    json_object_set_int(pRData->resps.resp.rCell08,
                        htons(pRespData->rResp01.st_daq_resp_01.sCell08));
    json_object_set_int(pRData->resps.resp.rCell09,
                        htons(pRespData->rResp02.st_daq_resp_02.sCell09));
    json_object_set_int(pRData->resps.resp.rCell10,
                        htons(pRespData->rResp02.st_daq_resp_02.sCell10));
    json_object_set_int(pRData->resps.resp.rCell11,
                        htons(pRespData->rResp02.st_daq_resp_02.sCell11));
    json_object_set_int(pRData->resps.resp.rCell12,
                        htons(pRespData->rResp02.st_daq_resp_02.sCell12));
    json_object_set_int(pRData->resps.resp.rCell13,
                        htons(pRespData->rResp03.st_daq_resp_03.sCell13));
    json_object_set_int(pRData->resps.resp.rCell14,
                        htons(pRespData->rResp03.st_daq_resp_03.sCell14));
    json_object_set_int(pRData->resps.resp.rCell15,
                        htons(pRespData->rResp03.st_daq_resp_03.sCell15));
    json_object_set_int(pRData->resps.resp.rCell16,
                        htons(pRespData->rResp03.st_daq_resp_03.sCell16));
    json_object_set_int(pRData->resps.resp.rCell17,
                        htons(pRespData->rResp04.st_daq_resp_04.sCell17));
    json_object_set_int(pRData->resps.resp.rCell18,
                        htons(pRespData->rResp04.st_daq_resp_04.sCell18));
    json_object_set_int(pRData->resps.resp.rCell19,
                        htons(pRespData->rResp04.st_daq_resp_04.sCell19));
    json_object_set_int(pRData->resps.resp.rCell20,
                        htons(pRespData->rResp04.st_daq_resp_04.sCell20));
    json_object_set_int(pRData->resps.resp.rCell21,
                        htons(pRespData->rResp05.st_daq_resp_05.sCell21));
    json_object_set_int(pRData->resps.resp.rCell22,
                        htons(pRespData->rResp05.st_daq_resp_05.sCell22));
    json_object_set_int(pRData->resps.resp.rCell23,
                        htons(pRespData->rResp05.st_daq_resp_05.sCell23));
    json_object_set_int(pRData->resps.resp.rCell24,
                        htons(pRespData->rResp05.st_daq_resp_05.sCell24));
    json_object_set_int(pRData->resps.resp.rCell25,
                        htons(pRespData->rResp06.st_daq_resp_06.sCell25));
    json_object_set_int(pRData->resps.resp.rCell26,
                        htons(pRespData->rResp06.st_daq_resp_06.sCell26));
    json_object_set_int(pRData->resps.resp.rCell27,
                        htons(pRespData->rResp06.st_daq_resp_06.sCell27));
    json_object_set_int(pRData->resps.resp.rCell28,
                        htons(pRespData->rResp06.st_daq_resp_06.sCell28));
    json_object_set_int(pRData->resps.resp.rCell29,
                        htons(pRespData->rResp07.st_daq_resp_07.sCell29));
    json_object_set_int(pRData->resps.resp.rCell30,
                        htons(pRespData->rResp07.st_daq_resp_07.sCell30));
    json_object_set_int(pRData->resps.resp.rCell31,
                        htons(pRespData->rResp07.st_daq_resp_07.sCell31));
    json_object_set_int(pRData->resps.resp.rCell32,
                        htons(pRespData->rResp07.st_daq_resp_07.sCell32));
    json_object_set_int(pRData->resps.resp.rCell33,
                        htons(pRespData->rResp08.st_daq_resp_08.sCell33));
    json_object_set_int(pRData->resps.resp.rCell34,
                        htons(pRespData->rResp08.st_daq_resp_08.sCell34));
    json_object_set_int(pRData->resps.resp.rCell35,
                        htons(pRespData->rResp08.st_daq_resp_08.sCell35));
    json_object_set_int(pRData->resps.resp.rCell36,
                        htons(pRespData->rResp08.st_daq_resp_08.sCell36));
    json_object_set_int(pRData->resps.resp.rCell37,
                        htons(pRespData->rResp09.st_daq_resp_09.sCell37));
    json_object_set_int(pRData->resps.resp.rCell38,
                        htons(pRespData->rResp09.st_daq_resp_09.sCell38));
    json_object_set_int(pRData->resps.resp.rCell39,
                        htons(pRespData->rResp09.st_daq_resp_09.sCell39));
    json_object_set_int(pRData->resps.resp.rCell40,
                        htons(pRespData->rResp09.st_daq_resp_09.sCell40));
    json_object_set_int(pRData->resps.resp.rCell41,
                        htons(pRespData->rResp0A.st_daq_resp_0A.sCell41));
    json_object_set_int(pRData->resps.resp.rCell42,
                        htons(pRespData->rResp0A.st_daq_resp_0A.sCell42));
    json_object_set_int(pRData->resps.resp.rCell43,
                        htons(pRespData->rResp0A.st_daq_resp_0A.sCell43));
    json_object_set_int(pRData->resps.resp.rCell44,
                        htons(pRespData->rResp0A.st_daq_resp_0A.sCell44));
    json_object_set_int(pRData->resps.resp.rCell45,
                        htons(pRespData->rResp0B.st_daq_resp_0B.sCell45));
    json_object_set_int(pRData->resps.resp.rCell46,
                        htons(pRespData->rResp0B.st_daq_resp_0B.sCell46));
    json_object_set_int(pRData->resps.resp.rCell47,
                        htons(pRespData->rResp0B.st_daq_resp_0B.sCell47));
    json_object_set_int(pRData->resps.resp.rCell48,
                        htons(pRespData->rResp0B.st_daq_resp_0B.sCell48));
    json_object_set_int(pRData->resps.resp.rCell49,
                        htons(pRespData->rResp0C.st_daq_resp_0C.sCell49));
    json_object_set_int(pRData->resps.resp.rCell50,
                        htons(pRespData->rResp0C.st_daq_resp_0C.sCell50));
    json_object_set_int(pRData->resps.resp.rCell51,
                        htons(pRespData->rResp0C.st_daq_resp_0C.sCell51));
    json_object_set_int(pRData->resps.resp.rCell52,
                        htons(pRespData->rResp0C.st_daq_resp_0C.sCell52));
    json_object_set_int(pRData->resps.resp.rCell53,
                        htons(pRespData->rResp0D.st_daq_resp_0D.sCell53));
    json_object_set_int(pRData->resps.resp.rCell54,
                        htons(pRespData->rResp0D.st_daq_resp_0D.sCell54));
    json_object_set_int(pRData->resps.resp.rCell55,
                        htons(pRespData->rResp0D.st_daq_resp_0D.sCell55));
    json_object_set_int(pRData->resps.resp.rCell56,
                        htons(pRespData->rResp0D.st_daq_resp_0D.sCell56));
    json_object_set_int(pRData->resps.resp.rCell57,
                        htons(pRespData->rResp0E.st_daq_resp_0E.sCell57));
    json_object_set_int(pRData->resps.resp.rCell58,
                        htons(pRespData->rResp0E.st_daq_resp_0E.sCell58));
    json_object_set_int(pRData->resps.resp.rCell59,
                        htons(pRespData->rResp0E.st_daq_resp_0E.sCell59));
    json_object_set_int(pRData->resps.resp.rCell60,
                        htons(pRespData->rResp0E.st_daq_resp_0E.sCell60));
    json_object_set_int(pRData->resps.resp.rCell61,
                        htons(pRespData->rResp0F.st_daq_resp_0F.sCell61));
    json_object_set_int(pRData->resps.resp.rCell62,
                        htons(pRespData->rResp0F.st_daq_resp_0F.sCell62));
    json_object_set_int(pRData->resps.resp.rCell63,
                        htons(pRespData->rResp0F.st_daq_resp_0F.sCell63));
    json_object_set_int(pRData->resps.resp.rCell64,
                        htons(pRespData->rResp0F.st_daq_resp_0F.sCell64));

    json_object_set_int(pRData->resps.resp.rTemp01,
                        (pRespData->rResp17.st_daq_resp_17.sTemp01));
    json_object_set_int(pRData->resps.resp.rTemp02,
                        (pRespData->rResp17.st_daq_resp_17.sTemp02));
    json_object_set_int(pRData->resps.resp.rTemp03,
                        (pRespData->rResp17.st_daq_resp_17.sTemp03));
    json_object_set_int(pRData->resps.resp.rTemp04,
                        (pRespData->rResp17.st_daq_resp_17.sTemp04));
    json_object_set_int(pRData->resps.resp.rTemp05,
                        (pRespData->rResp17.st_daq_resp_17.sTemp05));
    json_object_set_int(pRData->resps.resp.rTemp06,
                        (pRespData->rResp17.st_daq_resp_17.sTemp06));
    json_object_set_int(pRData->resps.resp.rTemp07,
                        (pRespData->rResp17.st_daq_resp_17.sTemp07));
    json_object_set_int(pRData->resps.resp.rTemp08,
                        (pRespData->rResp17.st_daq_resp_17.sTemp08));
    json_object_set_int(pRData->resps.resp.rTemp09,
                        (pRespData->rResp18.st_daq_resp_18.sTemp09));
    json_object_set_int(pRData->resps.resp.rTemp10,
                        (pRespData->rResp18.st_daq_resp_18.sTemp10));
    json_object_set_int(pRData->resps.resp.rTemp11,
                        (pRespData->rResp18.st_daq_resp_18.sTemp11));
    json_object_set_int(pRData->resps.resp.rTemp12,
                        (pRespData->rResp18.st_daq_resp_18.sTemp12));
    json_object_set_int(pRData->resps.resp.rTemp13,
                        (pRespData->rResp18.st_daq_resp_18.sTemp13));
    json_object_set_int(pRData->resps.resp.rTemp14,
                        (pRespData->rResp18.st_daq_resp_18.sTemp14));
    json_object_set_int(pRData->resps.resp.rTemp15,
                        (pRespData->rResp18.st_daq_resp_18.sTemp15));
    json_object_set_int(pRData->resps.resp.rTemp16,
                        (pRespData->rResp18.st_daq_resp_18.sTemp16));
    json_object_set_int(pRData->resps.resp.rTemp17,
                        (pRespData->rResp19.st_daq_resp_19.sTemp17));
    json_object_set_int(pRData->resps.resp.rTemp18,
                        (pRespData->rResp19.st_daq_resp_19.sTemp18));
    json_object_set_int(pRData->resps.resp.rTemp19,
                        (pRespData->rResp19.st_daq_resp_19.sTemp19));
    json_object_set_int(pRData->resps.resp.rTemp20,
                        (pRespData->rResp19.st_daq_resp_19.sTemp20));
    json_object_set_int(pRData->resps.resp.rTemp21,
                        (pRespData->rResp19.st_daq_resp_19.sTemp21));
    json_object_set_int(pRData->resps.resp.rTemp22,
                        (pRespData->rResp19.st_daq_resp_19.sTemp22));
    json_object_set_int(pRData->resps.resp.rTemp23,
                        (pRespData->rResp19.st_daq_resp_19.sTemp23));
    json_object_set_int(pRData->resps.resp.rTemp24,
                        (pRespData->rResp19.st_daq_resp_19.sTemp24));

    json_object_set_int(pRData->resps.resp.rModVol01,
                        htons(pRespData->rResp1B.st_daq_resp_1B.sModVol01));
    json_object_set_int(pRData->resps.resp.rModVol02,
                        htons(pRespData->rResp1B.st_daq_resp_1B.sModVol02));
    json_object_set_int(pRData->resps.resp.rModVol03,
                        htons(pRespData->rResp1B.st_daq_resp_1B.sModVol03));
    json_object_set_int(pRData->resps.resp.rModVol04,
                        htons(pRespData->rResp1B.st_daq_resp_1B.sModVol04));
    json_object_set_int(pRData->resps.resp.rModVol05,
                        htons(pRespData->rResp1C.st_daq_resp_1C.sModVol05));
    json_object_set_int(pRData->resps.resp.rModVol06,
                        htons(pRespData->rResp1C.st_daq_resp_1C.sModVol06));
    json_object_set_int(pRData->resps.resp.rModVol07,
                        htons(pRespData->rResp1C.st_daq_resp_1C.sModVol07));
    json_object_set_int(pRData->resps.resp.rModVol08,
                        htons(pRespData->rResp1C.st_daq_resp_1C.sModVol08));

    json_object_set_int(pRData->resps.resp.rPackVol,
                        htons(pRespData->rResp1D.st_daq_resp_1D.sPackVol));
    json_object_set_int(pRData->resps.resp.rInvVol,
                        htons(pRespData->rResp1D.st_daq_resp_1D.sInvVol));
    json_object_set_int(pRData->resps.resp.rPackSOC,
                        htons(pRespData->rResp1D.st_daq_resp_1D.sPackSOC));
    json_object_set_int(pRData->resps.resp.rPackSOH,
                        htons(pRespData->rResp1D.st_daq_resp_1D.sPackSOH));

    json_object_set_int(pRData->resps.resp.rPackCurr,
                        htons(pRespData->rResp1E.st_daq_resp_1E.sPackCurr));
    json_object_set_int(pRData->resps.resp.rBmsStat,
                        (pRespData->rResp1E.st_daq_resp_1E.sBmsStat));
    json_object_set_int(pRData->resps.resp.rBmsFltLvl,
                        (pRespData->rResp1E.st_daq_resp_1E.sBmsFltLvl));
    json_object_set_int(pRData->resps.resp.rRlyStat,
                        (pRespData->rResp1E.st_daq_resp_1E.sRlyStat));

    json_object_set_int(pRData->resps.resp.rMaxCellVol,
                        htons(pRespData->rResp1F.st_daq_resp_1F.sMaxCellVol));
    json_object_set_int(pRData->resps.resp.rAvgCellVol,
                        htons(pRespData->rResp1F.st_daq_resp_1F.sAvgCellVol));
    json_object_set_int(pRData->resps.resp.rMinCellVol,
                        htons(pRespData->rResp1F.st_daq_resp_1F.sMinCellVol));
    json_object_set_int(pRData->resps.resp.rMaxCellNo,
                        (pRespData->rResp1F.st_daq_resp_1F.sMaxCellNo));
    json_object_set_int(pRData->resps.resp.rMinCellNo,
                        (pRespData->rResp1F.st_daq_resp_1F.sMinCellNo));

    json_object_set_int(pRData->resps.resp.rMaxTemp,
                        (pRespData->rResp20.st_daq_resp_20.sMaxTemp));
    json_object_set_int(pRData->resps.resp.rAvgTemp,
                        (pRespData->rResp20.st_daq_resp_20.sAvgTemp));
    json_object_set_int(pRData->resps.resp.rMinTemp,
                        (pRespData->rResp20.st_daq_resp_20.sMinTemp));
    json_object_set_int(pRData->resps.resp.rMaxTempNo,
                        (pRespData->rResp20.st_daq_resp_20.sMaxTempNo));
    json_object_set_int(pRData->resps.resp.rMinTempNo,
                        (pRespData->rResp20.st_daq_resp_20.sMinTempNo));

    json_object_set_int(pRData->resps.resp.rMaxChgPwr,
                        htons(pRespData->rResp21.st_daq_resp_21.sMaxChgPwr));
    json_object_set_int(pRData->resps.resp.rMaxDChgPwr,
                        htons(pRespData->rResp21.st_daq_resp_21.sMaxDChgPwr));
    json_object_set_int(pRData->resps.resp.rRealPwr,
                        htons(pRespData->rResp21.st_daq_resp_21.sRealPwr));
    json_object_set_int(pRData->resps.resp.rAvailCapa,
                        htons(pRespData->rResp21.st_daq_resp_21.sAvailCapa));

    json_object_set_int(pRData->resps.resp.rWrnCurrOver,
                        (pRespData->rResp22.st_daq_resp_22.sWrnCurrOver));
    json_object_set_int(pRData->resps.resp.rWrnCurrUnder,
                        (pRespData->rResp22.st_daq_resp_22.sWrnCurrUnder));

    json_object_set_int(pRData->resps.resp.rWrnSocOver,
                        (pRespData->rResp22.st_daq_resp_22.sWrnSocOver));
    json_object_set_int(pRData->resps.resp.rWrnSocUnder,
                        (pRespData->rResp22.st_daq_resp_22.sWrnSocUnder));
    json_object_set_int(pRData->resps.resp.rWrnCellOver,
                        (pRespData->rResp22.st_daq_resp_22.sWrnCellOver));
    json_object_set_int(pRData->resps.resp.rWrnCellUnder,
                        (pRespData->rResp22.st_daq_resp_22.sWrnCellUnder));
    json_object_set_int(pRData->resps.resp.rWrnCellDiff,
                        (pRespData->rResp22.st_daq_resp_22.sWrnCellDiff));
    json_object_set_int(pRData->resps.resp.rWrnTempOver,
                        (pRespData->rResp22.st_daq_resp_22.sWrnTempOver));
    json_object_set_int(pRData->resps.resp.rWrnTempUnder,
                        (pRespData->rResp22.st_daq_resp_22.sWrnTempUnder));
    json_object_set_int(pRData->resps.resp.rWrnTempDiff,
                        (pRespData->rResp22.st_daq_resp_22.sWrnTempDiff));
    json_object_set_int(pRData->resps.resp.rWrnPackOver,
                        (pRespData->rResp22.st_daq_resp_22.sWrnPackOver));
    json_object_set_int(pRData->resps.resp.rWrnPackUnder,
                        (pRespData->rResp22.st_daq_resp_22.sWrnPackUnder));

    json_object_set_int(pRData->resps.resp.rFltRlyDrv,
                        (pRespData->rResp22.st_daq_resp_22.sFltRlyDrv));
    json_object_set_int(pRData->resps.resp.rFltAuxPwr,
                        (pRespData->rResp22.st_daq_resp_22.sFltAuxPwr));
    json_object_set_int(pRData->resps.resp.rFltCsm,
                        (pRespData->rResp22.st_daq_resp_22.sFltCsm));
    json_object_set_int(pRData->resps.resp.rFltAfe,
                        (pRespData->rResp22.st_daq_resp_22.sFltAfe));
    json_object_set_int(pRData->resps.resp.rFltMcu,
                        (pRespData->rResp22.st_daq_resp_22.sFltMcu));
    json_object_set_int(pRData->resps.resp.rFltCurrOver,
                        (pRespData->rResp22.st_daq_resp_22.sFltCurrOver));
    json_object_set_int(pRData->resps.resp.rFltCurrUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltCurrUnder));

    json_object_set_int(pRData->resps.resp.rFltSocUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltSocUnder));
    json_object_set_int(pRData->resps.resp.rFltSohUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltSohUnder));
    json_object_set_int(pRData->resps.resp.rFltRlyAge,
                        (pRespData->rResp22.st_daq_resp_22.sFltRlyAge));
    json_object_set_int(pRData->resps.resp.rFltRlyOpen,
                        (pRespData->rResp22.st_daq_resp_22.sFltRlyOpen));
    json_object_set_int(pRData->resps.resp.rFltRlyWeld,
                        (pRespData->rResp22.st_daq_resp_22.sFltRlyWeld));
    json_object_set_int(pRData->resps.resp.rFltCellOver,
                        (pRespData->rResp22.st_daq_resp_22.sFltCellOver));
    json_object_set_int(pRData->resps.resp.rFltCellUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltCellUnder));
    json_object_set_int(pRData->resps.resp.rFltCellDiff,
                        (pRespData->rResp22.st_daq_resp_22.sFltCellDiff));
    json_object_set_int(pRData->resps.resp.rFltTempOver,
                        (pRespData->rResp22.st_daq_resp_22.sFltTempOver));
    json_object_set_int(pRData->resps.resp.rFltTempUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltTempUnder));
    json_object_set_int(pRData->resps.resp.rFltTempDiff,
                        (pRespData->rResp22.st_daq_resp_22.sFltTempDiff));
    json_object_set_int(pRData->resps.resp.rFltPackOver,
                        (pRespData->rResp22.st_daq_resp_22.sFltPackOver));
    json_object_set_int(pRData->resps.resp.rFltPackUnder,
                        (pRespData->rResp22.st_daq_resp_22.sFltPackUnder));
}

#endif

#if false
static void composeBmsJsonObj(IoTRespBmsData *pRData, RespDataInfo *pRespData, uint64_t nRTime)
{
    json_object_set_int64(pRData->resps.timeStamp, nRTime);
    /* CAN Message */
    json_object_set_int64(pRData->resps.resp.timeStamp, nRTime);
    json_object_set_int(pRData->resps.resp.rCell01,
                        pRespData->rResp00.st_daq_resp_00.sCell01);
    json_object_set_int(pRData->resps.resp.rCell02,
                        pRespData->rResp00.st_daq_resp_00.sCell02);
    json_object_set_int(pRData->resps.resp.rCell03,
                        pRespData->rResp00.st_daq_resp_00.sCell03);
    json_object_set_int(pRData->resps.resp.rCell04,
                        pRespData->rResp00.st_daq_resp_00.sCell04);
    json_object_set_int(pRData->resps.resp.rCell05,
                        pRespData->rResp01.st_daq_resp_01.sCell05);
    json_object_set_int(pRData->resps.resp.rCell06,
                        pRespData->rResp01.st_daq_resp_01.sCell06);
    json_object_set_int(pRData->resps.resp.rCell07,
                        pRespData->rResp01.st_daq_resp_01.sCell07);
    json_object_set_int(pRData->resps.resp.rCell08,
                        pRespData->rResp01.st_daq_resp_01.sCell08);
    json_object_set_int(pRData->resps.resp.rCell09,
                        pRespData->rResp02.st_daq_resp_02.sCell09);
    json_object_set_int(pRData->resps.resp.rCell10,
                        pRespData->rResp02.st_daq_resp_02.sCell10);
    json_object_set_int(pRData->resps.resp.rCell11,
                        pRespData->rResp02.st_daq_resp_02.sCell11);
    json_object_set_int(pRData->resps.resp.rCell12,
                        pRespData->rResp02.st_daq_resp_02.sCell12);
    json_object_set_int(pRData->resps.resp.rCell13,
                        pRespData->rResp03.st_daq_resp_03.sCell13);
    json_object_set_int(pRData->resps.resp.rCell14,
                        pRespData->rResp03.st_daq_resp_03.sCell14);
    json_object_set_int(pRData->resps.resp.rCell15,
                        pRespData->rResp03.st_daq_resp_03.sCell15);
    json_object_set_int(pRData->resps.resp.rCell16,
                        pRespData->rResp03.st_daq_resp_03.sCell16);
    json_object_set_int(pRData->resps.resp.rCell17,
                        pRespData->rResp04.st_daq_resp_04.sCell17);
    json_object_set_int(pRData->resps.resp.rCell18,
                        pRespData->rResp04.st_daq_resp_04.sCell18);
    json_object_set_int(pRData->resps.resp.rCell19,
                        pRespData->rResp04.st_daq_resp_04.sCell19);
    json_object_set_int(pRData->resps.resp.rCell20,
                        pRespData->rResp04.st_daq_resp_04.sCell20);
    json_object_set_int(pRData->resps.resp.rCell21,
                        pRespData->rResp05.st_daq_resp_05.sCell21);
    json_object_set_int(pRData->resps.resp.rCell22,
                        pRespData->rResp05.st_daq_resp_05.sCell22);
    json_object_set_int(pRData->resps.resp.rCell23,
                        pRespData->rResp05.st_daq_resp_05.sCell23);
    json_object_set_int(pRData->resps.resp.rCell24,
                        pRespData->rResp05.st_daq_resp_05.sCell24);
    json_object_set_int(pRData->resps.resp.rCell25,
                        pRespData->rResp06.st_daq_resp_06.sCell25);
    json_object_set_int(pRData->resps.resp.rCell26,
                        pRespData->rResp06.st_daq_resp_06.sCell26);
    json_object_set_int(pRData->resps.resp.rCell27,
                        pRespData->rResp06.st_daq_resp_06.sCell27);
    json_object_set_int(pRData->resps.resp.rCell28,
                        pRespData->rResp06.st_daq_resp_06.sCell28);
    json_object_set_int(pRData->resps.resp.rCell29,
                        pRespData->rResp07.st_daq_resp_07.sCell29);
    json_object_set_int(pRData->resps.resp.rCell30,
                        pRespData->rResp07.st_daq_resp_07.sCell30);
    json_object_set_int(pRData->resps.resp.rCell31,
                        pRespData->rResp07.st_daq_resp_07.sCell31);
    json_object_set_int(pRData->resps.resp.rCell32,
                        pRespData->rResp07.st_daq_resp_07.sCell32);
    json_object_set_int(pRData->resps.resp.rCell33,
                        pRespData->rResp08.st_daq_resp_08.sCell33);
    json_object_set_int(pRData->resps.resp.rCell34,
                        pRespData->rResp08.st_daq_resp_08.sCell34);
    json_object_set_int(pRData->resps.resp.rCell35,
                        pRespData->rResp08.st_daq_resp_08.sCell35);
    json_object_set_int(pRData->resps.resp.rCell36,
                        pRespData->rResp08.st_daq_resp_08.sCell36);
    json_object_set_int(pRData->resps.resp.rCell37,
                        pRespData->rResp09.st_daq_resp_09.sCell37);
    json_object_set_int(pRData->resps.resp.rCell38,
                        pRespData->rResp09.st_daq_resp_09.sCell38);
    json_object_set_int(pRData->resps.resp.rCell39,
                        pRespData->rResp09.st_daq_resp_09.sCell39);
    json_object_set_int(pRData->resps.resp.rCell40,
                        pRespData->rResp09.st_daq_resp_09.sCell40);
    json_object_set_int(pRData->resps.resp.rCell41,
                        pRespData->rResp0A.st_daq_resp_0A.sCell41);
    json_object_set_int(pRData->resps.resp.rCell42,
                        pRespData->rResp0A.st_daq_resp_0A.sCell42);
    json_object_set_int(pRData->resps.resp.rCell43,
                        pRespData->rResp0A.st_daq_resp_0A.sCell43);
    json_object_set_int(pRData->resps.resp.rCell44,
                        pRespData->rResp0A.st_daq_resp_0A.sCell44);
    json_object_set_int(pRData->resps.resp.rCell45,
                        pRespData->rResp0B.st_daq_resp_0B.sCell45);
    json_object_set_int(pRData->resps.resp.rCell46,
                        pRespData->rResp0B.st_daq_resp_0B.sCell46);
    json_object_set_int(pRData->resps.resp.rCell47,
                        pRespData->rResp0B.st_daq_resp_0B.sCell47);
    json_object_set_int(pRData->resps.resp.rCell48,
                        pRespData->rResp0B.st_daq_resp_0B.sCell48);
    json_object_set_int(pRData->resps.resp.rCell49,
                        pRespData->rResp0C.st_daq_resp_0C.sCell49);
    json_object_set_int(pRData->resps.resp.rCell50,
                        pRespData->rResp0C.st_daq_resp_0C.sCell50);
    json_object_set_int(pRData->resps.resp.rCell51,
                        pRespData->rResp0C.st_daq_resp_0C.sCell51);
    json_object_set_int(pRData->resps.resp.rCell52,
                        pRespData->rResp0C.st_daq_resp_0C.sCell52);
    json_object_set_int(pRData->resps.resp.rCell53,
                        pRespData->rResp0D.st_daq_resp_0D.sCell53);
    json_object_set_int(pRData->resps.resp.rCell54,
                        pRespData->rResp0D.st_daq_resp_0D.sCell54);
    json_object_set_int(pRData->resps.resp.rCell55,
                        pRespData->rResp0D.st_daq_resp_0D.sCell55);
    json_object_set_int(pRData->resps.resp.rCell56,
                        pRespData->rResp0D.st_daq_resp_0D.sCell56);
    json_object_set_int(pRData->resps.resp.rCell57,
                        pRespData->rResp0E.st_daq_resp_0E.sCell57);
    json_object_set_int(pRData->resps.resp.rCell58,
                        pRespData->rResp0E.st_daq_resp_0E.sCell58);
    json_object_set_int(pRData->resps.resp.rCell59,
                        pRespData->rResp0E.st_daq_resp_0E.sCell59);
    json_object_set_int(pRData->resps.resp.rCell60,
                        pRespData->rResp0E.st_daq_resp_0E.sCell60);
    json_object_set_int(pRData->resps.resp.rCell61,
                        pRespData->rResp0F.st_daq_resp_0F.sCell61);
    json_object_set_int(pRData->resps.resp.rCell62,
                        pRespData->rResp0F.st_daq_resp_0F.sCell62);
    json_object_set_int(pRData->resps.resp.rCell63,
                        pRespData->rResp0F.st_daq_resp_0F.sCell63);
    json_object_set_int(pRData->resps.resp.rCell64,
                        pRespData->rResp0F.st_daq_resp_0F.sCell64);

    json_object_set_int(pRData->resps.resp.rTemp01,
                        pRespData->rResp17.st_daq_resp_17.sTemp01);
    json_object_set_int(pRData->resps.resp.rTemp02,
                        pRespData->rResp17.st_daq_resp_17.sTemp02);
    json_object_set_int(pRData->resps.resp.rTemp03,
                        pRespData->rResp17.st_daq_resp_17.sTemp03);
    json_object_set_int(pRData->resps.resp.rTemp04,
                        pRespData->rResp17.st_daq_resp_17.sTemp04);
    json_object_set_int(pRData->resps.resp.rTemp05,
                        pRespData->rResp17.st_daq_resp_17.sTemp05);
    json_object_set_int(pRData->resps.resp.rTemp06,
                        pRespData->rResp17.st_daq_resp_17.sTemp06);
    json_object_set_int(pRData->resps.resp.rTemp07,
                        pRespData->rResp17.st_daq_resp_17.sTemp07);
    json_object_set_int(pRData->resps.resp.rTemp08,
                        pRespData->rResp17.st_daq_resp_17.sTemp08);
    json_object_set_int(pRData->resps.resp.rTemp09,
                        pRespData->rResp18.st_daq_resp_18.sTemp09);
    json_object_set_int(pRData->resps.resp.rTemp10,
                        pRespData->rResp18.st_daq_resp_18.sTemp10);
    json_object_set_int(pRData->resps.resp.rTemp11,
                        pRespData->rResp18.st_daq_resp_18.sTemp11);
    json_object_set_int(pRData->resps.resp.rTemp12,
                        pRespData->rResp18.st_daq_resp_18.sTemp12);
    json_object_set_int(pRData->resps.resp.rTemp13,
                        pRespData->rResp18.st_daq_resp_18.sTemp13);
    json_object_set_int(pRData->resps.resp.rTemp14,
                        pRespData->rResp18.st_daq_resp_18.sTemp14);
    json_object_set_int(pRData->resps.resp.rTemp15,
                        pRespData->rResp18.st_daq_resp_18.sTemp15);
    json_object_set_int(pRData->resps.resp.rTemp16,
                        pRespData->rResp18.st_daq_resp_18.sTemp16);
    json_object_set_int(pRData->resps.resp.rTemp17,
                        pRespData->rResp19.st_daq_resp_19.sTemp17);
    json_object_set_int(pRData->resps.resp.rTemp18,
                        pRespData->rResp19.st_daq_resp_19.sTemp18);
    json_object_set_int(pRData->resps.resp.rTemp19,
                        pRespData->rResp19.st_daq_resp_19.sTemp19);
    json_object_set_int(pRData->resps.resp.rTemp20,
                        pRespData->rResp19.st_daq_resp_19.sTemp20);
    json_object_set_int(pRData->resps.resp.rTemp21,
                        pRespData->rResp19.st_daq_resp_19.sTemp21);
    json_object_set_int(pRData->resps.resp.rTemp22,
                        pRespData->rResp19.st_daq_resp_19.sTemp22);
    json_object_set_int(pRData->resps.resp.rTemp23,
                        pRespData->rResp19.st_daq_resp_19.sTemp23);
    json_object_set_int(pRData->resps.resp.rTemp24,
                        pRespData->rResp19.st_daq_resp_19.sTemp24);

    json_object_set_int(pRData->resps.resp.rModVol01,
                        pRespData->rResp1B.st_daq_resp_1B.sModVol01);
    json_object_set_int(pRData->resps.resp.rModVol02,
                        pRespData->rResp1B.st_daq_resp_1B.sModVol02);
    json_object_set_int(pRData->resps.resp.rModVol03,
                        pRespData->rResp1B.st_daq_resp_1B.sModVol03);
    json_object_set_int(pRData->resps.resp.rModVol04,
                        pRespData->rResp1B.st_daq_resp_1B.sModVol04);
    json_object_set_int(pRData->resps.resp.rModVol05,
                        pRespData->rResp1C.st_daq_resp_1C.sModVol05);
    json_object_set_int(pRData->resps.resp.rModVol06,
                        pRespData->rResp1C.st_daq_resp_1C.sModVol06);
    json_object_set_int(pRData->resps.resp.rModVol07,
                        pRespData->rResp1C.st_daq_resp_1C.sModVol07);
    json_object_set_int(pRData->resps.resp.rModVol08,
                        pRespData->rResp1C.st_daq_resp_1C.sModVol08);

    json_object_set_int(pRData->resps.resp.rPackVol,
                        pRespData->rResp1D.st_daq_resp_1D.sPackVol);
    json_object_set_int(pRData->resps.resp.rInvVol,
                        pRespData->rResp1D.st_daq_resp_1D.sInvVol);
    json_object_set_int(pRData->resps.resp.rPackSOC,
                        pRespData->rResp1D.st_daq_resp_1D.sPackSOC);
    json_object_set_int(pRData->resps.resp.rPackSOH,
                        pRespData->rResp1D.st_daq_resp_1D.sPackSOH);

    json_object_set_int(pRData->resps.resp.rPackCurr,
                        pRespData->rResp1E.st_daq_resp_1E.sPackCurr);
    json_object_set_int(pRData->resps.resp.rBmsStat,
                        pRespData->rResp1E.st_daq_resp_1E.sBmsStat);
    json_object_set_int(pRData->resps.resp.rBmsFltLvl,
                        pRespData->rResp1E.st_daq_resp_1E.sBmsFltLvl);
    json_object_set_int(pRData->resps.resp.rRlyStat,
                        pRespData->rResp1E.st_daq_resp_1E.sRlyStat);

    json_object_set_int(pRData->resps.resp.rMaxCellVol,
                        pRespData->rResp1F.st_daq_resp_1F.sMaxCellVol);
    json_object_set_int(pRData->resps.resp.rAvgCellVol,
                        pRespData->rResp1F.st_daq_resp_1F.sAvgCellVol);
    json_object_set_int(pRData->resps.resp.rMinCellVol,
                        pRespData->rResp1F.st_daq_resp_1F.sMinCellVol);
    json_object_set_int(pRData->resps.resp.rMaxCellNo,
                        pRespData->rResp1F.st_daq_resp_1F.sMaxCellNo);
    json_object_set_int(pRData->resps.resp.rMinCellNo,
                        pRespData->rResp1F.st_daq_resp_1F.sMinCellNo);

    json_object_set_int(pRData->resps.resp.rMaxTemp,
                        pRespData->rResp20.st_daq_resp_20.sMaxTemp);
    json_object_set_int(pRData->resps.resp.rAvgTemp,
                        pRespData->rResp20.st_daq_resp_20.sAvgTemp);
    json_object_set_int(pRData->resps.resp.rMinTemp,
                        pRespData->rResp20.st_daq_resp_20.sMinTemp);
    json_object_set_int(pRData->resps.resp.rMaxTempNo,
                        pRespData->rResp20.st_daq_resp_20.sMaxTempNo);
    json_object_set_int(pRData->resps.resp.rMinTempNo,
                        pRespData->rResp20.st_daq_resp_20.sMinTempNo);

    json_object_set_int(pRData->resps.resp.rMaxChgPwr,
                        pRespData->rResp21.st_daq_resp_21.sMaxChgPwr);
    json_object_set_int(pRData->resps.resp.rMaxDChgPwr,
                        pRespData->rResp21.st_daq_resp_21.sMaxDChgPwr);
    json_object_set_int(pRData->resps.resp.rRealPwr,
                        pRespData->rResp21.st_daq_resp_21.sRealPwr);
    json_object_set_int(pRData->resps.resp.rAvailCapa,
                        pRespData->rResp21.st_daq_resp_21.sAvailCapa);

    json_object_set_int(pRData->resps.resp.rWrnCurrOver,
                        pRespData->rResp22.st_daq_resp_22.sWrnCurrOver);
    json_object_set_int(pRData->resps.resp.rWrnCurrUnder,
                        pRespData->rResp22.st_daq_resp_22.sWrnCurrUnder);

    json_object_set_int(pRData->resps.resp.rWrnSocOver,
                        pRespData->rResp22.st_daq_resp_22.sWrnSocOver);
    json_object_set_int(pRData->resps.resp.rWrnSocUnder,
                        pRespData->rResp22.st_daq_resp_22.sWrnSocUnder);
    json_object_set_int(pRData->resps.resp.rWrnCellOver,
                        pRespData->rResp22.st_daq_resp_22.sWrnCellOver);
    json_object_set_int(pRData->resps.resp.rWrnCellUnder,
                        pRespData->rResp22.st_daq_resp_22.sWrnCellUnder);
    json_object_set_int(pRData->resps.resp.rWrnCellDiff,
                        pRespData->rResp22.st_daq_resp_22.sWrnCellDiff);
    json_object_set_int(pRData->resps.resp.rWrnTempOver,
                        pRespData->rResp22.st_daq_resp_22.sWrnTempOver);
    json_object_set_int(pRData->resps.resp.rWrnTempUnder,
                        pRespData->rResp22.st_daq_resp_22.sWrnTempUnder);
    json_object_set_int(pRData->resps.resp.rWrnTempDiff,
                        pRespData->rResp22.st_daq_resp_22.sWrnTempDiff);
    json_object_set_int(pRData->resps.resp.rWrnPackOver,
                        pRespData->rResp22.st_daq_resp_22.sWrnPackOver);
    json_object_set_int(pRData->resps.resp.rWrnPackUnder,
                        pRespData->rResp22.st_daq_resp_22.sWrnPackUnder);

    json_object_set_int(pRData->resps.resp.rFltRlyDrv,
                        pRespData->rResp22.st_daq_resp_22.sFltRlyDrv);
    json_object_set_int(pRData->resps.resp.rFltAuxPwr,
                        pRespData->rResp22.st_daq_resp_22.sFltAuxPwr);
    json_object_set_int(pRData->resps.resp.rFltCsm,
                        pRespData->rResp22.st_daq_resp_22.sFltCsm);
    json_object_set_int(pRData->resps.resp.rFltAfe,
                        pRespData->rResp22.st_daq_resp_22.sFltAfe);
    json_object_set_int(pRData->resps.resp.rFltMcu,
                        pRespData->rResp22.st_daq_resp_22.sFltMcu);
    json_object_set_int(pRData->resps.resp.rFltCurrOver,
                        pRespData->rResp22.st_daq_resp_22.sFltCurrOver);
    json_object_set_int(pRData->resps.resp.rFltCurrUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltCurrUnder);

    json_object_set_int(pRData->resps.resp.rFltSocOver,
                        pRespData->rResp22.st_daq_resp_22.sFltSocOver);
    json_object_set_int(pRData->resps.resp.rFltSocUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltSocUnder);
    json_object_set_int(pRData->resps.resp.rFltSohUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltSohUnder);
    json_object_set_int(pRData->resps.resp.rFltRlyAge,
                        pRespData->rResp22.st_daq_resp_22.sFltRlyAge);
    json_object_set_int(pRData->resps.resp.rFltRlyOpen,
                        pRespData->rResp22.st_daq_resp_22.sFltRlyOpen);
    json_object_set_int(pRData->resps.resp.rFltRlyWeld,
                        pRespData->rResp22.st_daq_resp_22.sFltRlyWeld);
    json_object_set_int(pRData->resps.resp.rFltCellOver,
                        pRespData->rResp22.st_daq_resp_22.sFltCellOver);
    json_object_set_int(pRData->resps.resp.rFltCellUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltCellUnder);
    json_object_set_int(pRData->resps.resp.rFltCellDiff,
                        pRespData->rResp22.st_daq_resp_22.sFltCellDiff);
    json_object_set_int(pRData->resps.resp.rFltTempOver,
                        pRespData->rResp22.st_daq_resp_22.sFltTempOver);
    json_object_set_int(pRData->resps.resp.rFltTempUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltTempUnder);
    json_object_set_int(pRData->resps.resp.rFltTempDiff,
                        pRespData->rResp22.st_daq_resp_22.sFltTempDiff);
    json_object_set_int(pRData->resps.resp.rFltPackOver,
                        pRespData->rResp22.st_daq_resp_22.sFltPackOver);
    json_object_set_int(pRData->resps.resp.rFltPackUnder,
                        pRespData->rResp22.st_daq_resp_22.sFltPackUnder);
}

static void composeGpsJsonObj(IoTRespBmsData *pRData, SenDataInfo *pSenData, uint64_t nRTime)
{
    json_object_set_int64(pRData->sens.timeStamp, nRTime);

    json_object_set_int64(pRData->sens.timeStamp, nRTime);
    json_object_set_double(pRData->sens.rLatitude, pSenData->rLatitude);
    json_object_set_double(pRData->sens.rLongitude, pSenData->rLongitude);
}

#endif
char* substr(const char *src, int m, int n)
{
    int len = n - m;
    char *dest = (char*)malloc(sizeof(char) * (len + 1));

    for (int i = m; i < n && (*(src + i) != '\0'); i++)
    {
        *dest = *(src + i);
        dest++;
    }

    *dest = '\0';

    return dest - len;
}


void usage(char *argv_0)
{
    printf("\nUsage: %s [-d <device>] [-s <speed>] [-h]\n", argv_0);
    printf("  -d <device>    Serial device, default is /dev/ttyO4\n");
    printf("  -s <speed>     Speed, default is 115200\n");
    printf("  -b <pattern>   Send this byte only, example -b 0xaa\n");
    printf("  -h             Show this help\n\n");
    exit(1);
}

#if 1
int serial_read(void)
{
    char device[64];
    int speed;
    unsigned char byte;
    struct termios oldopts;

    strcpy(device, DEFAULT_PORT);
    speed = DEFAULT_SPEED;
    byte = 0;

    int fd = open_port(device, speed, &oldopts);

    if (fd < 0)
        exit(1);

    done = 0;
    run_test(fd, byte);

    tcsetattr(fd, TCSANOW, &oldopts);

    close(fd);

    return 0;
}
#endif

void run_test(int fd, unsigned char byte)
{
    int len, txlen, pos;
    int retries;
    char tx[64];
    char rx[64];
    char fullMsg[1024] = "";
    bool gpsFlag = false;
    float tempLati = 0;
    float tempLongi = 0;

    if (byte)
    {
        memset(tx, byte, sizeof(tx));
        txlen = sizeof(tx);
    }
    else
    {
        strcpy(tx, "ABCDEFJHIJKLMNOPQRSTUVWXYZ1234567890abcdefjhijklmnopqrstuvwxyz");
        txlen = strlen(tx);
    }

    memset(fullMsg, 0, sizeof(fullMsg));
    while (!done)
    {
        memset(rx, 0, sizeof(rx));

        len = write(fd, tx, txlen);

        if (len != txlen)
        {
            perror("write");
            g_stSenInfo.iRunning = false;
            break;
        }

        pos = 0;
        retries = 0;

        while (pos < txlen && !done)
        {
            len = read(fd, rx + pos, txlen - pos);

            if (len < 0)
            {
                perror("read");
                done = 1;
                g_stSenInfo.iRunning = false;
                break;
            }

            pos += len;

            if (retries++ > 5)
            {
//                LOG_WARN("read error Giving up\n");
                g_stSenInfo.iRunning = false;
                done = 1;
            }
        }

        if (!done)
        {
            if (byte)
            {
                if (!memcmp(rx, tx, txlen))
                    printf("Read %d matching bytes\n", txlen);
                else
                {
                    g_stSenInfo.iRunning = false;
//                    LOG_WARN("Read did not match\n");
                }
            }
            else
            {
                strcat(fullMsg, rx);

                char *begin = strstr(fullMsg, "GNVTG,");
//                printf("begin:%s\n", begin);

                if (begin != NULL)
                {
                    char *end = strstr(fullMsg, "GNRMC");
                    if (end != NULL)
                    {
                        char *result = strtok(end, "\n");

//                        printf("1 : %s\n", result);

                        char *temp = strtok(result, ",");

                        for (int i = 0; i < 6; i++)
                        {
//                            printf("%s\n", temp);
                            switch(i)
                            {

                            case 3:
                                tempLati = (float) strtod(temp, NULL);
                                break;
                            case 4:
                                if(strcmp(temp, "N") == 0 || strcmp(temp, "S") == 0)
                                {
                                    gpsFlag = true;
                                }
                                else
                                    gpsFlag = false;
                                break;
                            case 5:
                                tempLongi = (float) strtod(temp, NULL);
                                break;
                            case 6:
                                if(strcmp(temp, "E") == 0 || strcmp(temp, "W") == 0)
                                {
                                    gpsFlag = true;
                                }
                                else
                                    gpsFlag = false;
                                break;

                            default:
                                break;

                            }

                            temp = strtok(NULL, ",");
                            if (temp == NULL)
                            {
//                                LOG_WARN("GPS Not Working");
                                break;
                            }

                        }

                        if (gpsFlag)
                        {
                            char src[15] = "";
                            sprintf(src, "%f", tempLati);
                            char *fnd = strstr(src, ".");

                            char *dd = substr(src, 0, fnd - src - 2);
                            char *dms = substr(src, fnd - src - 2, strlen(src));

                            float fDms = (float) strtod(dms, NULL) / 60;
                            float fDd = (float) strtod(dd, NULL);

                            gps[0] = fDms + fDd;

                            sprintf(src, "%f", tempLongi);
                            fnd = strstr(src, ".");

                            dd = substr(src, 0, fnd - src - 2);
                            dms = substr(src, fnd - src - 2, strlen(src));

                            fDms = (float) strtod(dms, NULL) / 60;
                            fDd = (float) strtod(dd, NULL);

                            gps[1] = fDms + fDd;
                        }

                        memset(fullMsg, 0, sizeof(fullMsg));
                        done = 1;
                        g_stSenInfo.iRunning = true;
                    }
                }

            }
        }
    }
}

int open_port(const char *device, int speed, struct termios *oldopts)
{
    int fd, unix_speed;
    struct termios opts;

    unix_speed = map_speed_to_unix(speed);

    if (unix_speed < 0)
        return -1;

    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd < 0) {
        perror("open");
        return -1;
    }

    // no delay
    fcntl(fd, F_SETFL, 0);

    tcgetattr(fd, &opts);

    if (oldopts)
        memcpy(oldopts, &opts, sizeof(struct termios));

    opts.c_cflag &= ~CSIZE;
    opts.c_cflag &= ~CSTOPB;
    opts.c_cflag &= ~PARENB;
    opts.c_cflag |= CLOCAL | CREAD | CS8;
    opts.c_iflag = IGNPAR;
    opts.c_oflag = 0;
    opts.c_lflag = 0;

    opts.c_cc[VTIME] = 5;
    opts.c_cc[VMIN] = 0;

    cfsetispeed(&opts, unix_speed);
    cfsetospeed(&opts, unix_speed);

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &opts);

    return fd;
}

int map_speed_to_unix(int speed)
{
    int unix_speed;

    switch (speed) {
    case 50: unix_speed = B50; break;
    case 75: unix_speed = B75; break;
    case 110: unix_speed = B110; break;
    case 134: unix_speed = B134; break;
    case 150: unix_speed = B150; break;
    case 300: unix_speed = B300; break;
    case 600: unix_speed = B600; break;
    case 1200: unix_speed = B1200; break;
    case 1800: unix_speed = B1800; break;
    case 2400: unix_speed = B2400; break;
    case 4800: unix_speed = B4800; break;
    case 9600: unix_speed = B9600; break;
    case 19200: unix_speed = B19200; break;
    case 38400: unix_speed = B38400; break;
    case 57600: unix_speed = B57600; break;
    case 115200: unix_speed = B115200; break;
    case 230400: unix_speed = B230400; break;
    case 460800: unix_speed = B460800; break;
    case 500000: unix_speed = B500000; break;
    case 576000: unix_speed = B576000; break;
    case 921600: unix_speed = B921600; break;
    case 1000000: unix_speed = B1000000; break;
    case 1152000: unix_speed = B1152000; break;
    case 1500000: unix_speed = B1500000; break;
    case 2000000: unix_speed = B2000000; break;
    case 2500000: unix_speed = B2500000; break;
    case 3000000: unix_speed = B3000000; break;
    case 3500000: unix_speed = B3500000; break;
    case 4000000: unix_speed = B4000000; break;
    default: unix_speed = -1; break;
    }

    return unix_speed;
}

void register_sig_handler()
{
    struct sigaction sia;

    bzero(&sia, sizeof sia);
    sia.sa_handler = sigint_handler;

    if (sigaction(SIGINT, &sia, NULL) < 0) {
        perror("sigaction(SIGINT)");
        exit(1);
    }
}

void sigint_handler(int sig)
{
    done = 1;
}
/**
 * @ Name : static void checkMsgLED(RespDataInfo *pRespData)
 * @ Author : SK
 * @ Date : 2021.06.04
 *
 * @ Parameters[in]  : RespDataInfo *
 * @ Parameters[out] : -
 * @ Return Value : -
 *
 * @ brief :
 *    Send a specific CAN data to LED process
 *    1. Get a available power capacity from CAN
 *    2. Get a BMS failure data from CAN
 *    3. Send a data to LED process using message send system call
**/
static void checkMsgLED(RespDataInfo *pRespData)
{
    IoTLedMsg ledMsg;

    /* Available power */
    if(pRespData->rResp21.st_daq_resp_21.sAvailCapa >= 0)
    {
        /* Send message to LED proc */
        ledMsg.msgtype = IOT_LEDMSG_BATLEVEL;
        ledMsg.BMS_AvailPwr = pRespData->rResp21.st_daq_resp_21.sAvailCapa;
        msgsnd(g_s32LedMsgID, &ledMsg, sizeof(IoTLedMsg) - sizeof(long),0);

    }
    else ;

    /* BMS fault : defined CAN DB*/
    if(pRespData->rResp22.st_daq_resp_22.sFltAfe || pRespData->rResp22.st_daq_resp_22.sFltAuxPwr ||
       pRespData->rResp22.st_daq_resp_22.sFltCanComm || pRespData->rResp22.st_daq_resp_22.sFltCellDiff ||
       pRespData->rResp22.st_daq_resp_22.sFltCellOver || pRespData->rResp22.st_daq_resp_22.sFltCellUnder ||
       pRespData->rResp22.st_daq_resp_22.sFltCsm || pRespData->rResp22.st_daq_resp_22.sFltCurrOver ||
       pRespData->rResp22.st_daq_resp_22.sFltCurrUnder || pRespData->rResp22.st_daq_resp_22.sFltMcu ||
       pRespData->rResp22.st_daq_resp_22.sFltPackOver || pRespData->rResp22.st_daq_resp_22.sFltPackUnder ||
       pRespData->rResp22.st_daq_resp_22.sFltRlyAge || pRespData->rResp22.st_daq_resp_22.sFltRlyDrv ||
       pRespData->rResp22.st_daq_resp_22.sFltRlyOpen || pRespData->rResp22.st_daq_resp_22.sFltRlyWeld ||
       pRespData->rResp22.st_daq_resp_22.sFltRsrv1 || pRespData->rResp22.st_daq_resp_22.sFltRsrv2 ||
       pRespData->rResp22.st_daq_resp_22.sFltSocOver || pRespData->rResp22.st_daq_resp_22.sFltSocUnder ||
       pRespData->rResp22.st_daq_resp_22.sFltSohUnder || pRespData->rResp22.st_daq_resp_22.sFltTempDiff ||
       pRespData->rResp22.st_daq_resp_22.sFltTempOver || pRespData->rResp22.st_daq_resp_22.sFltTempUnder)
    {
        ledMsg.msgtype = IOT_LEDMSG_BMSFAULT;
        ledMsg.BMS_status = 1;
        msgsnd(g_s32LedMsgID, &ledMsg, sizeof(IoTLedMsg) - sizeof(long),0);

    }
    else ;

}

static void procDCStatusUpdate(int evt, int fail_idx)
{
    int rtn = 0;

    pthread_mutex_lock(&g_mtxDcMainMsg);
    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
    g_stMainMsg.proc = PROC_TYPE_DC;
    g_stMainMsg.data[DATA_TYPE_EVENT] = evt;
    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = fail_idx;
    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg,
                 sizeof(IoTMainMsg) - sizeof(long), 0);
    pthread_mutex_unlock(&g_mtxDcMainMsg);

    if (rtn < 0) /* Fault inject num: 102 */
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed send message to MAIN proc message queue");
        reboot_system();
    }
}
void procDCStatusUpdate_Extern(int evt, int fail_idx)
{
    int rtn = 0;

    pthread_mutex_lock(&g_mtxDcMainMsg);
    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
    g_stMainMsg.proc = PROC_TYPE_DC;
    g_stMainMsg.data[DATA_TYPE_EVENT] = evt;
    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = fail_idx;
    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg,
                 sizeof(IoTMainMsg) - sizeof(long), 0);
    pthread_mutex_unlock(&g_mtxDcMainMsg);

    printf("rtn : %d\n", rtn);
    if (rtn < 0) /* Fault inject num: 102 */
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed send message to MAIN proc message queue");
        reboot_system();
    }
}

static void dc_event_reportCB(int evt, int idx)
{
    pthread_mutex_lock(&g_mtxEvtCB);
    procDCStatusUpdate(evt, idx);
    pthread_mutex_unlock(&g_mtxEvtCB);
}

static void *dc_alive_check_thread(void *arg)
{
    static int threadCnt = 0;
    int rtn = 0, retryCnt = 0;

    while (1)
    {
        if (IOT_ALIVE_TIME == threadCnt) /* 1 minute */
        {
            /* Tx alive message to MAIN proc */
            pthread_mutex_lock(&g_mtxDcMainMsg);
            g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
            g_stMainMsg.proc = PROC_TYPE_DC;
            g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
            g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
            rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg,
                         sizeof(IoTMainMsg) - sizeof(long), 0);
            pthread_mutex_unlock(&g_mtxDcMainMsg);

            if (rtn < 0) /* Fault inject num: 101 */
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Failed send message to MAIN proc message queue");
                procDCStatusUpdate(IOT_EVT_SYS_FAIL, 1);
                retryCnt = 0;

                do
                {
                    pthread_mutex_lock(&g_mtxDcMainMsg);
                    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
                    g_stMainMsg.proc = PROC_TYPE_DC;
                    g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
                    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
                    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg,
                                 sizeof(IoTMainMsg) - sizeof(long), 0);
                    pthread_mutex_unlock(&g_mtxDcMainMsg);

                    retryCnt++;
//                    LOG_INFO(
//                            "Retry to tx MAIN proc message queue (return:%d, retry count:%d)",
//                            rtn, retryCnt);
                }
                while ((rtn < 0) && (retryCnt < IOT_RETRY_CNT));

//                (rtn < 0) ?
//                        procDCStatusUpdate(IOT_EVT_SYS_RECOV_FAIL, 1) :
//                        procDCStatusUpdate(IOT_EVT_SYS_RECOVERED, 1);

                if(rtn < 0)
                {
                    printf("alive fail\n");
                    procDCStatusUpdate(IOT_EVT_SYS_RECOV_FAIL, 1);
                }
                else
                {
                    printf("recovered\n");
                    procDCStatusUpdate(IOT_EVT_SYS_RECOVERED, 1);
                }
            }

            threadCnt = 0;
        }

        usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
        threadCnt++;
    }

    return NULL;
}

static void *dc_transfer_data_thread(void *arg)
{
    int rtn = 0; //test send to soh
    uint64_t pst_secs = 0;

    IoTMainMsg mainMsg;
    IoTStorMsg storMsg;
    IoTLedMsg ledMsg;

    RespBmsCanInfo *pStCanInfo = NULL;
    RxCanDataInfo *pRxCanData = NULL;

    RespDataInfo tfRespData;

    SenInfo *pStSenInfo = NULL;
    SenDataInfo *pSenData = NULL;

    struct timeval pst_time;
    struct timeval respbms_pack_time;

    bool bFirstChkTime = false;

    int ringBufIdx = 0;

    pStCanInfo = IoTCan_GetRespBmsCanInfo();
    pStSenInfo = &g_stSenInfo;

    while (false == pStCanInfo->bRunning)
    {
        /* Wait.. */
        usleep(100 * USEC_TO_MILISEC);
    }

    while (true == pStCanInfo->bRunning)
    {
        if (false == bFirstChkTime)
        {
            gettimeofday(&respbms_pack_time, NULL);

            bFirstChkTime = true;
        }

        gettimeofday(&pst_time, NULL);
        pst_secs = (uint64_t)pst_time.tv_sec;

        gettimeofday(&respbms_pack_time, NULL);

        /* Copy rx CAN data */
        pRxCanData = &pStCanInfo->rxCanData;
        memcpy(&tfRespData, &pRxCanData->respData, sizeof(RespDataInfo));
        tfTimeStamp = pRxCanData->dataDoneTime;

        if (pRxCanData->bDoneRxData == true)
        {
            printf("can is running\n");

            pRxCanData->bDoneRxData = false;
            pRxCanData->rxMsgCnt = 0;

            if (tfRespData.rResp1E.st_daq_resp_1E.sBmsStat == 7) //send for power off to main
            {
//                printf("%d ", killCnt);
                killCnt++;

                if(killCnt > 3)
                {
                    pStCanInfo->stopDaq = 0x0003;
                    printf("##POWER_OFF : BMS STATUS=POST RUN\n");
//                    LOG_INFO("##POWER_OFF : BMS STATUS=POST RUN");
                    procDCStatusUpdate(IOT_EVT_POWER_OFF, 0);
                }
            }

            /* CAN message check using LED process */
//            checkMsgLED(&tfRespData);

            pSenData = &pStSenInfo->pSenData;

            composeEachJsonObj(&g_stJsonObjs, &tfRespData, pSenData, tfTimeStamp);
//            composeBmsJsonObj(&g_stJsonObjs, &tfRespData, tfTimeStamp);
            //@@ check data
//            fprintf(stderr, "json format %s\n", json_object_to_json_string(g_stJsonObjs.resps.resp.obj));
//            fprintf(stderr, "json format %s\n", json_object_to_json_string(g_stJsonObjs.sens.obj));

            ringBufIdx = RbufPush(RBUF_TYPE_DATA_RBMS, g_stJsonObjs.resps.obj);

            if (ringBufIdx < 0)
            {
                g_s32DcFaultInjNum = 0;
//                LOG_ERR("Can rack data ring buffer push error!");
                procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 5);
            }
            else
            {
                /* Send message to STOR proc */
                storMsg.msgtype = IOT_STORMSG_WRTDATA_RBMS;
                storMsg.idx = ringBufIdx;
                msgsnd(g_s32StorMsgID, &storMsg,
                       sizeof(IoTStorMsg) - sizeof(long), IPC_NOWAIT);
#if false
                /* Send message to SOH proc */
                g_stSohMsg.msgtype = IOT_SOHMSG_CHKVALID;
                g_stSohMsg.idx = 0;
                g_stSohMsg.measTimeStamp = pst_secs;
                g_stSohMsg.meas[MEAS_TYPE_AVGCELLV] = tfRespData.rResp1F.st_daq_resp_1F.sAvgCellVol;
                g_stSohMsg.meas[MEAS_TYPE_PACKCURR] = tfRespData.rResp1E.st_daq_resp_1E.sPackCurr;

                rtn = msgsnd(g_s32SohMsgID, &g_stSohMsg, sizeof(IoTSohMsg) - sizeof(long), 0);

                if(rtn < 0)
                {
                printf("Failed send message to SOH proc message queue\n");
                }
#endif
            }
//            LOG_INFO("#Compose RBMS data Time: %d#", respbms_pack_time);
            usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
        }
        else
        {
            pRxCanData->bFirstFlg = false;
#if false
            printf("can is not running\n");
            pSenData = &pStSenInfo->pSenData;

            tfTimeStamp = getMSecTimeStamp();
            composeEachJsonObj(&g_stJsonObjs, &tfRespData, pSenData, tfTimeStamp);
    //            composeBmsJsonObj(&g_stJsonObjs, &tfRespData, tfTimeStamp);
            //@@ check data
            fprintf(stderr, "json format %s\n", json_object_to_json_string(g_stJsonObjs.resps.obj));
//            fprintf(stderr, "json format %s\n", json_object_to_json_string(g_stJsonObjs.sens.obj));

            ringBufIdx = RbufPush(RBUF_TYPE_DATA_RBMS, g_stJsonObjs.resps.obj);

            if (ringBufIdx < 0)
            {
                g_s32DcFaultInjNum = 0;
//                LOG_ERR("Can rack data ring buffer push error!");
                procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 5);
            }
            else
            {
                /* Send message to STOR proc */
                storMsg.msgtype = IOT_STORMSG_WRTDATA_RBMS;
                storMsg.idx = ringBufIdx;
                msgsnd(g_s32StorMsgID, &storMsg,
                       sizeof(IoTStorMsg) - sizeof(long), IPC_NOWAIT);
            }
            usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
#endif
        }
    }

    return NULL;
}

#if true
static void *get_gps_data_thread(void *arg)
{
    SenInfo *pStSenInfo = NULL;
    SenDataInfo *pSenData = NULL;

    pStSenInfo = &g_stSenInfo;
    initSensData(pStSenInfo);

    do
    {
        serial_read();

        pSenData = &pStSenInfo->pSenData;
        pSenData->rLatitude = gps[0];
        pSenData->rLongitude = gps[1];

    }
    while (1 == pStSenInfo->iRunning);


}
#endif

static void procFakedDcSnd(uint64_t meas_tm, int16_t cell_volt, int16_t flt_curr)
{
    int rtn = 0;

    pthread_mutex_lock(&g_mtxDcMainMsg);
    g_stSohMsg.msgtype = IOT_SOHMSG_CHKVALID;
    g_stSohMsg.idx = 0;
    g_stSohMsg.measTimeStamp = meas_tm;
    g_stSohMsg.meas[MEAS_TYPE_AVGCELLV] = cell_volt;
    g_stSohMsg.meas[MEAS_TYPE_PACKCURR] = flt_curr;

    rtn = msgsnd(g_s32SohMsgID, &g_stSohMsg, sizeof(IoTSohMsg) - sizeof(long), 0);

    pthread_mutex_unlock(&g_mtxDcMainMsg);

    if(rtn < 0)
    {
        printf("Failed send message to SOH proc message queue\n");
    }
}

static void *dc_proc_thread(void *arg)
{
    int rtn = 0;

    while (1)
    {
        /* waiting for receiving message from other proc */
        rtn = msgrcv(g_s32DcMsgID, &g_stDcMsg, sizeof(IoTDcMsg) - sizeof(long), IOT_DCMSG_ALL, IPC_NOWAIT);
        if (FAIL == rtn)
        {
            /* No error in case of interrupt from timer */
            if (EINTR == errno)
            {
                usleep(10 * USEC_TO_MILISEC);
                continue;
            }
        }
        else
        {
            switch (g_stDcMsg.msgtype)
            {
                case IOT_DCMSG_FAULTINJECT:
                    g_s32DcFaultInjNum = g_stDcMsg.faultNum;
//                    LOG_INFO("Received fault inject num: %d", g_s32DcFaultInjNum);
                    break;
                case IOT_DCMSG_TERMINATE :
                    IoTCan_Close();

                    g_stMainMsg.msgtype = IOT_MAINMSG_TERMRSP;
                    g_stMainMsg.proc = PROC_TYPE_DC;

                    if(FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0)) /* Fault inject num: 119 */
                    {
                        g_s32DcFaultInjNum = 0;
//                        LOG_WARN("Failed send terminate_resp message to MAIN proc message queue");
                        procDCStatusUpdate(IOT_EVT_SYS_IGNORE, 100);
                    }
                    LogClose();
                    return 0;
                default:
//                    LOG_INFO("Invalid message is received!");
                    break;
            }
        }

        usleep(10 * USEC_TO_MILISEC); /* sleep 10 ms */
    }
    return NULL;
}

int dc_main(void)
{
    int line_cnt = 0;
    int rtn = 0;
    struct timeval tv;
    uint64_t pst_secs = 0;
    char str_tmp[CSV_LINE_LEN];
    char *ptr = NULL;
    int idx = 0;
    float csv_row[CSV_LINE_COL]; /* time, cycle, ideal curr, filtered curr, integrated Q, cell voltage */
    int16_t flt_curr = 0, cell_volt = 0;

    config_t cfg;
    config_setting_t *setting = NULL;
    const char *strcfg = NULL;
    char bankID[IOT_CFG_STR_SZ] = { 0, };

    pthread_t aliveThreadID, transferThreadID, procThread;

//    LogSetInfo(LOG_FILE_PATH, "IoTDC"); /* Initialize logging */
//    LOG_INFO("Start IoT DC Proc");

    g_s32DcFaultInjNum = 0;
    getFaultInjectNum(); /* Get fault injection number */
    initMsgQueue();      /* Initialize message queue */
    pthread_mutex_init(&g_mtxDcMainMsg, NULL);

    /* Send result message to Main Proc */
    g_stMainMsg.msgtype = IOT_MAINMSG_PROCRSP;
    g_stMainMsg.proc = PROC_TYPE_DC;
    g_stMainMsg.data[0] = SUCCESS;

    if (FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg,
                      sizeof(IoTMainMsg) - sizeof(long), 0))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("Failed send proc resp message to main proc message queue");
        reboot_system();
    }

    /* Attach(Initialize) Ring Buffer */
    if (0 != RbufInit(1))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_ERR("Failed initialize ring buffer");
        procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 12);
    }

    /* Read configuration file */
    config_init(&cfg);
    if (CONFIG_FALSE == config_read_file(&cfg, IOT_CONFIG_FILE_PATH))
    {
        g_s32DcFaultInjNum = 0;
//        LOG_WARN("DC Proc - config file read error(%s:%d - %s)",
//                 config_error_file(&cfg), config_error_line(&cfg),
//                 config_error_text(&cfg));
        procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 13);
        config_destroy(&cfg);
    }
    else
    {
        setting = config_lookup(&cfg, "bank");
        if (NULL != setting)
        {
            if (0 != config_setting_lookup_string(setting, "id", &strcfg))
            {
                snprintf(bankID, IOT_CFG_STR_SZ, strcfg);
            }
            else
            {
                g_s32DcFaultInjNum = 0;
//                LOG_WARN("Can't find BANK ID");
                procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 14);

                snprintf(bankID, IOT_CFG_STR_SZ, IOT_CFG_BANKID);
            }
        }
        config_destroy(&cfg);
    }

    /* Initialize Json Object */

    initJsonObjs(&g_stJsonObjs, bankID);

    pthread_create(&aliveThreadID, 0, dc_alive_check_thread, NULL); /* Alive thread to report status */
    pthread_mutex_init(&g_mtxEvtCB, NULL);
#if false

    /* read cell measurement file */
        FILE *fp = fopen("/home/debian/eviot/LFP_BattSimAgingData_aa.csv", "r");

        gettimeofday(&tv, NULL);
        pst_secs = (uint64_t)tv.tv_sec;

        if(NULL != fp)
        {
            while(!feof(fp))
            {
                fgets(str_tmp, sizeof(str_tmp), fp);

                idx = 0;
                ptr = strtok(str_tmp, ",");
                while(NULL != ptr)
                {
                    csv_row[idx++] = atof(ptr);
                    ptr = strtok(NULL, ",");
                }

                flt_curr = (int16_t)(-1 * floor(10 * csv_row[CSV_COL_FCURR]));
                cell_volt = (int16_t)(floor(1000 * csv_row[CSV_COL_CVOLT]));

                /* Send message for cell measurements to SOH proc */
                procFakedDcSnd(pst_secs, cell_volt, flt_curr);

                pst_secs += TIME_GAP_STEP;

    #if 0
                line_cnt++;
                if(line_cnt >= 4000)
                {
                    break;
                }
    #endif
                usleep(20 * USEC_TO_MILISEC); /* 20ms */
            }
        }
        else
        {
            printf("Can't open /home/debian/eviot/LFP_BattSimAgingData_aa.csv file\n");
        }

        fclose(fp);
        printf("cell fake measurements sending finish!!\n");

#endif
#if true
         /* Invoke CAN & transfer thread */
//    IoTCan_SetCallBackFunc(dc_event_reportCB);

    if (0 == IoTCan_Open())
    {
        IoTCan_Start();

        if (pthread_create(&transferThreadID, NULL, dc_transfer_data_thread,NULL) < 0)
        {
            g_s32DcFaultInjNum = 0;
//            LOG_WARN("Failed to create transfer data thread: %d (%s)", errno,
//                     strerror(errno));
            procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 15);
            IoTCan_Close();
        }
        if (pthread_create(&transferThreadID, NULL, get_gps_data_thread,NULL) < 0)
        {
            g_s32DcFaultInjNum = 0;
//            LOG_WARN("Failed to create transfer data thread: %d (%s)", errno, strerror(errno));
            procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 15);
        }
    }
    else
    {
        g_s32DcFaultInjNum = 0;
//        LOG_ERR("Failed open CAN device");
        procDCStatusUpdate(IOT_EVT_INIT_UNRECOVER, 16);
    }

    procDCStatusUpdate(IOT_EVT_INIT_SUCCESS, 0);
#endif

    pthread_create(&procThread, NULL, dc_proc_thread,NULL);
}
