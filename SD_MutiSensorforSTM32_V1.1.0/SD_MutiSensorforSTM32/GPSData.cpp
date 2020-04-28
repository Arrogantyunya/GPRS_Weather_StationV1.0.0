#include "GPSData.h"

 /**
  *@brief  将GPS世界时间转换为北京时间
  *@param [in] GPS gps数据流的指针
  *@return None
  */
static void UTC2BTC(date_time *GPS)
{
   //如果秒号先出,再出时间数据,则将时间数据+1秒
   GPS->seconds++; //加一秒
   if(GPS->seconds>59){
      GPS->seconds=0;
      GPS->minutes++;
      if(GPS->minutes>59){
        GPS->minutes=0;
        GPS->hour++;
      }
  } 
  GPS->hour+=8;   //北京时间与世界时间相差8个时区，即相差8个钟
  if(GPS->hour>23)
  {
     GPS->hour-=24;
     GPS->day+=1;
     if(GPS->month==2 || GPS->month==4 || GPS->month==6 || GPS->month==9 || GPS->month==11 ){
        if(GPS->day>30){
           GPS->day=1;
           GPS->month++;
        }
     }
     else{
        if(GPS->day>31){
           GPS->day=1;
           GPS->month++;
        }
   }
   if((GPS->year % 4 ==0) && (GPS->year % 400 == 0 || GPS->year % 100 != 0)){  //判断闰年
       if(GPS->day > 29 && GPS->month ==2){   //闰年二月比平年二月多一天
          GPS->day=1;
          GPS->month++;
       }
   }
   else{
       if(GPS->day>28 &&GPS->month ==2){
         GPS->day=1;
         GPS->month++;
       }
   }
   if(GPS->month>12){
      GPS->month-=12;
      GPS->year++;
   }  
  }
}
/**
  *@brief  解释gps发出的数据流，并将解析数据放入数据结构中
  *@param [in] line gps数据流的指针
  *@param [in] GPS GPS数据结构的指针
  *@return None
  *@note    
  *  +CLBS: 0,102.655042,25.065250,550,17/06/21,09:29:08
  */
void lbs_parse(String line, GPS_INFO *GPS)
{
  String strTemp;
  char str[30] = {0};

  if (line[0] == 'N' && line[1] == 'O'){
    return;
  }

  strTemp = (line.substring(0, line.indexOf(",")));
  int Parameter1 = strTemp.toInt();
  //获取经度
  line = line.substring(line.indexOf(",") + 1); 
  if (line[0] == '-'){
    GPS->longitude_flag = 1;
    strTemp = (line.substring(1, line.indexOf(",")));
  }else{
    GPS->longitude_flag = 0;
    strTemp = (line.substring(0, line.indexOf(",")));
  }
  strTemp.toCharArray(GPS->longitudestr, strTemp.length() + 1);
  GPS->longitude = strTemp.toFloat();   

  //获取纬度
  line = line.substring(line.indexOf(",") + 1);  
  if (line[0] == '-'){
    GPS->latitude_flag = 1;
    strTemp = (line.substring(1, line.indexOf(",")));
  }else{
    GPS->latitude_flag = 0;
    strTemp = (line.substring(0, line.indexOf(",")));
  }
  //strTemp = (line.substring(0, line.indexOf(",")));
  strTemp.toCharArray(GPS->latitudestr, strTemp.length() + 1);
  GPS->latitude = strTemp.toFloat();   

  //获取定位半径，精度
  line = line.substring(line.indexOf(",") + 1);
  strTemp = (line.substring(0, line.indexOf(",")));
  int PosRadius = strTemp.toInt();
  GPS->ACC = PosRadius;

  //获取日期字符串
  line = line.substring(line.indexOf(",") + 1);
  strTemp = (line.substring(0, line.indexOf(",")));
  char Datebuf[10] = {0};
  strTemp.toCharArray(Datebuf, 9);
  GPS->D.year = 2000 + (Datebuf[0] - '0') * 10 + (Datebuf[1] - '0');
  GPS->D.month = (Datebuf[3] - '0') * 10 + (Datebuf[4] - '0');
  GPS->D.day = (Datebuf[6] - '0') * 10 + (Datebuf[7] - '0');

  //获取时间字符串
  line = line.substring(line.indexOf(",") + 1);
  strTemp = (line.substring(0, line.indexOf(",")));
  char Timebuf[9] = {0};
  strTemp.toCharArray(Timebuf, 9);
  GPS->D.hour = (Timebuf[0] - '0') * 10 + (Timebuf[1] - '0');
  GPS->D.minutes = (Timebuf[3] - '0') * 10 + (Timebuf[4] - '0');
  GPS->D.seconds = (Timebuf[6] - '0') * 10 + (Timebuf[7] - '0');

  UTC2BTC(&GPS->D);   
}

