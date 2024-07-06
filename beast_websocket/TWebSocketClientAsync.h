#ifndef TWEBSOCKETCLIENTASYNC_H
#define TWEBSOCKETCLIENTASYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
//#include <boost/exception/all.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "include_logger.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option;


class TWebSocketClientAsync //: public std::enable_shared_from_this<TWebSocketClientAsync>
{

public:
    TWebSocketClientAsync(std::string host, uint16_t port)
    {
        m_host = host;
        m_port = port;

//        std::thread thrCnct(&TWebSocketClientAsync::connect, this,
//                                                    host, port);
//        thrCnct.detach();

        connect();
        m_ioc.run();

        std::thread thrPing(&TWebSocketClientAsync::ping, this);
        thrPing.detach();
    }

    ~TWebSocketClientAsync()
    {
        try {
           close(websocket::close_code::normal);
        }
        catch (std::exception& e) {
           LOG(ERROR) << "websocket close exception: " << e.what() <<"\n";
        }
    }

public:

    void ping()
    {
        bool reservIsWork = true;
        if(p_isWorking == nullptr) {
           p_isWorking = &reservIsWork;
        }
        while(*p_isWorking)
        {
            try
            {
                if(isConnected()) {
                    m_ws.ping({});
                }
                else {
                    //setIsConnected(false);
                    LOG(WARNING) << "Нет соединения с сервером " << m_host<<":"<<m_port;
                }
            }
            catch(boost::exception const& e)
            {
                LOG(ERROR) << "Ошибка пинга: "
                           << dynamic_cast<std::exception const&>(e).what()
                           << std::endl;

                setIsConnected(false);

                m_ioc.stop();
                // prime to make ready for future run/run_one/poll/poll_one calls
                m_ioc.reset();

                connect();
                m_ioc.run();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void connect(std::string host = "", uint16_t port = 0)
    {
        host = host.empty() ? m_host : host;
        port = (port == 0) ? m_port : port;

        if(host.empty() || port < 1) {
            LOG(ERROR) << "Wrong Host port!";
            return;
        }

        m_resolver.async_resolve(host, std::to_string(port),
            std::bind(
                &TWebSocketClientAsync::on_resolve,
                this,
                std::placeholders::_1,
                std::placeholders::_2
            ));
    }

    void write(const std::string &buff, bool needAnswer = true)
    {
        m_ioc.stop();
        m_ioc.reset();

        m_ws.async_write(
            net::buffer(buff),
            std::bind(
                &TWebSocketClientAsync::on_write,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                needAnswer
            ));

        m_ioc.run();
    }

    void loopRead()
    {
        m_ioc.stop();
        m_ioc.reset();

        m_ws.async_read(
            m_buffer,
            std::bind(
               &TWebSocketClientAsync::on_read,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                true
        ));

        m_ioc.run();
        LOG(WARNING) << "Exit loop read!";
    }

    void close(websocket::close_reason const& cr = websocket::close_code::normal)
    {
        m_ioc.stop();
        m_ioc.reset();

        m_ws.async_close(cr,
            std::bind(
                &TWebSocketClientAsync::on_close,
                this,
                std::placeholders::_1
            ));

        m_ioc.run();
    }

    void setCallBackAnswer(std::function<std::string(const std::string &inBuffStr)> callBackAnswer)
    {
        m_fCallBackAnswer = callBackAnswer;
    }

    void setCallBackConnect(const std::function<void ()> &callBackConnect)
    {
        m_fCallBackConnect = callBackConnect;
    }
//----------------------

    bool isConnected()
    {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        return m_isConnected;
    }

    void setIsConnected(bool isConnected)
    {
        std::lock_guard<std::mutex> lgMtx(m_mtx);
        m_isConnected = isConnected;
    }

protected:

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
    {
        if(ec) {
            LOG(ERROR) << "Failed resolve : " << ec.message();
            return;
        }
        // Set the timeout for the operation
        beast::get_lowest_layer(m_ws).expires_after(std::chrono::seconds(1));

        beast::get_lowest_layer(m_ws).async_connect(
            results,
            std::bind(
                &TWebSocketClientAsync::on_connect,
                this,
                std::placeholders::_1
            ));
    }

    void on_ready_to_reconnect(const boost::system::error_code &error)
    {
        connect();
    }

    void on_connect(beast::error_code ec)
    {
        f_host = m_host + ':' + std::to_string(m_port);
        if(ec) {
            LOG(ERROR) << "Ошибка соединения с " << f_host << ":" << ec.message();
            setIsConnected(false);
            //close();
            m_timer.expires_from_now(boost::asio::chrono::seconds{2});
            m_timer.async_wait(std::bind(&TWebSocketClientAsync::on_ready_to_reconnect,
                                         this, std::placeholders::_1));
            return;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(m_ws).expires_never();

        // Set suggested timeout settings for the websocket
        m_ws.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        m_ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
                req.keep_alive(true);
            }));

        websocket::stream_base::timeout opt{
            std::chrono::seconds(4),   // handshake timeout
            websocket::stream_base::none(),       // idle timeout. no data recv from peer for 6 sec, then it is timeout
            true   //enable ping-pong to keep alive
        };

        m_ws.set_option(opt);

        m_ws.control_callback(
                [](websocket::frame_type kind, beast::string_view payload){
                    try{
                        if(kind == boost::beast::websocket::frame_type::pong){
                            //  LOG(WARNING) << "pong";
                        }
                        else {
                            LOG(WARNING) << "kind control_call: " << static_cast<int>(kind);
                        }
                    }
                    catch(boost::exception const& e) {
                        LOG(ERROR) << "control_callback except: "
                                   << dynamic_cast<std::exception const&>(e).what() << std::endl;
                    }}
        );

        m_ws.binary(false);
        m_ws.auto_fragment(false);
        m_ws.read_message_max(maxMsgSize); // sets the limit message

        try {
            m_ws.async_handshake(m_resp,f_host, "/",
                                 std::bind(
                                    &TWebSocketClientAsync::on_handshake,
                                    this,
                                    std::placeholders::_1
                                ));
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "async_handshake except: "
                       << dynamic_cast<std::exception const&>(e).what() << std::endl;
        }
    }

    void on_handshake(beast::error_code er)
    {
        if(er) {
            LOG(ERROR) << "Handshake Failed: ("<<f_host<<") " << er.message();
            setIsConnected(false);
            //close();
            return ;
        }
        LOG(INFO) << "Подключено к : ws://" << f_host << " как :" << m_resp << std::endl;
        setIsConnected(true);

        try {
            m_fCallBackConnect();
        }
        catch(std::exception &e) {
            LOG(ERROR) << "Не задана функция обработки события после подключения!";
        }
    }


    void on_write(beast::error_code ec, std::size_t bytes_transferred, bool needAnswer = true)
    {
        if(ec) {
            LOG(ERROR) << "Ошибка записи на "<< f_host << ": " << ec.message() <<"\n";
            return;// fail(ec, "write");
        }

        if(needAnswer)
        {
            LOG(DEBUG) << "needAnswer";
            m_ws.async_read(
                m_buffer,
                std::bind(
                   &TWebSocketClientAsync::on_read,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    false
            ));
        }
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred, const bool loop = false)
    {
        if(ec) {
            LOG(ERROR) << "Ошибка чтения с "<< f_host << ": " << ec.message() <<"\n";
            return ; // fail(ec, "read");
        }

        LOG(DEBUG) << "before m_fCallBackAnswer";

        std::string res = "";
        beast::buffers_to_string(m_buffer.data()).swap(res);
        m_fCallBackAnswer(res);
        m_buffer.clear();

        if(p_isWorking != nullptr) {
            if(*p_isWorking == false) return;
        }

        if(loop)
        {
            m_ws.async_read(
                m_buffer,
                std::bind(
                   &TWebSocketClientAsync::on_read,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2,
                    loop
            ));
        }
    }

    void on_close(beast::error_code ec)
    {
        if(ec) {
            LOG(ERROR) << "Ошибка закрытия сокета c " <<
                f_host << " : " << ec.message() << std::endl;
            return;// fail(ec, "close");
        }
    }
    
public:
    net::io_context m_ioc;

    void setP_isWorking(bool *newP_isWorking);

private:

    beast::flat_buffer m_buffer;

    std::string m_host = "";
    uint16_t    m_port  = 0;
    std::string f_host = "";

    bool *p_isWorking  = nullptr;
    bool m_isConnected = false;
    //beast::error_code m_ec;

    tcp::resolver m_resolver{m_ioc};
    websocket::stream<beast::tcp_stream> m_ws{m_ioc};

    const uint64_t maxMsgSize = 25*1024*1024;

    boost::asio::steady_timer m_timer{m_ioc, boost::asio::chrono::seconds{2}};

    std::function<std::string(const std::string &inBuffStr)> m_fCallBackAnswer;

    std::function<void(void)> m_fCallBackConnect;
    //websocket::response_type resp;

    websocket::response_type m_resp;

    std::mutex m_mtx;

};

inline void TWebSocketClientAsync::setP_isWorking(bool *newP_isWorking)
{
    p_isWorking = newP_isWorking;
}







#endif // TWEBSOCKETCLIENTASYNC_H
