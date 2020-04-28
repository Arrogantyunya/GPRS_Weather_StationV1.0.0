#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_


#define TYPE06    0 //棚内+大气
#define TYPE07    0 //中科院带三路ADC
#define TYPE08    0 //第三代气象站，新增光学雨量计
#define TYPE09    1 //第二代扩展三路ADC，带TF卡储存

#define PRINT_DEBUG   0

//EEPROM capacity
#define TABLE_SIZE 131072 

//Serial port config.
#define GSM_Serial                  Serial3
#define ModBus_Serial               Serial2
#define LoRa_Serial                 Serial1

//Button config
#define KEY1_INPUT                  PA4
#define KEY2_INPUT                  PA5

//LED config
#define GREEN1_PIN                  PC2
#define RED1_PIN                    PC3
#define GREEN2_PIN                  PC0
#define RED2_PIN                    PC1

//External ADC config
#define HOST_VOL_ADC_INPUT_PIN      PB1

//外设电源、信号控制IO,输出控制
#define DC12V_PWR_PIN               PC6
#define RS485_BUS_PWR_PIN           PC7
#define LORA_PWR_PIN                PB8
#define LORA_M0_PIN                 PD2
#define LORA_M1_PIN                 PB3
#define LORA_AUX_PIN                PC13


//EEPROM W/R enable pin.
#define WP_PIN                      PB5

//USB使能脚
#define USB_EN_PIN                  PC9

//GPRS相关引脚
#define GPS_ANT_PWR_CON_PIN         PC12
#define GPRS_RST_PIN                PC10
#define GPRS_PWRKEY_PIN             PC11
#define GPRS_PWR_CON_PIN            PB4
//#define NETLIGHT_PIN                PA15


#define ANALOGY_PIN_1               PC4
#define ANALOGY_PIN_2               PA7
#define ANALOGY_PIN_3               PA6


//ADC定义,12位ADC，参考电压为3.3V
#define ADC_RATE                            0.8056 //3300/4096
//电池输入电压分压比
#define VBAT_DIVIDER_RATIO                  6  

#define MIN_BAT_VOL                         6800

#define DEFALUT_PHONE   "15095310716"

#endif