
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

TinyGsm modem(GSM_Serial);
TinyGsmClient client(modem);

unsigned int Run_Time_Out_Sec = 0;          //运行超时计数
char Toggle = 0;                             //状态灯闪灭位翻转

void setup()
{
  Some_GPIO_Init();//IO口初始化定义

  Serial.begin(9600); //USB serial port.
  ModBus_Serial.begin(9600);
  GSM_Serial.begin(9600);
  
  bkp_init();

  //初始化RTC闹钟
  Init_RTC(180);

  //如果电池电压小于6.5V，设备休眠
  if(Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES) < MIN_BAT_VOL)
    SYS_Sleep();

  System_ID_Self_Check(SysHostID);

  Key_Clear_Device_Parameter(); //按键清除系统参数，慎用
  Key_Clear_Muti_Sensor_Data();

  //初始化定时器2
  Timer2.setChannel1Mode(TIMER_OUTPUTCOMPARE);
  Timer2.setPeriod(1000000); // in microseconds，1S
  Timer2.setCompare1(1);   // overflow might be small
  Timer2.attachCompare1Interrupt(Time2_Handler);

  //RS485模块开始采集传感器数据
  Data_Acquisition();
} 

void loop()
{
  if(Get_Bat_Voltage(DEFAULT_VOL_CHANGE_TIMES) < MIN_BAT_VOL)
    SYS_Sleep();

  if(!Pre_Access_Network()) SYS_Sleep();

  if(!Connect_to_The_Server()) SYS_Sleep();

  Send_Data_To_Server() == true ? SYS_Sleep() : nvic_sys_reset();
}

/*
 *brief   : 关闭相关电源，进入待机模式
 *para    : 无
 *return  :
 */
void SYS_Sleep(void)
{
  Timer2.detachCompare1Interrupt();
  LED1_OFF;
  LED2_OFF;
  LED3_OFF;
  LED4_OFF;
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
  if (digitalRead(KEY2_INPUT) == LOW){
    GSM_STATUS_LED_ON;
    Delay_ms(1000);
    if (digitalRead(KEY2_INPUT) == LOW){
      Clear_HostID = true;
      Serial.println("Clear System parameter OK...");
      GSM_STATUS_LED_OFF; 
    } 
  }
}

void Key_Clear_Muti_Sensor_Data(void)
{ 
  if (digitalRead(KEY1_INPUT) == LOW){
    GSM_STATUS_LED_ON;
    Delay_ms(1000);
    if (digitalRead(KEY1_INPUT) == LOW){

      #if DEVICE_V2_5
        EP_Write_Enable();
      #endif

      EpromDb.open(EEPROM_BASE_ADDR);
      EpromDb.clear(); 
      
      #if DEVICE_V2_5
        EP_Write_Disable();
      #endif

      Serial.println("Clear Sensor Data OK...");
      GSM_STATUS_LED_OFF; 
    }
  }
}  


/*
 *brief   : 定时器2中断函数
 *para    : 无
 *return  : 无
*/
void Time2_Handler(void)
{
    Toggle ^= 1;
    digitalWrite(LED1, Toggle); //状态灯闪烁

    Run_Time_Out_Sec++;
    //如果运行超时，复位
    if(Run_Time_Out_Sec >= 300){
        Run_Time_Out_Sec = 0;
        noInterrupts();
        nvic_sys_reset();
    }
}
