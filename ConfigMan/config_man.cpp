#include <cstring>
#include "config_man.h"
#include <Utils/files.h>
#include <DirManager/dirman.h>


bool ConfigMan::init(int argc, char **argv, int (*callback)(const std::string &, bool))
{
    const char* arg0 = argv[0];
    m_loadCallback = callback;
    m_self = Files::dirname(arg0);
    m_configRootInput = DirMan(m_self + "/../configurations/input").absolutePath();
    m_configRootOutput = DirMan(m_self + "/../configurations/output").absolutePath();
    m_constraintsRoot = DirMan(m_self + "/../constraints").absolutePath();

    m_configPath = m_configRootInput + "/configuration-1-1.json";

    // Обратная совместимость с флажком -i
    if(argc >= 3 && !std::strcmp(argv[1], "-i"))
    {
        argv++;
        argc--;
    }

    if(argc >= 2)
    {
        // Если абсолютный путь
        if(Files::isAbsolute(argv[1]))
            m_configPath = std::string(argv[1]);
        // Если относительный путь, но к существующему файлу
        else if(Files::fileExists(std::string(argv[1])))
            m_configPath = std::string(argv[1]);
        else // Имя файла без пути
            m_configPath = m_configRootInput + "/" + std::string(argv[1]);
    }

    if(argc >= 3)
    {
        // Если абсолютный путь
        if(Files::isAbsolute(argv[2]))
            m_constraintsPath = std::string(argv[2]);
        // Если относительный путь, но к существующему файлу
        else if(Files::fileExists(std::string(argv[2])))
            m_constraintsPath = std::string(argv[2]);
        else // Имя файла без пути
            m_constraintsPath = m_constraintsRoot + "/" + std::string(argv[2]);
    }
    else // Путь по умолчанию
        m_constraintsPath = m_constraintsRoot + "/constraints.json";

    if(!Files::fileExists(m_configPath))
    {
        fprintf(stderr,
                "\n"
                "Файл конфигурации не найден: %s"
                "\n\n"
                "Синтаксис:"
                "\n\n"
                "%s [<имя конфига без пути или абсолютный путь к файлу конфига>] [любые другие аргументы-заглушки]"
                "\n\n",
                m_configPath.c_str(),
                arg0);
        fflush(stderr);
        return false;
    }

    return true;
}

int ConfigMan::load()
{
    return m_loadCallback(m_configPath, false);
}

int ConfigMan::reLoad()
{
    return m_loadCallback(m_configPath, true);
}

std::string ConfigMan::logPath() const
{
    return DirMan(m_self + "/../log").absolutePath();
}

std::string ConfigMan::configPath() const
{
    return m_configPath;
}

std::string ConfigMan::constraintsPath() const
{
    return m_constraintsPath;
}

std::string ConfigMan::pathInput() const
{
    return m_configRootInput;
}

std::string ConfigMan::pathOutput() const
{
    return m_configRootOutput;
}

std::string ConfigMan::pathConstraints() const
{
    return m_constraintsRoot;
}
