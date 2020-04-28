#include "memory.h"
#include "AT24C1024.h"
#include "board_config.h"
#include "Private_Sensor.h"
#include <libmaple/bkp.h>
#include "Periph.h"
#include "User_CRC8.h"

//EEPROM最大存储记录数
unsigned int EPMaxRecord = (TABLE_SIZE - EEPROM_BASE_ADDR) / sizeof(SENSOR_DATA) - 1;

// Create an EDB object with the appropriate write and read handlers
void writer(unsigned long address, unsigned char data)
{
  EEPROM1024.write(address, data);
}

unsigned char reader(unsigned long address)
{
  return EEPROM1024.read(address);
}

EDB EpromDb(&writer,&reader);

/*
 *brief   : 保存本机ID到备份寄存器中
 *para    : 无
 *return  : 布尔值
 */
bool Save_HostID(unsigned char *sys_hostID)
{
  bkp_enable_writes();
  delay(10);
  for (unsigned char i = 0; i < 4; i++)
    bkp_write(SYS_HOSTID_BASE_ADDR + i, sys_hostID[i]);

  unsigned char ID_Verify = GetCrc8(&sys_hostID[0], 4);

  bkp_write(SYS_HOSTID_VERIFY, ID_Verify);

  delay(10);
  bkp_disable_writes();

  for (unsigned char i = 0; i < 4; i++)
    if (sys_hostID[i] != bkp_read(SYS_HOSTID_BASE_ADDR + i))
      return false;

  return true;
}

/*
 *brief   : 保存本机ID另一份备份到备份寄存器中
 *para    : 无
 *return  : 布尔值
 */
bool Save_BKP_HostID(unsigned char *sys_hostID)
{
  bkp_enable_writes();
  delay(10);
  for (unsigned char i = 0; i < 4; i++)
    bkp_write(SYS_HOSTID_BKP_BASE_ADDR + i, sys_hostID[i]);

  unsigned char ID_Verify = GetCrc8(&sys_hostID[0], 4);

  bkp_write(SYS_HOSTID_BKP_VERIFY, ID_Verify);

  delay(10);
  bkp_disable_writes();

  for (unsigned char i = 0; i < 4; i++)
    if (sys_hostID[i] != bkp_read(SYS_HOSTID_BKP_BASE_ADDR + i))
      return false;

  return true;
}

/*
 *brief   : 从备份寄存器读取一份本机ID
 *para    : 无
 *return  : 布尔值
 */
bool Read_HostID(unsigned char *sys_hostID)
{
  for (unsigned char i = 0; i < 4; i++)
    sys_hostID[i] = bkp_read(SYS_HOSTID_BASE_ADDR + i);

  unsigned char ID_Verify = bkp_read(SYS_HOSTID_VERIFY);
  unsigned char ID_Verify_Temp = GetCrc8(&sys_hostID[0], 4);

  if (ID_Verify != ID_Verify_Temp)  return false;

  if (sys_hostID[0] == 0 && sys_hostID[1] == 0 && sys_hostID[2] == 0 && sys_hostID[3] == 0)
    return false;

  return true;
}

/*
 *brief   : 从备份寄存器读取另一份本机ID
 *para    : 无
 *return  : 布尔值
 */
bool Read_BKP_HostID(unsigned char *sys_hostID)
{
  for (unsigned char i = 0; i < 4; i++)
    sys_hostID[i] = bkp_read(SYS_HOSTID_BKP_BASE_ADDR + i);

  unsigned char ID_Verify = bkp_read(SYS_HOSTID_BKP_VERIFY);
  unsigned char ID_Verify_Temp = GetCrc8(&sys_hostID[0], 4);

  if (ID_Verify != ID_Verify_Temp)  return false;

  if (SysHostID[0] == 0 && SysHostID[1] == 0 && SysHostID[2] == 0 && SysHostID[3] == 0)
    return false;

  return true;
}

