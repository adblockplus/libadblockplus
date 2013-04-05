#include "GlobalJsObject.h"

using namespace AdblockPlus;

v8::Handle<v8::ObjectTemplate> GlobalJsObject::Create(
  ErrorCallback& errorCallback)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();
  const v8::Handle<v8::ObjectTemplate> console =
    AdblockPlus::ConsoleJsObject::Create(errorCallback);
  global->Set(v8::String::New("console"), console);
  return handleScope.Close(global);
}
