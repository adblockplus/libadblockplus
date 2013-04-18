#include <iostream>
#include <AdblockPlus/DefaultErrorCallback.h>

void AdblockPlus::DefaultErrorCallback::operator()(const std::string& message)
{
  std::cerr << "Error: " << message << std::endl;
}
