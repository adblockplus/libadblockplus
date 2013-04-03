#ifndef ADBLOCKPLUS_THREAD_H
#define ADBLOCKPLUS_THREAD_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AdblockPlus
{
  class Thread
  {
  public:
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

    class Condition
    {
    public:
      Condition();
      ~Condition();
      void Wait(Mutex& mutex);
      void Signal();

    private:
#ifdef WIN32
      CONDITION_VARIABLE nativeCondition;
#else
      pthread_cond_t nativeCondition;
#endif
    };

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
