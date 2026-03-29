#ifndef INDEXER_H
#define INDEXER_H

#include <string>
#include <map>
#include <sstream>

std::string cleanText(const std::string& text);

std::map<std::string, int> indexer(const std::string& text);

#endif // !INDEXER_H
