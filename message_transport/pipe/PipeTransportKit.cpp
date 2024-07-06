#include "PipeTransportKit.h"
#include "LoggerPP.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/poll.h>


PipeTransportKit::PipeTransportKit()
{

}

int PipeTransportKit::getMaxPipeSize(const PipeDesc_t &pipe)
{
    auto retFcntl = fcntl(pipe.fd, F_GETPIPE_SZ);
    LOG(DEBUG) << "Текущий макс. размер пайпа: " << retFcntl;
    return retFcntl;
}

bool PipeTransportKit::initPipeLink(PipeDesc_t &pipe, std::string &errInfo)
{
    if(!createPipeFile(pipe.path, errInfo))
    {
        return false;
    }

    if(!openPipeFile(pipe, errInfo))
    {
        return false;
    }

    return true;
}

bool PipeTransportKit::releasePipeLink(PipeDesc_t &pipe, std::string &errInfo)
{
    if(!closePipeFile(pipe, errInfo))
    {
        return false;
    }

    if(!pipe.path.empty() && remove(pipe.path.c_str()) != 0)
    {
        errInfo = "Ошибка удаления канала " + pipe.path ;
        return false;
    }
    return true;
}

bool PipeTransportKit::closePipeFile(PipeDesc_t &pipe, std::string &errInfo)
{
    if(pipe.fd)
    {
        pipe.isOpen = false;
        if(::close(pipe.fd) != 0)
        {
            errInfo = "Ошибка закрытия канала " + pipe.path
                      + "; errno("+std::to_string(errno)+"): "
                      +std::strerror(errno);
            pipe.fd = 0;
            return false;
        }
    }
    pipe.fd = 0;

    return true;
}

bool PipeTransportKit::createPipeFile(const std::string &path, std::string &errInfo)
{
    if(access(path.c_str(), F_OK) == 0)
    {
        // pipe уже существует
        LOG(WARNING) << "Пайп-файл " << path << " уже создан! Пересоздание..";
        if(remove(path.c_str()) != 0)
        {
            errInfo = "Ошибка удаления пайп-файла " + path
                      + "; errno("+std::to_string(errno)+"): "
                      +std::strerror(errno);
            return false;
        }
    }

    mkfifo(path.c_str(), 0777);

    return true;
}

bool PipeTransportKit::openPipeFile(PipeDesc_t &pipe, std::string &errInfo)
{
    pipe.fd = ::open(pipe.path.c_str(), pipe.oflag);
    if(pipe.fd <= 0)
    {
        errInfo = "Ошибка открытия канала " + pipe.path
                  + "; errno("+std::to_string(errno)+"): "
                  +std::strerror(errno);
        return false;
    }

    pipe.isOpen = true;
    pipe.maxSize = getMaxPipeSize(pipe);

    return true;
}

std::string PipeTransportKit::parsePipePath(const std::string credens, std::string &errInfo)
{
    std::string pipePath = credens;
    if(Strings::startsWith(credens, "pipe://"))
    {
        auto pos = credens.find("://");
        pipePath = credens.substr (pos+strlen("://"));
        if(pipePath.empty())
        {
            errInfo = "Заданы пустые креды!";
            return "";
        }
    }
    else if(Strings::stdv_split(credens, "://").size() > 1) // Какой-то другой протокол
    {
        errInfo = "Неверный тип кредов для pipe: " + credens;
        return "";
    }

    return pipePath;
}

void PipeTransportKit::changeMaxSize(PipeDesc_t &pipe, size_t newSize)
{
    int ret = fcntl(pipe.fd, F_SETPIPE_SZ, newSize);
    if(ret == -1)
    {
        LOG(ERROR) << "Ошибка изменения размера канала " << pipe.path
                   << "; errno("<<errno<<"): " << std::strerror(errno);
    }

    pipe.maxSize = fcntl(pipe.fd, F_GETPIPE_SZ);
    if(pipe.maxSize < 0)
        pipe.isOpen = false;

    LOG(WARNING) << "Новый макс. размер пайпа: " << pipe.maxSize;
}

int32_t PipeTransportKit::timeoutRead (PipeDesc_t &pipe, uint8_t* buf, size_t size, int mlsec_timeout)
{
    struct pollfd fd = { .fd = pipe.fd, .events = POLLIN };

    int bytesread = 0;

    while (poll (&fd, 1, mlsec_timeout) == 1)
    {
        // check pipe disconnect event to prevent read/write data through
        if (fd.revents & POLLHUP) {
            if(pipe.isOpen) {
                LOG(ERROR) << "Пайп " << pipe.path << " был отключен!";
                pipe.isOpen = false;
            }
            usleep(mlsec_timeout*1000);
            break;
        }

        if(pipe.fd <= 0)
        {
            LOG(ERROR) << "Pipe.fd <=0 : " << pipe.path;
            pipe.isOpen = false;
            break;
        }

        bytesread = read (pipe.fd, buf + bytesread, size);

        if (bytesread == -1)
        {
            LOG(ERROR) << "read fd error: " << std::strerror(errno); /* an error accured */
            return -1;
        }

        if (bytesread > 0)
        {
            if(pipe.isOpen == false) {
                LOG(WARNING) << "Пайп " << pipe.path << " снова подключился!";
                pipe.isOpen = true;
            }
            return bytesread;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1)); // Нужен, иначе загрузка 100%
    }

    if(bytesread == 0)
    {
        LOG(DEBUG) << "Истекло время ожидания пакета.."; /* a timeout occured */
    }
    return bytesread;
}

bool PipeTransportKit::readInNonBLock(const PipeDesc_t &pipe, void *data, ssize_t size)
{
    if(size <= 0)
        return false;

    // bool ret = true;
    int64_t got = 0, rBytes = 0;
    auto *frame_p = (uint8_t*)data;

    int cntBlockInLoop = 0;
    int cntIterateLoop = 0;

    for(int i = 0; i < 1000000; ++i)
    {
        ++cntIterateLoop;

        rBytes = size;
        got = read(pipe.fd, frame_p, rBytes);

        if(got == 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(1)); // труба ещё не подключилась
            continue;
        }

        if(got < 0 && errno == EWOULDBLOCK)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            ++cntBlockInLoop;
            continue;
        }

        if(got != rBytes)
        {
            LOG(ERROR) << "ОШИБКА ПРИЁМА ВХОДЯЩИХ ДАННЫХ! " << got << " != " << rBytes;
            // ret = false;
            break;
        }

        size -= got;

        if(size <= 0)
            break;

        frame_p += got;
    }

    //fprintf(stdout, "cntIterateLoop: %d, cntBlockInLoop: %d\n", cntIterateLoop, cntBlockInLoop);
    //fflush(stdout);

    return (size <=0);
}
