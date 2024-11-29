// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/test/helpers/assert_catcher_guard.h"
#include "nau/math/math.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/type_info.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/utils/functor.h"

using namespace testing;

namespace nau::test
{
    template <typename T>
    concept SharedStateWithClassMarker = requires(const T& state) {
        { state.m_classMarker };
    };

    struct INonRttiBase
    {
    };

    struct NonRtti1 : INonRttiBase
    {
        NAU_CLASS_BASE(INonRttiBase)
    };

    struct NonRtti2 : INonRttiBase
    {
        NAU_CLASS_BASE(INonRttiBase)
    };

    struct BaseWithRtti1 : virtual IRttiObject
    {
        NAU_INTERFACE(nau::test::BaseWithRtti1, IRttiObject)
    };

    template <typename T1, typename T2>
    concept SameAs = std::is_same_v<T2, T1>;

    struct BaseWithRtti2 : virtual IRttiObject
    {
        NAU_INTERFACE(nau::test::BaseWithRtti2, IRttiObject)
    };

    TEST(TestRttiTypeInfo, HasTypeInfo)
    {
        static_assert(!rtti::HasTypeInfo<INonRttiBase>);
        static_assert(!rtti::HasTypeInfo<NonRtti1>);
        static_assert(!rtti::HasTypeInfo<NonRtti2>);
        static_assert(rtti::HasTypeInfo<BaseWithRtti1>);
        static_assert(rtti::HasTypeInfo<BaseWithRtti2>);
    }

    TEST(TestRttiTypeInfo, GetTypeInfo)
    {
        const auto& typeInfo = rtti::getTypeInfo<BaseWithRtti1>();

        ASSERT_TRUE(!typeInfo.getTypeName().empty());
    }

    TEST(TestRttiTypeInfo, Comparison)
    {
        const auto& typeInfo1 = rtti::getTypeInfo<BaseWithRtti1>();
        const auto& typeInfo2 = rtti::getTypeInfo<BaseWithRtti2>();

        ASSERT_EQ(typeInfo1, rtti::getTypeInfo<BaseWithRtti1>());
        ASSERT_EQ(typeInfo2, rtti::getTypeInfo<BaseWithRtti2>());

        ASSERT_NE(typeInfo2, typeInfo1);
    }

    TEST(TestRttiTypeIndex, Comparison)
    {
        using namespace rtti;

        TypeIndex index1{getTypeInfo<BaseWithRtti1>()};
        TypeIndex index2{getTypeInfo<BaseWithRtti2>()};

        ASSERT_FALSE(index1 == index2);
        ASSERT_TRUE(index1 != index2);
        ASSERT_TRUE((index1 > index2) || (index2 > index1));
        ASSERT_TRUE((index1 < index2) || (index2 < index1));

        const TypeIndex index11{getTypeInfo<BaseWithRtti1>()};
    }

    TEST(TestRttiTypeIndex, AsKey)
    {
        using namespace nau::rtti;

        const std::map<TypeIndex, std::string> typeNames = {
            {TypeIndex::of<BaseWithRtti1>(), "one"},
            {TypeIndex::of<BaseWithRtti2>(), "two"}
        };

        ASSERT_EQ(std::string{"one"}, typeNames.at(TypeIndex::of<BaseWithRtti1>()));
        ASSERT_EQ(std::string{"two"}, typeNames.at(TypeIndex::of<BaseWithRtti2>()));
    }

    struct NAU_ABSTRACT_TYPE IBaseRC1 : virtual IRefCounted
    {
        NAU_INTERFACE(IBaseRC1, IRefCounted)
    };

    struct NAU_ABSTRACT_TYPE IBaseRC2 : virtual IRefCounted
    {
        NAU_INTERFACE(IBaseRC2, IRefCounted)
    };

    struct NAU_ABSTRACT_TYPE Interface1 : IBaseRC1,
                                          IBaseRC2
    {
        NAU_INTERFACE(Interface1, IBaseRC1, IBaseRC2)

        virtual void f1() = 0;
    };

    struct NAU_ABSTRACT_TYPE Interface2 : IRefCounted
    {
        NAU_INTERFACE(Interface2)

