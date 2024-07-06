#ifndef THTTPCLIENTSYNC_H
#define THTTPCLIENTSYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include "include_logger.h"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

class THttpClientSync
{
public:
    THttpClientSync(const std::string host, const uint16_t port)
    {
        m_host = host;
        m_port = port;
        resolver = new tcp::resolver(ioc);
        stream = new beast::tcp_stream(ioc);
    }

    ~THttpClientSync() {
        close();
    }

    bool connect()
    {
        // return false; // TODO убрать после записи логов

        // Look up the domain name
        auto const results = resolver->resolve(m_host, std::to_string(m_port));

        // Make the connection on the IP address we get from a lookup
        try {
            stream->connect(results);
            stream->socket().wait(boost::asio::socket_base::wait_type::wait_write);
            isConnect = true;
        }
        catch(std::exception const& e) {
            LOG(ERROR) << "Connect to Http Client Error: " << e.what() << std::endl;
            isConnect = false;
        }
        return isConnect;
    }

    bool isConnected(){
      //  LOG(DEBUG) << "isConnected == " << isConnect << std::endl;
       // LOG(DEBUG) << "is_open == " << stream->socket().wait_write();
        return isConnect;
    }

    int write(const std::string targetReq, const http::verb type, const std::string body) // Для EFCAPI - только Get
    {
        // usleep(100*1000);
        // return 0; // TODO убрать после записи логов

        int wrByte = 0;
        try{
            // Set up an HTTP GET request message
            http::request<http::string_body> req{type, targetReq, version};
            req.set(http::field::host, m_host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Send the HTTP request to the remote host
            wrByte = http::write(*stream, req, ec);
            if(ec) {
                throw beast::system_error{ec};
                isConnect = false;
            }
            else {
                isConnect = true;
            }
        }
        catch(std::exception const& e)
        {
            LOG(ERROR) << "Error: " << e.what() << std::endl;
        }

        return wrByte;
    }

    void close()
    {
        try {
            // Gracefully close the socket
            stream->socket().shutdown(tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes so don't bother reporting it.
            if(ec && ec != beast::errc::not_connected) {
                LOG(ERROR) << "Failed socket shutdown()!";
                throw beast::system_error{ec};
            }
            else {
                stream->socket().close(ec);
                if(ec) {
                    LOG(ERROR) << "Failed socket close()!";
                    throw beast::system_error{ec};
                }
                else {
//                    stream->socket().release(ec);
//                    if(ec) {
//                        LOG(ERROR) << "Failed socket release()!";
//                        throw beast::system_error{ec};
//                    }
//                    else {
                        LOG(WARNING) << "HTTP client socket close success!\n";
//                    }
                }
            }
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "Error close socket: "
                << dynamic_cast<std::exception const&>(e).what() << std::endl;
        }
    }

    int read(std::string &body)
    {
        // return 0; // TODO убрать после записи логов

        int rdByte = 0;
        try{
            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::dynamic_body> res;

            // Receive the HTTP response
            rdByte = http::read(*stream, buffer, res, ec);
            if(ec) {
                throw beast::system_error{ec};
                isConnect = false;
            }
            else {
                isConnect = true;
            }

            // Write the message to &body
            body = beast::buffers_to_string(res.body().data());
        }
        catch(std::exception const& e)
        {
            LOG(ERROR) << "Error: " << e.what() << std::endl;
        }
        return rdByte;
    }

private:
    // The io_context is required for all I/O
    net::io_context ioc;
    // These objects perform our I/O
    tcp::resolver *resolver = nullptr;
    beast::tcp_stream *stream = nullptr;

    const uint32_t version = 10; // HTTP version: 1.0
    std::string    m_host = "";
    uint16_t       m_port = 0;

    bool isConnect = false;
    beast::error_code ec;
};

#endif // THTTPCLIENTSYNC_H
