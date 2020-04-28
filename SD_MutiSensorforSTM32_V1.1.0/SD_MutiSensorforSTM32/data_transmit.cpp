#define TINY_GSM_MODEM_SIM800
#include "GSM/TinyGsmClient.h"
#include "data_transmit.h"
#include "BCD_CON.h"
#include "User_CRC8.h"
#include "Private_Sensor.h"
#include "Periph.h"
#include "memory.h"
#include "Private_RTC.h"
#include "Ope_SD.h"

extern TinyGsm modem;
extern TinyGsmClient client;
extern unsigned int Run_Time_Out_Sec;

//连接到服务器相关变量
const char nwk_apn[] = "CMIOT";
const char sms_apn[] = "CMNET";
const char user[] = "";
const char pass[] = "";
const char server[] = "118.25.4.217";
int port = 6969;

const char SMS_PARAM[] = "#SMSPARAM:";
const char AT_DELETE_ALL[] = "AT+CMGD=1,4\r\n";
const char AT_DELETE_[] = "AT+CMGD=";
const char AT_READ_INDEX_SMS[] = "AT+CMGR=";

System_Run_Parameter Sys_Run_Para;

unsigned char Com_PWD[4] = {0x1A, 0xC4, 0xEE, 0x0B};  //握手帧密码
unsigned char SysHostID[4] = {0};                     //设备ID
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

bool LED_For_Network_Flag = false;
bool Pre_Network_LED_Status = false;
bool Is_Access_Server_Status = false;

bool Quick_Work_Flag = false;

void Get_Trg_String(char *buf, char trg_char)
{
  unsigned int Index = 0;
  unsigned int Ope_Index = 0;
  while (1){
    if(buf[Index] == '\0' || buf[Index] == trg_char)
      break;
    Index++;
  }
  Index++;
  while (buf[Index] != '\0'){
    buf[Ope_Index++] = buf[Index++];
  }
  memset(&buf[Ope_Index], 0x00, sizeof(buf));
}

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

  #elif TYPE09
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0xCE;
  Send_Air_Sensor_Buff[Air_Data_Length++] = 0x39;
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
  #elif TYPE09
  Send_Air_Sensor_Buff[Air_Data_Length++] = AIR_V4;
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
  #endif

  #if (TYPE06 || TYPE07 || TYPE09)
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
  
  #if (TYPE07 || TYPE09)
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

  #if TYPE09
  Send_Air_Sensor_Buff[Air_Data_Length++] = SD_Ope.Get_StoreStatus();
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
  unsigned char CRC8 = GetCrc8(&Send_Air_Sensor_Buff[3], Air_Data_Length - 3);
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

    EP_Write_Enable();

    EpromDb.open(EEPROM_BASE_ADDR);
    EpromDb.clear();  //发送完成后，清除EEPROM数据

    EP_Write_Disable();
  }

  // time_t Now_Time = 0;
  // Now_Time = InRtc.getTime();

  // if (Sys_Run_Para.g_Transmit_Cycle < 60 || Sys_Run_Para.g_Transmit_Cycle > 43200)
  //   Sys_Run_Para.g_Transmit_Cycle = 180;

  // Now_Time += Sys_Run_Para.g_Transmit_Cycle; 
  // InRtc.setAlarmTime(Now_Time); //设置RTC闹钟

  interrupts();

  NowRecord = 1;
  Save_Sys_Current_Record(NowRecord);
  
  client.stop();
  return true;
}       

