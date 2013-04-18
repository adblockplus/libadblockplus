#ifndef ADBLOCKPLUS_THREAD_H
#define ADBLOCKPLUS_THREAD_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AdblockPlus
{
  void Sleep(const int millis);

  class Mutex
  {
  public:
#ifdef WIN32
    CRITICAL_SECTION nativeMutex;
#else
    pthread_mutex_t nativeMutex;
#endif

    Mutex();
    ~Mutex();
    void Lock();
    void Unlock();
  };

  class Lock
  {
  public:
    Lock(Mutex& mutex);
    ~Lock();

  private:
    Mutex& mutex;
  };

  class Thread
  {
  public:
    virtual ~Thread();
    virtual void Run() = 0;
    void Start();
    void Join();

  private:
#ifdef WIN32
    HANDLE nativeThread;
#else
    pthread_t nativeThread;
#endif
  };
}

#endif
