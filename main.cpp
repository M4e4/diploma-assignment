#include <atomic>
#include <thread>
#include <iostream>
#include <locale>
#include "httpServer.h"
#include "database.h"
#include "crawler.h"
#include "iniParser.h"

int main()
{
    std::setlocale(LC_ALL, "");
    //SetConsoleOutputCP(CP_UTF8);
    Configuration config = parse("Configuration.ini");

    std::string connection = "host=" + config.dbHost +
        " port=" + std::to_string(config.dbPort) +
        " dbname=" + config.dbName +
        " user=" + config.dbUser +
        " password=" + config.dbPassword;

    try
    {
        Database db(connection);
        db.clear();
        db.initTables();

        Crawler crawler(config, db);

        std::thread crawlerThread([&]() { crawler.run(); });

        runSearchServer(config, db);

        if (crawlerThread.joinable())
            crawlerThread.join();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Search engine stopped." << std::endl;
    return EXIT_SUCCESS;
}