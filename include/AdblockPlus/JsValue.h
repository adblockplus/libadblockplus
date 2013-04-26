#ifndef ADBLOCKPLUS_JS_VALUE_H
#define ADBLOCKPLUS_JS_VALUE_H

#include <string>
#include <vector>
#include <v8.h>
#include "tr1_memory.h"

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
    std::string GetClassName() const;
    JsValuePtr Call(const JsValueList& params = JsValueList(),
        AdblockPlus::JsValuePtr thisPtr = AdblockPlus::JsValuePtr()) const;
  protected:
    JsValue(JsEnginePtr jsEngine, v8::Handle<v8::Value> value);
    void SetProperty(const std::string& name, v8::Handle<v8::Value> val);

    JsEnginePtr jsEngine;
    v8::Persistent<v8::Value> value;
  };
}

#endif
