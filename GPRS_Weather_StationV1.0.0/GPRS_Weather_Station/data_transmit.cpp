#define TINY_GSM_MODEM_SIM800
#include "GSM/TinyGsmClient.h"
#include "data_transmit.h"
#include "BCD_CON.h"
#include "User_CRC8.h"
#include "Private_Sensor.h"
#include "Periph.h"
#include "memory.h"
#include "Private_RTC.h"

extern TinyGsm modem;
extern TinyGsmClient client;

//连接到服务器相关变量
const char apn[] = "CMIOT";
const char user[] = "";
const char pass[] = "";
const char server[] = "118.25.4.217";
int port = 6969;

System_Run_Parameter Sys_Run_Para;

unsigned char Com_PWD[4] = {0x1A, 0xC4, 0xEE, 0x0B};  //握手帧密码
unsigned char SysHostID[4] = {0};              //设备ID
bool All_ID_Status = false;
bool Clear_HostID = false;
String SIMCCID;                                       //获取CCID码

//传感器数据缓存相关变量
unsigned char Send_Air_Sensor_Buff[128] = {0};
unsigned char Air_Data_Length = 0;
unsigned char Send_Screen_Buf[128] = {0};
unsigned char Screen_Sindex = 0;
unsigned char Phone_Sindex = 0;
unsigned char Send_Phone_Buf[128] = {0};
GPS_INFO g_GPS_Store_Data; //存放分离后的基站定位数据
GPS_Dat g_G_GPS_Data;//存放分离后的GPS定位数据

/*
 *brief   : 发送基站定位信息或者GPS定位信息，取决于服务器给的参数
 *para    : 无
 *return  : 无
 */
