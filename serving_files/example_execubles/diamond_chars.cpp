#include <iostream>
#include <string>
#include <algorithm>

void printDiamond(const std::string& str) {
    int len = str.length();
    for (int i = 1; i <= len; i++) {
        std::cout << std::string(len - i, ' ') << str.substr(0, i) << str.substr(0, i - 1) << std::endl;
    }
    for (int i = len - 1; i > 0; i--) {
        std::cout << std::string(len - i, ' ') << str.substr(0, i) << str.substr(0, i - 1) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    printDiamond(str);

    return 0;
}