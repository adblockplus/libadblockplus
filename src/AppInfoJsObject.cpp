/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/JsValue.h>

#include "AppInfoJsObject.h"
#include "Utils.h"

using namespace AdblockPlus;

JsValuePtr AppInfoJsObject::Setup(JsEnginePtr jsEngine, const AppInfo& appInfo,
    JsValuePtr obj)
{
  obj->SetProperty("version", appInfo.version);
  obj->SetProperty("name", appInfo.name);
  obj->SetProperty("application", appInfo.application);
  obj->SetProperty("applicationVersion", appInfo.applicationVersion);
  obj->SetProperty("locale", appInfo.locale);
  obj->SetProperty("developmentBuild", appInfo.developmentBuild);
  return obj;
}
