#ifndef CRAWLER_H
#define CRAWLER_H

#include "iniParser.h"
#include "database.h"
#include "threadPool.h"
#include <mutex>
#include <unordered_set>
#include <queue>

class Crawler
{
public:
	Crawler(const Configuration& config, Database& db);

public:
	void run();

private:
	std::string normalizeUrl(const std::string& base, const std::string& link);
	void processUrl(const std::string& url, int depth);

private:
	Configuration config;
	Database& db;
	ThreadPool pool;

	std::mutex mtx;
	std::string baseHost;
	std::condition_variable cv;
	std::unordered_set<std::string> visited;
	std::unordered_set<std::string> seen;
	std::queue<std::pair<std::string, int>> urlQueue;
};

#endif // !CRAWLER_H
