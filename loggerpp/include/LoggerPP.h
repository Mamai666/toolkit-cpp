#ifndef LOGGERPP_H
#define LOGGERPP_H

#define ELPP_NO_DEFAULT_LOG_FILE
#include "../3rdparty/easylogging++.h"

class LoggerPP
{
public:

    struct LogDispatchMsg_t
    {
        std::string logLevel;
        std::string timestamp;
        std::string msgText;
    };

    class DispatchLogger : public el::LogDispatchCallback
    {
    public:
        void setF_callBackFullLogLine(const std::function<void (el::base::type::string_t &&)> &newF_callBackFullLogLine);
        void setF_callBackLogMsg(const std::function<void (const LogDispatchMsg_t&)> &newF_callBackLogMsg);

    protected:
        void handle(const el::LogDispatchData* data) noexcept override;

    private:
        //const el::LogDispatchData* m_data;
        std::function<void(el::base::type::string_t&&)> f_callBackFullLogLine = nullptr;
        std::function<void(const LogDispatchMsg_t& logData)> f_callBackLogMsg = nullptr;
    };

    LoggerPP(const std::string &logConfPath, const std::string &suffixForLog);
    ~LoggerPP()
    {
        if(m_pElConf)
            delete m_pElConf;
    }

    void run(bool *isWorking = nullptr);

    el::Configurations *pElConf() const;

    uint32_t timeLiveDays() const;
    void setTimeLiveDays(uint32_t newTimeLiveDays);

private:
    el::Configurations *m_pElConf = nullptr;
    uint32_t m_timeLiveDays = 7;

    std::string m_logConfPath;
    std::string m_suffixForLog;

    void addSuffixForLogFile(const std::string sfx);

    bool checkNewDay(std::string& lastTimeStamp);

    std::string m_fileGlobalLogName = "";
    std::string m_fileDebugLogName  = "";
    std::string m_fileErrorLogName  = "";
    void recreateLogger();

};

struct LogRecord
{
    const std::string &file;
    uint64_t line;
    const std::string &func;
    const std::string &message;
    uint32_t level; // moved to the end because of alignment
    // can be extended
};

// Name changed from installCallbackLogger to avoid collision.
//
// Callback takes LogRecord by reference to allow for extension of LogRecord.
extern "C" void installRecordLogger(const std::function<void(LogRecord &)> &callback);

#endif // LOGGERPP_H
