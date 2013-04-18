#include "GcCommand.h"

GcCommand::GcCommand(AdblockPlus::JsEnginePtr jsEngine)
  : Command("gc"), jsEngine(jsEngine)
{
}

void GcCommand::operator()(const std::string& arguments)
{
  jsEngine->Gc();
}

std::string GcCommand::GetDescription() const
{
  return "Initiates a garbage collection";
}

std::string GcCommand::GetUsage() const
{
  return name;
}
