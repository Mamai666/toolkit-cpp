#ifndef CONFIGVALIDATOR_H
#define CONFIGVALIDATOR_H

#include <string>
#include "json/TBaseJsonWork.h"
#include "Utils/mstrings.h"
#include "loggerpp/include/LoggerPP.h"

// Производный класс для генерирования собственных исключений
class config_error: public std::exception
{
public:
    config_error(const std::string& error): errorMsg{error}
    {}
    const char* what() const noexcept override
    {
        return errorMsg.c_str();
    }
private:
    std::string errorMsg; // сообщение об ошибке
};

enum ValidStatus
{
    SUCCESS = 1,
    WRONG_VALUE = 0,
    NOT_EXIST = -1
};

class ConfigValidator
{
public:
    ConfigValidator(const std::string &constraintPath);

    template<typename T>
    T getDefaultValueParam(const std::string &keyName);

    template<typename T>
    std::pair<T, T> getMinMaxValueParam(const std::string &keyName);

    ValidStatus validateParam(const std::string &keyName);
    void validateAndReplaceConfig(JSON &newConfig, const JSON &oldConfig);

    //JSON getDefaultFlattenConfig();
    JSON getDefaultConfig();

    template<typename T>
    T getParamValueFromJson(const JSON& js, std::string keyName) const; // Если ошибка - выбрасывается исключение

    template<typename T>
    void setParamValueToJson(JSON &js, std::string keyPath, T value); // Если ошибка - выбрасывается исключение

    void removeElementFromJson(JSON& js, std::string keyName);

    bool isContainParamInJson(const JSON &js, std::string keyName) const;

private:
    JSON m_constraintJS;
    JSON m_constraintJSFlatten;

    void recurseJogConfig(JSON &confNew, const JSON &confOld, const std::string &parentKey, JSON jsonObj);
    void generateDefaultConfig(JSON &jsonUnflatten, const std::string &parentKey = "");

    bool validateForOne(std::string keyPath, JSON value, size_t sizeArrMap = 0);

    void checkOnRequiredKeys(const std::string &paramFullName, const JSON &paramDesc);

    JSON m_defaultConfigFlatten;
    JSON m_defaultForErase;

    bool anyParamProc(JSON &confNew, const JSON &confOld, std::string parentKey,
                      std::string &key, const int idx, const JSON &object);
    void checkLostParamInConfig(JSON &confNew, JSON &jsonUnflatten, const std::string &parentKey);
};

// Не выносить шаблонные функции в cpp:
// реализация шаблонов должна быть рядом с определением



template<typename T>
T ConfigValidator::getParamValueFromJson(const JSON &js, std::string keyName) const
{
    if(!Strings::startsWith(keyName, '/'))
        keyName = '/' + keyName;

    if(Strings::endsWith(keyName, '/'))
        Strings::chop(keyName, 1);

    //std::cerr << "keyName: " << keyName << " JSON поиска :" << js.dump(4) << "\n";

    bool isFindParamFull = false;
    // Разбиваем путь на части
    std::istringstream iss(keyName);
    std::string token;
    const JSON *current = &js;

    // Итерируемся по частям пути
    while (std::getline(iss, token, '/'))
    {
        // Проверяем, существует ли ключ в текущем уровне json
        if (current->is_object() && current->count(token) > 0)
        {
            // Если ключ существует, переходим к следующему уровню
            current = &((*current)[token]);
            isFindParamFull = true;
        }
        else if(!token.empty())
        {
            // Если ключ не существует, выходим
            isFindParamFull = false;
            break;
        }
    }

    if(!isFindParamFull)
        throw config_error("getParamValueFromJson except: Нет такого параметра в JSON ("+keyName+")");

    return *current;
}

template<typename T>
void ConfigValidator::setParamValueToJson(JSON &js, std::string keyPath, T value)
{
    if(!Strings::startsWith(keyPath, '/')) {
        keyPath = '/' + keyPath;
    }
    bool keyPathFind = false;

    try
    {
        // Разбиваем путь на части
        std::istringstream iss(keyPath);
        std::string token;
        JSON *current = &js;

        // Итерируемся по частям пути
        while (std::getline(iss, token, '/'))
        {
            // Проверяем, существует ли ключ в текущем уровне json
            if (current->is_object() && current->count(token) > 0)
            {
                // Если ключ существует, переходим к следующему уровню
                current = &((*current)[token]);
                keyPathFind = true;
            }
            else if(!token.empty())
            {
                // Если ключ не существует, выходим
                keyPathFind = false;
                break;
            }
        }

        // Устанавливаем новое значение
        *current = value;

        LOG(DEBUG) << "setParamValueToJson: confNew after replace: " << js.dump(4);
    }
    catch (JSON::exception &err)
    {
        throw config_error(std::string("Словили JSON-исключение: ") + err.what());
    }

    if(!keyPathFind)
        throw config_error("setParamValueToJson except: Нет такого параметра в JSON ("+keyPath+")");
}

#endif // CONFIGVALIDATOR_H
