#ifndef ADBLOCK_PLUS_APP_INFO_JS_OBJECT_H
#define ADBLOCK_PLUS_APP_INFO_JS_OBJECT_H

#include <v8.h>
#include <AdblockPlus/JsEngine.h>

namespace AdblockPlus
{
  struct AppInfo;
  class ErrorCallback;

  namespace AppInfoJsObject
  {
    JsValuePtr Setup(JsEnginePtr jsEngine, const AppInfo& appInfo, JsValuePtr obj);
  }
}

#endif
