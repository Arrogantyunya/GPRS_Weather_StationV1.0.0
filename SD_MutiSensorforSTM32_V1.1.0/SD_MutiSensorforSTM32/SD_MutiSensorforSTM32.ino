
#define TINY_GSM_MODEM_SIM800

#include "GSM/TinyGsmClient.h"
#include <Arduino.h>
#include <libmaple/nvic.h>
#include <libmaple/pwr.h>
#include <libmaple/bkp.h>
#include "memory.h"
#include "Private_Sensor.h"
#include "data_transmit.h"
#include "private_delay.h"
#include "Periph.h"
#include "Private_RTC.h"
#include "Ope_SD.h"

/* GSM */
TinyGsm modem(GSM_Serial);
TinyGsmClient client(modem);

unsigned int Run_Time_Out_Sec = 0;          //运行超时计数
bool Clear_Green1_Flag = true;              //LED灯提示相关标志位
int Toggle = 0;                            //状态灯闪灭位翻转

void setup()
{
  Some_GPIO_Init();//IO口初始化定义
  
  Serial.begin(9600);
  ModBus_Serial.begin(9600);
  GSM_Serial.begin(9600);
  
  bkp_init(); //初始化备份寄存器

  //初始化RTC闹钟
  Init_RTC(300);  //默认五分钟，也就是300秒，只程序最多只能运行5分钟，超时就软件复位。
  
  //如果电池电压小于6.8V，设备休眠
  if(Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES) < MIN_BAT_VOL)
    SYS_Sleep();

  System_ID_Self_Check(SysHostID);

  Key_Reset_Sys_RunMode();
  Key_Clear_Device_Parameter(); //按键清除系统参数，慎用
  Key_Clear_Muti_Sensor_Data();

  Get_SYS_RunMode(&Sys_Run_Para.g_RunMode);

  if (Sys_Run_Para.g_RunMode == NWK_TF || Sys_Run_Para.g_RunMode == SMS_TF || Sys_Run_Para.g_RunMode == TF){
    SD_Ope.SD_SPI_Init(18);
    SD_Ope.Print_SD_Card_Info();
  }

  //初始化定时器2
  Timer2.setChannel1Mode(TIMER_OUTPUTCOMPARE);
  Timer2.setPeriod(1000000); // in microseconds，1s
  Timer2.setCompare1(1);   // overflow might be small
  Timer2.attachCompare1Interrupt(Time2_Handler);

  //RS485模块开始采集传感器数据
  Data_Acquisition();
} 

void loop()
{
  if(Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES) < MIN_BAT_VOL)
    SYS_Sleep();

  LED_For_Network_Flag = true;

  if (Sys_Run_Para.g_RunMode != TF){
    if(!Pre_Access_Network()) SYS_Sleep();

    if (Sys_Run_Para.g_RunMode == SMS || Sys_Run_Para.g_RunMode == SMS_TF){
      Send_Message_to_User();
      Read_SMS_Setting_From_User();
      SYS_Sleep();

    }else if (Sys_Run_Para.g_RunMode == NWK || Sys_Run_Para.g_RunMode == NWK_TF){
      if(!Connect_to_The_Server()) SYS_Sleep();
      Send_Data_To_Server() == true ? SYS_Sleep() : nvic_sys_reset();
    }
  }

  SYS_Sleep();
}

/*
 *brief   : 关闭相关电源，进入待机模式
 *para    : 无
 *return  :
 */
void SYS_Sleep(void)
{
  Timer2.detachCompare1Interrupt();

  time_t Now_Time = 0;
  Now_Time = InRtc.getTime();

  if (Sys_Run_Para.g_Transmit_Cycle < 60 || Sys_Run_Para.g_Transmit_Cycle > 43200)
    Sys_Run_Para.g_Transmit_Cycle = 180;

  if (Quick_Work_Flag == true){
    if (Sys_Run_Para.g_Transmit_Cycle > 120){
      Sys_Run_Para.g_Transmit_Cycle = 120;
    }
  }

  Now_Time += Sys_Run_Para.g_Transmit_Cycle; 
  InRtc.setAlarmTime(Now_Time); //设置RTC闹钟

  GREEN1_OFF;
  RED1_OFF;
  GREEN2_OFF;
  RED2_OFF;
  GPRS_PWR_OFF;
  DC12V_PWR_OFF;
  USB_PORT_DIS;
  GPS_ANT_PWR_OFF;

  PWR_WakeUpPinCmd(ENABLE);//使能唤醒引脚，默认PA0
  PWR_ClearFlag(PWR_FLAG_WU);
  PWR_EnterSTANDBYMode();//进入待机
}

/*
 *brief   : 按键2清除储存的系统相关参数
 *para    : 无
 *return  : 无
 */
void Key_Clear_Device_Parameter(void)
{
  if (digitalRead(KEY1_INPUT) == LOW){
    RED1_ON;
    RED2_ON;
    Delay_ms(3000);
    if (digitalRead(KEY2_INPUT) == LOW){
      Clear_HostID = true;
      Serial.println("Clear System parameter OK...");
    } 
  }
  RED1_OFF;
  RED2_OFF;
}

void Key_Clear_Muti_Sensor_Data(void)
{ 
  if (digitalRead(KEY1_INPUT) == LOW){
    RED1_ON;
    Delay_ms(5000);
    if (digitalRead(KEY1_INPUT) == LOW){

      EP_Write_Enable();
      EpromDb.open(EEPROM_BASE_ADDR);
      EpromDb.clear(); 
      EP_Write_Disable();

      Serial.println("Clear Sensor Data OK...");
    }
  }
  RED1_OFF;
}  

void Key_Reset_Sys_RunMode(void)
{
  if (digitalRead(KEY2_INPUT) == LOW){
    RED2_ON;
    Delay_ms(5000);
    if (digitalRead(KEY2_INPUT) == LOW){
      Reset_Sys_RunMode();
    }
  }
  RED2_OFF;
}

/*
 *brief   : 定时器2中断函数
 *para    : 无
 *return  : 无
*/
void Time2_Handler(void)
{
  if (LED_For_Network_Flag != true){
    Toggle ^= 1;
    digitalWrite(GREEN1_PIN, Toggle); //状态灯闪烁    
  }
  else{
    if (Clear_Green1_Flag == true){
      Clear_Green1_Flag = false;
      if (digitalRead(GREEN1_PIN) == HIGH){
        GREEN1_OFF;
      }
    }
  }

    Run_Time_Out_Sec++;
    //如果运行超时，复位
    if(Run_Time_Out_Sec >= 300){
        Run_Time_Out_Sec = 0;

        nvic_sys_reset();
    }
}
