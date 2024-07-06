#include "ConfigManager.h"
#include <unistd.h>
#include <regex>

#include "json/TBaseJsonWork.h"
#include "DirManager/dirman.h"
#include "LoggerPP.h"

ConfigManager::ConfigManager(const std::string &inConfigPath,
                             const std::string &constraintPath,
                             const std::string binaryName)
{
    bool retOK = checkExistConfig(inConfigPath, constraintPath);
    if(!retOK)
    {
        LOG(ERROR) << "checkExistConfig вернула ошибку!";
        exit(-1);
    }   
    m_suffixConfig = getSuffixFromConfig(inConfigPath);
    m_binaryName   = binaryName;

    // Здесь должно быть заполнение m_previousConfig из constraint-а

    sp_confValidator = std::make_unique<ConfigValidator>(m_constraintPath);
    sp_confMonitor = std::make_unique<ConfigMonitor>(m_inConfigPath, 500);
    setPreviousConfig(sp_confValidator->getDefaultConfig());
}

ConfigManager::~ConfigManager()
{
    deleteOutConfig();
}

void ConfigManager::startMonitor(bool *pIsWorking, std::function<void ()> userCallbackUpd)
{
    //saveOutConfig();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    if(m_delayCheckUpdMs != 500)
    {
        sp_confMonitor.reset();
        sp_confMonitor = std::make_unique<ConfigMonitor>(m_inConfigPath, m_delayCheckUpdMs);
    }
    sp_confMonitor->start(pIsWorking, [ &userCallbackUpd, this, &pIsWorking](const JSON &js)->void
    {
        JSON newInJson = js;

        try {
            std::lock_guard<std::mutex> lgMtx(m_mutexOnConfPrevAccess);
            sp_confValidator->validateAndReplaceConfig(newInJson, getPrevConfig());
        }
        catch (config_error e) {
            LOG(ERROR) << "config_error при валидации конфига по констрейну: " << e.what()
                       << " Завершение работы..";
            *pIsWorking = false;
            return;
        }
        catch (JSON::exception e) {
            LOG(ERROR) << "JSON::exception при валидации конфига по констрейну: " << e.what()
                       << " Завершение работы..";
            *pIsWorking = false;
            return;
        }
        catch (std::exception e) {
            LOG(ERROR) << "std::exception при валидации конфига по констрейну: " << e.what()
                       << " Завершение работы..";
            *pIsWorking = false;
            return;
        }
        catch (...) {
            LOG(ERROR) << "Неизвестная ошибка при валидации конфига по констрейну! Выход..";
            *pIsWorking = false;
            return;
        }

        setCurrentConfig(newInJson);
        userCallbackUpd();
    });
}

void ConfigManager::stopMonitor()
{
    sp_confMonitor->stop();
}


bool ConfigManager::saveOutConfig()
{
    JSON outJson = getConfig();
    setPreviousConfig(outJson);

    bool retOK = TBaseJsonWork::saveJson(m_outConfigPath+"~", outJson);

    if ( rename( (m_outConfigPath+"~").c_str(), (m_outConfigPath).c_str() ) == 0 )  // переименование файла
    {
        LOG(DEBUG) << "Временный файл успешно переименован";
        retOK = true;
    }
    else {
        LOG(ERROR) << "Ошибка переименования временного файла : "
                   << m_outConfigPath+"~" << std::endl;
        retOK = false;
    }

    if(retOK)
    {
        // Фикс прав доступа к файлу
        retOK = chown(m_outConfigPath.c_str(), 1000, 1000) == 0 ? true : false;
        retOK = chmod(m_outConfigPath.c_str(), 0644) == 0 ? true : false;
    }

    return retOK;
}

bool ConfigManager::deleteOutConfig()
{
    if(!Files::deleteFile(m_outConfigPath))
    {
        LOG(ERROR) << "Ошибка удаления файла " << m_outConfigPath << std::endl;
        return false;
    }
    return true;
}

std::string ConfigManager::binaryName() const
{
    return m_binaryName;
}

std::string ConfigManager::inConfigPath() const
{
    return m_inConfigPath;
}

std::string ConfigManager::constraintPath() const
{
    return m_constraintPath;
}

std::string ConfigManager::suffixConfig() const
{
    return m_suffixConfig;
}

bool ConfigManager::isContainParam(std::string keyPath) const
{
    return sp_confValidator->isContainParamInJson(sp_confValidator->getDefaultConfig(), keyPath);
}

std::string ConfigManager::outConfigPath() const
{
    return m_outConfigPath;
}

