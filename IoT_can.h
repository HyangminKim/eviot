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
**   $FILNAME   : IoT_can.h                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 17, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_CAN_H_
#define IOT_CAN_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/


/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/

/* Definitions for a Rack BMS H/W Configuration */
#define RBMS_AFIC_CELL_NO       12
#define RBMS_AFIC_TEMP_NO       3
#define RBMS_MODULE_AFIC_NO     4
#define RBMS_RACK_MODULE_NO     7

#define RBMS_RACK_TEMP_NO       (RBMS_AFIC_TEMP_NO * RBMS_MODULE_AFIC_NO * RBMS_RACK_MODULE_NO) /* Temp. count per Rack: 84 */
#define RBMS_RACK_CELL_NO       (RBMS_AFIC_CELL_NO * RBMS_MODULE_AFIC_NO * RBMS_RACK_MODULE_NO) /* Cell count per Rack: 336 */

#define RBMS_RXCAN_DATA_FIFOCNT 20
#define RBMS_JSON_DATA_FIFOCNT  20

/* Definitions for CAN configuration */
#define CAN_DEV_NAME_SZ         64
#define CAN_SYS_CMD_SZ          128

//#define CAN_MAX_MSGID_CNT       10  /* Periodic Msg: 1 + Sync Msg: 4 + On Demand Msg: 5 */
#define CAN_MAX_MSGNO_CNT       145 /* Periodic Msg: 1 + Sync Msg: 4 + On Demand Msg: 5 * 28 = 140 */
#define CAN_DATA_ALIGN_SZ       8   /* 8 bytes align */

#define CAN_CHK_BUS_OFF_TIME    500 /* CAN link down if CAN bus off state is over 500ms */
#define CAN_CHK_OVERFLOW_TIME   500 /* CAN link down if CAN overflow state is over 500ms */
#define CAN_CHK_NEED_RACKERR    (-100)

#define CAN_RCVMSG_OVER_TIME    5000 /* Re-initialize FIFO buffer if CAN message is not received for 5 seconds */

//new

#define CAN_MAX_MSGID_CNT 27//26//34
#define CAN_MAX_ID_CNT 27//26//34
/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/



//Cell Info
typedef union _ST_DAQ_RESP_00
{
    struct
    {
        unsigned long long sCell01 : 16;
        unsigned long long sCell02 : 16;
        unsigned long long sCell03 : 16;
        unsigned long long sCell04 : 16;

    }st_daq_resp_00;

    unsigned long long resp00;
} ST_DAQ_RESP_00;

typedef union _ST_DAQ_RESP_01
{
    struct
    {
        unsigned long long sCell05 : 16;
        unsigned long long sCell06 : 16;
        unsigned long long sCell07 : 16;
        unsigned long long sCell08 : 16;

    }st_daq_resp_01;

    unsigned long long resp01;
} ST_DAQ_RESP_01;

typedef union _ST_DAQ_RESP_02
{
    struct
    {
        unsigned long long sCell09 : 16;
        unsigned long long sCell10 : 16;
        unsigned long long sCell11 : 16;
        unsigned long long sCell12 : 16;

    }st_daq_resp_02;

    unsigned long long resp02;
} ST_DAQ_RESP_02;

typedef union _ST_DAQ_RESP_03
{
    struct
    {
        unsigned long long sCell13 : 16;
        unsigned long long sCell14 : 16;
        unsigned long long sCell15 : 16;
        unsigned long long sCell16 : 16;

    }st_daq_resp_03;

    unsigned long long resp03;
} ST_DAQ_RESP_03;

typedef union _ST_DAQ_RESP_04
{
    struct
    {
        unsigned long long sCell17 : 16;
        unsigned long long sCell18 : 16;
        unsigned long long sCell19 : 16;
        unsigned long long sCell20 : 16;

    }st_daq_resp_04;

    unsigned long long resp04;
} ST_DAQ_RESP_04;

typedef union _ST_DAQ_RESP_05
{
    struct
    {
        unsigned long long sCell21 : 16;
        unsigned long long sCell22 : 16;
        unsigned long long sCell23 : 16;
        unsigned long long sCell24 : 16;

    }st_daq_resp_05;

    unsigned long long resp05;
} ST_DAQ_RESP_05;

typedef union _ST_DAQ_RESP_06
{
    struct
    {
        unsigned long long sCell25 : 16;
        unsigned long long sCell26 : 16;
        unsigned long long sCell27 : 16;
        unsigned long long sCell28 : 16;

    }st_daq_resp_06;

    unsigned long long resp06;
} ST_DAQ_RESP_06;

