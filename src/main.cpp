#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "parser.hpp"
#include "scanner.hpp"

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: compiler [input.txt|-]\n";
        return 1;
    }

    // The teacher runs ./compiler from build/, hence the default ../read.txt.
    const std::string path = argc == 2 ? argv[1] : "../read.txt";
    std::ifstream file;
    std::istream* input = &std::cin;
    if (path != "-") {
        file.open(path);
        if (!file) {
            std::cerr << "Cannot open file: " << path << '\n';
            return 1;
        }
        input = &file;
    }

    try {
        auto tokens = scan_tokens(*input);
        Parser parser(std::move(tokens));
        auto root = parser.get_root();
        print_ast(*root, std::cout);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
