#ifndef DUMMYCLIENT_H
#define DUMMYCLIENT_H

#include "BaseTransportClient.h"

class DummyClient : public BaseTransportClient
{
public:
    DummyClient(std::string credentials,
                    std::string instanceName, // for_encoder, for_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~DummyClient();

    virtual uint8_t* readAnswer(uint8_t *buffData, size_t &sizeData,
                                const int timeoutMs);

protected:

    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual uint64_t sendToStream(uint8_t* msgData, const size_t msgSize);
    virtual bool checkConnection();

    virtual void doAfter();
};

extern "C" BaseTransportClient* createClient(std::string credentials,
                                             std::string instanceName,
                                             std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new DummyClient(credentials, instanceName, other);
}

#endif // DUMMYCLIENT_H
