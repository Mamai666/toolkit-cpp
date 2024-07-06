#ifndef TSIMPLEHTTPSERVER_H
#define TSIMPLEHTTPSERVER_H

#include <restbed>
#include <iostream>
#include "include_logger.h"

const std::string realmStr = "Glosav";
const std::string nonceStr = "Ny8yLzIwMDIgMzoyNjoyNCBQTQ";

class TSimpleHttpServer
{
public:
    TSimpleHttpServer(const std::string &host, const uint16_t &port,
                      const bool isDigestAuthorized = false);

    virtual ~TSimpleHttpServer() {
    //    if(m_pService) delete m_pService;
    }

    void startListen(bool isNoneEmptyResources = true); // isNoneEmptyResources = true - доп ресурсы обязаны быть
    bool stopListen();
    void restartListen();
    void setIsDigestAuthorized(bool isDigestAuthorized);

    std::shared_ptr<restbed::Service> getPService();

    void setPort(uint16_t newPort);
    void setHost(const std::string &newHost);

    void sigStop_handler(const int signal_number);

protected:
    uint16_t m_port = 0;
    std::string m_host = "127.0.0.1";
    bool m_isDigestAuthorized = false;

    std::shared_ptr<restbed::Service> m_pService;// = nullptr;
    std::shared_ptr<restbed::Settings> m_settings;
    std::map<std::string, std::shared_ptr<restbed::Resource>> m_resources;
    //std::string build_authenticate_header();
    //  void authentication_handler(const std::shared_ptr<restbed::Session> session, const std::function<void (const std::shared_ptr<restbed::Session>)> &callback);
private:
    void get_heartbeat(const std::shared_ptr<restbed::Session> session);
};

#endif // TSIMPLEHTTPSERVER_H