typedef union _ST_DAQ_RESP_07
{
    struct
    {
        unsigned long long sCell29 : 16;
        unsigned long long sCell30 : 16;
        unsigned long long sCell31 : 16;
        unsigned long long sCell32 : 16;

    }st_daq_resp_07;

    unsigned long long resp07;
} ST_DAQ_RESP_07;

typedef union _ST_DAQ_RESP_08
{
    struct
    {
        unsigned long long sCell33 : 16;
        unsigned long long sCell34 : 16;
        unsigned long long sCell35 : 16;
        unsigned long long sCell36 : 16;

    }st_daq_resp_08;

    unsigned long long resp08;
} ST_DAQ_RESP_08;

typedef union _ST_DAQ_RESP_09
{
    struct
    {
        unsigned long long sCell37 : 16;
        unsigned long long sCell38 : 16;
        unsigned long long sCell39 : 16;
        unsigned long long sCell40 : 16;

    }st_daq_resp_09;

    unsigned long long resp09;
} ST_DAQ_RESP_09;

typedef union _ST_DAQ_RESP_0A
{
    struct
    {
        unsigned long long sCell41 : 16;
        unsigned long long sCell42 : 16;
        unsigned long long sCell43 : 16;
        unsigned long long sCell44 : 16;

    }st_daq_resp_0A;

    unsigned long long respOA;
} ST_DAQ_RESP_0A;

typedef union _ST_DAQ_RESP_0B
{
    struct
    {
        unsigned long long sCell45 : 16;
        unsigned long long sCell46 : 16;
        unsigned long long sCell47 : 16;
        unsigned long long sCell48 : 16;

    }st_daq_resp_0B;

    unsigned long long resp0B;
} ST_DAQ_RESP_0B;

typedef union _ST_DAQ_RESP_0C
{
    struct
    {
        unsigned long long sCell49 : 16;
        unsigned long long sCell50 : 16;
        unsigned long long sCell51 : 16;
        unsigned long long sCell52 : 16;

    }st_daq_resp_0C;

    unsigned long long resp0C;
} ST_DAQ_RESP_0C;

typedef union _ST_DAQ_RESP_0D
{
    struct
    {
        unsigned long long sCell53 : 16;
        unsigned long long sCell54 : 16;
        unsigned long long sCell55 : 16;
        unsigned long long sCell56 : 16;

    }st_daq_resp_0D;

    unsigned long long resp0D;
} ST_DAQ_RESP_0D;

typedef union _ST_DAQ_RESP_0E
{
    struct
    {
        unsigned long long sCell57 : 16;
        unsigned long long sCell58 : 16;
        unsigned long long sCell59 : 16;
        unsigned long long sCell60 : 16;

    }st_daq_resp_0E;

    unsigned long long resp0E;
} ST_DAQ_RESP_0E;

typedef union _ST_DAQ_RESP_0F
{
    struct
    {
        unsigned long long sCell61 : 16;
        unsigned long long sCell62 : 16;
        unsigned long long sCell63 : 16;
        unsigned long long sCell64 : 16;

    }st_daq_resp_0F;

    unsigned long long resp0F;
} ST_DAQ_RESP_0F;

typedef union _ST_DAQ_RESP_10
{
    struct
    {
        unsigned long long sCell65 : 16;
        unsigned long long sCell66 : 16;
        unsigned long long sCell67 : 16;
        unsigned long long sCell68 : 16;

    }st_daq_resp_10;

    unsigned long long resp10;
} ST_DAQ_RESP_10;

typedef union _ST_DAQ_RESP_11
{
    struct
    {
        unsigned long long sCell69 : 16;
        unsigned long long sCell70 : 16;
        unsigned long long sCell71 : 16;
        unsigned long long sCell72 : 16;

    }st_daq_resp_11;

    unsigned long long resp11;
} ST_DAQ_RESP_11;

typedef union _ST_DAQ_RESP_12
{
    struct
    {
        unsigned long long sCell73 : 16;
        unsigned long long sCell74 : 16;
        unsigned long long sCell75 : 16;
        unsigned long long sCell76 : 16;

    }st_daq_resp_12;

    unsigned long long resp12;
} ST_DAQ_RESP_12;

typedef union _ST_DAQ_RESP_13
{
    struct
    {
        unsigned long long sCell77 : 16;
        unsigned long long sCell78 : 16;
        unsigned long long sCell79 : 16;
        unsigned long long sCell80 : 16;

    }st_daq_resp_13;

    unsigned long long resp13;
} ST_DAQ_RESP_13;

