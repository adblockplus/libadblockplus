#ifndef ADBLOCKPLUS_ERROR_CALLBACK_H
#define ADBLOCKPLUS_ERROR_CALLBACK_H

#include <string>

#include "tr1_memory.h"

namespace AdblockPlus
{
  class ErrorCallback
  {
  public:
    virtual ~ErrorCallback();
    virtual void operator()(const std::string& message) = 0;
  };

  typedef std::tr1::shared_ptr<ErrorCallback> ErrorCallbackPtr;
}

#endif