/*
 *brief   : 保存本机ID到EEPROM中
 *para    : 无
 *return  : 布尔值
 */
bool EP_Save_HostID(unsigned char *sys_hostID)
{
  EP_Write_Enable();
  for (unsigned char i = 0; i < 4; i++)
    EEPROM1024.write(EP_SYS_HOSTID_BASE_ADDR + i, sys_hostID[i]);

  unsigned char ID_Verify = GetCrc8(&sys_hostID[0], 4);

  EEPROM1024.write(EP_SYS_HOSTID_VERIFY, ID_Verify);

  EP_Write_Disable();

  for (unsigned char i = 0; i < 4; i++)
    if (sys_hostID[i] != EEPROM1024.read(EP_SYS_HOSTID_BASE_ADDR + i))
      return false;

  return true;
}

/*
 *brief   : 保存本机ID另一份备份到EEPROM中
 *para    : 无
 *return  : 布尔值
 */
bool EP_Save_BKP_HostID(unsigned char *sys_hostID)
{
  EP_Write_Enable();

  for (unsigned char i = 0; i < 4; i++)
    EEPROM1024.write(EP_SYS_HOSTID_BKP_BASE_ADDR + i, sys_hostID[i]);

  unsigned char ID_Verify = GetCrc8(&sys_hostID[0], 4);

  EEPROM1024.write(EP_SYS_HOSTID_BKP_VERIFY, ID_Verify);

  EP_Write_Disable();

  for (unsigned char i = 0; i < 4; i++)
    if (sys_hostID[i] != EEPROM1024.read(EP_SYS_HOSTID_BKP_BASE_ADDR + i))
      return false;

  return true;
}

/*
 *brief   : 从EEPROM读取一份本机ID
 *para    : 无
 *return  : 布尔值
 */
bool EP_Read_HostID(unsigned char *sys_hostID)
{
  for (unsigned char i = 0; i < 4; i++)
    sys_hostID[i] = EEPROM1024.read(EP_SYS_HOSTID_BASE_ADDR + i);

  unsigned char ID_Verify = EEPROM1024.read(EP_SYS_HOSTID_VERIFY);
  unsigned char ID_Verify_Temp = GetCrc8(&sys_hostID[0], 4);

  if (ID_Verify != ID_Verify_Temp)
    return false;

  if (sys_hostID[0] == 0 && sys_hostID[1] == 0 && sys_hostID[2] == 0 && sys_hostID[3] == 0)
    return false;

  return true;
}

/*
 *brief   : 从EEPROM读取另一份本机ID
 *para    : 无
 *return  : 布尔值
 */
bool EP_Read_BKP_HostID(unsigned char *sys_hostID)
{
  for(unsigned char i = 0; i < 4; i++)
    sys_hostID[i] = EEPROM1024.read(EP_SYS_HOSTID_BKP_BASE_ADDR + i);

  unsigned char ID_Verify = EEPROM1024.read(EP_SYS_HOSTID_BKP_VERIFY);
  unsigned char ID_Verify_Temp = GetCrc8(&sys_hostID[0], 4);

  if (ID_Verify != ID_Verify_Temp)
    return false;

  if (sys_hostID[0] == 0 && sys_hostID[1] == 0 && sys_hostID[2] == 0 && sys_hostID[3] == 0)
    return false;

  return true;
}

/*
 @brief   : 自检本设备的设备ID
 */
