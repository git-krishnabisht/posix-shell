#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// It takes a command as its parameter and checks whether it exist in the environment variables - PATH - or not. if the command exists then the function suffixes the command to the path it exists in and returns the whole path containing the command it in otherwise returns an empty string. 
std::string find_executable_in_path(const std::string& command) {
  const char* path_env = std::getenv("PATH");
  if (!path_env) return "";

  std::istringstream paths(path_env);
  std::string dir;

  while (std::getline(paths, dir, ':')) {
    std::string full_path = dir + "/" + command;
    if (access(full_path.c_str(), X_OK) == 0) {
      return full_path;
    }
  }

  return "";
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::unordered_set<std::string> _builtin_types = { "echo", "exit", "type" };

  while (true) {
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    if (std::cin.eof()) {
      std::cout << std::endl;
      break;
    }

    std::stringstream ss(input);
    std::string command;
    ss >> command;

    if (input == "exit 0") {
      return 0;
    }
    else if (command == "echo") {
      std::string args;
      std::getline(ss, args);
      args.erase(0, args.find_first_not_of(" \t"));
      std::cout << args << (args.empty() ? "" : "\n");

    }
    else if (command == "type") {
      std::string type;
      ss >> type;

      if (type.empty()) {
        std::cout << "no type given" << std::endl;
        continue;
      }

      if (_builtin_types.find(type) != _builtin_types.end()) {
        std::cout << type << " is a shell builtin" << std::endl;
        continue;
      }

      std::string res_path = find_executable_in_path(type);
      if (res_path.empty()) {
        std::cout << type << ": not found" << std::endl;
        continue;
      }

      std::cout << type << " is " << res_path << std::endl;
    }
    else if (!command.empty()) {
      std::string path = find_executable_in_path(command);
      if (path.empty()) { // this means that the PATH env variable doesn't have any PATH variable that contains the 'command'.
        std::cout << command << ": command not found" << std::endl;
        continue;
      }

      std::vector<std::string> args;
      args.push_back(command);
      std::string arg;
      while (ss >> arg) {
        args.push_back(arg);
      }

      std::vector<char*> c_args;
      for (auto& s : args) {
        c_args.push_back(const_cast<char*>(s.c_str()));
      }
      c_args.push_back(nullptr);

      pid_t pid = fork();
      if (pid == 0) {
        execvp(path.c_str(), c_args.data());
        std::perror("execvp failed");
        std::exit(1);
      }
      else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
      }
      else {
        std::perror("fork failed");
      }
    }
  }

  return 0;
}

