#ifndef GC_COMMAND_H
#define GC_COMMAND_H

#include <AdblockPlus.h>
#include <string>

#include "Command.h"

class GcCommand : public Command
{
public:
  explicit GcCommand(AdblockPlus::JsEnginePtr jsEngine);
  void operator()(const std::string& arguments);
  std::string GetDescription() const;
  std::string GetUsage() const;

private:
  AdblockPlus::JsEnginePtr jsEngine;
};

#endif
