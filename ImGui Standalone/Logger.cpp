#include "Logger.h"

namespace MyLogger {
    void LogHRESULT(HRESULT hr, const char* message, const char* file, int line) {
        // Get the path of the DLL
        char dllPath[MAX_PATH];
        GetModuleFileNameA(nullptr, dllPath, MAX_PATH);

        // Remove the DLL filename to get the directory
        std::string directory = dllPath;
        size_t lastSlashPos = directory.find_last_of("\\/");
        if (lastSlashPos != std::string::npos) {
            directory = directory.substr(0, lastSlashPos + 1);
        }

        // Append the log file name to the directory
        std::string logFilePath = directory + "logfile.txt";

        // Open the log file
        std::ofstream logFile(logFilePath.c_str(), std::ios::app);
        if (logFile.is_open()) {
            logFile << "File: " << file << ", Line: " << line << ", Message: " << message << ", HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
            logFile.close();
        }
        else {
            std::cerr << "Error opening log file." << std::endl;
        }
    }
}

