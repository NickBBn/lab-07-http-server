#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "suggester.hpp"


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------



// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<
    //class Body, class Allocator,
    class Send>
void
handle_request(
    http::request<http::string_body>&& req,
    Send&& send)
{
  // Returns a bad request response
  auto const bad_request =
      [&req](beast::string_view why)
      {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
      };

  bool json_error = false;
  std::string error_msg;
  std::string output_to_send;

  try {
    std::string received_input = suggester::parse_request(req.body());
    output_to_send = suggester::suggest(received_input);
  } catch (const std::runtime_error& e) {
    error_msg = e.what();
    json_error = true;
  }

  // Make sure we can handle the method
  if ((req.method() == http::verb::post) && (!json_error)){
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = output_to_send;
    res.prepare_payload();
    return send(std::move(res));
  }
  else if (json_error) {
    return send(bad_request(error_msg));
  } else {
    return send(bad_request("Unknown HTTP-method"));
  }
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
  std::cerr << what << ": " << ec.message() << "\n";
}

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
template<class Stream>
struct send_lambda
{
  Stream& stream_;
  bool& close_;
  beast::error_code& ec_;

  explicit
  send_lambda(
      Stream& stream,
      bool& close,
      beast::error_code& ec)
      : stream_(stream)
      , close_(close)
      , ec_(ec)
  {
  }

  template<bool isRequest, class Body, class Fields>
  void
  operator()(http::message<isRequest, Body, Fields>&& msg) const
  {
    // Determine if we should close the connection after
    close_ = msg.need_eof();

    // We need the serializer here because the serializer requires
    // a non-const file_body, and the message oriented version of
    // http::write only works with const messages.
    http::serializer<isRequest, Body, Fields> sr{msg};
    http::write(stream_, sr, ec_);
  }
};

// Handles an HTTP server connection
void
do_session(tcp::socket& socket)
{
  bool close = false;
  beast::error_code ec;

  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  // This lambda is used to send messages
  send_lambda<tcp::socket> lambda{socket, close, ec};

  for(;;)
  {
    // Read a request
    http::request<http::string_body> req;
    http::read(socket, buffer, req, ec);
    if(ec == http::error::end_of_stream)
      break;
    if(ec)
      return fail(ec, "read");

    // Send the response
    handle_request(std::move(req), lambda);
    if(ec)
      return fail(ec, "write");
    if(close)
    {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Send a TCP shutdown
  socket.shutdown(tcp::socket::shutdown_send, ec);

  // At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------



int main(int argc, char* argv[])
{
  const std::string filename("json_source.json");
  suggester::_collection = std::make_unique<nlohmann::json>(nlohmann::json());
  std::thread update(update_collection, std::ref(filename));
  update.detach();
  try
  {
    // Check command line arguments.
    if (argc != 3)
    {
      std::cerr <<
                "Usage: http-server-sync <address> <port> <doc_root>\n" <<
                "Example:\n" <<
                "    http-server-sync 0.0.0.0 8080\n";
      return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

    // The io_context is required for all I/O
    net::io_context ioc{1};

    // The acceptor receives incoming connections
    tcp::acceptor acceptor{ioc, {address, port}};
    for(;;)
    {
      // This will receive the new connection
      tcp::socket socket{ioc};

      // Block until we get a connection
      acceptor.accept(socket);

      // Launch the session, transferring ownership of the socket
      std::thread{std::bind(&do_session,std::move(socket))}.detach();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}