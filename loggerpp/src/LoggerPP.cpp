#include "LoggerPP.h"

#include <chrono>
#include <unistd.h>

#include <string>
#include <chrono>
#include <vector>

#include "Utils/FileMonitor.h"
#include "Utils/files.h"
#include "Utils/time_convert.h"

// C++ language standard detection
// if the user manually specified the used c++ version this is skipped
#if !defined(HAS_CPP_20) && !defined(HAS_CPP_17) && !defined(HAS_CPP_14) && !defined(HAS_CPP_11)
#if (defined(__cplusplus) && __cplusplus >= 202002L)
#define HAS_CPP_20
#define HAS_CPP_17
#define HAS_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1) // fix for issue #464
#define HAS_CPP_17
#define HAS_CPP_14
#elif (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
#define HAS_CPP_14
#endif
// the cpp 11 flag is always specified because it is the minimal required version
#define HAS_CPP_11
#endif

#if !defined(HAS_FILESYSTEM) && !defined(HAS_EXPERIMENTAL_FILESYSTEM)
#ifdef HAS_CPP_17
#if defined(__cpp_lib_filesystem)
#define HAS_FILESYSTEM 1
#elif defined(__cpp_lib_experimental_filesystem)
#define HAS_EXPERIMENTAL_FILESYSTEM 1
#elif !defined(__has_include)
#define HAS_EXPERIMENTAL_FILESYSTEM 1
#elif __has_include(<filesystem>)
#define HAS_FILESYSTEM 1
#elif __has_include(<experimental/filesystem>)
#define HAS_EXPERIMENTAL_FILESYSTEM 1
#endif

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 10
#undef HAS_FILESYSTEM
// no filesystem support before GCC 8: https://en.cppreference.com/w/cpp/compiler_support
#if __GNUC__ < 8
#undef HAS_EXPERIMENTAL_FILESYSTEM
#endif
#endif

// no filesystem support before Clang 7: https://en.cppreference.com/w/cpp/compiler_support
#if defined(__clang_major__) && __clang_major__ < 7
#undef HAS_FILESYSTEM
#undef HAS_EXPERIMENTAL_FILESYSTEM
#endif
#endif
#endif

#ifndef HAS_EXPERIMENTAL_FILESYSTEM
#define HAS_EXPERIMENTAL_FILESYSTEM 0
#endif

#ifndef HAS_FILESYSTEM
#define HAS_FILESYSTEM 0
#endif

#if HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#elif HAS_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include "Utils/filesystem17.h"
#endif

INITIALIZE_EASYLOGGINGPP

LoggerPP::LoggerPP(const std::string &logConfPath, const std::string &suffixForLog)
{
    std::cout << "Инициализация логгера..\n";
    std::fflush(stdout);

    m_logConfPath  = logConfPath;
    m_suffixForLog = suffixForLog;

    recreateLogger();
}

static bool is_older_than( const fs::path& path, int hrs )
{
    auto now = fs::file_time_type::clock::now() ;
    using namespace std::chrono ;
    return duration_cast<hours>( now - fs::last_write_time(path) ).count() > hrs ;
}

static std::vector<fs::path> files_older_than( fs::path dir, const int32_t hrs, const std::string ext = "" )
{
    std::vector<fs::path> result ;
    for( const auto& p : fs::recursive_directory_iterator(dir) )
    {
        if( fs::is_regular_file(p) && is_older_than( p, hrs ))
        {
            std::string fileName = fs::path(p).filename();
            if(ext.empty() || ext == "*") {
                result.push_back(p) ;
            }
            else if(fileName.find(ext) != std::string::npos) {
                result.push_back(p) ;
            }
        }
    }
    return result ;
}

static std::pair<int,bool> remove_files_older_than( fs::path dir, const uint32_t days, const std::string ext = "")
{
    int cnt = 0 ;
    try
    {
        for( const auto& p : files_older_than( dir, days*24, ext ) )
        {
            if(!fs::remove(p))
            {
                LOG(ERROR) << "Ошибка удаления файла: " << p.filename() << std::endl;
            }
            ++cnt ;
        }
        return { cnt, true };
    }
    catch( const std::exception& e)
    {
        LOG(ERROR) <<"Ошибка очистки лог-файлов: " << e.what();
        return { cnt, false };
    }
}