bool System_ID_Self_Check(unsigned char *sys_hostID)
{
  if (Read_BKP_HostID(sys_hostID) && Read_HostID(sys_hostID) && EP_Read_HostID(sys_hostID) && EP_Read_BKP_HostID(sys_hostID)){
    Serial.println("All Save OK...");
    delay(100);
    return true;

  }else if (Read_BKP_HostID(sys_hostID) && Read_HostID(sys_hostID)){
    Serial.println("Read host ID and host backup ID OK but read EEPROM failed!");
    EP_Save_HostID(sys_hostID);
    EP_Save_BKP_HostID(sys_hostID);
    delay(100);
    return true;

  }else if (EP_Read_HostID(sys_hostID) && EP_Read_BKP_HostID(sys_hostID)){
    Serial.println("Read EEPROM host ID OK but read host ID and host backup ID failed!");
    Save_HostID(sys_hostID);
    Save_BKP_HostID(sys_hostID);
    delay(100);
    return true;

  }else if (Read_HostID(sys_hostID)){
    Serial.println("Only read host ID OK!");
    Save_BKP_HostID(sys_hostID);
    EP_Save_HostID(sys_hostID);
    EP_Save_BKP_HostID(sys_hostID);
    delay(100);
    return true;

  }else if (Read_BKP_HostID(sys_hostID)){
    Serial.println("Only read host backup ID OK!");
    Save_HostID(sys_hostID);
    EP_Save_HostID(sys_hostID);
    EP_Save_BKP_HostID(sys_hostID);
    delay(100);
    return true;

  }else if (EP_Read_HostID(sys_hostID)){
    Serial.println("Only read EP host ID OK!");
    Save_HostID(sys_hostID);
    Save_BKP_HostID(sys_hostID);
    EP_Save_BKP_HostID(sys_hostID);
    delay(100);
    return true;

  }else if (EP_Read_BKP_HostID(sys_hostID)){
    Serial.println("Only read EP host backup ID OK!");
    Save_HostID(sys_hostID);
    Save_BKP_HostID(sys_hostID);
    EP_Save_HostID(sys_hostID);
    delay(100);
    return true;

  }else{
    Serial.println("All hostID ERROR!");
    Clear_SYS_HOSTID();
    delay(100);
    return false;
  }
}

bool Save_System_ID(unsigned char *sys_hostID)
{
  unsigned char OverTime = 0;
  unsigned char Save_Times = 0;

  do{
    if(Save_HostID(sys_hostID)){
      Serial.println("Save Reg OK...");
      delay(100);
      Save_Times++;
    }

    if (EP_Save_HostID(sys_hostID)){
      Serial.println("Save EP OK...");
      delay(100);
      Save_Times++;
    }

    if(Save_BKP_HostID(sys_hostID)){
      Serial.println("Save Reg bkp OK...");
      delay(100);
      Save_Times++;
    }

    if(EP_Save_BKP_HostID(sys_hostID)){
      Serial.println("Save EP bkp OK...");
      delay(100);
      Save_Times++;
    }

    if (Save_Times >= 2)
      return true;
    else
      OverTime++;

    Save_Times = 0;

  }while (OverTime < 3);

  return false;
}

/*
 *brief   : 清除所有ID
 *para    : 无
 *return  : 无
 */
void Clear_SYS_HOSTID(void)
{
  bkp_enable_writes();

  EP_Write_Enable();

  for (unsigned char i = 0; i < 4; i++){
    bkp_write(SYS_HOSTID_BASE_ADDR + i, 0);
    bkp_write(SYS_HOSTID_BKP_BASE_ADDR + i, 0);
    EEPROM1024.write(EP_SYS_HOSTID_BASE_ADDR + i, 0);
    EEPROM1024.write(EP_SYS_HOSTID_BKP_BASE_ADDR + i, 0);
  }
  
  delay(10);
  bkp_disable_writes();

  EP_Write_Disable();
}

void Reset_Sys_RunMode(void)
{
  EP_Write_Enable();
  EEPROM1024.write(SYS_RUN_MODE_ADDR, 0);
  EP_Write_Disable();
}

void Get_SYS_RunMode(char *runmode)
{
  *runmode = EEPROM1024.read(SYS_RUN_MODE_ADDR);
  if (*runmode < 0 || *runmode > 4)
    *runmode = 0; //enum: NWK
}

