#include "httpParser.h"
#include "urlParser.h"
#include <regex>
#include <string>
#include <vector>





std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl)
{
    std::vector<std::string> links;
    std::regex re("<a[^>]*href=[\"'](.*?)[\"']", std::regex::icase);
    auto begin = std::sregex_iterator(html.begin(), html.end(), re);
    auto end = std::sregex_iterator();

    UrlParts base = parseUrl(baseUrl);

    for (auto it = begin; it != end; ++it)
    {
        std::string link = (*it)[1].str();

        if (link.empty()) continue;
        if (link.find("javascript:") == 0) continue;
        if (link.find("mailto:") == 0) continue;
        if (link.find("tel:") == 0) continue;
        if (link[0] == '#') continue;

        if (link.find("http://") != 0 && link.find("https://") != 0)
        {
            if (link[0] == '/') link = (base.isHttps ? "https://" : "http://") + base.host + link;
            else link = (base.isHttps ? "https://" : "http://") + base.host + "/" + link;
        }

        links.push_back(link);
    }

    return links;
}