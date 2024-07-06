#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include "BaseTransportClient.h"

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

using ws_client = websocketpp::client<websocketpp::config::asio_client>;

class WebSocketClient : public BaseTransportClient
{
public:
    WebSocketClient(std::string credentials,
                    std::string instanceName, // for_encoder, for_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~WebSocketClient();

    virtual uint8_t* readAnswer(uint8_t *buffData, size_t &sizeData,
                            const int timeoutMs);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual uint64_t sendToStream(uint8_t *msgData, const size_t msgSize);
    virtual bool checkConnection();

    virtual void doAfter();

private:
    ws_client m_clientWS;
    websocketpp::connection_hdl m_conHandle;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_asioThread;

    std::condition_variable m_conditionAnsw;
    std::condition_variable m_conditionOpen;

    std::mutex m_mutexAnsw;
    std::mutex m_mutexOpen;

    std::mutex m_flagRecieveMutex;
    std::mutex m_flagOpenMutex;

    bool m_isOpenned = false;

    bool   m_isReceivedData   = false;
    size_t m_receivedDataSize = 0;

    std::vector<uint8_t> m_receivedData;

    void on_message(websocketpp::connection_hdl hdl, ws_client::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

    std::string parseWebSocketPath(const std::string credens, std::string &errInfo);
};

extern "C" BaseTransportClient* createClient(std::string credentials,
                                            std::string instanceName,
                                            std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new WebSocketClient(credentials, instanceName, other);
}

#endif // WEBSOCKETCLIENT_H
