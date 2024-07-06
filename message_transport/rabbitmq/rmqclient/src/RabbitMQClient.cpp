#include "RabbitMQClient.h"
#include "Timer.h"
#include "Utils/mstrings.h"
#include "LoggerPP.h"

RabbitMQClient::RabbitMQClient(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportClient(credentials, instanceName, other)
{
    this->setOutDeqCapacity(50);
    this->setIsKeepConnect(true);

    // Инициализация собственного handler-а событий на базе libev
    m_pOwnLibEvHandler = new OwnLibEvHandler(EV_DEFAULT);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        throw send_error(this->m_errorInfo);
    }
}

RabbitMQClient::~RabbitMQClient()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }

//    if(m_thrLoop->joinable())
//        m_thrLoop->join();
}

uint8_t *RabbitMQClient::readAnswer(uint8_t *buffData, size_t& sizeData,
                                 const int timeoutMs)
{
    LOG(WARNING) << "Функция пока не поддерживается на С++ и Python3";
    return nullptr;
}



bool RabbitMQClient::parseRMQPath(const std::string credens, std::string &errInfo)
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
            auto pos = mainPartUri.find("@");
            std::string hostPort = "";
            if(pos != std::string::npos)
            {
                hostPort = mainPartUri.substr(pos+1, mainPartUri.size());

                auto vecUserPass = Strings::split(mainPartUri.substr(0, pos), ":");
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

bool RabbitMQClient::createConnection(std::string &errInfo)
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

        this->changeStatus(TransportState::STATUS_ERROR_CONNECT);

        LOG(INFO) << "Соединение с " << connectionPath << ", очередь: " << m_credent.queue;

        m_pChannel = new AMQP::TcpChannel(m_pConnection);

        bool isDeclareOK = false;

        m_pChannel->declareQueue(m_credent.queue, AMQP::passive).onSuccess(
        [&isDeclareOK](const std::string &name, uint32_t messagecount, uint32_t consumercount)
        {
            LOG(INFO) << "Declared queue " << name << std::endl;
            isDeclareOK = true;
            // now we can close the connection
            //m_pConnection->close();
        })
        .onError([](const char *message) {
            LOG(ERROR) << "Ошибка Declared rabbitmq: " << message;
        });

        m_thrLoop.reset(new std::thread(&OwnLibEvHandler::startLoop, m_pOwnLibEvHandler));
        m_thrLoop->detach();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(!isDeclareOK)
            return false;

        return true;
    }
    catch (const std::exception &e) {
        errInfo = e.what();
    }
    catch(...)
    {
        errInfo = "Поймал неизвестное исключение!";
    }

    return false;
}

bool RabbitMQClient::releaseConnection(std::string &errInfo)
{
    bool retOK = true;
    if (m_pChannel->usable()) {
        retOK = m_pChannel->close();
    }

    if (m_pConnection->usable()) {
        retOK = m_pConnection->close();
    }

    return retOK;
}

uint64_t RabbitMQClient::sendToStream(uint8_t* msgData, const size_t msgSize)
{
    try
    {
        AMQP::Envelope envelope(reinterpret_cast<const char *>(msgData), msgSize);
        // m_pChannel->startTransaction();
        m_pChannel->publish("", m_credent.queue, envelope);
        /*m_pChannel->commitTransaction()
            .onSuccess([]() {
                LOG(INFO) << "Сообщение успешно отправленно в rabbitmq!";
            })
            .onError([](const char *message) {
                LOG(ERROR) << "Ошибка отправки сообщения в rabbitmq: " << message;
            });
	*/
//       // create a channel
//       AMQP::TcpChannel mychannel(m_pConnection);
//       AMQP::Reliable reliable(mychannel);

//       reliable.publish("", m_credent.queue, envelope)
//        .onAck([]() {
//            LOG(INFO) << "Сообщение успешно отправленно в rabbitmq!";
//             // the message has been acknowledged by RabbitMQ (in your application
//             // code you can now safely discard the message as it has been picked up)

//        }).onNack([]() {

//            LOG(WARNING) << "Нет ответа при отправке сообщения в rabbitmq!";
//            // the message has _explicitly_ been nack'ed by RabbitMQ (in your application
//            // code you probably want to log or handle this to avoid data-loss)

//        }).onLost([]() {

//            LOG(ERROR) << "Потеря при отправке сообщения в rabbitmq!";
//            // because the implementation for onNack() and onError() will be the same
//            // in many applications, you can also choose to install a onLost() handler,
//            // which is called when the message has either been nack'ed, or lost.

//        }).onError([](const char *message) {

//            LOG(ERROR) << "Ошибка отправки сообщения в rabbitmq: " << message;
//            // a channel-error occurred before any ack or nack was received, and the
//            // message is probably lost (which you might want to handle)

//        });

        return envelope.bodySize();
    }
    catch (...) {
        LOG(ERROR) << "RabbitClient exception!";
        return 0;
    }
}

bool RabbitMQClient::checkConnection()
{
    bool retConOK = false;
    if(m_pChannel && m_pConnection)
    {
        if(m_pChannel->ready() && m_pConnection->ready())
        {
            retConOK = true;
        }
        else
        {
            std::unique_lock<std::mutex> lock(m_mutexOpen);

            LOG(DEBUG) << "Is isWaitOpenning True";
            m_conditionOpen.wait_for(lock, std::chrono::milliseconds(2000), [&]
             {
                 retConOK = m_pChannel->ready() && m_pConnection->ready();
                 return retConOK;
             });
        }

        //Timer tmr; tmr.start();

//        m_pChannel->get(m_credent.queue.c_str())
//        .onError([&tmr](const char *message) {
//            LOG(ERROR) << "Queue is error: " << message << std::endl;
//        });



//        Timer tmr; tmr.start();

//        int isDeclareState = -1;

//        m_pChannel->declareQueue(m_credent.queue, AMQP::durable | AMQP::passive)
//            .onSuccess([&isDeclareState](const std::string &name, uint32_t messagecount, uint32_t consumercount)
//            {
//                isDeclareState = 1;
//                LOG(INFO) << "Success Declared queue " << name << std::endl;
//            })
//            .onError([&isDeclareState](const char *message) {
//                isDeclareState = 0;
//                LOG(ERROR) << "Error Declared rabbitmq: " << message;
//            });

//        for(int i = 0; i < 500; ++i)
//        {
//            if(isDeclareState >= 0)
//                break;

//            std::this_thread::sleep_for(std::chrono::microseconds(150));
//        }

//        std::cerr << "declareQueue tmr.elapsed, mks: " << tmr.elapsedMicroseconds() << "\n";

//        if(isDeclareState == 0 )
//        {

//            return false;
//        }
    }

    return retConOK;
}

void RabbitMQClient::doAfter()
{

}
