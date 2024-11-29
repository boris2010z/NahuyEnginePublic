//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//
#include "nau/threading/thread_local_value.h"
#include <unordered_set>
#include <chrono>
#include <mutex>

namespace nau::test
{
    TEST(TestThreadLocalValue, ConstructorDestructor)
    {
        ThreadLocalValue<int> value;
        value.value() = 0;
        ASSERT_EQ(value.value(), 0);
        ThreadLocalValue<int> value2(std::move(value));
        ASSERT_EQ(value2.value(), 0);
    }

    struct TestValue
    {
        TestValue()
        {
            std::lock_guard lock(mutex);
            ++count;
        }

        ~TestValue()
        {
            std::lock_guard lock(mutex);
            --count;
        }
        static std::mutex mutex;
        static int count;
    };
    int TestValue::count = 0;
    std::mutex TestValue::mutex;

    TEST(TestThreadLocalValue, Destroy)
    {
        TestValue::count = 0;
        ThreadLocalValue<TestValue> value;
        auto& unused = value.value();
        value.destroy();
        ASSERT_EQ(TestValue::count, 0);
    }

    TEST(TestThreadLocalValue, MultipleThreads)
    {
        std::unordered_set<void*> ptrs;
        ThreadLocalValue<int> value;
        std::thread thread1([&]() 
            { 
                ptrs.emplace( &value.value()); 
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        std::thread thread2([&]() 
            { 
                ptrs.emplace( &value.value()); 
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        thread1.join();
        thread2.join();
        ASSERT_EQ(ptrs.size(), 2);
    }

    TEST(TestThreadLocalValue, MultipleThreadsDestroy)
    {
        TestValue::count = 0;
        ThreadLocalValue<TestValue> value;
        std::thread thread1([&value]
        { 
            auto& unused = value.value();
            value.destroy();
        });
        
        std::thread thread2([&value]
        { 
            auto& unused = value.value();
            value.destroy(); 
        });

        thread1.join();
        thread2.join();
        ASSERT_EQ(TestValue::count, 0);
    }

    TEST(TestThreadLocalValue, MultipleThreadsDestAll)
    {
        TestValue::count = 0;
        ThreadLocalValue<TestValue> value;
        std::thread thread1([&value]
        { 
            auto& unused = value.value();
        });
        
        std::thread thread2([&value]
        { 
            auto& unused = value.value();
        });

        thread1.join();
        thread2.join();

        value.destroyAll();
        ASSERT_EQ(TestValue::count, 0);
    }

    TEST(TestThreadLocalValue, MultipleThreadsReuse)
    {
        std::mutex mutex;
        std::unordered_set<void*> ptrs;
        std::vector<std::thread> threads;
        ThreadLocalValue<int> value;
        for(int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            threads.push_back(
                std::thread([&]() 
                { 
                    ptrs.emplace( &value.value()); 
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                })
            );
        }
        for(auto &t : threads) 
        { 
            t.join(); 
        }
        threads.clear();
        auto curSize = ptrs.size();

        for(int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            threads.push_back(
                std::thread([&]() 
                { 
                    ptrs.emplace( &value.value()); 
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                })
            );
        }
        for (auto& t : threads)
        {
            t.join();
        }

        ASSERT_EQ(ptrs.size(), curSize);
    }

}