bool Save_SMS_Phone(char *phone_number)
{
  unsigned char Phone_Num_Temp[11];
  for (unsigned char i = 0; i < 11; i++)
    Phone_Num_Temp[i] = phone_number[i];

  EP_Write_Enable();
  unsigned char crc8 = GetCrc8(Phone_Num_Temp, 11);

  for (unsigned char i = 0; i < 11; i++)
    EEPROM1024.write(SYS_SMS_PHONE_BASE_ADDR + i, Phone_Num_Temp[i]);

  EEPROM1024.write(SYS_SMS_PHONE_VERIFY_ADDR, crc8);
  EP_Write_Disable();
}

bool Save_BKP_SMS_Phone(char *phone_number)
{
  unsigned char Phone_Num_Temp[11];
  for (unsigned char i = 0; i < 11; i++)
    Phone_Num_Temp[i] = phone_number[i];

  EP_Write_Enable();
  unsigned char crc8 = GetCrc8(Phone_Num_Temp, 11);
  
  for (unsigned char i = 0; i < 11; i++)
    EEPROM1024.write(SYS_BKP_SMS_PHONE_BASE_ADDR + i, Phone_Num_Temp[i]);

  EEPROM1024.write(SYS_BKP_SMS_PHONE_VERIFY_ADDR, crc8);
  EP_Write_Disable();
}

void Get_SYS_SMS_Phone_Number(char *phone_number)
{
  unsigned char phone_num_temp[11];
  unsigned char crc8;
  for (unsigned char i = 0; i < 11; i++)
    phone_num_temp[i] = EEPROM1024.read(SYS_SMS_PHONE_BASE_ADDR + i);

  crc8 = GetCrc8(phone_num_temp, 11);
  if (EEPROM1024.read(SYS_SMS_PHONE_VERIFY_ADDR) == crc8){
    for (unsigned char i = 0; i < 11; i++)
      phone_number[i] = phone_num_temp[i];
    
    Serial.println("Get Saved SMS Phone OK...");

  }else{
    for (unsigned char i = 0; i < 11; i++)
      phone_num_temp[i] = EEPROM1024.read(SYS_BKP_SMS_PHONE_BASE_ADDR + i);

    crc8 = GetCrc8(phone_num_temp, 11);
    if (EEPROM1024.read(SYS_BKP_SMS_PHONE_VERIFY_ADDR) == crc8){
      for (unsigned char i = 0; i < 11; i++)
        phone_number[i] = phone_num_temp[i];

      Serial.println("Get BKP Saved SMS Phone OK...");

      EP_Write_Enable();
      for (unsigned char i = 0; i < 11; i++)
        EEPROM1024.write(SYS_SMS_PHONE_BASE_ADDR + i, phone_num_temp[i]);

      EEPROM1024.write(SYS_SMS_PHONE_VERIFY_ADDR, crc8);

      EP_Write_Disable();
      
    }else{
      Serial.println("Get Saved and BKP Saved SMS Phone failed, use default phone!");
      for (unsigned char i = 0; i < 11; i++)
        phone_number[i] = DEFALUT_PHONE[i];
    }
  }
}

/*
 *brief   : 验证已经储存过的系统参数和目前的新参数是否相同。
 *para    : 无
 *return  : 布尔值
 */
