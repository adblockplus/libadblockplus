#include <iostream>

namespace
{
  bool ReadCommandLine(std::string& commandLine)
  {
    std::cout << "> ";
    bool success = std::getline(std::cin, commandLine);
    if (!success)
      std::cout << std::endl;
    return success;
  }
}

int main()
{
  std::string commandLine;
  while (ReadCommandLine(commandLine))
    std::cout << commandLine << std::endl;
  return 0;
}
