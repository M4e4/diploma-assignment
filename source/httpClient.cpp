#include "httpClient.h"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/ssl.hpp"
#include "boost/asio/ssl.hpp"
#include "boost/asio/connect.hpp"
#include "urlParser.h"
#include <iostream>





std::string normalizeRedirect(const std::string& baseUrl, const std::string& location)
{
    if (location.empty()) return "";

    if (location.find("http://") == 0 || location.find("https://") == 0) return location;

    UrlParts base = parseUrl(baseUrl);

    if (location.find("//") == 0)  return (base.isHttps ? "https:" : "http:") + location;

    if (location[0] == '/') return (base.isHttps ? "https://" : "http://") + base.host + location;

    std::string path = base.target;

    auto pos = path.rfind('/');
    if (pos != std::string::npos)
        path = path.substr(0, pos + 1);
    else
        path = "/";

    return (base.isHttps ? "https://" : "http://") + base.host + path + location;
}





std::string httpGET(const std::string& url, int redirectCount)
{
    if (redirectCount > 5) return "";

    UrlParts parts = parseUrl(url);
    std::string host = parts.host;
    std::string target = parts.target;
    bool useHttps = parts.isHttps;

    try {
        boost::asio::io_context ioc;
        std::unique_ptr<boost::beast::tcp_stream> tcpStream;
        std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> sslStream;

        if (useHttps) {
            boost::asio::ssl::context ctx(boost::asio::ssl::context::tls_client);
            ctx.set_verify_mode(boost::asio::ssl::verify_none);

            sslStream = std::make_unique<boost::beast::ssl_stream<boost::beast::tcp_stream>>(ioc, ctx);
            boost::asio::ip::tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, "443");
            boost::beast::get_lowest_layer(*sslStream).connect(results);

            if (!SSL_set_tlsext_host_name(sslStream->native_handle(), host.c_str()))
            {
                throw std::runtime_error("Failed to set SNI");
            }

            sslStream->handshake(boost::asio::ssl::stream_base::client);
        }
        else {
            tcpStream = std::make_unique<boost::beast::tcp_stream>(ioc);
            boost::asio::ip::tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, "80");
            tcpStream->connect(results);
        }

        boost::beast::http::request<boost::beast::http::string_body> req{
            boost::beast::http::verb::get, target, 11
        };
        req.set(boost::beast::http::field::host, host);
        req.set(boost::beast::http::field::user_agent, "Spider");

        if (useHttps) boost::beast::http::write(*sslStream, req);
        else boost::beast::http::write(*tcpStream, req);

        boost::beast::flat_buffer buffer;
        boost::beast::http::response<boost::beast::http::string_body> res;
        if (useHttps) boost::beast::http::read(*sslStream, buffer, res);
        else boost::beast::http::read(*tcpStream, buffer, res);

        if (res.result_int() >= 300 && res.result_int() < 400)
        {
            auto loc = res[boost::beast::http::field::location];
            if (!loc.empty())
            {
                std::string newUrl = normalizeRedirect(url, loc);
                return httpGET(newUrl, redirectCount + 1);
            }
        }

        boost::beast::error_code ec;
        if (useHttps) sslStream->shutdown(ec);
        else tcpStream->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (res.result() != boost::beast::http::status::ok)
            return "";

        if (res.body().size() > 1'000'000) return "";

        return res.body();
    }
    catch (std::exception& e)
    {
        std::cerr << "HTTP/HTTPS error: " << e.what() << std::endl;
        return "";
    }
}