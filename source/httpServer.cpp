#include "httpServer.h"
#include "threadPool.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>





std::string searchFormHtml()
{
    return R"(
        <html>
        <body>
            <form method="POST" action="/search">
                <input name="q" type="text"/>
                <button>Search</button>
            </form>
        </body>
        </html>
    )";
}





std::string resultsHtml(const std::vector<std::string>& urls)
{
    std::string html = "<html><body><ul>";

    if (urls.empty())
    {
        html += "<li>No results found</li>";
    } 
    else
    {
        for (auto& url : urls) html += "<li><a href=\"" + url + "\">" + url + "</a></li>";
    }

    html += "</ul></body></html>";
    return html;
}





std::string urlDecode(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (str[i] == '%' && i + 2 < str.size())
        {
            unsigned char byte = static_cast<unsigned char>(std::stoi(str.substr(i + 1, 2), nullptr, 16));
            result.push_back(static_cast<char>(byte));
            i += 2;
        }
        else if (str[i] == '+')
        {
            result.push_back(' ');
        }
        else
        {
            result.push_back(str[i]);
        }
    }

    return result;
}





std::string htmlDecodeUtf8(const std::string& input) 
{
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) 
    {
        if (input[i] == '&' && i + 1 < input.size() && input[i + 1] == '#') 
        {
            size_t semicolon = input.find(';', i);

            if (semicolon != std::string::npos) 
            {
                try 
                {
                    int code = std::stoi(input.substr(i + 2, semicolon - (i + 2)));

                    if (code <= 0x7F) result += static_cast<char>(code);

                    else if (code <= 0x7FF)
                    {
                        result += static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    else if (code <= 0xFFFF)
                    {
                        result += static_cast<char>(0xE0 | ((code >> 12) & 0x0F));
                        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    else 
                    {
                        result += static_cast<char>(0xF0 | ((code >> 18) & 0x07));
                        result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
                        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    }
                }
                catch (...) {}

                i = semicolon;
                continue;
            }
        }
        result += input[i];
    }

    return result;
}





void handleRequest(boost::asio::ip::tcp::socket socket, Database& db)
{
    try {
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;
        boost::beast::http::read(socket, buffer, req);

        if (req.target() == "/favicon.ico") return;

        boost::beast::http::response<boost::beast::http::string_body> result{ boost::beast::http::status::ok, req.version() };
        result.set(boost::beast::http::field::server, "SpiderSearch");
        result.set(boost::beast::http::field::content_type, "text/html; charset=utf-8");
        result.keep_alive(req.keep_alive());

        if (req.method() == boost::beast::http::verb::get) 
        {
            result.body() = searchFormHtml();
        }
        else if (req.method() == boost::beast::http::verb::post) 
        {
            auto body = req.body();
            auto pos = body.find("q=");
            std::string query;

            if (pos != std::string::npos) query = htmlDecodeUtf8(urlDecode(body.substr(pos + 2)));

            std::vector<std::string> results = db.search(query);
            result.body() = resultsHtml(results);

            try 
            {
                std::cout << "[SERVER] Request from " << socket.remote_endpoint() << " query=" << query << std::endl;
            }
            catch (...) 
            {
                std::cout << "[SERVER] Request from unknown endpoint query=" << query << std::endl;
            }
        }

        result.prepare_payload();
        boost::beast::http::write(socket, result);

    }
    catch (std::exception& e) 
    {
        try 
        {
            boost::beast::http::response<boost::beast::http::string_body> errorResult{
                boost::beast::http::status::internal_server_error, 11
            };
            errorResult.set(boost::beast::http::field::server, "SpiderSearch");
            errorResult.set(boost::beast::http::field::content_type, "text/html");
            errorResult.body() = "<html><body><h2>Internal server error</h2></body></html>";
            errorResult.prepare_payload();
            boost::beast::http::write(socket, errorResult);
        }
        catch (...) {}

        std::cerr << "Error handling request: " << e.what() << std::endl;
    }
}





void runSearchServer(const Configuration& config, Database& db)
{
    try
    {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::acceptor acceptor(
            ioc,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), config.serverPort)
        );

        std::cout << "Server running on port " << config.serverPort << std::endl;

        ThreadPool pool(std::max(2u, std::thread::hardware_concurrency() - 1));

        while (true)
        {
            boost::asio::ip::tcp::socket socket(ioc);
            boost::system::error_code ec;

            acceptor.accept(socket, ec);

            if (!ec)
            {
                pool.push([sock = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket)), &db]() {
                    handleRequest(std::move(*sock), db);
                    });
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}