        virtual void f2() const = 0;
    };

    struct NAU_ABSTRACT_TYPE Interface3
    {
        NAU_TYPEID(Interface3);

        virtual ~Interface3() = default;
    };

    struct NotImplemented : IRefCounted
    {
        NAU_INTERFACE(NotImplemented)
    };

    class MyRttiClass final
        : public Interface1,
          public Interface2,
          public Interface3
    {
        NAU_CLASS_(nau::test::MyRttiClass, Interface1, Interface2, Interface3)

    public:
        using Callback = Functor<void()>;

        MyRttiClass() = default;

        MyRttiClass(Callback callback) :
            m_callback(std::move(callback))
        {
        }

        ~MyRttiClass()
        {
            if(m_callback)
            {
                m_callback();
            }
        }

    private:
        void f1() override
        {
        }

        void f2() const override
        {
        }

        Callback m_callback;
    };

    struct alignas(32) CustomAlignedType32
    {
        uint64_t field1[55];
        nau::math::vec3 field2[23];
    };

    /**
     */
    class CustomAlignedType : public IRefCounted
    {
        NAU_CLASS_(CustomAlignedType, IRefCounted)
    public:
        CustomAlignedType()
        {
            static_assert(alignof(CustomAlignedType) > alignof(std::max_align_t));
        }

        CustomAlignedType32 m_value;
    };

    enum class RcClassAllocationType
    {
        DefaultAllocator,
        CustomAllocator,
        InplaceMemory
    };

    template <typename T>
    decltype(auto) operator<<(std::basic_ostream<T>& stream, RcClassAllocationType type)
    {
        if(type == RcClassAllocationType::CustomAllocator)
        {
            stream << "Custom allocator";
        }
        else if(type == RcClassAllocationType::DefaultAllocator)
        {
            stream << "Default allocator";
        }
        else
        {
            stream << "Inplace mem";
        }

        return (stream);
    }

    class TestAllocator final : public IMemAllocator
    {
    public:
        void* allocate(size_t size) override
        {
            return ::malloc(size);
        }

        void* reallocate(void* ptr, size_t size) override
        {
            return ::realloc(ptr, size);
        }

        void deallocate(void* ptr) override
        {
            ::free(ptr);
        }

        size_t getSize(const void* ptr) const override
        {
            return 0;
        }

        void* allocateAligned(size_t size, size_t alignment) override
        {
            return nullptr;
        }

        void* reallocateAligned(void* ptr, size_t size, size_t alignment) override
        {
            return nullptr;
        }

        void deallocateAligned(void* ptr) override
        {
        }

        size_t getSizeAligned(const void* ptr, size_t alignment) const override
        {
            return 0;
        }

        bool isAligned(const void* ptr) const override
        {
            return false;
        }

        bool isValid(const void* ptr) const override
        {
            return false;
        }

        const char* getName() const
        {
            return nullptr;
        }

        void setName(const char* name)
        {
        }
    };

    template<typename T>
    struct MyInplaceStorage
    {
        alignas(alignof(T)) char space[rtti::InstanceStorageSize<T>];
    };

    class TestRttiClass : public ::testing::TestWithParam<RcClassAllocationType>
    {
    protected:
        template <typename T = MyRttiClass>
        Ptr<IRefCounted> createTestInstance()
        {
            const RcClassAllocationType allocationType = GetParam();

            if(allocationType == RcClassAllocationType::DefaultAllocator)
            {
                return rtti::createInstance<T, IRefCounted>();
            }

            if(allocationType == RcClassAllocationType::InplaceMemory)
            {
                static MyInplaceStorage<T> inplaceStorage;
                return rtti::createInstanceInplace<T, IRefCounted>(inplaceStorage);
            }

            NAU_ASSERT(allocationType == RcClassAllocationType::CustomAllocator);
            m_customAllocator = eastl::make_shared<TestAllocator>();

            return rtti::createInstanceWithAllocator<T, IRefCounted>(m_customAllocator);
        }

    private:
        IMemAllocator::Ptr m_customAllocator;
    };

    TEST_P(TestRttiClass, IsRefCounted)
    {
        auto itf = createTestInstance();

        ASSERT_TRUE(itf);
        ASSERT_TRUE(itf->is<IRefCounted>());
    }