static void Send_GPS_Info(void)
{
  unsigned char PosData_Buffer[255];
  unsigned char PosIndex = 0;
  unsigned char chflag;
  unsigned char Data_BCD[6] = {0};
  char intstr[15];
  unsigned char Slen;
  unsigned char NumOfDot = 0;
  unsigned char HiByte, LoByte, flag;

  //如果从服务器获取的定位信息参数是L，发送基站定位
  if(Sys_Run_Para.g_GPS_Mode == 'L') {
    PosData_Buffer[PosIndex++] = 0xFE;
    PosData_Buffer[PosIndex++] = 0xD0;
    PosData_Buffer[PosIndex++] = 0x27;
    PosData_Buffer[PosIndex++] = SysHostID[0];
    PosData_Buffer[PosIndex++] = SysHostID[1];
    PosData_Buffer[PosIndex++] = SysHostID[2];
    PosData_Buffer[PosIndex++] = SysHostID[3];

    #if TYPE06
    PosData_Buffer[PosIndex++] = WEATHER_GREENHOUSE_V2;
    #elif TYPE07
    PosData_Buffer[PosIndex++] = AIR_V2;
    #elif TYPE08
    PosData_Buffer[PosIndex++] = AIR_V3;
    #endif

    PosData_Buffer[PosIndex++] = SERVER_DECISION;

    PosData_Buffer[PosIndex++] = 0x4C;//GPS_Store_Data.LocationFlag;

    //GPS(纬度)
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    Slen = strlen(g_GPS_Store_Data.latitudestr);
    FloatStringToIntString(g_GPS_Store_Data.latitudestr, intstr, &NumOfDot, Slen);
    ASC2BCD(Data_BCD, intstr, strlen(intstr));

    PosData_Buffer[PosIndex++] = Data_BCD[0];
    PosData_Buffer[PosIndex++] = Data_BCD[1];
    PosData_Buffer[PosIndex++] = Data_BCD[2];
    PosData_Buffer[PosIndex++] = Data_BCD[3];
    PosData_Buffer[PosIndex++] = Data_BCD[4];
    
    if (g_GPS_Store_Data.latitude_flag == 0)
      PosData_Buffer[PosIndex++] = 0xE0 | 0x08;
    else if (g_GPS_Store_Data.latitude_flag == 1)
      PosData_Buffer[PosIndex++] = 0xF0 | 0x08;

    //GPS(经度)
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    Slen = strlen(g_GPS_Store_Data.longitudestr);
    FloatStringToIntString(g_GPS_Store_Data.longitudestr, intstr, &NumOfDot, Slen);
    ASC2BCD(Data_BCD, intstr, strlen(intstr));

    PosData_Buffer[PosIndex++] = Data_BCD[0];
    PosData_Buffer[PosIndex++] = Data_BCD[1];
    PosData_Buffer[PosIndex++] = Data_BCD[2];
    PosData_Buffer[PosIndex++] = Data_BCD[3];
    PosData_Buffer[PosIndex++] = Data_BCD[4];

    if (g_GPS_Store_Data.longitude_flag == 0)
      PosData_Buffer[PosIndex++] = 0xE0 | 0x07;
    else if (g_GPS_Store_Data.longitude_flag == 1)
      PosData_Buffer[PosIndex++] = 0xF0 | 0x07;

    //海拔
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xE0 | 0x02;

    //地面速度
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xE0 | 0x05;

    //航向角
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xFF;
    PosData_Buffer[PosIndex++] = 0xE0 | 0x03;

    //使用的卫星数量
    memset(Data_BCD, 0x00, sizeof(Data_BCD));
    PosData_Buffer[PosIndex++] = 0xFF;

    ToBCD(g_GPS_Store_Data.D.year, &HiByte, &LoByte, &flag);
    PosData_Buffer[PosIndex++] = HiByte;
    PosData_Buffer[PosIndex++] = LoByte;
    PosData_Buffer[PosIndex++] = ByteTOBcd(g_GPS_Store_Data.D.month);
    PosData_Buffer[PosIndex++] = ByteTOBcd(g_GPS_Store_Data.D.day);
    PosData_Buffer[PosIndex++] = ByteTOBcd(g_GPS_Store_Data.D.hour);
    PosData_Buffer[PosIndex++] = ByteTOBcd(g_GPS_Store_Data.D.minutes);
    PosData_Buffer[PosIndex++] = ByteTOBcd(g_GPS_Store_Data.D.seconds);

    //计算数据长度
    PosData_Buffer[2] = PosIndex - 3;
    unsigned char CRC8 = GetCrc8(PosData_Buffer + 3,PosIndex - 3);
    PosData_Buffer[PosIndex++] = CRC8;//CRC8校验码

    PosData_Buffer[PosIndex++] = 0x0D;

    client.write(PosData_Buffer, PosIndex);
  }
  //如果从服务器获取的定位信息参数是G，发送GPS定位
  else if (Sys_Run_Para.g_GPS_Mode = 'G') {
    //GPS Data:1,1,20180717061922.000,25.051970,102.652083,1866.774,0.81,16.1,1,,1.9,2.1,1.0,,7,5,,,46,,
      PosIndex = 0;
      PosData_Buffer[PosIndex++] = 0xFE;
      PosData_Buffer[PosIndex++] = 0xD0;
      PosData_Buffer[PosIndex++] = 0x27;

      PosData_Buffer[PosIndex++] = SysHostID[0];
      PosData_Buffer[PosIndex++] = SysHostID[1];
      PosData_Buffer[PosIndex++] = SysHostID[2];
      PosData_Buffer[PosIndex++] = SysHostID[3];

      #if TYPE06
      PosData_Buffer[PosIndex++] = WEATHER_GREENHOUSE_V2;
      #elif TYPE07
      PosData_Buffer[PosIndex++] = AIR_V2;
      #elif TYPE08
      PosData_Buffer[PosIndex++] = AIR_V3;
      #endif

      PosData_Buffer[PosIndex++] = SERVER_DECISION;

      PosData_Buffer[PosIndex++] = 0x47;//GPS_Store_Data.LocationFlag;
      
      //GPS(纬度)
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      Slen = strlen(g_G_GPS_Data.GPS_latitude_str);
      FloatStringToIntString(g_G_GPS_Data.GPS_latitude_str, intstr, &NumOfDot, Slen);

      ASC2BCD(Data_BCD, intstr, strlen(intstr));
      PosData_Buffer[PosIndex++] = Data_BCD[0];
      PosData_Buffer[PosIndex++] = Data_BCD[1];
      PosData_Buffer[PosIndex++] = Data_BCD[2];
      PosData_Buffer[PosIndex++] = Data_BCD[3];
      PosData_Buffer[PosIndex++] = Data_BCD[4];

      if (g_G_GPS_Data.GPS_latitude_flag == 0)
        PosData_Buffer[PosIndex++] = 0xE0 | 0x08;
      else if (g_G_GPS_Data.GPS_latitude_flag == 1)
        PosData_Buffer[PosIndex++] = 0xF0 | 0x08;

      //GPS(经度)
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      Slen = strlen(g_G_GPS_Data.GPS_longitude_str);
      FloatStringToIntString(g_G_GPS_Data.GPS_longitude_str, intstr, &NumOfDot, Slen);
      ASC2BCD(Data_BCD, intstr, strlen(intstr));

      PosData_Buffer[PosIndex++] = Data_BCD[0];
      PosData_Buffer[PosIndex++] = Data_BCD[1];
      PosData_Buffer[PosIndex++] = Data_BCD[2];
      PosData_Buffer[PosIndex++] = Data_BCD[3];
      PosData_Buffer[PosIndex++] = Data_BCD[4];

      if (g_G_GPS_Data.GPS_longitude_flag == 0)
        PosData_Buffer[PosIndex++] = 0xE0 | 0x07;
      else if (g_G_GPS_Data.GPS_longitude_flag == 1)
        PosData_Buffer[PosIndex++] = 0xF0 | 0x07;

      //海拔
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      Slen = strlen(g_G_GPS_Data.GPS_altitude_str);
      FloatStringToIntString(g_G_GPS_Data.GPS_altitude_str, intstr, &NumOfDot, Slen);
      ASC2BCD(Data_BCD, intstr, strlen(intstr));

      PosData_Buffer[PosIndex++] = Data_BCD[0];
      PosData_Buffer[PosIndex++] = Data_BCD[1];
      PosData_Buffer[PosIndex++] = Data_BCD[2];
      PosData_Buffer[PosIndex++] = 0xE0 | 0x02;

      //地面速度
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      Slen = strlen(g_G_GPS_Data.GPS_Speed_str);
      FloatStringToIntString(g_G_GPS_Data.GPS_Speed_str, intstr, &NumOfDot, Slen);
      ASC2BCD(Data_BCD, intstr, strlen(intstr));

      PosData_Buffer[PosIndex++] = Data_BCD[0];
      PosData_Buffer[PosIndex++] = Data_BCD[1];
      PosData_Buffer[PosIndex++] = Data_BCD[2];
      PosData_Buffer[PosIndex++] = 0xE0 | 0x05;

      //航向角
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      Slen = strlen(g_G_GPS_Data.GPS_angle_str);
      FloatStringToIntString(g_G_GPS_Data.GPS_angle_str, intstr, &NumOfDot, Slen);

      ASC2BCD(Data_BCD, intstr, strlen(intstr));
      PosData_Buffer[PosIndex++] = Data_BCD[0];
      PosData_Buffer[PosIndex++] = Data_BCD[1];
      PosData_Buffer[PosIndex++] = Data_BCD[2];
      PosData_Buffer[PosIndex++] = 0xE0 | 0x03;

      //使用的卫星数量
      memset(Data_BCD, 0x00, sizeof(Data_BCD));
      PosData_Buffer[PosIndex++] = g_G_GPS_Data.GPS_satellites_used;

      //日期时间
      ToBCD(g_G_GPS_Data.GPS_time.year, &HiByte, &LoByte, &flag);
      PosData_Buffer[PosIndex++] = HiByte;
      PosData_Buffer[PosIndex++] = LoByte;
      PosData_Buffer[PosIndex++] = ByteTOBcd(g_G_GPS_Data.GPS_time.month);
      PosData_Buffer[PosIndex++] = ByteTOBcd(g_G_GPS_Data.GPS_time.day);
      PosData_Buffer[PosIndex++] = ByteTOBcd(g_G_GPS_Data.GPS_time.hour);
      PosData_Buffer[PosIndex++] = ByteTOBcd(g_G_GPS_Data.GPS_time.minutes);
      PosData_Buffer[PosIndex++] = ByteTOBcd(g_G_GPS_Data.GPS_time.seconds);

      //计算数据长度
      PosData_Buffer[2] = PosIndex - 3;
      unsigned char CRC8 = GetCrc8(PosData_Buffer + 3,PosIndex - 3);
      PosData_Buffer[PosIndex++] = CRC8;//CRC8校验码

      PosData_Buffer[PosIndex++] = 0x0D;

      client.write(PosData_Buffer, PosIndex);
  }
}

