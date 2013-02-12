#ifndef ADBLOCK_PLUS_JS_CONSOLE_H
#define ADBLOCK_PLUS_JS_CONSOLE_H

#include <AdblockPlus/ErrorCallback.h>
#include <v8.h>

namespace JsConsole
{
  v8::Handle<v8::ObjectTemplate> Create(
    AdblockPlus::ErrorCallback& errorCallback);
};

#endif
