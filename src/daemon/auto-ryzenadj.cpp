#include <chrono>
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
#include <boost/asio/io_service.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read_until.hpp>

#include "util.hpp"

using std::cout;
using std::cerr;
using std::string;
namespace bp = boost::process;
namespace ba = boost::asio;

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
            bp::child process(exec, args, bp::std_out > stdout, bp::std_err > stdout);

            process.wait();
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
    CLI11_PARSE(app, argc, argv);


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

    // main loop
    std::thread loop_thread(ryzenadj_loop, std::ref(conf)); // start thread
    while (!EXIT) {
        try {
            // create connection
            ::unlink(socket_path.c_str());
            ba::io_service service;
            ba::local::stream_protocol::endpoint ep(socket_path);
            ba::local::stream_protocol::acceptor acceptor(service, ep);
            ba::local::stream_protocol::socket socket(service);
            // wait for connection
            acceptor.accept(socket);
            ba::streambuf buf(2);
            ba::read_until(socket, buf, "\n");
            cout << buf.data().data() << "\n";
        }
        catch (boost::system::system_error& err) {
            cerr << "Connection error: " << err.what() << "\n";
            clean_exit(-1);
        }
    }
}