    TEST_P(TestRttiClass, CastToRefCounted)
    {
        auto itf = createTestInstance();
        auto anything = itf->as<IRttiObject*>();
        ASSERT_THAT(anything, NotNull());

        auto refCounted = anything->as<IRefCounted*>();
        ASSERT_THAT(refCounted, NotNull());
    }

    TEST_P(TestRttiClass, IsAnything)
    {
        auto itf = createTestInstance();

        ASSERT_TRUE(itf->is<IRttiObject>());
        ASSERT_TRUE(itf->as<IRttiObject*>() != nullptr);
    }

    TEST_P(TestRttiClass, InterfaceAccess)
    {
        auto itf = createTestInstance();

        ASSERT_TRUE(itf->is<Interface1>());
        ASSERT_TRUE(itf->is<Interface2>());
        ASSERT_TRUE(itf->is<Interface3>());
        ASSERT_TRUE(itf->is<IBaseRC1>());
        ASSERT_TRUE(itf->is<IBaseRC2>());

        ASSERT_THAT(itf->as<Interface1*>(), NotNull());
        ASSERT_THAT(itf->as<Interface2*>(), NotNull());
        ASSERT_THAT(itf->as<Interface3*>(), NotNull());
        ASSERT_THAT(itf->as<IBaseRC1*>(), NotNull());
        ASSERT_THAT(itf->as<IBaseRC2*>(), NotNull());

        ASSERT_FALSE(itf->is<std::string>());
    }

    TEST_P(TestRttiClass, WeakReferenceNotNull)
    {
        auto itf = createTestInstance();
        IWeakRef* const weakRef = itf->getWeakRef();
        ASSERT_THAT(weakRef, NotNull());
        weakRef->releaseRef();
    }

    TEST_P(TestRttiClass, WeakReferenceNotDeadWhileInstanceAlive)
    {
        auto itf = createTestInstance();
        IWeakRef* const weakRef = itf->getWeakRef();
        ASSERT_FALSE(weakRef->isDead());
        weakRef->releaseRef();
    }

    TEST_P(TestRttiClass, WeakReferenceIsDeadAfterInstanceReleased)
    {
        IWeakRef* const weakRef = createTestInstance()->getWeakRef();
        ASSERT_TRUE(weakRef->isDead());
        weakRef->releaseRef();
    }

    TEST_P(TestRttiClass, WeakReferenceAcquire)
    {
        auto itf = createTestInstance();
        IWeakRef* const weakRef = itf->getWeakRef();
        IRefCounted* const instance = weakRef->acquire();
        ASSERT_THAT(instance, NotNull());
    }

    TEST_P(TestRttiClass, WeakReferenceAcquireNull)
    {
        IWeakRef* const weakRef = createTestInstance()->getWeakRef();
        IRefCounted* const instance = weakRef->acquire();
        ASSERT_THAT(instance, IsNull());
    }



    TEST_P(TestRttiClass, NonDefaultAlignment)
    {
        const AssertCatcherGuard assertGuard;

        nau::Ptr<CustomAlignedType> instance = createTestInstance<CustomAlignedType>();

        ASSERT_TRUE(reinterpret_cast<std::uintptr_t>(instance.get()) % alignof(CustomAlignedType) == 0);
        ASSERT_TRUE(assertGuard.assertFailureCounter == 0);
        ASSERT_TRUE(assertGuard.fatalFailureCounter == 0);
    }

    TEST_P(TestRttiClass, SharedStateAccess)
    {
        using Storage = rtti_detail::RttiClassStorage<MyRttiClass::ClassPolicy::RC>;
        using SharedState = Storage::SharedState;

        // Storage::getSharedState can be used only with class implementation
        nau::Ptr<MyRttiClass> instance = createTestInstance<MyRttiClass>();

        {
            const AssertCatcherGuard assertCatcher;
            [[maybe_unused]] SharedState& sharedState = Storage::getSharedState(*instance);
            ASSERT_EQ(assertCatcher.assertFailureCounter, 0);
        }
    }

