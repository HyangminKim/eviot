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
**   $FILNAME   : IoT_stor.c                                                            **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 22, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains STOR process implementation.                       **
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
#include <sys/msg.h>
#include <sys/reboot.h>
#include <sys/stat.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_rbuf.h"
#include "IoT_main.h"
#include "IoT_mq.h"
#include "IoT_stor.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define STOR_FILE_NUM_EXIST     2

#define STOR_FILE_SIZE_STAT     1024 /* 1KB */
#define STOR_FILE_SIZE_RBMS     (3 * 1024 * RBMS_MAX_RACK_CNT) /* 3KB size per one rack data */
#define STOR_FILE_SIZE_SBMS     1024 /* 1KB */
#define STOR_FILE_SIZE_LOC      1024 /* 1KB */

#define STOR_FILE_CNT_STAT      100 /* 1 file per 60 sec + event occurrence, 60 + 40 files per 1 day: 1(hr) * 60(min) * 1(files) */
#define STOR_FILE_CNT_RBMS      180 /* 1 file per 20 sec, 180 files per 1 hours: 1(hr) * 60(min) * 3(files) */
#define STOR_FILE_CNT_SBMS      60  /* 1 file per 60 sec, 1440 files per 1 hours: 1(hr) * 60(min) * 1(files) */
#define STOR_FILE_CNT_LOC       60  /* 1 file per 60 sec, 1440 files per 1 hours: 1(hr) * 60(min) * 1(files) */

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/
typedef enum _StorType
{
    STOR_TYPE_DATA_RBMS = 0x00,
    STOR_TYPE_DATA_SBMS = 0x01,
    STOR_TYPE_LOC       = 0x02,
    STOR_TYPE_MAX       = 0x03
} StorType;

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/
static const char *g_strStorPath[STOR_TYPE_MAX] =
{
    "./rbms",
    "./sbms",
    "./loc"
};

static const char *g_strStorPrefix[STOR_TYPE_MAX] =
{
    "rbms",
    "sbms",
    "loc"
};

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static int g_s32FaultInjNum;

static int g_s32MainMsgID, g_s32StorMsgID,g_s32MqMsgID,g_s32DcMsgID;
static IoTMainMsg g_stMainMsg;
static IoTStorMsg g_stStorMsg;
static IoTMqMsg g_stMqMsg;

static char g_s8StorRBmsFile[STOR_FILE_CNT_RBMS][IOT_FILE_PATH_MAXSZ];
static char g_s8StorSBmsFile[STOR_FILE_CNT_SBMS][IOT_FILE_PATH_MAXSZ];
static char g_s8StorLocFile[STOR_FILE_CNT_LOC][IOT_FILE_PATH_MAXSZ];

static pthread_mutex_t g_mtxStorMainMsg;
static bool g_bAliveThreadStopFlag;

