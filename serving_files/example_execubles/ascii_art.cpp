#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    std::cout << "+--" << std::string(str.length(), '-') << "--+" << std::endl;
    std::cout << "|  " << str << "  |" << std::endl;
    std::cout << "+--" << std::string(str.length(), '-') << "--+" << std::endl;

    return 0;
}