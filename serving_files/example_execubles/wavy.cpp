#include <iostream>
#include <string>

void printWave(const std::string& str) {
    int len = str.length();
    for (int i = 0; i < len; i++) {
        for (int j = 0; j < len; j++) {
            if (i == j || i + j == len - 1) {
                std::cout << str[j];
            } else {
                std::cout << ' ';
            }
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    printWave(str);

    return 0;
}