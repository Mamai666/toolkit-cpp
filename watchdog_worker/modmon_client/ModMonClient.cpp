#include "ModMonClient.h"
#include "Utils/files.h"

#include "json/TBaseJsonWork.h"
#include "Utils/time_convert.h"
#include <unistd.h>

ModMonClient::ModMonClient(WatchDogSettings_t wdSets, std::string moduleName,
                           std::string inConfigPath, bool *pIsWork)
    : IWatchDogClient(wdSets, moduleName, inConfigPath, pIsWork)
{
    m_queueName = m_moduleName + "@" + Files::basenameNoSuffix(inConfigPath)+"@events";
    LOG(INFO) << "Имя очереди для отправки статусов: " << m_queueName;
}

ModMonClient::~ModMonClient()
{
    if(m_logDispatch)
        delete m_logDispatch;

    if(m_transClientThr.joinable())
        m_transClientThr.join();

    if(m_pTransClient)
        delete m_pTransClient;
}

bool ModMonClient::sendMsg(std::string &errInfo)
{
    if(m_messageStatus.empty())
    {
        return false;
    }

    LOG(DEBUG) << "Sending msg: " << m_messageStatus;

    m_pTransClient->send((uint8_t*)m_messageStatus.c_str(), m_messageStatus.size());
    m_messageStatus.clear();

    return true;
}

bool ModMonClient::createMsg(const std::string& msg)
{
    JSON msgJson;

    msgJson["MainPID"] = getpid();
    msgJson["Timestamp"] = MTime::nowTimeStamp("%Y-%m-%d %H:%M:%S");
    msgJson["Status"] = "Keepalive";

    if(msg == "OK")
    {
        msgJson["Status"] = "Keepalive";
    }
    else if(msg == "Module Start")
    {
        msgJson["Status"] = "ModuleStarted";
        msgJson["StartDelayMs"] = 4000;
    }
    else if(msg == "Module Stop")
    {
        msgJson["Status"] = "ModuleStopped";
    }
    else if(msg.find("WARNING") != std::string::npos)
    {
        msgJson["Status"] = "StatusWarning";
        msgJson["Text"] = msg;
    }
    else if(msg.find("ERROR") != std::string::npos)
    {
        msgJson["Status"] = "StatusError";
        msgJson["Text"] = msg;
    }
    else if(msg.find("FATAL") != std::string::npos)
    {
        msgJson["Status"] = "StatusFatal";
        msgJson["Text"] = msg;
    }

    m_messageStatus = msgJson.dump();

    return (m_messageStatus.size() > 0);
}

bool ModMonClient::open(std::string &errInfo)
{
    try
    {
        m_wdSets.broker.credentials += m_queueName;
        m_pTransClient = BaseTransportClient::createBroker(m_wdSets.broker, nullptr);
        if(m_pTransClient == nullptr)
            return false;

        m_pTransClient->setP_GlobalIsWork(p_GlobalIsWork);
        m_transClientThr = std::thread(&BaseTransportClient::run, m_pTransClient);
    }
    catch (send_error &e)
    {
        errInfo = std::string("Ошибка открытия rmq-канала: ")+e.what();
        return false;
    }

    for(int i = 0; i < 30; ++i)
    {
        if(m_pTransClient->getStatus() == TransportState::STATUS_OK)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "m_pTransClient->getStatus(): " << (int)m_pTransClient->getStatus() << "\n";

    if(m_pTransClient->getStatus() != TransportState::STATUS_OK)
    {
        m_pTransClient->setLocalIsWork(false);
        if(m_transClientThr.joinable())
            m_transClientThr.join();
        return false;
    }

    // Установка обработчика сообщений логгера для отправки их в ММ
    if(m_logDispatch)
        delete m_logDispatch;

    el::Helpers::installLogDispatchCallback<LoggerPP::DispatchLogger>("LoggerPP::DispatchLogger");
    m_logDispatch = el::Helpers::logDispatchCallback<LoggerPP::DispatchLogger>("LoggerPP::DispatchLogger");
    m_logDispatch->setEnabled(true);

    m_logDispatch->setF_callBackLogMsg([this](const LoggerPP::LogDispatchMsg_t &logData)
    {
        if(logData.logLevel != "INFO" && logData.logLevel != "DEBUG")
        {
            JSON msgJson;

            msgJson["MainPID"] = getpid();
            msgJson["Timestamp"] = MTime::nowTimeStamp("%Y-%m-%d %H:%M:%S");
            msgJson["Status"] = "Unknown";
            msgJson["Text"] = logData.msgText;

            if(logData.logLevel.find("WARNING") != std::string::npos)
            {
                msgJson["Status"] = "StatusWarning";
            }
            else if(logData.logLevel.find("ERROR") != std::string::npos)
            {
                msgJson["Status"] = "StatusError";
            }
            else if(logData.logLevel.find("FATAL") != std::string::npos)
            {
                msgJson["Status"] = "StatusFatal";
            }
            else {
                std::cerr << "unknow logData.logLevel: " << logData.logLevel << "\n";
            }

            std::string msgSend = msgJson.dump();
            m_pTransClient->send((uint8_t*)msgSend.c_str(), msgSend.size());
        }
    });

    return true;
}

bool ModMonClient::close(std::string &errInfo)
{
    (void) errInfo;
    return true;
}
