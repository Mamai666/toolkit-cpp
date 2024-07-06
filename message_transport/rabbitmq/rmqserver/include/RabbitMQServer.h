#ifndef RABBITMQSERVER_H
#define RABBITMQSERVER_H

#include "BaseTransportServer.h"
#include "OwnLibEvHandler.h"
#include <condition_variable>
#include <thread>

struct Credent4Server_t
{
    std::string hostPort = "";
    std::string vHost = "/";
    std::string user = "";
    std::string pass = "";
    std::string queue = "";
};

class RabbitMQServer : public BaseTransportServer
{
public:
    RabbitMQServer(std::string credentials,
                    std::string instanceName, // for_encoder, for_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~RabbitMQServer();

    virtual bool sendAnswer(uint8_t *buffData, const size_t sizeData, std::string senderID,
                            uint8_t *buffHead = nullptr, const size_t sizeHead = 0);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual void startListen();
    virtual bool checkConnection();

    virtual void doAfter();

private:
    bool parseRMQPath(const std::string credens, std::string &errInfo);
    std::shared_ptr<std::thread> m_thrLoop;
    
    std::thread m_thrListen;

    Credent4Server_t m_credent;

    AMQP::TcpConnection *m_pConnection = nullptr;
    AMQP::TcpChannel *m_pChannel = nullptr;

    std::condition_variable m_conditionOpen;
    std::mutex m_mutexOpen;

    OwnLibEvHandler* m_pOwnLibEvHandler = nullptr;
};

extern "C" BaseTransportServer* createServer(std::string credentials,
                                            std::string instanceName,
                                            std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new RabbitMQServer(credentials, instanceName, other);
}

#endif // RABBITMQSERVER_H
