#ifndef TWEBSOCKETSERVERSYNC_H
#define TWEBSOCKETSERVERSYNC_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
//#include <boost/exception/all.hpp>
#include <cstdlib>
#include <iostream>
#include "include_logger.h"
#include <string>
#include <thread>
#include <memory>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace std;

//#define cout LOG(DEBUG)
//#define cerr LOG(ERROR)

class TWebSocketServerSync
{
public:
    TWebSocketServerSync(const std::string host, const uint16_t port)
    {
        m_port = port;
        m_address = net::ip::make_address(host);
    }

    bool accepting_connections(net::ip::address address, uint16_t port) {
        //using namespace boost::asio;
        //using ip::tcp;
        //using ec = boost::system::error_code;

        bool result = false;

        try
        {
            boost::asio::io_context svc;
            tcp::socket s(svc);
            boost::asio::deadline_timer tim(svc, boost::posix_time::seconds(1));

            tim.async_wait([&](boost::system::error_code) { s.cancel(); });
            s.async_connect({address, port}, [&](boost::system::error_code ec) {
                result = !ec;
            });

            svc.run();
        } catch(...) { }

        return result;
    }

    void startListen()
    {
        for(;;) {
            LOG(INFO) << "Try websocket server accepting_connections..";
            if(!accepting_connections(m_address, m_port)) {
                LOG(INFO) << "ws://" << m_address.to_string() << ":" << m_port
                          << " is free for accept !";
                break;
            }
            else {
                LOG(ERROR) << "ws://" << m_address.to_string() << ":" << m_port
                           << " is already accepting !";
            }
            sleep(3);
        }

        tcp::acceptor acceptor{ioc, {m_address, m_port}};
        LOG(INFO) << "Start wsServer listen on ws://" << m_address.to_string()
                  << ":" << m_port << std::endl;
        try {
            for(;;)
            {
                // This will receive the new connection
                tcp::socket socket{ioc};

                // Block until we get a connection
                acceptor.accept(socket);
                LOG(INFO) << "New connection from : "
                             << socket.remote_endpoint().address() << " : "
                             << socket.remote_endpoint().port() << std::endl;
                // Launch the session, transferring ownership of the socket
                std::thread{std::bind( &TWebSocketServerSync::do_session, this, std::move(socket))}.detach();
            }
        }
        catch (const std::exception& e) {
            LOG(ERROR) << "Error: " << e.what() << std::endl;
            //return EXIT_FAILURE;
        }
    }

    ~TWebSocketServerSync()
    {
        //        // Close the WebSocket connection
        //        try {
        //            ws->close(websocket::close_code::normal);
        //        }  catch (std::exception& e) {
        //            std::cout << "\n TWebSocketServerSync::close error: " << e.what() <<"\n";
        //            std::fflush(stdout);
        //        }
    }

    void setCallBackPrepareAnswer(std::function<std::string(const std::string &inBuffStr)> callBackPrepareAnswer)
    {
        m_callBackPrepareAnswer = callBackPrepareAnswer;
    }

public:

    //std::string (*m_callBackPrepareAnswer) (const std::string &) = nullptr;
    std::function<std::string(const std::string &inBuffStr)> m_callBackPrepareAnswer;

    void recvAndAnswer(websocket::stream<tcp::socket> *ws, const std::string &fromAddr, const unsigned short &fromPort)
    {
        while(1)
        {
            beast::error_code ec;
           // mtx.lock();
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            auto rdByte = ws->read(buffer, ec); // Read a message
            // error_code = 125 - Operation canceled
            // ec == websocket::error::closed || ec.value() == 125
            if(ec) {
                LOG(ERROR) << "ec_msg: " << ec.message() << ", ec_code: " << ec.value();
                LOG(ERROR) << "rdByte : " << rdByte << std::endl;
                LOG(ERROR) << "Close connection from : "
                           << fromAddr << ":" << fromPort << std::endl;
                break;
            }

            ws->text(ws->got_text());
            const std::string &buffString = beast::buffers_to_string(buffer.data());
            std::string res;
            if(m_callBackPrepareAnswer == nullptr) {
                LOG(ERROR) << "Error : Not init m_callBackPrepareAnswer function!" << std::endl;
            }
            else if(rdByte > 0){
                res = m_callBackPrepareAnswer(buffString);
                //ws.close();
            }

            if(!res.empty() && res != "error") {
                ws->write(net::buffer(std::string(res)));
            }
            else if (res == "error") {
                LOG(ERROR) << "Bad inJson from ws://" <<
                              fromAddr << ":" << fromPort << std::endl;
            }
            //mtx.unlock();
        }
    }

