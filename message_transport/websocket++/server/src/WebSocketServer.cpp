#include "WebSocketServer.h"
#include "Utils/mstrings.h"
#include "LoggerPP.h"
#include <regex>

WebSocketServer::WebSocketServer(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportServer(credentials, instanceName, other)
{
    this->setInDeqCapacity(50);
    this->setIsKeepConnect(true);

    m_serverWS.clear_access_channels(websocketpp::log::alevel::all);
    m_serverWS.clear_error_channels(websocketpp::log::elevel::all);
    m_serverWS.set_max_message_size(200*1024*1024);

    m_serverWS.set_message_handler(std::bind(&WebSocketServer::on_message,
                                             this,
                                             std::placeholders::_1,
                                             std::placeholders::_2
                                    ));

    m_serverWS.set_open_handler(std::bind(&WebSocketServer::on_open,
                                          this,
                                          std::placeholders::_1));

    m_serverWS.set_close_handler(std::bind(&WebSocketServer::on_close,
                                           this,
                                           std::placeholders::_1));

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw receive_error(this->m_errorInfo);
    }
}

WebSocketServer::~WebSocketServer()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }
}

void WebSocketServer::on_open(websocketpp::connection_hdl hdl)
{
    try
    {
        websocketpp::lib::error_code ec;
        auto con = m_serverWS.get_con_from_hdl(hdl, ec);
        if (ec) {
            LOG(ERROR) << "Error getting ws-connection: " << ec.message() << std::endl;
        }
        else
        {
            auto remoteEP = con->get_remote_endpoint() + con->get_resource();
            m_endPointList[remoteEP] = hdl;
            LOG(INFO) << "Подключился новый клиент " << remoteEP;
        }
    }
    catch (const websocketpp::exception& e)
    {
        LOG(ERROR) << "websocketpp::Exception: " + std::string(e.what());
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "Std::Exception: " + std::string(e.what());
    }
}

void WebSocketServer::on_close(websocketpp::connection_hdl hdl)
{
    try
    {
        websocketpp::lib::error_code ec;
        auto con = m_serverWS.get_con_from_hdl(hdl, ec);
        if (ec) {
            LOG(ERROR) << "Error getting ws-connection: " << ec.message() << std::endl;
        }
        else
        {
            auto remoteEP = con->get_remote_endpoint() + con->get_resource();
            m_endPointList.erase(remoteEP);
            LOG(WARNING) << "Клиент " << remoteEP << " отключился!";
        }
    }
    catch (const websocketpp::exception& e)
    {
        LOG(ERROR) << "websocketpp::Exception: " + std::string(e.what());
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "Std::Exception: " + std::string(e.what());
    }
}

bool WebSocketServer::parseWebSocketPath(const std::string credens, std::string &errInfo)
{
    if(Strings::startsWith(credens, "ws://"))
    {
        auto pos = credens.find("://");
        auto wsPath = credens.substr(pos+strlen("://"));
        if(wsPath.empty())
        {
            errInfo = "Заданы пустые креды!";
        }
        else
        {
            auto wsChain = Strings::stdv_split(wsPath, ":");
            if(wsChain.size() == 2)
            {
                m_wsServerHost = wsChain.at(0);
                m_wsServerPort = std::stoi(wsChain.at(1));

                return true;
            }
            else
            {
                errInfo = "Неверный формат ws-кредов: "+wsPath;
            }
        }
    }
    else if(Strings::stdv_split(credens, "://").size() > 1) // Какой-то другой протокол
    {
        errInfo = "Неверный тип кредов для websocket: " + credens;
    }

    return false;
}

std::vector<websocketpp::connection_hdl> WebSocketServer::findEndpointBySubstring(const std::map<std::string, websocketpp::connection_hdl> &epList, const std::string &epNameMask)
{
    std::vector<websocketpp::connection_hdl> result_vector;
    for (const auto& pair : epList) {
        if (pair.first.find(epNameMask) != std::string::npos) {
            LOG(DEBUG) << "FindEP: " << pair.first << " by mask: " << epNameMask;
            result_vector.push_back(pair.second);
        }
    }
    return result_vector;
}