/*
 *brief   : 发送大气和大棚的一些气象数据到服务器
 *para    : 无
 *return  : 无
 */
static void Send_Air_Muti_Sensor_Data_to_Server(void)
{
  unsigned char HiByte, LoByte, flag;
  unsigned char NumOfDot = 0;
  unsigned char Data_BCD[4] = {0};
  char weathertr[20] = {0};
  float Temperature;
 
  Air_Data_Length = 0;

  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFE;
  #if TYPE06
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xCB;
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0x54;

  #elif TYPE07
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xCC;
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0x38;

  #elif TYPE08
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xCD;
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0x3A;
  #endif

  Send_Air_Sensor_Buff[Air_Data_Length++] = SysHostID[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = SysHostID[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = SysHostID[2];
  Send_Air_Sensor_Buff[Air_Data_Length++] = SysHostID[3];

  #if TYPE06
  Send_Air_Sensor_Buff[Air_Data_Length++] = WEATHER_GREENHOUSE_V2;
  #elif TYPE07
  Send_Air_Sensor_Buff[Air_Data_Length++] = AIR_V2;
  #elif TYPE08
  Send_Air_Sensor_Buff[Air_Data_Length++] = AIR_V3;
  #endif

  //大气温度
  NumOfDot = 2;
  if ((Muti_Sensor_Data.Air_Temp >= 65535) && (Muti_Sensor_Data.Air_Temp_Flag != 1))
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {

    if (Muti_Sensor_Data.Air_Temp_Flag == 1)
    {
      Temperature = (float)(65536 - Muti_Sensor_Data.Air_Temp) / 10.0;
    }
    else
    {
      Temperature =  (float)(Muti_Sensor_Data.Air_Temp) / 10.0;
    }

    PackBCD((char *)Data_BCD, Temperature, 4, NumOfDot);//读取回来BCD数据数组、大气温度传感器数据、数据宽度、数据小数
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];//把转换好的BCD码数据给发送缓存数组。
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }

  if(Muti_Sensor_Data.Air_Temp_Flag == 1)//判断大气温度是零上还是零下
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xF0 | NumOfDot;//最高位是1，表示负的数值
  }
  else
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;//最高位是0，表示正的数值
  }


  //大气湿度
  memset(Data_BCD, 0x00, sizeof(Data_BCD));
  NumOfDot = 2;

  if ((int)(Muti_Sensor_Data.Air_Humi) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Air_Humi, 4, NumOfDot);//把大气湿度转换成BCD码
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  
  //大气光照强度
  NumOfDot = 0;

  memset(Data_BCD,0x00,sizeof(Data_BCD));
  memset(weathertr, 0x00, sizeof(weathertr));

  if (Muti_Sensor_Data.Air_Lux > 200000)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    sprintf(weathertr, "%08ld", Muti_Sensor_Data.Air_Lux);
    //该函数是将一个ASCII码字符串转换成BCD码
    ASC2BCD(Data_BCD, weathertr, strlen(weathertr));//读取BCD数据数组、ASCII码字符串、该字符串的长度

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[2];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[3];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  
  //大气气压
  NumOfDot = 2;

  memset(Data_BCD,0x00,sizeof(Data_BCD));
  memset(weathertr, 0x00, sizeof(weathertr));

  sprintf(weathertr, "%08ld", Muti_Sensor_Data.Air_Atmos);
  //该函数是将一个ASCII码字符串转换成BCD码
  ASC2BCD(Data_BCD, weathertr, strlen(weathertr));//读取BCD数据数组、ASCII码字符串、该字符串的长度

  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[2];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[3];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大气紫外线强度
  NumOfDot = 0;
  memset(Data_BCD,0x00,sizeof(Data_BCD));

  if ((int)(Muti_Sensor_Data.Air_UV) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Air_UV, 4, NumOfDot);

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;


  //风速
  NumOfDot = 1;
  if ((unsigned int)(Muti_Sensor_Data.Air_Wind_Speed) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Air_Wind_Speed, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  
  //风向
  NumOfDot = 0;
  if (Muti_Sensor_Data.Wind_DirCode >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Wind_DirCode, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }

  //大气二氧化碳
  NumOfDot = 0;
  if (Muti_Sensor_Data.Air_CO2 >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Air_CO2, 4, NumOfDot);

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大气TVOC 
  NumOfDot = 0;
  if (Muti_Sensor_Data.Air_TVOC >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Air_TVOC, 4, NumOfDot);

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  #if TYPE06
  //大棚温度
  NumOfDot = 2;
  memset(Data_BCD,0x00,sizeof(Data_BCD));

  if ((Muti_Sensor_Data.GreenHouse_Temp >= 65535) && (Muti_Sensor_Data.GreenHouse_Temp_Flag != 1))
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    if (Muti_Sensor_Data.GreenHouse_Temp_Flag == 1)
    {
      Temperature = (float)(65536 - Muti_Sensor_Data.GreenHouse_Temp) / 10.0;
    }
    else
    {
      Temperature =  (float)(Muti_Sensor_Data.GreenHouse_Temp) / 10.0;
    }

    PackBCD((char *)Data_BCD, Temperature, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }

  if(Muti_Sensor_Data.GreenHouse_Temp_Flag == 1)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xF0 | NumOfDot;
  }
  else
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  }

  //大棚湿度
  NumOfDot = 2;
  memset(Data_BCD,0x00,sizeof(Data_BCD));

  if ((int)(Muti_Sensor_Data.GreenHouse_Humi) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.GreenHouse_Humi, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大棚光照度
  NumOfDot = 0;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));
  memset(weathertr, 0x00, sizeof(weathertr));

  if (Muti_Sensor_Data.GreenHouse_Lux > 200000)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    sprintf(weathertr, "%08ld", Muti_Sensor_Data.GreenHouse_Lux);
    ASC2BCD(Data_BCD, weathertr, strlen(weathertr));

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[2];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[3];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大棚大气压力
  NumOfDot = 2;

  memset(Data_BCD,0x00,sizeof(Data_BCD));
  memset(weathertr, 0x00, sizeof(weathertr));

  sprintf(weathertr, "%08ld", Muti_Sensor_Data.GreenHouse_Atmos);
  //该函数是将一个ASCII码字符串转换成BCD码
  ASC2BCD(Data_BCD, weathertr, strlen(weathertr));//读取BCD数据数组、ASCII码字符串、该字符串的长度

  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[2];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[3];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大棚紫外线强度
  NumOfDot = 0;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));

  if ((int)(Muti_Sensor_Data.GreenHouse_UV) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.GreenHouse_UV, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大棚CO2
  NumOfDot = 0;
  if (Muti_Sensor_Data.GreenHouse_CO2 >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.GreenHouse_CO2, 4, NumOfDot);

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //大棚TVOC
  NumOfDot = 0;
  if (Muti_Sensor_Data.GreenHouse_TVOC >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.GreenHouse_TVOC, 4, NumOfDot);

    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

   //土壤温度
  NumOfDot = 2;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));

  if ((Muti_Sensor_Data.Soil_Temp >= 65535) && (Muti_Sensor_Data.Soil_Temp_Flag != 1))
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    if (Muti_Sensor_Data.Soil_Temp_Flag == 1)
    {
      Temperature = (float)(65536 - Muti_Sensor_Data.Soil_Temp) / 10.0;
    }
    else
    {
      Temperature =  (float)(Muti_Sensor_Data.Soil_Temp) / 10.0;
    }

    PackBCD((char *)Data_BCD, Temperature, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }

  if(Muti_Sensor_Data.Soil_Temp_Flag == 1)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xF0 | NumOfDot;
  }
  else
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  }
  
  //土壤湿度
  NumOfDot = 2;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));

  if ((int)(Muti_Sensor_Data.Soil_Humi) >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Soil_Humi, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //土壤导电率
  NumOfDot = 0;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));

  if (Muti_Sensor_Data.Soil_Cond >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Soil_Cond, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //土壤盐分
  NumOfDot = 0;
  memset(Data_BCD, 0x00, sizeof(Data_BCD));

  if (Muti_Sensor_Data.Soil_Salt >= 65535)
  {
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
    Send_Air_Sensor_Buff[Air_Data_Length++] = 0xFF;
  }
  else
  {
    PackBCD((char *)Data_BCD, Muti_Sensor_Data.Soil_Salt, 4, NumOfDot);
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
    Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  }
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;
  
  #elif TYPE07
  //水位值
  NumOfDot = 0;
  PackBCD((char *)Data_BCD, Muti_Sensor_Data.Water_Level, 4, NumOfDot);
  
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];

  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0 | NumOfDot;

  //扩展的ADC电压值
  NumOfDot = 0;

  PackBCD((char *)Data_BCD, Muti_Sensor_Data.ADC_Value1, 4, NumOfDot);

  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;//E是正数，后面是小数位，0就是0位该数据有0位小数值

  //扩展的ADC电压值
  NumOfDot = 0;

  PackBCD((char *)Data_BCD, Muti_Sensor_Data.ADC_Value2, 4, NumOfDot);

  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;//E是正数，后面是小数位，0就是0位该数据有0位小数值

  #endif

  #if (TYPE06 || TYPE07)
  //雨雪变送器
  Send_Air_Sensor_Buff[Air_Data_Length++] = Muti_Sensor_Data.Rainfall;

  #elif TYPE08
  //光学雨量传感器
  NumOfDot = 0;
  unsigned int Rainfall_Temp;
  //实时降雨量
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[0] << 8 | Muti_Sensor_Data.Rainfall_Buffer[1];
  PackBCD((char *)Data_BCD, Rainfall_Temp, 4, NumOfDot);
  
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;

  //小时降雨量
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[2] << 8 | Muti_Sensor_Data.Rainfall_Buffer[3];
  PackBCD((char *)Data_BCD, Rainfall_Temp, 4, NumOfDot);
  
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;

  //月降雨量
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[4] << 8 | Muti_Sensor_Data.Rainfall_Buffer[5];
  PackBCD((char *)Data_BCD, Rainfall_Temp, 4, NumOfDot);
  
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;

  //年降雨量
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[6] << 8 | Muti_Sensor_Data.Rainfall_Buffer[7];
  PackBCD((char *)Data_BCD, Rainfall_Temp, 4, NumOfDot);
  
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[0];
  Send_Air_Sensor_Buff[Air_Data_Length++] = Data_BCD[1];
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xE0;

  #endif

  //电压
  unsigned int BatVol = Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES);
  ToBCD(BatVol, &HiByte, &LoByte, &flag);
  Send_Air_Sensor_Buff[Air_Data_Length++] = HiByte;
  Send_Air_Sensor_Buff[Air_Data_Length++] = LoByte;

  //信号强度
  int CSQ = 0;
  CSQ = modem.getSignalQuality();
  ToBCD(CSQ, &HiByte, &LoByte, &flag);
  Send_Air_Sensor_Buff[Air_Data_Length++] = HiByte;
  Send_Air_Sensor_Buff[Air_Data_Length++] = LoByte;

  //时间
  ToBCD(Muti_Sensor_Data.curTime.tm_year, &HiByte, &LoByte, &flag);
  Send_Air_Sensor_Buff[Air_Data_Length++] = HiByte;
  Send_Air_Sensor_Buff[Air_Data_Length++] = LoByte;
  Send_Air_Sensor_Buff[Air_Data_Length++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_mon);
  Send_Air_Sensor_Buff[Air_Data_Length++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_mday);
  Send_Air_Sensor_Buff[Air_Data_Length++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_hour);
  Send_Air_Sensor_Buff[Air_Data_Length++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_min);
  Send_Air_Sensor_Buff[Air_Data_Length++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_sec);

  //计算数据长度
  Send_Air_Sensor_Buff[2] = Air_Data_Length - 3;
  unsigned char CRC8 = GetCrc8(Send_Air_Sensor_Buff + 3, Air_Data_Length - 3);
  Send_Air_Sensor_Buff[Air_Data_Length++] = CRC8;//CRC8校验码

  Send_Air_Sensor_Buff[Air_Data_Length++] = 0x0D;

  client.write(Send_Air_Sensor_Buff, Air_Data_Length);
}

