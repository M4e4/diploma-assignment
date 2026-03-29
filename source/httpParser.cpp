#include "httpParser.h"
#include <regex>





std::vector<std::string> extractLinks(const std::string& html)
{
    std::vector<std::string> links;

    std::regex re("<a[^>]*href=[\"'](.*?)[\"']");
    auto begin = std::sregex_iterator(html.begin(), html.end(), re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
        links.push_back((*it)[1].str());
    }

    return links;
}