bool Verify_Sys_Para(void)
{
  int acquisition_temp;
  int transmit_temp;
  char run_mode_temp;
  unsigned char location_flag_temp;
  char sms_phone[11];

  noInterrupts();

  acquisition_temp = EEPROM1024.read(SYS_ACQUISITION_CYCLE_HIGH_ADDR) << 8;
  acquisition_temp |= EEPROM1024.read(SYS_ACQUISITION_CYCLE_LOW_ADDR);

  transmit_temp = EEPROM1024.read(SYS_TRANSMIT_CYCLE_HIGH_ADDR) << 8;
  transmit_temp |= EEPROM1024.read(SYS_TRANSMIT_CYCLE_LOW_ADDR);

  run_mode_temp = EEPROM1024.read(SYS_RUN_MODE_ADDR);

  location_flag_temp = EEPROM1024.read(SYS_LOCATION_FLAG_ADDR);

  for (unsigned char i = 0; i < 11; i++){
    sms_phone[i] = EEPROM1024.read(SYS_SMS_PHONE_BASE_ADDR + i);
    if (sms_phone[i] != Sys_Run_Para.g_SMS_Phone[i])
      return true;
  }


  if (acquisition_temp != Sys_Run_Para.g_Acquisition_Cycle || transmit_temp != Sys_Run_Para.g_Transmit_Cycle 
  || run_mode_temp != Sys_Run_Para.g_RunMode || location_flag_temp != Sys_Run_Para.g_Location_Flag){
    interrupts();
    return true;

  }else{
    interrupts();
    return false;
  }
}

/*
 *brief   : EEPROM创建传感器数据文件
 *para    : 无
 *return  : 无
 */
void Muti_Sensor_Data_Base_Init(void)
{    
  noInterrupts();
  unsigned long Sensor_Count = 0;
  EDB_Status Sensor_Result;

  EP_Write_Enable();

  EpromDb.open(EEPROM_BASE_ADDR);

  Sensor_Count = EpromDb.count();

  if (Sensor_Count == 0){
    Serial.println("EEPROM create sensor data table");
    Sensor_Result = EpromDb.create(EEPROM_BASE_ADDR, TABLE_SIZE, (unsigned int)sizeof(Muti_Sensor_Data));
  }

  EP_Write_Disable();

  interrupts();
}

/*
 *brief   : 读读取系统相关参数
 *para    : 无
 *return  : 无
 */
void Read_EEPROM_Server_Param(unsigned char *sys_hostID, System_Run_Parameter *para)
{
  if(Clear_HostID == true){
    for (unsigned char i = 0; i < 4; i++)
      sys_hostID[i] = 0;
  }

  noInterrupts();

  para->g_Transmit_Cycle = EEPROM1024.read(SYS_TRANSMIT_CYCLE_HIGH_ADDR) << 8;
  para->g_Transmit_Cycle |= EEPROM1024.read(SYS_TRANSMIT_CYCLE_LOW_ADDR);

  para->g_Acquisition_Cycle = EEPROM1024.read(SYS_ACQUISITION_CYCLE_HIGH_ADDR) << 8;
  para->g_Acquisition_Cycle |= EEPROM1024.read(SYS_ACQUISITION_CYCLE_LOW_ADDR);

  para->g_Now_Record_Count = EEPROM1024.read(SYS_CURRENT_RECORD_HIGH_ADDR) << 8;
  para->g_Now_Record_Count |= EEPROM1024.read(SYS_CURRENT_RECORD_LOW_ADDR);

  para->g_Location_Flag =  EEPROM1024.read(SYS_LOCATION_FLAG_ADDR);
  
  interrupts();
}

/*
 *brief   : 将采集的数据保存到EEPROM中
 *para    : 无
 *return  : 无
 */
void Save_SensorData_to_EEPROM(void)
{
  Serial.print("EEPROM max record: ");
  Serial.println(EEPROM_MAX_RECORD);

  noInterrupts();

  EP_Write_Enable(); 

  EpromDb.open(EEPROM_BASE_ADDR);
  unsigned long Muti_Sensor_Data_Count = EpromDb.count();

  Serial.print("Sensor data cout: ");
  Serial.println(Muti_Sensor_Data_Count);

  if(Muti_Sensor_Data_Count >= EEPROM_MAX_RECORD)  //如果新增加的数据笔数超过EEPROM最大保存笔数
    EpromDb.clear();  //清空数据

  //如果目前数据笔数大于0笔同时小于最大笔数
  if ((Muti_Sensor_Data_Count >= 0) && (Muti_Sensor_Data_Count < EEPROM_MAX_RECORD)) {
    EDB_Status Sensor_Result = EpromDb.appendRec(EDB_REC Muti_Sensor_Data);  //增加一笔数据

    //如果增加储存数据成功
    if (Sensor_Result == EDB_OK){
      Serial.println("Save a Sensor Data done...");
      Sys_Run_Para.g_Send_EP_Data_Flag = true;
    }
    //如果增加储存数据失败，清空数据
    else{
      Serial.println("Sensor save falied!");
      Sys_Run_Para.g_Send_EP_Data_Flag = false;
      EpromDb.clear();
      interrupts();
    }
  }
  //如果目前的数据笔数小于0，EEPROM异常或损坏，不能存数据到EEPROM，只能采集一笔，发送一笔。
  else
    Sys_Run_Para.g_Send_EP_Data_Flag = false; //用来标识EEPROM损坏与否

  EP_Write_Disable();

  interrupts();
}