typedef union _ST_DAQ_RESP_14
{
    struct
    {
        unsigned long long sCell81 : 16;
        unsigned long long sCell82 : 16;
        unsigned long long sCell83 : 16;
        unsigned long long sCell84 : 16;

    }st_daq_resp_14;

    unsigned long long resp14;
} ST_DAQ_RESP_14;

typedef union _ST_DAQ_RESP_15
{
    struct
    {
        unsigned long long sCell85 : 16;
        unsigned long long sCell86 : 16;
        unsigned long long sCell87 : 16;
        unsigned long long sCell88 : 16;

    }st_daq_resp_15;

    unsigned long long resp15;
} ST_DAQ_RESP_15;

typedef union _ST_DAQ_RESP_16
{
    struct
    {
        unsigned long long sCell89 : 16;
        unsigned long long sCell90 : 16;

    }st_daq_resp_16;

    unsigned long long resp16;
} ST_DAQ_RESP_16;



//TempInfo
typedef union _ST_DAQ_RESP_17
{
    struct
    {
        unsigned long long sTemp01 : 8;
        unsigned long long sTemp02 : 8;
        unsigned long long sTemp03 : 8;
        unsigned long long sTemp04 : 8;
        unsigned long long sTemp05 : 8;
        unsigned long long sTemp06 : 8;
        unsigned long long sTemp07 : 8;
        unsigned long long sTemp08 : 8;

    }st_daq_resp_17;

    unsigned long long resp17;
} ST_DAQ_RESP_17;

typedef union _ST_DAQ_RESP_18
{
    struct
    {
        unsigned long long sTemp09 : 8;
        unsigned long long sTemp10 : 8;
        unsigned long long sTemp11 : 8;
        unsigned long long sTemp12 : 8;
        unsigned long long sTemp13 : 8;
        unsigned long long sTemp14 : 8;
        unsigned long long sTemp15 : 8;
        unsigned long long sTemp16 : 8;

    }st_daq_resp_18;

    unsigned long long resp18;
} ST_DAQ_RESP_18;

typedef union _ST_DAQ_RESP_19
{
    struct
    {
        unsigned long long sTemp17 : 8;
        unsigned long long sTemp18 : 8;
        unsigned long long sTemp19 : 8;
        unsigned long long sTemp20 : 8;
        unsigned long long sTemp21 : 8;
        unsigned long long sTemp22 : 8;
        unsigned long long sTemp23 : 8;
        unsigned long long sTemp24 : 8;

    }st_daq_resp_19;

    unsigned long long resp19;
} ST_DAQ_RESP_19;

typedef union _ST_DAQ_RESP_1A
{
    struct
    {
        unsigned long long sTemp25 : 8;
        unsigned long long sTemp26 : 8;
        unsigned long long sTemp27 : 8;
        unsigned long long sTemp28 : 8;
        unsigned long long sTemp29 : 8;
        unsigned long long sTemp30 : 8;

    }st_daq_resp_1A;

    unsigned long long resp1A;
} ST_DAQ_RESP_1A;



//ModVolInfo
typedef union _ST_DAQ_RESP_1B
{
    struct
    {
        unsigned long long sModVol01 : 16;
        unsigned long long sModVol02 : 16;
        unsigned long long sModVol03 : 16;
        unsigned long long sModVol04 : 16;

    }st_daq_resp_1B;

    unsigned long long resp1B;
} ST_DAQ_RESP_1B;

typedef union _ST_DAQ_RESP_1C
{
    struct
    {
        unsigned long long sModVol05 : 16;
        unsigned long long sModVol06 : 16;
        unsigned long long sModVol07 : 16;
        unsigned long long sModVol08 : 16;

    }st_daq_resp_1C;

    unsigned long long resp1C;
} ST_DAQ_RESP_1C;



//...
typedef union _ST_DAQ_RESP_1D
{
    struct
    {
        unsigned long long sPackVol : 16;
        unsigned long long sInvVol : 16;
        unsigned long long sPackSOC : 16;
        unsigned long long sPackSOH : 16;

    }st_daq_resp_1D;

    unsigned long long resp1D;
} ST_DAQ_RESP_1D;

typedef union _ST_DAQ_RESP_1E
{
    struct
    {
        unsigned long long sPackCurr : 16;
        unsigned long long sBmsStat : 8;
        unsigned long long sBmsFltLvl : 8;
        unsigned long long sRlyStat : 8;

    }st_daq_resp_1E;

    unsigned long long resp1E;
} ST_DAQ_RESP_1E;

