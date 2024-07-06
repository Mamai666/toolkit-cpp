#ifndef BASETRANSPORTCLIENT_H
#define BASETRANSPORTCLIENT_H

#include <functional>
#include "BaseTransportAny.h"
#include "loggerpp/include/LoggerPP.h"

// Производный класс для генерирования собственных исключений
class send_error: public std::exception
{
public:
    send_error(const std::string& error): errorMsg{error}
    {}
    const char* what() const noexcept override
    {
        return errorMsg.c_str();
    }

private:
    std::string errorMsg = "none"; // сообщение об ошибке
};

class BaseTransportClient : public BaseTransportAny
{
protected: // Делаем конструктор защищенными от вызова вне потомка

    BaseTransportClient(std::string credentials,
                         std::string instanceName,
                         std::initializer_list<void*> *other = nullptr
                     );

public:

    ~BaseTransportClient();

    using CreateTsClientFnType = BaseTransportClient* (*)(std::string, std::string, std::initializer_list<void*>*);
    static BaseTransportClient* createBroker(BrokerDescription_t params,
                                   std::initializer_list<void*> *other = nullptr);

    virtual uint8_t* readAnswer(uint8_t *buffData, size_t &sizeData,
                            const int timeoutMs) = 0;

    /**
     * @brief send - добавление сообщения в очередь отправки
     * @param msg - сообщение любого типа
     */
    void send(uint8_t *msgData, size_t msgSize);

    /**
     * @brief run - Запуск обработчика отправки и проверки подключения
     */
    void run();

    /**
     * @brief checkConnection - проверка соединения с каналом
     * @return - true/false
     */
    virtual bool     checkConnection() = 0;

    void setFCallBackResponse(const std::function<void (uint8_t* , size_t )> &newFCallBackResponse);

    void setOutDeqCapacity(const size_t newCapacity);
    size_t capacityMsgDeque() const;

protected: // protected функции класса

    /**
     * @brief createConnection - создание соединения с каналом (брокером)
     * @param errInfo - возвращаемое описание ошибки
     * @return - true, в случае успешного соединения
     */
    virtual bool createConnection(std::string &errInfo) = 0;

    /**
     * @brief releaseConnection - удаление соединения с каналом (брокером)
     * @param errInfo - возвращаемое описание ошибки
     * @return - true, в случае успешного удаления соединения
     */
    virtual bool releaseConnection(std::string &errInfo) = 0;

    /**
     * @brief sendToStream - отправка сообщения в канал (брокер)
     * @param msg - сообщение любого типа
     * @return
     */
    virtual uint64_t sendToStream(uint8_t* msgData, const size_t msgSize) = 0;

    /**
     * @brief doAfter - дополнительная постобработка
     *                  после отправки
     */
    virtual void     doAfter() = 0;

    /**
     * @brief m_fCallBackResponse - callback на приём ответного сообщения
     */
    std::function<void(uint8_t* buff, size_t size)> m_fCallBackResponse;

private:
    std::deque< std::tuple< std::unique_ptr<uint8_t[]>, size_t> > m_msgOutDeque;
    size_t     m_capacityMsgDeque;
    std::mutex m_mtxDeque;

    void setPLibBrokerHandle(std::pair<std::string, void*> newPLibBrokerHandle);
    std::pair<std::string, void*> m_pLibBrokerHandle = {"", nullptr};

    // Закрыть некоторые поля и методы от наследников
    using BaseTransportAny::m_isKeepConnect;
    using BaseTransportAny::m_localIsWork;
    using BaseTransportAny::p_GlobalIsWork;
};

#endif // BASETRANSPORTCLIENT_H
