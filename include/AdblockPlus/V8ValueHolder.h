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

#ifndef ADBLOCK_PLUS_V8_VALUE_HOLDER_H
#define ADBLOCK_PLUS_V8_VALUE_HOLDER_H

#include <memory>

namespace v8
{
  class Isolate;
  template<class T> class Handle;
  template<class T> class Persistent;
}

namespace AdblockPlus
{
  template<class T>
  class V8ValueHolder
  {
  public:
    V8ValueHolder()
    {
      reset(0, v8::Persistent<T>());
    }
    V8ValueHolder(V8ValueHolder& value)
    {
      reset(value.isolate, static_cast<v8::Handle<T> >(value));
    }
    V8ValueHolder(v8::Isolate* isolate, v8::Persistent<T> value)
    {
      reset(isolate, value);
    }
    V8ValueHolder(v8::Isolate* isolate, v8::Handle<T> value)
    {
      reset(isolate, value);
    }
    ~V8ValueHolder()
    {
      if (this->value.get())
      {
        this->value->Dispose(this->isolate);
        this->value.reset(0);
      }
    }
    void reset(v8::Isolate* isolate, v8::Persistent<T> value)
    {
      if (this->value.get())
      {
        this->value->Dispose(this->isolate);
        this->value.reset(0);
      }

      if (!value.IsEmpty())
      {
        this->isolate = isolate;
        this->value.reset(new v8::Persistent<T>(value));
      }
    }

    void reset(v8::Isolate* isolate, v8::Handle<T> value)
    {
      reset(isolate, v8::Persistent<T>::New(isolate, value));
    }

    T* operator->() const
    {
      return **value;
    }
    operator v8::Handle<T>() const
    {
      return *value;
    }
    operator v8::Persistent<T>() const
    {
      return *value;
    }
  private:
    v8::Isolate* isolate;
    std::auto_ptr<v8::Persistent<T> > value;
  };
}

#endif
