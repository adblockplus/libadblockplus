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
    typedef typename v8::Persistent<T> V8Persistent;
    V8ValueHolder()
    {
      reset();
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
    void reset(v8::Isolate* isolate, v8::Handle<T> value)
    {
      reset(isolate, std::auto_ptr<V8Persistent>(new V8Persistent(isolate, value)));
    }
    void reset(v8::Isolate* isolate = 0, std::auto_ptr<V8Persistent> value = std::auto_ptr<V8Persistent>(new V8Persistent()))
    {
      if (this->value.get())
      {
        this->value->Dispose(this->isolate);
        this->value.reset(0);
      }

      if (!value->IsEmpty())
      {
        this->isolate = isolate;
        this->value = value;
      }
    }

    operator V8Persistent&() const
    {
      return *value;
    }
  private:
    v8::Isolate* isolate;
    std::auto_ptr<V8Persistent> value;
  };
}

#endif
