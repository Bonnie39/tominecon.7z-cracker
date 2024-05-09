#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

const int GREEN = 10;
const int RED = 12;
const int DEFAULT_GRAY = 7;

std::string FINAL_MESSAGE = "";
int FINAL_COLOR = 0;

CRITICAL_SECTION consoleCriticalSection; // For console output synchronization
HANDLE passwordFoundEvent; // Event to signal when password is found
volatile bool passwordFound = false; // Variable to indicate whether the password is found

void setConsoleColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void finalMessage(const std::string& message, int color) {
    setConsoleColor(color);
    std::cout << std::endl << message << std::endl;
    setConsoleColor(DEFAULT_GRAY);
}

void bruteForceThread(int length, const std::string& filePath, int threadId, int totalThreads) {
    // Calculate the starting index for this thread
    int startIndex = threadId - 1;

    while (!passwordFound) { // Continue until password found
        // Generate the combination
        int combination = startIndex;
        std::string combinationString = std::to_string(combination);
        combinationString = std::string(length - combinationString.length(), '0') + combinationString;

        EnterCriticalSection(&consoleCriticalSection);
        std::cout << std::endl << "Thread " << threadId << ": Trying code: " << combinationString << std::endl;
        LeaveCriticalSection(&consoleCriticalSection);

        // Try combination using 7z command
        std::string command = "7z x -p" + combinationString + " \"" + filePath + "\" -y > nul";
        int result = system(command.c_str());

        // We found the password
        if (result == 0) {
            EnterCriticalSection(&consoleCriticalSection);
            SetEvent(passwordFoundEvent);
            FINAL_MESSAGE = "Thread " + std::to_string(threadId) + ": Password found: " + combinationString;
            FINAL_COLOR = GREEN;
            LeaveCriticalSection(&consoleCriticalSection);
            return;
        }

        // Increment to the next combination
        startIndex += totalThreads;
        if (startIndex >= 10000000000000000) break; // Break if we've exhausted all combinations
    }

    // We didn't find the password (cringe)
    EnterCriticalSection(&consoleCriticalSection);
    if (!passwordFound) {
        FINAL_MESSAGE = "Thread " + std::to_string(threadId) + ": Password not found.";
        FINAL_COLOR = RED;
    }
    LeaveCriticalSection(&consoleCriticalSection);
}

void bruteForce(int length, const std::string& filePath, int totalThreads) {
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

    finalMessage(FINAL_MESSAGE, FINAL_COLOR);
}

int main(int argc, char* argv[]) {
    InitializeCriticalSection(&consoleCriticalSection);
    passwordFoundEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Dheck for .7z path input
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

    int length = 16; // We assume this is supposed to be 16
    bruteForce(length, filePath, totalThreads);

    // Cleanup
    CloseHandle(passwordFoundEvent);
    DeleteCriticalSection(&consoleCriticalSection);

    return 0;
}
