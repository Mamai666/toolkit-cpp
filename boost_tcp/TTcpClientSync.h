#ifndef TTCPCLIENTSYNC_H
#define TTCPCLIENTSYNC_H

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "include_logger.h"

using namespace boost::asio;

class TTcpClientSync
{
public:
    TTcpClientSync(const std::string host, const uint16_t port)
    {
        m_host = host;
        m_port = port;
        f_host = m_host + ':' + std::to_string(m_port);
    }

    bool connectOn()
    {
        try {
            auto ep = ip::tcp::endpoint(ip::address::from_string(m_host), m_port);
            //m_socket.non_blocking(true);

            //deadline_.expires_from_now(boost::posix_time::seconds (5));

            m_socket.connect(ep, m_errCode);
            if(m_errCode) {
                LOG(ERROR) << "Failed connect-tcp : " << m_errCode.message();
                isConnected = false;
                return false;
            }
            LOG(INFO) << "Connect to tcp-server: tcp://" << f_host << std::endl;
            isConnected = true;
        }
        catch(boost::exception const& e)
        {
            LOG(ERROR) << "Failed Connect to tcp-server (" << f_host << "): "
                << dynamic_cast<std::exception const&>(e).what() << std::endl;
            isConnected = false;
            return false;
        }
        return true;
  }

  ~TTcpClientSync() {
      try {
          m_socket.close();
      }
      catch(boost::exception const& e)
      {
          LOG(ERROR) << "Connect to tcp-server (" << f_host << "): "
              << dynamic_cast<std::exception const&>(e).what() << std::endl;
      }
  }

  size_t readOn(char* data, const size_t needSize)
  {
      boost::asio::streambuf buf;
      auto rdByte = boost::asio::read(m_socket, buf,
                                       boost::asio::transfer_exactly(needSize));
      if(rdByte > 0) {
         memcpy(data, buf.data().data(), buf.data().size());
      }

      return rdByte;
  }

  size_t writeOn(const std::string message)
  {
      auto wrByte = boost::asio::write(m_socket, buffer(message));
      return wrByte;
  }

  bool getIsConnected() const
  {
      return isConnected;
  }

private:

  io_context m_io_context;
  std::string m_host = "127.0.0.1";
  uint16_t m_port = 8080;
  std::string f_host = "";

  boost::system::error_code m_errCode;

  ip::tcp::socket m_socket{m_io_context};

  bool isConnected = false;

};

#endif // TTCPCLIENTSYNC_H
