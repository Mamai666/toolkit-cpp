#include "ConfigValidator.h"
#include "LoggerPP.h"
#include "Utils/mstrings.h"
#include <regex>

ConfigValidator::ConfigValidator(const std::string &constraintPath)
{
    // Считываем всё содержимое файла констрейна
    std::string constrDump;
    if(!Files::dumpFile(constraintPath, constrDump))
    {
        LOG(ERROR) << "Не удалось взять дамп от констрейнт-файла: "+constraintPath;
        exit(-1);
    }

    // Попытка прочитать JSON-содержимое констрейнта
    try
    {
        m_constraintJS = JSON::parse(constrDump);
        m_constraintJSFlatten = m_constraintJS.flatten();
    }
    catch(const std::exception &e)
    {
        LOG(WARNING) << "Содержимое файла " << constraintPath
                     << " не является корректным JSON-блоком: " << e.what();
        exit(-1);
    }
}

bool ConfigValidator::anyParamProc(JSON &confNew, const JSON &confOld, std::string parentKey,
                                        std::string &key, const int idx, const JSON& object)
{
    const std::string realKey = key;
    if(idx >=0)
        key += ("/" + std::to_string(idx));

    std::string parentPath = parentKey.empty() ? key + "/" : parentKey;

    std::string typeParent = m_constraintJSFlatten.value("/"+parentPath+"Type", "");
    if(typeParent == "map.object" && !parentKey.empty())
    {
        LOG(WARNING) << "Подмена ключа map.object! " << key << " -> " << "OBJECT_KEY";
        key = "OBJECT_KEY";
    }
    else if(typeParent.empty() && parentPath != "ForDebug/")
        LOG(WARNING) << "У родителя " << "/"+parentPath << " нет типа!";

    LOG(DEBUG) << "/ + parentKey: " << "/"+parentKey;
    LOG(DEBUG) << "/ + parentPath: " << "/"+parentPath;
    LOG(DEBUG) << "realKey: " << realKey;
    LOG(DEBUG) << "key: " << key;

    std::string keyForConstr = key;
    std::string parentKeyForConstr = parentKey;

    if(keyForConstr.find("OBJECT_KEY") != std::string::npos)
    {
        keyForConstr = regex_replace(keyForConstr, std::regex("OBJECT_KEY/"), "");
        keyForConstr = regex_replace(keyForConstr, std::regex("OBJECT_KEY"), "");
    }

    if(parentKeyForConstr.find("OBJECT_KEY") != std::string::npos)
    {
        parentKeyForConstr = regex_replace(parentKeyForConstr, std::regex("OBJECT_KEY/"), "");
    }

    if(parentPath.find("OBJECT_KEY") != std::string::npos)
    {
        parentPath = regex_replace(parentPath, std::regex("/OBJECT_KEY/"), "");
    }

    JSON prmParent = getParamValueFromJson<JSON>(confNew, "/"+parentPath);

    bool retValid = validateForOne("/"+parentKeyForConstr+keyForConstr, object, prmParent.size());
    if(!retValid)
    {
        LOG(ERROR) << "Валидация значения (объекта) " << "/"+parentKey+realKey
            << " не пройдена! Замена объекта целиком на предыдущий..";

        if(confOld == getDefaultConfig() && (parentKey.find("OBJECT_KEY") != std::string::npos))// && (typeParent == "map.object" || typeParent == "array.object"))
        {
            LOG(WARNING) << "Предыдущий конфиг == конфигу по умолчанию! "
                         << "Замена map(array).object целиком на default!";
            setParamValueToJson<JSON>(confNew, "/"+parentPath, m_constraintJSFlatten.value("/"+parentPath+"/Default", JSON{}));
        }
        else if(!isContainParamInJson(confOld, "/"+parentKey+realKey))
        {
            const std::string errInfo = "В confOld не найден объект /"+parentKey+realKey+"!";
            if(typeParent == "map.object" || typeParent == "array.object" || (parentKey.find("OBJECT_KEY") != std::string::npos))
            {
                LOG(WARNING) << errInfo << " Попытка замены целиком родителя..!";
                JSON oldParentValue = getParamValueFromJson<JSON>(confOld, "/"+parentPath);
                setParamValueToJson<JSON>(confNew, "/"+parentPath, oldParentValue);
            }
            else
                throw config_error(errInfo);
        }
        else
        {
            JSON oldPrmValue = getParamValueFromJson<JSON>(confOld, "/"+parentKey+realKey);
            LOG(WARNING) << "oldPrmValue для замены: " << oldPrmValue << ", /+parentKey+realKey: " << "/"+parentKey+realKey;

            setParamValueToJson<JSON>(confNew, "/"+parentKey+realKey, oldPrmValue);
        }

        return false;
    }

    if(object.empty())
        return false;

    if(!key.empty())
        key += "/";

    return true;
}

