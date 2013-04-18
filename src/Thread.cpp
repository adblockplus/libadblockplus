#ifndef WIN32
#include <unistd.h>
#endif

#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  void CallRun(Thread* thread)
  {
    thread->Run();
  }
}

void AdblockPlus::Sleep(const int millis)
{
#ifdef WIN32
  ::Sleep(millis);
#else
  usleep(millis * 1000);
#endif
}

Mutex::Mutex()
{
#ifdef WIN32
  InitializeCriticalSection(&nativeMutex);
#else
  pthread_mutex_init(&nativeMutex, 0);
#endif
}

Mutex::~Mutex()
{
#ifdef WIN32
  DeleteCriticalSection(&nativeMutex);
#else
  pthread_mutex_destroy(&nativeMutex);
#endif
}

void Mutex::Lock()
{
#ifdef WIN32
  EnterCriticalSection(&nativeMutex);
#else
  pthread_mutex_lock(&nativeMutex);
#endif
}

void Mutex::Unlock()
{
#ifdef WIN32
  LeaveCriticalSection(&nativeMutex);
#else
  pthread_mutex_unlock(&nativeMutex);
#endif
}

Lock::Lock(Mutex& mutex) : mutex(mutex)
{
  mutex.Lock();
}

Lock::~Lock()
{
  mutex.Unlock();
}

Thread::~Thread()
{
}

void Thread::Start()
{
#ifdef WIN32
  nativeThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&CallRun, this, 0, 0);
#else
  pthread_create(&nativeThread, 0, (void* (*)(void*)) &CallRun, this);
#endif
}

void Thread::Join()
{
#ifdef WIN32
  WaitForSingleObject(nativeThread, INFINITE);
#else
  pthread_join(nativeThread, 0);
#endif
}
