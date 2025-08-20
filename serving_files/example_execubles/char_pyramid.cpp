#include <iostream>
#include <string>


void printPattern(const std::string& str) {
    for (int i = 1; i <= str.length(); i++) {
        std::cout << str.substr(0, i) << std::endl;
    }

    for (int i = str.length() - 1; i > 0; i--) {
        std::cout << str.substr(0, i) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    printPattern(str);

    return 0;
}