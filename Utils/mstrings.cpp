#include "mstrings.h"
#include <algorithm>
#include <functional>
//#include <cstring>
#include <cctype>
#include <locale>
//#include <strings.h>
#include <regex>

bool Strings::endsWith(const std::string& str, char what)
{
    if(str.empty())
        return false;
    return (str.back() == what);
}

bool Strings::endsWith(const std::string& str, const std::string& what)
{
    if(str.size() < what.size())
        return false;
    return (str.substr( str.size() - what.size(), what.size()).compare(what) == 0);
}


bool Strings::startsWith(const std::string &str, char what)
{
    if(str.empty())
        return false;
    return (str.front() == what);
}

bool Strings::startsWith(const std::string &str, const std::string &what)
{
    if(str.size() < what.size())
        return false;
    return (str.substr(0, what.size()).compare(what) == 0);
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](int c) {return !std::isspace(c); }
    ));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](int c) {return !std::isspace(c); }
    ).base(), s.end());
}

std::string Strings::trim(std::string str)
{
    ltrim(str);
    rtrim(str);
    return str;
}

void Strings::doTrim(std::string& str)
{
    ltrim(str);
    rtrim(str);
}

void Strings::c_split(Strings::List& out, const char* c_str, char delimiter)
{
    std::string str(c_str);
    split(out, str, delimiter);
}

Strings::List Strings::c_split(const char* c_str, char delimiter)
{
    List res;
    std::string str(c_str);
    split(res, str, delimiter);
    return res;
}

void Strings::split(Strings::List& out, const std::string& str, char delimiter)
{
    std::string::size_type beg = 0;
    std::string::size_type end = 0;
    do
    {
        end = str.find(delimiter, beg);
        if(end == std::string::npos)
            end = str.size();
        out.push_back( str.substr(beg, end-beg) );
        beg = end + 1;
    }
    while(end < str.size() - 1);
}

void Strings::split(Strings::List& out, const std::string& str, const std::string& delimiter)
{
    std::string::size_type beg = 0;
    std::string::size_type end = 0;
    do
    {
        end = str.find(delimiter, beg);
        if(end == std::string::npos)
            end = str.size();
        out.push_back( str.substr(beg, end-beg) );
        beg = end + delimiter.size();
    }
    while(end < str.size() - 1);
}

Strings::List Strings::split(const std::string& str, char delimiter)
{
    List res;
    split(res, str, delimiter);
    return res;
}

Strings::List Strings::split(const std::string& str, const std::string& delimiter)
{
    List res;
    split(res, str, delimiter);
    return res;
}

std::vector<std::string> Strings::stdv_split(const std::string &input, const std::string &regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
    first{input.begin(), input.end(), re, -1}, last;
    return {first, last};
}

bool Strings::isContainNonEmptyString(const std::vector<std::string> &vec)
{
    bool retB = vec.empty();
    retB = std::any_of(vec.begin(), vec.end(), [](const std::string& str)
                       { return !str.empty(); }
                       );
    return retB;
}

std::string Strings::toLower(std::string source)
{
    std::transform(source.begin(), source.end(), source.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return source;
}

std::string Strings::toUpper(std::string source)
{
    std::transform(source.begin(), source.end(), source.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return source;
}

void Strings::chop(std::string &str, size_t numChars) {
    if (numChars >= str.length()) {
        // Возвращаем пустую строку, если numChars больше или равно длине строки
        str = "";
    }
    str = str.substr(0, str.length() - numChars);
}

std::string Strings::join(const std::vector<std::string> &strings, const std::string &delimiter)
{
    std::string result;

    // Проход по всем элементам вектора строк
    for (size_t i = 0; i < strings.size(); ++i) {
        // Добавляем текущую строку в результат
        result += strings[i];

        // Если не последний элемент, добавляем разделитель
        if (i != strings.size() - 1) {
            result += delimiter;
        }
    }

    return result;
}
