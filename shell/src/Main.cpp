#include <ErrorCallback.h>
#include <FileReader.h>
#include <fstream>
#include <iostream>
#include <JsEngine.h>
#include <sstream>

#include "HelpCommand.h"
#include "SubscriptionsCommand.h"
#include "MatchesCommand.h"

namespace
{
  class LibFileReader : public AdblockPlus::FileReader
  {
  public:
    std::auto_ptr<std::istream> Read(const std::string& path) const
    {
      std::ifstream* file = new std::ifstream;
      file->open(("lib/" + path).c_str());
      return std::auto_ptr<std::istream>(file);
    }
  };

  class CerrErrorCallback : public AdblockPlus::ErrorCallback
  {
  public:
    void operator()(const std::string& message)
    {
      std::cerr << "Error: " << message << std::endl;
    }
  };

  void Add(CommandMap& commands, Command* command)
  {
    commands[command->name] = command;
  }

  bool ReadCommandLine(std::string& commandLine)
  {
    std::cout << "> ";
    const bool success = std::getline(std::cin, commandLine);
    if (!success)
      std::cout << std::endl;
    return success;
  }

  void ParseCommandLine(const std::string& commandLine, std::string& name,
                        std::string& arguments)
  {
    std::istringstream lineStream(commandLine);
    lineStream >> name;
    std::getline(lineStream, arguments);
  }
}

int main()
{
  try
  {
    LibFileReader fileReader;
    CerrErrorCallback errorCallback;
    AdblockPlus::JsEngine jsEngine(&fileReader, 0);
    jsEngine.Load("start.js");

    CommandMap commands;
    Add(commands, new HelpCommand(commands));
    Add(commands, new SubscriptionsCommand(jsEngine));
    Add(commands, new MatchesCommand());

    std::string commandLine;
    while (ReadCommandLine(commandLine))
      {
        std::string commandName;
        std::string arguments;
        ParseCommandLine(commandLine, commandName, arguments);
        const CommandMap::const_iterator it = commands.find(commandName);
        try
          {
            if (it != commands.end())
              (*it->second)(arguments);
            else
              throw NoSuchCommandError(commandName);
          }
        catch (NoSuchCommandError error)
          {
            std::cout << error.what() << std::endl;
          }
      }
  }
  catch (const std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
