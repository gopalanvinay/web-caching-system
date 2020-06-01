//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, small
//
//------------------------------------------------------------------------------

#include "cache.hh"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


std::mutex cache_mtx;           // mutex for critical section


class http_connection : public std::enable_shared_from_this<http_connection> {
    public:
        http_connection(tcp::socket socket, Cache& cache) : socket_(std::move(socket)), cache(cache) {
        }
        // Initiate the asynchronous operations associated with the connection.
        void start() {
            read_request();
        }

    private:
        // The socket for the currently connected client.
        tcp::socket socket_;

        // The cache object
        Cache& cache;

        // The buffer for performing reads.
        beast::flat_buffer buffer_{8192};

        // The request message.
        http::request<http::dynamic_body> request_;

        // The response message.
        http::response<http::dynamic_body> response_;

        // Asynchronously receive a complete request message.
        void read_request() {
            auto self = shared_from_this();

            http::async_read(
                socket_,
                buffer_,
                request_,
                [self](beast::error_code ec, std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);
                    if(!ec)
                        self->process_request();
                });
        }

        // Determine what needs to be done with the request message.
        void process_request() {
            response_.version(request_.version());
            response_.keep_alive(false);
            response_.set(http::field::content_type, "text/plain");

            key_type key;
            Cache::val_type val;
            Cache::size_type size = 0;
            boost::beast::string_view target = request_.target();
            target.remove_prefix(1); // gets rid of / before target
            switch(request_.method()) {
                case http::verb::get:
                    // GET /key
                    val = cache.get((key_type) target, size);
                    if (val == nullptr) {
                        response_.result(http::status::bad_request);
                        beast::ostream(response_.body())
                            << target << " isn't in the cache\n";
                    } else {
                        response_.result(http::status::ok);
                        beast::ostream(response_.body())
                            << "{key: "<< target << ", value: " << val << " }\n";
                    }
                    break;
                case http::verb::put:
                    // PUT /key/val
                    if (target.find("/") != boost::beast::string_view::npos) {
                        response_.result(http::status::ok);
                        boost::beast::string_view key_str = target.substr(0, target.find("/"));
                        boost::beast::string_view val_str = target.substr(target.find("/")+1, target.size());
                        std::string str{val_str};
                        beast::ostream(response_.body())
                            << key_str << "=" << str << "\n";
                        cache_mtx.lock();
                        cache.set((key_type) key_str, str.data(), str.size());
                        cache_mtx.unlock();
                    } else {
                        response_.result(http::status::bad_request);
                        beast::ostream(response_.body())
                            << "usage: PUT /key/val \n";
                    }
                    break;
                case http::verb::delete_:
                    // DELETE /key
                    cache_mtx.lock();
                    if (cache.del((key_type) target)) {
                        response_.result(http::status::ok);
                        beast::ostream(response_.body())
                            << target << " deleted\n";
                    } else {
                        response_.result(http::status::bad_request);
                        beast::ostream(response_.body())
                            << target << " wasn't in the Cache\n";
                    }
                    cache_mtx.unlock();
                    break;
                case http::verb::head:
                    // HEAD
                    response_.result(http::status::ok);
                    response_.version(11);
                    beast::ostream(response_.body())
                        << "Space_used: " << cache.space_used() << "\n";
                    break;
                case http::verb::post:
                    // POST /reset
                    if (target == "reset") {
                        cache_mtx.lock();
                        cache.reset();
                        cache_mtx.unlock();
                        response_.result(http::status::ok);
                        beast::ostream(response_.body())
                            << "Cache Reset\n";
                    } else {
                        response_.result(http::status::not_found);
                        beast::ostream(response_.body())
                            << "usage: POST /reset\n";
                    }
                    break;
                default:
                    // We return responses indicating an error if
                    // we do not recognize the request method.
                    response_.result(http::status::bad_request);
                    beast::ostream(response_.body())
                        << "Invalid request-method '"
                        << std::string(request_.method_string())
                        << "'\n";
                    break;
            }
            response_.prepare_payload();
            write_response();
        }

        // Asynchronously transmit the response message.
        void write_response() {
            auto self = shared_from_this();
            response_.set(http::field::content_length, response_.body().size());
            http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
        }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket, Cache& cache) {
  acceptor.async_accept(socket, [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket), cache)->start();
          http_server(acceptor, socket, cache);
      });
}

int main(int argc, char* argv[]) {
    try {
        int opt;
        Cache::size_type maxmem = 20;
        std::string host_string = "127.0.0.1";
        std::string port_string = "4000";
        int8_t threads = 0;
        while((opt = getopt(argc, argv, "m:s:p:t:")) != -1) {
            switch(opt) {
                case 'm':
                    maxmem = atoi(optarg);
                    break;
                case 's':
                    host_string = optarg;
                    break;
                case 'p':
                    port_string = optarg;
                    break;
                case 't':
                    threads = atoi(optarg);
                    break;
            }
        }
        auto const address = net::ip::make_address(host_string.c_str());
        unsigned short port = static_cast<unsigned short>(std::atoi(port_string.c_str()));

        net::io_context ioc{threads};
        Cache cache(maxmem);
        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        std::vector<std::thread> instances;
        auto work = boost::asio::make_work_guard(ioc);
        instances.reserve(threads);
        for(auto i = 0; i <= threads; i++) {
            instances.emplace_back(
            [&socket, &acceptor, &cache]
            {
                http_server(acceptor, socket, cache);
            });
        }
        // Block until all the threads exit
        for(auto& t : instances)
            t.join();
            ioc.run();
        }
    catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << "\n" << std::endl;
        return EXIT_FAILURE;
    }
}
