#ifndef IWATCHDOGCLIENT_H
#define IWATCHDOGCLIENT_H

#include "Utils/mstrings.h"
#include <string>
#include "message_transport/include/BaseTransportAny.h"

struct WatchDogSettings_t
{
    uint32_t    timeWaitConnectMs = 4000;
    uint32_t    delaySendMessageMs = 2000;
    std::string consumer;

    BrokerDescription_t broker;

    void operator=(const struct WatchDogSettings_t& r_val)
    {
        this->timeWaitConnectMs  = r_val.timeWaitConnectMs;
        this->delaySendMessageMs = r_val.delaySendMessageMs;
        this->consumer           = r_val.consumer;
        this->broker             = r_val.broker;
    }
};

class IWatchDogClient
{
public:
    IWatchDogClient(WatchDogSettings_t wdSets, std::string moduleName,
                    std::string inConfigPath, bool *pIsWork)
    {
        std::vector<std::string> vecSplitExe = Strings::stdv_split(moduleName, "/");
        m_moduleName = vecSplitExe.size() > 1 ? vecSplitExe.back() : moduleName;
        m_wdSets = wdSets;
        p_GlobalIsWork = pIsWork;
    }

    virtual ~IWatchDogClient(){}

    virtual bool sendMsg(std::string &errInfo) = 0;
    virtual bool createMsg(const std::string& msg) = 0;

    virtual bool open(std::string &errInfo) = 0;
    virtual bool close(std::string &errInfo) = 0;

    std::string nameWatcher() const;

    void setP_GlobalIsWork(bool *newP_GlobalIsWork);

    void setWdSets(const WatchDogSettings_t &newWdSets);
    WatchDogSettings_t wdSets() const;

protected:
    std::string m_nameWatcher = "НЕ ЗАДАНО";
    bool *p_GlobalIsWork = nullptr;

    WatchDogSettings_t m_wdSets;

    std::string m_messageStatus = "";
    std::string m_moduleName = "";
    //std::string m_credens   = ""; // rmq://mtp:mtp@localhost:5672/
};

inline std::string IWatchDogClient::nameWatcher() const
{
    return m_nameWatcher;
}

inline void IWatchDogClient::setP_GlobalIsWork(bool *newP_GlobalIsWork)
{
    p_GlobalIsWork = newP_GlobalIsWork;
}

inline void IWatchDogClient::setWdSets(const WatchDogSettings_t &newWdSets)
{
    m_wdSets = newWdSets;
}

inline WatchDogSettings_t IWatchDogClient::wdSets() const
{
    return m_wdSets;
}

#endif // IWATCHDOGCLIENT_H
