#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/JsValue.h>

#include "AppInfoJsObject.h"
#include "Utils.h"

using namespace AdblockPlus;

JsValuePtr AppInfoJsObject::Setup(JsEnginePtr jsEngine, const AppInfo& appInfo,
    JsValuePtr obj)
{
  obj->SetProperty("id", appInfo.id);
  obj->SetProperty("version", appInfo.version);
  obj->SetProperty("name", appInfo.name);
  obj->SetProperty("platform", appInfo.platform);
  return obj;
}
