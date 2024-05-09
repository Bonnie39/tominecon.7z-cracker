#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

const int GREEN = 10;
const int RED = 12;
const int DEFAULT_GRAY = 7;

std::string PROGRAM_FINISH = "";

CRITICAL_SECTION consoleCriticalSection; // for console output synchronization
HANDLE passwordFoundEvent; // event to signal when password is found
volatile bool passwordFound = false; // variable to indicate whether the password is found

void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void bruteForceThread(int length, const std::string& filePath, int threadId, int totalThreads) {
    // Calculate the starting index for this thread's range
    int startIndex = threadId - 1;

    while (!passwordFound) { // continue until password found
        // Generate the combination
        int combination = startIndex;
        std::string combinationString = std::to_string(combination);
        combinationString = std::string(length - combinationString.length(), '0') + combinationString;

        EnterCriticalSection(&consoleCriticalSection);
        std::cout << std::endl << "Thread " << threadId << ": Trying code: " << combinationString << std::endl;
        LeaveCriticalSection(&consoleCriticalSection);

        // Try the combination
        std::string command = "7z x -p" + combinationString + " \"" + filePath + "\" -y > nul";
        int result = system(command.c_str());

        // If password is found, set the flag and print the result
        if (result == 0) {
            EnterCriticalSection(&consoleCriticalSection);
            SetEvent(passwordFoundEvent);
            setConsoleColor(GREEN);
            PROGRAM_FINISH = "Thread " + std::to_string(threadId) + ": Password found: " + combinationString;
            std::cout << std::endl << PROGRAM_FINISH;
            setConsoleColor(DEFAULT_GRAY);
            LeaveCriticalSection(&consoleCriticalSection);
            passwordFound = true;
            return;
        }

        // Increment to the next combination
        startIndex += totalThreads;
        if (startIndex >= 10000000000000000) break; // Break if we've exhausted all combinations
    }

    // If password not found, print failure message
    EnterCriticalSection(&consoleCriticalSection);
    setConsoleColor(RED);
    PROGRAM_FINISH = "Thread " + std::to_string(threadId) + ": Password not found.";
    std::cout << std::endl << PROGRAM_FINISH;
    setConsoleColor(DEFAULT_GRAY);
    LeaveCriticalSection(&consoleCriticalSection);
}

void bruteForce(int length, const std::string& filePath, int totalThreads) {
    // Create threads
    std::vector<std::thread> threads;
    threads.reserve(totalThreads);

    // Create and start threads
    for (int i = 1; i <= totalThreads; ++i) {
        // Start each thread at its respective index
        threads.emplace_back(bruteForceThread, length, filePath, i, totalThreads);
    }

    // Wait for any thread to find the password or for all threads to finish
    WaitForSingleObject(passwordFoundEvent, INFINITE);

    // Signal other threads to stop searching
    passwordFound = true;

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
}

int main(int argc, char* argv[]) {
    InitializeCriticalSection(&consoleCriticalSection);
    passwordFoundEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Check for .7z path input
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_7z_file> [number_of_threads]" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];

    int totalThreads = 1; // Default to 1 thread if not specified
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

    int length = 16; // Length of the combination
    bruteForce(length, filePath, totalThreads);

    // Cleanup
    CloseHandle(passwordFoundEvent);
    DeleteCriticalSection(&consoleCriticalSection);

    return 0;
}
