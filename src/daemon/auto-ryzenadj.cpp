#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <csignal>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <vector>

#include <CLI/CLI.hpp>
#include <toml++/toml.hpp>
#include <boost/process.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>

#include "util.hpp"
#include "../license.hpp"

#define VERSION "1.0.0b"

using std::cout;
using std::cerr;
using std::string;
namespace bp = boost::process;
namespace ba = boost::asio;

//#define LOG cout // temporary dirty fix for the incomplete LOG class

// global vars
ThreadSafeLogger LOG;
std::atomic<bool> EXIT = false;

void clean_exit(int e) {
    EXIT = true;
    exit(e);
}

void sig(int s) {
    cerr << "recived signal " << s << "! exiting cleanly...\n";
    clean_exit(-1);
}

void ryzenadj_loop(Config& conf) {
    while (!EXIT) {
        try {
            conf.mutex.lock();
            auto exec = bp::search_path(conf.executable);
            std::vector<string> args = conf.profiles[conf.cur_profile];
            LOG << "> " << exec.string();
            for (auto arg : args) {
                LOG << " " << arg; 
            }
            LOG << "\n";

            // run process and capture output
            bp::ipstream pipe_stream;
            bp::child process(exec, args, (bp::std_out & bp::std_err) > pipe_stream);

            process.wait();

            // read output and write to LOG
            std::string line;
            while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
                LOG << line << "\n";
            }
        } catch (std::exception& err) {
            cerr << "Executing ryzenadj failed: " << err.what() << "\n";
        }
        long timer = conf.timer;
        conf.mutex.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(timer));
    }
}

