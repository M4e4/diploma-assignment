#include "database.h"
#include "indexer.h"





Database::Database(const std::string& connection)
	:	connection(connection)
{
	if (!this->connection.is_open())
	{
		throw std::runtime_error("Failed to connect to the database!");
	}
}





void Database::initTables()
{
	pqxx::work transaction(connection);

	transaction.exec(R"(
		CREATE TABLE IF NOT EXISTS Document (
			id SERIAL PRIMARY KEY,
			url TEXT UNIQUE,
			content TEXT
		)
	)");

	transaction.exec(R"(
		CREATE TABLE IF NOT EXISTS Word (
			id SERIAL PRIMARY KEY,
			word TEXT UNIQUE
		)
	)");

	transaction.exec(R"(
		CREATE TABLE IF NOT EXISTS WordCount (
			document_id INT REFERENCES Document(id),
			word_id INT REFERENCES Word(id),
			count INT,
			PRIMARY KEY (document_id, word_id)
		)
	)");

	transaction.commit();
}





int Database::saveDocument(const std::string& url, const std::string& content)
{
	std::lock_guard<std::mutex> lg(mtx);
	pqxx::work transaction(connection);
	pqxx::result result = transaction.exec(
		"INSERT INTO Document (url, content) VALUES (" +
		transaction.quote(url) + ", " + transaction.quote(content) + ") "
		"ON CONFLICT (url) DO UPDATE SET content = EXCLUDED.content "
		"RETURNING id"
	);

	transaction.commit();

	return result[0][0].as<int>();
}





void Database::saveWordStats(int document_id, const std::map<std::string, int> wordCount)
{
	std::lock_guard<std::mutex> lg(mtx);
	pqxx::work transaction(connection);

	for (auto [word, count] : wordCount)
	{
		transaction.exec("INSERT INTO Word (word) VALUES (" + transaction.quote(word) + ") ON CONFLICT DO NOTHING");

		pqxx::result result = transaction.exec("SELECT id FROM Word WHERE word=" + transaction.quote(word));
		int word_id = result[0][0].as<int>();

		transaction.exec("INSERT INTO WordCount (document_id, word_id, count) VALUES (" +
			std::to_string(document_id) + ", " +
			std::to_string(word_id) + ", " +
			std::to_string(count) + ") " +
			"ON CONFLICT (document_id, word_id) DO UPDATE SET count = EXCLUDED.count"
		);
	}

	transaction.commit();
}





std::vector<std::string> Database::search(const std::string& query)
{
	std::lock_guard<std::mutex> lg(mtx);
	pqxx::work transaction(connection);
	std::map<std::string, int> wordsMap = indexer(query);
	std::vector<std::string> words;

	for (auto& [word, count] : wordsMap) words.push_back(word);

	if (words.empty()) return {};

	if (words.size() > 4) words.resize(4);

	std::string inClause;

	for (size_t i = 0; i < words.size(); ++i)
	{
		inClause += transaction.quote(words[i]);
		if (i + 1 < words.size()) inClause += ",";
	}

	std::string sql =
		"SELECT d.url, SUM(wc.count) AS relevance "
		"FROM Document d "
		"JOIN WordCount wc ON d.id = wc.document_id "
		"JOIN Word w ON w.id = wc.word_id "
		"WHERE w.word IN (" + inClause + ") "
		"GROUP BY d.id "
		"HAVING COUNT(DISTINCT w.word) = " + std::to_string(words.size()) + " "
		"ORDER BY relevance DESC "
		"LIMIT 10";

	pqxx::result result = transaction.exec(sql);
	std::vector<std::string> urls;

	for (auto row : result)
	{
		urls.push_back(row["url"].c_str());
	}

	return urls;
}