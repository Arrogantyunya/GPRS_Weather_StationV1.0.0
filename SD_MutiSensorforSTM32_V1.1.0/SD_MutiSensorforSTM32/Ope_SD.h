#ifndef _OPE_SD_H
#define _OPE_SD_H

#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h>

#define SD_CS  PB12//PA4  // chip select for SD

//------------------------------------------------------------------------------
// print error msg, any SD error codes, and halt.
// store messages in flash
#define errorExit(msg) errorHalt(F(msg))
#define initError(msg) initErrorHalt(F(msg))
//------------------------------------------------------------------------------

#define ROOT        "/"

#define MIN_WRITE_CAPACITY      102400L //100MiB

void DateTime(unsigned short *date, unsigned short *time);

enum StoreStatus{
    STORE_OK = 0, RUN_OUT_OF_STORE, STORE_EXCEPTION
};

struct SD_Ope_Param{
    bool g_Init_Flag;
    unsigned char g_Store_Sta;
    unsigned long g_Remain_Capacity;
};

extern SD_Ope_Param SD_Param;

class SD_Operation{

public:
    /**
    @brief     : 初始化SD卡。
                Initialize SD card.
    @para      : MHz for SPI
    @return    : None
    **/
    void SD_SPI_Init(unsigned char mhz);

public:
    void Mark_StoreStatus(enum StoreStatus sta);
    unsigned char Get_StoreStatus(void);

public:
    bool Read_CardType_and_Capacity(void);
    void Read_Volume(void);
    void Print_SD_Card_Info(void);

public:
    bool Create_DeviceID_Dir(unsigned char *host_id);
    bool Create_Year_Dir(unsigned short year);
    bool Create_Month_Dir(unsigned char month);
    bool Create_SensorData_File(void);
    bool Write_File_Headline(void);
    bool Write_Sensor_Data_to_SD(char *dat);

public:
    bool cd(char *path);
    void ls(bool isDetail);
    bool mkdir(char *dir_name);
    bool touch(char *file_name);
    bool rmDir(char *dir);
    bool remove(char *file);
    bool cat(char *file_name);

public:
    bool String_Analyze(unsigned char *rcv_cmd,unsigned char space_pos, unsigned char start_len, char *get_str);
    bool write_data(unsigned char *rcv_cmd);
    void Terminal_vi(unsigned char *rcv_cmd);
    void Terminal_ls(unsigned char *rcv_cmd);
    void Terminal_cd(unsigned char *rcv_cmd);
    void Terminal_mkdir(unsigned char *rcv_cmd);
    void Terminal_touch(unsigned char *rcv_cmd);
    void Terminal_rm(unsigned char *rcv_cmd);
    void Terminal_cat(unsigned char *rcv_cmd);
    void Terminal_reboot(unsigned char *rcv_cmd);

    void SD_Terminal(void);
};

extern SdFat SD;
extern SdFile Dir_File;
extern SdFile Last_dir;
extern SdFile File;

extern SD_Operation SD_Ope;

#endif