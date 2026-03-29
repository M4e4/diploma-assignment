#ifndef PARSER_INI_H
#define PARSER_INI_H

#include <map>
#include <string>

struct Configuration {
    std::string dbHost, dbName, dbUser, dbPassword;
    int dbPort;
    std::string spiderStartUrl;
    int spiderMaxDepth;
    int serverPort;
};

Configuration parse(const std::string& path);

#endif // !PARSER_INI_H
