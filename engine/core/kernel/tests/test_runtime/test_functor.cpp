// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/utils/functor.h"

namespace nau::test
{
    namespace
    {
        inline void ignore([[maybe_unused]] const auto&)
        {
        }

        struct MoveOnly
        {
            MoveOnly() = default;

            MoveOnly(MoveOnly&&) = default;

            MoveOnly& operator=(MoveOnly&&) = default;
        };
    }  // namespace

    TEST(TestFunctor, EmptyByDefault)
    {
        Functor<void()> f;
        ASSERT_FALSE(f);
    }

    TEST(TestFunctor, AutoDeclaration)
    {
        Functor f = [](float x, float y) -> float
        {
            return x + y;
        };

        f(1.f, 2.f);
    }

    TEST(TestFunctor, FullSignatureDeclaration)
    {
        Functor<float(float, float)> f = [](float x, float y) -> float
        {
            return x + y;
        };

        f(1.f, 2.f);
    }

    TEST(TestFunctor, MoveConstructible)
    {
        Functor f = [value = MoveOnly{}]
        {
            ignore(value);
        };
        Functor<void()> f1 = std::move(f);

        ASSERT_FALSE(f);
        ASSERT_TRUE(f1);
    }

    TEST(TestFunctor, MoveAssignable)
    {
        Functor f = [value = MoveOnly{}]()
        {
            ignore(value);
        };
        Functor<void()> f1;

        f1 = std::move(f);

        ASSERT_FALSE(f);
        ASSERT_TRUE(f1);
    }

    TEST(TestFunctor, CopyNotAllowed)
    {
        using FunctorType = Functor<void()>;

        static_assert(std::is_move_constructible_v<FunctorType>);
        static_assert(std::is_move_assignable_v<FunctorType>);
        static_assert(!std::is_copy_constructible_v<FunctorType>);
        static_assert(!std::is_copy_assignable_v<FunctorType>);
    }

    TEST(TestFunctor, AcquireAnyCallable)
    {
        const auto factory = []()
        {
            struct
            {
                int operator()(int, int)
                {
                    return 0;
                }
            } callableObject;

            return callableObject;
        };

        [[maybe_unused]]
        Functor f = factory();

        static_assert(std::is_invocable_r_v<int, decltype(f), int, int>);
    }

}  // namespace nau::test
