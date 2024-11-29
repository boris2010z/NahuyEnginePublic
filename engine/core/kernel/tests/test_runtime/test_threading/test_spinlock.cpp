// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/test/helpers/assert_catcher_guard.h"
#include "nau/threading/barrier.h"
#include "nau/threading/lock_guard.h"
#include "nau/threading/spin_lock.h"

namespace nau::test
{
    /**
        Test: spin lock multithread access.
     */
    TEST(TestSpinLock, MultithreadAccess)
    {
        constexpr size_t ThreadCount = 10;
        constexpr size_t IterationPerThread = 2000;

        std::vector<std::thread> threads;
        threads.reserve(ThreadCount);

        threading::Barrier barrier(ThreadCount);
        threading::SpinLock mutex;

        unsigned counter = 0;

        for (size_t i = 0; i < ThreadCount; ++i)
        {
            threads.emplace_back([&]
            {
                barrier.enter();

                for (unsigned i = 0; i < IterationPerThread; ++i)
                {
                    lock_(mutex);
                    ++counter;
                }
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        ASSERT_EQ(counter, ThreadCount * IterationPerThread);
    }

    /**
        Test: SpinLock::lock() cannot be called consecutively multiple times and must cause an assert violation.
     */
    TEST(TestSpinLock, MultipleSubsequentLockMustFail)
    {
        AssertCatcherGuard assertGuard;

        threading::SpinLock mutex;
        mutex.lock();
        ASSERT_EQ(assertGuard.fatalFailureCounter, 0);

        mutex.lock();
        ASSERT_GT(assertGuard.fatalFailureCounter, 0);
    }

    /**
     */
    TEST(TestSpinLock, DestructWhileLockedMustFail)
    {
        AssertCatcherGuard assertGuard;
        {
            threading::SpinLock mutex;
            mutex.lock();
        }
        ASSERT_GT(assertGuard.fatalFailureCounter, 0);
    }

    /**
          Test: recursive spin lock multithread access.
    */
    TEST(TestRecursiveSpinLock, MultithreadAccess)
    {
        constexpr size_t ThreadCount = 10;
        constexpr size_t IterationPerThread = 200;
        constexpr size_t LocksPerIteration = 3;

        std::vector<std::thread> threads;
        threads.reserve(ThreadCount);

        threading::Barrier barrier(ThreadCount);
        threading::RecursiveSpinLock mutex;

        unsigned counter = 0;

        for (size_t i = 0; i < ThreadCount; ++i)
        {
            threads.emplace_back([&]
            {
                barrier.enter();

                for (unsigned i = 0; i < IterationPerThread; ++i)
                {
                    for (size_t j = 0; j < LocksPerIteration; ++j)
                    {
                        mutex.lock();
                    }

                    ++counter;

                    for (size_t j = 0; j < LocksPerIteration; ++j)
                    {
                        mutex.unlock();
                    }
                }
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        ASSERT_EQ(counter, ThreadCount * IterationPerThread);
    }

    /**
        Test: RecursiveSpinLock::lock() can be called consecutively multiple times and must NOT cause an assert violation.
     */
    TEST(TestRecursiveSpinLock, MultipleLocks)
    {
        AssertCatcherGuard assertGuard;

        threading::RecursiveSpinLock mutex;
        lock_(mutex);
        lock_(mutex);
        lock_(mutex);

        ASSERT_EQ(assertGuard.fatalFailureCounter, 0);
    }

    /**
     */
    TEST(TestRecursiveSpinLock, DestructWhileLockedMustFail)
    {
        AssertCatcherGuard assertGuard;
        {
            threading::RecursiveSpinLock mutex;
            mutex.lock();
        }
        ASSERT_GT(assertGuard.fatalFailureCounter, 0);
    }
}  // namespace nau::test