void ConfigValidator::recurseJogConfig(JSON &confNew, const JSON &confOld, const std::string& parentKey, JSON jsonObj)
{
    try {

    if (jsonObj.is_object())
    {
        for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it)
        {
            if (it.value().is_object())
            {
                LOG(DEBUG) << "1) Обнаружен параметр типа object "
                             << "(Parent: "<<parentKey<<") : "
                             << it.key() << ", значение: " << it.value();

                std::string key = it.key();
                if(!anyParamProc(confNew, confOld, parentKey, key, -1, it.value()))
                    continue;

                recurseJogConfig(confNew, confOld, parentKey + key, it.value());
            }
            else if (it.value().is_array())
            {
                for (size_t i = 0; i < it.value().size(); ++i)
                {
                    if (it.value()[i].is_object())
                    {
                        LOG(DEBUG) << "2) Обнаружен параметр типа object "
                                     << "(Parent: "<<parentKey<<") : "
                                     << it.key() << ", значение: " << it.value()[i];

                        std::string key = it.key();
                        if(!anyParamProc(confNew, confOld, parentKey, key, i, it.value()[i]))
                            continue;

                        recurseJogConfig(confNew, confOld, parentKey+it.key() + "/" + std::to_string(i) + "/", it.value()[i]);
                    }
                    else
                    {
                        //std::string fullpathToParam = "/"+parentKey+it.key()+"/"+std::to_string(i);
                        //std::cout << "3) " << fullpathToParam<<": " << it.value()[i] << std::endl;

                        std::string key = it.key();
                        if(!anyParamProc(confNew, confOld, parentKey, key, i, it.value()[i]))
                            continue;
                    }
                }
            }
            else
            {
                std::string key = it.key();
                if(!anyParamProc(confNew, confOld, parentKey, key, -1, it.value()))
                    continue;
            }
        }
    }
    }
    catch(config_error &e)
    {
        throw e;
    }
}

//JSON getParamDescription(const std::string keyFlattenPath)
//{

//}

