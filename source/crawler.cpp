#include "crawler.h"
#include "httpClient.h"
#include "httpParser.h"
#include "indexer.h"
#include "urlParser.h"
#include "httpServer.h"
#include <thread>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <regex>





std::string normalizeHost(std::string host)
{
    std::transform(host.begin(), host.end(), host.begin(),
        [](unsigned char c) { return std::tolower(c); });

    if (host.find("www.") == 0) host = host.substr(4);

    return host;
}





Crawler::Crawler(const Configuration& config, Database& db)
    : config(config), db(db),
      pool(std::max(1u, std::thread::hardware_concurrency() - 1))
{
    UrlParts start = parseUrl(config.spiderStartUrl);
    baseHost = normalizeHost(start.host);

    urlQueue.push({ config.spiderStartUrl, 1 });
}





std::string Crawler::normalizeUrl(const std::string& base, const std::string& link)
{
    if (link.empty()) return "";

    if (link.find("javascript:") == 0 || 
        link.find("mailto:") == 0 ||
        link.find("tel:") == 0 || 
        link[0] == '#') return "";

    UrlParts baseParts = parseUrl(base);
    std::transform(baseParts.host.begin(), baseParts.host.end(), baseParts.host.begin(),
        [](unsigned char c) { return std::tolower(c); });

    std::string url;
    if (link.find("http://") == 0 || 
        link.find("https://") == 0) 
    {
        url = link;
    }
    else 
    {
        std::filesystem::path basePath(baseParts.target);
        std::filesystem::path fullPath = (link[0] == '/') ? std::filesystem::path(link) : basePath.parent_path() / link;
        url = (baseParts.isHttps ? "https://" : "http://") + baseParts.host + fullPath.generic_string();
    }

    auto hashPos = url.find('#');

    if (hashPos != std::string::npos) url = url.substr(0, hashPos);

    auto qpos = url.find('?');

    if (qpos != std::string::npos) url = url.substr(0, qpos);

    return url;
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

    std::string decodedHtml = htmlDecodeUtf8(html);

    std::map<std::string, int> words = indexer(decodedHtml);

    std::string decodedUrl = urlDecode(url);
    int document_id = db.saveDocument(decodedUrl, decodedHtml);
    db.saveWordStats(document_id, words);

    if (depth == config.spiderMaxDepth) return;

    std::vector<std::string> links = extractLinks(html, url);
    for (const auto& link : links)
    {
        std::string normalized = normalizeUrl(url, link);
        if (normalized.empty()) continue;

        std::string decodedNormalized = urlDecode(normalized);

        UrlParts current = parseUrl(decodedNormalized);

        if (normalizeHost(current.host) != baseHost)
        {
            std::cout << "[SKIP DOMAIN] " << decodedNormalized << std::endl;
            continue;
        }

        std::lock_guard<std::mutex> lg(mtx);

        if (!seen.count(decodedNormalized))
        {
            urlQueue.push({ decodedNormalized, depth + 1 });
            seen.insert(decodedNormalized);
            cv.notify_all();
        }
    }
}





void Crawler::run()
{
    while (true)
    {
        std::pair<std::string, int> current;

        {
            std::unique_lock<std::mutex> ul(mtx);
            cv.wait(ul, [this]() { return !urlQueue.empty() || pool.isIdle(); });

            if (urlQueue.empty() && pool.isIdle()) break;

            current = urlQueue.front();
            urlQueue.pop();
        }

        pool.push([this, current]() { processUrl(current.first, current.second); });
        std::cout << "[CRAWLER] " << current.first << " depth=" << current.second << std::endl;
    }

    pool.wait();
}