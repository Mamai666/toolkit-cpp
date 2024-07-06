#ifndef BASETRANSPORTANY_H
#define BASETRANSPORTANY_H

#include "json/TBaseJsonWork.h"
#include <chrono>
#include <cstring>
#include <string>
#include <atomic>
#include <tuple>

#include <deque>
#include <memory>
#include <mutex>

typedef std::chrono::steady_clock::time_point TimePoint;

enum class TransportState
{
    // статус успешного функционирования
    STATUS_OK = 1,
    // Ошибка соединения
    STATUS_ERROR_CONNECT = -1,
    // ошибка отправки/приема cообщения
    STATUS_ERROR_MESSAGE = -2,
    // ошибка - долго не обновляется статус
    STATUS_ERROR_UPDATE = -3,
    //Неизвестная ошибка
    STATUS_ERROR_UNDEFINED = 0
};

struct StatusTCS_t
{
    std::atomic<TransportState> status{TransportState::STATUS_ERROR_MESSAGE};
    std::atomic<TimePoint> updateTime{std::chrono::steady_clock::time_point::clock::now()};
};

static std::string nowcurrentTimeStampStr(std::string dateFormat = "%Y-%m-%dT%H:%M:%S")
{
    auto t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    auto timeUTCMks = t.count();

    auto sss = std::chrono::microseconds(timeUTCMks);
    auto tp = std::chrono::system_clock::time_point(sss);
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(tp);
    auto fraction = tp - seconds;
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fraction);

    auto in_time_t = std::chrono::system_clock::to_time_t(tp);
    char out_time[24], out[35];
    std::memset(out, 0, sizeof(out));
    std::memset(out_time, 0, sizeof(out_time));
    std::string timestampHuman = "";
    if(0 < strftime(out_time, sizeof(out_time), dateFormat.c_str(), std::localtime(&in_time_t)))
    {
        std::snprintf(out, sizeof(out), "%s.%06ld", out_time, microseconds.count());
        timestampHuman = out;
    }
    return timestampHuman;
}

struct BrokerDescription_t
{
    //std::string libraryName;
    std::string role;
    //std::string name;
    std::string credentials;

    uint32_t    maxLenQueueSend;
    uint32_t    maxLenQueueRecv;

    bool        needKeepConnect;
    uint32_t    timeoutOpenMks;
    uint64_t    maxSizeMessage;

    void operator=(const struct BrokerDescription_t& r_val)
    {
        this->role              = r_val.role;
        this->credentials       = r_val.credentials;
        this->maxLenQueueSend   = r_val.maxLenQueueSend;
        this->maxLenQueueRecv   = r_val.maxLenQueueRecv;
        this->needKeepConnect   = r_val.needKeepConnect;
        this->maxSizeMessage    = r_val.maxSizeMessage;
    }

    // Сериализация структуры в JSON
    JSON serializeToJson() {
        return {
            {"Role", this->role},
            {"Credentials", this->credentials},
            {"MaxLenQueueSend", this->maxLenQueueSend},
            {"MaxLenQueueRecv", this->maxLenQueueRecv},
            {"KeepConnect", this->needKeepConnect},
            {"MaxSizeMessage", this->maxSizeMessage}
        };
    }

    // Десериализация JSON в структуру
    void deserializeFromJson(const JSON& jsonData) {

        if(jsonData.is_null())
        {
            this->credentials = "";
            this->maxLenQueueSend = 0;
            this->maxLenQueueRecv = 0;
            this->needKeepConnect = true;
            this->maxSizeMessage = 0;
            return;
        }

        this->role            = jsonData.value("Role", "");
        this->credentials     = jsonData.value("Credentials", "");
        this->maxLenQueueSend = jsonData.value("MaxLenQueueSend", 0);
        this->maxLenQueueRecv = jsonData.value("MaxLenQueueRecv", 0);
        this->needKeepConnect = jsonData.value("KeepConnect", true);
        this->maxSizeMessage  = jsonData.value("MaxSizeMessage", 0);

    }
};

class BaseTransportAny
{
public:
    BaseTransportAny(std::string credentials,
                 std::string instanceName, // from_database, from_analytic
                 std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                );

    virtual ~BaseTransportAny();

    /**
     * @brief setP_GlobalIsWork - задать указатель на состояние работы
     * @param globalIsWork - указатель на общий флаг.
     */
    void setP_GlobalIsWork(bool *globalIsWork);

    /**
     * @brief setIsKeepConnect - установить, нужен ли авто-реконнект при закрытии соединения
     * @param newIsKeepConnect - true-нужен
     */
    void setIsKeepConnect(bool newIsKeepConnect);

    /**
     * @brief getStatus - получить текущий статус
     * @return - статус
     */
    TransportState getStatus();

    std::string credentials();

    bool localIsWork() const;
    void setLocalIsWork(bool newLocalIsWork);

    uint32_t delayMks() const;
    //void setDelayMks(uint32_t newCDelayMks); // Запретили менять setDelayMks, чтобы не было перегрузки на холостом ходу

    uint32_t waitOpenMks() const;
    void setWaitOpenMks(uint32_t newWaitOpenMks);

protected: // protected функции класса
    /**
     * @brief changeStatus - изменить текущий статус
     * @param newCurrStatus - значение нового статуса
     */
    void changeStatus(TransportState newCurrStatus);

    /**
     * @brief updateStatus - обновить время статуса
     */
    void updateStatus();

    int elapsed();

protected: // protected поля класса
    std::string m_errorInfo = "none";

    bool m_isKeepConnect = true;
    bool m_localIsWork   = false;
    bool *p_GlobalIsWork = nullptr;

private: // Приватные поля класса

    //BrokerDescription_t m_brokerDesc;
    std::string m_credentials = "none"; // proto://login:pass@host:port/path

    /**
     * @brief m_delayMks - время паузы цикла, микросекунды
     */
    uint32_t m_delayMks     = 1000;

    /**
     * @brief m_waitOpenMks - таймаут открытия соединения
     *                       (при необходимости), микросекунды
     */
    uint32_t m_waitOpenMks  = 1e5;

    StatusTCS_t m_currStatus;
    std::atomic_int m_delayStatus{1000};

};

#endif // BASETRANSPORTANY_H