typedef union _ST_DAQ_RESP_1F
{
    struct
    {
        unsigned long long sMaxCellVol : 16;
        unsigned long long sAvgCellVol : 16;
        unsigned long long sMinCellVol : 16;
        unsigned long long sMaxCellNo : 8;
        unsigned long long sMinCellNo : 8;

    }st_daq_resp_1F;

    unsigned long long resp1F;
} ST_DAQ_RESP_1F;

typedef union _ST_DAQ_RESP_20
{
    struct
    {
        unsigned long long sMaxTemp : 8;
        unsigned long long sAvgTemp : 8;
        unsigned long long sMinTemp : 8;
        unsigned long long sMaxTempNo : 8;
        unsigned long long sMinTempNo : 8;

    }st_daq_resp_20;

    unsigned long long resp20;
} ST_DAQ_RESP_20;

typedef union _ST_DAQ_RESP_21
{
    struct
    {
        unsigned long long sMaxChgPwr : 16;
        unsigned long long sMaxDChgPwr : 16;
        unsigned long long sRealPwr : 16;
        unsigned long long sAvailCapa : 16;

    }st_daq_resp_21;

    unsigned long long resp21;
} ST_DAQ_RESP_21;

typedef union _ST_DAQ_RESP_22
{
    struct
    {
        unsigned long long sWrnRsrv1 : 8;
        unsigned long long sWrnRlyDrv : 1;
        unsigned long long sWrnCanComm : 1;
        unsigned long long sWrnAuxPwr : 1;
        unsigned long long sWrnCsm : 1;
        unsigned long long sWrnAfe : 1;
        unsigned long long sWrnMcu : 1;
        unsigned long long sWrnRsrv2 : 2;
        unsigned long long sWrnCurrOver : 1;
        unsigned long long sWrnCurrUnder : 1;

        unsigned long long sWrnSocOver : 1;
        unsigned long long sWrnSocUnder : 1;
        unsigned long long sWrnSohUnder : 1;
        unsigned long long sWrnRlyAge : 1;
        unsigned long long sWrnRlyOpen : 1;
        unsigned long long sWrnRlyWeld: 1;
        unsigned long long sWrnCellOver : 1;
        unsigned long long sWrnCellUnder : 1;
        unsigned long long sWrnCellDiff : 1;
        unsigned long long sWrnTempOver : 1;
        unsigned long long sWrnTempUnder : 1;
        unsigned long long sWrnTempDiff : 1;
        unsigned long long sWrnPackOver : 1;
        unsigned long long sWrnPackUnder : 1;

        unsigned long long sFltRsrv1 : 8;
        unsigned long long sFltRlyDrv : 1;
        unsigned long long sFltCanComm : 1;
        unsigned long long sFltAuxPwr : 1;
        unsigned long long sFltCsm : 1;
        unsigned long long sFltAfe : 1;
        unsigned long long sFltMcu : 1;
        unsigned long long sFltRsrv2 : 2;
        unsigned long long sFltCurrOver : 1;
        unsigned long long sFltCurrUnder : 1;

        unsigned long long sFltSocOver : 1;
        unsigned long long sFltSocUnder : 1;
        unsigned long long sFltSohUnder : 1;
        unsigned long long sFltRlyAge : 1;
        unsigned long long sFltRlyOpen : 1;
        unsigned long long sFltRlyWeld: 1;
        unsigned long long sFltCellOver : 1;
        unsigned long long sFltCellUnder : 1;
        unsigned long long sFltCellDiff : 1;
        unsigned long long sFltTempOver : 1;
        unsigned long long sFltTempUnder : 1;
        unsigned long long sFltTempDiff : 1;
        unsigned long long sFltPackOver : 1;
        unsigned long long sFltPackUnder : 1;

    } st_daq_resp_22;

    unsigned long long resp22;
} ST_DAQ_RESP_22;


