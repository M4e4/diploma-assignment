#ifndef PARSER_HTTP_H
#define PARSER_HTTP_H

#include <vector>
#include <string>

std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);

#endif // !PARSER_HTTP_H
