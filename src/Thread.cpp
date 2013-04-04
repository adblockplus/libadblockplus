#include "Thread.h"

using namespace AdblockPlus;

namespace
{
  void CallRun(Thread* thread)
  {
    thread->Run();
  }
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

ConditionVariable::ConditionVariable()
{
#ifdef WIN32
  InitializeConditionVariable(&nativeCondition);
#else
  pthread_cond_init(&nativeCondition, 0);
#endif
}

ConditionVariable::~ConditionVariable()
{
#ifndef WIN32
  pthread_cond_destroy(&nativeCondition);
#endif
}

void ConditionVariable::Wait(Mutex& mutex)
{
#ifdef WIN32
  SleepConditionVariableCS(&nativeCondition, &mutex.nativeMutex, INFINITE);
#else
  pthread_cond_wait(&nativeCondition, &mutex.nativeMutex);
#endif
}

void ConditionVariable::Signal()
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
