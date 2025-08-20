#include <iostream>
#include <string>
#include <vector>

void printSpiral(const std::string& str) {
    int len = str.length();
    std::vector<std::vector<char> > spiral(len, std::vector<char>(len, ' '));

    int top = 0, bottom = len - 1, left = 0, right = len - 1;
    int index = 0;

    while (top <= bottom && left <= right) {
        for (int i = left; i <= right; i++) {
            spiral[top][i] = str[index % len];
            index++;
        }
        top++;

        for (int i = top; i <= bottom; i++) {
            spiral[i][right] = str[index % len];
            index++;
        }
        right--;

        if (top <= bottom) {
            for (int i = right; i >= left; i--) {
                spiral[bottom][i] = str[index % len];
                index++;
            }
            bottom--;
        }

        if (left <= right) {
            for (int i = bottom; i >= top; i--) {
                spiral[i][left] = str[index % len];
                index++;
            }
            left++;
        }
    }

    for (const auto& row : spiral) {
        for (char c : row) {
            std::cout << c << ' ';
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
    printSpiral(str);

    return 0;
}