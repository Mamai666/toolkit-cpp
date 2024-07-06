#include <string>

class ConfigMan
{
    std::string m_self;
    std::string m_configRootInput;
    std::string m_configRootOutput;
    std::string m_constraintsRoot;

    std::string m_configPath;
    std::string m_constraintsPath;

    int (*m_loadCallback)(const std::string&, bool);

public:
    ConfigMan() = default;
    virtual ~ConfigMan() = default;

    bool init(int argc, char **argv, int (*callback)(const std::string &, bool));

    /*!
     * \brief Загрузить конфигурацию в первый раз (флаг update будет выключен)
     * \return код, возвращённый обратным вызовом
     */
    int load();

    /*!
     * \brief Перезагрузить конфигурацию (флаг update будет включен)
     * \return код, возвращённый обратным вызовом
     */
    int reLoad();

    std::string logPath() const;

    std::string configPath() const;
    std::string constraintsPath() const;


    std::string pathInput() const;
    std::string pathOutput() const;

    std::string pathConstraints() const;
};
