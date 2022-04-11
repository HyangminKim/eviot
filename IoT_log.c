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
**   $FILNAME   : IoT_log.c                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 10, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains LOG implementation.                                **
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
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/time.h>
#include <sys/stat.h>

#include "IoT_log.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/
#define LOG_PATH_INFO_SZ    64
#define LOG_SRC_INFO_SZ     64

#define LOG_FILE_NAME_SZ    1024
#define LOG_FILE_SAVE_TIME  (2 * 60 * 60 * 1000) /* (2 * 60 minute) */

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static FILE *g_pfLogFile = NULL;
static char g_s8LogFilePrefix[LOG_PATH_INFO_SZ] = {0, };
static char g_s8LogFolder[LOG_PATH_INFO_SZ] = ".";

static int g_s32LogLevel = LOG_LVL_INFO;
static int g_s32LogPrint = 0;

/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/
static int LogCreateFile(struct tm *tm1)
{
    char filenm[LOG_FILE_NAME_SZ] = {0, };

    if(0x00 == g_s8LogFolder[0])
    {
        strcpy(g_s8LogFolder, ".");
    }

    if(0x00 == g_s8LogFilePrefix[0])
    {
        strcpy(g_s8LogFilePrefix, "logfile");
    }

    snprintf(filenm, sizeof(filenm), "%s/%s-%04d%02d%02d-%02d%02d.log", g_s8LogFolder, g_s8LogFilePrefix,
             1900 + tm1->tm_year, tm1->tm_mon + 1, tm1->tm_mday, tm1->tm_hour , tm1->tm_min);

    if(NULL != g_pfLogFile)
    {
        fclose(g_pfLogFile);
        g_pfLogFile = NULL;
    }

    if(NULL == (g_pfLogFile = fopen(filenm, "a")))
    {
        return -1;
    }

    setvbuf(g_pfLogFile, NULL, _IOLBF, 0);

    //delete log files older than 7 days
    char syscmd[128] = {0, };
    snprintf(syscmd, 128, "find %s -name '*.log' -ctime +7 -delete",g_s8LogFolder );
    system((const char *)syscmd);


    return 0;
}

static int LogGetPrint(void)
{
    static bool bFirstExecute = false;
    char *log_env = NULL;

    if(false == bFirstExecute)
    {
        if(NULL == (log_env = getenv("DAD_LOG_PRINT")))
        {
            g_s32LogPrint = 0;
        }
        else
        {
            g_s32LogPrint = (0 == strcmp(log_env, "Y")) ? 1 : 0;
        }

        bFirstExecute = true;
    }

    return g_s32LogPrint;
}

int LogSetLevel(int log_lvl)
{
    int level = LogGetLevel();

    g_s32LogLevel = log_lvl;

    return level;
}

int LogGetLevel(void)
{
    static bool bChkEnv = false;
    char *log_env = NULL;

    if(false == bChkEnv)
    {
        log_env = getenv("DAD_LOG_LEVEL");

        if(NULL == log_env) g_s32LogLevel = LOG_LVL_INFO;
        else if(0 == strcmp(log_env, "DEBUG")) g_s32LogLevel = LOG_LVL_DEBUG;
        else if(0 == strcmp(log_env, "INFO"))  g_s32LogLevel = LOG_LVL_INFO;
        else if(0 == strcmp(log_env, "WARN"))  g_s32LogLevel = LOG_LVL_WARN;
        else if(0 == strcmp(log_env, "ERROR")) g_s32LogLevel = LOG_LVL_ERR;
        else  g_s32LogLevel = LOG_LVL_INFO;

        bChkEnv = true;
    }

    return g_s32LogLevel;
}

int LogSetInfo(const char *dir, const char *prefix)
{
    if((NULL == dir) || (0x00 == dir[0]))
    {
        fprintf(stderr, "Log folder set error\n");
        return -1;
    }

    if((NULL == prefix) || (0x00 == prefix[0]))
    {
        fprintf(stderr, "Log file prefix set error\n");
        return -1;
    }

    if((0 == strcmp(dir, g_s8LogFolder)) && (0 == strcmp(prefix, g_s8LogFilePrefix)))
    {
        return 0;
    }

    if(access(dir, F_OK) < 0)
    {
        mkdir(dir, 0766);
    }

    /* Copy log file prefix name */
    snprintf(g_s8LogFilePrefix, sizeof(g_s8LogFilePrefix), prefix);

    /* Copy log file folder name */
    snprintf(g_s8LogFolder, sizeof(g_s8LogFolder), dir);

    /* File close */
    if(NULL != g_pfLogFile)
    {
        fclose(g_pfLogFile);
        g_pfLogFile = NULL;
    }

    return 0;
}

int LogLogging(char log_type, const char *func, int line_no, const char *fmt, ...)
{
    static pid_t pid = -1;
    static int day = -1;
    static bool bFirstPerformed = false;

    static struct timeval old_tv;
    static unsigned long prevTime = 0;

    va_list ap;
    int sz = 0;
    struct timeval tv;
    struct tm *tm1;
    char src_info[LOG_SRC_INFO_SZ];
    unsigned long processTime = 0;

    gettimeofday(&tv, NULL);
    tm1 = localtime(&tv.tv_sec);

    va_start(ap, fmt);

    if(-1 == pid) pid = getpid();

    if(false == bFirstPerformed)
    {
        memcpy(&old_tv, &tv, sizeof(struct timeval));
        prevTime = 0;
        bFirstPerformed = true;
    }

    processTime = ((tv.tv_sec - old_tv.tv_sec) * 1000) + (int)((tv.tv_usec - old_tv.tv_usec) / 1000);

    /* First time or day is changed */
    if((day != tm1->tm_mday) || ((processTime - prevTime) > LOG_FILE_SAVE_TIME))
    {
        if(0 != LogCreateFile(tm1))
        {
            return -1;
        }

        if(day != tm1->tm_mday)
        {
            day = tm1->tm_mday;
            memcpy(&old_tv, &tv, sizeof(struct timeval));
            prevTime = 0;
        }
        else
        {
            prevTime = processTime;
        }

    }

    if(LogGetPrint())
    {
        sz += fprintf(stderr, "[%04d/%02d/%02d %02d:%02d:%02d.%03d] %s: ",
                      1900 + tm1->tm_year, tm1->tm_mon + 1, tm1->tm_mday, tm1->tm_hour, tm1->tm_min, tm1->tm_sec, (int)tv.tv_usec/1000, func);
        sz += vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
    }
    else
    {
        sz += fprintf(g_pfLogFile, "[%04d/%02d/%02d %02d:%02d:%02d.%03d]",
                      1900 + tm1->tm_year, tm1->tm_mon + 1, tm1->tm_mday, tm1->tm_hour, tm1->tm_min, tm1->tm_sec, (int)tv.tv_usec/1000);

        snprintf(src_info, sizeof(src_info), "%s(%d)", func, line_no);

        sz += fprintf(g_pfLogFile, " %s: ", src_info);
        sz += vfprintf(g_pfLogFile, fmt, ap);
        sz += fprintf(g_pfLogFile, "\n");
    }

    va_end(ap);

    return sz;
}

void LogClose(void)
{
    if(NULL != g_pfLogFile)
    {
        fclose(g_pfLogFile);
        g_pfLogFile = NULL;
    }
}
