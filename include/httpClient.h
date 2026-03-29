#ifndef CLIENT_H
#define CLIENT_H

#include <string>

std::string httpGET(const std::string& url, int redirectCount = 0);

#endif // !CLIENT_H