/*
 *brief   : 发送本机物联卡20位卡号后13位给服务器
 *para    : 无
 *return  : 无
*/
static void Send_Phone_Data_to_Server(void)
{
  unsigned char HiByte, LoByte, flag;
  char Phone_number[20];
  
  Phone_Sindex = 0;
  Send_Phone_Buf[Phone_Sindex++] = 0xFE;
  Send_Phone_Buf[Phone_Sindex++] = 0xC1;
  Send_Phone_Buf[Phone_Sindex++] = 24;

  Send_Phone_Buf[Phone_Sindex++] = SysHostID[0];
  Send_Phone_Buf[Phone_Sindex++] = SysHostID[1];
  Send_Phone_Buf[Phone_Sindex++] = SysHostID[2];
  Send_Phone_Buf[Phone_Sindex++] = SysHostID[3];

  strcpy(Phone_number, SIMCCID.c_str());

  for (int i = 0; i < 20; i++){
    Phone_number[i] = Phone_number[i] - '0';
    if (Phone_number[i] > 9)
    {
      Phone_number[i] -= 39;
    }
  }              

  for(int i = 7; i < 20; i++)
  {
    Send_Phone_Buf[Phone_Sindex++] = Phone_number[i];
  }

  ToBCD(Muti_Sensor_Data.curTime.tm_year, &HiByte, &LoByte, &flag);
  Send_Phone_Buf[Phone_Sindex++] = HiByte;
  Send_Phone_Buf[Phone_Sindex++] = LoByte;
  Send_Phone_Buf[Phone_Sindex++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_mon);
  Send_Phone_Buf[Phone_Sindex++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_mday);
  Send_Phone_Buf[Phone_Sindex++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_hour);
  Send_Phone_Buf[Phone_Sindex++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_min);
  Send_Phone_Buf[Phone_Sindex++] = ByteTOBcd(Muti_Sensor_Data.curTime.tm_sec);
  //计算数据长度
  Send_Phone_Buf[2] = Phone_Sindex - 3;
  unsigned char CRC8_OfTxDataToServer = GetCrc8(Send_Phone_Buf+3,Phone_Sindex-3);//计算CRC8 
  Send_Phone_Buf[Phone_Sindex++] = CRC8_OfTxDataToServer;
  Send_Phone_Buf[Phone_Sindex++] = 0x0D;

  client.write(Send_Phone_Buf, Phone_Sindex);
}

