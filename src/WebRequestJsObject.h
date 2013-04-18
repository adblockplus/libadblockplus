#ifndef ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H
#define ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H

#include <v8.h>
#include <AdblockPlus/JsEngine.h>

namespace AdblockPlus
{
  class JsEngine;

  namespace WebRequestJsObject
  {
    JsValuePtr Setup(JsEnginePtr jsEngine, JsValuePtr obj);
  }
}

#endif
