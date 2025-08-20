#include <iostream>
#include <string>
#include <algorithm>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    std::reverse(str.begin(), str.end());
    std::cout << "Reversed string: " << str << std::endl;

    return 0;
}