void WebSocketServer::startListen()
{
    try
    {
        m_serverWS.start_accept();
        m_serverWS.run();

        //m_asioThread.reset(new websocketpp::lib::thread(&ws_server::run, &m_serverWS));
        //m_asioThread->join();
        //m_serverWS.stop_listening();
    }
    catch (websocketpp::exception const & e) {
        throw receive_error(std::string("WebSocket++ exception: ") + e.what());
    }
    catch (receive_error const &e) {
        throw receive_error(std::string("receive_error exception: ") + e.what());
    }
    catch (std::exception const &e) {
        throw receive_error(std::string("Std exception: ") + e.what());
    }
}

void WebSocketServer::on_message(websocketpp::connection_hdl hdl, ws_server::message_ptr msg)
{
    websocketpp::lib::error_code ec;
    auto con = m_serverWS.get_con_from_hdl(hdl, ec);
    if (ec) {
        LOG(ERROR) << "Error getting ws-connection: " << ec.message() << std::endl;
    }
    else
    {
        auto remoteEP = con->get_remote_endpoint() + con->get_resource();

        this->addToIncoming((uint8_t*)msg->get_payload().c_str(),
                            msg->get_payload().size(),
                            remoteEP);
    }
}

bool WebSocketServer::checkConnection()
{
    return !m_isListenWrong;
}

bool WebSocketServer::createConnection(std::string &errInfo)
{
    m_serverWS.init_asio();
    m_serverWS.set_reuse_addr(true);

    if(!parseWebSocketPath(this->credentials(), errInfo))
    {
        return false;
    }

    if(!checkConnection())
    {
        return false;
    }

    ip::tcp::endpoint listenEP;
    listenEP = ip::tcp::endpoint( ip::tcp::v4( ), m_wsServerPort );
    listenEP.address(ip::address::from_string(m_wsServerHost));

    websocketpp::lib::error_code ec;
    m_serverWS.listen(listenEP, ec);
    if(ec)
    {
        m_isListenWrong = true;
        errInfo = "Ошибка биндинга " + listenEP.address().to_string();
        return false;
    }
    else
    {
        LOG(INFO) << "WS-server start listen " << listenEP;
        m_isListenWrong = false;
    }

    m_thrListen = std::thread(&WebSocketServer::startListen, this);
    return true;
}

bool WebSocketServer::releaseConnection(std::string &errInfo)
{
    this->setLocalIsWork(false);

    LOG(INFO) << "Закрытие " << credentials();

    m_serverWS.stop_listening();    
    m_serverWS.stop();

    if(m_thrListen.joinable())
        m_thrListen.join();

    // m_serverWS.close(m_serverWS.get_connection()->get_handle(), websocketpp::close::status::normal, "no_reason");

    return true;
}

bool WebSocketServer::sendAnswer(uint8_t *buffData, const size_t sizeData,
                                 std::string senderID,
                                 uint8_t *buffHead, const size_t sizeHead)
{
    try
    {
        websocketpp::connection_hdl findEP;

        if(senderID.empty())
        {
            LOG(ERROR) << "Пустой или битый senderHandler";
            return false;
        }
        else
        {
            if(Strings::startsWith(senderID, "*") || Strings::endsWith(senderID,"*"))
            {
                //senderID = regex_replace(senderID, std::regex("*"), "");
                std::string withoutMaskSenderID = "";
                for(auto c: senderID)
                {
                    if(c != '*')
                        withoutMaskSenderID += c;
                }

                LOG(DEBUG) << "senderID after replace *: " << withoutMaskSenderID;
                auto vecEP = findEndpointBySubstring(m_endPointList, withoutMaskSenderID);
                if(vecEP.size() < 1)
                {
                    LOG(ERROR) << "По маске senderID не найдено в таблице подключений: " << senderID;
                    return false;
                }
                findEP = vecEP.front();
            }
            else if(m_endPointList.find(senderID) == m_endPointList.end())
            {
                LOG(ERROR) << "Не найден senderID в таблице подключений: " << senderID;
                return false;
            }
            else findEP = m_endPointList[senderID];
        }

        ws_server::connection_ptr con = m_serverWS.get_con_from_hdl(findEP);
        websocketpp::lib::error_code ec = con->send(std::string((const char*)buffData, sizeData), websocketpp::frame::opcode::text);
        if (ec) {
            LOG(ERROR) << "Ошибка отправки ответа на узел: " << ec.message() << std::endl;
        }
        else {
            return true;
        }
    }
    catch (websocketpp::exception const & e) {
        LOG(ERROR) << "WebSocket++ exception: " << e.what() << std::endl;
    }
    catch (...) {
        LOG(ERROR) << "Unknown exception" << std::endl;
    }

    return false;
}

void WebSocketServer::doAfter()
{

}

