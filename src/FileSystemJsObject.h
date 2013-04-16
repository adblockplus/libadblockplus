#ifndef ADBLOCK_PLUS_FILE_SYSTEM_JS_OBJECT_H
#define ADBLOCK_PLUS_FILE_SYSTEM_JS_OBJECT_H

#include <v8.h>

namespace AdblockPlus
{
  class FileSystem;

  namespace FileSystemJsObject
  {
    v8::Handle<v8::ObjectTemplate> Create(FileSystem& fileSystem);
  }
}

#endif