/*
 *brief   : 将传感器数据发送到服务器
 *para    : 无
 *return  : 无
 */
bool Send_EEPROM_Muti_Sensor_Data_to_Server(void)
{   
  unsigned NowRecord = Read_Sys_Current_Record();  //读取目前已发送了多少笔数的记录

  //如果目前发送的数据笔数小于0或者大于最大笔数，EEPROM异常
  //笔数记录默认只发送本次采集的这次数据
  if(NowRecord <= 0 || NowRecord > EEPROM_MAX_RECORD)
    NowRecord = 1;

  noInterrupts();
  EpromDb.open(EEPROM_BASE_ADDR);  //打开传感器数据基地址
  unsigned long Remain_Record = EpromDb.count();  //得到已经存储的剩余笔数
  interrupts();

  Serial.print("SensorData Count:");
  Serial.println(Remain_Record);

  if ((Remain_Record >= 1) && (Remain_Record < EEPROM_MAX_RECORD)){
    Serial.println("Send saved data...");
    Sys_Run_Para.g_Send_EP_Data_Flag = true;  //EP已经保存了数据，发送EP里的数据标志位

  }else{
    Serial.println("Send real time data...");
    Sys_Run_Para.g_Send_EP_Data_Flag = false;  //EP为空，发送实时不保存的数据
    Remain_Record = 1;
  }

  //循环发送保存的数据
  while (NowRecord <= Remain_Record){   

    if (Sys_Run_Para.g_Send_EP_Data_Flag == true){
      noInterrupts();
      EpromDb.open(EEPROM_BASE_ADDR);
      EDB_Status result = EpromDb.readRec(NowRecord, EDB_REC Muti_Sensor_Data);
      interrupts();
    }

      Serial.print("Now record count: "); 
      Serial.println(NowRecord);

      Serial.print("Save sensor data RTC:");
      Serial.print(Muti_Sensor_Data.curTime.tm_year);
      Serial.print("-");
      Serial.print(Muti_Sensor_Data.curTime.tm_mon);
      Serial.print("-");
      Serial.print(Muti_Sensor_Data.curTime.tm_mday);
      Serial.print("  ");
      Serial.print(Muti_Sensor_Data.curTime.tm_hour);
      Serial.print(":");
      Serial.print(Muti_Sensor_Data.curTime.tm_min);
      Serial.print(":");
      Serial.println(Muti_Sensor_Data.curTime.tm_sec);

      if (client.connected()){
        if (Sys_Run_Para.g_Location_Flag == 1) { 
          Serial.println("Send GPS to server OK...");
          Send_GPS_Info();  //发送GPS定位帧
          delay(1000);
        }
        
        Serial.println("Send air and greenhouse data to server...");
        Send_Air_Muti_Sensor_Data_to_Server();//发送大气传感器数据到服务器
        delay(1000);

        Serial.println("Send phone number to server...");
        Send_Phone_Data_to_Server();//发送手机号码传感器数据到服务器
        delay(1000);
        
        NowRecord++;  //每发送完一笔数据，目前发送笔数加1
        Save_Sys_Current_Record(NowRecord);  //并将目前发送笔数记录时时保存
      }
      else{
        client.stop();
        return false;
      }

      if (Sys_Run_Para.g_Send_EP_Data_Flag == false)  //如果该标志位为假，EEPROM异常，只发送目前采集这一笔，然后退出循环
        break;
    }

  noInterrupts();

  if (Sys_Run_Para.g_Send_EP_Data_Flag == true){

    #if DEVICE_V2_5
    EP_Write_Enable();
    #endif

    EpromDb.open(EEPROM_BASE_ADDR);
    EpromDb.clear();  //发送完成后，清除EEPROM数据

    #if DEVICE_V2_5
        EP_Write_Disable();
    #endif
  }

  time_t Now_Time = 0;
  Now_Time = InRtc.getTime();

  if (Sys_Run_Para.g_Transmit_Cycle < 60 || Sys_Run_Para.g_Transmit_Cycle > 43200)
    Sys_Run_Para.g_Transmit_Cycle = 180;

  Now_Time += Sys_Run_Para.g_Transmit_Cycle; 
  InRtc.setAlarmTime(Now_Time); //设置RTC闹钟

  interrupts();

  NowRecord = 1;
  Save_Sys_Current_Record(NowRecord);
  
  client.stop();
  return true;
}       

