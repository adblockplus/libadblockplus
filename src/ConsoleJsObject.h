#ifndef ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H
#define ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class ErrorCallback;

  namespace ConsoleJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(ErrorCallback& errorCallback);
  }
}

#endif
