#ifndef PIPECLIENT_H
#define PIPECLIENT_H

#include "BaseTransportClient.h"
#include "pipe/PipeTransportKit.h"

class PipeClient : public BaseTransportClient
{
public:
    PipeClient(std::string credentials,
                    std::string instanceName, // for_encoder, for_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~PipeClient();

    virtual uint8_t* readAnswer(uint8_t *buffData, size_t &sizeData,
                            const int timeoutMs);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual uint64_t sendToStream(uint8_t *msgData, const size_t msgSize);
    virtual bool checkConnection();

    virtual void doAfter();

private:
    //! Канал данных
    PipeDesc_t m_dataPipe;
    //! Сигнальный канал
    PipeDesc_t m_cmdPipe;
};

extern "C" BaseTransportClient* createClient(std::string credentials,
                                             std::string instanceName,
                                             std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new PipeClient(credentials, instanceName, other);
}

#endif // PIPECLIENT_H
