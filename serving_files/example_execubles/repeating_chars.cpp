#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    std::string result;
    for (char c : str) {
        result += std::string(3, c);
    }

    std::cout << "Repeating characters: " << result << std::endl;

    return 0;
}