/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/
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
        //LOG_INFO("IoTStor Proc Fault Injection Number: %d", g_s32FaultInjNum);
    }

    close(fdscr);
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
/**
 * @ Name : static void procStorStatusUpdate(int evt, int fail_idx)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : int evt      - Event value
 *                     int fail_idx - Fail index
 * @ Parameters[out] : -
 * @ Return Value : void
 *
 * @ brief :
 *    Send a status of stor process to main process
**/
static void procStorStatusUpdate(int evt, int fail_idx)
{
    int rtn = 0;

    pthread_mutex_lock(&g_mtxStorMainMsg);
    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
    g_stMainMsg.proc = PROC_TYPE_STOR;
    g_stMainMsg.data[DATA_TYPE_EVENT] = evt;
    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = fail_idx;
    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0);
    pthread_mutex_unlock(&g_mtxStorMainMsg);

    if(rtn < 0) /* Fault inject num: 402 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send message to MAIN proc message queue");
        reboot_system();
    }
}
/**
 * @ Name : static void initMsgQueue(void)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : -
 * @ Parameters[out] : -
 * @ Return Value : -
 *
 * @ brief :
 *    Initialize Process message queue
**/
static void initMsgQueue(void)
{
    /* 1. Initialize MAIN Proc message queue - fault num: 403 */
    if(FAIL == (g_s32MainMsgID = msgget((key_t)MSGQ_TYPE_MAIN, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create MAIN Proc message queue");
        reboot_system();
    }

    /* 2. Initialize STOR Proc message queue - fault num: 404 */
    if(FAIL == (g_s32StorMsgID = msgget((key_t)MSGQ_TYPE_STOR, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create STOR Proc message queue");
        reboot_system();
    }

    /* 3. Initialize DC Proc message queue - fault num: 406 */
    if(FAIL == (g_s32DcMsgID = msgget((key_t)MSGQ_TYPE_DC, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create DC Proc message queue");
        reboot_system();
    }


    /* 4. Initialize MQ Proc message queue - fault num: 406 */
    if(FAIL == (g_s32MqMsgID = msgget((key_t)MSGQ_TYPE_MQ, IPC_CREAT | 0666)))
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed create MQ Proc message queue");
        reboot_system();
    }
}
/**
 * @ Name : static void initStorFileStructure(void)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : -
 * @ Parameters[out] : -
 * @ Return Value : -
 *
 * @ brief :
 *    Initialize file structure
 *    1. Create a folder
 *    2. Create a files
**/
static void initStorFileStructure(void)
{
    int idx = 0, cnt = 0;
    int rtn = 0;
    int fdscr = -1;
    int nFileNum = 0;
    struct dirent **namelist;
    char existFileNm[IOT_FILE_PATH_MAXSZ >> 1] = {0, };


    /* RBMS Data Info - create directory and initial files */
    rtn = access(g_strStorPath[STOR_TYPE_DATA_RBMS], F_OK);
    if(rtn < 0)
    {
        mkdir(g_strStorPath[STOR_TYPE_DATA_RBMS], 0766);
        for(idx = 0; idx < STOR_FILE_CNT_RBMS; idx++)
        {
            snprintf(g_s8StorRBmsFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_DATA_RBMS], g_strStorPrefix[STOR_TYPE_DATA_RBMS], idx);

            fdscr = open(g_s8StorRBmsFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
            if(fdscr < 0) /* Fault inject num: 414 */
            {
                g_s32FaultInjNum = 0;
                //LOG_WARN("%s file open error", g_s8StorRBmsFile[idx]);
                procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 14);
                return;
            }

            /* Truncate a file to a specific length */
            ftruncate(fdscr, STOR_FILE_SIZE_RBMS);
            close(fdscr);
        }
    }
    else
    {
        nFileNum = scandir(g_strStorPath[STOR_TYPE_DATA_RBMS], &namelist, NULL, alphasort);
        if(nFileNum < 0) /* Fault inject num: 415 */
        {
            g_s32FaultInjNum = 0;
            //LOG_WARN("scan directory error: %s", g_strStorPath[STOR_TYPE_DATA_RBMS]);
            procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 15);
            return;
        }

        if(nFileNum > STOR_FILE_NUM_EXIST)
        {
            idx = 0;
            if((nFileNum - STOR_FILE_NUM_EXIST) < STOR_FILE_CNT_RBMS)
            {
                for(idx = 0; idx < (STOR_FILE_CNT_RBMS - (nFileNum - STOR_FILE_NUM_EXIST)); idx++)
                {
                    snprintf(g_s8StorRBmsFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_DATA_RBMS], g_strStorPrefix[STOR_TYPE_DATA_RBMS], idx);

                    fdscr = open(g_s8StorRBmsFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
                    if(fdscr < 0) /* Fault inject num: 416 */
                    {
                        g_s32FaultInjNum = 0;
                        //LOG_WARN("%s file open error", g_s8StorRBmsFile[idx]);
                        procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 16);
                        return;
                    }

                    /* Truncate a file to a specific length */
                    ftruncate(fdscr, STOR_FILE_SIZE_RBMS);
                    close(fdscr);
                }
            }

            for(cnt = 0; cnt < (nFileNum - STOR_FILE_NUM_EXIST); cnt++)
            {
                strncpy(existFileNm, namelist[cnt + 2]->d_name, IOT_FILE_PATH_MAXSZ >> 1);
                snprintf(g_s8StorRBmsFile[idx + cnt], IOT_FILE_PATH_MAXSZ, "%s/%s", g_strStorPath[STOR_TYPE_DATA_RBMS], existFileNm);

                fdscr = open(g_s8StorRBmsFile[idx + cnt], O_WRONLY | O_CREAT | O_SYNC, 0644);
                if(fdscr < 0) /* Fault inject num: 417 */
                {
                    g_s32FaultInjNum = 0;
                    //LOG_WARN("%s file open error", g_s8StorRBmsFile[idx + cnt]);
                    procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 17);
                    return;
                }

                /* Truncate a file to a specific length */
                ftruncate(fdscr, STOR_FILE_SIZE_RBMS);
                close(fdscr);
            }
        }

        while(nFileNum--) free(namelist[nFileNum]);
        free(namelist);
    }

    /* SBMS Data Info - create directory and initial files */
    rtn = access(g_strStorPath[STOR_TYPE_DATA_SBMS], F_OK);
    if(rtn < 0)
    {
        mkdir(g_strStorPath[STOR_TYPE_DATA_SBMS], 0766);
        for(idx = 0; idx < STOR_FILE_CNT_SBMS; idx++)
        {
            snprintf(g_s8StorSBmsFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_DATA_SBMS], g_strStorPrefix[STOR_TYPE_DATA_SBMS], idx);

            fdscr = open(g_s8StorSBmsFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
            if(fdscr < 0) /* Fault inject num: 418 */
            {
                g_s32FaultInjNum = 0;
                //LOG_WARN("%s file open error", g_s8StorSBmsFile[idx]);
                procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 18);
                return;
            }

            /* Truncate a file to a specific length */
            ftruncate(fdscr, STOR_FILE_SIZE_SBMS);
            close(fdscr);
        }
    }
    else
    {
        nFileNum = scandir(g_strStorPath[STOR_TYPE_DATA_SBMS], &namelist, NULL, alphasort);
        if(nFileNum < 0) /* Fault inject num: 419 */
        {
            g_s32FaultInjNum = 0;
            //LOG_WARN("scan directory error: %s", g_strStorPath[STOR_TYPE_DATA_SBMS]);
            procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 19);
            return;
        }

        if(nFileNum > STOR_FILE_NUM_EXIST)
        {
            idx = 0;
            if((nFileNum - STOR_FILE_NUM_EXIST) < STOR_FILE_CNT_SBMS)
            {
                for(idx = 0; idx < (STOR_FILE_CNT_SBMS - (nFileNum - STOR_FILE_NUM_EXIST)); idx++)
                {
                    snprintf(g_s8StorSBmsFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_DATA_SBMS], g_strStorPrefix[STOR_TYPE_DATA_SBMS], idx);

                    fdscr = open(g_s8StorSBmsFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
                    if(fdscr < 0) /* Fault inject num: 420 */
                    {
                        g_s32FaultInjNum = 0;
                        //LOG_WARN("%s file open error", g_s8StorSBmsFile[idx]);
                        procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 20);
                        return;
                    }

                    /* Truncate a file to a specific length */
                    ftruncate(fdscr, STOR_FILE_SIZE_SBMS);
                    close(fdscr);
                }
            }

            for(cnt = 0; cnt < (nFileNum - STOR_FILE_NUM_EXIST); cnt++)
            {
                strncpy(existFileNm, namelist[cnt + 2]->d_name, IOT_FILE_PATH_MAXSZ >> 1);
                snprintf(g_s8StorSBmsFile[idx + cnt], IOT_FILE_PATH_MAXSZ, "%s/%s", g_strStorPath[STOR_TYPE_DATA_SBMS], existFileNm);

                fdscr = open(g_s8StorSBmsFile[idx + cnt], O_WRONLY | O_CREAT | O_SYNC, 0644);
                if(fdscr < 0) /* Fault inject num: 421 */
                {
                    g_s32FaultInjNum = 0;
                    //LOG_WARN("%s file open error", g_s8StorSBmsFile[idx + cnt]);
                    procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 21);
                    return;
                }

                /* Truncate a file to a specific length */
                ftruncate(fdscr, STOR_FILE_SIZE_SBMS);
                close(fdscr);
            }
        }

        while(nFileNum--) free(namelist[nFileNum]);
        free(namelist);
    }

    /* Location Data Info - create directory and initial files */
    rtn = access(g_strStorPath[STOR_TYPE_LOC], F_OK);
    if(rtn < 0)
    {
        mkdir(g_strStorPath[STOR_TYPE_LOC], 0766);
        for(idx = 0; idx < STOR_FILE_CNT_LOC; idx++)
        {
            snprintf(g_s8StorLocFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_LOC], g_strStorPrefix[STOR_TYPE_LOC], idx);

            fdscr = open(g_s8StorLocFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
            if(fdscr < 0) /* Fault inject num: 418 */
            {
                g_s32FaultInjNum = 0;
                //LOG_WARN("%s file open error", g_s8StorLocFile[idx]);
                procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 22);
                return;
            }

            /* Truncate a file to a specific length */
            ftruncate(fdscr, STOR_FILE_SIZE_LOC);
            close(fdscr);
        }
    }
    else
    {
        nFileNum = scandir(g_strStorPath[STOR_TYPE_LOC], &namelist, NULL, alphasort);
        if(nFileNum < 0) /* Fault inject num: 419 */
        {
            g_s32FaultInjNum = 0;
            //LOG_WARN("scan directory error: %s", g_strStorPath[STOR_TYPE_LOC]);
            procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 23);
            return;
        }

        if(nFileNum > STOR_FILE_NUM_EXIST)
        {
            idx = 0;
            if((nFileNum - STOR_FILE_NUM_EXIST) < STOR_FILE_CNT_LOC)
            {
                for(idx = 0; idx < (STOR_FILE_CNT_LOC - (nFileNum - STOR_FILE_NUM_EXIST)); idx++)
                {
                    snprintf(g_s8StorLocFile[idx], IOT_FILE_PATH_MAXSZ, "%s/%s_%013d", g_strStorPath[STOR_TYPE_LOC], g_strStorPrefix[STOR_TYPE_LOC], idx);

                    fdscr = open(g_s8StorLocFile[idx], O_WRONLY | O_CREAT | O_SYNC, 0644);
                    if(fdscr < 0) /* Fault inject num: 420 */
                    {
                        g_s32FaultInjNum = 0;
                        //LOG_WARN("%s file open error", g_s8StorLocFile[idx]);
                        procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 24);
                        return;
                    }

                    /* Truncate a file to a specific length */
                    ftruncate(fdscr, STOR_FILE_SIZE_LOC);
                    close(fdscr);
                }
            }

            for(cnt = 0; cnt < (nFileNum - STOR_FILE_NUM_EXIST); cnt++)
            {
                strncpy(existFileNm, namelist[cnt + 2]->d_name, IOT_FILE_PATH_MAXSZ >> 1);
                snprintf(g_s8StorLocFile[idx + cnt], IOT_FILE_PATH_MAXSZ, "%s/%s", g_strStorPath[STOR_TYPE_LOC], existFileNm);

                fdscr = open(g_s8StorLocFile[idx + cnt], O_WRONLY | O_CREAT | O_SYNC, 0644);
                if(fdscr < 0) /* Fault inject num: 421 */
                {
                    g_s32FaultInjNum = 0;
                    //LOG_WARN("%s file open error", g_s8StorLocFile[idx + cnt]);
                    procStorStatusUpdate(IOT_EVT_INIT_UNRECOVER, 25);
                    return;
                }

                /* Truncate a file to a specific length */
                ftruncate(fdscr, STOR_FILE_SIZE_LOC);
                close(fdscr);
            }
        }

        while(nFileNum--) free(namelist[nFileNum]);
        free(namelist);
    }
}
/**
 * @ Name : static int writeStorFile(StorType type, uint8_t *data, int dataLen, char *path)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : StorType type - Type value of stor
 *                     uint8_t *data - Data to write a file
 *                     int dataLen   - Length of data
 *                     char *path    - Path of file
 * @ Parameters[out] : -
 * @ Return Value : int - SUCCESS or FAIL
 *
 * @ brief :
 *    Write a Json data to file
 *    1. Find a time stamp
 *    2. Open and write file
 *    3. Rename a file
**/
static int writeStorFile(StorType type, uint8_t *data, int dataLen, char *path)
{
    char *ptr = NULL;
    char ts[20] = {0, };

    int idx = 0, rtn = 0;
    int fdscr = -1;
    char updatePath[IOT_FILE_PATH_MAXSZ] = {0, };

    ptr = strstr((char *)data, "Created");
    ptr += 10;

    while(',' != *ptr)
    {
        ts[idx++] = *ptr++;
    }
    ts[idx] = 0;

    /* Open file */
    fdscr = open(path, O_WRONLY | O_CREAT | O_SYNC, 0644);
    if(fdscr < 0) /* Fault inject num: 407 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("%s file open error", path);
        procStorStatusUpdate(IOT_EVT_STOR_FAIL, 7);

        return FAIL;
    }

    /* Write file */
    rtn = write(fdscr, data, dataLen);
    if(rtn != dataLen) /* Fault inject num: 408 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("%s file write failed", path);
        procStorStatusUpdate(IOT_EVT_STOR_FAIL, 8);
        close(fdscr);

        return FAIL;
    }
    close(fdscr);

    /* Update Path Name */
    snprintf(updatePath, IOT_FILE_PATH_MAXSZ, "%s/%s_%s", g_strStorPath[type], g_strStorPrefix[type], ts);
    rtn = rename(path, updatePath);

    if(rtn < 0) /* Fault inject num: 409 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("%s->%s file rename failed", path, updatePath);
        procStorStatusUpdate(IOT_EVT_STOR_FAIL, 9);

        return FAIL;
    }

    strncpy(path, updatePath, IOT_FILE_PATH_MAXSZ);
    //LOG_INFO("Stored file: %s(%d)", path, dataLen);

    return SUCCESS;
}
/**
 * @ Name : static void *stor_alive_check_thread(void *arg)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : -
 * @ Parameters[out] : -
 * @ Return Value : -
 *
 * @ brief :
 *    Alive thread for Main process
 *    1. Send an alive message to Main process every 1 minutes.
 *    2. If there is error occurrence then retry it 3 times.
**/
static void *stor_alive_check_thread(void *arg)
{
    static int threadCnt = 0;
    int rtn = 0, retryCnt = 0;

    g_bAliveThreadStopFlag = false;

    while(false == g_bAliveThreadStopFlag)
    {
        if(IOT_ALIVE_TIME == threadCnt) /* 1 minute */
        {
            /* Tx alive message to MAIN proc */
            pthread_mutex_lock(&g_mtxStorMainMsg);
            g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
            g_stMainMsg.proc = PROC_TYPE_STOR;
            g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
            g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
            rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0);
            pthread_mutex_unlock(&g_mtxStorMainMsg);

            if(rtn < 0) /* Fault inject num: 401 */
            {
                g_s32FaultInjNum = 0;
                //LOG_WARN("Failed send message to MAIN proc message queue");
                procStorStatusUpdate(IOT_EVT_SYS_FAIL, 1);
                retryCnt = 0;

                do
                {
                    pthread_mutex_lock(&g_mtxStorMainMsg);
                    g_stMainMsg.msgtype = IOT_MAINMSG_STATUS;
                    g_stMainMsg.proc = PROC_TYPE_STOR;
                    g_stMainMsg.data[DATA_TYPE_EVENT] = 0;
                    g_stMainMsg.data[DATA_TYPE_FAULT_IDX] = 0;
                    rtn = msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0);
                    pthread_mutex_unlock(&g_mtxStorMainMsg);

                    retryCnt++;
                    //LOG_INFO("Retry to tx MAIN proc message queue (return:%d, retry count:%d)", rtn, retryCnt);
                } while((rtn < 0) && (retryCnt < IOT_RETRY_CNT));

                (rtn < 0) ? procStorStatusUpdate(IOT_EVT_SYS_RECOV_FAIL, 1) : procStorStatusUpdate(IOT_EVT_SYS_RECOVERED, 1);
            }

            threadCnt = 0;
        }

        usleep(1000 * USEC_TO_MILISEC); /* sleep 1 sec */
        threadCnt++;
    }

    return NULL;
}