    void sendOne(std::pair<const std::string , websocket::stream<tcp::socket>*> *itOne, const std::string msg) {
        try {
            auto wrByte = itOne->second->write(net::buffer(msg));
            if(wrByte != msg.size()) {
                LOG(ERROR) << "Не удалось отправить сообщение клиенту " << itOne->first ;
            }
        }
        catch(beast::system_error const& se) {
            // This indicates that the session was closed
            if(se.code() != websocket::error::closed)
                LOG(ERROR) << "Error: " << se.code().message() << std::endl;
        }
    }

    void sendAll(const std::string msg)
    {
        LOG(DEBUG) << "Before mutex.lock()";
        m_mtx.lock();
        try {
            LOG(DEBUG) << "WS-clients size(): " << m_wsClients.size();
            for(auto &it : m_wsClients) {
                LOG(DEBUG) << "Prepare for Sending msg to " << it.first;
                sendOne(&it, msg);
                LOG(DEBUG) << "End Sending msg to " << it.first;
                //std::thread thr(&TWebSocketServerSync::sendOne, this, it, msg);
                //thr.detach();
            }
        }
        catch(std::exception const& e) {
            LOG(ERROR) << "Error: " << e.what() << std::endl;
        }
        m_mtx.unlock();
        LOG(DEBUG) << "After mutex.unlock()" << std::endl;
    }

    void do_session(tcp::socket &socket)
    {
        try {
            // Construct the stream by moving in the socket
            const std::string    fromAddr = socket.remote_endpoint().address().to_string();
            const unsigned short fromPort = socket.remote_endpoint().port();

            std::string keyHostPort = fromAddr + ":" + std::to_string(fromPort);

            //websocket::stream<tcp::socket> ws{std::move(socket)};
            m_mtx.lock();
            m_wsClients.emplace(keyHostPort, new websocket::stream<tcp::socket>{std::move(socket)});//ws);
            m_mtx.unlock();

            auto ws = m_wsClients.at(keyHostPort);

            // Set a decorator to change the Server of the handshake
            ws->set_option(websocket::stream_base::decorator(
                [](websocket::response_type& res)
                {
                    res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-sync");
                }));

            ws->accept(); // Accept the websocket handshake
            std::thread thrRecv(&TWebSocketServerSync::recvAndAnswer, this, ws, std::ref(fromAddr), std::ref(fromPort));

            thrRecv.join();

            m_mtx.lock();
            if(m_wsClients.at(keyHostPort)) {
                delete m_wsClients.at(keyHostPort);
            }
            m_wsClients.erase(keyHostPort);
            m_mtx.unlock();
            LOG(INFO) << "Deleted client: ws://" << keyHostPort << " ; wsClient.amount() == " << m_wsClients.size();
        }
        catch(beast::system_error const& se) {
            // This indicates that the session was closed
            if(se.code() != websocket::error::closed)
                LOG(ERROR) << "Error: " << se.code().message() << std::endl;
        }
        catch(std::exception const& e) {
            LOG(ERROR) << "Error: " << e.what() << std::endl;
        }
    }

    net::ip::address m_address;
    uint16_t m_port;

    // The io_context is required for all I/O
    net::io_context ioc;

    std::map<std::string , websocket::stream<tcp::socket>*> m_wsClients;

    std::mutex m_mtx;

    // These objects perform our I/O
 //   tcp::resolver resolver{ioc};
};

#endif // TWEBSOCKETSERVERSYNC_H
