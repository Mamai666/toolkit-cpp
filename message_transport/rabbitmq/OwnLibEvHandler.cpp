#include "OwnLibEvHandler.h"
#include "LoggerPP.h"

void OwnLibEvHandler::asyncCallback(EV_P_ ev_async*, int)
{
    ev_break(m_pLoop, EVBREAK_ONE);
}

void OwnLibEvHandler::stopLoop()
{
    //ev_async_init(&m_asyncWatcher, asyncCallback);
    //ev_async_start(m_pLoop, &m_asyncWatcher);
    //ev_async_send(m_pLoop, &m_asyncWatcher);

    //m_thread->join();
}

OwnLibEvHandler::OwnLibEvHandler(struct ev_loop *pLoop) : AMQP::LibEvHandler(pLoop)
{
    m_pLoop = pLoop;
}

void OwnLibEvHandler::onAttached(AMQP::TcpConnection *connection)
{
    LOG(INFO) << "Call onAttached!";
}

void OwnLibEvHandler::onConnected(AMQP::TcpConnection *connection)
{
    LOG(INFO) << "Call onConnected!";
}

bool OwnLibEvHandler::onSecured(AMQP::TcpConnection *connection, const SSL *ssl)
{
    // @todo
    //  add your own implementation, for example by reading out the
    //  certificate and check if it is indeed yours
    return true;
}

void OwnLibEvHandler::onReady(AMQP::TcpConnection *connection)
{
    LOG(INFO) << "Call onReady!";
}

void OwnLibEvHandler::onError(AMQP::TcpConnection *connection, const char *message)
{
    LOG(ERROR) << "Call onError: " << message;
}

void OwnLibEvHandler::onClosed(AMQP::TcpConnection *connection)
{
    // @todo
    //  add your own implementation (probably not necessary, but it could
    //  be useful if you want to do some something immediately after the
    //  amqp connection is over, but do not want to wait for the tcp
    //  connection to shut down
    LOG(WARNING) << "Call onClosed!";
}

void OwnLibEvHandler::onLost(AMQP::TcpConnection *connection)
{
    LOG(WARNING) << "Call onLost!";
}

void OwnLibEvHandler::onDetached(AMQP::TcpConnection *connection)
{
    LOG(WARNING) << "Call onDetached!";
}

void OwnLibEvHandler::startLoop()
{
    ev_loop(m_pLoop, 0); // Start Libev loop
}