    INSTANTIATE_TEST_SUITE_P(
        Default,
        TestRttiClass,
        testing::ValuesIn({RcClassAllocationType::DefaultAllocator, RcClassAllocationType::CustomAllocator, RcClassAllocationType::InplaceMemory}));

    TEST(TestRtti, InstanceStorageSize)
    {
        constexpr size_t TypeStorageSize = rtti::InstanceStorageSize<MyRttiClass>;
        ASSERT_THAT(TypeStorageSize, Gt(sizeof(MyRttiClass)));
    }

    TEST(TestRtti, InvalidClassConstruction)
    {
        using Storage = rtti_detail::RttiClassStorage<MyRttiClass::ClassPolicy::RC>;
        using SharedState = Storage::SharedState;

        [[maybe_unused]] MyRttiClass invalidInstance;

        if constexpr(SharedStateWithClassMarker<SharedState>)
        {
            const AssertCatcherGuard assertCatcher;

            [[maybe_unused]] SharedState& sharedState = Storage::getSharedState(invalidInstance);
            ASSERT_GT(assertCatcher.fatalFailureCounter, 0);
        }
    }

#if 0
    class MyClass1 final : public BaseWithRtti1,
                           public BaseWithRtti2,
                           public nau::IRefCounted
    {
        NAU_CLASS_(nau::test::MyClass1, BaseWithRtti1, BaseWithRtti2, nau::IRefCounted)

    public:
        ~MyClass1()
        {
        }
    };

    TEST(TestRtti, RefCountedOnlyInClass)
    {
        auto instance = rtti::createInstance<MyClass1, BaseWithRtti1>();

        nau::Ptr<BaseWithRtti2> ptr2 = instance;

        // ComPtr<
    }

    template <nau::RefCountedConcept RC>
    class TestRefCounted : public testing::Test
    {
    protected:
        RC m_counter;
    };

    using RCTypes = testing::Types<ConcurrentRC, StrictSingleThreadRC>;
    TYPED_TEST_SUITE(TestRefCounted, RCTypes);

    TYPED_TEST(TestRefCounted, InitiallyHasOneRef)
    {
        ASSERT_THAT(refsCount(this->m_counter), Eq(1));
    }

    TYPED_TEST(TestRefCounted, AddRef)
    {
        ASSERT_THAT(this->m_counter.addRef(), Eq(1));
        ASSERT_THAT(this->m_counter.addRef(), Eq(2));
    }

    TYPED_TEST(TestRefCounted, RemoveRef)
    {
        ASSERT_THAT(this->m_counter.addRef(), Eq(1));
        ASSERT_THAT(this->m_counter.removeRef(), Eq(2));
        ASSERT_THAT(this->m_counter.removeRef(), Eq(1));
    }

    TEST(Test_ConcurrentRC, Multithread)
    {
        // using namespace Runtime::Async;

        // using Counter = Runtime::ConcurrentRC;

        // constexpr size_t ThreadsCount = 20;
        // constexpr size_t IterationsCount = 10000;

        // const RuntimeScopeGuard runtimeGuard;

        // Counter counter;

        // const auto RunConcurrent = [&](auto f)
        // {
        //     std::vector<Task<>> tasks;
        //     tasks.reserve(ThreadsCount);

        //     for(size_t i = 0; i < ThreadsCount; ++i)
        //     {
        //         tasks.emplace_back(Async::Run([](Counter& counter, size_t iterationsCount, decltype(f)& f)
        //                                       {
        //                                           for(size_t x = 0; x < iterationsCount; ++x)
        //                                           {
        //                                               f(counter);
        //                                           }
        //                                       },
        //                                       nullptr, std::ref(counter), IterationsCount, std::ref(f)));
        //     }

        //     for(auto& t : tasks)
        //     {
        //         Async::Wait(t);
        //     }
        // };

        // RunConcurrent([](Counter& counter)
        //               {
        //                   counter.addRef();
        //               });

        // ASSERT_THAT(refsCount(counter), Eq(ThreadsCount * IterationsCount + 1));

        // RunConcurrent([](Counter& counter)
        //               {
        //                   counter.removeRef();
        //               });

        // ASSERT_THAT(refsCount(counter), Eq(1));
    }
#endif
}  // namespace nau::test
