#ifndef OWNLIBEVHANDLER_H
#define OWNLIBEVHANDLER_H

#include <ev.h>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <amqpcpp/throttle.h>
#include <amqpcpp/reliable.h>

class OwnLibEvHandler : public AMQP::LibEvHandler
{
public:
    OwnLibEvHandler(struct ev_loop *pLoop);
    virtual ~OwnLibEvHandler() = default;

    /**
     *  Method that is called by the AMQP library when a new connection
     *  is associated with the handler. This is the first call to your handler
     *  @param  connection      The connection that is attached to the handler
     */
    virtual void onAttached(AMQP::TcpConnection *connection) override;

    /**
     *  Method that is called by the AMQP library when the TCP connection
     *  has been established. After this method has been called, the library
     *  still has take care of setting up the optional TLS layer and of
     *  setting up the AMQP connection on top of the TCP layer., This method
     *  is always paired with a later call to onLost().
     *  @param  connection      The connection that can now be used
     */
    virtual void onConnected(AMQP::TcpConnection *connection) override;

    /**
     *  Method that is called when the secure TLS connection has been established.
     *  This is only called for amqps:// connections. It allows you to inspect
     *  whether the connection is secure enough for your liking (you can
     *  for example check the server certificate). The AMQP protocol still has
     *  to be started.
     *  @param  connection      The connection that has been secured
     *  @param  ssl             SSL structure from openssl library
     *  @return bool            True if connection can be used
     */
    virtual bool onSecured(AMQP::TcpConnection *connection, const SSL *ssl) override;

    /**
     *  Method that is called by the AMQP library when the login attempt
     *  succeeded. After this the connection is ready to use.
     *  @param  connection      The connection that can now be used
     */
    virtual void onReady(AMQP::TcpConnection *connection) override;

    /**
     *  Method that is called by the AMQP library when a fatal error occurs
     *  on the connection, for example because data received from RabbitMQ
     *  could not be recognized, or the underlying connection is lost. This
     *  call is normally followed by a call to onLost() (if the error occurred
     *  after the TCP connection was established) and onDetached().
     *  @param  connection      The connection on which the error occurred
     *  @param  message         A human readable error message
     */
    virtual void onError(AMQP::TcpConnection *connection, const char *message) override;

    /**
     *  Method that is called when the AMQP protocol is ended. This is the
     *  counter-part of a call to connection.close() to graceful shutdown
     *  the connection. Note that the TCP connection is at this time still
     *  active, and you will also receive calls to onLost() and onDetached()
     *  @param  connection      The connection over which the AMQP protocol ended
     */
    virtual void onClosed(AMQP::TcpConnection *connection) override;

    /**
     *  Method that is called when the TCP connection was closed or lost.
     *  This method is always called if there was also a call to onConnected()
     *  @param  connection      The connection that was closed and that is now unusable
     */
    virtual void onLost(AMQP::TcpConnection *connection) override;

    /**
     *  Final method that is called. This signals that no further calls to your
     *  handler will be made about the connection.
     *  @param  connection      The connection that can be destructed
     */
    virtual void onDetached(AMQP::TcpConnection *connection) override;

    struct ev_loop *getLoop()
    {
        return m_pLoop;
    }

    void startLoop();
    void stopLoop();

private:
    struct ev_loop *m_pLoop = nullptr;
    void asyncCallback(struct ev_loop *loop, ev_async *, int);
};

/**
 *  Class that runs a timer
 */
class OwnLibEvTimer
{
private:
    /**
     *  The actual watcher structure
     *  @var struct ev_io
     */
    struct ev_timer _timer;

    /**
     *  Pointer towards the AMQP channel
     *  @var AMQP::TcpChannel
     */
    AMQP::TcpChannel *_channel;

    /**
     *  Name of the queue
     *  @var std::string
     */
    std::string _queue;


    /**
     *  Callback method that is called by libev when the timer expires
     *  @param  loop        The loop in which the event was triggered
     *  @param  timer       Internal timer object
     *  @param  revents     The events that triggered this call
     */
    static void callback(struct ev_loop *loop, struct ev_timer *timer, int revents)
    {
        // retrieve the this pointer
        OwnLibEvTimer *self = static_cast<OwnLibEvTimer*>(timer->data);

        // publish a message
        self->_channel->publish("", self->_queue, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");
    }

public:
    /**
     *  Constructor
     *  @param  loop
     *  @param  channel
     *  @param  queue
     */
    OwnLibEvTimer(struct ev_loop *loop, AMQP::TcpChannel *channel, std::string queue) :
        _channel(channel), _queue(std::move(queue))
    {
        // initialize the libev structure
        ev_timer_init(&_timer, callback, 0.005, 1.005);

        // this object is the data
        _timer.data = this;

        // and start it
        ev_timer_start(loop, &_timer);
    }

    /**
     *  Destructor
     */
    virtual ~OwnLibEvTimer()
    {
        // @todo to be implemented
    }
};

#endif // OWNLIBEVHANDLER_H
