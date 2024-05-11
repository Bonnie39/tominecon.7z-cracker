#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <sys/wait.h>

const int GREEN = 32;
const int RED = 31;
const int DEFAULT_GRAY = 0;

int FINAL_COLOR = 0;
std::string FINAL_MESSAGE = "";

std::mutex consoleMutex;
std::atomic<bool> passwordFound(false);
std::condition_variable passwordFoundCondition;

void setConsoleColor(int color) {
    std::cout << "\033[" << color << "m";
}

void finalMessage(const std::string &message, int color) {
    std::lock_guard<std::mutex> lock(consoleMutex);
    setConsoleColor(color);
    std::cout << std::endl << message << std::endl;
    setConsoleColor(DEFAULT_GRAY);
}

void bruteForceThread(int length, const std::string &filePath, int threadId, int totalThreads) {
    int startIndex = threadId - 1;
    while (!passwordFound.load()) {
        int combination = startIndex;

        std::string combinationString = std::to_string(combination);
        combinationString = (std::string(length - combinationString.length(), '0') + combinationString);
        //combinationString.erase(0, combinationString.find_first_not_of('0'));

        std::string command = "7z x -p" + combinationString + " \"" + filePath + "\" -y > /dev/null 2>&1";
        int result = system(command.c_str());
        int exitStatus = WEXITSTATUS(result);

        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "Combination: " << combinationString << ", Exit Status: " << exitStatus << ", Thread " << threadId << std::endl;
        }

        if (exitStatus == 0) {
            std::lock_guard<std::mutex> lock(consoleMutex);
            std::cout << "Password correctly identified by thread " << threadId << std::endl;
            std::cout << "Password: " << combinationString << std::endl;
            //FINAL_MESSAGE = "Thread " + std::to_string(threadId) + ": Password found: " + combinationString;
            //FINAL_COLOR = GREEN;
            passwordFound.store(true);
            passwordFoundCondition.notify_all();
            std::exit(0);
        }

        startIndex += totalThreads;
    }
}

void bruteForce(int length, const std::string &filePath, int totalThreads) {
    std::vector<std::thread> threads;
    threads.reserve(totalThreads);

    for (int i = 1; i <= totalThreads; ++i) {
        threads.emplace_back(bruteForceThread, length, filePath, i, totalThreads);
    }

    std::unique_lock<std::mutex> lock(consoleMutex);
    passwordFoundCondition.wait(lock, [] { return passwordFound.load(); });

    for (auto &thread : threads) {
        thread.join();
    }

    //finalMessage(FINAL_MESSAGE, FINAL_COLOR);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_7z_file> [number_of_threads]" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    int totalThreads = 1;
    if (argc >= 3) {
        try {
            totalThreads = std::stoi(argv[2]);
            if (totalThreads < 1) {
                std::cerr << "Number of threads must be a positive integer. Input value: " << argv[2] << ". Defaulting to 1." << std::endl;
                totalThreads = 1;
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid number of threads: " << argv[2] << ". Defaulting to 1." << std::endl;
            totalThreads = 1;
        }
    }

    bruteForce(16, filePath, totalThreads);

    return 0;
}
