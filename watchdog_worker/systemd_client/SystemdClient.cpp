#include "SystemdClient.h"
#include <unistd.h>
#include "loggerpp/include/LoggerPP.h"

SystemdClient::SystemdClient(WatchDogSettings_t wdSets, std::string moduleName,
                             std::string inConfigPath, bool* pIsWorking)
    : IWatchDogClient(wdSets, moduleName, inConfigPath, pIsWorking)
{

}

bool SystemdClient::sendMsg(std::string &errInfo)
{
    if(m_messageStatus.empty())
    {
        return false;
    }
    sd_notify(0, "WATCHDOG=1");
    std::string st = "STATUS="+m_messageStatus+"\n";
    int ret = sd_notify(0, st.c_str());
    if(ret < 0)
    {
        errInfo = "Ошибка отправки статуса "+m_messageStatus+" в SystemD!";
        //m_lastError = errInfo;
    }
    else
    {
        m_messageStatus.clear();
    }
    return (ret > 0);
}

bool SystemdClient::createMsg(const std::string& msg)
{
    m_messageStatus = msg;
    return (m_messageStatus.size() > 0);
}

bool SystemdClient::open(std::string &errInfo)
{
    m_wdDelayValue = 0;

    // В переменную m_wdTimerValue считывается значение из файла сервиса
    if(sd_watchdog_enabled(0, &m_wdTimerValue) > 0)
    {
        sd_notifyf(0, "READY=1\n"
                      "STATUS=Start processing...\n"
                      "MAINPID=%lu", (unsigned long)getpid());

        m_wdDelayValue = m_wdSets.delaySendMessageMs;
        if(m_wdDelayValue > (m_wdTimerValue / 1000) / 2)
        {
            m_wdDelayValue = (m_wdTimerValue / 1000) / 2;
            m_wdSets.delaySendMessageMs = m_wdDelayValue;
        }

        LOG(INFO) << "Включён таймер SystemD с периодом опроса "
                  << m_wdDelayValue << " мс";

        sd_notify(0, "WATCHDOG=1");
        return true;
    }
    errInfo = "Ошибка открытия systemd канала!";
   // m_lastError = errInfo;

    return false;
}

bool SystemdClient::close(std::string &errInfo)
{
    LOG(WARNING) << "У SystemdClient нет специального метода для закрытия!";
    return true;
}
