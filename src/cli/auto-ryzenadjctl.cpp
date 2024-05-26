#include <iostream>
#include <csignal>

#include <CLI/CLI.hpp>

using std::cout;
using std::cerr;
using std::string;

void clean_exit(int e) {
    exit(e);
}

void sig(int s) {
    cerr << "recived signal " << s << "! exiting cleanly...\n";
    clean_exit(-1);
}

int main(int argc, char** argv) {
    // ensure clean exit
    signal(SIGINT, sig);
    signal(SIGTERM, sig);
    
    // parse arguments
    bool exit_immediatly = false;
    bool loop = false;
    string file;

    CLI::App app{"Terminal image_viewer"};
    app.add_flag("--exit-immediatly,-e", exit_immediatly, "Exits immediatly instead of waiting for keyboard input.");
    app.add_flag("--disable-loop,-l", loop, "Disables looping animated images (like GIFs).");
    app.add_option("image", file, "The image file to display.")
        ->check(CLI::ExistingFile)
        ->required(true);
    CLI11_PARSE(app, argc, argv);
}