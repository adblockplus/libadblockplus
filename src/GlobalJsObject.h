#ifndef ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H
#define ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class AppInfo;
  class JsEngine;

  namespace GlobalJsObject
  {
    JsValuePtr Setup(JsEngine& jsEngine, const AppInfo& appInfo, JsValuePtr obj);
  }
}

#endif
