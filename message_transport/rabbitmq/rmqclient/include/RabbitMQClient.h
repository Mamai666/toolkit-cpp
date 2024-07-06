#ifndef RABBITMQCLIENT_H
#define RABBITMQCLIENT_H

#include "BaseTransportClient.h"
#include "OwnLibEvHandler.h"
#include <condition_variable>
#include <thread>

struct Credent4Client_t
{
    std::string hostPort = "";
    std::string vHost = "/";
    std::string user = "";
    std::string pass = "";
    std::string queue = "";
};

class RabbitMQClient : public BaseTransportClient
{
public:
    RabbitMQClient(std::string credentials,
                   std::string instanceName, // for_encoder, for_analytic
                   std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~RabbitMQClient();

    virtual uint8_t* readAnswer(uint8_t *buffData, size_t &sizeData,
                            const int timeoutMs);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual uint64_t sendToStream(uint8_t *msgData, const size_t msgSize);
    virtual bool checkConnection();

    virtual void doAfter();

private:
    bool parseRMQPath(const std::string credens, std::string &errInfo);
    std::shared_ptr<std::thread> m_thrLoop;

    Credent4Client_t m_credent;

    AMQP::TcpConnection *m_pConnection = nullptr;
    AMQP::TcpChannel *m_pChannel = nullptr;

    std::condition_variable m_conditionOpen;
    std::mutex m_mutexOpen;

    OwnLibEvHandler* m_pOwnLibEvHandler = nullptr;
};

extern "C" BaseTransportClient* createClient(std::string credentials,
                                             std::string instanceName,
                                             std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new RabbitMQClient(credentials, instanceName, other);
}

#endif // RABBITMQCLIENT_H
