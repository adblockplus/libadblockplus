#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  void CallRun(Thread* thread)
  {
    thread->Run();
  }
}

Thread::Mutex::Mutex()
{
#ifdef WIN32
  InitializeCriticalSection(&nativeMutex);
#else
  pthread_mutex_init(&nativeMutex, 0);
#endif
}

Thread::Mutex::~Mutex()
{
  Unlock();
#ifdef WIN32
  DeleteCriticalSection(&nativeMutex);
#else
  pthread_mutex_destroy(&nativeMutex);
#endif
}

void Thread::Mutex::Lock()
{
#ifdef WIN32
  EnterCriticalSection(&nativeMutex);
#else
  pthread_mutex_lock(&nativeMutex);
#endif
}

void Thread::Mutex::Unlock()
{
#ifdef WIN32
  LeaveCriticalSection(&nativeMutex);
#else
  pthread_mutex_unlock(&nativeMutex);
#endif
}

Thread::Condition::Condition()
{
#ifdef WIN32
  InitializeConditionVariable(&nativeCondition);
#else
  pthread_cond_init(&nativeCondition, 0);
#endif
}

Thread::Condition::~Condition()
{
#ifndef WIN32
  Signal();
  pthread_cond_destroy(&nativeCondition);
#endif
}

void Thread::Condition::Wait(Thread::Mutex& mutex)
{
#ifdef WIN32
  SleepConditionVariableCS(&nativeCondition, &mutex.nativeMutex, INFINITE);
#else
  pthread_cond_wait(&nativeCondition, &mutex.nativeMutex);
#endif
}

void Thread::Condition::Signal()
{
#ifdef WIN32
  WakeConditionVariable(&nativeCondition);
#else
  pthread_cond_signal(&nativeCondition);
#endif
}

Thread::~Thread()
{
}

void Thread::Start()
{
#ifdef WIN32
  nativeThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&CallRun, this, 0, 0);
#else
  pthread_create(&nativeThread, 0, (void *(*)(void*)) &CallRun, this);
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
