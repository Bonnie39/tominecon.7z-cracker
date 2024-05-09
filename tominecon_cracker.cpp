#include <iostream>
#include <string>
#include <limits>
#include <thread>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

const int GREEN = 10;
const int RED = 12;
const int DEFAULT_GRAY = 7;

CRITICAL_SECTION consoleCriticalSection; // Critical Section for console output synchronization
HANDLE passwordFoundEvent; // Event object to signal when password is found
volatile bool passwordFound = false; // Shared variable to indicate whether the password is found

void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void bruteForceThread(int length, const std::string& startCode, const std::string& filePath, int threadId, int totalThreads) {
    std::string combination = startCode;

    while (!passwordFound) { // Continue searching until password is found by any thread
        EnterCriticalSection(&consoleCriticalSection);
        std::cout << "Thread " << threadId << ": Trying code: " << combination << std::endl;
        LeaveCriticalSection(&consoleCriticalSection);

        std::string command = "7z x -p" + combination + " \"" + filePath + "\" -y > nul";

        // Use 7z command
        int result = system(command.c_str());

        // If 7z returns 0, we found the password
        if (result == 0) {
            passwordFound = true; // Set the shared variable to indicate password found
            SetEvent(passwordFoundEvent); // Set the event to signal other threads
            EnterCriticalSection(&consoleCriticalSection);
            setConsoleColor(GREEN);
            std::cout << "Thread " << threadId << ": Password found: " << combination << std::endl;
            setConsoleColor(DEFAULT_GRAY);
            LeaveCriticalSection(&consoleCriticalSection);
            return;
        }

        // Increment combination
        int carry = 1;
        for (int i = length - 1; i >= 0; --i) {
            int digit = combination[i] - '0' + carry;
            combination[i] = (digit % 10) + '0'; // Update digit
            carry = digit / 10; // Update carry
        }

        // If carry is 1 after incrementing the last digit, we've exhausted all combinations
        if (carry == 1) {
            EnterCriticalSection(&consoleCriticalSection);
            setConsoleColor(RED);
            std::cout << "Thread " << threadId << ": Password not found." << std::endl;
            setConsoleColor(DEFAULT_GRAY);
            LeaveCriticalSection(&consoleCriticalSection);
            return;
        }
    }
}

void bruteForce(int length, const std::string& filePath, int totalThreads) {
    std::string startCode(length, '0');

    // Create threads
    std::vector<std::thread> threads;
    threads.reserve(totalThreads);
    for (int i = 0; i < totalThreads; ++i) {
        threads.emplace_back(bruteForceThread, length, startCode, filePath, i + 1, totalThreads);
    }

    // Wait for any thread to find the password or all threads to finish
    WaitForSingleObject(passwordFoundEvent, INFINITE);

    // Signal other threads to stop searching
    passwordFound = true;

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}

int main(int argc, char* argv[]) {
    // Initialize Critical Section
    InitializeCriticalSection(&consoleCriticalSection);

    // Create password found event
    passwordFoundEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // check for .7z path input
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_7z_file> [number_of_threads]" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];

    int totalThreads = 1; // default to 1 thread if not specified
    if (argc >= 3) {
        try {
            totalThreads = std::stoi(argv[2]);
            if (totalThreads < 1) {
                std::cerr << "Number of threads must be a positive integer. Input value: " << argv[2] << ". Defaulting to 1." << std::endl;
                totalThreads = 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Invalid number of threads: " << argv[2] << ". Defaulting to 1." << std::endl;
            totalThreads = 1;
        }
    }

    int length = 16; // length of the combination, assumed to be 16 from forum posts
    bruteForce(length, filePath, totalThreads);

    // Cleanup
    CloseHandle(passwordFoundEvent);
    DeleteCriticalSection(&consoleCriticalSection);

    return 0;
}
