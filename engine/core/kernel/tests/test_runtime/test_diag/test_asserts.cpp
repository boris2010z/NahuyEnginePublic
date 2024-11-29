// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "test_diag.h"

namespace nau::test
{
    class Test_Asserts : public ::testing::Test
    {
    protected:
        static inline constexpr bool SuccessFlag = true;
        static inline constexpr bool FailureFlag = false;

        Test_Asserts()
        {
            diag::setDeviceError(eastl::make_unique<AssertTestDeviceError>());
        }

        ~Test_Asserts()
        {
            diag::setDeviceError(nullptr);
        }

        static AssertTestDeviceError& getTestDeviceError()
        {
            return diag::getDeviceError()->as<AssertTestDeviceError&>();
        }
    };

    TEST_F(Test_Asserts, DefaultAssert)
    {
#ifndef NAU_ASSERT_ENABLED
        GTEST_SKIP_("ASSERT Disabled");
#endif
        NAU_ASSERT(SuccessFlag);
        ASSERT_TRUE(getTestDeviceError().hasNoErrors());

        NAU_ASSERT(FailureFlag);
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 1);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);

        NAU_ASSERT(FailureFlag, "Test Assertion");
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 2);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);

        NAU_FAILURE();
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 3);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);
    }

    TEST_F(Test_Asserts, FatalAssert)
    {
#ifndef NAU_ASSERT_ENABLED
        GTEST_SKIP_("ASSERT Disabled");
#endif
        NAU_FATAL(FailureFlag);
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 0);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 1);

        NAU_FATAL(FailureFlag, u8"Test Assertion");
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 0);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 2);
    }

    TEST_F(Test_Asserts, CombinedFailure)
    {
#ifndef NAU_ASSERT_ENABLED
        GTEST_SKIP_("ASSERT Disabled");
#endif
        NAU_FAILURE();
        NAU_ASSERT(FailureFlag);
        NAU_ASSERT(SuccessFlag);
        NAU_FATAL(FailureFlag);
        NAU_FATAL(SuccessFlag);
        NAU_FATAL_FAILURE();

        ASSERT_EQ(getTestDeviceError().getErrorCount(), 2);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 2);
    }

    TEST_F(Test_Asserts, DefaultVerify)
    {
#ifndef NAU_ASSERT_ENABLED
        GTEST_SKIP_("ASSERT Disabled");
#endif
        NAU_VERIFY(SuccessFlag);
        ASSERT_TRUE(getTestDeviceError().hasNoErrors());

        NAU_VERIFY(FailureFlag);
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 1);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);

        NAU_VERIFY(FailureFlag, u8"Test Assertion");
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 2);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);

        NAU_FAILURE_ALWAYS();
        ASSERT_EQ(getTestDeviceError().getErrorCount(), 3);
        ASSERT_EQ(getTestDeviceError().getFatalErrorCount(), 0);
    }

    void testAssertReturn(uint32_t& counter)
    {
        NAU_ASSERT_RETURN(false);
        counter++;
    }

    TEST_F(Test_Asserts, BreakContinueReturn)
    {
#ifndef NAU_ASSERT_ENABLED
        GTEST_SKIP_("ASSERT Disabled");
#endif
        NAU_VERIFY(SuccessFlag);
        ASSERT_TRUE(getTestDeviceError().hasNoErrors());
        uint32_t counter = 0;
        for (int i = 0; i < 10; i++)
        {
            counter++;
            NAU_ASSERT_BREAK(FailureFlag);
            NAU_VERIFY(FailureFlag);
        }
        EXPECT_EQ(counter, 1);
        EXPECT_EQ(getTestDeviceError().getErrorCount(), 1);
        EXPECT_EQ(getTestDeviceError().getFatalErrorCount(), 0);
        counter = 0;
        for (int i = 0; i < 10; i++)
        {
            counter++;
            NAU_ASSERT_CONTINUE(FailureFlag);
            NAU_VERIFY(FailureFlag);
        }
        EXPECT_EQ(counter, 10);
        EXPECT_EQ(getTestDeviceError().getErrorCount(), 11);
        EXPECT_EQ(getTestDeviceError().getFatalErrorCount(), 0);
        counter = 0;
        testAssertReturn(counter);
        EXPECT_EQ(counter, 0);
        EXPECT_EQ(getTestDeviceError().getErrorCount(), 12);
        EXPECT_EQ(getTestDeviceError().getFatalErrorCount(), 0);
    }

}  // namespace nau::test