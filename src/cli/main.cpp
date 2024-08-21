#include <CLI/Validators.hpp>
#include <array>
#include <boost/asio/read.hpp>
#include <boost/asio/streambuf.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <csignal>
#include <netinet/in.h>
#include <optional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include <CLI/CLI.hpp>
#include <string>

#include "../license.hpp"

#define VERSION "1.0.0b"

using std::cout;
using std::cerr;
using std::string;
namespace ba = boost::asio;

void clean_exit(int e) {
    exit(e);
}

void sig(int s) {
    cerr << "recived signal " << s << "! exiting cleanly...\n";
    clean_exit(-1);
}

#define handle_response(socket) { \
    uint32_t err_size; \
    std::array<uint8_t, sizeof(uint32_t)> err_size_buf; \
    ba::read(socket, ba::buffer(err_size_buf, sizeof(uint32_t))); \
    std::memcpy(&err_size, err_size_buf.data(), sizeof(uint32_t)); \
    err_size = ntohl(err_size); \
    ba::streambuf err_buf(err_size); \
    ba::read(socket, err_buf.prepare(err_size)); \
    err_buf.commit(err_size); \
    std::istream is(&err_buf); \
    std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>()); \
    cerr << "Daemon returned with " + data + "\n";  \
    if (data.find("ERR") == 0) \
        exit(1); \
}

int main(int argc, char** argv) {
    // ensure clean exit
    signal(SIGINT, sig);
    signal(SIGTERM, sig);
    
    // parse arguments
    string socket_path = "/tmp/auto-ryzenadj.socket";

    string profile_name;
    std::optional<uint32_t> settimer = std::nullopt;
    string profile_info;
    bool listprofiles = false;
    bool status = false;
    bool version = false;

    CLI::App app{"auto-ryzenadj daemon control interface"};
    app.add_option("--socket,-s", socket_path, "The unix socket path.")
        ->check(CLI::ExistingPath)
        ->required(false);
    app.add_option("--setprofile", profile_name, "Set profile");
    app.add_option("--searchprofile,--getprofile", profile_info, "Search for profile until it finds one");
    app.add_option("--settimer", settimer, "Set timer")
        ->check(CLI::NonNegativeNumber)
        ->required(false);
    app.add_flag("--listprofiles", listprofiles,  "Lists all profiles")
        ->required(false);
    app.add_flag("--status", status,  "Shows daemon status")
        ->required(false);
    app.add_flag("--version,-v", version, "Prints version and license information.")
        ->required(false);
    CLI11_PARSE(app, argc, argv);

    if (version) {
        cout << argv[0] << " v" << VERSION << "\n"
             << LICENSE;
        return 0;
    }

    // create endpoint for socket
    ba::io_context context;
    ba::local::stream_protocol::endpoint ep(socket_path);
    try {
        if (!profile_name.empty()) {
            // prepare data
            auto cmd_buf = ba::buffer("BA", 2);
            auto profile_buf = ba::buffer(profile_name);

            uint32_t size = ntohl(profile_buf.size());
            std::array<uint8_t, sizeof(uint32_t)> size_arr;
            std::memcpy(size_arr.data(), &size, sizeof(uint32_t));            // create connection
            
            auto size_buf = ba::buffer(size_arr, sizeof(uint32_t));
            // create connection
            ba::local::stream_protocol::socket socket(context);
            socket.connect(ep);
            // write data
            socket.write_some(cmd_buf); // send set profile coimmand
            socket.write_some(size_buf); // send size of response
            socket.write_some(profile_buf); // send response

            handle_response(socket);
        }
        if (settimer) {
            // prepare data
            auto cmd_buf = ba::buffer("BB", 2);

            uint32_t timer = htonl(settimer.value());
            std::array<uint8_t, sizeof(uint32_t)> timer_arr;
            std::memcpy(timer_arr.data(), &timer, sizeof(uint32_t));            // create connection
            
            auto timer_buf = ba::buffer(timer_arr, sizeof(uint32_t));
            // create connection
            ba::local::stream_protocol::socket socket(context);
            socket.connect(ep);
            // write data
            socket.write_some(cmd_buf); // send set profile coimmand
            socket.write_some(timer_buf); // send response

            handle_response(socket);
        }

        if (status) {
            // prepare data
            auto cmd_buf = ba::buffer("AA", 2);
            // create connection
            ba::local::stream_protocol::socket socket(context);
            socket.connect(ep);
            // write data
            socket.write_some(cmd_buf); // send status command
            // read response
            uint32_t size;
            std::array<uint8_t, sizeof(uint32_t)> size_buf;

            // read size
            ba::read(socket, ba::buffer(size_buf, sizeof(uint32_t)));
            std::memcpy(&size, size_buf.data(), sizeof(uint32_t));
            size = ntohl(size);
            // read response
            ba::streambuf status_buf(size);
            ba::read(socket, status_buf.prepare(size));
            status_buf.commit(size);
            // handle response
            std::istream is(&status_buf);
            std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            cout << data << "\n";
        }
        if (listprofiles) {
            // prepare data
            auto cmd_buf = ba::buffer("AB", 2);
            // create connection
            ba::local::stream_protocol::socket socket(context);
            socket.connect(ep);
            // write data
            socket.write_some(cmd_buf); // send profiles command
            // read response
            uint32_t size;
            std::array<uint8_t, sizeof(uint32_t)> size_buf;

            // read size
            ba::read(socket, ba::buffer(size_buf, sizeof(uint32_t)));
            std::memcpy(&size, size_buf.data(), sizeof(uint32_t));
            size = ntohl(size);
            // read response
            ba::streambuf status_buf(size);
            ba::read(socket, status_buf.prepare(size));
            status_buf.commit(size);
            // handle response
            std::istream is(&status_buf);
            std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            cout << data << "\n";
        }
        if (!profile_info.empty()) {
            // prepare data
            auto cmd_buf = ba::buffer("AB", 2);
            // create connection
            ba::local::stream_protocol::socket socket(context);
            socket.connect(ep);
            // write data
            socket.write_some(cmd_buf); // send status command
            // read response
            uint32_t size;
            std::array<uint8_t, sizeof(uint32_t)> size_buf;

            // read size
            ba::read(socket, ba::buffer(size_buf, sizeof(uint32_t)));
            std::memcpy(&size, size_buf.data(), sizeof(uint32_t));
            size = ntohl(size);
            // read response
            ba::streambuf status_buf(size);
            ba::read(socket, status_buf.prepare(size));
            status_buf.commit(size);
            // handle response
            std::istream is(&status_buf);
            std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
            
            // go through line by line until
            std::istringstream data_stream(data);
            string line, result;
            while (std::getline(data_stream, line)) {
                if (line.find(profile_info) == 0) {
                    result = line;
                    break;
                }
            }
            // handle the result
            if (result.empty()) {
                cerr << "Profile '" << profile_info << "' not found!\n";
                return 1;
            }
            cout << result << "\n";
        }
    }
    catch (boost::system::system_error& err) {
        cerr << "Connection error: " << err.what() << "\n";
        return 1;
    }
}