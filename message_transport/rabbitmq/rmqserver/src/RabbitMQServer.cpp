#include "RabbitMQServer.h"
#include "Utils/mstrings.h"
#include "LoggerPP.h"

RabbitMQServer::RabbitMQServer(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportServer(credentials, instanceName, other)
{
    this->setInDeqCapacity(50);
    this->setIsKeepConnect(true);

    // Инициализация собственного handler-а событий на базе libev
    m_pOwnLibEvHandler = new OwnLibEvHandler(EV_DEFAULT);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw receive_error(this->m_errorInfo);
    }
}

RabbitMQServer::~RabbitMQServer()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }
}

void RabbitMQServer::startListen()
{
    // callback function that is called when the consume operation starts
    auto startCb = [](const std::string &consumertag) {

        LOG(INFO) << "consume operation started";
    };

    // callback function that is called when the consume operation failed
    auto errorCb = [](const char *message) {

        LOG(ERROR) << "consume operation failed";
    };

    // callback operation when a message was received
    auto messageCb = [this](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {

        std::string msgData(reinterpret_cast<const char *>(message.body()), message.bodySize());

        //LOG(DEBUG) << "message received: " << msgData << std::endl;

        this->addToIncoming((uint8_t*)msgData.c_str(),
                            msgData.size(),
                            m_credent.hostPort+"@"+std::to_string(deliveryTag));

        // acknowledge the message
        m_pChannel->ack(deliveryTag);
    };

    // callback that is called when the consumer is cancelled by RabbitMQ (this only happens in
    // rare situations, for example when someone removes the queue that you are consuming from)
    auto cancelledCb = [](const std::string &consumertag) {

        LOG(ERROR) << "consume operation cancelled by the RabbitMQ server" << std::endl;
    };

    // start consuming from the queue, and install the callbacks
    m_pChannel->consume(m_credent.queue)
        .onReceived(messageCb)
        .onSuccess(startCb)
        .onCancelled(cancelledCb)
        .onError(errorCb);
}

bool RabbitMQServer::checkConnection()
{
    std::unique_lock<std::mutex> lock(m_mutexOpen);
    bool retConOK = false;
    if(m_pChannel && m_pConnection)
    {
        if(m_pChannel->ready() && m_pConnection->ready())
        {
            retConOK = true;
        }
        else
        {
            LOG(DEBUG) << "Is isWaitOpenning True";
            m_conditionOpen.wait_for(lock, std::chrono::milliseconds(2000), [&]
                                     {
                                         retConOK = m_pChannel->ready() && m_pConnection->ready();
                                         return retConOK;
                                     });

        }
    }

    //LOG(DEBUG) << "retConOK: " << retConOK;

    return retConOK;
}

bool RabbitMQServer::createConnection(std::string &errInfo)
{
    try
    {
        if(!parseRMQPath(this->credentials(), errInfo))
        {
            return false;
        }

        if(m_pConnection) delete m_pConnection;
        if(m_pChannel) delete m_pChannel;

        std::string connectionPath = "amqp://"+m_credent.user+":"+m_credent.pass+
                                     "@"+m_credent.hostPort;

        m_pConnection = new AMQP::TcpConnection(m_pOwnLibEvHandler,
                                                AMQP::Address(connectionPath));

        LOG(INFO) << "Соединение с " << connectionPath << ", очередь: " << m_credent.queue;

        m_pChannel = new AMQP::TcpChannel(m_pConnection);
        m_pChannel->declareQueue(m_credent.queue, AMQP::durable).onSuccess(
            [](const std::string &name, uint32_t messagecount, uint32_t consumercount)
            {
                LOG(INFO) << "Declared queue " << name << std::endl;
                // now we can close the connection
                //m_pConnection->close();
            });

        m_thrLoop.reset(new std::thread(&OwnLibEvHandler::startLoop, m_pOwnLibEvHandler));
        m_thrLoop->detach();
    }
    catch (const std::exception &e) {
        errInfo = e.what();
        return false;
    }

    m_thrListen = std::thread(&RabbitMQServer::startListen, this);
    return true;
}

bool RabbitMQServer::releaseConnection(std::string &errInfo)
{
    bool retOK = true;
    
    this->setLocalIsWork(false);

    if(m_thrListen.joinable())
        m_thrListen.join();
    
    if (m_pChannel->usable()) {
        retOK = m_pChannel->close();
    }

    if (m_pConnection->usable()) {
        retOK = m_pConnection->close();
    }

    return retOK;
}

bool RabbitMQServer::sendAnswer(uint8_t *buffData, const size_t sizeData,
                                 std::string senderID,
                                 uint8_t *buffHead, const size_t sizeHead)
{

    LOG(WARNING) << "Функция пока не поддерживается на С++ и Python3";
    return false;
}

void RabbitMQServer::doAfter()
{

}

bool RabbitMQServer::parseRMQPath(const std::string credens, std::string &errInfo)
{
    if(Strings::startsWith(credens, "rmq://"))
    {
        auto pos = credens.find("://");
        auto mainPartUri = credens.substr(pos+strlen("://"));
        if(mainPartUri.empty())
        {
            errInfo = "Заданы пустые креды!";
        }
        else
        {
            auto vec = Strings::split(mainPartUri, "@");
            std::string hostPort = "";
            if(vec.size() > 1)
            {
                hostPort = vec.at(1);
                auto vecUserPass = Strings::split(vec.at(0), ":");
                if(vecUserPass.size() == 2)
                {
                    m_credent.user = vecUserPass.at(0);
                    m_credent.pass = vecUserPass.at(1);
                }
                else {
                    errInfo = "Неверно задан логин-пароль!";
                    return false;
                }
            }
            else {
                hostPort = mainPartUri;
            }

            auto vecLastSlash = Strings::split(hostPort, "/");
            if(vecLastSlash.size() == 2)
            {
                auto vecHostPort = Strings::split(vecLastSlash.at(0), ":");
                if(vecHostPort.size() == 2)
                {
                    m_credent.hostPort = vecLastSlash.at(0);
                }
                else {
                    errInfo = "Неверно или не задан port: " + vecLastSlash.at(0);
                              return false;
                }

                m_credent.queue = vecLastSlash.back();

                return true;
            }
            else {
                errInfo = "Не задано имя очереди: " + hostPort;
            }
        }
    }
    else if(Strings::stdv_split(credens, "://").size() > 1) // Какой-то другой протокол
    {
        errInfo = "Неверный тип кредов для rabbitmq: " + credens;
    }

    return false;
}