bool ConfigValidator::validateForOne(std::string prmKeyPath, JSON prmValue, size_t sizeArrMap)
{
    if(Strings::endsWith(prmKeyPath, "/"))
        Strings::chop(prmKeyPath, 1);

    if(Strings::startsWith(prmKeyPath, "/ForDebug"))
    {
        LOG(WARNING) << "Параметр " << prmKeyPath << " из секции /ForDebug, игнорим валидацию..";
        return true;
    }

    LOG(INFO) << "Проверка параметра " << prmKeyPath << ": " << prmValue;
    std::string paramType = m_constraintJSFlatten.value(prmKeyPath+"/Type", "");
    if(paramType.empty()) {
        throw config_error("Параметр "+prmKeyPath+" отсутствует в констрейне!");
    }

    if(prmValue.is_number())
    {
        if(paramType != "int" && paramType != "float")
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                          " не соответствует реальному типу (number)";
            return false;
        }

        if(prmValue.is_number_float() && paramType != "float")
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                          " не соответствует реальному типу (float)";
            return false;
        }
    }

    if(prmValue.is_string())
    {
        if(paramType != "string")
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                       << " не соответствует реальному типу (string)";
            return false;
        }

        if(isContainParamInJson(m_constraintJS, prmKeyPath+"/Enum"))
        {
            JSON enumJSArr = getParamValueFromJson<JSON>(m_constraintJS, prmKeyPath+"/Enum");
            bool retEnum = false;
            for(const auto &itE : enumJSArr)
            {
                if(!itE.is_string())
                    throw config_error("Для параметра "+prmKeyPath+" Enum в констрейне");

                LOG(DEBUG) << "itE: " << itE << " <> " << prmValue;

                if(itE == prmValue)
                {
                    retEnum = true;
                    break;
                }
            }

            if(!retEnum) {
                throw config_error("Параметр "+prmKeyPath+" не соответствует Enum в констрейне!");
            }
        }
        else {
            //LOG(DEBUG) << "m_constraintJSFlatten: " << m_constraintJS;
            LOG(DEBUG) << "Для параметра "+prmKeyPath+" как для строки нет Enum-а";
        }
    }

    if(prmValue.is_boolean())
    {
        if(paramType != "bool")
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                       <<   " не соответствует реальному типу (bool)";
            return false;
        }
    }

    if(prmValue.is_null())
    {
        if(paramType != "object")
        {
            LOG(WARNING) << "Параметр "+prmKeyPath+" имеет значение null, проверка на допустимость..";
            if(!m_constraintJSFlatten[prmKeyPath+"/Default"].is_null())
            {
                LOG(ERROR) << "Для параметра "+prmKeyPath+" недопустимо значение null!";
                return false;
            }
        }
    }

    if(prmValue.is_object())
    {
        if(paramType != "object" && paramType != "map.object")
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                       << " не соответствует реальному типу (object или map.object)";
            return false;
        }
    }

    if(prmValue.is_array())
    {
        if(!Strings::startsWith(paramType, "array."))
        {
            LOG(ERROR) << "Для параметра "+prmKeyPath+" тип в констрейне ("+paramType+")"
                       << " не соответствует реальному типу (array.*)";
            return false;
        }
    }

    if(paramType != "string" && paramType != "object" && paramType != "bool")
    {
        JSON minVariantVal = m_constraintJSFlatten.value(prmKeyPath+"/Min", -1);
        JSON maxVariantVal = m_constraintJSFlatten.value(prmKeyPath+"/Max", -1);

        JSON valForDiff = prmValue;

        if(Strings::startsWith(paramType, "array.") || Strings::startsWith(paramType, "map."))
            valForDiff = sizeArrMap;

        if(valForDiff > maxVariantVal || valForDiff < minVariantVal)
        {
            LOG(ERROR) << "Значение " << valForDiff << " не удовлетворяет Min - Max "
                       << "(Min: " << minVariantVal << ", Max: " << maxVariantVal << ")";
            return false;
        }

    }

//    if(isContainParamInJson(m_constrForErase, prmKeyPath))
//    {
//        removeElementFromJson(m_constrForErase, prmKeyPath);
//    }

    return true;
}

void ConfigValidator::checkOnRequiredKeys(const std::string &paramFullName, const JSON &paramDesc)
{
    std::string paramType = paramDesc.value("Type", "");
    if(paramType.empty())
        throw config_error("Параметр "+paramFullName+" не имеет поля Type в констрейнт!");

    if(!paramDesc.contains("Default") && paramType != "object")
        throw config_error("Параметр "+paramFullName+" не имеет поля Default в констрейнт!");

    if(paramType != "string" && paramType != "object" && paramType != "bool" && paramType != "array.bool")
    {
        if(!paramDesc.contains("Min"))
            throw config_error("Параметр "+paramFullName+" не имеет поля Min в констрейнт!");

        if(!paramDesc.contains("Max"))
            throw config_error("Параметр "+paramFullName+" не имеет поля Max в констрейнт!");

        if( (paramDesc["Default"] > paramDesc["Max"]) || (paramDesc["Default"] < paramDesc["Min"]) )
        {
            if(!(paramDesc["Default"].is_null() && !paramDesc.contains("Enum")))
                throw config_error("Параметр "+paramFullName+" имеет Default > Max или < Min в констрейнт!");
        }
    }
}

bool ConfigValidator::isContainParamInJson(const JSON &js, std::string keyName) const
{
    if(!Strings::startsWith(keyName, '/'))
        keyName = '/' + keyName;

    bool isFindParamFull = false;
    JSON findParam = js;

    auto keyPathParts = Strings::stdv_split(keyName, "/");
    for(const auto &keyOne : keyPathParts)
    {
        if(keyOne.empty())
            continue;

        if(!findParam.contains(keyOne))
        {
            isFindParamFull = false;
            break;
        }
        else
        {
            findParam = findParam[keyOne];
            isFindParamFull = true;
        }
    }

    return isFindParamFull;
}

