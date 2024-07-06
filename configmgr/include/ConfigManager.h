#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "ConfigMonitor.h"
#include "ConfigValidator.h"
#include "Utils/mstrings.h"
#include <mutex>

class config_error; // Определение - в ConfigValidator

class ConfigManager
{
public:
    ConfigManager(const std::string &inConfigPath,
                  const std::string &constraintPath,
                  const std::string binaryName = "none");

    ~ConfigManager();

    static std::string getSuffixFromConfig(const std::string &inConfigPath);

    void startMonitor(bool *pIsWorking, std::function<void (void)> userCallbackUpd);
    void stopMonitor();

    JSON getConfig() const;
    JSON getPrevConfig() const;

    uint64_t delayCheckUpd() const;
    void setDelayCheckUpd(uint64_t newDelayCheckUpd);

    bool loadInputConfig();
    bool saveOutConfig();
    bool deleteOutConfig();

    std::string inConfigPath() const;
    std::string outConfigPath() const;
    std::string constraintPath() const;
    std::string suffixConfig() const;

    template<typename T>
    T getParamValue(std::string keyName) const; // Если ошибка - выбрасывается исключение

    template<typename T>
    T getPrevParamValue(std::string keyName) const; // Если ошибка - выбрасывается исключение

    template<typename T>
    void setParamValue(std::string keyName, T value); // Если ошибка - выбрасывается исключение

    bool isContainParam(std::string keyPath) const;

    std::string binaryName() const;

private:
    bool checkExistConfig(const std::string inConfigName,
                          const std::string constraintName);

    void setCurrentConfig(const JSON &newCurrentConfig);
    void setPreviousConfig(const JSON &newPreviousConfig);

private:
    std::unique_ptr<ConfigMonitor>   sp_confMonitor;
    std::unique_ptr<ConfigValidator> sp_confValidator;

    uint64_t    m_delayCheckUpdMs = 500;

    std::string m_binaryName     = "none";
    std::string m_inConfigPath   = "../configurations/input/config.json";
    std::string m_outConfigPath  = "../configurations/output/config.json";
    std::string m_constraintPath = "../constraints/constraint.json";
    std::string m_suffixConfig;

    JSON m_currentConfig;
    JSON m_previousConfig;

    std::mutex m_mutexOnConfAccess;
    std::mutex m_mutexOnConfPrevAccess;
};

template<typename T>
T ConfigManager::getParamValue(std::string keyName) const
{
    T res;
    //std::lock_guard<std::mutex> lgMtx(m_mutexOnConfAccess);
    try
    {
        res = sp_confValidator->getParamValueFromJson<T>(m_currentConfig, keyName);
    }
    catch (config_error &e) {

        std::cerr << "m_currentConfig: " << m_currentConfig << "\n";

        throw e;
    }
    catch (JSON::exception &e) {
        throw config_error("getParamValue: Ошибка JSON: "+ std::string(e.what()));
    }
    catch (...) {
        throw config_error("getParamValue: Неизвестная ошибка!");
    }

    return res;
}

template<typename T>
T ConfigManager::getPrevParamValue(std::string keyName) const
{
    T res;
    //std::lock_guard<std::mutex> lgMtx(m_mutexOnConfPrevAccess);
    try
    {
        res = sp_confValidator->getParamValueFromJson<T>(m_previousConfig, keyName);
    }
    catch (config_error &e) {
        throw e;
    }
    catch (JSON::exception &e) {
        throw config_error("getPrevParamValue: Ошибка JSON: "+ std::string(e.what()));
    }
    catch (...) {
        throw config_error("getPrevParamValue: Неизвестная ошибка!");
    }

    return res;
}

template<typename T>
void ConfigManager::setParamValue(std::string keyName, T value)
{
    std::lock_guard<std::mutex> lgMtx(m_mutexOnConfAccess);
    try
    {
        sp_confValidator->setParamValueToJson<T>(m_currentConfig, keyName, value);
    }
    catch (config_error &e) {
        throw e;
    }
    catch (JSON::exception &e) {
        throw config_error("getParamValue: Ошибка JSON: "+ std::string(e.what()));
    }
    catch (...) {
        throw config_error("getParamValue: Неизвестная ошибка!");
    }
}

//std::lock_guard<std::mutex> lgMtx(m_mutexOnConfAccess);

#endif // CONFIGMANAGER_H
