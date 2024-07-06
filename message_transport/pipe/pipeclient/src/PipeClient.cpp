#include "PipeClient.h"
#include "Utils/time_convert.h"
#include "LoggerPP.h"
#include <fstream>
#include <unistd.h>

PipeClient::PipeClient(std::string credentials, std::string instanceName, std::initializer_list<void *> *other)
    : BaseTransportClient(credentials, instanceName, other)
{
    this->setWaitOpenMks(1e6);
    this->setOutDeqCapacity(2);
    this->setIsKeepConnect(true);

    if(!this->createConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка создания соединения: "
                   << this->m_errorInfo << std::endl;

        //throw send_error(this->m_errorInfo);
    }
}

PipeClient::~PipeClient()
{
    if(!this->releaseConnection(this->m_errorInfo))
    {
        LOG(ERROR) << "Ошибка удаления соединения: "
                   << this->m_errorInfo << std::endl;
    }
}

uint8_t *PipeClient::readAnswer(uint8_t *buffData, size_t& sizeData,
                                 const int timeoutMs)
{
    PrimaryHeader_t primeHead;
    primeHead.length = sizeof(primeHead);
    primeHead.dataSize = sizeData;
    int del = 0; // Флаг, что была выделена память внутри и нужно почистить

    int32_t retBytes = PipeTransportKit::timeoutRead(m_dataPipe, (uint8_t*)&primeHead, sizeof(primeHead), timeoutMs);
    updateStatus();
    if(retBytes != sizeof(primeHead))
    {
        if(retBytes != 0) {
            LOG(ERROR) << "Ошибка чтения первичного заголовка pipe: " << retBytes << "!=" << sizeof(primeHead)
                       << "; errno("<<errno<<"): " << std::strerror(errno);
        }
        return nullptr;
    }

    if(primeHead.magicWord != 0xFDE8 || primeHead.length != sizeof(PrimaryHeader_t))
    {
        LOG(ERROR) << "Битый первичный заголовок!";
        return nullptr;
    }

    if(buffData == nullptr) {
        LOG(DEBUG) << "Выделение памяти: " << primeHead.dataSize*2;
        buffData = new uint8_t[primeHead.dataSize*2];
        memset(buffData, 0, primeHead.dataSize*2);
        sizeData = primeHead.dataSize;
        del = 1;
    }

    if (buffData == nullptr) {
        return nullptr;
    }

    if(sizeData < primeHead.dataSize){
        LOG(ERROR) << "Не передано достаточно памяти для вложения ответа!";
        if (del) delete buffData;
        return nullptr;
    }

    retBytes = PipeTransportKit::timeoutRead(m_dataPipe, buffData, primeHead.dataSize, timeoutMs);
    if(retBytes != static_cast<int32_t>(primeHead.dataSize))
    {
        LOG(ERROR) << "Ошибка чтения данных: " << retBytes <<"!="<<primeHead.dataSize;
        if (del) delete buffData;
        sizeData = 0;
        return nullptr;
    }
    else {
        LOG(DEBUG) << "Данные успешно получены: " << MTime::nowTimeStamp();
        sizeData = primeHead.dataSize;
    }

    return buffData;
}

bool PipeClient::createConnection(std::string &errInfo)
{
    // Проверка, правильно ли указаны
    std::string dataPipePath = PipeTransportKit::parsePipePath(this->credentials(), errInfo);
    if(dataPipePath.empty())
    {
        return false;
    }

    m_dataPipe.path  = dataPipePath;
    m_dataPipe.oflag = O_RDONLY | O_NONBLOCK;

    m_cmdPipe.path  = m_dataPipe.path + "_cmd";
    m_cmdPipe.oflag = O_WRONLY | O_NONBLOCK;

    LOG(INFO) << "Открытие канала данных " << m_dataPipe.path << "..";
        bool retOK = PipeTransportKit::openPipeFile(m_dataPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(INFO) << "Открытие командного канала " << m_cmdPipe.path << "..";
        retOK = PipeTransportKit::openPipeFile(m_cmdPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    return true;
}

bool PipeClient::releaseConnection(std::string &errInfo)
{
    LOG(INFO) << "Закрытие канала данных " << m_dataPipe.path << ".. ";
        bool retOK = PipeTransportKit::closePipeFile(m_dataPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    LOG(INFO) << "Закрытие командного канала " << m_cmdPipe.path << ".. ";
        retOK = PipeTransportKit::closePipeFile(m_cmdPipe, errInfo);
    if(!retOK)
    {
        return false;
    }

    return true;
}

uint64_t PipeClient::sendToStream(uint8_t* msgData, const size_t msgSize)
{
    if(m_cmdPipe.maxSize < (int)msgSize)
    {
        PipeTransportKit::changeMaxSize(m_cmdPipe, msgSize);
    }
    if(!m_cmdPipe.isOpen)
    {
        LOG(ERROR) << "Пайп похоже закрылся: " << m_cmdPipe.path;
        return 0;
    }

    PrimaryHeader_t primeHead;
    primeHead.length = sizeof(primeHead);
    primeHead.dataSize = msgSize;
    int retBytes = write(m_cmdPipe.fd, &primeHead, sizeof(primeHead));
    if(retBytes != (int)sizeof(primeHead))
    {
        LOG(ERROR) << "Ошибка отправки первичного заголовка pipe!" << retBytes << " != " << sizeof(primeHead)
                   << "; errno("<<errno<<"): " << std::strerror(errno);
        return false;
    }

    retBytes = write(m_cmdPipe.fd, msgData, msgSize);
    if(retBytes != (int)msgSize)
    {
        LOG(ERROR) << "Ошибка отправки сообщения! " << retBytes << " != " << msgSize
                   << "; errno("<<errno<<"): " << std::strerror(errno);
        return false;
    }
    LOG(DEBUG) << "Запрос успешно отправлен " << MTime::nowTimeStamp();
    return true;
}

bool PipeClient::checkConnection()
{
    return (m_dataPipe.fd > 0) && m_dataPipe.isOpen && (m_cmdPipe.fd > 0) && m_cmdPipe.isOpen;
}

void PipeClient::doAfter()
{

}
