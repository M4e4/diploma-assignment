#include "urlParser.h"





UrlParts parseUrl(const std::string& url)
{
    UrlParts parts;
    std::string temp = url;

    if (temp.find("http://") == 0) 
    { 
        temp = temp.substr(7); 
        parts.isHttps = false; 
    }
    else if (temp.find("https://") == 0)
    { 
        temp = temp.substr(8); 
        parts.isHttps = true; 
    }

    size_t pos = temp.find('/');

    if (pos == std::string::npos)
    {
        parts.host = temp;
        parts.target = "/";
    }
    else
    {
        parts.host = temp.substr(0, pos);
        parts.target = temp.substr(pos);
    }

    return parts;
}