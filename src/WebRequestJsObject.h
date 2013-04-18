#ifndef ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H
#define ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class JsEngine;

  namespace WebRequestJsObject
  {
    JsValuePtr Setup(JsEngine& jsEngine, JsValuePtr obj);
  }
}

#endif
