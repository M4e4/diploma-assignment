#include "crawler.h"
#include "httpClient.h"
#include "httpParser.h"
#include "indexer.h"
#include <thread>
#include <iostream>





Crawler::Crawler(const Configuration& config, Database& db)
    :   config(config), db(db), pool(std::max(1u, std::thread::hardware_concurrency() - 2)) // -1(main) -1(crawler)
{
	urlQueue.push({ config.spiderStartUrl, 1});
}






std::string Crawler::normalizeUrl(const std::string& base, const std::string& link)
{
    if (link.empty()) return "";

    if (link.find("http://") == 0 || link.find("https://") == 0) return link;

    auto pos = base.find("://");

    if (pos == std::string::npos) return "";

    std::string protocol = base.substr(0, pos + 3);
    auto host_end = base.find('/', pos + 3);
    std::string host = host_end == std::string::npos ? base.substr(pos + 3) : base.substr(pos + 3, host_end - (pos + 3));
    std::string path = (host_end == std::string::npos) ? "/" : base.substr(host_end);

    if (link.front() == '/') return protocol + host + link;

    if (path.back() != '/') path += '/';

    return protocol + host + path + link;
}





void Crawler::processUrl(const std::string& url, int depth)
{
    {
        std::lock_guard<std::mutex> lg(mtx);

        if (visited.count(url)) return;

        visited.insert(url);
    }

    if (depth > config.spiderMaxDepth) return;

    std::string html = httpGET(url);



    if (html.empty()) return;

    std::map<std::string, int> words = indexer(html);

    int document_id = db.saveDocument(url, html);
    db.saveWordStats(document_id, words);

    if (depth == config.spiderMaxDepth) return;

    std::vector<std::string> links = extractLinks(html);

    for (const auto& link : links)
    {
        if (link.find("javascript:") == 0) continue;
        if (link.find("mailto:") == 0) continue;

        std::string fullUrl = normalizeUrl(url, link);

        if (!fullUrl.empty())
        {
            std::lock_guard<std::mutex> lg(mtx);

            if (!seen.count(fullUrl))
            {
                urlQueue.push({ fullUrl, depth + 1 });
                seen.insert(fullUrl);
                cv.notify_one();
            }
        }
    }
}





void Crawler::run()
{
    while (true)
    {
        std::pair<std::string, int> current;
        std::unique_lock<std::mutex> ul(mtx);
        cv.wait(ul, [this]() { return !urlQueue.empty() || pool.isIdle(); });

        if (urlQueue.empty() && pool.isIdle()) break;

        current = urlQueue.front();
        urlQueue.pop();
        ul.unlock();
        pool.push([this, current]() { processUrl(current.first, current.second); });
        std::cout << current.first << ": " << current.second << std::endl;

        std::cout << "run: " << current.first << std::endl;
    }

    pool.wait();
}