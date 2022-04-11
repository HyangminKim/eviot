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
**   $FILNAME   : IoT_rbuf.h                                                            **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 16, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_RBUF_H_
#define IOT_RBUF_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/
#include <json-c/json.h>

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define MAX_RBUF_NUM            10  /* Rack BMS - 10ea */
#define MAX_RBUF_RBMS_DATA_SZ   ((3 * 1024 * MAX_RBUF_NUM) + 512) /* CAN message - 10, one pack - 3KB */
#define MAX_RBUF_SBMS_DATA_SZ   256
#define MAX_RBUF_LOC_DATA_SZ    1024

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/
typedef enum _RBufType
{
    RBUF_TYPE_DATA_RBMS = 0x00,
    RBUF_TYPE_DATA_SBMS = 0x01,
    RBUF_TYPE_DATA_LOC  = 0x02,
    RBUF_TYPE_MAX       = 0x03
} RBufType;

typedef struct _RBmsDataRBuf
{
    char value[MAX_RBUF_RBMS_DATA_SZ];
    int len;
} RBmsDataRBuf;

typedef struct _SBmsDataRBuf
{
    char value[MAX_RBUF_SBMS_DATA_SZ];
    int len;
} SBmsDataRBuf;

typedef struct _LocDataRBuf
{
    char value[MAX_RBUF_LOC_DATA_SZ];
    int len;
} LocDataRBuf;

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/
int RbufInit(uint8_t reset);
int RbufPush(RBufType type, struct json_object *jso);
int RbufPop(RBufType type, int idx, char **val);

#endif /* IOT_RBUF_H_ */
