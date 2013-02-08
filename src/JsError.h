#ifndef ADBLOCK_PLUS_JS_ERROR_H
#define ADBLOCK_PLUS_JS_ERROR_H

#include <stdexcept>

namespace AdblockPlus
{
  class JsError : public std::runtime_error
  {
  public:
    JsError(const std::string& message);
  };
}

#endif
