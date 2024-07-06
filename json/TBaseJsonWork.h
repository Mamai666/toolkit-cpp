#ifndef TBASEJSONWORK_H
#define TBASEJSONWORK_H

#include <iostream>
#include "Utils/files.h"
#include "DirManager/dirman.h"

#include "json/json.hpp"
using JSON = nlohmann::json;

class TBaseJsonWork
{
    TBaseJsonWork(){

    }

public:
    static JSON loadJson(const std::string &filePath);
    static bool exportConfigFull(JSON outJsonObj, const std::string outJsonName); // deprecated

    static bool saveJson(const std::string &filePath, JSON &outJson);

    static JSON jsonFromString(const std::string &str);
    static JSON removeUnfamiliarField(JSON &inJs, const JSON& templateJs);

    static std::string getLoggerConfPath(const std::string &configPath);
};

inline JSON TBaseJsonWork::loadJson(const std::string &filePath)
{
    std::string buffer;
    JSON result{};

    if(!Files::dumpFile(filePath, buffer)) {
        std::cerr << "\n Error open file :" << filePath << "\n";
    }
    else {
        try {
            result = JSON::parse(buffer);
        }
        catch (JSON::exception& e) {
            std::cerr << "\n Error Json from Filesystem : " << e.what() << "\n";
        }
    }
    return result;
}

inline JSON TBaseJsonWork::jsonFromString(const std::string &str)
{
    nlohmann::json result{};

    try {
        result = nlohmann::json::parse(str);
    }
    catch (nlohmann::json::exception& e) {
        std::cerr << "\n Error Json from String : " << e.what() << "\n";
    }
    return result;
}

inline JSON TBaseJsonWork::removeUnfamiliarField(JSON &inJs, const JSON &templateJs)
{
    JSON patch = JSON::diff(inJs, templateJs);
    JSON patchForSearch = patch;
    size_t idx = 0;
    for(auto &it : patchForSearch)
    {
        if(it.value("op", "") == "replace") {
            patch.erase(idx);
            idx--;
        }
        idx++;
    }

    if(!patch.empty()) {
        std::cout << std::setw(4) << "patch removeUnfamiliarField: \n" << patch.dump(4) << "\n\n";
        return inJs.patch(patch);
    }
    return inJs;
}

//inline bool TBaseJsonWork::exportConfigFull(JSON outJsonObj, const std::string outJsonName)
//{
//    std::string out = outJsonObj.dump(4);
//    FILE *f = std::fopen((outJsonName+"~").c_str(), "w");
//    if(f)
//    {
//        std::cout << "\n Обновление выходного конфига : " << outJsonName << std::endl;
//        std::fflush(stdout);
//        std::fwrite(out.c_str(), 1, out.size(), f);
//        std::fclose(f);

//        if ( rename( (outJsonName+"~").c_str(), (outJsonName).c_str() ) == 0 )  // переименование файла
//        {
//            // std::cout << "Файл успешно переименован";
//            return true;
//        }
//        else {
//            std::cerr << "\n Ошибка переименования файла : " << outJsonName << std::endl;
//        }
//    }

//    return false;
//}

inline bool TBaseJsonWork::saveJson(const std::string &filePath, JSON &outJson)
{
    bool ret = false;
    std::string out = outJson.dump(4, ' ');
    FILE *f = std::fopen(filePath.c_str(), "w");

    if(f)
    {
        ret = std::fwrite(out.c_str(), 1, out.size(), f) == out.size() ? true : false;
        std::fclose(f);
    }
    return ret;
}

//inline std::string TBaseJsonWork::getLoggerConfPath(const std::string &configPath)
//{
//    TConfigManager confMan;
//    std::string loggerConfPath = "";
//    const std::string defaultConfName = "../configurations/logger.conf";

//    if(confMan.checkExistConfig(configPath)) {
//        JSON main = loadJson(confMan.inConfigPath());
//        if(!main.empty())
//        {
//            loggerConfPath = main.value("LoggerConfPath", "");
//        }
//    }

//    if(loggerConfPath.empty()) {
//        loggerConfPath = defaultConfName;
//        std::cerr << "Не задан путь к конфигу логгера (LoggerConfPath). "
//                    "Задан по умолчанию: " << defaultConfName << std::endl;
//    }
//    return loggerConfPath;
//}

#endif // TBASEJSONWORK_H
