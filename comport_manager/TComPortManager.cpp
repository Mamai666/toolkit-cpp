#include "TComPortManager.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "loggerpp/include/LoggerPP.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/poll.h>

TComPortManager::TComPortManager()
{

}

bool TComPortManager::setPortOptions(const speed_t speed, const uint8_t timeoutDeciSec, const bool isBlocking)
{
    struct termios options;                     // структура для установки порта
    // speed: скорость, например: B9600, B57600, B115200

    if (tcgetattr(m_device.fd, &options) < 0) {        // читает пораметры порта
        LOG(ERROR) << "Не удалось получить параметры COM-порта " << m_device.path;
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
        return false;
    }

    if (cfsetispeed(&options, speed) < 0) {     // установка скорости порта
        LOG(ERROR) << "Не удалось установить входящую скорость COM-порта " << m_device.path;
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
        return false;
    }
    if (cfsetospeed(&options, speed) < 0) {     // установка скорости порта
        LOG(ERROR) << "Не удалось установить исходящую скорость COM-порта " << m_device.path;
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
        return false;
    }

    options.c_cc[VTIME] = timeoutDeciSec;        // Время ожидания байтов в 0.1 секунда
    options.c_cc[VMIN] = isBlocking ? 1 : 0;     // минимальное число байт для чтения

    options.c_cflag &= ~PARENB;     // бит четности не используется
    options.c_cflag &= ~CSTOPB;     // 1 стоп бит
    options.c_cflag &= ~CSIZE;      // Размер байта
    options.c_cflag |= CS8;         // 8 бит

    options.c_cflag &= ~CRTSCTS;    // Отключение управления RTS/CTS
    options.c_cflag &= ~HUPCL;      // Отключение управления DSR/DTR

options.c_iflag &= ~(IXON | IXOFF | IXANY);

options.c_iflag &= ~ICRNL; // Отключение параметра icrnl
options.c_oflag &= ~ONLCR; // Отключение параметра onlcr

// Включение приема (RX) и передачи (TX)
    options.c_cflag |= CREAD | CLOCAL;

    //options.c_lflag = 0;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Отключение канонического режима и эхо
    options.c_oflag &= ~OPOST;      // Обязательно отключить постобработку

    if (tcsetattr(m_device.fd, TCSANOW, &options) < 0) {   // сохранения параметров порта
        LOG(ERROR) << "Не удалось установить параметры COM-порта " << m_device.path;
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
        return false;
    }
    return true;
}

void TComPortManager::setErrorCountWrite(int newErrorCountWrite)
{
    m_ErrorCountWrite = newErrorCountWrite;
}

void TComPortManager::setMaxErrorCountWrite(int MaxErrorCountWrite)
{
    m_MaxErrorCountWrite = MaxErrorCountWrite;
}

void TComPortManager::setDeviceStatus(const DEVICE_STATUSES &deviceStatus)
{
    m_device.status = deviceStatus;
}

std::string TComPortManager::getDevicePath() const
{
    return m_device.path;
}

DEVICE_STATUSES TComPortManager::getDeviceStatus() const
{
    return m_device.status;
}

int TComPortManager::getErrorCountRead() const
{
    return m_ErrorCountRead;
}

long long TComPortManager::getTimeActivity() const
{
    return m_TimeActivity;
}

void TComPortManager::setErrorCountRead(int ErrorCountRead)
{
    m_ErrorCountRead = ErrorCountRead;
}

void TComPortManager::setTimeActivity(long long TimeActivity)
{
    m_TimeActivity = TimeActivity;
}

bool TComPortManager::checkAlivePort()
{
    if (m_device.fd >= 0) {
        // COM-порт уже открыт - проверить актуальность
        if(m_MaxWaitBytesSec > 0) {
            long long int currTime = getChrono();
            if (currTime - m_TimeActivity > m_MaxWaitBytesSec) {
                // долго нет новых данных
                m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
                LOG(ERROR) << "Отключение устройства по таймауту (байты не получены), секунд: " << currTime - m_TimeActivity;
                closePort();
                return false;
            }
        }

        if (m_ErrorCountRead > m_MaxErrorCountRead) {
            m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
            LOG(ERROR) << "Отключение устройства по количеству ошибок чтения: " << m_ErrorCountRead << " > " << m_MaxErrorCountRead;
            closePort();
            return false;
        }
        else if (m_ErrorCountWrite > m_MaxErrorCountWrite) {
            m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ERROR;
            LOG(ERROR) << "Отключение устройства по количеству ошибок записи: " << m_ErrorCountWrite << " > " << m_MaxErrorCountWrite;
            closePort();
            return false;
        }

        return true;
    }
    return false;
}

