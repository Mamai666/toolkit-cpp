#ifndef TWEBSOCKETCLIENTSYNC_H
#define TWEBSOCKETCLIENTSYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
//#include <boost/exception/all.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include "LoggerPP.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

typedef boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO> rcv_timeout_option;


class TWebSocketClientSync
{
public:
    TWebSocketClientSync(std::string host, uint16_t port, bool connectLater = false)
    {
        m_host = host;
        m_port = port;
        if(!connectLater) {
           connect();
        }
    }
    ~TWebSocketClientSync()
    {
        // Close the WebSocket connection
        try {
            ws.close(websocket::close_code::normal);
        }  catch (std::exception& e) {
            LOG(ERROR) << "websocket close error: " << e.what() <<"\n";
        }
    }

    void connect()
    {
        auto f_host = m_host + ':' + std::to_string(m_port);
        try
        {
            // Look up the domain name
            auto results = resolver.resolve(m_host, std::to_string(m_port), ec);
            if(ec) {
                LOG(ERROR) << "resolver : " << ec.message();
                return;
            }
            // Make the connection on the IP address we get from a lookup
            //websocket::stream_base::timeout()
            auto ep = net::connect(ws.next_layer(), results);
            // Set a decorator to change the User-Agent of the handshake
            ws.set_option(websocket::stream_base::decorator(
                              [](websocket::request_type& req)
                          {
                              websocket::stream_base::timeout::suggested(beast::role_type::client);
                              req.set(http::field::user_agent,
                              std::string(BOOST_BEAST_VERSION_STRING) +
                              " websocket-client-coro");
                             // req.keep_alive(true);
                          }));

            ws.binary(false);
            ws.read_message_max(maxMsgSize); // sets the limit message
            ws.handshake(resp, f_host, "/");

            LOG(INFO) << "Connect to ws-server: ws://" << f_host << std::endl;
            m_isClosed = false;
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "Connect to ws-server (" << f_host << "): "
                << dynamic_cast<std::exception const&>(e).what() << std::endl;

            m_isClosed = true;
        }
    }

    int write(std::string buff) {
        std::string withoutError = "";
        return write(buff, withoutError);
    }

    int write(std::string buff, std::string &errorMsg)
    {
        // Send the message
        int wrByte = 0;

        for(uint i = 0; i < 5; ++i) {
            if(!ws.is_open() || m_isClosed) {
                LOG(ERROR) << "websocket not open.. Reconnect..";
                connect();
                usleep(500*1000);
            }
            else {
                break;
            }
        }
        if(!ws.is_open()) {
            LOG(ERROR) << "websocket open error: stream not openned!\n";
            return 0;
        }

        try {
            wrByte = ws.write(net::buffer(std::string(buff)), ec);
            if(ec.value() != 0) {
                LOG(ERROR) << "Error connection to : ws://"
                             << m_host << ":" << m_port
                             << " , code=" << ec.message() << std::endl;
                errorMsg = ec.message();
                m_isClosed = true;
                return 0;
            }
            else {
                m_isClosed = false;
            }
        }
        catch (std::exception& e) {
            LOG(ERROR) << "websocket write error: " << e.what() <<"\n";
            errorMsg = e.what();
            return 0;
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "websocket write error: "
                << dynamic_cast<std::exception const&>(e).what() << std::endl;
            errorMsg = dynamic_cast<std::exception const&>(e).what();
            return 0;
        }

        return wrByte;
    }

    int read(std::string &buff) {
        std::string withoutError = "";
        return read(buff, withoutError);
    }

    int read(std::string &buff, std::string &errorMsg)
    {
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;
        // Read a message into our buffer
        int rdByte = 0;
        for(uint i = 0; i < 5; ++i) {
            if(!ws.is_open() || m_isClosed) {
                LOG(ERROR) << "websocket not open.. Reconnect..";
                connect();
                usleep(500*1000);
            }
            else {
                break;
            }
        }

        if(!ws.is_open()) {
            LOG(ERROR) << "websocket open error: stream not openned!\n";
            return 0;
        }

        try {
            rdByte = ws.read(buffer, ec);
            if(ec == beast::error::timeout) {
                LOG(ERROR) << "Timeout ended!" << std::endl;
                return 0;
            }
            else if(ec.value() != 0) {
                LOG(ERROR) << "Error connection to : ws://"
                             << m_host << ":" << m_port
                             << " , code=" << ec.message() << std::endl;
                errorMsg = ec.message();
                m_isClosed = true;
                return 0;
            }
            else {
                m_isClosed = false;
            }
        }
        catch (std::exception &e) {
            LOG(ERROR) << "websocket read error: " << e.what() <<"\n";
            errorMsg = e.what();
            return 0;
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "websocket read error: "
                << dynamic_cast<std::exception const&>(e).what() << std::endl;
            errorMsg = dynamic_cast<std::exception const&>(e).what();
            return 0;
        }

        beast::buffers_to_string(buffer.data()).swap(buff);
        return rdByte;
    }

private:
    std::string m_host = "";
    uint16_t    m_port  = 0;

    net::io_context ioc;
    beast::error_code ec;

    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    const uint64_t maxMsgSize = 25*1024*1024;

    websocket::response_type resp;

    bool m_isClosed = false;

};

#endif // TWEBSOCKETCLIENTSYNC_H
