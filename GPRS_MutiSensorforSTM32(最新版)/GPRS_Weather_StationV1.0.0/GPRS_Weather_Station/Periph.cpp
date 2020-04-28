#include "Periph.h"
#include <Arduino.h>

/*
 *brief   : 初始化相关引脚模式
 *para    : 无
 *return  : 无
 */
void Some_GPIO_Init(void)
{
    afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);

    //两个功能按键
    pinMode(KEY1_INPUT, INPUT);
    pinMode(KEY2_INPUT, INPUT);

    //两个双色LED灯
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);

    pinMode(DC12V_PWR_PIN, OUTPUT);
    pinMode(GPRS_PWR_CON_PIN, OUTPUT);
    pinMode(RS485_BUS_PWR_PIN, OUTPUT);
    
    //本设备未使用，默认模拟输入
    pinMode(LORA_PWR_PIN, INPUT_ANALOG);
    pinMode(LORA_M0_PIN, INPUT_ANALOG);
    pinMode(LORA_M1_PIN, INPUT_ANALOG);
    pinMode(LORA_AUX_PIN, INPUT_ANALOG);

    pinMode(USB_EN_PIN, OUTPUT);
    pinMode(GPRS_RST_PIN, OUTPUT);
    pinMode(GPRS_PWRKEY_PIN, OUTPUT);
    pinMode(GPS_ANT_PWR_CON_PIN, OUTPUT);

    #if DEVICE_V2_5
    pinMode(WP_PIN, OUTPUT);
    //pinMode(NETLIGHT_PIN, OUTPUT);
    #endif

    //ADC 输入通道
    pinMode(HOST_VOL_ADC_INPUT_PIN, INPUT_ANALOG);

    //扩展的ADC读取电压的三个引脚
    pinMode(ANALOGY_PIN_1, INPUT_ANALOG);
    pinMode(ANALOGY_PIN_2, INPUT_ANALOG);
    pinMode(ANALOGY_PIN_3, INPUT_ANALOG);

    GPRS_RST_LO;
    GPRS_PWR_OFF;
    DC12V_PWR_ON;
    USB_PORT_EN;
}

#if DEVICE_V2_5
void EP_Write_Enable(void)
{
  digitalWrite(WP_PIN, LOW);
}

void EP_Write_Disable(void)
{
  digitalWrite(WP_PIN, HIGH);
}
#endif

/*
 *brief   : 得到电池电压
 *para    : 无
 *return  : 无符号整型的电压值
 */
unsigned int Get_Bat_Voltage(unsigned char change_times = 11)
{ 
    unsigned char Change_Times;

    if ((change_times == 0) || (change_times > 101) || (change_times % 2 == 0))
        Change_Times = 11;
    else
        Change_Times = change_times;

    unsigned int Vol_Buff[Change_Times]; 
  
    for (unsigned char i = 0; i < Change_Times; i++)
        Vol_Buff[i] = analogRead(HOST_VOL_ADC_INPUT_PIN);

    unsigned int temp;
    for (unsigned char i = 0; i < Change_Times; i++){
        for (unsigned char j = 0; j < (Change_Times - 1); j++){
            if (Vol_Buff[j] > Vol_Buff[j + 1]){
                temp = Vol_Buff[j + 1];
                Vol_Buff[j + 1] = Vol_Buff[j];
                Vol_Buff[j] = temp;
            }
        }
    }

  float Vol_Temp = Vol_Buff[(Change_Times + 1) / 2];
  Vol_Temp *= ADC_RATE;
  Vol_Temp *= VBAT_DIVIDER_RATIO;
  unsigned int Vol_Value = (unsigned int)Vol_Temp;

  return Vol_Value;
}

/*
 *brief   : SIM868电源控制
 *para    : 无
 *return  : 无
 */
void SIM800_PWR_CON(void)
{
  GPRS_PWRKEY_LO;
  delay(1000);
  GPRS_PWRKEY_HI;
  delay(500);
}