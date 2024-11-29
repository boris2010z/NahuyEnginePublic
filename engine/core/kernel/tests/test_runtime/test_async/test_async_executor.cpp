// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


//#include "osApiWrappers/dag_cpuJobs.h"
#include "nau/async/thread_pool_executor.h"
//#include "util/dag_threadPool.h"


namespace nau::test
{
    using namespace testing;
    using namespace std::chrono_literals;

    using ExecutorFactory = async::Executor::Ptr (*)();

    /**
     */
    class TestAsyncExecutor : public testing::Test,
                              public testing::WithParamInterface<ExecutorFactory>
    {
    protected:
        static constexpr size_t ThreadsCount = 8;

        auto createExecutor() const
        {
            auto factory = GetParam();
            return factory();
        }

        static void waitWorks(async::Executor::Ptr executor)
        {
            executor->waitAnyActivity();
        }
    };

    /**
     */
    TEST_P(TestAsyncExecutor, Execute)
    {
        constexpr size_t JobsCount = 200'000;

        std::atomic_size_t counter = 0;

        auto executor = createExecutor();

        for(size_t i = 0; i < JobsCount; ++i)
        {
            executor->execute([](void* counterPtr, void*) noexcept
                            {
                                reinterpret_cast<std::atomic_size_t*>(counterPtr)->fetch_add(1);
                            },
                            &counter);
        }

        waitWorks(executor);

        ASSERT_THAT(counter, Eq(JobsCount));
    }

    const ExecutorFactory createDefaultPoolExecutor = []
    {
        return async::createThreadPoolExecutor();
    };

    const ExecutorFactory createDagPoolExecutor = []
    {
        //return async::createDagThreadPoolExecutor(true);
        return async::createThreadPoolExecutor();
    };

    INSTANTIATE_TEST_SUITE_P(Default,
                             TestAsyncExecutor,
                             testing::Values(createDefaultPoolExecutor, createDagPoolExecutor));

}  // namespace nau::test
