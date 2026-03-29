#include "httpServer.h"
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





void handleRequest(boost::asio::ip::tcp::socket socket, Database& db)
{
    try {
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> req;
        boost::beast::http::read(socket, buffer, req);
        boost::beast::http::response<boost::beast::http::string_body> result{ boost::beast::http::status::ok, req.version() };
        result.set(boost::beast::http::field::server, "SpiderSearch");
        result.set(boost::beast::http::field::content_type, "text/html");
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

            if (pos != std::string::npos) query = body.substr(pos + 2);

            std::replace(query.begin(), query.end(), '+', ' ');
            std::vector<std::string> results = db.search(query);
            result.body() = resultsHtml(results);
        }

        result.prepare_payload();
        boost::beast::http::write(socket, result);
    }
    catch (std::exception& e)
    {
        boost::beast::http::response<boost::beast::http::string_body> errorResult{
            boost::beast::http::status::internal_server_error, 11
        };
        errorResult.set(boost::beast::http::field::server, "SpiderSearch");
        errorResult.set(boost::beast::http::field::content_type, "text/html");
        errorResult.body() = "<html><body><h2>Internal server error</h2></body></html>";
        errorResult.prepare_payload();
        boost::beast::http::write(socket, errorResult);

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

        while (true)
        {
            boost::asio::ip::tcp::socket socket(ioc);
            boost::system::error_code ec;

            acceptor.accept(socket, ec);

            if (!ec)
            {
                std::thread(
                    [&db](boost::asio::ip::tcp::socket s)
                    {
                        handleRequest(std::move(s), db);
                    },
                    std::move(socket)
                ).detach();

                std::cout << "Request handled in new thread\n";
            }
        }

        std::cout << "Server stopped." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}