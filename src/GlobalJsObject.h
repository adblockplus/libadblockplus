#ifndef ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H
#define ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H

#include <v8.h>
#include <AdblockPlus/JsEngine.h>

namespace AdblockPlus
{
  class AppInfo;
  class JsEngine;

  namespace GlobalJsObject
  {
    JsValuePtr Setup(JsEnginePtr jsEngine, const AppInfo& appInfo, JsValuePtr obj);
  }
}

#endif
