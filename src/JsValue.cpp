/*
 * This file is part of Adblock Plus <http://adblockplus.org/>,
 * Copyright (C) 2006-2013 Eyeo GmbH
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

#include <AdblockPlus.h>

namespace
{
  std::string fromV8String(v8::Handle<v8::Value> value)
  {
    v8::String::Utf8Value stringValue(value);
    if (stringValue.length())
      return std::string(*stringValue, stringValue.length());
    else
      return std::string();
  }

  v8::Local<v8::String> toV8String(const std::string& str)
  {
    return v8::String::New(str.c_str(), str.length());
  }
}

AdblockPlus::JsValue::JsValue(AdblockPlus::JsEnginePtr jsEngine,
      v8::Handle<v8::Value> value)
    : jsEngine(jsEngine),
      value(v8::Persistent<v8::Value>::New(jsEngine->isolate, value))
{
}

AdblockPlus::JsValue::JsValue(AdblockPlus::JsValuePtr value)
    : jsEngine(value->jsEngine),
      value(v8::Persistent<v8::Value>::New(jsEngine->isolate, value->value))
{
}

AdblockPlus::JsValue::~JsValue()
{
  value.Dispose(jsEngine->isolate);
}

bool AdblockPlus::JsValue::IsUndefined() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsUndefined();
}

bool AdblockPlus::JsValue::IsNull() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsNull();
}

bool AdblockPlus::JsValue::IsString() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsString() || value->IsStringObject();
}

bool AdblockPlus::JsValue::IsNumber() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsNumber() || value->IsNumberObject();
}

bool AdblockPlus::JsValue::IsBool() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsBoolean() || value->IsBooleanObject();
}

bool AdblockPlus::JsValue::IsObject() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsObject();
}

bool AdblockPlus::JsValue::IsArray() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsArray();
}

bool AdblockPlus::JsValue::IsFunction() const
{
  const JsEngine::Context context(jsEngine);
  return value->IsFunction();
}

std::string AdblockPlus::JsValue::AsString() const
{
  const JsEngine::Context context(jsEngine);
  return fromV8String(value);
}

int64_t AdblockPlus::JsValue::AsInt() const
{
  const JsEngine::Context context(jsEngine);
  return value->IntegerValue();
}

bool AdblockPlus::JsValue::AsBool() const
{
  const JsEngine::Context context(jsEngine);
  return value->BooleanValue();
}

AdblockPlus::JsValueList AdblockPlus::JsValue::AsList() const
{
  if (!IsArray())
    throw std::runtime_error("Cannot convert a non-array to list");

  const JsEngine::Context context(jsEngine);
  JsValueList result;
  v8::Persistent<v8::Array> array = v8::Persistent<v8::Array>::Cast(value);
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

  const JsEngine::Context context(jsEngine);
  const v8::Persistent<v8::Object> object = v8::Persistent<v8::Object>::Cast(value);
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

  const JsEngine::Context context(jsEngine);
  v8::Local<v8::String> property = toV8String(name);
  v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::Cast(value);
  return JsValuePtr(new JsValue(jsEngine, obj->Get(property)));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, v8::Handle<v8::Value> val)
{
  if (!IsObject())
    throw new std::runtime_error("Attempting to set property on a non-object");

  v8::Local<v8::String> property = toV8String(name);
  v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::Cast(value);
  obj->Set(property, val);
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, const std::string& val)
{
  const JsEngine::Context context(jsEngine);
  SetProperty(name, toV8String(val));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, int64_t val)
{
  const JsEngine::Context context(jsEngine);
  SetProperty(name, v8::Number::New(val));
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, JsValuePtr val)
{
  const JsEngine::Context context(jsEngine);
  SetProperty(name, val->value);
}

void AdblockPlus::JsValue::SetProperty(const std::string& name, bool val)
{
  const JsEngine::Context context(jsEngine);
  SetProperty(name, v8::Boolean::New(val));
}

std::string AdblockPlus::JsValue::GetClass() const
{
  if (!IsObject())
    throw new std::runtime_error("Cannot get constructor of a non-object");

  const JsEngine::Context context(jsEngine);
  v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::Cast(value);
  return fromV8String(obj->GetConstructorName());
}

AdblockPlus::JsValuePtr AdblockPlus::JsValue::Call(
    const JsValueList& params,
    AdblockPlus::JsValuePtr thisPtr) const
{
  if (!IsFunction())
    throw new std::runtime_error("Attempting to call a non-function");

  const JsEngine::Context context(jsEngine);

  if (!thisPtr)
    thisPtr = JsValuePtr(new JsValue(jsEngine, jsEngine->context->Global()));
  if (!thisPtr->IsObject())
    throw new std::runtime_error("`this` pointer has to be an object");
  v8::Persistent<v8::Object> thisObj = v8::Persistent<v8::Object>::Cast(thisPtr->value);

  size_t argc = params.size();
  v8::Handle<v8::Value>* argv = new v8::Handle<v8::Value>[argc];
  for (size_t i = 0; i < argc; ++i)
    argv[i] = params[i]->value;

  const v8::TryCatch tryCatch;
  v8::Persistent<v8::Function> func = v8::Persistent<v8::Function>::Cast(value);
  v8::Local<v8::Value> result = func->Call(thisObj, argc, argv);
  delete argv;

  if (tryCatch.HasCaught())
    throw AdblockPlus::JsError(tryCatch.Exception(), tryCatch.Message());

  return JsValuePtr(new JsValue(jsEngine, result));
}
