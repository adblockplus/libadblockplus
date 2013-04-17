#include <AdblockPlus/AppInfo.h>

#include "AppInfoJsObject.h"
#include "Utils.h"

using namespace AdblockPlus;

v8::Handle<v8::ObjectTemplate> AppInfoJsObject::Create(const AppInfo& appInfo)
{
  v8::HandleScope handleScope;
  const v8::Handle<v8::ObjectTemplate> infoObject = v8::ObjectTemplate::New();
  infoObject->Set(v8::String::New("id"), Utils::ToV8String(appInfo.id));
  infoObject->Set(v8::String::New("version"),
                  Utils::ToV8String(appInfo.version));
  infoObject->Set(v8::String::New("name"), Utils::ToV8String(appInfo.name));
  infoObject->Set(v8::String::New("platform"),
                  Utils::ToV8String(appInfo.platform));
  return handleScope.Close(infoObject);
}
