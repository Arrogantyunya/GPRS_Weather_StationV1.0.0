#include "Private_Sensor.h"
#include "MODBUS_RTU_CRC16.h"
#include "board_config.h"
#include <Arduino.h>
#include "private_delay.h"
#include "Private_RTC.h"
#include "memory.h"
#include "data_transmit.h"
#include "Periph.h"

SENSOR_DATA Muti_Sensor_Data;

/*
 *brief   : ModBus协议采集土壤电导率和盐分
 *para    : 无符号整型的电导率参数、无符号整型的盐分参数、ModBus土壤传感器地址
 *return  : 无
 */
void Read_Cond_and_Salt(unsigned int *cond, unsigned int *sat, unsigned char address)
{
  unsigned char Cond_Salt_Cmd[8] = {0x01, 0x03, 0x00, 0x14, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned int Cond_temp = 0xFFFF, Salt_temp = 0xFFFF;
  unsigned char i = 0, j = 0;
  unsigned char Try_Collection_Num = 0;

  Cond_Salt_Cmd[0] = address;

  CRC16 = N_CRC16(Cond_Salt_Cmd, 6);
  Cond_Salt_Cmd[6] = CRC16 >> 8;
  Cond_Salt_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(Cond_Salt_Cmd, 8); 
      Delay_ms(g_Wait_Receive_Time);

      while (ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0;
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
        Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16){
          Cond_temp = (Receive_Data[3] << 8) | Receive_Data[4]; 
          Salt_temp = (Receive_Data[5] << 8) | Receive_Data[6]; 
        }

        if (Cond_temp == 0 || Salt_temp == 0){
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
        else
          break;

      }else
        Try_Collection_Num++;
    
  }while (Try_Collection_Num < 3);

  *cond = Cond_temp;
  *sat  = Salt_temp;
}

/*
 *brief   : ModBus协议采集C02和TVOC浓度
 *para    : 无符号整型的CO2参数、无符号整型的TVOC参数、ModBus百叶箱地址
 *return  : 无
 */
