#ifndef PIPETRANSPORTKIT_H
#define PIPETRANSPORTKIT_H

#include <string>
#include <stdint.h>
#include <fcntl.h>
#include "Utils/mstrings.h"

#ifndef PrimaryHeaderStruct
#define PrimaryHeaderStruct
struct alignas(4) PrimaryHeader_t
{
    uint16_t magicWord = 0xFDE8;
    uint8_t length = sizeof(PrimaryHeader_t);
    uint8_t typeMsg = 0;
    uint32_t dataSize;
};
#endif

struct PipeDesc_t
{
    std::string path    = "";
    int         fd      = -1;
    int         maxSize = 0;
    int         oflag   = O_RDONLY;
    bool        isOpen  = false;
};

class PipeTransportKit
{
public:

    PipeTransportKit();

    static bool initPipeLink(PipeDesc_t &pipe, std::string &errInfo);
    static bool releasePipeLink(PipeDesc_t &pipe, std::string &errInfo);

    static bool createPipeFile(const std::string &path, std::string &errInfo);
    static bool openPipeFile(PipeDesc_t &pipe, std::string &errInfo);

    static std::string parsePipePath(const std::string credens, std::string &errInfo);

    static void changeMaxSize(PipeDesc_t &pipe, size_t newSize);
    static int getMaxPipeSize(const PipeDesc_t &pipe);

    /**
     * @brief Получить блок данных с сервера
     * @param data Буфер для входящих данных
     * @param size Размер запрашиваемых данных
     * @return Удалось ли принять данные?
     */
    static bool readInNonBLock(const PipeDesc_t &pipe, void *data, ssize_t size);

    static bool closePipeFile(PipeDesc_t &pipe, std::string &errInfo);
    static int32_t timeoutRead(PipeDesc_t &pipe, uint8_t *buf, size_t size, int mlsec_timeout);
};

#endif // PIPETRANSPORTKIT_H
