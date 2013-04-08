#ifndef ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H
#define ADBLOCK_PLUS_GLOBAL_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class ErrorCallback;
  class WebRequest;

  namespace GlobalJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(ErrorCallback& errorCallback,
        WebRequest& webRequest);
  }
}

#endif
