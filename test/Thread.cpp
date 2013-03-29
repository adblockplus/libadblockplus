#include <gtest/gtest.h>
#include <queue>

#include "../src/Thread.h"

namespace
{
#ifndef WIN32
  void Sleep(const int millis)
  {
      usleep(millis * 1000);
  }
#endif

  class Mock : public AdblockPlus::Thread
  {
  public:
    int timesCalled;

    Mock() : timesCalled(0)
    {
    }

    void Run()
    {
      timesCalled++;
    }
  };

  class LockingMock : public AdblockPlus::Thread
  {
  public:
    bool working;

    LockingMock(AdblockPlus::Thread::Mutex& mutex, const int millisToSleep)
      : mutex(mutex), millisToSleep(millisToSleep)
    {
    }

    void Run()
    {
      mutex.Lock();
      working = true;
      Sleep(millisToSleep);
      working = false;
      mutex.Unlock();
    }

  private:
    AdblockPlus::Thread::Mutex& mutex;
    int millisToSleep;
  };

  class Enqueuer : public AdblockPlus::Thread
  {
  public:
    Enqueuer(std::queue<int>& queue, AdblockPlus::Thread::Mutex& mutex,
             AdblockPlus::Thread::Condition& notEmpty)
      : queue(queue), mutex(mutex), notEmpty(notEmpty)
    {
    }

    void Run()
    {
      Sleep(5);
      mutex.Lock();
      queue.push(1);
      Sleep(10);
      notEmpty.Signal();
      mutex.Unlock();
    }

  private:
    std::queue<int>& queue;
    AdblockPlus::Thread::Mutex& mutex;
    AdblockPlus::Thread::Condition& notEmpty;
  };

  class Dequeuer : public AdblockPlus::Thread
  {
  public:
    Dequeuer(std::queue<int>& queue, AdblockPlus::Thread::Mutex& mutex,
             AdblockPlus::Thread::Condition& notEmpty)
      : queue(queue), mutex(mutex), notEmpty(notEmpty)
    {
    }

    void Run()
    {
      mutex.Lock();
      if (!queue.size())
        notEmpty.Wait(mutex);
      queue.pop();
      mutex.Unlock();
    }

  private:
    std::queue<int>& queue;
    AdblockPlus::Thread::Mutex& mutex;
    AdblockPlus::Thread::Condition& notEmpty;
  };
}

TEST(ThreadTest, Run)
{
  Mock mock;
  ASSERT_EQ(0, mock.timesCalled);
  mock.Start();
  ASSERT_EQ(0, mock.timesCalled);
  mock.Join();
  ASSERT_EQ(1, mock.timesCalled);
}

TEST(ThreadTest, Mutex)
{
  AdblockPlus::Thread::Mutex mutex;
  LockingMock mock1(mutex, 10);
  LockingMock mock2(mutex, 20);
  mock1.Start();
  mock2.Start();
  Sleep(5);
  ASSERT_TRUE(mock1.working);
  ASSERT_FALSE(mock2.working);
  Sleep(10);
  ASSERT_TRUE(mock2.working);
  ASSERT_FALSE(mock1.working);
  mock1.Join();
  mock2.Join();
}

TEST(ThreadTest, ConditionVariable)
{
  std::queue<int> queue;
  AdblockPlus::Thread::Mutex mutex;
  AdblockPlus::Thread::Condition notEmpty;
  Enqueuer enqueuer(queue, mutex, notEmpty);
  Dequeuer dequeuer(queue, mutex, notEmpty);
  enqueuer.Start();
  dequeuer.Start();
  Sleep(10);
  ASSERT_EQ(1, queue.size());
  Sleep(15);
  ASSERT_EQ(0, queue.size());
  enqueuer.Join();
  dequeuer.Join();
}