void Print_GSM_Card_Info(void)
{
  int CSQ = 0;
  CSQ = modem.getSignalQuality(); //得到信号强度

  GSM_STATUS_LED_ON;
  Server_STATUS_LED_OFF;
  Serial.println("GSM Enter network OK... <Print_GSM_Card_Info>");
  Serial.print("CSQ: ");
  Serial.println(CSQ);

  String IMEI = modem.getIMEI();  //得到本卡的IMEI码
  Serial.println("IMEI:" + IMEI);

  SIMCCID = modem.getSimCCID(); //得到本卡的CCID码
  Serial.println("SIMCCID:" + SIMCCID);  
}

bool Is_Apply_For_ID(void)
{
  //如果本机四个帧ID都为0，请求服务器分配用户终端编号
  if(SysHostID[0] == 0 && SysHostID[1] == 0 && SysHostID[2] == 0 && SysHostID[3] == 0){

    Serial.println("HostUserID is zero,requesting server provide hostUserID... <Is_Applay_For_ID>");
    unsigned char Request_ID_Frame[128];
    unsigned char Frame_length = 0;
    //请求服务器分配帧ID的协议帧
    Request_ID_Frame[Frame_length++] = 0xFE;
    Request_ID_Frame[Frame_length++] = 0x02;
    Request_ID_Frame[Frame_length++] = 0x09;

    Request_ID_Frame[Frame_length++] = SysHostID[0];
    Request_ID_Frame[Frame_length++] = SysHostID[1];
    Request_ID_Frame[Frame_length++] = SysHostID[2];
    Request_ID_Frame[Frame_length++] = SysHostID[3];

    Request_ID_Frame[Frame_length++] = WEATHERSTATION;

    Request_ID_Frame[Frame_length++] = Com_PWD[0];
    Request_ID_Frame[Frame_length++] = Com_PWD[1];
    Request_ID_Frame[Frame_length++] = Com_PWD[2];
    Request_ID_Frame[Frame_length++] = Com_PWD[3];   

    unsigned char CRC8 = GetCrc8(&Request_ID_Frame[3], Frame_length - 3);//计算CRC8  
    
    Request_ID_Frame[Frame_length++] = CRC8;//CRC8校验码
    Request_ID_Frame[Frame_length++] = 0x0D;
      
    String hostUserID;
    bool Save_ID_Flag = false;
    unsigned char Save_ID_Times = 0;

    while (client.connected() && (Save_ID_Flag != true)){
      //请求服务器分配用户终端编号
      client.write(Request_ID_Frame, Frame_length);
      delay(1000);

      while ((client.available() > 0) && (Save_ID_Flag != true)){
        char c = client.read();
        if (c <= 0)
          continue; // Skip 0x00 bytes, just in case

        hostUserID += c;

        //如果遇到数据的帧头是：#USERNUM:
        if (hostUserID.endsWith("#USERNUM:")){
          //读取数据直到遇到\n
          String ID = client.readStringUntil('\n');
          char Idstr[5];
          ID.toCharArray(Idstr, 5);

          for (unsigned char i = 0; i < 4; i++){
            if (Idstr[i] < 'A')
              SysHostID[i] = Idstr[i] - '0';
            else
              SysHostID[i] = Idstr[i] - '7';
          }
          Serial.println("Host user ID:" + ID);

          //将本机ID保存4份
          if (Save_System_ID(SysHostID)){
            Clear_HostID = false;
            Save_ID_Flag = true;
          }
          else{
            Save_ID_Times++;
            Clear_SYS_HOSTID();
            Serial.println("Configure ID Failed!");
          }
        }
      }
      if (Save_ID_Times > 2){
        client.stop();
        return false;
      }
      delay(4000);
    }
  }
  return true;
}

