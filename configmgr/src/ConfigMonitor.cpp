#include "ConfigMonitor.h"
#include "LoggerPP.h"

#define SLEEPMS(x) usleep(x*1000)

ConfigMonitor::ConfigMonitor(const std::string &pathToFile, const uint64_t &delayCheckUpd)
    : FileMonitor(pathToFile)
{
    m_delayCheckUpd = delayCheckUpd;
}

ConfigMonitor::~ConfigMonitor()
{
    stop();
}


void ConfigMonitor::start(bool *pIsWorking, std::function<void(const JSON &)> callbackUpd)
{
    if(pIsWorking == nullptr) {
        LOG(ERROR) << "Не задан указатель pIsWorking! Будет использован внутренний флаг!";
        pIsWorking = &m_isWorks;
    }

    //m_callbackUpdWithArg = callbackUpd;
    m_isWorks = true;

    while(*pIsWorking && m_isWorks)
    {
        checkConfigNow(callbackUpd);
        SLEEPMS(m_delayCheckUpd);
    }
}

bool ConfigMonitor::checkConfigNow(std::function<void(const JSON &)> callbackUpdWithArg)
{
    JSON        newJson;
    std::string newFileDump;
    struct stat newFileStat;

    bool changeYes = checkFileChange(newFileDump, newFileStat);

    if(changeYes) // Файл как-то изменился
    {
        // Попытка прочитать JSON-содержимое
        try
        {
            newJson = JSON::parse(newFileDump);
        }
        catch(const std::exception &e)
        {
            LOG(WARNING) << "Содержимое файла " << pathToFile()
                         << " не является корректным JSON-блоком: " << e.what();
            return false;
        }

        if(!m_lastConfigContent.empty())  // Ранее дамп сгружен в JSON
        {
            // Сравнение нового JSON-дерева с предыдущим
            if(newJson == m_lastConfigContent)
            {
                LOG(DEBUG) << "Файлы равны по JSON-содержимому";
                m_lastConfigContent = newJson;
                rewriteLastDump(newFileDump, newFileStat);
                return false;
            }
        }

        // Файл считается изменённым лишь тогда, когда содержимое JSON-деревьев различное
        // Может быть вариант, что дамп разный (строки в json переставлены местами),
        // но по факту это одинаковые JSON-ы
        LOG(INFO) << "Обнаружено обновление Json-конфига " << pathToFile();
        if(callbackUpdWithArg)
        {
            callbackUpdWithArg(newJson);
        }

        m_lastConfigContent = newJson;
        rewriteLastDump(newFileDump, newFileStat);
    }

    return changeYes;
}

JSON ConfigMonitor::getLastConfig() const
{
    return m_lastConfigContent;
}


void ConfigMonitor::stop()
{
    if(!m_isWorks) {
        return;
    }
    m_isWorks = false;
}