//
typedef struct _RespDataInfo
{
    ST_DAQ_RESP_00  rResp00 ;
    ST_DAQ_RESP_01  rResp01 ;
    ST_DAQ_RESP_02  rResp02 ;
    ST_DAQ_RESP_03  rResp03 ;
    ST_DAQ_RESP_04  rResp04 ;
    ST_DAQ_RESP_05  rResp05 ;
    ST_DAQ_RESP_06  rResp06 ;
    ST_DAQ_RESP_07  rResp07 ;
    ST_DAQ_RESP_08  rResp08 ;
    ST_DAQ_RESP_09  rResp09 ;
    ST_DAQ_RESP_0A  rResp0A ;
    ST_DAQ_RESP_0B  rResp0B ;
    ST_DAQ_RESP_0C  rResp0C ;
    ST_DAQ_RESP_0D  rResp0D ;
    ST_DAQ_RESP_0E  rResp0E ;
    ST_DAQ_RESP_0F  rResp0F ;
    ST_DAQ_RESP_10  rResp10 ;
    ST_DAQ_RESP_11  rResp11 ;
    ST_DAQ_RESP_12  rResp12 ;
    ST_DAQ_RESP_13  rResp13 ;
    ST_DAQ_RESP_14  rResp14 ;
    ST_DAQ_RESP_15  rResp15 ;
    ST_DAQ_RESP_16  rResp16 ;
    ST_DAQ_RESP_17  rResp17 ;
    ST_DAQ_RESP_18  rResp18 ;
    ST_DAQ_RESP_19  rResp19 ;
    ST_DAQ_RESP_1A  rResp1A ;
    ST_DAQ_RESP_1B  rResp1B ;
    ST_DAQ_RESP_1C  rResp1C ;
    ST_DAQ_RESP_1D  rResp1D ;
    ST_DAQ_RESP_1E  rResp1E ;
    ST_DAQ_RESP_1F  rResp1F ;
    ST_DAQ_RESP_20  rResp20 ;
    ST_DAQ_RESP_21  rResp21 ;
    ST_DAQ_RESP_22  rResp22 ;
} RespDataInfo;

typedef struct _RxCanDataInfo
{
    RespDataInfo    respData; /* Received Rack Data over CAN */
    unsigned int    rxMsgCnt;
    uint64_t        dataDoneTime;
    bool            bDoneRxData;
    struct timeval  startTime;

    bool bFirstFlg;
} RxCanDataInfo;


typedef union _ST_DAQ_REQ
{
    struct
    {
        unsigned long long sReqPwrDwn : 1;
        unsigned long long sStopDaq : 1;
        unsigned long long sAlive : 4;
        unsigned long long sRT_SEC : 8;
        unsigned long long sRT_MIN : 8;
        unsigned long long sRT_HR : 8;
        unsigned long long sRT_DAY : 8;
        unsigned long long sRT_WEEK : 8;
        unsigned long long sRT_MNTH : 8;
        unsigned long long sRT_YEAR : 8;

    }st_daq_req;

    unsigned long long req;
} ST_DAQ_REQ;

typedef struct _ReqDataInfo
{
    ST_DAQ_REQ  rReq ;
} ReqDataInfo;

typedef struct _TxCanDataInfo
{
    ReqDataInfo     reqData; /* Transmit Rack Data over CAN */
    unsigned int    txMsgCnt;
    uint64_t        dataDoneTime;
    bool            bDoneTxData;
    struct timeval  startTime;
} TxCanDataInfo;

typedef struct _DevCanInfo
{
    char    devName[CAN_DEV_NAME_SZ];
    int     bitrate;
} DevCanInfo;

typedef struct _CanSettingInfo
{
    int                 can_s; /* socket id */
    DevCanInfo          devInfo;

    /* Rack info */
    int                 rack_total_cnt;
    int                 rack_msg_max_cnt;

    /* CAN error info */
    pthread_t           busoff_chk_threadID;
    pthread_t           ovf_chk_threadID;
    bool                bChkBufOff;
    int                 nOvfCnt;
} CanSettingInfo;

typedef struct _RespBmsCanInfo
{
    bool            bRunning;
    bool            bTxRunning;

    /* related to CAN */
    CanSettingInfo  canSetting;
    RxCanDataInfo   rxCanData;
    TxCanDataInfo   txCanData;
    int             rxCanMsgMaxCnt;

    unsigned long long stopDaq;

    /* related to  */
    pthread_t       rx_can_threadID;
    pthread_t       tx_can_threadID;
    pthread_mutex_t dataMutex;

} RespBmsCanInfo;




/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/

int IoTCan_SetCallBackFunc(void *cbfunc);
RespBmsCanInfo *IoTCan_GetRespBmsCanInfo(void);
//void IoTCan_Write(void);

//int IoTCan_SetCallBackFunc(void *cbfunc);
//RBmsCanInfo *IoTCan_GetRBmsCanInfo(void);
//int IoTCan_GetReadyRackCnt(bool bReset, RackReadyInfo *pReadyInfo);

int IoTCan_UpDownLink(bool up);
int IoTCan_Open(void);
int IoTCan_Start(void);
int IoTCan_Stop(void);
int IoTCan_Close(void);

#endif /* IOT_CAN_H_ */
