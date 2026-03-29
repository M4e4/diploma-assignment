#include "indexer.h"
#include <regex>
#include <iostream>





std::string removeHtmlTags(const std::string& html)
{
	std::regex re("<.*?>");
	return std::regex_replace(html, re, " ");
}





std::string cleanText(const std::string& text)
{
    std::string result;

    for (unsigned char ch : text)
    {
        if (ch < 128)
        {
            if (std::isalnum(ch)) result += std::tolower(ch);
            else result += ' ';
        }
        else
        {
            result += ch;
        }
    }

    return result;
}





std::map<std::string, int> indexer(const std::string& text)
{
	std::string clean = cleanText(removeHtmlTags(text));
	std::istringstream iss(clean);
	std::string word;
	std::map<std::string, int> result;

	while (iss >> word)
	{
		if (word.size() >= 3 && word.size() <= 32)
		{
			result[word]++;
		}
	}

	return result;
}