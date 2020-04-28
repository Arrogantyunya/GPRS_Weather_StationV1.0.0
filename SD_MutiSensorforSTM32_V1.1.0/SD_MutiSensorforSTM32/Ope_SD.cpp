#include "Ope_SD.h"
#include "Private_RTC.h"

//SdFat SD;
SPIClass SPI_2(2);
SdFat SD(&SPI_2);

SdFile Dir_File;
SdFile Last_dir;
SdFile File;

SD_Ope_Param SD_Param;

SD_Operation SD_Ope;

const char *Weather_Headline = "Date,Temperature(C),Humidity(%),Lumination(Lux),Barometric(Kpa),UV_Intensity(W/cm),CO2(ppm),TVOC(ppm),Is_Rainy_Or_Snowy,Wind_Speed(m/s),Wind_Direction,Water_Level(ADC),Extended_ADC_Value_Channel2(ADC),Extended_ADC_Value_Channel3(ADC)\n";

char Last_Path_Buffer[256] = {0};

volatile bool Last_Dir_Flag = true;
volatile unsigned char Wait_Recod_Last = 0;

bool g_write_flag = false;

void SD_Operation::SD_SPI_Init(unsigned char mhz)
{
    SD_Param.g_Init_Flag = false;
    Mark_StoreStatus(STORE_EXCEPTION);

    if (SD.begin(SD_CS, SD_SCK_MHZ(mhz)))
        SD_Param.g_Init_Flag = true;
    else
        SD_Param.g_Init_Flag = false;
}

void SD_Operation::Mark_StoreStatus(enum StoreStatus sta)
{
    SD_Param.g_Store_Sta = sta;
}

unsigned char SD_Operation::Get_StoreStatus(void)
{
    return SD_Param.g_Store_Sta;
}

bool SD_Operation::Read_CardType_and_Capacity(void)
{
    csd_t CSD;
    unsigned long CardSize;

    if (!SD_Param.g_Init_Flag) return false;

    if (!SD.card()->readCSD(&CSD)){
        Serial.println("Read CSD information failed!");
        return false;
    }else{
        CardSize = SD.card()->cardSize();

        //Print SD card type.
        Serial.print("->SD Card type: ");
        switch (SD.card()->type()) {
            case SD_CARD_TYPE_SD1 : Serial.println("SD1");    break;
            case SD_CARD_TYPE_SD2 : Serial.println("SD2");    break;
            case SD_CARD_TYPE_SDHC: CardSize < 70000000 ? Serial.println("SDHC") : Serial.println("SDXC"); break;
            default :   Serial.println("Unknown!");
        }
        
        //Print the total capacity of SD card.
        unsigned long CardSize_kb = (CardSize /= 1024,  CardSize *= 512);
        unsigned int CardSize_mb  = CardSize_kb / 1024;
        float CardSize_gb = (float)CardSize_mb / 1024.0;
        Serial.print("->SD card capacity: "); 
        Serial.print(CardSize_gb); Serial.println(" GiB");

        //Print the remaining capacity of SD card.
        unsigned long volFree = SD.vol()->freeClusterCount();
        float fs = volFree * SD.vol()->blocksPerCluster() / 1024;
        fs *= 512;
        SD_Param.g_Remain_Capacity = fs;
        Serial.print("->Free capacity = ");
        Serial.print(fs);
        Serial.println(" KiB");
        fs /= 1024;
        Serial.print("                = ");
        Serial.print(fs);
        Serial.println(" MiB");
        fs /= 1024;
        Serial.print("                = ");
        Serial.print(fs);
        Serial.println(" GiB");
    }
    return true;
}

void SD_Operation::Read_Volume(void)
{
    Serial.print("->Volume: FAT"); Serial.println(int(SD.vol()->fatType()));
}

void SD_Operation::Print_SD_Card_Info(void)
{
    Serial.println("-------------------------------------------------------");
    Serial.println("                SD Card Information                    \n");
    Read_CardType_and_Capacity();
    Read_Volume();
    Serial.println("-------------------------------------------------------");
}

/*
 *brief     : 回调函数。使用本设备提供的RTC，可以记录目录、文件的创建时间和更新时间。
              Callback function. Using the RTC provided by this device, you can 
              record the creation time and update time of the directory and files.
 */