int main(int argc, char** argv) {
    // ensure clean exit
    signal(SIGINT, sig);
    signal(SIGTERM, sig);

    Config conf;

    // parse arguments
    string config_path = "/etc/auto-ryzenadj.conf";
    string socket_path = "/tmp/auto-ryzenadj.socket";
    string logfile;
    bool version = false;

    CLI::App app{"Automatic ryzenadj profile loading daemon"};
    app.add_option("--config,-c", config_path, "The config file.")
        ->check(CLI::ExistingPath)
        ->required(false);
    app.add_option("--socket,-s", socket_path, "The unix socket path.")
        ->check(CLI::NonexistentPath)
        ->required(false);
    app.add_option("--logfile,-l", logfile, "The log file.")
        ->check(CLI::NonexistentPath)
        ->required(false);
    app.add_flag("--version,-v", version, "Prints version and license information.")
        ->required(false);
    CLI11_PARSE(app, argc, argv);

    if (version) {
        cout << argv[0] << " v" << VERSION << "\n"
             << LICENSE;
        return 0;
    }

    // parse config file
    toml::table config_tb;
    try {
        // parse toml 
        config_tb = toml::parse_file(config_path);
        
        // logfile
        auto logging_tb = config_tb["logging"].as_table();
        if (logging_tb->contains("file") && logfile.empty()) {
            logfile = logging_tb->get_as<string>("file")->get();
        }

        // timer
        auto main_tb = config_tb["main"].as_table();
        conf.timer = main_tb->get_as<long>("timer")->get();
        // default profile
        conf.cur_profile = main_tb->get_as<string>("default")->get();
        // executable
        if (main_tb->contains("executable"))
            conf.executable = main_tb->get_as<string>("executable")->get();
        else
            conf.executable = "ryzenadj";

        // iterate through the "profiles" table
        for (auto& profile : *config_tb["profiles"].as_table()) {
            std::vector<string> tmp_vec;
            // iterate through the array of profile
            for (auto& val : *profile.second.as_array()) {
                tmp_vec.push_back(val.as_string()->get());
            }
            conf.profiles[string(profile.first)] = tmp_vec;
        }
    }
    catch (toml::parse_error& err) {
        cerr << "Reading config failed:\n" << err << "\n";
        clean_exit(1);
    }

#ifdef DEBUG
    cout << "logfile: " << logfile << "\n";
#endif
    // init logger
    if (!logfile.empty() && logfile != "-" ) {
        // get time and date for logfile
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::ostringstream date_stream;
        std::ostringstream time_stream;
        date_stream << std::put_time(&tm, "%Y-%m-%d");
        time_stream << std::put_time(&tm, "%H-%M-%S");
        std::string date = date_stream.str();
        std::string time = time_stream.str();
        // replace %date% and %time%
        logfile = replaceAll(logfile, "%date%", date);
        logfile = replaceAll(logfile, "%time%", time);
        LOG.open(logfile);
    }

    // start ryzenadj thread
    std::thread loop_thread(ryzenadj_loop, std::ref(conf));
    LOG << "Starting ryzenadj thread\n";

    // create socket
    ::unlink(socket_path.c_str());
    ba::io_context context;
    ba::local::stream_protocol::endpoint ep(socket_path);
    ba::local::stream_protocol::acceptor acceptor(context, ep);
    while (!EXIT) {
        try {
            ba::local::stream_protocol::socket socket(context);
            // wait for connection
            acceptor.accept(socket);
            // read 2 bytes from connection
            ba::streambuf buf(2);
            ba::read(socket, buf.prepare(2));
            buf.commit(2);
            // write buffer to string using a stream
            std::istream is(&buf);
            std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
#ifdef DEBUG
            cout << "cmd:'" << data << "'\n";
#endif
            string response = "OK";
            // parse data and generate response
            if (data == "AA")  { // status
                conf.mutex.lock();
                response = "profile:" + conf.cur_profile + "\ntimer:" + std::to_string(conf.timer);
                conf.mutex.unlock();
            }
            else if (data == "AB") { // detailed profile information
                response = "";
                conf.mutex.lock();
                for(const auto& [k,v] : conf.profiles) {
                    response += k + ":" + boost::algorithm::join(v, ",") + "\n";
                }
                conf.mutex.unlock();
            }
            else if (data == "BA") { // set profile
                // read size 
                std::array<uint8_t, sizeof(uint32_t)> size_buf;
                ba::read(socket, ba::buffer(size_buf, sizeof(uint32_t)));
                uint32_t size;
                std::memcpy(&size, size_buf.data(), sizeof(uint32_t));
                size = ntohl(size);
                // read profile
                ba::streambuf profile_buf(size);
                ba::read(socket, profile_buf.prepare(size));
                profile_buf.commit(size);
                // write buffer to string using a stream
                std::istream is(&profile_buf);
                std::string data((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

#ifdef DEBUG
                cout << "profile:'" << data << "'" << "\n";
#endif

                // check if profile exists
                conf.mutex.lock();
                if (conf.profiles.find(data) != conf.profiles.end()) {
                    conf.cur_profile = data;
                }
                else {
                    response = "ERR - Profile '" + data + "' not available!";
                }
                LOG << "Changed profile to '" <<  conf.cur_profile << "'\n";
                conf.mutex.unlock();
            }
            else if (data == "BB") { // set timer
                // read timer 
                std::array<uint8_t, sizeof(uint32_t)> timer_buf;
                ba::read(socket, ba::buffer(timer_buf, 4));
                uint32_t timer;
                std::memcpy(&timer, timer_buf.data(), sizeof(uint32_t));
#ifdef DEBUG
                cout << "timer = " << timer << std::endl;
                timer = ntohl(timer);
                cout << "timer = ntohl " << timer << std::endl;
#endif
                // set timer
                conf.mutex.lock();
                conf.timer = ntohl(timer);
                LOG << "Changed timer to '" <<  conf.timer << "'\n";
                conf.mutex.unlock();
            }
            else {
                //socket.close();
                //continue;
                response = "ERR - invalid command";
            }
            // convert size to network
            uint32_t response_size = htonl(response.size());
            // copy size to array
            std::array<uint8_t, sizeof(uint32_t)> response_sizebuf;
            std::memcpy(response_sizebuf.data(), &response_size, sizeof(uint32_t));
            // send size and response
            socket.write_some(ba::buffer(response_sizebuf, sizeof(uint32_t)));
            socket.write_some(ba::buffer(response));
        }
        catch (boost::system::system_error& err) {
            cerr << "Connection error: " << err.what() << "\n";
            //clean_exit(-1);
        }
    }
}