//void CustomCrashHandler(int signal)
//{
//    LOG(ERROR) << "Всё сломалось!!";
//    el::base::debug::StackTrace();
//    /* Вызываем на помошь библиотеку */
//    el::Helpers::crashAbort(signal);
//}

void LoggerPP::run(bool *isWorking)
{
    bool reservIsWork = true;
    if(isWorking == nullptr) {
        isWorking = &reservIsWork;
    }

    FileMonitor fileMon(m_logConfPath);
    std::string newFDump = "";
    struct stat newFStat;
    static std::string lastTimeStamp = MTime::nowTimeStamp("%Y-%m-%d");

    while(*isWorking)
    {
        if(!Files::fileExists(m_fileGlobalLogName))
        {
            LOG(ERROR) << "Отсутствует файл лога: " << m_fileGlobalLogName << "!!\n";
        }

        if(!Files::fileExists(m_fileDebugLogName))
        {
            LOG(ERROR) << "Отсутствует файл лога: " << m_fileDebugLogName << "!!\n";
        }

        if(!Files::fileExists(m_fileErrorLogName))
        {
            LOG(ERROR) << "Отсутствует файл лога: " << m_fileErrorLogName << "!!\n";
        }

        bool retChangeLogConf = fileMon.checkFileChange(newFDump, newFStat);
        if(retChangeLogConf)
        {
            fileMon.rewriteLastDump(newFDump, newFStat);
            LOG(WARNING) << "Изменился файл лог-конфига!" << std::endl;
            recreateLogger();
        }

        std::string oldLastTimeStamp = lastTimeStamp;

        if(checkNewDay(lastTimeStamp))
        {
            LOG(WARNING) << "Новый день!\n";

            Files::copyFile(m_fileGlobalLogName+"." + oldLastTimeStamp, m_fileGlobalLogName);
            Files::copyFile(m_fileDebugLogName+"."  + oldLastTimeStamp, m_fileDebugLogName);
            Files::copyFile(m_fileErrorLogName+"."  + oldLastTimeStamp, m_fileErrorLogName);

            FILE *fileInfo = fopen(m_fileGlobalLogName.c_str(), "w");
            fclose(fileInfo);

            FILE *fileDebug = fopen(m_fileDebugLogName.c_str(), "w");
            fclose(fileDebug);

            FILE *fileError = fopen(m_fileErrorLogName.c_str(), "w");
            fclose(fileError);

            recreateLogger();
        }

        auto pairRes = remove_files_older_than("../logs", m_timeLiveDays, ".log.");
        if(pairRes.second && pairRes.first > 0)
        {
            LOG(WARNING) << "Лог-файлы старше "  << m_timeLiveDays
                         << " дней были удалены, число: " << pairRes.first;
        }

        if(*isWorking) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

bool LoggerPP::checkNewDay(std::string& lastTimeStamp) // Проверка Новый день
{
    auto currTimeStamp = MTime::nowTimeStamp("%Y-%m-%d");
    if(lastTimeStamp != currTimeStamp)
    {
        lastTimeStamp = currTimeStamp;
        return true;
    }
    return false;
}

void LoggerPP::addSuffixForLogFile(const std::string sfx)
{
    m_fileGlobalLogName = m_pElConf->get(el::Level::Global, el::ConfigurationType::Filename)->value();
    m_fileDebugLogName = m_pElConf->get(el::Level::Debug, el::ConfigurationType::Filename)->value();
    m_fileErrorLogName = m_pElConf->get(el::Level::Error, el::ConfigurationType::Filename)->value();

    auto pos = m_fileGlobalLogName.rfind(".log");
    if(pos > 0) {
        m_fileGlobalLogName.insert(pos, sfx);
    }

    pos = m_fileDebugLogName.rfind(".log");
    if(pos > 0) {
        m_fileDebugLogName.insert(pos, sfx);
    }

    pos = m_fileErrorLogName.rfind(".log");
    if(pos > 0) {
        m_fileErrorLogName.insert(pos, sfx);
    }

    m_pElConf->set(el::Level::Global, el::ConfigurationType::Filename, m_fileGlobalLogName);
    m_pElConf->set(el::Level::Debug, el::ConfigurationType::Filename, m_fileDebugLogName);
    m_pElConf->set(el::Level::Error, el::ConfigurationType::Filename, m_fileErrorLogName);
}

el::Configurations *LoggerPP::pElConf() const
{
    return m_pElConf;
}

uint32_t LoggerPP::timeLiveDays() const
{
    return m_timeLiveDays;
}

void LoggerPP::setTimeLiveDays(uint32_t newTimeLiveDays)
{
    m_timeLiveDays = newTimeLiveDays;
}

void LoggerPP::recreateLogger()
{
    if(m_pElConf) delete m_pElConf;
    m_pElConf = new el::Configurations(m_logConfPath);

    el::Loggers::addFlag(el::LoggingFlag::ImmediateFlush);

    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    auto maxSizeLog = m_pElConf->get(el::Level::Error, el::ConfigurationType::MaxLogFileSize)->value();
    if(maxSizeLog == "0")
    {
        LOG(WARNING) << "Не задан MAX_LOG_FILE_SIZE, будет по умолчанию 128 МБайт!";
        maxSizeLog = "128000000";
        m_pElConf->set(el::Level::Error, el::ConfigurationType::MaxLogFileSize, maxSizeLog);
    }

    addSuffixForLogFile(m_suffixForLog);

    el::Loggers::reconfigureAllLoggers(*m_pElConf);

    LOG(DEBUG) << "Логгер успешно инициализирован!";
    /* Установим свой обработчик */
    //el::Helpers::setCrashHandler(CustomCrashHandler);
}

void LoggerPP::DispatchLogger::setF_callBackFullLogLine(const std::function<void (el::base::type::string_t &&)> &newF_callBackFullLogLine)
{
    f_callBackFullLogLine = newF_callBackFullLogLine;
}

void LoggerPP::DispatchLogger::setF_callBackLogMsg(const std::function<void (const LoggerPP::LogDispatchMsg_t&)> &newF_callBackLogMsg)
{
    f_callBackLogMsg = newF_callBackLogMsg;
}

void LoggerPP::DispatchLogger::handle(const el::LogDispatchData *data) noexcept
{
    //m_data = data;
    // Dispatch using default log builder of logger
    if(f_callBackFullLogLine)
    {
        auto lgBuilder = data->logMessage()->logger()->logBuilder();
        f_callBackFullLogLine(lgBuilder->build(data->logMessage(),
                                               data->dispatchAction() == el::base::DispatchAction::NormalLog));
    }

    if(f_callBackLogMsg)
    {
        std::string levelStr = "UNKNOWN";
        auto lvl = data->logMessage()->level()        ;
        if(lvl == el::Level::Info) levelStr = "INFO";
        else if(lvl == el::Level::Debug) levelStr = "DEBUG";
        else if(lvl == el::Level::Warning) levelStr = "WARNING";
        else if(lvl == el::Level::Error) levelStr = "ERROR";
        else if(lvl == el::Level::Fatal) levelStr = "FATAL";

        f_callBackLogMsg(
            {
                levelStr,
                MTime::nowTimeStamp("%Y-%m-%d %H:%M:%S"),
                data->logMessage()->message()
            });
    }

}

class RecordLogger : public el::LogDispatchCallback
{
public:
    void setF_callBackLogMsg(const std::function<void(LogRecord &)> &newF_callBackLogMsg);

protected:
    void handle(const el::LogDispatchData *data) noexcept override;

private:
    std::function<void(LogRecord &)> f_callBackLogMsg = nullptr;
};

void RecordLogger::setF_callBackLogMsg(const std::function<void(LogRecord &)> &newF_callBackLogMsg)
{
    f_callBackLogMsg = newF_callBackLogMsg;
}

void RecordLogger::handle(const el::LogDispatchData *data) noexcept
{
    if (f_callBackLogMsg)
    {
        LogRecord record({
            data->logMessage()->file(),
            data->logMessage()->line(),
            data->logMessage()->func(),
            data->logMessage()->message(),
            el::LevelHelper::castToInt(data->logMessage()->level()),
        });

        f_callBackLogMsg(record);
    }
}

extern "C" void installRecordLogger(const std::function<void(LogRecord &)> &callback)
{
    el::Configurations defaultConf;
    defaultConf.clear();
    defaultConf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
    el::Loggers::reconfigureLogger("default", defaultConf);
    el::Helpers::installLogDispatchCallback<RecordLogger>("RecordLogger");
    auto dispatch = el::Helpers::logDispatchCallback<RecordLogger>("RecordLogger");
    dispatch->setF_callBackLogMsg(callback);
    dispatch->setEnabled(true);
}
