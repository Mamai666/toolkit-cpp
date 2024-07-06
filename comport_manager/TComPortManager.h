#ifndef TCOMPORTREADER_H
#define TCOMPORTREADER_H

#include <string>
#include <chrono>
#include <termios.h>

// варианты состояний устройства с COM_Port
enum class DEVICE_STATUSES {
    DEVICE_STATUS_EMPTY=0,      // отсутствует в настройках
    DEVICE_STATUS_OFF,          // отключено
    DEVICE_STATUS_ERROR,        // неисправно
    DEVICE_STATUS_ON            // в работе
};

// ------------------------------------------------------------------------------------------------
static long long int   getChrono_us(void)
{
    auto t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());
    return (long long int)t.count();
}

static long long int   getChrono_ms(void)
{
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    return (long long int)t.count();
}

static long long int   getChrono(void)
{
    auto t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch());
    return (long long int)t.count();
}

struct Device_t
{
    int fd = -1;
    std::string path; //  == /dev/ttyUSB0
    // текущее состояние устройства
    DEVICE_STATUSES status = DEVICE_STATUSES::DEVICE_STATUS_EMPTY;
};

class TComPortManager
{
public:
    TComPortManager();

    bool openPort(std::string devicePath, const speed_t speedBaud = B115200,
                  const uint8_t timeoutDeciSec = 1, const bool isBlocking = false);
    void closePort();

    int64_t readBytes(unsigned char* buff, size_t numByte, int mlsec_timeout);
    int64_t writeBytes(unsigned char *buff, size_t numByte);

    bool checkAlivePort();

    void setTimeActivity(long long TimeActivity);
    void setErrorCountRead(int ErrorCountRead);
    void setErrorCountWrite(int newErrorCountWrite);

    long long getTimeActivity() const;
    int getErrorCountRead() const;

    DEVICE_STATUSES getDeviceStatus() const;
    void setDeviceStatus(const DEVICE_STATUSES &deviceStatus);

    std::string getDevicePath() const;

    void setMaxWaitBytesSec(int MaxWaitBytesSec);
    void setMaxErrorCountRead(int MaxErrorCountRead);
    void setMaxErrorCountWrite(int MaxErrorCountWrite);

private:
    bool setPortOptions(const speed_t speed, const uint8_t timeoutDeciSec, const bool isBlocking);

private:

    Device_t m_device;

    // время подключения или последнего полученного байта
    long long int m_TimeActivity = 0;

    // счетчик получения ошибочных байтов
    int     m_ErrorCountRead = 0;
    int     m_ErrorCountWrite = 0;
    int     m_WaitOpenCount = 0;

    int m_MaxWaitBytesSec    = -1;
    //int m_MaxTimeWaitOpenSec = 5;
    int m_MaxErrorCountRead  = 1;
    int m_MaxErrorCountWrite = 1;
};

#endif // TCOMPORTREADER_H
