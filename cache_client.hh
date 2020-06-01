#include <unistd.h>
#include <stdio.h>
#include <string>
#include <assert.h>
#include <stdlib.h>     /* atoi */

// Boost Libraries
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/format.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class Client {
    private:
        std::string host_, port_;
        // These objects perform our I/O
        net::io_context ioc;
        tcp::resolver resolver{ioc};
        beast::tcp_stream stream{ioc};
        beast::flat_buffer buffer;

    public:
        Client(std::string host, std::string port);
        ~Client();

        void connect();

        void disconnect();

        bool process(http::request<http::empty_body> request);

};
