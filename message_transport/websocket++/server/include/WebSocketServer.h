#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include "BaseTransportServer.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <thread>

using namespace websocketpp::lib::asio;

using ws_server = websocketpp::server<websocketpp::config::asio>;

class WebSocketServer : public BaseTransportServer
{
public:
    WebSocketServer(std::string credentials,
                    std::string instanceName, // for_encoder, for_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );

    ~WebSocketServer();

    virtual bool sendAnswer(uint8_t *buffData, const size_t sizeData, std::string senderID,
                            uint8_t *buffHead = nullptr, const size_t sizeHead = 0);

protected:
    virtual bool createConnection(std::string &errInfo);
    virtual bool releaseConnection(std::string &errInfo);

    virtual void startListen();
    virtual bool checkConnection();

    virtual void doAfter();

private:
    ws_server m_serverWS;
    std::string m_wsServerHost = "";
    uint16_t    m_wsServerPort = 0;

    bool        m_isListenWrong = false;
    
    std::thread m_thrListen;

    std::map<std::string, websocketpp::connection_hdl> m_endPointList;

    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_asioThread;

    void on_message(websocketpp::connection_hdl hdl, ws_server::message_ptr msg);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);

    bool parseWebSocketPath(const std::string credens, std::string &errInfo);

    std::vector<websocketpp::connection_hdl> findEndpointBySubstring(const std::map<std::string, websocketpp::connection_hdl>& epList, const std::string& epNameMask);
};

extern "C" BaseTransportServer* createServer(std::string credentials,
                                            std::string instanceName,
                                            std::initializer_list<void*> *other = nullptr)
{
    // Верните объект, созданный с использованием конструктора с параметрами
    return new WebSocketServer(credentials, instanceName, other);
}


#endif // WEBSOCKETSERVER_H