void DateTime(unsigned short *date, unsigned short *time)
{
    unsigned char rcv_date[7];
    unsigned short year;
    unsigned char month, day, hour, minute, second;

    Get_RTC(rcv_date);
    year = rcv_date[0] * 100 + rcv_date[1];
    month = rcv_date[2];
    day = rcv_date[3];
    hour = rcv_date[4];
    minute = rcv_date[5];
    second = rcv_date[6];

    *date = FAT_DATE(year, month, day);
    *time = FAT_TIME(hour, minute, second);
}

/*
 *brief     : 切换当前目录路径
              Change the current directory path.
 *brief     : path.
 *return    : true or false.
 */
bool SD_Operation::cd(char *path)
{
    Serial.write(path);

    if (!SD.chdir(path)){
        return false;
    }
    else{
        Wait_Recod_Last = 2;
        return true;
    }
}

void SD_Operation::ls(bool isDetail = false)
{
    isDetail == true ? SD.ls(LS_DATE | LS_SIZE) : SD.ls();
    Serial.println();
}

bool SD_Operation::mkdir(char *dir_name)
{
    //创建一个目录，如果它已经存在，忽略创建
    if (!SD.exists(dir_name)){
        SdFile::dateTimeCallback(DateTime);
        if (!SD.mkdir(dir_name)){
            return false;
        }
    }
    return true;
}

bool SD_Operation::touch(char *file_name)
{
    if (SD.exists(file_name)){
        if (File.isOpen())
            File.close();

        Serial.println("The file already exists.");
        if (!File.open(file_name, O_RDWR))
            return false;
    }else{
        SdFile::dateTimeCallback(DateTime);
        if (!File.open(file_name, O_RDWR | O_CREAT | O_TRUNC))
            return false;
    }
    File.close();
    return true;
}

bool SD_Operation::rmDir(char *dir_link)
{
    if (Dir_File.isOpen())
        Dir_File.close();

    if(!Dir_File.open(dir_link))
        return false;

    if(!Dir_File.rmRfStar()){
        Dir_File.close();
        return false;
    }

    Dir_File.close();
    return true;
}

bool SD_Operation::remove(char *file_name)
{
    if (File.isOpen())
        File.close();

    if (!File.open(file_name, O_RDWR))
        return false;

    if(!File.remove()){
        File.close();
        return false;
    }
    return true;
}