void Read_CO2_and_TVOC(unsigned int *co2, unsigned int *tvoc, unsigned char address)
{
  unsigned char CO2_TVOC_Cmd[8] = {0x01, 0x03, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned int CO2_temp = 0xFFFF, TVOC_temp = 0xFFFF;
  unsigned char i = 0, j = 0;
  unsigned char Try_Collection_Num = 0;

  CO2_TVOC_Cmd[0] = address;

  CRC16 = N_CRC16(CO2_TVOC_Cmd, 6);
  CO2_TVOC_Cmd[6] = CRC16 >> 8;
  CO2_TVOC_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(CO2_TVOC_Cmd, 8); 
      Delay_ms(g_Wait_Receive_Time);
      
      while(ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0;
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
        Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16){
          CO2_temp  = (Receive_Data[3] << 8) | Receive_Data[4];
          TVOC_temp = (Receive_Data[5] << 8) | Receive_Data[6];
        }

        if (CO2_temp < 400){
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
        else
          break;
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);

  *co2  = CO2_temp;
  *tvoc = TVOC_temp;
}

/*
 *brief   : ModBus协议采集土壤温度和湿度
 *para    : 浮点型的湿度参数、无符号整型的温度参数、ModBus土壤传感器地址
 *return  : 无
 */
void Read_Soil_Temp_and_Humi(float *hum, unsigned int *tep, unsigned char *tep_flag, unsigned char address)
{
  unsigned char Air_Temp_Humi_Cmd[8] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int tem_temp = 65535, hum_temp = 65535;
  float humi = 65535.0;
  unsigned int CRC16, CRC16_temp;
  unsigned char i = 0, j = 0;
  unsigned char temp_flag = 0;
  unsigned char Try_Collection_Num = 0;

  Air_Temp_Humi_Cmd[0] = address;

  CRC16 = N_CRC16(Air_Temp_Humi_Cmd, 6);
  Air_Temp_Humi_Cmd[6] = CRC16 >> 8;
  Air_Temp_Humi_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(Air_Temp_Humi_Cmd, 8);
      Delay_ms(g_Wait_Receive_Time);

      while (ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0; 
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
        Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16){
          hum_temp = (Receive_Data[3] << 8) | Receive_Data[4];
          humi = (float)hum_temp / 10.0;

          tem_temp = (Receive_Data[5] << 8) | Receive_Data[6];
          temp_flag = ((tem_temp >> 15) & 0x01);
        }

        if (hum_temp == 0 || tem_temp == 0){
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
        else
          break;
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);

  *tep = tem_temp;
  *hum = humi;
  *tep_flag = temp_flag;
}

/*
 *brief   : ModBus协议采集温度和湿度
 *para    : 浮点型的湿度参数、无符号整型的温度参数、ModBus百叶箱地址
 *return  : 无
 */
void Read_Temp_and_Humi(float *hum, unsigned int *tep, unsigned char *tep_flag, unsigned char address)
{
  unsigned char Air_Temp_Humi_Cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int tem_temp = 65535, hum_temp = 65535;
  float humi = 65535.0;
  unsigned int CRC16, CRC16_temp;
  unsigned char i = 0, j = 0;
  unsigned char temp_flag = 0;
  unsigned char Try_Collection_Num = 0;

  Air_Temp_Humi_Cmd[0] = address;

  CRC16 = N_CRC16(Air_Temp_Humi_Cmd, 6);
  Air_Temp_Humi_Cmd[6] = CRC16 >> 8;
  Air_Temp_Humi_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(Air_Temp_Humi_Cmd, 8);
      Delay_ms(g_Wait_Receive_Time);

      while (ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0; 
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
          Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16){
          hum_temp = (Receive_Data[3] << 8) | Receive_Data[4];
          humi = (float)hum_temp / 10.0;

          tem_temp = (Receive_Data[5] << 8) | Receive_Data[6];
          temp_flag = ((tem_temp >> 15) & 0x01);
        }

        //结合百叶箱无效数据情况
        if (hum_temp == 0 || tem_temp == 0 || hum_temp >= 9980 || tem_temp >= 9980){
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
        else
          break;
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);

  *tep = tem_temp;
  *hum = humi;
  *tep_flag = temp_flag;
}

/*
 *brief   : ModBus协议采集光照强度
 *para    : 无符号长整型的光照强度参数、ModBus百叶箱地址
 *return  : 无
 */
void Read_Lux(unsigned long int *lux_value, unsigned char address)
{
  unsigned char Lux_Cmd[8] = {0x01, 0x03, 0x00, 0x07, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned long int Lux_temp = 0xFFFFFFFF;
  unsigned char i = 0, j = 0;
  unsigned char Try_Collection_Num = 0;
  bool Collect_Flag = true;

  Lux_Cmd[0] = address;

  CRC16 = N_CRC16(Lux_Cmd, 6);
  Lux_Cmd[6] = CRC16 >> 8;
  Lux_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(Lux_Cmd, 8); 
      Delay_ms(g_Wait_Receive_Time);

      while (ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0;
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }  
      #if PRINT_DEBUG
        Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16)
          Lux_temp = (Receive_Data[3] << 24) | (Receive_Data[4] << 16) | (Receive_Data[5] << 8) | Receive_Data[6];

        if (Lux_temp == 0)
          Collect_Flag = false;
        
        if (Lux_temp >= 3941072){
          Collect_Flag = false;
          Lux_temp = 0xFFFFFFFF;
        }

        if (Collect_Flag = true)
          break;
        else{
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);

  *lux_value = Lux_temp;
} 

/*
 *brief   : ModBus协议采集大气气压
 *para    : 浮点型的大气气压参数、ModBus百叶箱地址
 *return  : 无
 */
void Read_Atmos(unsigned long int *atmos, unsigned char address)
{
  unsigned char Atmos_Cmd[8] = {0x01, 0x03, 0x00, 0x0A, 0x00, 0x02, 0x00, 0x00};
  unsigned char Receive_Data[9] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned long int Atmos_temp = 0xFFFFFFFF;
  unsigned char i = 0, j = 0;
  unsigned char Try_Collection_Num = 0;

  Atmos_Cmd[0] = address;
  
  CRC16 = N_CRC16(Atmos_Cmd, 6);
  Atmos_Cmd[6] = CRC16 >> 8;
  Atmos_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(Atmos_Cmd, 8); 
      Delay_ms(g_Wait_Receive_Time);

      while (ModBus_Serial.available() > 0){
        if (i >= 9){
          i = 0; 
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
          Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 7);
        CRC16_temp = (Receive_Data[7] << 8) | Receive_Data[8];

        if (CRC16_temp == CRC16)
          Atmos_temp = (Receive_Data[3] << 24) | (Receive_Data[4] << 16) | (Receive_Data[5] << 8) | Receive_Data[6];

        if (Atmos_temp == 0){
          Atmos_temp = 0xFFFFFFFF;
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
        else
          break; 
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);
  
  *atmos = Atmos_temp;
} 

/*
 *brief   : ModBus协议采集是否降雨或降雪
 *para    : 无符号字符型的开关量参数、ModBus雨雪传感器地址
 *return  : 无
 */
void Read_Rainfall(unsigned char *rainfall, unsigned char address)
{
  unsigned char Rainfall_Cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  unsigned char Receive_Data[7] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned char Rainfall_temp = 255;
  unsigned char i = 0, j = 0;

  Rainfall_Cmd[0] = address;

  CRC16 = N_CRC16(Rainfall_Cmd, 6);
  Rainfall_Cmd[6] = CRC16 >> 8;
  Rainfall_Cmd[7] = CRC16 & 0xFF;

  ModBus_Serial.write(Rainfall_Cmd, 8); 
  Delay_ms(g_Wait_Receive_Time);

  while (ModBus_Serial.available() > 0){
    if (i >= 7){
      i = 0;
      j++;
      if (j >= 10) break;
    }
    Receive_Data[i++] = ModBus_Serial.read();

    #if PRINT_DEBUG
      Serial.print(Receive_Data[i - 1], HEX);
      Serial.print(" ");
    #endif
  }
  #if PRINT_DEBUG
    Serial.println();
  #endif

  if (i > 0){
    i = 0;
    CRC16 = N_CRC16(Receive_Data, 5);
    CRC16_temp = (Receive_Data[5] << 8) | Receive_Data[6];

    if (CRC16_temp == CRC16)
      Rainfall_temp = Receive_Data[4];
  }
  *rainfall = Rainfall_temp;
}

void Read_Photics_Rainfall(unsigned char *rainfall_buffer, unsigned char address)
{
  unsigned char Rainfall_Cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00};
  unsigned char Receive_Data[13] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned char Receive_Length = 0, bug_break = 0; 

  for (unsigned char i = 0; i < 6; i++)
    rainfall_buffer[i] = 0xFF;

  Rainfall_Cmd[0] = address;

  CRC16 = N_CRC16(Rainfall_Cmd, 6);

  Rainfall_Cmd[6] = CRC16 >> 8;
  Rainfall_Cmd[7] = CRC16 & 0xFF;

  ModBus_Serial.write(Rainfall_Cmd, 8); 
  Delay_ms(g_Wait_Receive_Time);

  while (ModBus_Serial.available() > 0){
    if (Receive_Length >= 13){
      Receive_Length = 0;
      bug_break++;
      if (bug_break >= 10) break; 
    }
    Receive_Data[Receive_Length++] = ModBus_Serial.read();

    #if PRINT_DEBUG
      Serial.print(Receive_Data[Receive_Length - 1], HEX);
      Serial.print(" ");
    #endif
  }
  #if PRINT_DEBUG
      Serial.println();
  #endif

  if (Receive_Length > 0){
    Receive_Length = 0;
    CRC16 = N_CRC16(Receive_Data, 11);
    CRC16_temp = (Receive_Data[11] << 8) | Receive_Data[12];

    if (CRC16_temp == CRC16){
      for (unsigned char i = 0; i < 8; i++)
        rainfall_buffer[i] = Receive_Data[3 + i];
    }
  }
}

/*
 *brief   : ModBus协议采集紫外线强度
 *para    : 无符号整型的紫外线强度参数、ModBus百叶箱地址
 *return  : 无
 */
void Read_UV(unsigned int *uv, unsigned char address)
{
  unsigned char UV_Cmd[8] = {0x01, 0x03, 0x00, 0x0D, 0x00, 0x01, 0x00, 0x00};
  unsigned char Receive_Data[7] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned int UV_temp = 65535;
  unsigned char i = 0, j = 0;
  unsigned char Try_Collection_Num = 0;
  bool Collect_Flag = true;

  UV_Cmd[0] = address;

  CRC16 = N_CRC16(UV_Cmd, 6);
  UV_Cmd[6] = CRC16 >> 8;
  UV_Cmd[7] = CRC16 & 0xFF;

  do{
      ModBus_Serial.write(UV_Cmd, 8); 
      Delay_ms(g_Wait_Receive_Time);
            
      while (ModBus_Serial.available() > 0){
        if (i >= 7){
          i = 0; 
          j++;
          if (j >= 10) break;
        }
        Receive_Data[i++] = ModBus_Serial.read();

        #if PRINT_DEBUG
          Serial.print(Receive_Data[i - 1], HEX);
          Serial.print(" ");
        #endif
      }
      #if PRINT_DEBUG
      Serial.println();
      #endif

      if (i > 0){
        i = 0;
        CRC16 = N_CRC16(Receive_Data, 5);
        CRC16_temp = (Receive_Data[5] << 8) | Receive_Data[6];

        if (CRC16_temp == CRC16)
          UV_temp = (Receive_Data[3] << 8) | Receive_Data[4];

        if (UV_temp == 0)
          Collect_Flag = false;
        else if (UV_temp >= 368){
          Collect_Flag = false;
          UV_temp = 65535;
        }

        if (Collect_Flag == true)
          break;
        else{
          memset(Receive_Data, 0x00, sizeof(Receive_Data));
          Try_Collection_Num++;
        }
      }else
        Try_Collection_Num++;

  }while (Try_Collection_Num < 3);

  *uv = UV_temp;
} 

/*
 *brief   : ModBus协议采集风速传感器
 *para    : 浮点型的风速参数、ModBus风速传感器地址
 *return  : 无
 */
void Read_Wind_Speed(float *wind_speed, unsigned int address)
{
  unsigned char Wind_Speed_Cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  unsigned char Receive_Data[7] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  float WS_temp = 65535.0;
  unsigned char i = 0, j = 0;
  
  Wind_Speed_Cmd[0] = address;

  CRC16 = N_CRC16(Wind_Speed_Cmd, 6);
  Wind_Speed_Cmd[6] = CRC16 >> 8;
  Wind_Speed_Cmd[7] = CRC16 & 0xFF;

  ModBus_Serial.write(Wind_Speed_Cmd, 8); 
  Delay_ms(g_Wait_Receive_Time);
        
  while (ModBus_Serial.available() > 0){
    if (i >= 7){
      i = 0; 
      j++;
      if (j >= 10) break;
    }
    Receive_Data[i++] = ModBus_Serial.read();

    #if PRINT_DEBUG
      Serial.print(Receive_Data[i - 1], HEX);
      Serial.print(" ");
    #endif
  }
  #if PRINT_DEBUG
      Serial.println();
  #endif

  if (i > 0){
    i = 0;
    CRC16 = N_CRC16(Receive_Data, 5);
    CRC16_temp = (Receive_Data[5] << 8) | Receive_Data[6];

    if (CRC16_temp == CRC16){
      WS_temp = (Receive_Data[3] << 8) | Receive_Data[4];
      WS_temp /= 10.0;
    }
  }
  *wind_speed = WS_temp;
}

/*
 *brief   : ModBus协议采集风向传感器
 *para    : 无符号整型的风向参数（方位或角度值）、ModBus风向传感器地址
 *return  : 无
 */
void Read_Wind_Direction(unsigned int *wind_direction, unsigned char address)
{
  unsigned char Wind_Direction_Cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};
  unsigned char Receive_Data[7] = {0};
  unsigned int CRC16 = 0, CRC16_temp = 0;
  unsigned char i = 0, j = 0;
  unsigned int Wind_Direction_Temp = 65535;

  Wind_Direction_Cmd[0] = address;

  CRC16 = N_CRC16(Wind_Direction_Cmd, 6);
  Wind_Direction_Cmd[6] = CRC16 >> 8;
  Wind_Direction_Cmd[7] = CRC16 & 0xFF;

  ModBus_Serial.write(Wind_Direction_Cmd, 8); 
  Delay_ms(g_Wait_Receive_Time);

  while (ModBus_Serial.available() > 0){
    if (i >= 7){
      i = 0;
      j++;
      if (j >= 10) break;
    }
    Receive_Data[i++] = ModBus_Serial.read();

    #if PRINT_DEBUG
      Serial.print(Receive_Data[i - 1], HEX);
      Serial.print(" ");
    #endif
  }
  #if PRINT_DEBUG
      Serial.println();
  #endif

  if (i > 0){
    i = 0;
    CRC16 = N_CRC16(Receive_Data, 5);
    CRC16_temp = (Receive_Data[5] << 8) | Receive_Data[6];

    if (CRC16_temp == CRC16)
      Wind_Direction_Temp = (Receive_Data[3] << 8) | Receive_Data[4];
  }

  *wind_direction = Wind_Direction_Temp;
  Delay_ms(10);
}

