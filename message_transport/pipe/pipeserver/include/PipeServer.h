#ifndef PIPESERVER_H
#define PIPESERVER_H

#include "BaseTransportServer.h"
#include "pipe/PipeTransportKit.h"
#include <thread>

class PipeServer : public BaseTransportServer
{
public:
    PipeServer(std::string credentials,
               std::string instanceName, // for_encoder, for_analytic
               std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
               );
    
    ~PipeServer();

    virtual bool sendAnswer(uint8_t *buffData, const size_t sizeData,
                            const std::string senderID,
                            uint8_t *buffHead = nullptr, const size_t sizeHead = 0);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual void startListen();
    virtual bool checkConnection();

    virtual void doAfter();

private:
    //! Канал данных
    PipeDesc_t m_dataPipe;
    //! Сигнальный канал
    PipeDesc_t m_cmdPipe;
    size_t timeoutRead(int port, uint8_t *buf, size_t size, int mlsec_timeout);

    uint8_t *m_allBuff = nullptr;
    size_t  m_sizeAllBuff = 0;

    std::thread m_thrListen;
};

extern "C" BaseTransportServer* createServer(std::string credentials,
                                             std::string instanceName,
                                             std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new PipeServer(credentials, instanceName, other);
}

#endif // PIPESERVER_H
