#include <iostream>
#include <string>
#include <vector>
#include <string>

void printSpiral(const std::string& str) {
    int len = str.length();
    std::string out = "";
    const char *input = (char *)malloc(len+1);
    input = str.c_str();

    for(int i = 0; i < len+1; i++){

        for(int j = i; j < len + i; j++){
           out += input[j%len];
        }

        out += '\n';
    }

    std::cout << out << std::endl;
}


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Please provide a string argument." << std::endl;
        return 1;
    }

    std::string str = argv[1];
    printSpiral(str);

    return 0;
}
