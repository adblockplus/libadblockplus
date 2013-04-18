#ifndef ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H
#define ADBLOCK_PLUS_CONSOLE_JS_OBJECT_H

#include <v8.h>
#include <AdblockPlus/JsEngine.h>

namespace AdblockPlus
{
  class JsEngine;

  namespace ConsoleJsObject
  {
    JsValuePtr Setup(JsEnginePtr jsEngine, JsValuePtr obj);
  }
}

#endif
