#include "WebSocketClient.h"
#include <functional>
#include "Utils/mstrings.h"

WebSocketClient::WebSocketClient(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportClient(credentials, instanceName, other)
{
    this->setOutDeqCapacity(50);
    this->setIsKeepConnect(true);

    m_clientWS.clear_access_channels(websocketpp::log::alevel::all);
    m_clientWS.clear_error_channels(websocketpp::log::elevel::all);

    m_clientWS.init_asio();
    m_clientWS.start_perpetual();

    m_clientWS.set_message_handler(std::bind(&WebSocketClient::on_message,
                                             this,
                                             std::placeholders::_1,
                                             std::placeholders::_2));

    m_clientWS.set_open_handler(std::bind(&WebSocketClient::on_open,
                                          this,
                                          std::placeholders::_1));

    m_clientWS.set_close_handler(std::bind(&WebSocketClient::on_close,
                                           this,
                                           std::placeholders::_1));

    m_asioThread.reset(new websocketpp::lib::thread(&ws_client::run, &m_clientWS));

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw send_error(this->m_errorInfo);
    }
}

WebSocketClient::~WebSocketClient()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }

    m_clientWS.stop();
    m_clientWS.stop_perpetual();
    m_asioThread->join();
}

uint8_t *WebSocketClient::readAnswer(uint8_t *buffData, size_t& sizeData,
                                 const int timeoutMs)
{
    if(m_fCallBackResponse)
    {
        LOG(WARNING) << "Уже задан callback на обработку ответа! Выход..";
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(m_mutexAnsw);
    m_conditionAnsw.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return m_isReceivedData; });

    if (m_isReceivedData)
    {
        std::lock_guard<std::mutex> guard(m_flagRecieveMutex);
        if (m_receivedDataSize <= sizeData)
        {
            sizeData = m_receivedDataSize;
            std::memcpy(buffData, m_receivedData.data(),m_receivedDataSize);
            //std::copy(m_receivedData.begin(), m_receivedData.end(), buffData);
            m_receivedData.clear();
            m_receivedDataSize = 0;
            m_isReceivedData = false;
            return buffData;
        }
        else {
            LOG(ERROR) << "Размер буфера недостаточен для копирования данных: "
                       << m_receivedDataSize <<" > " << sizeData << std::endl;
            return nullptr;
        }
    }
    return nullptr;
}

void WebSocketClient::on_message(websocketpp::connection_hdl hdl, ws_client::message_ptr msg)
{
    // Обработка входящих сообщений здесь
    std::lock_guard<std::mutex> guard(m_mutexAnsw);
    m_isReceivedData = true;
    m_receivedDataSize = msg->get_payload().size();
    m_receivedData.resize(m_receivedDataSize);

    m_conditionAnsw.notify_one();

    if(m_fCallBackResponse)
    {
        m_fCallBackResponse((uint8_t*)msg->get_payload().c_str(),msg->get_payload().size());
    }
    else
    {
        std::copy(msg->get_payload().begin(), msg->get_payload().end(), m_receivedData.begin());
    }
}

void WebSocketClient::on_open(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> guard(m_mutexOpen);
    m_conHandle = hdl;
    m_isOpenned = true;

    m_conditionOpen.notify_all();
    // Соединение открыто
    LOG(INFO) << "Соединение открыто";
}

void WebSocketClient::on_close(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> guard(m_mutexOpen);
    // Соединение закрыто
    LOG(WARNING) << "Соединение закрыто";
    m_isOpenned = false;
}

std::string WebSocketClient::parseWebSocketPath(const std::string credens, std::string &errInfo)
{
    std::string wsPath = credens;
    if(Strings::startsWith(credens, "ws://"))
    {
        auto pos = credens.find("://");
        auto mainPartUri = credens.substr(pos+strlen("://"));
        if(mainPartUri.empty())
        {
            errInfo = "Заданы пустые креды!";
            return "";
        }
    }
    else if(Strings::stdv_split(credens, "://").size() > 1) // Какой-то другой протокол
    {
        errInfo = "Неверный тип кредов для websocket: " + credens;
        return "";
    }

    return wsPath;
}

bool WebSocketClient::createConnection(std::string &errInfo)
{
    std::lock_guard<std::mutex> guard(m_flagOpenMutex);
    LOG(DEBUG) << "createConnection start";
    m_isOpenned = false;
    try
    {
        websocketpp::lib::error_code ec;
        std::string URI = parseWebSocketPath(this->credentials(), errInfo);
        if(URI.empty())
        {
            return false;
        }

        ws_client::connection_ptr con = m_clientWS.get_connection(URI, ec);

        if (ec)
        {
            errInfo = "Error connecting: " + ec.message();
            return false;
        }

        m_clientWS.connect(con);

        return true;
    }
    catch (const websocketpp::exception& e)
    {
        errInfo = "websocketpp::Exception: " + std::string(e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        errInfo = "Std::Exception: " + std::string(e.what());
        return false;
    }
}

bool WebSocketClient::releaseConnection(std::string &errInfo)
{
    try
    {
        if(m_isOpenned)
        {
            m_clientWS.close(m_conHandle, websocketpp::close::status::normal, "no_reason");
        }
        return true;
    }
    catch (const std::exception& e)
    {
        errInfo = "Exception: " + std::string(e.what());
        return false;
    }
}

uint64_t WebSocketClient::sendToStream(uint8_t* msgData, const size_t msgSize)
{
    websocketpp::lib::error_code ec;
    ws_client::connection_ptr con = m_clientWS.get_con_from_hdl(m_conHandle, ec);
    if (ec) {
        LOG(ERROR) << "Error getting connection: " << ec.message() << std::endl;
        return 0;
    }

    std::string message(reinterpret_cast<char*>(msgData), msgSize);
    ec = con->send(msgData, msgSize, websocketpp::frame::opcode::text);

    if (ec) {
        LOG(ERROR) << "Error sending message: " << ec.message() << std::endl;
        return 0;
    }

    return msgSize;
}

bool WebSocketClient::checkConnection()
{
    std::unique_lock<std::mutex> lock(m_mutexOpen);
    if(!m_isOpenned)
    {
        LOG(DEBUG) << "Is isWaitOpenning True";
        m_conditionOpen.wait_for(lock, std::chrono::milliseconds(2000), [&] { return m_isOpenned; });
        LOG(DEBUG) << "m_isOpenned: " << m_isOpenned;
    }

    websocketpp::lib::error_code ec;
    auto con = m_clientWS.get_con_from_hdl(m_conHandle, ec);
    if (ec) {
        LOG(ERROR) << "Error getting ws-connection: " << ec.message() << std::endl;
        return false;
    }
    return con->get_state() == websocketpp::session::state::open;
}

void WebSocketClient::doAfter()
{

}
