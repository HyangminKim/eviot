
#ifndef SENSOR_H
#define SENSOR_H


typedef struct _SenDataInfo
{
    float  rLatitude;
    float  rLongitude;
} SenDataInfo;


typedef struct _SenInfo
{
    int iRunning;

    SenDataInfo pSenData;

} SenInfo;


SenInfo *IoT_GetImuInfo(void);


#endif