void Receive_Sys_Param_From_Server(void)
{
  String Rcv_Data;

  //接收服务器发下来的参数信息
  while (client.connected() && (Sys_Run_Para.g_Recive_Param_Flag != true)){
    while (client.available() > 0 && (Sys_Run_Para.g_Recive_Param_Flag != true)){
      char c = client.read();
      if (c <= 0) continue;
      Rcv_Data += c;

      /*应答帧#PARAM:2018-01-17,15:01:57,120,300,O,L\n*/

      //如果遇到#PARAM:
      if (Rcv_Data.endsWith("#PARAM:")){
        String DataCut = client.readStringUntil('\n');  //获取字符串数据，直到遇到\n字符
        String Str_Temp;
        char Data_Buff[11] = {0};
        Serial.println("Server parameters:" + DataCut);

        //截取日期字符串
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));
        Str_Temp.toCharArray(Data_Buff, 11);

        RtcTime.year  = (Data_Buff[0] - '0') * 1000 + (Data_Buff[1] - '0')  *100 + (Data_Buff[2] - '0') * 10 + (Data_Buff[3] - '0');
        RtcTime.month = (Data_Buff[5] - '0') * 10 + (Data_Buff[6] - '0');
        RtcTime.day   = (Data_Buff[8] - '0') * 10 + (Data_Buff[9] - '0');
        memset(Data_Buff, 0x00, sizeof(Data_Buff));
        Serial.println("Received server date:" + Str_Temp);

        //截取时间字符串
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));

        Str_Temp.toCharArray(Data_Buff, 11);
        Serial.println("Received server time:" + Str_Temp);

        RtcTime.hour    = (Data_Buff[0] - '0') * 10 + (Data_Buff[1] - '0');
        RtcTime.minutes = (Data_Buff[3] - '0') * 10 + (Data_Buff[4] - '0');
        RtcTime.seconds = (Data_Buff[6] - '0') * 10 + (Data_Buff[7] - '0');

        //以从服务器接收到的RTC，来更新本地RTC。
        UTCTime CurrentSec = osal_ConvertUTCSecs(&RtcTime);
        InRtc.setTime(CurrentSec);

        //判断是否该获取定位信息
        if (RtcTime.hour == 15 && (RtcTime.minutes >= 0 && RtcTime.minutes <= 59) )
          Sys_Run_Para.g_Location_Flag = 1;
        else
          Sys_Run_Para.g_Location_Flag = 0;

        //打印出从服务器保存的RTC，确保保存正确
        CurrentSec = InRtc.getTime();
        osal_ConvertUTCTime(&RtcTime, CurrentSec);
        char str[50];
        sprintf(str, "%04ld-%02ld-%02ld,%02ld:%02ld:%02ld\r\n", RtcTime.year, RtcTime.month, RtcTime.day, RtcTime.hour, RtcTime.minutes, RtcTime.seconds);
        Serial.println(str);

        //获取采集周期参数
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));
        Sys_Run_Para.g_Acquisition_Cycle = Str_Temp.toInt();
        Serial.println("Acquisition cycle:" + Str_Temp);
        if (Sys_Run_Para.g_Acquisition_Cycle < 60 || Sys_Run_Para.g_Acquisition_Cycle > 43200)  // 边界范围：1分钟到12小时之间
          Sys_Run_Para.g_Acquisition_Cycle = 180;

        //获取发送周期参数
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));
        Sys_Run_Para.g_Transmit_Cycle = Str_Temp.toInt();
        Serial.println("Transmit cycle:" + Str_Temp);
        if (Sys_Run_Para.g_Transmit_Cycle < 60 || Sys_Run_Para.g_Transmit_Cycle > 43200)
          Sys_Run_Para.g_Transmit_Cycle = 180;

        //获取运行模式参数
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));
        Sys_Run_Para.g_Run_Mode = Str_Temp.toInt();
        Serial.println("Run mode: " + Str_Temp);

        //获取定位模式参数
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        //Serial.println("DataCut:  " + DataCut);
        //Str_Temp = DataCut.substring(0, 1);
        Sys_Run_Para.g_GPS_Mode = DataCut[0];
        Serial.println("GPS_mode: "  + DataCut);

        //如果定位标志位为1，获取定位信息
        if (Sys_Run_Para.g_Location_Flag == 1){
          if (Sys_Run_Para.g_LBS_Connect_Seriver_Flag == false){
            if (modem.LBS_Connect() == true){
              Serial.println("LBS Connect OK!");
              Sys_Run_Para.g_LBS_Connect_Seriver_Flag = true;

            }else{
              Serial.println("LBS Connect Fail!");
              Sys_Run_Para.g_LBS_Connect_Seriver_Flag = false;
            }
          }

          if (Sys_Run_Para.g_LBS_Connect_Seriver_Flag == true){
            if (Sys_Run_Para.g_GPS_Mode == 'L'){
              String LOCData; 
              LOCData = modem.Get_LOCData();
              Serial.println("LBS Data:" + LOCData);  
              Sys_Run_Para.g_LBS_Connect_Seriver_Flag = false;
              lbs_parse(LOCData, &g_GPS_Store_Data);

            }else if (Sys_Run_Para.g_GPS_Mode == 'G'){
                String GPSData = modem.GPS_Get_Data();
                Serial.println("GPS Data:" + GPSData);
                gps_parse(GPSData, &g_G_GPS_Data);    
                Sys_Run_Para.g_LBS_Connect_Seriver_Flag = false;
            }
          }
        }

        //将服务器给的参数存到EEPROM中
        if (Verify_Sys_Para())
          Save_Param_to_EEPROM();

        Sys_Run_Para.g_Recive_Param_Flag = true;
      }
    }
  }
}

