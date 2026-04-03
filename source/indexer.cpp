#include "indexer.h"
#include <string>
#include <map>
#include <sstream>
#include <locale>
#include <codecvt>
#include <regex>
#include <cwctype>





std::string removeHtmlTags(const std::string& html) 
{
    std::regex re("<.*?>");
    return std::regex_replace(html, re, " ");
}





std::wstring toWideString(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.from_bytes(str);
}





std::string toUtf8String(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
}





std::wstring cleanTextWide(const std::wstring& text)
{
    std::wstring result;
    result.reserve(text.size());
    std::locale loc("");
    auto& facet = std::use_facet<std::ctype<wchar_t>>(loc);

    for (wchar_t ch : text)
    {
        if (facet.is(std::ctype_base::alnum, ch))
            result += facet.tolower(ch);
        else
            result += L' ';
    }

    return result;
}




std::map<std::string, int> indexer(const std::string& text)
{
    std::string clean = removeHtmlTags(text);
    std::wstring wideClean = toWideString(clean);
    wideClean = cleanTextWide(wideClean);

    std::wistringstream wiss(wideClean);
    std::wstring word;
    std::map<std::string, int> result;

    while (wiss >> word) 
    {
        if (word.size() >= 3 && word.size() <= 32) result[toUtf8String(word)]++;
    }

    return result;
}