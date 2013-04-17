#ifndef ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H
#define ADBLOCK_PLUS_WEB_REQUEST_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class JsEngine;

  namespace WebRequestJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(JsEngine& jsEngine);
  }
}

#endif
