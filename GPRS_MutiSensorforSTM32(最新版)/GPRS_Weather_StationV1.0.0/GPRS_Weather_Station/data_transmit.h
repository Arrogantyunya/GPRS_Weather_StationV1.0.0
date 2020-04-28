#ifndef _DATA_TRANSMIT_H
#define _DATA_TRANSMIT_H

#include <Arduino.h>
#include "GPSData.h"

//终端类型码
#define WEATHERSTATION                      0x01 //第一代气象站
#define GREENHOUSE                          0x02  //第一代大棚
#define WEATHER_GREENHOUSE                  0x03 //第一代多合一气象站
#define WEATHER_GREENHOUSE_V2               0x06 //第二代多合一气象站
#define AIR_V2                              0x07 //第二代多合一气象站
#define AIR_V3                              0x08 //第三代多合一气象站

//GPS保存策略
#define NOT_SAVE                            0x00
#define SAVE                                0x01
#define SERVER_DECISION                     0x02

struct System_Run_Parameter{
    unsigned int g_Acquisition_Cycle = 0;            //采集周期
    unsigned int g_Transmit_Cycle = 0;               //发送周期
    int g_Run_Mode = 0;                              //运行模式
    unsigned char g_Location_Flag = 0;               //获取定位信息标志位
    char g_GPS_Mode;                                    //获取GPS定位模式
    bool g_Send_EP_Data_Flag = false;              //传感器发送数据标志位
    bool g_LBS_Connect_Seriver_Flag = false;         //基站定位服务器是否连接成功标志，TREU-成功，false-失败，不采集
    bool g_Already_Save_Data_Flag = false;                //已经存储过数据标志位
    bool g_Recive_Param_Flag = false;              //接收到服务器参数标志位
    unsigned int g_Now_Record_Count;                 //目前已发送的传感器数据记录笔数。
};

extern System_Run_Parameter Sys_Run_Para;

//连接到服务器相关变量
extern const char apn[];
extern const char user[];
extern const char pass[];
extern const char server[];
extern int port;

extern unsigned char Com_PWD[4];  //握手帧密码
extern unsigned char SysHostID[4];              //设备ID
extern bool All_ID_Status;
extern bool Clear_HostID;
extern bool Clear_HostID;
extern String SIMCCID;                                       //获取CCID码

//传感器数据缓存相关变量
extern unsigned char Send_Air_Sensor_Buff[128];
extern unsigned char Air_Data_Length;
extern unsigned char Send_Screen_Buf[128];
extern unsigned char Screen_Sindex;
extern unsigned char Phone_Sindex;
extern unsigned char Send_Phone_Buf[128];
extern GPS_INFO g_GPS_Store_Data; //存放分离后的基站定位数据
extern GPS_Dat g_G_GPS_Data;//存放分离后的GPS定位数据

bool Pre_Access_Network(void);
bool Connect_to_The_Server(void);
bool Send_Data_To_Server(void);

#endif