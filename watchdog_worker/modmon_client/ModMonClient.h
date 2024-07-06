#ifndef MODMONCLIENT_H
#define MODMONCLIENT_H

#include <string>
#include "watchdog_worker/IWatchDogClient.h"
#include "message_transport/include/BaseTransportClient.h"

#include "loggerpp/include/LoggerPP.h"

class ModMonClient : public IWatchDogClient
{
public:
    ModMonClient(WatchDogSettings_t wdSets, std::string moduleName,
                 std::string inConfigPath, bool *pIsWork);
    ~ModMonClient();

    virtual bool sendMsg(std::string &errInfo);
    virtual bool createMsg(const std::string& msg);

    virtual bool open(std::string &errInfo);
    virtual bool close(std::string &errInfo);

private:
    std::string m_queueName = "";
    BrokerDescription_t m_brokerSets;
//    RabbitTransportClient *rmqClient = nullptr;
    BaseTransportClient *m_pTransClient = nullptr;

    std::thread m_transClientThr;

    LoggerPP::DispatchLogger* m_logDispatch = nullptr;
};

#endif // MODMONCLIENT_H
