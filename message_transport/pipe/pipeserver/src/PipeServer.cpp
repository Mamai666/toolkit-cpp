#include "PipeServer.h"
#include <sys/poll.h>
#include <fstream>
#include <unistd.h>
#include "LoggerPP.h"

PipeServer::PipeServer(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportServer(credentials, instanceName, other)
{
    this->setWaitOpenMks(1e6);
    this->setInDeqCapacity(2);
    this->setIsKeepConnect(true);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        //throw receive_error(this->m_errorInfo);
    }
}

PipeServer::~PipeServer()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }

    if(m_allBuff)
        delete[] m_allBuff;
        
    LOG(DEBUG) << "Конец деструктора PipeServer";
}

bool PipeServer::sendAnswer(uint8_t *buffData, const size_t sizeData,
                                 const std::string senderID,
                                 uint8_t *buffHead, const size_t sizeHead)
{
    if(m_dataPipe.maxSize < (int)sizeData*2)
    {
        PipeTransportKit::changeMaxSize(m_dataPipe, 2*sizeData);
    }

    // PipeTransport::getMaxPipeSize(m_dataPipe);

    PrimaryHeader_t primeHead;
    primeHead.length = sizeof(primeHead);
    primeHead.dataSize = sizeData + sizeHead;

#if 0
    int retBytes = write(m_dataPipe.fd, &primeHead, sizeof(primeHead));
    if(retBytes != (int)sizeof(primeHead))
    {
        LOG(ERROR) << "Ошибка отправки первичного заголовка pipe!"
                   << retBytes << " != " << sizeof(primeHead);
        return false;
    }
    else
    {
        LOG(DEBUG) << "Отправлен первичный заголовок в пайп " << m_dataPipe.path;
    }

    if(buffHead && sizeHead > 0)
    {
        retBytes = write(m_dataPipe.fd, buffHead, sizeHead);
        if(retBytes != (int)sizeHead)
        {
            LOG(ERROR) << "Ошибка отправки заголовка данных! "
                       << retBytes << " != " << sizeHead;
            return false;
        }
        else
        {
            LOG(DEBUG) << "Отправлен заголовок данных в пайп " << m_dataPipe.path;
        }
    }

    retBytes = write(m_dataPipe.fd, buffData, sizeData);
    if(retBytes != (int)sizeData)
    {
        LOG(ERROR) << "Ошибка отправки бинарных данных! "
                   << retBytes << " != " << sizeData;
        return false;
    }
    else
    {
        LOG(DEBUG) << "Отправлены данные в пайп " << m_dataPipe.path
                   << " (" << nowcurrentTimeStampStr() << ")";
    }
#else
// Отправка все одним целым!
    if(m_allBuff == nullptr) {
        m_sizeAllBuff = sizeof(primeHead)+primeHead.dataSize;
        m_allBuff = new uint8_t[m_sizeAllBuff];
    }
    else if((sizeof(primeHead)+primeHead.dataSize) != m_sizeAllBuff)
    {
        delete[] m_allBuff;
        m_sizeAllBuff = sizeof(primeHead)+primeHead.dataSize;
        m_allBuff = new uint8_t[m_sizeAllBuff];
    }

    std::memcpy(&m_allBuff[0], &primeHead, sizeof(primeHead));

    size_t shift = sizeof(primeHead);
    std::memcpy(&m_allBuff[shift], buffHead, sizeHead);
    shift += sizeHead;

    std::memcpy(&m_allBuff[shift], buffData, sizeData);
    shift += sizeData;

    int retBytes = write(m_dataPipe.fd, m_allBuff, shift);
    if(retBytes != (int)shift)
    {
        LOG(ERROR) << "Ошибка отправки бинарных данных! "
                   << retBytes << " != " << shift;
        return false;
    }
    else
    {
        LOG(DEBUG) << "Отправлены данные в пайп " << m_dataPipe.path
                   << " (" << nowcurrentTimeStampStr() << ")";
    }
#endif

    return true;
}

