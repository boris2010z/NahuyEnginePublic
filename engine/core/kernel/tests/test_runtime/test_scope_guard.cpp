// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/diag/error.h"
#include "nau/utils/scope_guard.h"

namespace nau::test
{

    TEST(TestScopeGuard, ScopeLeave)
    {
        bool leave11 = false;
        bool leave21 = false;
        bool leave22 = false;

        SCOPE_ON_LEAVE
        {
            leave11 = true;
        };

        ASSERT_FALSE(leave11);

        {
            SCOPE_ON_LEAVE
            {
                leave21 = true;
            };
            SCOPE_ON_LEAVE
            {
                leave22 = true;
            };

            ASSERT_FALSE(leave21);
            ASSERT_FALSE(leave22);
        }

        ASSERT_TRUE(leave21);
        ASSERT_TRUE(leave22);
    }

    TEST(TestScopeGuard, ScopeFailure)
    {
        bool leave = false;
        bool failure = false;
        bool success = false;
        bool neverBeHere = false;

        const auto throwInScope = [&]
        {
            SCOPE_ON_LEAVE
            {
                leave = true;
            };
            SCOPE_ON_FAIL
            {
                failure = true;
            };
            SCOPE_ON_SUCCESS
            {
                success = true;
            };

            throw nau::DefaultError{{}, "test_fail"};

            neverBeHere = true;
        };

        try
        {
            throw nau::DefaultError{{}, "test_fail"};
        }
        catch(const std::exception&)
        {
        }

        try
        {
            throwInScope();
        }
        catch(const std::exception&)
        {
        }

        ASSERT_TRUE(leave);
        ASSERT_TRUE(failure);
        ASSERT_FALSE(success);
        ASSERT_FALSE(neverBeHere);
    }

    TEST(TestScopeGuard, ScopeSuccess)
    {
        bool leave = false;
        bool failure = false;
        bool success = false;

        {
            SCOPE_ON_LEAVE
            {
                leave = true;
            };
            SCOPE_ON_FAIL
            {
                failure = true;
            };
            SCOPE_ON_SUCCESS
            {
                success = true;
            };
        }

        ASSERT_TRUE(leave);
        ASSERT_FALSE(failure);
        ASSERT_TRUE(success);
    }

    TEST(TestScopeGuard, ScopeNestedException)
    {
    }

}  // namespace nau::test
