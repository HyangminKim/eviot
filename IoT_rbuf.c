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
**   $FILNAME   : IoT_rbuf.c                                                            **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 16, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
**   DESCRIPTION: This file contains Ring Buffer implementation.                        **
**                                                                                      **
**                                                                                      **
******************************************************************************************/


/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>

#include <json-c/json_object.h>

#include "IoT.h"
#include "IoT_log.h"
#include "IoT_rbuf.h"

/*****************************************************************************************
**                      Definitions                                                     **
******************************************************************************************/

/*****************************************************************************************
**                      Type Definitions                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Constants                                                       **
******************************************************************************************/

/*****************************************************************************************
**                      Variables                                                       **
******************************************************************************************/
static RBmsDataRBuf *g_pstRDataBuf = NULL;
static SBmsDataRBuf *g_pstSDataBuf = NULL;
static LocDataRBuf *g_pstLDataBuf = NULL;

static int g_s32RDataBufIdx, g_s32SDataBufIdx,  g_s32LDataBufIdx;

/*****************************************************************************************
**                      Function Prototypes                                             **
******************************************************************************************/

/*****************************************************************************************
**                      Functions                                                       **
******************************************************************************************/
/**
 * @ Name : int RbufInit(uint8_t reset)
 * @ Author : SK
 * @ Date : 2021.05.03
 *
 * @ Parameters[in]  : uint8_t reset - initialize a shared memory to 0x00
 * @ Parameters[out] : -
 * @ Return Value : int - Fail or not
 *
 * @ brief :
 *    Init a shared memory
 *    1. Initialize(Create) a shared memory
 *    2. Attach a shared memory
**/
int RbufInit(uint8_t reset)
{
    int idx = 0, shmID = 0;

    g_s32RDataBufIdx = -1;
    g_s32SDataBufIdx = -1;
    g_s32LDataBufIdx = -1;

    /* RBMS Data RBUF */
    if(FAIL == (shmID = shmget((key_t)SHM_KEY_RBMS_DATA, sizeof(RBmsDataRBuf) * MAX_RBUF_NUM, IPC_CREAT|0666)))
    {
        LOG_WARN("Failed create IOT RBMS Data shared memory");
        return FAIL;
    }

    if((void *)-1 == (g_pstRDataBuf = (RBmsDataRBuf *)shmat(shmID, (void *)0, 0)))
    {
        LOG_WARN("Failed attach IOT RBMS Data shared memory");
        return FAIL;
    }

    /* SBMS Data RBUF */
    if(FAIL == (shmID = shmget((key_t)SHM_KEY_SBMS_DATA, sizeof(SBmsDataRBuf) * MAX_RBUF_NUM, IPC_CREAT|0666)))
    {
        LOG_WARN("Failed create IOT SBMS Data shared memory");
        return FAIL;
    }

    if((void *)-1 == (g_pstSDataBuf = (SBmsDataRBuf *)shmat(shmID, (void *)0, 0)))
    {
        LOG_WARN("Failed attach IOT SBMS Data shared memory");
        return FAIL;
    }

    /* Location Data RBUF */
    if(FAIL == (shmID = shmget((key_t)SHM_KEY_LOC_DATA, sizeof(LocDataRBuf) * MAX_RBUF_NUM, IPC_CREAT|0666)))
    {
        LOG_WARN("Failed create IOT LOC Data shared memory");
        return FAIL;
    }

    if((void *)-1 == (g_pstLDataBuf = (LocDataRBuf *)shmat(shmID, (void *)0, 0)))
    {
        LOG_WARN("Failed attach IOT LOC Data shared memory");
        return FAIL;
    }

    if(reset > 0)
    {
        for(idx = 0; idx < MAX_RBUF_NUM; idx++)
        {
            memset(&g_pstRDataBuf[idx], 0x00, sizeof(RBmsDataRBuf));
            memset(&g_pstSDataBuf[idx], 0x00, sizeof(SBmsDataRBuf));
            memset(&g_pstLDataBuf[idx], 0x00, sizeof(LocDataRBuf));
        }
    }

    return 0;
}
/**
 * @ Name : int RbufPush(RBufType type, struct json_object *jso)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : RBufType type           - Buffer type value
 *                     struct json_object *jso - Composed Json object
 * @ Parameters[out] : -
 * @ Return Value : int - Index of data buffer or Fail
 *
 * @ brief :
 *    Push a data to ring buffer
 *    1. Increase a buffer index
 *    2. Get a length of Json data
 *    3. Copy to Json data to ring buffer
**/
int RbufPush(RBufType type, struct json_object *jso)
{
    unsigned int bufLen = 0;

    RBmsDataRBuf *rdBuf = g_pstRDataBuf;
    SBmsDataRBuf *sdBuf = g_pstSDataBuf;
    LocDataRBuf *lBuf = g_pstLDataBuf;

    if((type >= RBUF_TYPE_MAX) || (NULL == jso))
    {
        return FAIL;
    }

    switch(type)
    {
        case RBUF_TYPE_DATA_RBMS :
            if(NULL == rdBuf)
            {
                LOG_WARN("RBMS Data Ring buffer push error - not initialized");
                return FAIL;
            }

            g_s32RDataBufIdx++;
            if(MAX_RBUF_NUM == g_s32RDataBufIdx) g_s32RDataBufIdx = 0;

            rdBuf[g_s32RDataBufIdx].len = strlen(json_object_to_json_string(jso));
            bufLen = rdBuf[g_s32RDataBufIdx].len;
            memcpy(rdBuf[g_s32RDataBufIdx].value, json_object_to_json_string_length(jso, JSON_C_TO_STRING_SPACED, &(bufLen)), bufLen);
            return g_s32RDataBufIdx;

        case RBUF_TYPE_DATA_SBMS :
            if(NULL == sdBuf)
            {
                LOG_WARN("SBMS Data Ring buffer push error - not initialized");
                return FAIL;
            }

            g_s32SDataBufIdx++;
            if(MAX_RBUF_NUM == g_s32SDataBufIdx) g_s32SDataBufIdx = 0;

            sdBuf[g_s32SDataBufIdx].len = strlen(json_object_to_json_string(jso));
            bufLen = sdBuf[g_s32SDataBufIdx].len;
            memcpy(sdBuf[g_s32SDataBufIdx].value, json_object_to_json_string_length(jso, JSON_C_TO_STRING_SPACED, &(bufLen)), bufLen);
            return g_s32SDataBufIdx;

        case RBUF_TYPE_DATA_LOC:
            if (NULL == lBuf)
            {
                LOG_WARN("LOC Data Ring buffer push error - not initialized");
                return FAIL;
            }

            g_s32LDataBufIdx++;
            if (MAX_RBUF_NUM == g_s32LDataBufIdx)
                g_s32LDataBufIdx = 0;

            lBuf[g_s32LDataBufIdx].len = strlen(json_object_to_json_string(jso));
            bufLen = lBuf[g_s32LDataBufIdx].len;
            memcpy(lBuf[g_s32LDataBufIdx].value, json_object_to_json_string_length(jso, JSON_C_TO_STRING_SPACED, &(bufLen)), bufLen);
            return g_s32LDataBufIdx;

        default :
            printf("push msg type default\n");
            break;
    }

    return 0;
}
/**
 * @ Name : int RbufPop(RBufType type, int idx, char **val)
 * @ Author : SK
 * @ Date : 2021.05.02
 *
 * @ Parameters[in]  : RBufType type - Buffer type value
 *                     int idx       - Index of buffer
 * @ Parameters[out] : char **val
 * @ Return Value : int - length of data in buffer or Fail
 *
 * @ brief :
 *    Pop a data from ring buffer
 *    1. Copy a data of ring buffer to out parameter
 *    2. Get a length of data in ring buffer
**/
int RbufPop(RBufType type, int idx, char **val)
{
    RBmsDataRBuf *rdBuf = g_pstRDataBuf;
    SBmsDataRBuf *sdBuf = g_pstSDataBuf;
    LocDataRBuf *lBuf = g_pstLDataBuf;


    if((type >= RBUF_TYPE_MAX) || (idx < 0) || (idx >= MAX_RBUF_NUM))
    {
        return FAIL;
    }

    switch(type)
    {
        case RBUF_TYPE_DATA_RBMS :
            *val = rdBuf[idx].value;
            //LOG_INFO("RbufPop Status : idx=%d, buflen=%d, bufptr=0x%x", idx, rdBuf[idx].len, rdBuf[idx].value);
            return rdBuf[idx].len;

        case RBUF_TYPE_DATA_SBMS :
            *val = sdBuf[idx].value;
            //LOG_INFO("RbufPop Status : idx=%d, buflen=%d, bufptr=0x%x", idx, sdBuf[idx].len, sdBuf[idx].value);
            return sdBuf[idx].len;

        case RBUF_TYPE_DATA_LOC :
           *val = lBuf[idx].value;
           //LOG_INFO("RbufPop Status : idx=%d, buflen=%d, bufptr=0x%x", idx, lBuf[idx].len, lBuf[idx].value);
           return lBuf[idx].len;


        default :
            break;
    }

    return 0;
}
