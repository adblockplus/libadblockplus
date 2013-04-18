#ifndef ADBLOCK_PLUS_FILE_SYSTEM_JS_OBJECT_H
#define ADBLOCK_PLUS_FILE_SYSTEM_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class FileSystem;

  namespace FileSystemJsObject
  {
    JsValuePtr Setup(JsEngine& jsEngine, JsValuePtr obj);
  }
}

#endif
