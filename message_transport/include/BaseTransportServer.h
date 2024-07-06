#ifndef BASETRANSPORTSERVER_H
#define BASETRANSPORTSERVER_H

#include <functional>
#include "BaseTransportAny.h"
#include "loggerpp/include/LoggerPP.h"

// Производный класс для генерирования собственных исключений
class receive_error: public std::exception
{
public:
    receive_error(const std::string& error): errorMsg{error}
    {}
    const char* what() const noexcept override
    {
        return errorMsg.c_str();
    }

private:
    std::string errorMsg = "none"; // сообщение об ошибке
};

class BaseTransportServer : public BaseTransportAny
{
protected: // Делаем конструктор защищенными от вызова вне потомка
    
    BaseTransportServer(std::string credentials,
                    std::string instanceName, // from_database, from_analytic
                    std::initializer_list<void*> *other = nullptr // Дополнительные входные данные
                    );
public:

    ~BaseTransportServer();

    static BaseTransportServer* createBroker(BrokerDescription_t params,
                                   std::initializer_list<void*> *other = nullptr);

    virtual bool sendAnswer(uint8_t *buffData, const size_t sizeData,
                            const std::string senderID,
                            uint8_t *buffHead = nullptr, const size_t sizeHead = 0) = 0;

    /**
     * @brief run - Запуск обработчика отправки и проверки подключения
     */
    void run();

    /**
     * @brief checkConnection - проверка соединения с каналом
     * @return true/false
     */
    virtual bool checkConnection() = 0;

    void setFCallBackResponse(const std::function<void (uint8_t*, size_t, const std::string)> &newFCallBackResponse);

    void setInDeqCapacity(const size_t newCapacity);
    size_t capacityMsgDeque() const;
    size_t sizeMsgDeque() const;

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
     * @brief startListen - функция запуска слушания хост-порта на предмет подключений и сообщений
     * Эту функцию нужно вызывать внутри createConnection в отдельном потоке или асинхронно в цикле.
     */
    virtual void startListen() = 0;

    /**
     * @brief doAfter - дополнительная постобработка
     *                  после приема
     */
    virtual void doAfter() = 0;

    /**
     * @brief m_fCallBackReceive - callback на приём на сообщения
     */
    std::function<void(uint8_t* buff, size_t size, const std::string senderID)> m_fCallBackResponse;

    /**
     * @brief addToIncoming - добавление сообщения в очередь полученных
     * @param msg - сообщение любого типа
     * Эта функция должна вызываться внутри конкретного callback-a, при приёме сообщения
     * Сделана Virtual - чтобы была видна в производном классе, но нельзя было менять
     */
    virtual void addToIncoming(uint8_t* msg, size_t size, std::string senderID);

private:
    std::deque< std::tuple< std::unique_ptr<uint8_t[]>, size_t , std::string> > m_msgInDeque;
    size_t     m_capacityMsgDeque;
    std::mutex m_mtxDeque;

    void setPLibBrokerHandle(std::pair<std::string, void*> newPLibBrokerHandle);
    std::pair<std::string, void*> m_pLibBrokerHandle = {"", nullptr};

    // Закрыть некоторые поля и методы от наследников
    using BaseTransportAny::m_isKeepConnect;
    using BaseTransportAny::m_localIsWork;
    using BaseTransportAny::p_GlobalIsWork;
};

#endif // BASETRANSPORTSERVER_H