void Print_GSM_Card_Info(void)
{
  RED1_OFF;
  int CSQ = 0;
  CSQ = modem.getSignalQuality(); //得到信号强度

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

bool Set_RunMode(char *runmode_buffer)
{
  switch (runmode_buffer[0]){
    case 'N' : runmode_buffer[3] == '_' ? Sys_Run_Para.g_RunMode = NWK_TF : Sys_Run_Para.g_RunMode = NWK;  return true; break;
    
    case 'S' : runmode_buffer[3] == '_' ? Sys_Run_Para.g_RunMode = SMS_TF : Sys_Run_Para.g_RunMode = SMS; return true; break;

    case 'T' : Sys_Run_Para.g_RunMode = TF; return true; break;

    default  : Sys_Run_Para.g_RunMode = DEFAULT_RUNMODE; return false; break;
  }
}

bool Send_SMS_Receipt(char *transmit_cycle, char *run_mode)
{
  char Send_Phone_Number[15] = {0};
  char Send_Param_Receipt[64] = {0};
  unsigned char Receipt_Len = 0;
  Send_Phone_Number[0] = '+';
  Send_Phone_Number[1] = '8';
  Send_Phone_Number[2] = '6';

  Get_SYS_SMS_Phone_Number(Sys_Run_Para.g_SMS_Phone);
  
  for (unsigned char i = 0; i < 11; i++)
    Send_Phone_Number[i + 3] = Sys_Run_Para.g_SMS_Phone[i];

  Serial.println("Prepare Send param receipt to user...");
  Serial.print("Phone number is : "); Serial.write(Send_Phone_Number); Serial.println();
  Serial.flush();

  Send_Param_Receipt[Receipt_Len++] = 'S';
  Send_Param_Receipt[Receipt_Len++] = 'e';
  Send_Param_Receipt[Receipt_Len++] = 't';
  Send_Param_Receipt[Receipt_Len++] = ':';
  Send_Param_Receipt[Receipt_Len++] = ' ';
  Send_Param_Receipt[Receipt_Len++] = 'O';
  Send_Param_Receipt[Receipt_Len++] = 'K';
  Send_Param_Receipt[Receipt_Len++] = '\r';
  Send_Param_Receipt[Receipt_Len++] = '\n';

  for (unsigned char i = 0; transmit_cycle[i] != '\0'; i++)
    Send_Param_Receipt[Receipt_Len++] = transmit_cycle[i];

  Send_Param_Receipt[Receipt_Len++] = ',';

  for (unsigned char i = 0; run_mode[i] != '\0'; i++)
    Send_Param_Receipt[Receipt_Len++] = run_mode[i];
  
  modem.sendSMS(Send_Phone_Number, Send_Param_Receipt);
  delay(200);
  return true;
}

bool Analysis_SMS_Param(char *data)
{
  //#SMSPARAM:90,NWK
  char TransmitCycle_Buffer[10] = {0};
  char Run_Mode_Temp[7] = {0};
  unsigned char Len = 0;
  for (unsigned char i = 0; i < 10; i++){
    if (data[i] != SMS_PARAM[i]){
      Serial.println("Prameters frame head ERROR!");
      return false;
    }
  }

  for (unsigned int i = 0; ; i++){
    if (data[i] == ',')
      break;
    if (data[i] == '\0'){
      Serial.println("Parameter no , ERROR!");
      return false;
    }
  }

  //Get Transmit Cycle.
  while (data[Len + 10] != ','){
    if (data[Len + 10] < '0' || data[Len + 10] > '9'){
      Serial.println("Transmit Cycle ERROR !");
      return false;
    }
    TransmitCycle_Buffer[Len] = data[Len + 10];
    Len++;
    if (Len > 5){
      return false;
    }
  }

  if (Len == 0)
    return false;

  switch (Len){
    case 1 : Sys_Run_Para.g_Transmit_Cycle = TransmitCycle_Buffer[0] - '0'; break;
    case 2 : Sys_Run_Para.g_Transmit_Cycle = ((TransmitCycle_Buffer[0] - '0') * 10 + (TransmitCycle_Buffer[1] - '0')); break;
    case 3 : Sys_Run_Para.g_Transmit_Cycle = ((TransmitCycle_Buffer[0] - '0') * 100 + (TransmitCycle_Buffer[1] - '0') * 10 + (TransmitCycle_Buffer[2] - '0')); break;
    case 4 : Sys_Run_Para.g_Transmit_Cycle = ((TransmitCycle_Buffer[0] - '0') * 1000 + (TransmitCycle_Buffer[1] - '0') * 100 + (TransmitCycle_Buffer[2] - '0') * 10 + (TransmitCycle_Buffer[3] - '0')); break;
    case 5 : Sys_Run_Para.g_Transmit_Cycle = ((TransmitCycle_Buffer[0] - '0') * 10000 + (TransmitCycle_Buffer[1] - '0') * 1000 + (TransmitCycle_Buffer[2] - '0') * 100 + (TransmitCycle_Buffer[3] - '0') * 10 + (TransmitCycle_Buffer[4] - '0')); break;
  }
  TransmitCycle_Buffer[9] = 0;
  Serial.print("Transmit_Cycle:");
  Serial.println(Sys_Run_Para.g_Transmit_Cycle);

  if (Sys_Run_Para.g_Transmit_Cycle < 60 || Sys_Run_Para.g_Transmit_Cycle > 43200)
    Sys_Run_Para.g_Transmit_Cycle = 180;

  //Get Run Mode
  Get_Trg_String(data, ',');


  for (unsigned char i = 0; data[i] != '\r'; i++){
    if (i >= 7) return false;
    Run_Mode_Temp[i] = data[i];
  }

  Run_Mode_Temp[6] = 0;

  for (unsigned char i = 0; Run_Mode_Temp[i] != '\0'; i++){
    if ((Run_Mode_Temp[i] >= 'A' && Run_Mode_Temp[i] <= 'Z') || Run_Mode_Temp[i] == '_'){
      ;
    }else{
      Serial.println("SMS Run mode param ERROR!");
      return false;
    }
  }

  if(!Set_RunMode(Run_Mode_Temp)){
    Serial.println("Run Mode inexistence!");
    return false;
  }

  Serial.print("Run_Mode:");
  Serial.write(Run_Mode_Temp);

  Serial.println("SMS Parameters Set OK...");
  Save_SMS_Param_to_EEPROM();
  Send_SMS_Receipt(TransmitCycle_Buffer, Run_Mode_Temp);
  
  return true;
}

void Prepare_SMS(void)
{
  char Rcv_Data[200] = {0};
  unsigned char Rcv_Len = 0;
  GSM_Serial.write("AT+CNMI?\r\n");
  delay(5000);
  while (GSM_Serial.available() > 0){
    if (Rcv_Len >= 200) break;
    Rcv_Data[Rcv_Len++] = GSM_Serial.read();
  }
  if (Rcv_Len > 0){
    Rcv_Len = 0;
    Serial.write(Rcv_Data);
  }
}

bool Wait_Receive_SMS(void)
{
  char Rcv_Data[200] = {0};
  unsigned char Rcv_Len = 0;
  unsigned int Max_TimeOut = Run_Time_Out_Sec + 25;

  while (Run_Time_Out_Sec <= Max_TimeOut){
  while (GSM_Serial.available() > 0){
    if (Rcv_Len >= 200) return false;
      Rcv_Data[Rcv_Len++] = GSM_Serial.read();
    }
    if (Rcv_Len > 0){
      Rcv_Len = 0;
      Serial.write(Rcv_Data);
      return true;
    }
  }  
  Serial.println("No Respond for parameter.");
  return true;
}

unsigned char Preferred_SMS(void)
{
  char Rcv_Data[200] = {0};
  unsigned int Rcv_Len = 0;
  bool Rcv_Flag = false;
  unsigned long TimeOut;

  GSM_Serial.write("AT+CPMS?\r\n");
  TimeOut = millis();
  while (millis() <= (TimeOut + 10000)){
    while (GSM_Serial.available() > 0){
      if (Rcv_Len >= 200) return 0xFF;
      Rcv_Data[Rcv_Len++] = GSM_Serial.read();
      if (Rcv_Len > 2){
        if (Rcv_Data[Rcv_Len - 2] == '\r' && Rcv_Data[Rcv_Len - 1] == '\n'){
          Rcv_Flag = true;
          break;
        }
      }
    }
    if (Rcv_Flag) break;
  }

    if (Rcv_Len > 0){
      Rcv_Len = 0;
      Serial.write(Rcv_Data);
      /* rcv_buffer[0] = '\r', rcv_buffer[1] = '\n' */
      if (Rcv_Data[2] == 'E' && Rcv_Data[3] == 'R' && Rcv_Data[4] == 'R'){
        return 0xFF;
      }else{
        Get_Trg_String(Rcv_Data, ','); 
        Rcv_Data[0] -= '0';
        if (Rcv_Data[1] != ','){
          Rcv_Data[1] -= '0';
          if ((Rcv_Data[0] >= 0 && Rcv_Data[0] <= 50) && (Rcv_Data[1] >= 0 && Rcv_Data[1] <= 50))
            return (Rcv_Data[0] * 10 + Rcv_Data[1]);
          else
          return 0xFF;
        }else{
          if (Rcv_Data[0] >= 0 && Rcv_Data[0] <= 50){
            return Rcv_Data[0];
          }else{
            return 0xFF;
          }
        }
      }
    }
  return 0xFF;
}

bool Set_SMS_TextMode(void)
{
  char Rcv_Data[100] = {0};
  unsigned char Rcv_Len = 0;
  unsigned long TimeOut;
  unsigned char Rcv_Num = 0;

  GSM_Serial.write("AT+CMGF=1\r\n");
  delay(3000);
  //TimeOut = millis();
  //while (millis() <= TimeOut + 10000){
    while (GSM_Serial.available() > 0){
      if (Rcv_Len >= 100) return false;
      Rcv_Data[Rcv_Len++] = GSM_Serial.read();
      // if (Rcv_Len > 2){
      //   if (Rcv_Data[Rcv_Len - 2] == '\r' && Rcv_Data[Rcv_Len - 1] == '\n'){
      //     Rcv_Num++;
      //     if (Rcv_Num == 2) break;
      //   }
      // }
    }
    //if (Rcv_Num == 2) break;
  //}

  if (Rcv_Len > 0){
    Rcv_Len = 0;
    if (Rcv_Data[2] == 'O' && Rcv_Data[3] == 'K')
      return true;
  }
  return false;  
}

bool Read_Latest_SMS(unsigned char trg_sms)
{
  char Rcv_Data[512] = {0};
  char Read_SMS_Buff[20] = {0};
  unsigned int Rcv_Len = 0;
  bool Rcv_Flag = 0;
  unsigned char i = 0;
  unsigned long TimeOut;

  for (; i < 8; i++)
    Read_SMS_Buff[i] = AT_READ_INDEX_SMS[i];

  if (trg_sms > 0 && trg_sms <= 50){
    if (trg_sms / 10 > 0){
      Read_SMS_Buff[i++] = (trg_sms / 10) - 1 + '0';
      Read_SMS_Buff[i++] = trg_sms % 10 + '0';
    }else{
      Read_SMS_Buff[i++] = trg_sms + '0';
    }
  }else{
    Serial.println("trg_sms over 50!");
    return false;
  }

  Read_SMS_Buff[i++] = ',';
  Read_SMS_Buff[i++] = '0';
  Read_SMS_Buff[i++] = '\r';
  Read_SMS_Buff[i++] = '\n';

  GSM_Serial.write(Read_SMS_Buff);
  TimeOut = millis();
  while (millis() <= (TimeOut + 15000)){
    while (GSM_Serial.available() > 0){
      if (Rcv_Len >= 512){
        Serial.println("Received data over length!");
        return false;
      }
      Rcv_Data[Rcv_Len++] =  GSM_Serial.read();
      if (Rcv_Len > 2){
        if (Rcv_Data[Rcv_Len - 4] == 'O' && Rcv_Data[Rcv_Len - 3] == 'K' && Rcv_Data[Rcv_Len - 2] == '\r' && Rcv_Data[Rcv_Len - 1] == '\n'){
          Rcv_Flag = true;
          break;
        }
      }
    }
    if (Rcv_Flag == true) break;
  }

  if (Rcv_Len > 0){
    Rcv_Len = 0;

    if (Rcv_Data[3] == 'C' && Rcv_Data[4] == 'M' && Rcv_Data[5] == 'G' && Rcv_Data[6] == 'R'){
      Get_Trg_String(Rcv_Data, ',');
      if (Rcv_Data[1] == '+' && Rcv_Data[2] == '8' && Rcv_Data[3] == '6'){
        Get_Trg_String(Rcv_Data, '6');  //+86.....
        char Phone_Temp[11];
        bool Verify_Flag = true;
        Get_SYS_SMS_Phone_Number(Phone_Temp);
        for (unsigned char i = 0; i < 11; i++){
          if (Rcv_Data[i] != Phone_Temp[i]){
            Serial.println("Verify phone number failed!");
            return false;
          }
        }
      }else{
        Serial.println("Not 11 phone number !");
        return false;
      }
      Get_Trg_String(Rcv_Data, '\n');
      Serial.write(Rcv_Data);
      Serial.println("------------------------------");
      if(!Analysis_SMS_Param(Rcv_Data))
        return false;
      
    }else{
      Serial.println("Frame head Err!");
      Serial.flush();
      return false;
    }
    return true;
  }
  Serial.println("Read SMS no respond!");
  return false;  
}

bool Delete_All_SMS(unsigned char sms_num)
{
  char Rcv_Data[100] = {0};
  unsigned char Rcv_Len = 0;

  GSM_Serial.write(AT_DELETE_ALL);
  delay(10000);
  while (GSM_Serial.available() > 0){
    if (Rcv_Len >= 100) return false;
    Rcv_Data[Rcv_Len++] = GSM_Serial.read();
  }

  if (Rcv_Len > 0){
    Rcv_Len = 0;
    if (Rcv_Data[2] != 'O' && Rcv_Data[3] != 'K'){
      return false;
    }
  }
  return true;
}

// bool Delete_All_SMS(unsigned char sms_num)
// {
//   char Rcv_Data[100] = {0};
//   unsigned char Rcv_Len = 0;
//   char Delete_Cmd[15] = {0};

//   for (unsigned char i = 0; i < 8; i++)
//     Delete_Cmd[i] = AT_DELETE_[i];

//   Serial.print("All number is :");
//   Serial.println(sms_num);

//   for (unsigned char i = sms_num; i >= 1; i--){
//     if (i >= 10){
//       Delete_Cmd[8] = (i / 10) + '0';
//       Delete_Cmd[9] = (i % 10) + '0';
//       Delete_Cmd[10] = ',';
//       Delete_Cmd[11] = '0';
//       Delete_Cmd[12] = '\r';
//       Delete_Cmd[13] = '\n';
//     }else{
//       Delete_Cmd[8] = i + '0';
//       Delete_Cmd[9] = ',';
//       Delete_Cmd[10] = '0';
//       Delete_Cmd[11] = '\r';
//       Delete_Cmd[12] = '\n';
//       Delete_Cmd[13] = 0;
//     }
//     GSM_Serial.write(Delete_Cmd);
//     delay(5000);
//     while (GSM_Serial.available() > 0){
//       if (Rcv_Len >= 100) return false;
//       Rcv_Data[Rcv_Len++] = GSM_Serial.read();
//     }

//     if (Rcv_Len > 0){
//       Rcv_Len = 0;
//       if (Rcv_Data[2] != 'O' && Rcv_Data[3] != 'K'){
//         return false;
//       }else{
//         Serial.print("Delete ");
//         Serial.print(i);
//         Serial.println(" OK.");
//       }
//     }
//   }
//   return true;
// }

void Read_SMS_Setting_From_User(void)
{
  unsigned char Latest_SMS;
  bool Status_Flag = false;
  unsigned char Try_Num = 0;

  //Prepare_SMS();
  Wait_Receive_SMS();

  do{
    Latest_SMS = Preferred_SMS();
    Try_Num++;
    delay(1000);
  }while (Latest_SMS > 50 && Try_Num < 10);

  Serial.print("SMS number: ");
  Serial.println(Latest_SMS);
  Try_Num = 0;

  if (Latest_SMS == 0){
    Serial.println("SMS is empty.");

  }else if (Latest_SMS > 50){
    Serial.println("Read SMS Err!");

  }else{
    Serial.print("Set SMS TextMode... ");
    do{
      Status_Flag = Set_SMS_TextMode();
      Try_Num++;
      delay(500);
    }while (!Status_Flag && Try_Num < 10);
    Status_Flag == true ? Serial.println("OK") : Serial.println("Failed");
    Try_Num = 0;

    if (Status_Flag){
      Serial.println("Read latest SMS... ");
      //do{
        Status_Flag = Read_Latest_SMS(Latest_SMS);
        //Try_Num++;
        //delay(500);
      //}while (!Status_Flag && Try_Num < 10);
      //Try_Num = 0;
    }

    Serial.println("Delete All SMS... ");
    Status_Flag = Delete_All_SMS(Latest_SMS);
    Status_Flag == true ? Serial.println("OK") : Serial.println("Failed");
  }
  modem.gprsDisconnect();
  delay(500);
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
        bkp_enable_writes();
        //Delay_ms(10);
        UTCTime CurrentSec = osal_ConvertUTCSecs(&RtcTime);
        InRtc.setTime(CurrentSec);
        // Delay_ms(10);
        // bkp_disable_writes();

        //判断是否该获取定位信息
        if (RtcTime.hour == 0 && (RtcTime.minutes >= 0 && RtcTime.minutes <= 0) )
          Sys_Run_Para.g_Location_Flag = 1;
        else
          Sys_Run_Para.g_Location_Flag = 0;

        //打印出从服务器保存的RTC，确保保存正确
        CurrentSec = InRtc.getTime();
        osal_ConvertUTCTime(&RtcTime, CurrentSec);
        char str[100] = {0};
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
        memset(Sys_Run_Para.g_RunMode_Ascll, 0x00, sizeof(Sys_Run_Para.g_RunMode_Ascll));
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, DataCut.indexOf(","));
        Serial.println("Run mode: " + Str_Temp);
        strcpy(Sys_Run_Para.g_RunMode_Ascll, Str_Temp.c_str());
        Set_RunMode(Sys_Run_Para.g_RunMode_Ascll);

        //获取定位模式参数
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        Str_Temp = DataCut.substring(0, 1);
        Sys_Run_Para.g_GPS_Mode = Str_Temp[0];
        Serial.println("GPS_mode: "  + Str_Temp);

        //获取发送短信指定电话号码
        memset(Sys_Run_Para.g_SMS_Phone, 0x00, sizeof(Sys_Run_Para.g_SMS_Phone));
        DataCut = DataCut.substring(DataCut.indexOf(",") + 1);
        if (DataCut.length() == 11)
          Str_Temp = DataCut.substring(0, 11);
        Serial.println("SMS Phone: " + DataCut);
        strcpy(Sys_Run_Para.g_SMS_Phone, DataCut.c_str());

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
  RED1_ON;  
  //开启GPRS模块电源
  Serial.println("Initializing GSM modem...");
  Serial.flush();
  GPRS_PWR_ON;
  while (digitalRead(GPRS_PWR_CON_PIN) == LOW);
  //GPS_ANT_PWR_ON;
  SIM800_PWR_CON();
  modem.restart();
  delay(500);

  //准备注册到网络
  Serial.print("Waiting for network...  ");
  Serial.flush();
  //noInterrupts();
  //如果等待网络连接失败
  if (!modem.waitForNetwork()){
    Serial.println("Connect NetWork Fail, prepare to sleep! <Pre_Access_Network>");
    Serial.flush();
    Quick_Work_Flag = true;
    if (Sys_Run_Para.g_RunMode == NWK || Sys_Run_Para.g_RunMode == NWK_TF){
      if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
        Muti_Sensor_Data_Base_Init();
        Save_SensorData_to_EEPROM();
      }
    }
    return false;
  }
  //如果附着网络成功
  else{
    RED2_ON;
    //interrupts();
    Serial.println("OK"); Serial.flush();
    Serial.print("Connecting to ");

    if (Sys_Run_Para.g_RunMode == NWK || Sys_Run_Para.g_RunMode == NWK_TF){
      Serial.println(nwk_apn);  Serial.flush();
      //如果入网失败
      if (!modem.gprsConnect(nwk_apn, user, pass)){
        Serial.println("Access to the network failed, prepare to sleep! <Pre_Access_Network>");
        Serial.flush();
        Quick_Work_Flag = true;
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
    else if (Sys_Run_Para.g_RunMode == SMS || Sys_Run_Para.g_RunMode == SMS_TF){
      Serial.println(sms_apn);  Serial.flush();
      //如果入网失败
      if (!modem.gprsConnect(sms_apn, user, pass)){
        Serial.println("Access to the network failed, prepare to sleep! <Pre_Access_Network>");
        Serial.flush();
        Quick_Work_Flag = true;
        return false;
      }else{
        Read_EEPROM_Server_Param(SysHostID, &Sys_Run_Para); //读取本机系统相关参数
      }
      RED1_OFF;    
    }
  }
  return true; 
}

bool Send_Message_to_User(void)
{
  RED2_OFF;
  GREEN2_ON;
  char Send_Phone_Number[15] = {0};
  Send_Phone_Number[0] = '+';
  Send_Phone_Number[1] = '8';
  Send_Phone_Number[2] = '6';

  Get_SYS_SMS_Phone_Number(Sys_Run_Para.g_SMS_Phone);
  
  for (unsigned char i = 0; i < 11; i++)
    Send_Phone_Number[i + 3] = Sys_Run_Para.g_SMS_Phone[i];

  Serial.println("Prepare Send message to user...");
  Serial.print("Phone number is : "); Serial.write(Send_Phone_Number); Serial.println();
  Serial.flush();

  unsigned int i = 0;
  while (g_SD_Sensor_Data[i] != '\0'){
    i++;
  }

  unsigned int vol_temp = Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES);
  g_SD_Sensor_Data[i++] = 'V';
  g_SD_Sensor_Data[i++] = 'o';
  g_SD_Sensor_Data[i++] = 'l';
  g_SD_Sensor_Data[i++] = ':';
  g_SD_Sensor_Data[i++] = ' ';

  if (vol_temp > 1000 && vol_temp < 10000){
    g_SD_Sensor_Data[i++] = (vol_temp / 1000) + '0';
    g_SD_Sensor_Data[i++] = (vol_temp / 1000 % 100) + '0';
    g_SD_Sensor_Data[i++] = (vol_temp / 1000 / 100 % 10) + '0';
    g_SD_Sensor_Data[i++] = (vol_temp % 10) + '0';
  }else{
    g_SD_Sensor_Data[i++] = 'N';
    g_SD_Sensor_Data[i++] = 'U';
    g_SD_Sensor_Data[i++] = 'L';
    g_SD_Sensor_Data[i++] = 'L';
  }

  g_SD_Sensor_Data[i++] = 'm';
  g_SD_Sensor_Data[i++] = 'V';
  g_SD_Sensor_Data[i++] = '\n';

  if (Sys_Run_Para.g_RunMode == SMS_TF){
    g_SD_Sensor_Data[i++] = 'T';
    g_SD_Sensor_Data[i++] = 'F';
    g_SD_Sensor_Data[i++] = ':';
    g_SD_Sensor_Data[i++] = ' ';
    unsigned char TF_Status = SD_Ope.Get_StoreStatus();
    if (TF_Status == 0x00){
      g_SD_Sensor_Data[i++] = 'O';
      g_SD_Sensor_Data[i++] = 'K';
    }else if (TF_Status == 0x01){
      g_SD_Sensor_Data[i++] = 'W';
      g_SD_Sensor_Data[i++] = 'a';
      g_SD_Sensor_Data[i++] = 'r';
      g_SD_Sensor_Data[i++] = 'n';
      g_SD_Sensor_Data[i++] = 'i';
      g_SD_Sensor_Data[i++] = 'n';
      g_SD_Sensor_Data[i++] = 'g';
    }else if (TF_Status == 0x02){
      g_SD_Sensor_Data[i++] = 'E';
      g_SD_Sensor_Data[i++] = 'R';
      g_SD_Sensor_Data[i++] = 'R';
      g_SD_Sensor_Data[i++] = 'O';
      g_SD_Sensor_Data[i++] = 'R';
  }
  g_SD_Sensor_Data[i++] = '\n';
  }

  g_SD_Sensor_Data[i++] = '0';
  g_SD_Sensor_Data[i++] = SysHostID[0] + '0';
  g_SD_Sensor_Data[i++] = '0';
  g_SD_Sensor_Data[i++] = SysHostID[1] + '0';
  g_SD_Sensor_Data[i++] = '0';
  g_SD_Sensor_Data[i++] = SysHostID[2] + '0';
  g_SD_Sensor_Data[i++] = '0';
  g_SD_Sensor_Data[i++] = SysHostID[3] + '0';
  
  if(modem.sendSMS(Send_Phone_Number, g_SD_Sensor_Data)){
    delay(200);
    return true;
  }else{
    Serial.println("Send SMS failed!");
    return false;
  }
}

