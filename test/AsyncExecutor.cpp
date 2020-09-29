/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-present eyeo GmbH
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

#include <future>
#include <gtest/gtest.h>

#include <AdblockPlus/AsyncExecutor.h>

using namespace AdblockPlus;

// For present gtest does not provide public API to have value- and type-
// parameterized tests, so the approach is to accumalte the common code in
// BaseAsyncExecutorTest, then define it with different types and then for each
// type instantiate value-parameterized tests and call the common test body.

template<typename Executor> struct BaseAsyncExecutorTestTraits;

typedef ::testing::tuple<uint16_t, // number of procucers
                         uint16_t  // number of tasks issued by each producer
                         >
    BaseAsyncExecutorTestParams;

template<typename TExecutor>
class BaseAsyncExecutorTest : public ::testing::TestWithParam<BaseAsyncExecutorTestParams>
{
public:
  typedef typename ::testing::tuple_element<0, ParamType>::type ProducersNumber;
  typedef typename ::testing::tuple_element<1, ParamType>::type ProducerTasksNumber;

  typedef BaseAsyncExecutorTestTraits<TExecutor> Traits;
  typedef typename Traits::Executor Executor;

protected:
  void SetUp() override
  {
    testThreadID = std::this_thread::get_id();
  }

  void MultithreadedCallsTest();

  std::thread::id testThreadID;
};

template<> struct BaseAsyncExecutorTestTraits<ActiveObject>
{
  typedef ActiveObject Executor;
  typedef std::vector<uint32_t> PayloadResults;

  static void Dispatch(typename BaseAsyncExecutorTest<Executor>::Executor& executor,
                       std::function<void()> call)
  {
    executor.Post(std::move(call));
  }
};

template<> struct BaseAsyncExecutorTestTraits<AsyncExecutor>
{
  typedef AsyncExecutor Executor;

  // The API of this class is not thread-safe! It's merely a helper to
  // generalize particular tests, don't use it anywhere else.
  class PayloadResults : public SynchronizedCollection<std::vector<uint32_t>>
  {
  public:
    typename Container::size_type size() const
    {
      return collection.size();
    }
    typename Container::iterator begin()
    {
      return collection.begin();
    }
    typename Container::iterator end()
    {
      return collection.end();
    }
    typename Container::const_reference operator[](typename Container::size_type pos) const
    {
      return collection[pos];
    }
  };

  static void Dispatch(typename BaseAsyncExecutorTest<Executor>::Executor& executor,
                       std::function<void()> call)
  {
    executor.Dispatch(std::move(call));
  }
};

template<typename Executor> void BaseAsyncExecutorTest<Executor>::MultithreadedCallsTest()
{
  typename Traits::PayloadResults results;

  std::promise<void> prepare;
  std::shared_future<void> asyncPrepared = prepare.get_future();
  const ProducersNumber producersNumber = ::testing::get<0>(GetParam());
  const ProducerTasksNumber producerTasksNumber = ::testing::get<1>(GetParam());
  const uint32_t totalTasksNumber = producersNumber * producerTasksNumber;
  {
    Executor executor;
    {
      // Defining AsyncExecutor (async) after tested Executor ensures that all
      // producers finish before destroying the Executor. Otherwise there could
      // be cases when the Executor is already destroyed but payload task is not
      // dispatched yet, what would result in a data race.
      // It seems at least strange to use AsyncExecutor in the test of
      // AsyncExecutor, but perhaps it can be considered as addition testing.
      AsyncExecutor async;
      for (ProducersNumber producer = 0; producer < producersNumber; ++producer)
      {
        auto producerTask = [producerTasksNumber, this, &executor, producer, &results](
                                std::thread::id producerThreadID) {
          for (ProducerTasksNumber task = 0; task < producerTasksNumber; ++task)
          {
            auto id = producer * producerTasksNumber + task;
            Traits::Dispatch(executor, /*payload task*/ [this, id, &results, producerThreadID] {
              results.push_back(id);
              EXPECT_NE(producerThreadID, std::this_thread::get_id());
              EXPECT_NE(testThreadID, std::this_thread::get_id());
            });
          }
        };
        async.Dispatch([this, producerTask, asyncPrepared] {
          const auto producerThreadID = std::this_thread::get_id();
          EXPECT_NE(testThreadID, producerThreadID);
          asyncPrepared.wait();
          producerTask(producerThreadID);
        });
        std::this_thread::yield();
      }
      // it's not ideal because some threads can still have not reached
      // asyncPrepared.wait() but it's good enough, in particular yield should
      // be helpful there.
      prepare.set_value();
    }
  }

  std::sort(results.begin(), results.end());
  // ensure that all tasks are executed by finding their IDs
  ASSERT_EQ(totalTasksNumber, results.size());
  for (uint16_t id = 0; id < totalTasksNumber; ++id)
  {
    EXPECT_EQ(id, results[id]);
  }
}

namespace
{

  std::string
  humanReadbleParams(const ::testing::TestParamInfo<BaseAsyncExecutorTestParams> paramInfo)
  {
    std::stringstream ss;
    ss << "producers_" << ::testing::get<0>(paramInfo.param) << "_tasks_"
       << ::testing::get<1>(paramInfo.param);
    return ss.str();
  }

  // The reason for such splitting for different implementations and different
  // platforms is the limited number of threads and time to run tests on
  // travis-ci.
  // E.g. 100 x 1000 is already too much, so the generators are split.
  // 0 - 10      1 - 100, 1000
  const auto MultithreadedCallsGenerator1 = ::testing::Combine(
      ::testing::Values(0, 1, 2, 3, 5, 10), ::testing::Values(1, 2, 3, 5, 10, 100, 1000));
  //        100, 1 - 100
  const auto MultithreadedCallsGenerator2 =
      ::testing::Combine(::testing::Values(100), ::testing::Values(1, 2, 3, 5, 10, 100));
  // 0 - 10                    2000|4000
  const auto MultithreadedCallsGenerator3 = ::testing::Combine(::testing::Values(0, 1, 2, 3, 5, 10),
#ifdef WIN32
                                                               ::testing::Values(4000)
#else
                                                               ::testing::Values(2000)
#endif
  );

  typedef BaseAsyncExecutorTest<ActiveObject> ActiveObjectTest;

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber1,
                           ActiveObjectTest,
                           MultithreadedCallsGenerator1,
                           humanReadbleParams);

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber2,
                           ActiveObjectTest,
                           MultithreadedCallsGenerator2,
                           humanReadbleParams);

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber3,
                           ActiveObjectTest,
                           MultithreadedCallsGenerator3,
                           humanReadbleParams);

  TEST_P(ActiveObjectTest, MultithreadedCalls)
  {
    MultithreadedCallsTest();
  }

  typedef BaseAsyncExecutorTest<AsyncExecutor> AsyncExecutorTest;

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber1,
                           AsyncExecutorTest,
                           MultithreadedCallsGenerator1,
                           humanReadbleParams);

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber2,
                           AsyncExecutorTest,
                           MultithreadedCallsGenerator2,
                           humanReadbleParams);

  INSTANTIATE_TEST_SUITE_P(DifferentProducersNumber3,
                           AsyncExecutorTest,
                           MultithreadedCallsGenerator3,
                           humanReadbleParams);

  TEST_P(AsyncExecutorTest, MultithreadedCalls)
  {
    MultithreadedCallsTest();
  }
}