bool TComPortManager::openPort(std::string devicePath, const speed_t speedBaud,
                               const uint8_t timeoutDeciSec, const bool isBlocking)
{
    m_device.path     = devicePath;
    if (m_device.fd >= 0) {
        closePort();
    }

//    if (m_WaitOpenCount > 0) {
//        m_WaitOpenCount--;
//        return false;
//    }

    if (devicePath.empty()) { // COM-порт не указан
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_EMPTY;
        return false;
    }

    // m_devicePath: путь к устройству, например: /dev/ttyS0 или  /dev/ttyUSB0 - для USB-Uart
    m_device.fd = open(m_device.path.c_str(), O_RDWR | O_NOCTTY);
    if (m_device.fd == -1) {
        LOG(ERROR) << "Не удалось открыть com-порт " << m_device.path;
        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_OFF;
        return false;
    }

    bool retOK = setPortOptions(speedBaud, timeoutDeciSec, isBlocking);
    if(retOK) {

        LOG(INFO) << m_device.path << " настроен и открыт";

        m_device.status = DEVICE_STATUSES::DEVICE_STATUS_ON;
        // сбросить счетчик ошибок при получении неправильных байтов
        m_ErrorCountRead = 0;
        m_TimeActivity = getChrono();

        tcflush(m_device.fd,TCIOFLUSH);

        return true;
    }
    return false;
}

int64_t TComPortManager::readBytes(unsigned char* buff, size_t numByte, int mlsec_timeout)
{
    using namespace std::chrono;

    struct pollfd fd = { .fd = m_device.fd, .events = POLLIN };
    int bytesread = 0;
    auto start_time = steady_clock::now();

    while (bytesread < numByte)
    {
        int remaining_timeout = mlsec_timeout - duration_cast<milliseconds>(steady_clock::now() - start_time).count();
        if (remaining_timeout <= 0) {
            LOG(DEBUG) << "Истекло время ожидания пакета.."; /* a timeout occured */
            break;
        }

        int poll_result = poll(&fd, 1, remaining_timeout);
        if (poll_result == 1)
        {
            if (fd.revents & POLLIN)
            {
                int result = read(m_device.fd, buff + bytesread, numByte - bytesread);
                if (result == -1) {
                    LOG(ERROR) << "read fd error: " << errno; /* an error occurred */
                    bytesread = -1;
                    break;
                }
                else if (result == 0) {
                    // No more data available to read
                    break;
                }
                else {
                    bytesread += result;
                    start_time = steady_clock::now(); // Reset timeout
                }
            }
        }
        else if (poll_result == -1)
        {
            if (errno != EINTR) {
                LOG(ERROR) << "poll error: " << errno; /* an error occurred */
                bytesread = -1;
                break;
            }
        }
        else {
            // poll_result == 0, timeout occurred
            LOG(DEBUG) << "Истекло время ожидания пакета.."; /* a timeout occurred */
            break;
        }
    }

    if(bytesread < numByte) {
        m_ErrorCountRead++;
    }
    else {
        m_ErrorCountRead = 0;
    }

    m_TimeActivity = getChrono();

    return bytesread;
}

int64_t TComPortManager::writeBytes(unsigned char* buff, size_t numByte)
{
    auto n = write(m_device.fd, buff, numByte);
    if (n == -1) {
        closePort();
    }
    else if(n < numByte) {
        m_ErrorCountWrite++;
    }
    else {
        m_ErrorCountWrite = 0;
    }

    m_TimeActivity = getChrono();

    return n;
}

void TComPortManager::setMaxWaitBytesSec(int MaxWaitBytesSec)
{
    m_MaxWaitBytesSec = MaxWaitBytesSec;
}

void TComPortManager::setMaxErrorCountRead(int MaxErrorCountRead)
{
    m_MaxErrorCountRead = MaxErrorCountRead;
}

void TComPortManager::closePort(void)
{
    if (m_device.fd >= 0) {
        auto codeClose = close(m_device.fd);
        LOG(INFO) << m_device.path << " был закрыт с кодом : " << codeClose;
    }
    m_device.fd = -1;

    m_device.status = DEVICE_STATUSES::DEVICE_STATUS_OFF;
    m_TimeActivity = 0;
    m_ErrorCountRead = 0;
    //m_WaitOpenCount = m_MaxTimeWaitOpenSec;
}