bool Connect_to_The_Server(void)
{
  Serial.print("Connecting to ");
  Serial.print(server); Serial.print(" : ");  Serial.println(port);
  Serial.flush();
  //如果连接服务器失败
  if (!client.connect(server, port)){
    Serial.println("Connect to server failed, prepare reboot! <Connect_to_The_Server>");
    Serial.flush();
    if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
      Muti_Sensor_Data_Base_Init();
      Save_SensorData_to_EEPROM();
    }
    nvic_sys_reset();
  }
  //如果连接服务器成功
  else{
    GREEN1_ON;
    Serial.println("Connect Sevice OK...");
    Serial.flush();

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
    #elif TYPE08
    Hand_Shake_Frame[Frame_Length++] = AIR_V3;
    #elif TYPE09
    Hand_Shake_Frame[Frame_Length++] = AIR_V4;
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
        Serial.flush();
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
      RED2_OFF; 
      GREEN2_ON;

      //读取EEPROM中的传感器数据和发送数据
      if(Send_EEPROM_Muti_Sensor_Data_to_Server())
        return true;
      else
        return false;
    }else{
      Serial.println("Network disconnected! <Send_Data_To_Server>");
      Serial.flush();
      Quick_Work_Flag = true;
      if (Sys_Run_Para.g_Already_Save_Data_Flag != true){
        Muti_Sensor_Data_Base_Init();
        Save_SensorData_to_EEPROM();
      }
      client.stop();
      return false;
    }
}