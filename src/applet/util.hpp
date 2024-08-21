#include <iostream>
#include <vector>

#include <boost/process.hpp>
#include <boost/process/io.hpp>
#include <boost/process/pipe.hpp>
#include <boost/process/search_path.hpp>

namespace bp = boost::process;

static inline std::vector<std::string> exec_ryzenctl(std::vector<std::string> args) {
    // print command
    //std::cout << "running\n> pkexec auto-ryzenadjctl ";
    std::cout << "running\n> auto-ryzenadjctl ";
    for (auto& arg : args)
        std::cout << "'" << arg << "' ";
    std::cout << "\n";

    // variables
    std::vector<std::string> lines = {};

    // run process and capture output
    bp::ipstream pipe_stream;
    bp::child process(/*bp::search_path("pkexec"),*/ bp::search_path("auto-ryzenadjctl"), args, (bp::std_out & bp::std_err) > pipe_stream);
    process.wait();

    if (process.exit_code() != 0) abort();

    // read output line by line 
    std::string line;
    while (pipe_stream && std::getline(pipe_stream, line) && !line.empty()) {
        lines.push_back(line);        
    }

    return lines;
}