#ifndef ADBLOCKPLUS_DEFAULT_ERROR_CALLBACK_H
#define ADBLOCKPLUS_DEFAULT_ERROR_CALLBACK_H

#include "ErrorCallback.h"

namespace AdblockPlus
{
  class DefaultErrorCallback : public ErrorCallback
  {
  public:
    void operator()(const std::string& message);
  };
}

#endif