bool SD_Operation::Create_DeviceID_Dir(unsigned char *host_id)
{
    if (SD_Param.g_Init_Flag == false){
        Mark_StoreStatus(STORE_EXCEPTION);
        return false;
    }
    
    char DeviceID_Name[9] = {0};
    for (unsigned char i = 0; i < 8; i++)
        i % 2 == 0 ? DeviceID_Name[i] = '0' : DeviceID_Name[i] = host_id[i / 2] + '0';

    if (!SD.exists(DeviceID_Name)){
        if (!SD.mkdir(DeviceID_Name)){
            Serial.println("Create deviceID name directory failed! <Create_DeviceID_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }else{
            SdFile::dateTimeCallback(DateTime);
            Serial.println("Create deviceID name directory success... <Create_DeviceID_Dir>");
            Serial.flush();
            if (SD.chdir(DeviceID_Name)){
                Serial.println("Open the directory success... <Create_DeviceID_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_OK);
            }else{
                Serial.println("Open the directory failed! <Create_DeviceID_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_EXCEPTION);
                return false;  
            }
        }
    }else{
        Serial.write(DeviceID_Name);
        Serial.println(" directory already exist...");
        Serial.flush();
        if (SD.chdir(DeviceID_Name)){
            Serial.println("Open the directory success...");
            Serial.flush();
            Mark_StoreStatus(STORE_OK);
        }else{
            Serial.println("Open the directory failed!");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }
    }
    return true;
}

bool SD_Operation::Create_Year_Dir(unsigned short year)
{
    if (Get_StoreStatus() == STORE_EXCEPTION) return false;

    char Year_Name[5] = {0};
    Year_Name[0] = year / 1000 + '0';
    Year_Name[1] = year % 1000 / 100 + '0';
    Year_Name[2] = year % 1000 % 100 / 10 + '0';
    Year_Name[3] = year % 10 + '0';

    if (!SD.exists(Year_Name)){
        if (!SD.mkdir(Year_Name)){
            Serial.println("Create year directory failed! <Create_Year_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }else{
            SdFile::dateTimeCallback(DateTime);
            Serial.println("Create year directory success... <Create_Year_Dir>");
            Serial.flush();
            if (SD.chdir(Year_Name)){
                Serial.println("Open the directory success... <Create_Year_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_OK);
            }else{
                Serial.println("Open the directory failed! <Create_Year_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_EXCEPTION);
                return false;  
            }
        }
    }else{
        Serial.write(Year_Name);
        Serial.println(" directory already exist... <Create_Year_Dir>");
        Serial.flush();
        if (SD.chdir(Year_Name)){
            Serial.println("Open the directory success... <Create_Year_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_OK);
        }else{
            Serial.println("Open the directory failed! <Create_Year_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }
    }
    return true;
}

bool SD_Operation::Create_Month_Dir(unsigned char month)
{
    if (Get_StoreStatus() == STORE_EXCEPTION) return false;

    char Month_Name[3] = {0};
    Month_Name[0] = month / 10 + '0';
    Month_Name[1] = month % 10 + '0';

    if (!SD.exists(Month_Name)){
        if (!SD.mkdir(Month_Name)){
            Serial.println("Create month directory failed! <Create_Month_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }else{
            SdFile::dateTimeCallback(DateTime);
            Serial.println("Create month directory success... <Create_Month_Dir>");
            Serial.flush();
            if (SD.chdir(Month_Name)){
                Serial.println("Open the directory success... <Create_Month_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_OK);
            }else{
                Serial.println("Open the directory failed! <Create_Month_Dir>");
                Serial.flush();
                Mark_StoreStatus(STORE_EXCEPTION);
                return false;  
            }                
        }
    }else{
        Serial.write(Month_Name);
        Serial.println(" directory already exist... <Create_Month_Dir>");
        Serial.flush();
        if (SD.chdir(Month_Name)){
            Serial.println("Open the directory success... <Create_Month_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_OK);
        }else{
            Serial.println("Open the directory failed! <Create_Month_Dir>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }
    }
    return true;
}


bool SD_Operation::Create_SensorData_File(void)
{
    if (Get_StoreStatus() == STORE_EXCEPTION) return false;

    const char *File_Name = "SensorData.csv";

    if (File.isOpen())
        File.close();

    if (!SD.exists(File_Name)){
        if (!File.open(File_Name, O_RDWR | O_CREAT | O_TRUNC)){
            Serial.println("Create day file failed! <Create_SensorData_File>");
            Serial.flush();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }else{
            SdFile::dateTimeCallback(DateTime);
            Serial.println("Create day file success... <Create_SensorData_File>");
            Serial.flush();
            File.close();       
            Mark_StoreStatus(STORE_OK);     
        }
    }else{
        Serial.write(File_Name);
        Serial.println(" file already exist... <Create_SensorData_File>");
        Serial.flush();
        Mark_StoreStatus(STORE_OK);
    }        
    return true;
}

bool SD_Operation::Write_File_Headline(void)
{
    if (Get_StoreStatus() == STORE_EXCEPTION) return false;

    const char *File_Name = "SensorData.csv";

    if (File.isOpen())  File.close();

    if(File.open(File_Name, O_RDWR | O_AT_END)){
        if(File.fileSize() == 0){
            Serial.println("Open <SensorData.csv> file success, prepare to write headline... <Write_File_Headline>");
            Serial.flush();
            if(File.write(Weather_Headline) == -1){
                Serial.println("Write headline failed! <Write_File_Headline>");
                Serial.flush();
                File.close();
                Mark_StoreStatus(STORE_EXCEPTION);
                return false;
            }else{
                SdFile::dateTimeCallback(DateTime);
                Serial.println("Write headline success... <Write_File_Headline>");
                Serial.flush();
                File.close();
                Mark_StoreStatus(STORE_OK);
                return true;
            }
        }else{
            Serial.println("Open <SensorData.csv> file success, headline has exist... <Write_File_Headline>");
            Serial.flush();
            File.close();   
            Mark_StoreStatus(STORE_OK);                
            return true; 
        }
    }else{
        Serial.println("Open <SensorData.csv> file failed! <Write_File_Headline>");
        Serial.flush();
        Mark_StoreStatus(STORE_EXCEPTION);
        return false;
    }
}

bool SD_Operation::Write_Sensor_Data_to_SD(char *dat)
{
    if (Get_StoreStatus() == STORE_EXCEPTION) return false;
    if (SD_Param.g_Remain_Capacity < MIN_WRITE_CAPACITY){
        Mark_StoreStatus(RUN_OUT_OF_STORE);
        return false;
    }

    const char *File_Name = "SensorData.csv";

    if (File.isOpen())  File.close();

    if(File.open(File_Name, O_RDWR | O_AT_END)){
        if(File.write(dat) == -1){
            Serial.println("Write sensor data failed! <Write_Sensor_Data_to_SD>");
            Serial.flush();
            File.close();
            Mark_StoreStatus(STORE_EXCEPTION);
            return false;
        }else{
            SdFile::dateTimeCallback(DateTime);
            Serial.println("Write sensor data success... <Write_Sensor_Data_to_SD>");
            Serial.flush();
            File.close();
            Mark_StoreStatus(STORE_OK);
            return true;
        }
    }else{
        Serial.println("Open <SensorData.csv> file failed!!! <Write_Sensor_Data_to_SD>");
        Mark_StoreStatus(STORE_EXCEPTION);
        return false;
    }
}


bool SD_Operation::write_data(unsigned char *rcv_cmd)
{
    unsigned int i = 0;

    while (1){
        if (rcv_cmd[i] == '\0')
            break;
        i++;
    }

    if (i == 0)
        return false;

    char data[i];
    for (unsigned int j = 0; j < i; j++)
        data[j] = rcv_cmd[j];

    data[i] = '\0';

    SdFile::dateTimeCallback(DateTime);
    if(File.write(data) == -1){
        Serial.println("Write data failed!");
        Serial.flush();
        return false;
    }
    return true;
}

bool SD_Operation::String_Analyze(unsigned char *rcv_cmd,unsigned char space_pos, unsigned char start_len, char *get_str)
{
    if (rcv_cmd[space_pos] == ' '){

        unsigned char i = start_len;
        while (1){
            if (rcv_cmd[i] == '\r' && rcv_cmd[i + 1] == '\n')
                break;
            if (i >= 128)
                return false;
            i++;
        }
        i -= start_len;
        if (i == 0){
            Serial.println("^The string cannot be empty");
            return false;
        }

        for (unsigned char j = 0; j < i; j++)
            get_str[j] = rcv_cmd[start_len + j];

        get_str[i] = '\0';
        return true;

    }else{
            Serial.println("^Error !!!");
            Serial.println("^There must be SPACES between instructions and parameters");
            return false;
        }
}

void SD_Operation::Terminal_vi(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'v' && rcv_cmd[1] == 'i'){
        char file_name[128] = {0};
        unsigned char mode = 0;
        bool file_status = false;

        //Cover redact mode.
        if (rcv_cmd[3] == '-' && rcv_cmd[4] == 'c'){
            if(!String_Analyze(rcv_cmd, 2, 6, file_name))
                return;
            mode = 1;
        }
        //End redact mode.
        else if (rcv_cmd[3] == '-' && rcv_cmd[4] == 'e'){
            if(!String_Analyze(rcv_cmd, 2, 6, file_name))
                return;
            mode = 2;
        }
        //Default redact mode (End redact mode)
        else{
            if(!String_Analyze(rcv_cmd, 2, 3, file_name))
                return;
            mode = 2;            
        }
        
        if (File.isOpen())
        File.close();

        switch (mode){
            case 1 : file_status = File.open(file_name, O_RDWR); break;
            case 2 : file_status = File.open(file_name, O_RDWR | O_AT_END); break;
        }
        if (!file_status){
            Serial.println("Open file failed!");
            return;            
        }else{
            Serial.println("Waiting for write data...\n");
            g_write_flag = true;            
        }
    }
}

bool SD_Operation::cat(char *file_name)
{
    if (File.isOpen())
        File.close();

    if (!File.open(file_name, O_RDONLY)){
        Serial.println("Open file failed!");
        return false;               
    }

    char Read_Data[2048] = {0};
    unsigned char copy_Read_Data[2048] = {0};
    unsigned int i = 0;
    unsigned int num = 0;

    while (1){
        Read_Data[i] = File.read();
        num++;

        if ((Read_Data[i] == -1) && (i > 0)){
            i--;
            break;
        }else if ((Read_Data[i] == -1) && (i < 1)){
            File.close();
            Serial.println("Read data failed!");
            return false;
        }
        
        if (Read_Data[i] >= 128){
            i--;
            break;
        }

        i++;
        if (i >= 2048){
            Serial.println("Data overflow!");
            i--;
            break;
        }
    }
    
    for (unsigned int j = 0; j < i; j++)
        copy_Read_Data[j] = Read_Data[j];

    Serial.print("---> ");
    Serial.write(copy_Read_Data, i);
    Serial.println(" <---");
    File.close();
    return true;
}

void SD_Operation::Terminal_cat(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'c' && rcv_cmd[1] == 'a' && rcv_cmd[2] == 't'){
        char file_name[128] = {0};
        if (!String_Analyze(rcv_cmd, 3, 4, file_name))
            return;

        cat(file_name);
    }
}

void SD_Operation::Terminal_ls(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'l' && rcv_cmd[1] == 's'){
        if (rcv_cmd[2] == ' ' && rcv_cmd[3]){
            if (rcv_cmd[4] == 'l')
                ls(true);
            else
                Serial.println("^Pameramter Error !!!");
        }else{
            ls();
        }
    }
}

void SD_Operation::Terminal_cd(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'c' && rcv_cmd[1] == 'd'){
        char dir_path[128] = {0};
        if (rcv_cmd[2] == ' ' && rcv_cmd[3] == '.' && rcv_cmd[4] == '.'){
            if(!cd(Last_Path_Buffer)){
                Serial.println("^Pameramter Error !!!");
            }
            return;
        }

        if (!String_Analyze(rcv_cmd, 2, 3, dir_path))
            return;

        if(!cd(dir_path)){
            Serial.println("^Pameramter Error !!!");
        }
    }
}

void SD_Operation::Terminal_mkdir(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'm' && rcv_cmd[1] == 'k' && rcv_cmd[2] == 'd' && rcv_cmd[3] == 'i' && rcv_cmd[4] == 'r'){
        char dir_name[128] = {0};
        if (!String_Analyze(rcv_cmd, 5, 6, dir_name))
            return;

        if (!mkdir(dir_name))
            Serial.println("^Pameramter Error!");
    }
}

void SD_Operation::Terminal_touch(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 't' && rcv_cmd[1] == 'o' && rcv_cmd[2] == 'u' && rcv_cmd[3] == 'c' && rcv_cmd[4] == 'h'){
        char file_name[128] = {0};
        if (!String_Analyze(rcv_cmd, 5, 6, file_name))
            return;

        if (!touch(file_name))
            Serial.println("^Pameramter Error!");
    }
}

void SD_Operation::Terminal_rm(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'r' && rcv_cmd[1] == 'm'){
        //删除目录
        if (rcv_cmd[3] == '-' && rcv_cmd[4] == 'r'){
            char dir_name[128] = {0};
            if (!String_Analyze(rcv_cmd, 2, 6, dir_name))
                return;

            if (!rmDir(dir_name))
                Serial.println("^Pameramter Error!");   
        }   
        //删除文件
        else{
            char file_name[128] = {0};
            if (!String_Analyze(rcv_cmd, 2, 3, file_name))
                return;

            if (!remove(file_name))
                Serial.println("^Pameramter Error!");               
        }
    }
}

void SD_Operation::Terminal_reboot(unsigned char *rcv_cmd)
{
    if (rcv_cmd[0] == 'r' && rcv_cmd[1] == 'e' && rcv_cmd[2] == 'b'
       && rcv_cmd[3] == 'o' && rcv_cmd[4] == 'o' && rcv_cmd[5] == 't'){
           Serial.println("Prepare reboot...");
           delay(500);
           nvic_sys_reset();
       }
}

void SD_Operation::SD_Terminal(void)
{
    unsigned char rcv_cmd[2048] = {0};
    unsigned int rcv_len = 0;

    while (Serial.available() > 0){
        rcv_cmd[rcv_len++] = Serial.read();
        if (rcv_len >= 2048){
            rcv_len = 0;
            Serial.println("^Command length limit exceeded !");
            return;
        }
    }

    if (rcv_len > 0){
        if (rcv_cmd[rcv_len - 1] == '\n' && rcv_cmd[rcv_len - 2] == '\r'){
            rcv_len = 0;
            if (g_write_flag == true){
                write_data(rcv_cmd);
                File.close();
                g_write_flag = false;
                memset(rcv_cmd, 0x00, sizeof(rcv_cmd));

            }else{
                Terminal_ls(rcv_cmd);
                Terminal_cd(rcv_cmd);
                Terminal_mkdir(rcv_cmd);
                Terminal_touch(rcv_cmd);
                Terminal_rm(rcv_cmd);
                Terminal_vi(rcv_cmd);
                Terminal_cat(rcv_cmd);
                Terminal_reboot(rcv_cmd);
            }
        }else{
            Serial.println("End Error!");
            rcv_len = 0;
        }

        memset(rcv_cmd, 0x00, sizeof(rcv_cmd));
    }
}