#include <iostream>
#include <fstream>
#include <vector>
#include <optional>
#include <regex>
#include <cassert>
#include <thread>
#include <iomanip>

#include <common/stream.hpp>

#include <windows.h>

std::regex resultsRegex("^(//.*)|(\"([^\"]+)\"|([^,]+))(,(\"([^\"]+)\"|(.*)))?$");
std::vector<std::optional<std::pair<std::string, std::optional<std::string>>>> results;

#ifdef _DEBUG
#define CONFIG "Debug"
#else
#define CONFIG "Release"
#endif


std::string getTaskOutput(int taskNumber) {
  SECURITY_ATTRIBUTES pipeSecurityAttributes = {
    .nLength = sizeof(SECURITY_ATTRIBUTES),
    .bInheritHandle = TRUE
  };
  HANDLE readPipe, writePipe;
  BOOL success = CreatePipe(
    &readPipe, // hReadPipe
    &writePipe, // hWritePipe
    &pipeSecurityAttributes, // lpPipeAttributes
    0); // nSize (0 means default)
  assert(success);

  std::string output;

  auto readingThreadFunc = [readPipe, &output]() {
    char buf[256];
    for (;;) {
      DWORD bytesRead;
      BOOL ok = ReadFile(readPipe, buf, sizeof(buf), &bytesRead, NULL);
      if (ok) {
        output.append(buf, bytesRead);
      } else {
        return;
      }
    }
  };
  std::thread readingThread{ readingThreadFunc };

  STARTUPINFOA startupInfo = {
      .cb = sizeof(STARTUPINFOA),
      .dwFlags = STARTF_USESTDHANDLES,
      .hStdInput = INVALID_HANDLE_VALUE,
      .hStdOutput = writePipe,
      .hStdError = writePipe
  };
  PROCESS_INFORMATION processInfo = {};

  std::string taskId = std::format("{:02d}", taskNumber);
  auto exePath = "..\\x64\\" CONFIG "\\" + taskId + ".exe";
  auto workDir = "..\\" + taskId + "\\";

  success = CreateProcessA(
    exePath.c_str(), // lpApplicationName
    NULL, // lpCommandLine
    NULL, // lpProcessAttributes
    NULL, // lpThreadAttributes
    TRUE, // bInheritHandles
    0, // dwCreationFlags
    NULL, // lpEnvironment
    workDir.c_str(), // lpCurrentDirectory
    &startupInfo,
    &processInfo);
  assert(success);

  WaitForSingleObject(processInfo.hProcess, INFINITE);
  CloseHandle(processInfo.hThread);
  CloseHandle(processInfo.hProcess);

  CloseHandle(writePipe);
  CloseHandle(readPipe);

  readingThread.join();

  return output;
}

std::regex part1Regex("(Part|Result) ?1: ?(.*)");
std::regex part2Regex("(Part|Result) ?2: ?(.*)");


bool checkResult(int taskNr, const std::pair<std::string, std::optional<std::string>>& expected, const std::string& output) {
  std::smatch match;
  bool okay = true;
  if (!std::regex_search(output, match, part1Regex)) {
    std::cout << std::setfill('0') << std::setw(2) << taskNr << ": Failed to recognize part1 output in: \"" << output << "\"\n";
    okay = false;
  }

  auto actualFirst = match[2].str();
  if (expected.second && !std::regex_search(output, match, part2Regex)) {
    std::cout << std::setfill('0') << std::setw(2) << taskNr << ": Failed to recognize part2 output in: \"" << output << "\"\n";
    okay = false;
  }
  auto actualSecond = match[2].str();

  if (!okay) {
    return false; // don't compare results if parsing of output failed
  }



  if (actualFirst != expected.first) {
    std::cout << std::setfill('0') << std::setw(2) << taskNr << ": Wrong result in Part 1:\n"
      << "  expected: \"" << expected.first << "\"\n"
      << "  actual:   \"" << actualFirst << "\"\n";
    okay = false;
  }

  if (expected.second && *expected.second != actualSecond) {
    std::cout << std::setfill('0') << std::setw(2) << taskNr << ": Wrong result in Part 2:\n"
      << "  expected: \"" << *expected.second << "\"\n"
      << "  actual:   \"" << actualSecond << "\"\n";
    okay = false;
  }

  if (okay) { // no errors so far
    std::cout << std::setfill('0') << std::setw(2) << taskNr << ": OK\n";
  }

  return okay;
}




// This project will check the results of all exercises (to validate correct it works correct after refactorings)
int main() {
  for (auto line : stream::lines(std::ifstream("..\\data\\results.txt"))) {
    std::smatch match;
    std::regex_match(line, match, resultsRegex);
    if (match[1].matched) {
      // commented out
      results.push_back(std::nullopt);
    } else {
      std::pair<std::string, std::optional<std::string>> result;
      result.first = match[3].matched ? match[3].str() : match[4].str();
      if (match[5].matched) {
        result.second = match[7].matched ? match[7].str() : match[8].str();
      }
      results.push_back(result);
    }
  }


  int wrongTasks = 0;
  int taskNr = 0;
  for (auto& expected : results) {
    ++taskNr;
    if (expected) { // don't run commented out tasks
      auto output = getTaskOutput(taskNr);
      if (!checkResult(taskNr, *expected, output)) {
        ++wrongTasks;
      }
    } else {
      std::cout << std::setfill('0') << std::setw(2) << taskNr << ": skipped\n";
    }
  }

  std::cout << "\n\n" << wrongTasks << " Errors!";
}