/*
 *brief   : 扩展的模拟输入1，返回一个无符号int型值
 *para    : 无
 *return  : 返回无符号int的电压模拟值
 */
unsigned int Get_Analogy1_Value(void)
{
  float analogy_temp = 0.0;
  unsigned int Analogy_Value = 0;

  analogy_temp = analogRead(ANALOGY_PIN_1);
  Analogy_Value = analogy_temp * 0.8056; //如果测电压值，需要乘以11.

  return Analogy_Value;
}

/*
 *brief   : 扩展的模拟输入2，返回一个无符号int型值
 *para    : 无
 *return  : 返回无符号int的电压模拟值
 */
unsigned int Get_Analogy2_Value(void)
{
  float analogy_temp = 0.0;
  unsigned int Analogy_Value = 0;

  analogy_temp = analogRead(ANALOGY_PIN_2);
  Analogy_Value = analogy_temp * 11 * 0.8056; //如果测电流值，需要取消乘以11倍.

  return Analogy_Value;
}

/*
 *brief   : 扩展的模拟输入3，返回一个无符号int型值
 *para    : 无
 *return  : 返回无符号int的电压模拟值
 */
unsigned int Get_Analogy3_Value(void)
{
  float analogy_temp = 0.0;
  unsigned int Analogy_Value = 0;

  analogy_temp = analogRead(ANALOGY_PIN_3);
  Analogy_Value = analogy_temp * 11 * 0.8056; //如果测电流值，需要取消乘以11倍.

  return Analogy_Value;
}