/*
 *brief   : 保存得到的系统参数
 *para    : 无
 *return  : 无
 */
void Save_Param_to_EEPROM(void)
{
  noInterrupts();

  EP_Write_Enable();

  EEPROM1024.write(SYS_ACQUISITION_CYCLE_HIGH_ADDR, highByte(Sys_Run_Para.g_Acquisition_Cycle));
  EEPROM1024.write(SYS_ACQUISITION_CYCLE_LOW_ADDR, lowByte(Sys_Run_Para.g_Acquisition_Cycle));

  EEPROM1024.write(SYS_TRANSMIT_CYCLE_HIGH_ADDR, highByte(Sys_Run_Para.g_Transmit_Cycle));
  EEPROM1024.write(SYS_TRANSMIT_CYCLE_LOW_ADDR, lowByte(Sys_Run_Para.g_Transmit_Cycle));

  EEPROM1024.write(SYS_LOCATION_FLAG_ADDR, Sys_Run_Para.g_Location_Flag);

  EEPROM1024.write(SYS_RUN_MODE_ADDR, Sys_Run_Para.g_RunMode);

  Save_SMS_Phone(Sys_Run_Para.g_SMS_Phone);
  Save_BKP_SMS_Phone(Sys_Run_Para.g_SMS_Phone);
  
  EP_Write_Disable();

  interrupts();
}

void Save_SMS_Param_to_EEPROM(void)
{
  noInterrupts();

  EP_Write_Enable();
  EEPROM1024.write(SYS_TRANSMIT_CYCLE_HIGH_ADDR, highByte(Sys_Run_Para.g_Transmit_Cycle));
  EEPROM1024.write(SYS_TRANSMIT_CYCLE_LOW_ADDR, lowByte(Sys_Run_Para.g_Transmit_Cycle));
  EEPROM1024.write(SYS_RUN_MODE_ADDR, Sys_Run_Para.g_RunMode);
  EP_Write_Disable();

  interrupts(); 
}

/*
 *brief   : 保存目前已发送的传感器数据笔数
 *para    : 无
 *return  : 无
 */
void Save_Sys_Current_Record(unsigned int now_record)
{
  noInterrupts();

  EP_Write_Enable();

  EEPROM1024.write(SYS_CURRENT_RECORD_HIGH_ADDR, highByte(now_record));
  EEPROM1024.write(SYS_CURRENT_RECORD_LOW_ADDR, lowByte(now_record));

  EP_Write_Disable();

  interrupts();
}

/*
 *brief   : 读取目前已发送的传感器数据笔数
 *para    : 无
 *return  : 无
 */
unsigned int Read_Sys_Current_Record(void)
{
  unsigned int Cur_Record_Temp;
  noInterrupts();
  Cur_Record_Temp = EEPROM1024.read(SYS_CURRENT_RECORD_HIGH_ADDR) << 8;
  Cur_Record_Temp |= EEPROM1024.read(SYS_CURRENT_RECORD_LOW_ADDR);
  interrupts();
  return Cur_Record_Temp;
}