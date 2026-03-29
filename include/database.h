#ifndef DB_H
#define DB_H

#include <pqxx/pqxx>
#include <mutex>

class Database
{
public:
	Database(const std::string& connection);

public:
	void initTables();
	int saveDocument(const std::string& url, const std::string& content);
	void saveWordStats(int document_id, const std::map<std::string, int> wordCount);
	std::vector<std::string> search(const std::string& query);

private:
	pqxx::connection connection;
	std::mutex mtx;
};

#endif // !DB_H
