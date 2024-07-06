#include "DummyServer.h"
#include "LoggerPP.h"

DummyServer::DummyServer(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportServer(credentials, instanceName, other)
{
    this->setInDeqCapacity(20);
    this->setIsKeepConnect(true);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw receive_error(this->m_errorInfo);
    }
}

DummyServer::~DummyServer()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }
}

bool DummyServer::sendAnswer(uint8_t *buffData, const size_t sizeData,
                                 const std::string senderID,
                                 uint8_t *buffHead, const size_t sizeHead)
{
    (void)buffData;
    (void)sizeData;
    (void)buffHead;
    (void)sizeHead;

    LOG(WARNING) << "Пустой вызов sendAnswer";
    return false;
}

bool DummyServer::createConnection(std::string &errInfo)
{
    LOG(WARNING) << "Пустой вызов createConnection";
    return false;
}

bool DummyServer::releaseConnection(std::string &errInfo)
{
    (void)errInfo;
    LOG(WARNING) << "Пустой вызов releaseConnection";
    return false;
}

void DummyServer::startListen()
{
    LOG(WARNING) << "Пустой вызов startListen";
}

bool DummyServer::checkConnection()
{
    LOG(WARNING) << "Пустой вызов checkConnection";
    return false;
}

void DummyServer::doAfter()
{
    LOG(WARNING) << "Пустой вызов doAfter";
}