bool ConfigManager::checkExistConfig(const std::string inConfigName, const std::string constraintName)
{
    bool isGlobalPath = (inConfigName.size() > 0) ? inConfigName[0] == '/' : false;
    bool isLocalPath = (inConfigName.size() > 0) ? inConfigName[0] == '.' : false;
    bool isOnlyFileName = false;

    if(!isGlobalPath && !isLocalPath) {
        isOnlyFileName = true;
    }

    if(isOnlyFileName)
    {
        m_inConfigPath   = "../configurations/input/"  + inConfigName;
        m_outConfigPath  = "../configurations/output/" + inConfigName;
        if(!constraintName.empty()) {
            m_constraintPath = "../constraints/" + constraintName;
        }
    }
    else
    {
        m_inConfigPath   = inConfigName;
        m_outConfigPath  = "../configurations/output/"+Files::basename(m_inConfigPath);
        if(!constraintName.empty()) {
            m_constraintPath = constraintName;
        }
    }

    // Проверка наличия входного конфига
    if(!Files::fileExists(m_inConfigPath))
    {
        LOG(ERROR) << "Файл configurations/input/* не найден: " << m_inConfigPath;
        return false;
    }
    else {
        DirMan dirM(m_inConfigPath.c_str());
        m_inConfigPath = dirM.absolutePath();
    }

    // Проверка наличия папки выходного конфига
    std::string outDir = Files::dirname(m_inConfigPath) + "/../output";
    if(!DirMan::exists(outDir))
    {
        LOG(WARNING) << "Каталог "+outDir+" не создан! Создание..";
        // Попытка создать папку
        if(!DirMan::mkAbsDir(outDir))
        {
            LOG(ERROR) << "Ошибка создания каталога " << outDir;
            return false;
        }

        if(chown(outDir.c_str(), 1000, 1000) != 0)
        {
            LOG(ERROR) << "Ошибка настройки доступа для " << outDir;
            return false;
        }
    }

    DirMan dirM(m_outConfigPath);
    m_outConfigPath = dirM.absolutePath();

    if(!constraintName.empty())
    {
        if(!Files::fileExists(m_constraintPath))
        {
            LOG(ERROR) << "Файл Constraint не найден: " << m_constraintPath;
            return false;
        }
        else {
            DirMan dirM(m_constraintPath.c_str());
            m_constraintPath = dirM.absolutePath();
        }
    }

    LOG(DEBUG) << "m_inConfigPath: " << m_inConfigPath;
    LOG(DEBUG) << "m_outConfigPath: " << m_outConfigPath;
    LOG(DEBUG) << "m_constraintPath: " << m_constraintPath;

    return true;
}

std::string ConfigManager::getSuffixFromConfig(const std::string &inConfigPath)
{
    std::string suffixConf = "";
    auto l = Strings::stdv_split(inConfigPath, "/");
    std::string onlyConfName = l.at(l.size()-1);

    if(!onlyConfName.empty()) {
        suffixConf = regex_replace(onlyConfName, std::regex(".json"), "");
    }

    return suffixConf;
}

JSON ConfigManager::getConfig() const
{
    //std::lock_guard<std::mutex> lgMtx(m_mutexOnConfAccess);
    return m_currentConfig;
}

JSON ConfigManager::getPrevConfig() const
{
    //std::lock_guard<std::mutex> lgMtx(m_mutexOnConfPrevAccess);
    return m_previousConfig;
}

void ConfigManager::setPreviousConfig(const JSON &newPreviousConfig)
{
    std::lock_guard<std::mutex> lgMtx(m_mutexOnConfPrevAccess);
    m_previousConfig = newPreviousConfig;
}

void ConfigManager::setCurrentConfig(const JSON &newCurrentConfig)
{
    std::lock_guard<std::mutex> lgMtx(m_mutexOnConfAccess);
    m_currentConfig = newCurrentConfig;
}

uint64_t ConfigManager::delayCheckUpd() const
{
    return m_delayCheckUpdMs;
}

void ConfigManager::setDelayCheckUpd(uint64_t newDelayCheckUpd)
{
    m_delayCheckUpdMs = newDelayCheckUpd;
}

bool ConfigManager::loadInputConfig()
{
    bool retNoError = true;
    bool ret = sp_confMonitor->checkConfigNow([this, &retNoError](const JSON &js)->void
    {
      JSON newInJson = js;

      try {
          std::lock_guard<std::mutex> lgMtx(m_mutexOnConfPrevAccess);
          sp_confValidator->validateAndReplaceConfig(newInJson, getPrevConfig());
      }
      catch (config_error e) {
          LOG(ERROR) << "config_error при валидации конфига по констрейну: " << e.what()
                     << " Завершение работы..";
          retNoError = false;
      }
      catch (JSON::exception e) {
          LOG(ERROR) << "JSON::exception при валидации конфига по констрейну: " << e.what()
                     << " Завершение работы..";
          retNoError = false;
      }
      catch (std::exception e) {
          LOG(ERROR) << "std::exception при валидации конфига по констрейну: " << e.what()
                     << " Завершение работы..";
          retNoError = false;
      }
      catch (...) {
          LOG(ERROR) << "Неизвестная ошибка при валидации конфига по констрейну! Выход..";
          retNoError = false;
      }

      if(retNoError)
      {
          setCurrentConfig(newInJson);
          saveOutConfig();
      }
      //userCallbackUpd();
    });
    return ret*retNoError;
}
