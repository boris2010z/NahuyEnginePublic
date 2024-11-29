// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/math/math.h"
#include "nau/serialization/json_utils.h"

using namespace ::testing;

namespace nau::test
{
    /**
     */
    TEST(TestSerializationMath, RepresentVec2AsRuntimeValue)
    {
        using VecType = math::vec2;

        static_assert(HasRuntimeValueRepresentation<VecType>);

        {
            VecType vec1 = {1.0F, 1.0F};
            auto runtimeVec1 = makeValueRef(vec1);
            ASSERT_TRUE(runtimeVec1->isMutable());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyDictionary>());
            ASSERT_TRUE(runtimeVec1->is<IRuntimeValueEvents>());
        }

        {
            const VecType vec2 = {1.0F, 1.0F};
            auto runtimeVec2 = makeValueRef(vec2);
            ASSERT_FALSE(runtimeVec2->isMutable());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec3 = {1.0F, 1.0F};
            auto runtimeVec3 = makeValueCopy(vec3);
            ASSERT_TRUE(runtimeVec3->isMutable());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyDictionary>());
        }

        static_assert(HasRuntimeValueRepresentation<math::vec2>);
    }

    /**
     */
    TEST(TestSerializationMath, RepresentVec3AsRuntimeValue)
    {
        using VecType = math::vec3;

        static_assert(HasRuntimeValueRepresentation<VecType>);

        {
            VecType vec1 = VecType::one();
            auto runtimeVec1 = makeValueRef(vec1);
            ASSERT_TRUE(runtimeVec1->isMutable());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyDictionary>());
            ASSERT_TRUE(runtimeVec1->is<IRuntimeValueEvents>());
        }

        {
            const VecType vec2 = VecType::one();
            auto runtimeVec2 = makeValueRef(vec2);
            ASSERT_FALSE(runtimeVec2->isMutable());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec3 = VecType::one();
            auto runtimeVec3 = makeValueCopy(vec3);
            ASSERT_TRUE(runtimeVec3->isMutable());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyDictionary>());
        }

        static_assert(HasRuntimeValueRepresentation<math::vec3>);
    }

    /**
     */
    TEST(TestSerializationMath, RepresentVec4AsRuntimeValue)
    {
        using VecType = math::vec4;

        static_assert(HasRuntimeValueRepresentation<VecType>);

        {
            VecType vec1 = VecType::one();
            auto runtimeVec1 = makeValueRef(vec1);
            ASSERT_TRUE(runtimeVec1->isMutable());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec2 = VecType::one();
            auto runtimeVec2 = makeValueRef(vec2);
            ASSERT_FALSE(runtimeVec2->isMutable());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec3 = VecType::one();
            auto runtimeVec3 = makeValueCopy(vec3);
            ASSERT_TRUE(runtimeVec3->isMutable());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyDictionary>());
        }
    }

    /**
     */
    TEST(TestSerializationMath, RepresentQuatAsRuntimeValue)
    {
        using VecType = math::Quat;

        static_assert(HasRuntimeValueRepresentation<VecType>);

        {
            VecType vec1 = VecType::identity();
            auto runtimeVec1 = makeValueRef(vec1);
            ASSERT_TRUE(runtimeVec1->isMutable());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec1->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec2 = VecType::identity();
            auto runtimeVec2 = makeValueRef(vec2);
            ASSERT_FALSE(runtimeVec2->isMutable());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec2->is<RuntimeReadonlyDictionary>());
        }

        {
            const VecType vec3 = VecType::identity();
            auto runtimeVec3 = makeValueCopy(vec3);
            ASSERT_TRUE(runtimeVec3->isMutable());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeVec3->is<RuntimeReadonlyDictionary>());
        }
    }

    /**
     */
    TEST(TestSerializationMath, RepresentMat3AsRuntimeValue)
    {
        using MatType = math::mat3;

        static_assert(HasRuntimeValueRepresentation<MatType>);

        {
            MatType mat1 = MatType::identity();
            auto runtimeMat1 = makeValueRef(mat1);
            ASSERT_TRUE(runtimeMat1->isMutable());
            ASSERT_TRUE(runtimeMat1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat1->is<RuntimeReadonlyDictionary>());
            ASSERT_TRUE(runtimeMat1->is<IRuntimeValueEvents>());
        }

        {
            const MatType mat2 = MatType::identity();
            auto runtimeMat2 = makeValueRef(mat2);
            ASSERT_FALSE(runtimeMat2->isMutable());
            ASSERT_TRUE(runtimeMat2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat2->is<RuntimeReadonlyDictionary>());
        }

        {
            const MatType mat3 = MatType::identity();
            auto runtimeMat3 = makeValueCopy(mat3);
            ASSERT_TRUE(runtimeMat3->isMutable());
            ASSERT_TRUE(runtimeMat3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat3->is<RuntimeReadonlyDictionary>());
        }
    }

    /**
     */
    TEST(TestSerializationMath, RepresentMat4AsRuntimeValue)
    {
        using MatType = math::mat4;

        static_assert(HasRuntimeValueRepresentation<MatType>);

        {
            MatType mat1 = MatType::identity();
            auto runtimeMat1 = makeValueRef(mat1);
            ASSERT_TRUE(runtimeMat1->isMutable());
            ASSERT_TRUE(runtimeMat1->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat1->is<RuntimeReadonlyDictionary>());
            ASSERT_TRUE(runtimeMat1->is<IRuntimeValueEvents>());
        }

        {
            const MatType mat2 = MatType::identity();
            auto runtimeMat2 = makeValueRef(mat2);
            ASSERT_FALSE(runtimeMat2->isMutable());
            ASSERT_TRUE(runtimeMat2->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat2->is<RuntimeReadonlyDictionary>());
        }

        {
            const MatType mat3 = MatType::identity();
            auto runtimeMat3 = makeValueCopy(mat3);
            ASSERT_TRUE(runtimeMat3->isMutable());
            ASSERT_TRUE(runtimeMat3->is<RuntimeReadonlyCollection>());
            ASSERT_TRUE(runtimeMat3->is<RuntimeReadonlyDictionary>());
        }
    }

    /**
     */
    TEST(TestSerializationMath, Mat3SerializationDeserialization)
    {
        using MatType = math::mat3;
        using VecType = math::vec3;

        static_assert(HasRuntimeValueRepresentation<MatType>);

        const VecType col0 = {1.0F, 2.0F, 3.0F};
        const VecType col1 = {0.4F, 0.5F, 0.6F};
        const VecType col2 = {0.07F, 0.08F, 0.09F};

        MatType mat1 = {col0, col1, col2};
        auto runtimeMat1 = makeValueCopy(mat1);

        MatType mat2 = *runtimeValueCast<MatType>(runtimeMat1);
        ASSERT_TRUE(mat1 == mat2);
        ASSERT_FALSE(mat1 != mat2);

        mat2.setElem(0, 0, 10.0F);
        ASSERT_TRUE(mat1 != mat2);
        ASSERT_FALSE(mat1 == mat2);
    }

    /**
     */
    TEST(TestSerializationMath, Mat4SerializationDeserialization)
    {
        using MatType = math::mat4;
        using VecType = math::vec4;

        static_assert(HasRuntimeValueRepresentation<MatType>);

        const VecType col0 = {1.0F, 2.0F, 3.0F, 4.0F};
        const VecType col1 = {0.5F, 0.6F, 0.7F, 0.8F};
        const VecType col2 = {0.09F, 0.10F, 0.11F, 0.12};
        const VecType col3 = {0.013F, 0.014F, 0.015F, 0.016F};

        MatType mat1 = {col0, col1, col2, col3};
        auto runtimeMat1 = makeValueCopy(mat1);

        MatType mat2 = *runtimeValueCast<MatType>(runtimeMat1);
        ASSERT_TRUE(mat1 == mat2);
        ASSERT_FALSE(mat1 != mat2);

        mat2.setElem(0, 0, 10.0F);
        ASSERT_TRUE(mat1 != mat2);
        ASSERT_FALSE(mat1 == mat2);
    }

    /**
     */
    TEST(TestSerializationMath, VecAsMutableRefCollection)
    {
        math::vec4 vec1{11.f, 22.f, 33.f, 44.f};
        const RuntimeValue::Ptr vecValue = makeValueRef(vec1);
        ASSERT_TRUE(vecValue->isMutable());

        RuntimeReadonlyCollection& collectionValue = vecValue->as<RuntimeReadonlyCollection&>();

        ASSERT_THAT(collectionValue.getSize(), Ge(4));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[0]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[1]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[2]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[3]), Eq(44.f));

        const std::array ExpectedVec{111.f, 222.f, 333.f, 444.f};

        collectionValue.setAt(0, makeValueCopy(ExpectedVec[0])).ignore();
        collectionValue.setAt(1, makeValueCopy(ExpectedVec[1])).ignore();
        collectionValue.setAt(2, makeValueCopy(ExpectedVec[2])).ignore();
        collectionValue.setAt(3, makeValueCopy(ExpectedVec[3])).ignore();

        ASSERT_THAT(vec1.getX(), Eq(ExpectedVec[0]));
        ASSERT_THAT(vec1.getY(), Eq(ExpectedVec[1]));
        ASSERT_THAT(vec1.getZ(), Eq(ExpectedVec[2]));
        ASSERT_THAT(vec1.getW(), Eq(ExpectedVec[3]));
    }

    /**
     */
    TEST(TestSerializationMath, VecAsConstRefCollection)
    {
        const math::vec4 vec1{11.f, 22.f, 33.f, 44.f};
        const RuntimeValue::Ptr vecValue = makeValueRef(vec1);
        ASSERT_FALSE(vecValue->isMutable());

        RuntimeReadonlyCollection& collectionValue = vecValue->as<RuntimeReadonlyCollection&>();

        ASSERT_THAT(collectionValue.getSize(), Ge(4));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[0]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[1]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[2]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[3]), Eq(44.f));
    }

    /**
     */
    TEST(TestSerializationMath, VecAsValueCollection)
    {
        const RuntimeValue::Ptr vecValue = makeValueCopy(math::vec4{11.f, 22.f, 33.f, 44.f});
        ASSERT_TRUE(vecValue->isMutable());

        RuntimeReadonlyCollection& collectionValue = vecValue->as<RuntimeReadonlyCollection&>();

        ASSERT_THAT(collectionValue.getSize(), Ge(4));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[0]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[1]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[2]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(collectionValue[3]), Eq(44.f));
    }

    /**
     */
    TEST(TestSerializationMath, VecAsMutableDictionary)
    {
        math::vec4 vec1{11.f, 22.f, 33.f, 44.f};
        const RuntimeValue::Ptr vecValue = makeValueRef(vec1);
        ASSERT_TRUE(vecValue->isMutable());

        RuntimeReadonlyDictionary& dictValue = vecValue->as<RuntimeReadonlyDictionary&>();

        ASSERT_THAT(dictValue.getSize(), Ge(4));

        ASSERT_TRUE(dictValue.containsKey("x"));
        ASSERT_TRUE(dictValue.containsKey("y"));
        ASSERT_TRUE(dictValue.containsKey("z"));
        ASSERT_TRUE(dictValue.containsKey("w"));

        ASSERT_THAT(*runtimeValueCast<float>(dictValue["x"]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["y"]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["z"]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["w"]), Eq(44.f));

        const std::array ExpectedVec{111.f, 222.f, 333.f, 444.f};

        dictValue.setValue("x", makeValueCopy(ExpectedVec[0])).ignore();
        dictValue.setValue("y", makeValueCopy(ExpectedVec[1])).ignore();
        dictValue.setValue("z", makeValueCopy(ExpectedVec[2])).ignore();
        dictValue.setValue("w", makeValueCopy(ExpectedVec[3])).ignore();

        ASSERT_THAT(vec1.getX(), Eq(ExpectedVec[0]));
        ASSERT_THAT(vec1.getY(), Eq(ExpectedVec[1]));
        ASSERT_THAT(vec1.getZ(), Eq(ExpectedVec[2]));
        ASSERT_THAT(vec1.getW(), Eq(ExpectedVec[3]));
    }

    /**
     */
    TEST(TestSerializationMath, VecAsConstRefDictionary)
    {
        const math::vec4 vec1{11.f, 22.f, 33.f, 44.f};
        const RuntimeValue::Ptr vecValue = makeValueRef(vec1);
        ASSERT_FALSE(vecValue->isMutable());

        RuntimeReadonlyDictionary& dictValue = vecValue->as<RuntimeReadonlyDictionary&>();

        ASSERT_THAT(dictValue.getSize(), Ge(4));

        ASSERT_TRUE(dictValue.containsKey("x"));
        ASSERT_TRUE(dictValue.containsKey("y"));
        ASSERT_TRUE(dictValue.containsKey("z"));
        ASSERT_TRUE(dictValue.containsKey("w"));

        ASSERT_THAT(*runtimeValueCast<float>(dictValue["x"]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["y"]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["z"]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["w"]), Eq(44.f));
    }

    /**
     */
    TEST(TestSerializationMath, VecAsValueDictionary)
    {
        const RuntimeValue::Ptr vecValue = makeValueCopy(math::vec4{11.f, 22.f, 33.f, 44.f});
        ASSERT_TRUE(vecValue->isMutable());

        RuntimeReadonlyDictionary& dictValue = vecValue->as<RuntimeReadonlyDictionary&>();

        ASSERT_THAT(dictValue.getSize(), Ge(4));

        ASSERT_TRUE(dictValue.containsKey("x"));
        ASSERT_TRUE(dictValue.containsKey("y"));
        ASSERT_TRUE(dictValue.containsKey("z"));
        ASSERT_TRUE(dictValue.containsKey("w"));

        ASSERT_THAT(*runtimeValueCast<float>(dictValue["x"]), Eq(11.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["y"]), Eq(22.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["z"]), Eq(33.f));
        ASSERT_THAT(*runtimeValueCast<float>(dictValue["w"]), Eq(44.f));
    }

    /**
     */
    TEST(TestSerializationMath, VecValueTrackChanges)
    {
        math::vec4 vec1{11.f, 22.f, 33.f, 44.f};
        const RuntimeValue::Ptr vecValue = makeValueRef(vec1);

        size_t changesCounter = 0;

        auto subscription = vecValue->as<IRuntimeValueEvents&>().subscribeOnChanges([&](const RuntimeValue&, std::string_view)
        {
            ++changesCounter;
        });

        {
            RuntimeReadonlyDictionary& dictValue = vecValue->as<RuntimeReadonlyDictionary&>();
            dictValue.setValue("x", makeValueCopy(111.f)).ignore();
            ASSERT_EQ(changesCounter, 1);
        }

        {
            RuntimeReadonlyCollection& collValue = vecValue->as<RuntimeReadonlyCollection&>();
            collValue.setAt(1, makeValueCopy(222.f)).ignore();
            ASSERT_EQ(changesCounter, 2);
        }
    }

    TEST(TestSerializationMath, SerializeJson)
    {
        const char* json1 = R"-(
                [1,2,3,4];
         )-";

        const char* json2 = R"-(
                {
                    "x": 11.0, "y": 22.0, "z": null
                }
         )-";

        const auto vec1 = *serialization::JsonUtils::parse<math::vec4>(json1);
        ASSERT_THAT(vec1.getX(), Eq(1.f));
        ASSERT_THAT(vec1.getY(), Eq(2.f));
        ASSERT_THAT(vec1.getZ(), Eq(3.f));
        ASSERT_THAT(vec1.getW(), Eq(4.f));

        math::vec4 vec2 = math::vec4::one();
        serialization::JsonUtils::parse(vec2, json2).ignore();

        ASSERT_THAT(vec2.getX(), Eq(11.f));
        ASSERT_THAT(vec2.getY(), Eq(22.f));
        ASSERT_THAT(vec2.getZ(), Eq(0.f));
        ASSERT_THAT(vec2.getW(), Eq(1.f));
    }
}  // namespace nau::test
