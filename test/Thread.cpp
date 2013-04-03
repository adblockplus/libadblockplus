#include <gtest/gtest.h>
#include <queue>
#include <vector>

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
    AdblockPlus::Thread::Mutex mutex;

    Mock() : timesCalled(0)
    {
    }

    void Run()
    {
      Sleep(5);
      mutex.Lock();
      timesCalled++;
      mutex.Unlock();
    }
  };

  class LockingMock : public AdblockPlus::Thread
  {
  public:
    bool working;

    LockingMock(const std::string& name, std::vector<std::string>& log,
                AdblockPlus::Thread::Mutex& logMutex)
      : name(name), log(log), logMutex(logMutex)
    {
    }

    void Run()
    {
      logMutex.Lock();
      log.push_back(name);
      logMutex.Unlock();
    }

  private:
    const std::string name;
    std::vector<std::string>& log;
    AdblockPlus::Thread::Mutex& logMutex;
  };

  class Enqueuer : public AdblockPlus::Thread
  {
  public:
    Enqueuer(std::queue<int>& queue, AdblockPlus::Thread::Mutex& queueMutex,
             AdblockPlus::Thread::Condition& notEmpty)
      : queue(queue), queueMutex(queueMutex), notEmpty(notEmpty)
    {
    }

    void Run()
    {
      queueMutex.Lock();
      queue.push(1);
      notEmpty.Signal();
      queueMutex.Unlock();
    }

  private:
    std::queue<int>& queue;
    AdblockPlus::Thread::Mutex& queueMutex;
    AdblockPlus::Thread::Condition& notEmpty;
  };

  class Dequeuer : public AdblockPlus::Thread
  {
  public:
    Dequeuer(std::queue<int>& queue, AdblockPlus::Thread::Mutex& queueMutex,
             AdblockPlus::Thread::Condition& notEmpty)
      : queue(queue), queueMutex(queueMutex), notEmpty(notEmpty)
    {
    }

    void Run()
    {
      queueMutex.Lock();
      if (!queue.size())
        notEmpty.Wait(queueMutex);
      queue.pop();
      queueMutex.Unlock();
    }

  private:
    std::queue<int>& queue;
    AdblockPlus::Thread::Mutex& queueMutex;
    AdblockPlus::Thread::Condition& notEmpty;
  };
}

TEST(ThreadTest, Run)
{
  Mock mock;
  ASSERT_EQ(0, mock.timesCalled);
  mock.Start();
  mock.mutex.Lock();
  ASSERT_EQ(0, mock.timesCalled);
  mock.mutex.Unlock();
  mock.Join();
  ASSERT_EQ(1, mock.timesCalled);
}

TEST(ThreadTest, Mutex)
{
  std::vector<std::string> log;
  AdblockPlus::Thread::Mutex logMutex;
  LockingMock mock1("mock1", log, logMutex);
  LockingMock mock2("mock2", log, logMutex);
  mock1.Start();
  Sleep(5);
  mock2.Start();
  mock1.Join();
  mock2.Join();
  ASSERT_EQ("mock1", log[0]);
  ASSERT_EQ("mock2", log[1]);
}

TEST(ThreadTest, ConditionVariable)
{
  std::queue<int> queue;
  AdblockPlus::Thread::Mutex queueMutex;
  AdblockPlus::Thread::Condition notEmpty;
  Dequeuer dequeuer(queue, queueMutex, notEmpty);
  Enqueuer enqueuer(queue, queueMutex, notEmpty);
  dequeuer.Start();
  Sleep(5);
  enqueuer.Start();
  enqueuer.Join();
  dequeuer.Join();
  ASSERT_EQ(0, queue.size());
}
