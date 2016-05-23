/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2016 Eyeo GmbH
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
#include <memory>

namespace v8
{
  class Value;
  template<class T> class Handle;
  template<class T> class Local;
  template<class T> class Persistent;
}

namespace AdblockPlus
{
  class JsValue;
  class JsEngine;

  typedef std::shared_ptr<JsEngine> JsEnginePtr;

  /**
   * Shared smart pointer to a `JsValue` instance.
   */
  typedef std::shared_ptr<JsValue> JsValuePtr;

  /**
   * List of JavaScript values.
   */
  typedef std::vector<AdblockPlus::JsValuePtr> JsValueList;

  /**
   * Wrapper for JavaScript values.
   * See `JsEngine` for creating `JsValue` objects.
   */
  class JsValue
  {
    friend class JsEngine;
  public:
    JsValue(JsValue&& src);
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

    /**
     * Returns a list of property names if this is an object (see `IsObject()`).
     * @return List of property names.
     */
    std::vector<std::string> GetOwnPropertyNames() const;

    /**
     * Returns a property value if this is an object (see `IsObject()`).
     * @param name Property name.
     * @return Property value, undefined (see `IsUndefined()`) if the property
     *         does not exist.
     */
    JsValuePtr GetProperty(const std::string& name) const;

    //@{
    /**
     * Sets a property value if this is an object (see `IsObject()`).
     * @param name Property name.
     * @param val Property value.
     */
    void SetProperty(const std::string& name, const std::string& val);
    void SetProperty(const std::string& name, int64_t val);
    void SetProperty(const std::string& name, bool val);
    void SetProperty(const std::string& name, const JsValuePtr& value);
    inline void SetProperty(const std::string& name, const char* val)
    {
      SetProperty(name, std::string(val));
    }
    inline void SetProperty(const std::string& name, int val)
    {
      SetProperty(name, static_cast<int64_t>(val));
    }
    //@}

    /**
     * Returns the value's class name, e.g.\ _Array_ for arrays
     * (see `IsArray()`).
     * Technically, this is the name of the function that was used as a
     * constructor.
     * @return Class name of the value.
     */
    std::string GetClass() const;

    /**
     * Invokes the value as a function (see `IsFunction()`).
     * @param params Optional list of parameters.
     * @param thisPtr Optional `this` value.
     * @return Value returned by the function.
     */
    JsValuePtr Call(const JsValueList& params = JsValueList(),
        AdblockPlus::JsValuePtr thisPtr = AdblockPlus::JsValuePtr()) const;

    /**
     * Invokes the value as a function (see `IsFunction()`) with single
     * parameter.
     * @param arg A single required parameter.
     * @return Value returned by the function.
     */
    JsValuePtr Call(const JsValue& arg) const;

    v8::Local<v8::Value> UnwrapValue() const;
  protected:
    JsEnginePtr jsEngine;
  private:
    JsValue(JsEnginePtr jsEngine, v8::Handle<v8::Value> value);
    void SetProperty(const std::string& name, v8::Handle<v8::Value> val);
    std::unique_ptr<v8::Persistent<v8::Value>> value;
  };
}

#endif
