#include <iostream>
#include <string>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

const int GREEN = 10;
const int RED = 12;
const int DEFAULT_GRAY = 7;

void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void bruteForce(int length, const std::string& filePath) {
    std::string combination(length, '0'); // initializes combo as 16 0's

    while (true) {
        std::cout << "Trying code: " << combination << std::endl;
        std::string command = "7z x -p" + combination + " \"" + filePath + "\"";

        // use 7z command
        int result = system(command.c_str());

        // if 7z returns 0, we gotem
        if (result == 0) {
            setConsoleColor(GREEN);
            std::cout << "Password found: " << combination << std::endl;
            setConsoleColor(DEFAULT_GRAY);
            break;
        }

        // increment combo
        int carry = 1;
        for (int i = length - 1; i >= 0; --i) {
            int digit = combination[i] - '0' + carry;
            combination[i] = (digit % 10) + '0'; // update digit
            carry = digit / 10; // update carry
        }

        // If carry is 1 after incrementing the last digit, we're fucked (it's not a 16 digit password with only numbers)
        if (carry == 1) {
            setConsoleColor(RED);
            std::cout << "Password not found." << std::endl;
            setConsoleColor(DEFAULT_GRAY);
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    // check for .7z path input
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_7z_file>" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];

    int length = 16; // length of the combination, assumed to be 16 from forum posts
    bruteForce(length, filePath);
    return 0;
}
