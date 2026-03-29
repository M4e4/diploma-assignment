#include "iniParser.h"
#include <fstream>
#include <sstream>





std::string trim(const std::string& str)
{
    size_t start = str.find_first_not_of(" \n\t\r");
    size_t end = str.find_last_not_of(" \n\t\r");
    return start == std::string::npos ? "" : str.substr(start, end - start + 1);
}





Configuration parse(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open()) return {};

    Configuration config;
    std::string line;
    std::string section;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') 
        {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        size_t pos = line.find('=');

        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key = trim(key);
        value = trim(value);

        if (section == "database") 
        {
            if (key == "host") config.dbHost = value;
            else if (key == "port") config.dbPort = std::stoi(value);
            else if (key == "name") config.dbName = value;
            else if (key == "user") config.dbUser = value;
            else if (key == "password") config.dbPassword = value;
        }
        else if (section == "spider") 
        {
            if (key == "start_url") config.spiderStartUrl = value;
            else if (key == "max_depth") config.spiderMaxDepth = std::stoi(value);
        }
        else if (section == "server") 
        {
            if (key == "port") config.serverPort = std::stoi(value);
        }
    }

    return config;
}