bool PipeServer::createConnection(std::string &errInfo)
{
    // Проверка, правильно ли указаны
    std::string dataPipePath = PipeTransportKit::parsePipePath(this->credentials(), errInfo);
    if(dataPipePath.empty())
    {
        return false;
    }

    m_dataPipe.path  = dataPipePath;
    m_dataPipe.oflag = O_SYNC | O_RDWR; //O_WRONLY;

    m_cmdPipe.path  = m_dataPipe.path + "_cmd";
    m_cmdPipe.oflag = O_SYNC| O_RDONLY | O_NONBLOCK;

    LOG(INFO) << "Создание командного канала " << m_cmdPipe.path << "..";
    bool retOK = PipeTransportKit::initPipeLink(m_cmdPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(DEBUG) << "Канал команд успешно создан и проинициализирован!";

    LOG(INFO) << "Создание канала данных " << m_dataPipe.path << "..";
    retOK = PipeTransportKit::initPipeLink(m_dataPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(DEBUG) << "Канал данных успешно создан и проинициализирован!";

    m_thrListen = std::thread(&PipeServer::startListen, this);
    return true;
}

bool PipeServer::releaseConnection(std::string &errInfo)
{
    this->setLocalIsWork(false);

    if(m_thrListen.joinable())
        m_thrListen.join();

    LOG(INFO) << "Закрытие канала данных " << m_dataPipe.path << ".. ";
    bool retOK = PipeTransportKit::releasePipeLink(m_dataPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(INFO) << "Успешное закрытие канала данных " << m_dataPipe.path << ".. ";

    LOG(INFO) << "Закрытие командного канала " << m_cmdPipe.path << ".. ";
    retOK = PipeTransportKit::releasePipeLink(m_cmdPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(INFO) << "Успешное закрытие командного канала " << m_cmdPipe.path << ".. ";

    return true;
}

void PipeServer::startListen()
{
    PrimaryHeader_t primeHead;
    uint8_t* msgData = nullptr;
    size_t msgDataSize = 0;

    size_t needReadBytes = sizeof(primeHead);
    size_t shift = 0;

    uint8_t *ptrToRead = (uint8_t*)(&primeHead);

    bool isPrimeHeadReaded = false;

    memset(&primeHead, 0, sizeof(primeHead));

    uint32_t cntTempUnavailable = 0;
    while(this->localIsWork() && this->checkConnection() )
    {
        int32_t retBytes = PipeTransportKit::timeoutRead(m_cmdPipe, ptrToRead+shift, needReadBytes, 500);
        //int ret = read(m_cmdPipe.fd, ptrToRead+shift, needReadBytes);
        updateStatus();

        if(retBytes < 0)
        {
            if(cntTempUnavailable > 5)
            {
                LOG(WARNING) << "Командный пайп холост (клиент отключился)?"
                             << m_cmdPipe.path << "errno("<<errno<<"):"
                             << strerror(errno);
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }

            continue;
        }
        else if(retBytes == 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            continue;
        }

        cntTempUnavailable = 0;

        shift += retBytes;
        if(retBytes != static_cast<int32_t>(needReadBytes)) // Запрошенный блок данных вычитан не полностью
        {
            if(errno != EWOULDBLOCK)
            {
                LOG(ERROR) << "Ошибка чтения командного канала: " << m_cmdPipe.path;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            if(retBytes > 0)
            {
                LOG(WARNING) << "1) Запрошенный блок данных вычитан не полностью: "
                             << retBytes << " < " << needReadBytes;
            }

            LOG(DEBUG) << "2) Запрошенный блок данных вычитан не полностью: "
                       << retBytes << " < " << needReadBytes;
            continue;
        }

        // Запрошенный блок данных вычитан полностью

        if(shift == sizeof(primeHead) && needReadBytes <= sizeof(primeHead) && !isPrimeHeadReaded) // Был вычитан блок, равный размеру prime-заголовка
        {
            // LOG(DEBUG) << "Был вычитан блок, равный размеру prime-заголовка";
            if(primeHead.magicWord == 0xFDE8)
            {
                //LOG(DEBUG) << "Обнаружен magic_word!";
                // Magic_Word корректный - этот блок данных и есть prime-заголовок
                if(primeHead.length == sizeof(primeHead))
                {
                    shift             = 0;
                    isPrimeHeadReaded = true;
                    needReadBytes     = primeHead.dataSize;

                    if(msgData == nullptr || msgDataSize != primeHead.dataSize)
                    {
                        if(msgData != nullptr)
                        {
                            delete[] msgData;
                        }

                        msgData = new uint8_t[primeHead.dataSize];
                        msgDataSize = primeHead.dataSize;
                    }

                    memset(&msgData[0], 0, msgDataSize);
                    ptrToRead = (uint8_t*)(msgData);
                    continue;
                }
            }
        }
        else if(shift == primeHead.dataSize && shift > 0) // Вычитаны данные, равные сообщению (запросу)
        {
            // Добавляем сообщение в очередь полученных
            LOG(DEBUG) << "Добавляем сообщение от "<< m_cmdPipe.path
                       << " в очередь полученных (" << nowcurrentTimeStampStr()<<")";

            this->addToIncoming(msgData, msgDataSize, m_cmdPipe.path);

            shift         = 0;
            needReadBytes = sizeof(primeHead);
            memset(&primeHead, 0, sizeof(primeHead));
            ptrToRead = (uint8_t*)(&primeHead);
            isPrimeHeadReaded = false;
        }
       // LOG(DEBUG) << "End Loop!!";
    }
    LOG(DEBUG) << "Завершение TransportServer::startListen()..";
    ptrToRead = nullptr;
    if(msgData != nullptr)
        delete[] msgData;
    //this->setLocalIsWork(false);
}

bool PipeServer::checkConnection()
{
    return (m_dataPipe.fd > 0) && (m_cmdPipe.fd > 0);
}

void PipeServer::doAfter()
{

}
