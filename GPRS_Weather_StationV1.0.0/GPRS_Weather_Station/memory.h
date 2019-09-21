#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "EDB.h"
#include "data_transmit.h"

//传感器数据存储起始地址
#define EEPROM_BASE_ADDR                    1280

//本机系统相关数据存储地址
//存在BKP备份寄存器
#define SYS_HOSTID_BASE_ADDR                1
#define SYS_HOSTID_END_ADDR                 4
#define SYS_HOSTID_VERIFY                   5

#define SYS_HOSTID_BKP_BASE_ADDR            11
#define SYS_HOSTID_BKP_END_ADDR             14
#define SYS_HOSTID_BKP_VERIFY               15

//本机系统相关数据存储地址
//存在EEPROM1024
#define EP_SYS_HOSTID_BASE_ADDR             8
#define EP_SYS_HOSTID_END_ADDR              11
#define EP_SYS_HOSTID_VERIFY                12

#define EP_SYS_HOSTID_BKP_BASE_ADDR         16
#define EP_SYS_HOSTID_BKP_END_ADDR          19
#define EP_SYS_HOSTID_BKP_VERIFY            20

//存在EEPROM1024
#define SYS_ACQUISITION_CYCLE_HIGH_ADDR     60
#define SYS_ACQUISITION_CYCLE_LOW_ADDR      61

#define SYS_TRANSMIT_CYCLE_HIGH_ADDR        64
#define SYS_TRANSMIT_CYCLE_LOW_ADDR         65

#define SYS_RUN_MODE_HIGH_ADDR              68
#define SYS_RUN_MODE_LOW_ADDR               69

#define SYS_POSITION_HIGH_ADDR              72
#define SYS_POSITION_LOW_ADDR               73

#define SYS_CURRENT_RECORD_HIGH_ADDR        76
#define SYS_CURRENT_RECORD_LOW_ADDR         77

#define SYS_LOCATION_FLAG_ADDR              80 

#define SYS_PARA_END_ADDR                   81

extern unsigned int         EPMaxRecord;
#define EEPROM_MAX_RECORD   EPMaxRecord

extern EDB EpromDb;

void writer(unsigned long address, unsigned char data);
unsigned char reader(unsigned long address);

bool Save_HostID(unsigned char *sys_hostID);
bool Save_BKP_HostID(unsigned char *sys_hostID);
bool Read_HostID(unsigned char *sys_hostID);
bool Read_BKP_HostID(unsigned char *sys_hostID);

bool EP_Save_HostID(unsigned char *sys_hostID);
bool EP_Save_BKP_HostID(unsigned char *sys_hostID);
bool EP_Read_HostID(unsigned char *sys_hostID);
bool EP_Read_BKP_HostID(unsigned char *sys_hostID);

bool System_ID_Self_Check(unsigned char *sys_hostID);

bool Save_System_ID(unsigned char *sys_hostID);

void Clear_SYS_HOSTID(void);

bool Verify_Sys_Para(void);

void Muti_Sensor_Data_Base_Init(void);
void Read_EEPROM_Server_Param(unsigned char *sys_hostID, System_Run_Parameter *para);
void Save_SensorData_to_EEPROM(void);
void Save_Param_to_EEPROM(void);
void Save_Sys_Current_Record(unsigned int now_record);
unsigned int Read_Sys_Current_Record(void);

#endif