void ConfigValidator::validateAndReplaceConfig(JSON &newConfig, const JSON &oldConfig)
{
    //LOG(DEBUG) << "Input Json: " << newConfig.dump(4) << "\n";
    //LOG(DEBUG) << "Patch--1: " << JSON::diff(newConfig, oldConfig).dump(4);
    //LOG(DEBUG) << "Patch--2: " << JSON::diff(oldConfig, newConfig).dump(4);

    //m_constrForErase = getDefaultConfig();
    checkLostParamInConfig(newConfig, m_constraintJS, "");
    recurseJogConfig(newConfig, oldConfig, "", newConfig);

    //LOG(DEBUG) << "m_constrForErase: " << m_constrForErase.dump(4);
}

JSON ConfigValidator::getDefaultConfig()
{
    if(m_defaultConfigFlatten.empty())
    {
        generateDefaultConfig(m_constraintJS);
        LOG(DEBUG) << "m_defaultConfig: " << m_defaultConfigFlatten.unflatten().dump(4) << "\n";
    }

    return m_defaultConfigFlatten.unflatten();
}

void ConfigValidator::generateDefaultConfig(JSON &jsonUnflatten, const std::string &parentKey)
{
    if (jsonUnflatten.is_object())
    {
        for (auto it = jsonUnflatten.begin(); it != jsonUnflatten.end(); ++it)
        {
            if (it.value().is_object())
            {
                checkOnRequiredKeys("/"+parentKey+it.key(), it.value());
                std::string paramType = it.value().value("Type", "");

                if(paramType != "object")
                    m_defaultConfigFlatten["/"+parentKey+it.key()] = it.value().value("Default", JSON{});

                    generateDefaultConfig(it.value(), parentKey + it.key() + "/");
            }
            else if(it.value().is_array() && it.key() != "Enum" )
                throw config_error("Описание параметра /"+parentKey+it.key()+"в констрейне не может быть массивом!");
        }
    }
}

void ConfigValidator::checkLostParamInConfig(JSON &confNew, JSON &jsonUnflatten, const std::string &parentKey)
{
    if (jsonUnflatten.is_object())
    {
        for (auto it = jsonUnflatten.begin(); it != jsonUnflatten.end(); ++it)
        {
            if (it.value().is_object())
            {
                checkOnRequiredKeys("/"+parentKey+it.key(), it.value());
                std::string paramType = it.value().value("Type", "");

                if(paramType != "map.object" && paramType != "array.object")
                {
                    if(!isContainParamInJson(confNew, "/"+parentKey+it.key()))
                        throw config_error("параметра /"+parentKey+it.key()+" отсутствует в конфиге!");
                }
                else continue;

                    //m_defaultConfigFlatten["/"+parentKey+it.key()] = it.value().value("Default", JSON{});

                checkLostParamInConfig(confNew, it.value(), parentKey + it.key() + "/");
            }
            else if(it.value().is_array() && it.key() != "Enum" )
                throw config_error("Описание параметра /"+parentKey+it.key()+"в констрейне не может быть массивом!");
        }
    }
}

void ConfigValidator::removeElementFromJson(JSON &js, std::string keyName)
{
    if (!Strings::startsWith(keyName, '/'))
        keyName = '/' + keyName;

    if (Strings::endsWith(keyName, '/'))
        Strings::chop(keyName, 1);

    std::istringstream iss(keyName);
    std::string token;
    JSON* current = &js;
    JSON* parent = nullptr;
    std::string parentKey;

    while (std::getline(iss, token, '/')) {
        if (current->is_object() && current->count(token) > 0) {
            parent = current;
            parentKey = token;
            current = &((*current)[token]);
        } else if (!token.empty()) {
            throw config_error("removeElementFromJson exception: No such parameter in JSON (" + keyName + ")");
        }
    }

    if (parent != nullptr) {
        parent->erase(parentKey);
    } else {
        throw config_error("removeElementFromJson exception: No such parameter in JSON (" + keyName + ")");
    }
}
