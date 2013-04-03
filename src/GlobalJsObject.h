#ifndef ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H
#define ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H

#include "ConsoleJsObject.h"

namespace AdblockPlus
{
  namespace GlobalJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(ErrorCallback& errorCallback);
  }
}

#endif
