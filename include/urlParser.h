#ifndef URL_PARSER_H
#define URL_PARSER_H

#include <string>

struct UrlParts {
    std::string host;
    std::string target;
};

UrlParts parseUrl(const std::string& url);

#endif // !URL_PARSER_H
