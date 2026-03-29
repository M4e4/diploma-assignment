#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "iniParser.h"
#include "database.h"
#include <boost/asio.hpp>

void runSearchServer(const Configuration& config, Database& db);

#endif // !HTTP_SERVER_H
