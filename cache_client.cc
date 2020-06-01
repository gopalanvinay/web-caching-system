#include "cache_client.hh"

Client::Client(std::string host, std::string port): host_(host), port_(port) {
}
//Client::Client(): host_("127.0.0.1"), port_("4000"),{}

void Client::connect() {
    try {
        // Make the connection on the IP address we get from a lookup
        auto const results = resolver.resolve(host_, port_);
        stream.connect(results);
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void Client::disconnect() {
    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    // not_connected happens sometimes
    if(ec && ec != beast::errc::not_connected)
        throw beast::system_error{ec};
}

bool Client::process(http::request<http::empty_body> request) {
    http::response<http::string_body> res;
    connect();
    http::write(stream, request);
    if (request.method() == http::verb::get || request.method() == http::verb::delete_ || request.method() == http::verb::post) {
        http::read(stream, buffer, res);
        disconnect();
        if (res.result() == http::status::ok) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

Client::~Client() = default;
