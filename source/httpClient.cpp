#include "httpClient.h"
#include "boost/beast.hpp"
#include "boost/asio.hpp"
#include "urlParser.h"
#include <iostream>





std::string httpGET(const std::string& url, int redirectCount)
{
    if (redirectCount > 5) return "";

    UrlParts parts = parseUrl(url);
    std::string host = parts.host;
    std::string target = parts.target;

    try {
        boost::asio::io_context ioc;
        boost::beast::net::ip::tcp::resolver resolver(ioc);
        boost::beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, "80");
        stream.connect(results);

        boost::beast::http::request<boost::beast::http::string_body> req{
            boost::beast::http::verb::get, target, 11
        };
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, "Spider");

        boost::beast::http::write(stream, req);

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        boost::beast::http::read(stream, buffer, res);

        boost::beast::error_code ec;
        stream.socket().shutdown(boost::beast::net::ip::tcp::socket::shutdown_both, ec);

        if (res.result() == boost::beast::http::status::moved_permanently ||
            res.result() == boost::beast::http::status::found)
        {
            auto loc = res[boost::beast::http::field::location];
            if (!loc.empty())
                return httpGET(std::string(loc), redirectCount + 1);
            else
                return "";
        }

        if (res.result() != boost::beast::http::status::ok)
        {
            std::cerr << "HTTP status: " << res.result_int() << std::endl;
            return "";
        }

        if (res.body().size() > 1'000'000)
        {
            std::cerr << "Page too large\n";
            return "";
        }

        return res.body();
    }
    catch (std::exception& e)
    {
        std::cerr << "HTTP error: " << e.what() << std::endl;
        return "";
    }
}