bool gps_parse(String gps, GPS_Dat *format_gps)
{
  //GPS Data:1,1,20180717061922.000,25.051970,102.652083,1866.774,0.81,16.1,1,,1.9,2.1,1.0,,7,5,,,46,,    
  String str_temp;
  unsigned int i;
  bool negative_number_flag = false;

  if (gps[0] == '1' && gps[2] == '1')
  {
    gps = gps.substring(gps.indexOf(",", 0) + 1);//去除开头的1,
    gps = gps.substring(gps.indexOf(",", 0) + 1);//去除第二个1,从年月日重新开头

    /*格式日期*/
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    char TimeBuff[14];
    str_temp.toCharArray(TimeBuff, str_temp.length() + 1);
    for (i = 0; i < 14; i++)
    {
      TimeBuff[i] -= '0';
    }
    format_gps->GPS_time.year = TimeBuff[0] * 1000 + TimeBuff[1] * 100 + TimeBuff[2] * 10 + TimeBuff[3];//2018
    format_gps->GPS_time.month = TimeBuff[4] * 10 + TimeBuff[5];
    format_gps->GPS_time.day = TimeBuff[6] * 10 + TimeBuff[7];
    format_gps->GPS_time.hour = TimeBuff[8] * 10 + TimeBuff[9];
    format_gps->GPS_time.minutes = TimeBuff[10] * 10 + TimeBuff[11];
    format_gps->GPS_time.seconds = TimeBuff[12] * 10 + TimeBuff[13];

    /*格式纬度*/
    gps = gps.substring(gps.indexOf(",", 0) + 1);//去掉日期，字符串重新从纬度开始
    if (gps[0] == '-')
    {
      format_gps->GPS_latitude_flag = 1;
      str_temp = gps.substring(1, gps.indexOf(",", 0));
    }
    else
    {
      format_gps->GPS_latitude_flag = 0;
      str_temp = gps.substring(0, gps.indexOf(",", 0));
    }
    str_temp.toCharArray(format_gps->GPS_latitude_str, str_temp.length() + 1);
    format_gps->GPS_latitude = str_temp.toFloat();

    /*格式经度*/
    gps = gps.substring(gps.indexOf(",", 0) + 1);
    if (gps[0] == '-')
    {
      format_gps->GPS_longitude_flag = 1;
      str_temp = gps.substring(1, gps.indexOf(",", 0));
    }
    else
    {
      format_gps->GPS_longitude_flag = 0;
      str_temp = gps.substring(0, gps.indexOf(",", 0));
    }
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    str_temp.toCharArray(format_gps->GPS_longitude_str, str_temp.length() + 1);
    format_gps->GPS_longitude = str_temp.toFloat();

    /*格式海拔*/
    gps = gps.substring(gps.indexOf(",", 0) + 1);
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    str_temp.toCharArray(format_gps->GPS_altitude_str, str_temp.length() + 1);
    format_gps->GPS_altitude = str_temp.toFloat();

    /*格式地面速度*/
    gps = gps.substring(gps.indexOf(",", 0) + 1);
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    str_temp.toCharArray(format_gps->GPS_Speed_str, str_temp.length() + 1);
    format_gps->GPS_Speed = str_temp.toFloat();

    /*格式航向角*/
    gps = gps.substring(gps.indexOf(",", 0) + 1);
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    str_temp.toCharArray(format_gps->GPS_angle_str, str_temp.length() + 1);
    format_gps->GPS_angle = str_temp.toFloat();

    for (i = 0; i < 8; i++)
    {
      gps = gps.substring(gps.indexOf(",", 0) + 1);//排除卫星数量前面的七个数据
    }

    /*格式卫星数量*/
    str_temp = gps.substring(0, gps.indexOf(",", 0));
    format_gps->GPS_satellites_used = (char)str_temp.toInt();

    UTC2BTC(&format_gps->GPS_time);  

    return true;
  }
  else
  {
    return false;
  }
}