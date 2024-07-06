#include "BaseTransportServer.h"
#include <dlfcn.h>  // Для динамической загрузки библиотек

using CreateTsServerFnType = BaseTransportServer* (*)(std::string, std::string, std::initializer_list<void*>*);

BaseTransportServer::BaseTransportServer(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportAny(credentials, instanceName, other)
{
    //setInDeqCapacity(10);
}

BaseTransportServer::~BaseTransportServer()
{
    if(m_pLibBrokerHandle.second)
    {
        LOG(DEBUG) << "Закрытие m_pLibBrokerHandle: " << m_pLibBrokerHandle.first;
        dlclose(m_pLibBrokerHandle.second);
        LOG(DEBUG) << "dlclose " << m_pLibBrokerHandle.first << "is success!";
    }
}

BaseTransportServer *BaseTransportServer::createBroker(BrokerDescription_t params, std::initializer_list<void *> *other)
{
    std::size_t pos   = params.credentials.find("://");
    std::string proto = params.credentials.substr (0, pos);

    std::string libName = "lib"+proto+"server"+".so";
    std::string libraryPath = "/opt/lms/mtp-libs/message_transport/"+libName;

    LOG(DEBUG) << "Открытие библиотеки динамической брокера " << libraryPath;

    void* libHandle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_NODELETE);  // Загрузка библиотеки
    if(!libHandle)
    {
        LOG(FATAL) <<  "Ошибка загрузки динамической библиотеки брокера"
                   << "(" << libraryPath << "): " << dlerror();
        return nullptr;
    }

    CreateTsServerFnType funcCreateServer = reinterpret_cast<CreateTsServerFnType>(dlsym(libHandle, "createServer"));
    if(funcCreateServer == nullptr)
    {
        LOG(FATAL) << "Ошибка загрузки функции из библиотеки брокера"
                   << "(" << libraryPath << "): " << dlerror();
        dlclose(libHandle);
        return nullptr;
    }

    BaseTransportServer* resPtrClass = funcCreateServer(params.credentials, libName, other);
    if(resPtrClass == nullptr)
    {
        LOG(FATAL) << "Ошибка выделения памяти под объект ITransportServer..";
        return nullptr;
    }

    resPtrClass->setPLibBrokerHandle({libraryPath, std::move(libHandle)});

    // Настройки resPtrClass через setR-ы ..
    resPtrClass->setIsKeepConnect(params.needKeepConnect);
    //resPtrClass->setWaitOpenMks(params.timeoutOpenMks);
    resPtrClass->setInDeqCapacity(params.maxLenQueueRecv);
    // ...
    //
    return resPtrClass;
}

void BaseTransportServer::run()
{
    if(p_GlobalIsWork == nullptr) {
        p_GlobalIsWork = &m_localIsWork;
    }

    while(*p_GlobalIsWork && m_localIsWork)
    {
        if(m_isKeepConnect || sizeMsgDeque() > 0)
        {
            // Проверка соединения
            bool retOK = checkConnection();
            if(!retOK)
            {
                changeStatus(TransportState::STATUS_ERROR_CONNECT);

                LOG(ERROR) << "Соединение с " << credentials() << " разорвано!" << std::endl;
                // Попытаться соединиться..
                retOK = createConnection(m_errorInfo);
                if(!retOK)
                {
                    LOG(ERROR) << "Повторное Соединение не удалось: " << m_errorInfo;
                }
                else
                {
                    LOG(INFO) << "Соединение с " << credentials()
                              <<" восстановлено!" << std::endl;
                }
            }

            if(!retOK) // Что-то не так, подождем и попробуем ещё..
            {
                LOG(ERROR) << "Что-то не так, подождем и попробуем ещё..";
                std::this_thread::sleep_for(std::chrono::microseconds(waitOpenMks()));
                continue;
            }
        }

        changeStatus(TransportState::STATUS_OK);

        if(sizeMsgDeque() == 0) // Очередь пуста, идём на новую итерацию
        {
            std::this_thread::sleep_for(std::chrono::microseconds(delayMks()));
            LOG_EVERY_N(1e6/delayMks(), DEBUG) << "Очередь принятых сообщений пуста!";
            continue;
        }
        else {
            LOG(DEBUG) << "В очереди принятых сообщений есть элементы, обрабатываем.." << std::endl;
        }

        // Все хорошо, едем дальше..
        if(m_fCallBackResponse)
        {
            LOG(DEBUG) << "Время вызова обработки запроса: " << nowcurrentTimeStampStr();
                // Коллбэк на принятое сообщение (по одному за итерацию)

            std::unique_ptr<uint8_t[]> ptrMsg;
            size_t                     sizeMsg;
            std::string                senderID;
            {
                std::lock_guard<std::mutex> lgMtx(m_mtxDeque);
                ptrMsg = std::get<0>(std::move(m_msgInDeque.front()));
                sizeMsg = std::get<1>(m_msgInDeque.front());
                senderID = std::get<2>(m_msgInDeque.front());

                m_msgInDeque.pop_front();
            }
            m_fCallBackResponse(ptrMsg.get(), sizeMsg, senderID);
        }

        // Дополнительная постобработка, если необходима
        doAfter();

        if(!m_isKeepConnect)
        {
            bool retOK = releaseConnection(m_errorInfo);
            if(!retOK)
            {
                LOG(ERROR) << "Ошибка удаления соединения: "
                           << m_errorInfo << std::endl;
            }
        }
    }
    m_localIsWork = false;
}

void BaseTransportServer::setFCallBackResponse(const std::function<void (uint8_t *, size_t, const std::string)> &newFCallBackResponse)
{
    m_fCallBackResponse = newFCallBackResponse;
}

void BaseTransportServer::setInDeqCapacity(const size_t newCapacity)
{
    std::lock_guard<std::mutex> lgMtx(m_mtxDeque);
    m_msgInDeque.clear();
    m_capacityMsgDeque = newCapacity;
}

size_t BaseTransportServer::capacityMsgDeque() const
{
    return m_capacityMsgDeque;
}

size_t BaseTransportServer::sizeMsgDeque() const
{
    //std::lock_guard<std::mutex> lgMtx(m_mtxDeque); // Мьютекс не включать - иначе двойная блокировка
    size_t currInDeqSize = m_msgInDeque.size();
    return currInDeqSize;
}

void BaseTransportServer::addToIncoming(uint8_t *msg, size_t size, std::string senderID)
{
    std::unique_ptr<uint8_t[]> newMsg = std::make_unique<uint8_t[]>(size+1);
    std::memcpy(newMsg.get(), msg, size);

    std::lock_guard<std::mutex> lgMtx(m_mtxDeque);
    if(sizeMsgDeque() > capacityMsgDeque())
    {
        LOG(WARNING) << "Очередь заполнена, чистка по одному с начала..!";
        m_msgInDeque.pop_front();
    }

    //LOG(DEBUG) << "newMsg: " << newMsg.get();

    m_msgInDeque.push_back(std::make_tuple(std::move(newMsg), size, std::move(senderID)));
}

void BaseTransportServer::setPLibBrokerHandle(std::pair<std::string, void *> newPLibBrokerHandle)
{
    if(m_pLibBrokerHandle.second != nullptr)
    {
        m_pLibBrokerHandle.first = "";
        dlclose(m_pLibBrokerHandle.second);
    }

    m_pLibBrokerHandle = newPLibBrokerHandle;
}