/*
 *brief   : 读取各个传感器的值，得到采集数据RTC，并打印获取的传感器值到串口
 *para    : 无
 *return  : 无
 */
void Data_Acquisition(void)
{
  RS485_BUS_PWR_ON;
  delay(5000);

  #if TYPE06
  //读取土壤温度和湿度
  Read_Soil_Temp_and_Humi(&Muti_Sensor_Data.Soil_Humi, &Muti_Sensor_Data.Soil_Temp, &Muti_Sensor_Data.Soil_Temp_Flag, SOIL_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取土壤电导率和盐分
  Read_Cond_and_Salt(&Muti_Sensor_Data.Soil_Cond, &Muti_Sensor_Data.Soil_Salt, SOIL_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  #endif

  //读取大气温度和湿度
  Read_Temp_and_Humi(&Muti_Sensor_Data.Air_Humi, &Muti_Sensor_Data.Air_Temp, &Muti_Sensor_Data.Air_Temp_Flag, AIR_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取大气光照
  Read_Lux(&Muti_Sensor_Data.Air_Lux, AIR_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取大气气压
  Read_Atmos(&Muti_Sensor_Data.Air_Atmos, AIR_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取大气紫外线强度
  Read_UV(&Muti_Sensor_Data.Air_UV, AIR_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取风向
  Read_Wind_Direction(&Muti_Sensor_Data.Wind_DirCode, WIND_DIRECTION_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取风速
  Read_Wind_Speed(&Muti_Sensor_Data.Air_Wind_Speed, WIND_RATE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);

  //读取雨雪传感器开关量
  #if (TYPE06 || TYPE07)
  Read_Rainfall(&Muti_Sensor_Data.Rainfall, RAINFALL_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  #elif TYPE08
  memset(Muti_Sensor_Data.Rainfall_Buffer, 0xFF, sizeof(Muti_Sensor_Data.Rainfall_Buffer));
  Read_Photics_Rainfall(Muti_Sensor_Data.Rainfall_Buffer, RAINFALL_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  #endif

  #if TYPE06
  //读取棚内温度和湿度
  Read_Temp_and_Humi(&Muti_Sensor_Data.GreenHouse_Humi, &Muti_Sensor_Data.GreenHouse_Temp, &Muti_Sensor_Data.GreenHouse_Temp_Flag, GREENHOUSE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取棚内光照强度
  Read_Lux(&Muti_Sensor_Data.GreenHouse_Lux, GREENHOUSE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取棚内大气气压
  Read_Atmos(&Muti_Sensor_Data.GreenHouse_Atmos, GREENHOUSE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取棚内紫外线强度
  Read_UV(&Muti_Sensor_Data.GreenHouse_UV, GREENHOUSE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  //读取棚内CO2和TVOC浓度
  Read_CO2_and_TVOC(&Muti_Sensor_Data.GreenHouse_CO2, &Muti_Sensor_Data.GreenHouse_TVOC, GREENHOUSE_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);
  #endif

  //读取大气二氧化碳和TVOC浓度 (因为该传感器需要预热，所以排在后面采集)
  Read_CO2_and_TVOC(&Muti_Sensor_Data.Air_CO2, &Muti_Sensor_Data.Air_TVOC, AIR_SENSOR_ADDR);
  delay(g_Wait_Collect_Time);

  #if TYPE07
  //读取水位值
  Muti_Sensor_Data.Water_Level = Get_Analogy1_Value();
  delay(g_Wait_Collect_Time);

  //读取扩展ADC1的值
  Muti_Sensor_Data.ADC_Value1 = Get_Analogy2_Value();
  delay(g_Wait_Collect_Time);

  //读取扩展ADC2的值
  Muti_Sensor_Data.ADC_Value2 = Get_Analogy3_Value();
  delay(g_Wait_Collect_Time);
  #endif

  RS485_BUS_PWR_OFF;

  //得到当前采集时间
  UTCTime CurrentSec = 0;
  CurrentSec = InRtc.getTime();
  osal_ConvertUTCTime(&RtcTime, CurrentSec);

  Muti_Sensor_Data.curTime.tm_year = RtcTime.year;
  Muti_Sensor_Data.curTime.tm_mon  = RtcTime.month;
  Muti_Sensor_Data.curTime.tm_mday = RtcTime.day;
  Muti_Sensor_Data.curTime.tm_hour = RtcTime.hour;
  Muti_Sensor_Data.curTime.tm_min  = RtcTime.minutes;
  Muti_Sensor_Data.curTime.tm_sec  = RtcTime.seconds; 

  //打印采集的数据到串口
  Serial.print("Air Temp: "); 
  Muti_Sensor_Data.Air_Temp_Flag == 1 ? Serial.print(((float)(65536 - Muti_Sensor_Data.Air_Temp)) / 10 * -1) : Serial.print(Muti_Sensor_Data.Air_Temp / 10);
  Serial.println("℃");
  Serial.print("Air Humi: "); Serial.print(Muti_Sensor_Data.Air_Humi); Serial.println("%RH");
  Serial.print("Air Atmos: "); Serial.print(Muti_Sensor_Data.Air_Atmos); Serial.println("Pa");
  Serial.print("Air Lux: "); Serial.print(Muti_Sensor_Data.Air_Lux); Serial.println("Lux");
  Serial.print("Air uv: "); Serial.println(Muti_Sensor_Data.Air_UV); 
  Serial.print("Air CO2: "); Serial.print(Muti_Sensor_Data.Air_CO2); Serial.println("ppm");
  Serial.print("Air TVOC: "); Serial.print(Muti_Sensor_Data.Air_TVOC); Serial.println("ppm");
  Serial.print("Wind Speed: "); Serial.print(Muti_Sensor_Data.Air_Wind_Speed); Serial.println("m/s");
  Serial.print("Wind Direction:"); Serial.println(Muti_Sensor_Data.Wind_DirCode);

  #if TYPE06
  Serial.print("Soil Temp: ");
  Muti_Sensor_Data.Soil_Temp_Flag == 1 ? Serial.print(((float)(65536 - Muti_Sensor_Data.Soil_Temp)) / 10 * -1) : Serial.print(Muti_Sensor_Data.Soil_Temp / 10);
  Serial.println("℃");
  Serial.print("Soil Humi: "); Serial.print(Muti_Sensor_Data.Soil_Humi); Serial.println("%RH");
  Serial.print("Soil Cond: "); Serial.print(Muti_Sensor_Data.Soil_Cond); Serial.println("us/cm");
  Serial.print("Soil Salt；"); Serial.print(Muti_Sensor_Data.Soil_Salt); Serial.println("mg/L");
  Serial.print("GreenHouse Temp: "); 
  Muti_Sensor_Data.Air_Temp_Flag == 1 ? Serial.print(((float)(65536 - Muti_Sensor_Data.GreenHouse_Temp)) / 10 * -1) : Serial.print(Muti_Sensor_Data.GreenHouse_Temp / 10);
  Serial.println("℃");
  Serial.print("GreenHouse Humi: "); Serial.print(Muti_Sensor_Data.GreenHouse_Humi); Serial.println("%RH");
  Serial.print("GreenHouse Atmos: "); Serial.print(Muti_Sensor_Data.GreenHouse_Atmos); Serial.println("Pa");
  Serial.print("GreenHouse Lux: "); Serial.print(Muti_Sensor_Data.GreenHouse_Lux); Serial.println("Lux");
  Serial.print("GreenHouse uv: "); Serial.println(Muti_Sensor_Data.GreenHouse_UV); 
  Serial.print("GreenHouse CO2: "); Serial.print(Muti_Sensor_Data.GreenHouse_CO2); Serial.println("ppm");
  Serial.print("GreenHouse TVOC: "); Serial.print(Muti_Sensor_Data.GreenHouse_TVOC); Serial.println("ppm");
  #endif

  #if (TYPE06 || TYPE07)
  if (Muti_Sensor_Data.Rainfall == 0)
    Serial.println("Sunny day");
  else if (Muti_Sensor_Data.Rainfall == 1)
    Serial.println("Rain day");
  else
    Serial.println("Rain sensor abnormal");

  #elif TYPE08
  unsigned int Rainfall_Temp = 0;
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[0] << 8 | Muti_Sensor_Data.Rainfall_Buffer[1];
  Serial.print("Real time rainfall: "); Serial.print(Rainfall_Temp); Serial.println("mm");
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[2] << 8 | Muti_Sensor_Data.Rainfall_Buffer[3];
  Serial.print("hour rainfall: "); Serial.print(Rainfall_Temp); Serial.println("mm");
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[4] << 8 | Muti_Sensor_Data.Rainfall_Buffer[5];
  Serial.print("month time rainfall: "); Serial.print(Rainfall_Temp); Serial.println("mm");
  Rainfall_Temp = Muti_Sensor_Data.Rainfall_Buffer[6] << 8 | Muti_Sensor_Data.Rainfall_Buffer[7];
  Serial.print("year time rainfall: "); Serial.print(Rainfall_Temp); Serial.println("mm");
  #endif

  //查看目前EP保存的数据笔数。
  noInterrupts();
  EpromDb.open(EEPROM_BASE_ADDR);
  unsigned long Muti_Sensor_Data_Count = EpromDb.count();
  interrupts();

  //如果EP中已经保存过传感器数据，且数据笔数小于能保存的最大值
  if ((Muti_Sensor_Data_Count >= 1) && (Muti_Sensor_Data_Count < EEPROM_MAX_RECORD)){
    //将采集到的数据存入EEPROM中
    Muti_Sensor_Data_Base_Init();
    Save_SensorData_to_EEPROM();
    //置位已保存数据标志位，避免后面出现重复初始化保存等工作。
    Sys_Run_Para.g_Already_Save_Data_Flag = true;
  }
}