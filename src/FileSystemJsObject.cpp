#include &lt;AdblockPlus/FileSystem.h&gt;
#include &lt;stdexcept&gt;
#include &lt;sstream&gt;
#include &lt;vector&gt;

#include "FileSystemJsObject.h"
#include "Utils.h"
#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  class IoThread : public Thread
  {
  public:
    IoThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback)
      : isolate(v8::Isolate::GetCurrent()),
        context(v8::Persistent&lt;v8::Context&gt;::New(isolate,
                                                 v8::Context::GetCurrent())),
        fileSystem(fileSystem), callback(callback)
    {
    }

    virtual ~IoThread()
    {
      callback.Dispose(isolate);
      context.Dispose(isolate);
    }

  protected:
    v8::Isolate* const isolate;
    v8::Persistent&lt;v8::Context&gt; context;
    FileSystem&amp; fileSystem;
    v8::Persistent&lt;v8::Function&gt; callback;
  };

  class ReadThread : public IoThread
  {
  public:
    ReadThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback,
               const std::string&amp; path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string content;
      std::string error;
      try
      {
        std::auto_ptr&lt;std::istream&gt; stream = fileSystem.Read(path);
        content = Utils::Slurp(*stream);
      }
      catch (std::exception&amp; e)
      {
        error = e.what();
      }
      catch (...)
      {
        error =  "Unknown error while reading from " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle&lt;v8::Object&gt; result = v8::Object::New();
      result-&gt;Set(v8::String::New("content"), v8::String::New(content.c_str()));
      result-&gt;Set(v8::String::New("error"), v8::String::New(error.c_str()));
      callback-&gt;Call(callback, 1,
                     reinterpret_cast&lt;v8::Handle&lt;v8::Value&gt;*&gt;(&amp;result));
      delete this;
    }

  private:
    std::string path;
  };

  class WriteThread : public IoThread
  {
  public:
    WriteThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback,
                const std::string&amp; path, const std::string&amp; content)
      : IoThread(fileSystem, callback), path(path), content(content)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem.Write(path, content);
      }
      catch (std::exception&amp; e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while writing to " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle&lt;v8::Value&gt; errorValue = v8::String::New(error.c_str());
      callback-&gt;Call(callback, 1, &amp;errorValue);
      delete this;
    }

  private:
    std::string path;
    std::string content;
  };

  class MoveThread : public IoThread
  {
  public:
    MoveThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback,
               const std::string&amp; fromPath, const std::string&amp; toPath)
      : IoThread(fileSystem, callback), fromPath(fromPath), toPath(toPath)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem.Move(fromPath, toPath);
      }
      catch (std::exception&amp; e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while moving " + fromPath + " to " + toPath;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle&lt;v8::Value&gt; errorValue = v8::String::New(error.c_str());
      callback-&gt;Call(callback, 1, &amp;errorValue);
      delete this;
    }

  private:
    std::string fromPath;
    std::string toPath;
  };

  class RemoveThread : public IoThread
  {
  public:
    RemoveThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback,
                 const std::string&amp; path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      try
      {
        fileSystem.Remove(path);
      }
      catch (std::exception&amp; e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while removing " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle&lt;v8::Value&gt; errorValue = v8::String::New(error.c_str());
      callback-&gt;Call(callback, 1, &amp;errorValue);
      delete this;
    }

  private:
    std::string path;
  };

  class StatThread : public IoThread
  {
  public:
    StatThread(FileSystem&amp; fileSystem, v8::Persistent&lt;v8::Function&gt; callback,
               const std::string&amp; path)
      : IoThread(fileSystem, callback), path(path)
    {
    }

    void Run()
    {
      std::string error;
      FileSystem::StatResult statResult;
      try
      {
        statResult = fileSystem.Stat(path);
      }
      catch (std::exception&amp; e)
      {
        error = e.what();
      }
      catch (...)
      {
        error = "Unknown error while calling stat on " + path;
      }

      const v8::Locker locker(isolate);
      const v8::HandleScope handleScope;
      const v8::Context::Scope contextScope(context);
      v8::Handle&lt;v8::Object&gt; result = v8::Object::New();
      result-&gt;Set(v8::String::New("exists"),
                  v8::Boolean::New(statResult.exists));
      result-&gt;Set(v8::String::New("isFile"),
                  v8::Boolean::New(statResult.isFile));
      result-&gt;Set(v8::String::New("isDirectory"),
                  v8::Boolean::New(statResult.isDirectory));
      result-&gt;Set(v8::String::New("lastModified"),
                  v8::Number::New(statResult.lastModified));
      result-&gt;Set(v8::String::New("error"), v8::String::New(error.c_str()));
      callback-&gt;Call(callback, 1,
                     reinterpret_cast&lt;v8::Handle&lt;v8::Value&gt;*&gt;(&amp;result));
      delete this;
    }

  private:
    std::string path;
  };

  std::string CheckArguments(const v8::Arguments&amp; arguments,
                             const std::string&amp; functionName,
                             const std::vector&lt;std::string&gt; requiredTypes)
  {
    const int requiredCount = requiredTypes.size();
    if (arguments.Length() != requiredCount)
    {
      std::stringstream stream;
      stream &lt;&lt; functionName &lt;&lt; " requires " &lt;&lt; requiredCount
             &lt;&lt; (requiredCount == 1 ? " parameter" : " parameters");
      return stream.str();
    }

    for (int i = 0; i &lt; requiredCount; i++)
    {
      const v8::Handle&lt;v8::Value&gt; argument = arguments[i];
      const std::string requiredType = requiredTypes[i];
      if ((requiredType == "string" &amp;&amp; !argument-&gt;IsString())
          || (requiredType == "number" &amp;&amp; !argument-&gt;IsNumber())
          || (requiredType == "function" &amp;&amp; !argument-&gt;IsFunction()))
      {
        std::vector&lt;std::string&gt; countWords;
        countWords.push_back("First");
        countWords.push_back("Second");
        countWords.push_back("Third");
        std::stringstream stream;
        if (i &lt; countWords.size())
          stream &lt;&lt; countWords[i] &lt;&lt; " argument";
        else
          stream &lt;&lt; "Argument " &lt;&lt; i;
        stream &lt;&lt; " to " &lt;&lt; functionName &lt;&lt; " must be a " &lt;&lt; requiredType;
        return stream.str();
      }
    }

    return "";
  }

  v8::Handle&lt;v8::Value&gt; ReadCallback(const v8::Arguments&amp; arguments)
  {
    std::vector&lt;std::string&gt; requiredTypes;
    requiredTypes.push_back("string");
    requiredTypes.push_back("function");
    const std::string error = CheckArguments(arguments, "_fileSystem.read",
                                             requiredTypes);
    if (error.length())
      return v8::ThrowException(v8::String::New(error.c_str()));

    const v8::Handle&lt;const v8::External&gt; external =
      v8::Handle&lt;const v8::External&gt;::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast&lt;FileSystem*&gt;(external-&gt;Value());
    const std::string path = *v8::String::Utf8Value(arguments[0]-&gt;ToString());
    v8::Persistent&lt;v8::Function&gt; callback = v8::Persistent&lt;v8::Function&gt;::New(
      v8::Isolate::GetCurrent(), v8::Handle&lt;v8::Function&gt;::Cast(arguments[1]));
    ReadThread* const readThread = new ReadThread(*fileSystem, callback, path);
    readThread-&gt;Start();
    return v8::Undefined();
  }

  v8::Handle&lt;v8::Value&gt; WriteCallback(const v8::Arguments&amp; arguments)
  {
    std::vector&lt;std::string&gt; requiredTypes;
    requiredTypes.push_back("string");
    requiredTypes.push_back("string");
    requiredTypes.push_back("function");
    const std::string error = CheckArguments(arguments, "_fileSystem.write",
                                             requiredTypes);
    if (error.length())
      return v8::ThrowException(v8::String::New(error.c_str()));

    const v8::Handle&lt;const v8::External&gt; external =
      v8::Handle&lt;const v8::External&gt;::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast&lt;FileSystem*&gt;(external-&gt;Value());
    const std::string path = *v8::String::Utf8Value(arguments[0]-&gt;ToString());
    const std::string content =
      *v8::String::Utf8Value(arguments[1]-&gt;ToString());
    v8::Persistent&lt;v8::Function&gt; callback = v8::Persistent&lt;v8::Function&gt;::New(
      v8::Isolate::GetCurrent(), v8::Handle&lt;v8::Function&gt;::Cast(arguments[2]));
    WriteThread* const writeThread = new WriteThread(*fileSystem, callback,
                                                     path, content);
    writeThread-&gt;Start();
    return v8::Undefined();
  }

  v8::Handle&lt;v8::Value&gt; MoveCallback(const v8::Arguments&amp; arguments)
  {
    std::vector&lt;std::string&gt; requiredTypes;
    requiredTypes.push_back("string");
    requiredTypes.push_back("string");
    requiredTypes.push_back("function");
    const std::string error = CheckArguments(arguments, "_fileSystem.move",
                                             requiredTypes);
    if (error.length())
      return v8::ThrowException(v8::String::New(error.c_str()));

    const v8::Handle&lt;const v8::External&gt; external =
      v8::Handle&lt;const v8::External&gt;::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast&lt;FileSystem*&gt;(external-&gt;Value());
    const std::string fromPath =
      *v8::String::Utf8Value(arguments[0]-&gt;ToString());
    const std::string toPath = *v8::String::Utf8Value(arguments[1]-&gt;ToString());
    v8::Persistent&lt;v8::Function&gt; callback = v8::Persistent&lt;v8::Function&gt;::New(
      v8::Isolate::GetCurrent(), v8::Handle&lt;v8::Function&gt;::Cast(arguments[2]));
    MoveThread* const moveThread = new MoveThread(*fileSystem, callback,
                                                  fromPath, toPath);
    moveThread-&gt;Start();
    return v8::Undefined();
  }

  v8::Handle&lt;v8::Value&gt; RemoveCallback(const v8::Arguments&amp; arguments)
  {
    std::vector&lt;std::string&gt; requiredTypes;
    requiredTypes.push_back("string");
    requiredTypes.push_back("function");
    const std::string error = CheckArguments(arguments, "_fileSystem.remove",
                                             requiredTypes);
    if (error.length())
      return v8::ThrowException(v8::String::New(error.c_str()));

    const v8::Handle&lt;const v8::External&gt; external =
      v8::Handle&lt;const v8::External&gt;::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast&lt;FileSystem*&gt;(external-&gt;Value());
    const std::string path = *v8::String::Utf8Value(arguments[0]-&gt;ToString());
    v8::Persistent&lt;v8::Function&gt; callback = v8::Persistent&lt;v8::Function&gt;::New(
      v8::Isolate::GetCurrent(), v8::Handle&lt;v8::Function&gt;::Cast(arguments[1]));
    RemoveThread* const removeThread = new RemoveThread(*fileSystem, callback,
                                                        path);
    removeThread-&gt;Start();
    return v8::Undefined();
  }

  v8::Handle&lt;v8::Value&gt; StatCallback(const v8::Arguments&amp; arguments)
  {
    std::vector&lt;std::string&gt; requiredTypes;
    requiredTypes.push_back("string");
    requiredTypes.push_back("function");
    const std::string error = CheckArguments(arguments, "_fileSystem.stat",
                                             requiredTypes);
    if (error.length())
      return v8::ThrowException(v8::String::New(error.c_str()));

    const v8::Handle&lt;const v8::External&gt; external =
      v8::Handle&lt;const v8::External&gt;::Cast(arguments.Data());
    FileSystem* const fileSystem = static_cast&lt;FileSystem*&gt;(external-&gt;Value());
    const std::string path = *v8::String::Utf8Value(arguments[0]-&gt;ToString());
    v8::Persistent&lt;v8::Function&gt; callback = v8::Persistent&lt;v8::Function&gt;::New(
      v8::Isolate::GetCurrent(), v8::Handle&lt;v8::Function&gt;::Cast(arguments[1]));
    StatThread* const statThread = new StatThread(*fileSystem, callback, path);
    statThread-&gt;Start();
    return v8::Undefined();
  }
}

v8::Handle&lt;v8::ObjectTemplate&gt;
FileSystemJsObject::Create(FileSystem&amp; fileSystem)
{
  const v8::Locker locker(v8::Isolate::GetCurrent());
  v8::HandleScope handleScope;
  const v8::Handle&lt;v8::ObjectTemplate&gt; file = v8::ObjectTemplate::New();
  file-&gt;Set(v8::String::New("read"),
    v8::FunctionTemplate::New(ReadCallback, v8::External::New(&amp;fileSystem)));
  file-&gt;Set(v8::String::New("write"),
    v8::FunctionTemplate::New(WriteCallback, v8::External::New(&amp;fileSystem)));
  file-&gt;Set(v8::String::New("move"),
    v8::FunctionTemplate::New(MoveCallback, v8::External::New(&amp;fileSystem)));
  file-&gt;Set(v8::String::New("remove"),
    v8::FunctionTemplate::New(RemoveCallback, v8::External::New(&amp;fileSystem)));
  file-&gt;Set(v8::String::New("stat"),
    v8::FunctionTemplate::New(StatCallback, v8::External::New(&amp;fileSystem)));
  return handleScope.Close(file);
}
