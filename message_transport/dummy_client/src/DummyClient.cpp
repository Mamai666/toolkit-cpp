#include "DummyClient.h"
#include "LoggerPP.h"

DummyClient::DummyClient(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
        : BaseTransportClient(credentials, instanceName, other)
{
    this->setOutDeqCapacity(2);
    this->setIsKeepConnect(true);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw send_error(this->m_errorInfo);
    }
}

DummyClient::~DummyClient()
{
    if(!releaseConnection(m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << m_errorInfo << std::endl;
    }
}

uint8_t* DummyClient::readAnswer(uint8_t *buffData, size_t &sizeData,
                             const int timeoutMs)
{
    (void)buffData;
    (void)sizeData;

    LOG(WARNING) << "Пустой вызов readAnswer";
    return nullptr;
}

bool DummyClient::createConnection(std::string &errInfo)
{
    LOG(WARNING) << "Пустой вызов createConnection";
    return false;
}

bool DummyClient::releaseConnection(std::string &errInfo)
{
    LOG(WARNING) << "Пустой вызов releaseConnection";
    return false;
}

uint64_t DummyClient::sendToStream(uint8_t* msgData, const size_t msgSize)
{
    LOG(WARNING) << "Пустой вызов sendToStream";
    return 0;
}

bool DummyClient::checkConnection()
{
    LOG(WARNING) << "Пустой вызов checkConnection";
    return false;
}

void DummyClient::doAfter()
{
    LOG(WARNING) << "Пустой вызов doAfter";
}
