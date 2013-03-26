#ifndef ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H
#define ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H

#include <AdblockPlus/ErrorCallback.h>
#include <v8.h>

namespace AdblockPlus
{
  namespace ConsoleJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(ErrorCallback& errorCallback);
  }
}

#endif
