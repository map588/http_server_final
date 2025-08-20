#include <iostream>
#include <string>
#include <cctype>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    for (int i = 0; i < str.length(); i++) {
        if (i % 2 == 0)
            str[i] = toupper(str[i]);
        else
            str[i] = tolower(str[i]);
    }

    std::cout << "Alternating case: " << str << std::endl;

    return 0;
}