#include "JsError.h"

AdblockPlus::JsError::JsError(const std::string& message)
  : std::runtime_error(message)
{
}
