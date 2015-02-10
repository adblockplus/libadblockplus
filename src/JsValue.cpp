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

#include <vector>
#include <AdblockPlus.h>

#include "JsContext.h"
#include "JsError.h"
#include "Utils.h"

AdblockPlus::JsValue::JsValue(AdblockPlus::JsEnginePtr jsEngine,
      v8::Handle<v8::Value> value)
    : jsEngine(jsEngine),
      value(jsEngine->isolate, value)
{
}

AdblockPlus::JsValue::JsValue(AdblockPlus::JsValuePtr value)
    : jsEngine(value->jsEngine),
      value(value->value)
{
}

AdblockPlus::JsValue::~JsValue()
{
}

bool AdblockPlus::JsValue::IsUndefined() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IsUndefined();
}

bool AdblockPlus::JsValue::IsNull() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IsNull();
}

bool AdblockPlus::JsValue::IsString() const
{
  const JsContext context(jsEngine);
  v8::Local<v8::Value> value = UnwrapValue();
  return value->IsString() || value->IsStringObject();
}

bool AdblockPlus::JsValue::IsNumber() const
{
  const JsContext context(jsEngine);
  v8::Local<v8::Value> value = UnwrapValue();
  return value->IsNumber() || value->IsNumberObject();
}

bool AdblockPlus::JsValue::IsBool() const
{
  const JsContext context(jsEngine);
  v8::Local<v8::Value> value = UnwrapValue();
  return value->IsBoolean() || value->IsBooleanObject();
}

bool AdblockPlus::JsValue::IsObject() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IsObject();
}

bool AdblockPlus::JsValue::IsArray() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IsArray();
}

bool AdblockPlus::JsValue::IsFunction() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IsFunction();
}

std::string AdblockPlus::JsValue::AsString() const
{
  const JsContext context(jsEngine);
  return Utils::FromV8String(UnwrapValue());
}

int64_t AdblockPlus::JsValue::AsInt() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->IntegerValue();
}

bool AdblockPlus::JsValue::AsBool() const
{
  const JsContext context(jsEngine);
  return UnwrapValue()->BooleanValue();
}

AdblockPlus::JsValueList AdblockPlus::JsValue::AsList() const
{
  if (!IsArray())
    throw std::runtime_error("Cannot convert a non-array to list");

  const JsContext context(jsEngine);
  JsValueList result;
  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(UnwrapValue());
  uint32_t length = array->Length();
  for (uint32_t i = 0; i < length; i++)
  {
    v8::Local<v8::Value> item = array->Get(i);
    result.push_back(JsValuePtr(new JsValue(jsEngine, item)));
  }
  return result;
}

std::vector<std::string> AdblockPlus::JsValue::GetOwnPropertyNames() const
{
  if (!IsObject())
    throw new std::runtime_error("Attempting to get propert list for a non-object");

  const JsContext context(jsEngine);
  v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(UnwrapValue());
  JsValueList properties = JsValuePtr(new JsValue(jsEngine, object->GetOwnPropertyNames()))->AsList();
  std::vector<std::string> result;
  for (JsValueList::iterator it = properties.begin(); it != properties.end(); ++it)
    result.push_back((*it)->AsString());
  return result;
}


AdblockPlus::JsValuePtr AdblockPlus::JsValue::GetProperty(const std::string& name) const
{
  if (!IsObject())
    throw new std::runtime_error("Attempting to get property of a non-object");

  const JsContext context(jsEngine);
  v8::Local<v8::String> property = Utils::ToV8String(jsEngine->isolate, name);
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(UnwrapValue());
  return JsValuePtr(new JsValue(jsEngine, obj->Get(property)));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, v8::Handle<v8::Value> val)
{
  if (!IsObject())
    throw new std::runtime_error("Attempting to set property on a non-object");

  v8::Local<v8::String> property = Utils::ToV8String(jsEngine->isolate, name);
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(UnwrapValue());
  obj->Set(property, val);
}

v8::Local<v8::Value> AdblockPlus::JsValue::UnwrapValue() const
{
  return v8::Local<v8::Value>::New(jsEngine->isolate, value);
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, const std::string& val)
{
  const JsContext context(jsEngine);
  SetProperty(name, Utils::ToV8String(jsEngine->isolate, val));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, int64_t val)
{
  const JsContext context(jsEngine);
  SetProperty(name, v8::Number::New(jsEngine->isolate, val));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, const JsValuePtr& val)
{
  const JsContext context(jsEngine);
  SetProperty(name, val->UnwrapValue());
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, bool val)
{
  const JsContext context(jsEngine);
  SetProperty(name, v8::Boolean::New(val));
}

std::string AdblockPlus::JsValue::GetClass() const
{
  if (!IsObject())
    throw new std::runtime_error("Cannot get constructor of a non-object");

  const JsContext context(jsEngine);
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(UnwrapValue());
  return Utils::FromV8String(obj->GetConstructorName());
}

AdblockPlus::JsValuePtr AdblockPlus::JsValue::Call(const JsValueList& params, JsValuePtr thisPtr) const
{
  if (!IsFunction())
    throw new std::runtime_error("Attempting to call a non-function");

  const JsContext context(jsEngine);
  if (!thisPtr)
  {
    v8::Local<v8::Context> localContext = v8::Local<v8::Context>::New(jsEngine->isolate, jsEngine->context);
    thisPtr = JsValuePtr(new JsValue(jsEngine, localContext->Global()));
  }
  if (!thisPtr->IsObject())
    throw new std::runtime_error("`this` pointer has to be an object");
  v8::Local<v8::Object> thisObj = v8::Local<v8::Object>::Cast(thisPtr->UnwrapValue());

  std::vector<v8::Handle<v8::Value>> argv;
  for (JsValueList::const_iterator it = params.begin(); it != params.end(); ++it)
    argv.push_back((*it)->UnwrapValue());

  const v8::TryCatch tryCatch;
  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(UnwrapValue());
  v8::Local<v8::Value> result = func->Call(thisObj, argv.size(),
      argv.size() ? &argv.front() : 0);

  if (tryCatch.HasCaught())
    throw JsError(tryCatch.Exception(), tryCatch.Message());

  return JsValuePtr(new JsValue(jsEngine, result));
}
