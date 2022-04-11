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
**   $FILNAME   : IoT_log.h                                                             **
**                                                                                      **
**   $VERSION   : v0.1                                                                  **
**                                                                                      **
**   $Date      : Dec 10, 2020                                                          **
**                                                                                      **
**   AUTHOR     : SK Dev Engineer                                                       **
**                                                                                      **
**   PROJECT    : SK IoT Gateway                                                        **
**                                                                                      **
******************************************************************************************/

#ifndef IOT_LOG_H_
#define IOT_LOG_H_

/*****************************************************************************************
**                      Includes                                                        **
******************************************************************************************/

/*****************************************************************************************
**                      Global Definitions                                              **
******************************************************************************************/
#define LOG_FILE_PATH       "./log"

#define LOG_LVL_DEBUG       0x03
#define LOG_LVL_INFO        0x02
#define LOG_LVL_WARN        0x01
#define LOG_LVL_ERR         0x00

#define LOG_IS_DEBUG    (LogGetLevel() >= LOG_LVL_DEBUG)
#define LOG_IS_INFO     (LogGetLevel() >= LOG_LVL_INFO)
#define LOG_IS_WARNING  (LogGetLevel() >= LOG_LVL_WARN)
#define LOG_IS_ERROR    (LogGetLevel() >= LOG_LVL_ERR)

#define LOG_DBG(...) \
    do { \
        if(LOG_IS_DEBUG) { \
            LogLogging('D', __func__, __LINE__, __VA_ARGS__);\
        } \
    } while(0)


#define LOG_INFO(...) \
    do { \
        if(LOG_IS_INFO) { \
            LogLogging('I', __func__, __LINE__, __VA_ARGS__);\
        } \
    } while(0)


#define LOG_WARN(...) \
    do { \
        if(LOG_IS_WARNING) { \
            LogLogging('W', __func__, __LINE__, __VA_ARGS__);\
        } \
    } while(0)


#define LOG_ERR(...) \
    do { \
        if(LOG_IS_ERROR) { \
            LogLogging('E', __func__, __LINE__, __VA_ARGS__);\
        } \
    } while(0)

/*****************************************************************************************
**                      Global Type Definitions                                         **
******************************************************************************************/

/*****************************************************************************************
**                      Global Constants                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Variables                                                **
******************************************************************************************/

/*****************************************************************************************
**                      Global Function Prototypes                                      **
******************************************************************************************/
int LogSetInfo(const char *dir, const char *prefix);
int LogLogging(char log_type, const char *func, int line_no, const char *fmt, ...);
int LogSetLevel(int log_lvl);
int LogGetLevel(void);
void LogClose(void);

#endif /* IOT_LOG_H_ */
