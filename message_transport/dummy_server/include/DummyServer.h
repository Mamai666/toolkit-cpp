#ifndef DUMMYSERVER_H
#define DUMMYSERVER_H

#include "BaseTransportServer.h"

class DummyServer : public BaseTransportServer
{
public:
    DummyServer(std::string credentials,
               std::string instanceName, // for_encoder, for_analytic
               std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
               );

    ~DummyServer();

    virtual bool sendAnswer(uint8_t *buffData, const size_t sizeData,
                            const std::string senderID,
                            uint8_t *buffHead = nullptr, const size_t sizeHead = 0);

protected:

    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual void startListen();
    virtual bool checkConnection();

    virtual void doAfter();
};

extern "C" BaseTransportServer* createTransportServer(std::string credentials,
                                                   std::string instanceName,
                                                   std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new DummyServer(credentials, instanceName, other);
}

#endif // DUMMYSERVER_H
