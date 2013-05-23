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

#ifndef ADBLOCK_PLUS_JS_VALUE_H
#define ADBLOCK_PLUS_JS_VALUE_H

#include <stdint.h>
#include <string>
#include <vector>
#include "tr1_memory.h"
#include "V8ValueHolder.h"

namespace v8
{
  class Value;
  template <class T> class Handle;
  template <class T> class Persistent;
}

namespace AdblockPlus
{
  class JsValue;
  class JsEngine;

  typedef std::tr1::shared_ptr<JsValue> JsValuePtr;
  typedef std::vector<AdblockPlus::JsValuePtr> JsValueList;

  // Forward declaration to avoid including JsEngine.h
  typedef std::tr1::shared_ptr<JsEngine> JsEnginePtr;

  class JsValue
  {
    friend class JsEngine;
  public:
    JsValue(JsValuePtr value);
    virtual ~JsValue();

    bool IsUndefined() const;
    bool IsNull() const;
    bool IsString() const;
    bool IsNumber() const;
    bool IsBool() const;
    bool IsObject() const;
    bool IsArray() const;
    bool IsFunction() const;
    std::string AsString() const;
    int64_t AsInt() const;
    bool AsBool() const;
    JsValueList AsList() const;
    std::vector<std::string> GetOwnPropertyNames() const;
    JsValuePtr GetProperty(const std::string& name) const;
    void SetProperty(const std::string& name, const std::string& val);
    void SetProperty(const std::string& name, int64_t val);
    void SetProperty(const std::string& name, bool val);
    void SetProperty(const std::string& name, JsValuePtr value);
    inline void SetProperty(const std::string& name, const char* val)
    {
      SetProperty(name, std::string(val));
    }
    inline void SetProperty(const std::string& name, int val)
    {
      SetProperty(name, static_cast<int64_t>(val));
    }
    std::string GetClass() const;
    JsValuePtr Call(const JsValueList& params = JsValueList(),
        AdblockPlus::JsValuePtr thisPtr = AdblockPlus::JsValuePtr()) const;
  protected:
    JsValue(JsEnginePtr jsEngine, v8::Handle<v8::Value> value);
    void SetProperty(const std::string& name, v8::Handle<v8::Value> val);

    JsEnginePtr jsEngine;
    V8ValueHolder<v8::Value> value;
  };
}

#endif
