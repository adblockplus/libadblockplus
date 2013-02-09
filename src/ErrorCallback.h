#ifndef ADBLOCK_PLUS_ERROR_CALLBACK_H
#define ADBLOCK_PLUS_ERROR_CALLBACK_H

#include <string>

namespace AdblockPlus
{
  class ErrorCallback
  {
  public:
    virtual ~ErrorCallback();
    virtual void operator()(const std::string& message) = 0;
  };
}

#endif