static void *stor_proc_thread(void *arg)
{
    struct msqid_ds msgStat;
    int rtn = 0, bufLen = 0;
    bool bTerminateFlag = false;

    uint8_t *storData = NULL;
    int storFileNum[STOR_TYPE_MAX] = {0, };

    while(1)
    {
        rtn = msgrcv(g_s32StorMsgID, &g_stStorMsg, sizeof(IoTStorMsg)-sizeof(long), IOT_STORMSG_ALL, IPC_NOWAIT);
               if(FAIL == rtn)
               {
                   /* No error in case of interrupt from timer */
                   if(EINTR == errno)
                   {
                       usleep(100 * USEC_TO_MILISEC);
                       continue;
                   }
               }
               else
               {
                   bufLen = -1;
                   switch(g_stStorMsg.msgtype)
                   {
                       case IOT_STORMSG_WRTDATA_RBMS :
                           //LOG_INFO("############STOR RBMS rbuf pop : %d", g_stStorMsg.idx);
                           bufLen = RbufPop(RBUF_TYPE_DATA_RBMS, g_stStorMsg.idx, (char **)&storData);
                           if(bufLen > 0)
                           {
                               rtn = writeStorFile(STOR_TYPE_DATA_RBMS, storData, bufLen, g_s8StorRBmsFile[storFileNum[STOR_TYPE_DATA_RBMS]]);
                               if(rtn < 0) /* Fault inject num: 428 */
                               {
                                   g_s32FaultInjNum = 0;
                                   //LOG_WARN("RBMS Data write STOR file failed");
                                   procStorStatusUpdate(IOT_EVT_STOR_FAIL, 28);
                               }
                               else
                               {
                                   g_stMqMsg.msgtype = IOT_MQMSG_SENDDATA;
                                   g_stMqMsg.len = bufLen;
                                   strncpy(g_stMqMsg.data.path, g_s8StorRBmsFile[storFileNum[STOR_TYPE_DATA_RBMS]], IOT_FILE_PATH_MAXSZ);
                                   msgsnd(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IPC_NOWAIT);

                                   storFileNum[STOR_TYPE_DATA_RBMS]++;
                                   if(STOR_FILE_CNT_RBMS == storFileNum[STOR_TYPE_DATA_RBMS])
                                   {
                                       storFileNum[STOR_TYPE_DATA_RBMS] = 0;
                                   }
                               }
                           }
                           break;

                       case IOT_STORMSG_WRTDATA_SBMS :
                           //LOG_INFO("############STOR SBMS rbuf pop : %d", g_stStorMsg.idx);
                           bufLen = RbufPop(RBUF_TYPE_DATA_SBMS, g_stStorMsg.idx, (char **)&storData);
                           if(bufLen > 0)
                           {
                               rtn = writeStorFile(STOR_TYPE_DATA_SBMS, storData, bufLen, g_s8StorSBmsFile[storFileNum[STOR_TYPE_DATA_SBMS]]);
                               if(rtn < 0) /* Fault inject num: 429 */
                               {
                                   g_s32FaultInjNum = 0;
                                   //LOG_WARN("SBMS Data write STOR file failed");
                                   procStorStatusUpdate(IOT_EVT_STOR_FAIL, 29);
                               }
                               else
                               {

                                   g_stMqMsg.msgtype = IOT_MQMSG_SENDDATA;
                                   g_stMqMsg.len = bufLen;
                                   strncpy(g_stMqMsg.data.path, g_s8StorSBmsFile[storFileNum[STOR_TYPE_DATA_SBMS]], IOT_FILE_PATH_MAXSZ);
                                   msgsnd(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IPC_NOWAIT);

                                   storFileNum[STOR_TYPE_DATA_SBMS]++;
                                   if(STOR_FILE_CNT_SBMS == storFileNum[STOR_TYPE_DATA_SBMS])
                                   {
                                       storFileNum[STOR_TYPE_DATA_SBMS] = 0;
                                   }
                               }
                           }
                           break;

                       case IOT_STORMSG_WRTDATA_LOC :
                           bufLen = RbufPop(RBUF_TYPE_DATA_LOC, g_stStorMsg.idx, (char **)&storData);
       //                    //LOG_INFO("############STOR LOC rbuf pop : %d, length: %d, ptr: 0x%x", g_stStorMsg.idx, bufLen, storData);
                           if(bufLen > 0)
                           {
                               rtn = writeStorFile(STOR_TYPE_LOC, storData, bufLen, g_s8StorLocFile[storFileNum[STOR_TYPE_LOC]]);
                               if(rtn < 0) /* Fault inject num: 427 */
                               {
                                   g_s32FaultInjNum = 0;
                                   //LOG_WARN("LOC write STOR file failed");
                                   procStorStatusUpdate(IOT_EVT_STOR_FAIL, 30);
                               }
                               else
                               {

                                   g_stMqMsg.msgtype = IOT_MQMSG_SENDLOC;
                                   g_stMqMsg.len = bufLen;
                                   strncpy(g_stMqMsg.data.path, g_s8StorLocFile[storFileNum[STOR_TYPE_LOC]], IOT_FILE_PATH_MAXSZ);
                                   msgsnd(g_s32MqMsgID, &g_stMqMsg, sizeof(IoTMqMsg) - sizeof(long), IPC_NOWAIT);


                                   storFileNum[STOR_TYPE_LOC]++;
                                   if(STOR_FILE_CNT_LOC == storFileNum[STOR_TYPE_LOC])
                                   {
                                       storFileNum[STOR_TYPE_LOC] = 0;
                                   }
                               }
                           }
                           break;


                       case IOT_STORMSG_TERMINATE :
                           bTerminateFlag = true;
                           break;

                       case IOT_STORMSG_FAULTINJECT :
                           g_s32FaultInjNum = g_stStorMsg.idx;
                           break;
                   }
               }

               if(true == bTerminateFlag)
               {
                   if(FAIL == msgctl(g_s32StorMsgID, IPC_STAT, &msgStat)) /* Fault inject num: 432 */
                   {
                       g_s32FaultInjNum = 0;
                       //LOG_WARN("Failed msgctl IPC_STAT STOR proc message queue");
                       procStorStatusUpdate(IOT_EVT_SYS_IGNORE, 32);
                   }

                   //LOG_INFO("############STOR message queue left number : %d", msgStat.msg_qnum);

                   if(msgStat.msg_qnum == 0)
                   {
                       //LogClose();
                       g_stMainMsg.msgtype = IOT_MAINMSG_TERMRSP;
                       g_stMainMsg.proc = PROC_TYPE_STOR;
                       if(FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0)) /* Fault inject num: 433 */
                       {
                           g_s32FaultInjNum = 0;
                           //LOG_WARN("Failed send message to MAIN proc message queue");
                           procStorStatusUpdate(IOT_EVT_SYS_IGNORE, 33);
                       }

                       return SUCCESS;
                   }
               }

               usleep(10 * USEC_TO_MILISEC);
    }
}
/**
 * @ Name : int main(void)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : -
 * @ Parameters[out] : -
 * @ Return Value : int - Success or not
 *
 * @ brief :
 *    Main function of Stor Process
 *    1. Initialize a logging
 *    2. Initialize a message queue and ring buffer
 *    3. Create an alive thread for Main process
 *    4. If stor process received a message from DC process then write a file using ring buffer
 *    5. Send a message to MQ process for transmitting Json file
**/
int stor_main(void)
{
    struct msqid_ds msgStat;
    pthread_t aliveThreadID, procThreadID;

    int rtn = 0, bufLen = 0;
    bool bTerminateFlag = false;

    uint8_t *storData = NULL;
    int storFileNum[STOR_TYPE_MAX] = {0, };

//    LogSetInfo(LOG_FILE_PATH, "IoTStor"); /* Initialize logging */
    //LOG_INFO("Start IoT STOR Proc");

    getFaultInjectNum(); /* Get fault injection number */
    initMsgQueue(); /* Initialize message queue */

    initStorFileStructure();
    //LOG_INFO("Initialize File Structure to save at STOR proc");

    RbufInit(0); /* Initialize ring buffer */

    /* Send result message to Main Proc */
    g_stMainMsg.msgtype = IOT_MAINMSG_PROCRSP;
    g_stMainMsg.proc = PROC_TYPE_STOR;
    g_stMainMsg.data[0] = SUCCESS;

    if(FAIL == msgsnd(g_s32MainMsgID, &g_stMainMsg, sizeof(IoTMainMsg) - sizeof(long), 0)) /* Fault inject num: 426 */
    {
        g_s32FaultInjNum = 0;
        //LOG_WARN("Failed send proc resp message to main proc message queue");
        reboot_system();
    }

    pthread_mutex_init(&g_mtxStorMainMsg, NULL);
    pthread_create(&aliveThreadID, 0, stor_alive_check_thread, NULL); /* Alive thread to report status */

    procStorStatusUpdate(IOT_EVT_INIT_SUCCESS, 0);
    pthread_create(&procThreadID, 0, stor_proc_thread, NULL);
}
