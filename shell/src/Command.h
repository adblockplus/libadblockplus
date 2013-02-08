#ifndef COMMAND_H
#define COMMAND_H

#include <map>
#include <stdexcept>
#include <string>

struct Command
{
  const std::string name;

  Command(const std::string& name);
  virtual ~Command();
  virtual void operator()(const std::string& arguments) = 0;
  virtual std::string GetDescription() const = 0;
  virtual std::string GetUsage() const = 0;
};

typedef std::map<const std::string, Command*> CommandMap;

class NoSuchCommandError : public std::runtime_error
{
public:
  NoSuchCommandError(const std::string& commandName);
};

#endif
