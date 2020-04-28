 /**
  ******************************************************************************
  * @file    GPS.h 
  * @author  yangzhaoguo
  * @version V1.0.0
  * @date    11/03/2010
  * @brief   GPS模块数据接口定义
  ******************************************************************************
  * <h2><center>&copy; COPYRIGHT 2010 yw</center></h2>
  */
#ifndef _GPSDATA_H
#define _GPSDATA_H

#include <Arduino.h>

typedef struct{
   int year;    //年
   int month;   //月
   int day;     //日
   int hour;    //时
   int minutes;  //分
   int seconds;  //秒
}date_time;  //时间结构体

typedef struct{
    date_time D;//时间
    float latitude;   //纬度
    float longitude;  //经度
     char LocationFlag;   //定位状态，“A”,已定位，“V”未定位
     char latitudestr[16];   //纬度
     char longitudestr[16];   ////经度
     char altitudestr[16];   ////经度
     char latitude_flag = 0;//纬度正负号判断
     char longitude_flag = 0;//经度正负号判断
    double altitude;     //海拔
    float course_over_ground;//对地方向
     char fix_Mode;
    char NS;           //南北极
    char EW;           //东西
    float  rate;       //速度
    float HDOP;//水平分量精度因子：为纬度和经度等误差平方和的开根号值。
    float PDOP;//三维位置精度因子：为纬度、经度和高程等误差平方和的开根号值
    float VDOP;//垂直分量精度因子
    float TDOP;//钟差精度因子：为接收仪内时表偏移误差值。
    char SNR;
    char view;
    int ACC;//LBS定位精度
}GPS_INFO;

/*存储GPS数据*/
typedef struct{

  float GPS_latitude;//纬度
  float GPS_longitude;//经度
  char GPS_latitude_str[16];//纬度字符串数组
  char GPS_longitude_str[16];//经度字符串数组
  char GPS_latitude_flag = 0;//判断纬度是否为负数
  char GPS_longitude_flag = 0;//判断经度是否为负数
  date_time GPS_time;//时间日期
  double GPS_altitude;//海拔
  char GPS_altitude_str[16];//海拔字符串数组
  float GPS_Speed;//地面速度 km/h
  char GPS_Speed_str[16];//地面速度字符串数组
  float GPS_angle;//航向角
  char GPS_angle_str[16];//航向角字符串数组
  char GPS_satellites_used;//正在使用的卫星数量

}GPS_Dat;

bool gps_parse(String gps, GPS_Dat *format_gps);
void lbs_parse(String line, GPS_INFO *GPS);
void show_gps(GPS_INFO *GPS);
static void UTC2BTC(date_time *GPS);

#endif