bool Pre_Access_Network(void)
{
  //开启GPRS模块电源
  Serial.println("Initializing GSM modem...");
  delay(100);
  GPRS_PWR_ON;
  //GPS_ANT_PWR_ON;
  delay(100);
  LED2_ON;
  SIM800_PWR_CON();
  LED2_OFF;
  modem.restart();

  //准备注册到网络
  Serial.print("Waiting for network...  ");
  delay(100);
  //如果等待网络连接失败
  if (!modem.waitForNetwork()){
    Serial.println("Connect NetWork Fail, prepare to sleep! <Pre_Access_Network>");
    if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
      Muti_Sensor_Data_Base_Init();
      Save_SensorData_to_EEPROM();
    }
    return false;
  }
  //如果连接网络成功
  else{
    Serial.println("OK");
    Serial.print("Connecting to ");
    Serial.println(apn);
    //如果入网失败
    if (!modem.gprsConnect(apn, user, pass)){
      Serial.println("Access to the network failed, prepare to sleep! <Pre_Access_Network>");
      if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
        Muti_Sensor_Data_Base_Init();
        Save_SensorData_to_EEPROM();
      }
      return false;
    }
    //如果入网成功
    else
      Print_GSM_Card_Info();
  }
  return true; 
}

bool Connect_to_The_Server(void)
{
  Serial.print("Connecting to ");
  Serial.print(server); Serial.print(" : ");  Serial.println(port);
  //如果连接服务器失败
  if (!client.connect(server, port)){
    Serial.println("Connect to server failed, prepare reboot! <Connect_to_The_Server>");
    if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
      Muti_Sensor_Data_Base_Init();
      Save_SensorData_to_EEPROM();
    }
    nvic_sys_reset();
  }
  //如果连接服务器成功
  else{
    Serial.println("Connect Sevice OK...");
    GSM_STATUS_LED_OFF;
    Server_STATUS_LED_ON;

    Read_EEPROM_Server_Param(SysHostID, &Sys_Run_Para); //读取本机系统相关参数
    if(!Is_Apply_For_ID()) return false;

    //准备向服务器发送握手帧
    unsigned char Hand_Shake_Frame[128];
    unsigned char Frame_Length = 0;

    Hand_Shake_Frame[Frame_Length++] = 0xFE;
    Hand_Shake_Frame[Frame_Length++] = 0xC0;
    Hand_Shake_Frame[Frame_Length++] = 0x09;

    Read_EEPROM_Server_Param(SysHostID, &Sys_Run_Para);

    Hand_Shake_Frame[Frame_Length++] = SysHostID[0];
    Hand_Shake_Frame[Frame_Length++] = SysHostID[1];
    Hand_Shake_Frame[Frame_Length++] = SysHostID[2];
    Hand_Shake_Frame[Frame_Length++] = SysHostID[3];

    #if TYPE06
    Hand_Shake_Frame[Frame_Length++] = WEATHER_GREENHOUSE_V2;
    #elif TYPE07
    Hand_Shake_Frame[Frame_Length++] = AIR_V2;
    #endif

    Hand_Shake_Frame[Frame_Length++] = Com_PWD[0];
    Hand_Shake_Frame[Frame_Length++] = Com_PWD[1];
    Hand_Shake_Frame[Frame_Length++] = Com_PWD[2];
    Hand_Shake_Frame[Frame_Length++] = Com_PWD[3];

    unsigned char CRC8 = GetCrc8(&Hand_Shake_Frame[3], Frame_Length - 3);

    Hand_Shake_Frame[Frame_Length++] = CRC8; //CRC8校验码
    Hand_Shake_Frame[Frame_Length++] = 0x0D;

    unsigned long timeout = millis();

    do{
      //如果网络还在连接，发送握手帧
      if (client.connected()){
        client.write(Hand_Shake_Frame, Frame_Length);
        delay(1000);
      }else{
        Serial.println("Network disconnected, prepare reboot! <Connect_to_The_Server>");
        if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
          Muti_Sensor_Data_Base_Init();
          Save_SensorData_to_EEPROM();
        }
        client.stop();
        nvic_sys_reset();
      }
      //接收服务器发来的各项参数。
      Receive_Sys_Param_From_Server();

    } while ((Sys_Run_Para.g_Recive_Param_Flag != true) && (millis() - timeout < 15000L));
  }
}

bool Send_Data_To_Server(void)
{
    if(client.connected()){    

      //读取EEPROM中的传感器数据和发送数据
      if(Send_EEPROM_Muti_Sensor_Data_to_Server())
        return true;
      else
        return false;

    }else{
      Serial.println("Network disconnected! <Send_Data_To_Server>");
      if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
        Muti_Sensor_Data_Base_Init();
        Save_SensorData_to_EEPROM();
      }
      client.stop();
      return false;
    }
}