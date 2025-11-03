#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <climits>
#include <filesystem>

#ifdef _WIN32
#include <direct.h>   
#define chdir _chdir  
#endif

namespace fs = std::filesystem;

std::string trim_left(const std::string& str) {
  size_t start = str.find_first_not_of(" \t");
  return (start == std::string::npos) ? "" : str.substr(start);
}

std::string find_executable_in_path(const std::string& command) {
  const char* path_env = std::getenv("PATH");
  if (!path_env) return "";

  std::istringstream paths(path_env);
  std::string dir;

  while (std::getline(paths, dir, ':')) {
    if (dir.empty()) continue;

    std::string full_path = dir + "/" + command;
    if (access(full_path.c_str(), X_OK) == 0) {
      return full_path;
    }
  }
  return "";
}


void parse_input(const std::string& input, std::string& command, std::vector<std::string>& args) {
  std::istringstream ss(input);
  ss >> command;

  std::string arg;
  while (ss >> arg) {
    args.push_back(arg);
  }
}

void execute_external_command(const std::string& command, const std::vector<std::string>& args) {
  std::string path = find_executable_in_path(command);
  if (path.empty()) {
    std::cout << command << ": command not found" << std::endl;
    return;
  }

  std::vector<char*> c_args;
  c_args.push_back(const_cast<char*>(command.c_str()));
  for (const auto& arg : args) {
    c_args.push_back(const_cast<char*>(arg.c_str()));
  }
  c_args.push_back(nullptr);

  pid_t pid = fork();
  if (pid == 0) {
    execvp(path.c_str(), c_args.data());
    std::perror("execvp failed");
    std::exit(EXIT_FAILURE);
  }
  else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0);
  }
  else {
    std::perror("fork failed");
  }
}

void handle_echo(std::istringstream& ss) {
  std::string args;
  std::getline(ss, args);
  args = trim_left(args);
  std::cout << args << std::endl;
}

void handle_pwd() {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != nullptr) {
    std::cout << cwd << std::endl;
  }
  else {
    std::perror("getcwd failed");
  }
  return;
}

void handle_cd(std::string& path) {
  std::string err_msg = "cd: " + path + ": No such file or directory";
  if (fs::exists(path) && fs::is_directory(path)) {
    if (chdir(path.c_str()) == 0) {
      return;
    }
    else {
      std::cout << err_msg << std::endl;
    }
  }
  else {
    std::cout << err_msg << std::endl;
  }
  return;
}

void handle_type(const std::string& arg, const std::unordered_set<std::string>& builtins) {
  if (arg.empty()) {
    std::cout << "type: missing argument" << std::endl;
    return;
  }

  if (builtins.find(arg) != builtins.end()) {
    std::cout << arg << " is a shell builtin" << std::endl;
  }
  else {
    std::string path = find_executable_in_path(arg);
    if (path.empty()) {
      std::cout << arg << ": not found" << std::endl;
    }
    else {
      std::cout << arg << " is " << path << std::endl;
    }
  }
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  const std::unordered_set<std::string> builtins = { "echo", "exit", "type", "pwd" };

  while (true) {
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    if (std::cin.eof()) {
      std::cout << std::endl;
      break;
    }

    if (input.empty()) continue;

    std::istringstream ss(input);
    std::string command;
    ss >> command;

    if (command.empty()) continue;

    if (command == "exit") {
      std::string exit_code;
      ss >> exit_code;
      return (exit_code.empty()) ? 0 : std::atoi(exit_code.c_str());
    }

    if (command == "echo") {
      handle_echo(ss);
    }
    else if (command == "pwd") {
      handle_pwd();
    }
    else if (command == "type") {
      std::string arg;
      ss >> arg;
      handle_type(arg, builtins);
    }
    else if (command == "cd") {
      std::string path;
      ss >> path;
      handle_cd(path);
    }
    else {
      std::vector<std::string> args;
      std::string arg;
      while (ss >> arg) {
        args.push_back(arg);
      }
      execute_external_command(command, args);
    }
  }

  return 0;
}
