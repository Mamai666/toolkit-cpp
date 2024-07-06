#ifndef CONFIGMONITOR_H
#define CONFIGMONITOR_H

#include "Utils/FileMonitor.h"
#include "json/json.hpp"

using JSON = nlohmann::json;

class ConfigMonitor : public FileMonitor
{
public:
    ConfigMonitor(const std::string &pathToFile, const uint64_t &delayCheckUpd);
    ~ConfigMonitor();

    void start(bool *pIsWorking, std::function<void (const JSON &)> callbackUpd);
    void stop();

    JSON getLastConfig() const;

    bool checkConfigNow(std::function<void(const JSON &)> callbackUpdWithArg);

private:
    using FileMonitor::checkFileChange;
    using FileMonitor::rewriteLastDump;

private:
    bool        m_isWorks;
    uint64_t    m_delayCheckUpd;

    JSON        m_lastConfigContent; // Последнее сохраненное содержимое конфига

    //std::function<void(void)> m_callbackUpdNoArg;
    //std::function<void(const JSON &)> m_callbackUpdWithArg;

};

#endif // CONFIGMONITOR_H
