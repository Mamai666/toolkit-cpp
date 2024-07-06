#include "BaseTransportClient.h"
#include <dlfcn.h>  // Для динамической загрузки библиотек

BaseTransportClient::BaseTransportClient(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportAny(credentials, instanceName, other)
{
}

BaseTransportClient::~BaseTransportClient()
{
    if(m_pLibBrokerHandle.second)
    {
        LOG(INFO) << "Закрытие m_pLibBrokerHandle: " << m_pLibBrokerHandle.first;
        dlclose(m_pLibBrokerHandle.second);
        LOG(DEBUG) << "dlclose " << m_pLibBrokerHandle.first << "is success!";
    }
}

BaseTransportClient *BaseTransportClient::createBroker(BrokerDescription_t params, std::initializer_list<void *> *other)
{
    std::size_t pos   = params.credentials.find("://");
    std::string proto = params.credentials.substr (0, pos);

    std::string libName = "lib"+proto+"client"+".so";
    std::string libraryPath = "/opt/lms/mtp-libs/message_transport/"+libName;

    LOG(DEBUG) << "Открытие библиотеки динамической брокера " << libraryPath;

    void *libHandle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_NODELETE);  // Загрузка библиотеки
    if(!libHandle)
    {
        LOG(FATAL) <<  "Ошибка загрузки динамической библиотеки брокера"
                   << "(" << libraryPath << "): " << dlerror();
        return nullptr;
    }

    CreateTsClientFnType funcCreateClient = reinterpret_cast<CreateTsClientFnType>(dlsym(libHandle, "createClient"));
    if(funcCreateClient == nullptr)
    {
        LOG(FATAL) << "Ошибка загрузки функции из библиотеки брокера"
                   << "(" << libraryPath << "): " << dlerror();
        dlclose(libHandle);
        return nullptr;
    }

    BaseTransportClient* resPtrClass = funcCreateClient(params.credentials, libName, other);
    if(resPtrClass == nullptr)
    {
        LOG(FATAL) << "Ошибка выделения памяти под объект ITransportClient..";
        return nullptr;
    }

    resPtrClass->setPLibBrokerHandle({libraryPath, libHandle});

    // Настройки resPtrClass через setR-ы ..
    resPtrClass->setIsKeepConnect(params.needKeepConnect);
    //resPtrClass->setWaitOpenMks(params.timeoutOpenMks);
    resPtrClass->setOutDeqCapacity(params.maxLenQueueSend);
    // ...
    //
    return resPtrClass;
}

void BaseTransportClient::send(uint8_t *msgData, size_t msgSize)
{
    std::unique_ptr<uint8_t[]> newMsg = std::make_unique<uint8_t[]>(msgSize + 1);
    std::memcpy(newMsg.get(), msgData, msgSize);

    bool shouldLogWarning = false;
    {
        std::lock_guard<std::mutex> lgMtx(m_mtxDeque);
        if (m_msgOutDeque.size() > capacityMsgDeque())
        {
            shouldLogWarning = true;
            m_msgOutDeque.pop_front();
        }
        m_msgOutDeque.push_back(std::make_tuple(std::move(newMsg), msgSize));

        // Внутри критических секций нельзя логгировать!!
    }

    if (shouldLogWarning)
        LOG(WARNING) << "Очередь заполнена, чистка с начала..!";
}

void BaseTransportClient::run()
{
    if (p_GlobalIsWork == nullptr)
    {
        p_GlobalIsWork = &m_localIsWork;
    }

    while (*p_GlobalIsWork && m_localIsWork)
    {
        bool retOK = true;

        if (m_isKeepConnect || !m_msgOutDeque.empty())
        {
            // Проверка соединения
            retOK = checkConnection();
            if (!retOK)
            {
                changeStatus(TransportState::STATUS_ERROR_CONNECT);

                LOG(ERROR) << "Соединение с " << credentials() << " разорвано!" << std::endl;
                retOK = releaseConnection(m_errorInfo);
                if (!retOK)
                {
                    LOG(ERROR) << "Разъединение не удалось: " << m_errorInfo;
                }
                else
                {
                    // Попытаться соединиться..
                    retOK = createConnection(m_errorInfo);
                    if (!retOK)
                    {
                        LOG(ERROR) << "Соединение не удалось: " << m_errorInfo;
                    }
                    else if (checkConnection())
                    {
                        LOG(INFO) << "Соединение с " << credentials()
                                  << " восстановлено!" << std::endl;
                    }
                    else
                    {
                        retOK = false;
                    }
                }
            }

            if (!retOK) // Что-то не так, подождем и попробуем ещё..
            {
                LOG(ERROR) << credentials() << ": Что-то не так, подождем и попробуем ещё..";
                std::this_thread::sleep_for(std::chrono::microseconds(waitOpenMks()));
                continue;
            }
        }

        changeStatus(TransportState::STATUS_OK);

        if (m_msgOutDeque.empty()) // Очередь пуста, идём на новую итерацию
        {
            std::this_thread::sleep_for(std::chrono::microseconds(delayMks()));
            LOG_EVERY_N(1e6 / delayMks(), DEBUG) << credentials()
                                                 << ": Очередь сообщений на отправку пуста!";
            continue;
        }
        else
        {
            LOG(DEBUG) << credentials()
                       << ": В очереди сообщений на отправку есть элементы ("
                       << m_msgOutDeque.size() << "), обрабатываем.." << std::endl;
        }

        // Все хорошо, едем дальше..
        // Отправка всех накопленных сообщений, по одному за итерацию
        std::unique_ptr<uint8_t[]> ptrMsg;
        size_t sizeMsg;
        {
            std::lock_guard<std::mutex> lgMtx(m_mtxDeque);

            ptrMsg = std::get<0>(std::move(m_msgOutDeque.front()));
            sizeMsg = std::get<1>(m_msgOutDeque.front());

            m_msgOutDeque.pop_front();

            // Внутри критических секций нельзя логгировать!!
        }
        sendToStream(ptrMsg.get(), sizeMsg);

        // Дополнительная постобработка, если необходима
        doAfter();

        if (!m_isKeepConnect)
        {
            retOK = releaseConnection(m_errorInfo);
            if (!retOK)
            {
                LOG(ERROR) << "Ошибка удаления соединения: "
                           << m_errorInfo << std::endl;
            }
        }
    }
    m_localIsWork = false;
}

void BaseTransportClient::setFCallBackResponse(const std::function<void(uint8_t *, size_t)> &newFCallBackResponse)
{
    m_fCallBackResponse = newFCallBackResponse;
}

void BaseTransportClient::setOutDeqCapacity(const size_t newCapacity)
{
    std::lock_guard<std::mutex> lgMtx(m_mtxDeque);
    m_msgOutDeque.clear();
    m_capacityMsgDeque = newCapacity;
}

size_t BaseTransportClient::capacityMsgDeque() const
{
    return m_capacityMsgDeque;
}

void BaseTransportClient::setPLibBrokerHandle(std::pair<std::string, void *> newPLibBrokerHandle)
{
    if (m_pLibBrokerHandle.second != nullptr)
    {
        m_pLibBrokerHandle.first = "";
        dlclose(m_pLibBrokerHandle.second);
    }

    m_pLibBrokerHandle = newPLibBrokerHandle;
}
