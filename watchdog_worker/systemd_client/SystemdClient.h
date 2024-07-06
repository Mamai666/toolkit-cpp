#ifndef SYSTEMDCLIENT_H
#define SYSTEMDCLIENT_H

#include "watchdog_worker/IWatchDogClient.h"
#include <systemd/sd-daemon.h>
#include <string>

class SystemdClient : public IWatchDogClient
{
public:
    SystemdClient(WatchDogSettings_t wdSets, std::string moduleName,
                  std::string inConfigPath, bool *pIsWorking);
    ~SystemdClient(){}

    bool sendMsg(std::string &errInfo);
    bool createMsg(const std::string& msg);

    bool open(std::string &errInfo);
    bool close(std::string &errInfo);

    //void setWdDelayValue(uint64_t newWdDelayValue);

private:
    std::string m_messageStatus = "none";

    uint64_t m_wdTimerValue = 0;
    uint64_t m_wdDelayValue = 0;

    //std::string m_lastError;
    //std::string m_lastDebugInfo;

};

#endif // SYSTEMDWATCHDOGCLIENT_H
