// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <EASTL/string.h>

#include <atomic>
#include <concepts>

#include "nau/assets/asset_descriptor.h"
#include "nau/assets/asset_path.h"
#include "nau/serialization/native_runtime_value/native_value_base.h"
#include "reloadable_asset_view.h"

namespace nau::assets
{
    /**
      Temporary animation asset tags.
    */
    struct AnimationAssetTag
    {
    };

    /**
          Temporary mesh asset tags.
     */
    struct StaticMeshAssetTag
    {
    };

    /**
          Temporary mesh asset tags.
     */
    struct SkinnedMeshAssetTag
    {
    };

    /**
          Temporary mesh asset tags.
     */
    struct SkeletonAssetTag
    {
    };

    /**
        Temporary texture asset tags.
    */
    struct TextureAssetTag
    {
    };

    /**
        Temporary material asset tags.
    */
    struct ShaderAssetTag
    {
    };

    /**
        Temporary material asset tags.
    */
    struct MaterialAssetTag
    {
    };

    /**
        Temporary ui scene asset tags.
    */
    struct UiSceneAssetTag
    {
    };

    /**
        Temporary generic asset tags.
    */
    struct GenericAssetTag
    {
    };

}  // namespace nau::assets

namespace nau
{

    /**
     * @brief Provides asset reference generic functionality.
     */
    class NAU_COREASSETS_EXPORT AssetRefBase
    {
        NAU_TYPEID(nau::AssetRefBase)

    public:
        /**
         * @brief Destructor.
         */
        virtual ~AssetRefBase();

        /**
         * @brief Checks whether
         *
         * @return
         */
        explicit operator bool() const;
    private:
        /**
         * @brief Schedules asset view retrieval.
         *
         * @param [in] viewDescription  Encapsulates asset information.
         * @return                      Task object that provides operation status info as well as access to the retrieved view.
         */
        async::Task<IAssetView::Ptr> getAssetView(const AssetViewDescription& = {});
        async::Task<ReloadableAssetView::Ptr> getReloadableAssetView(const AssetViewDescription = {});

    public:

        /**
         * @brief Schedules asset view retrieval.
         *
         * @tparam ViewType Type of the asset view to retrieve. It has to be a subclass of IAssetView.
         *
         * @return                      Task object that provides operation status info as well as access to the retrieved view.
         */
        template <std::derived_from<IAssetView> ViewType>
        async::Task<nau::Ptr<ViewType>> getAssetViewTyped();
        template <std::derived_from<IAssetView> ViewType>
        async::Task<ReloadableAssetView::Ptr> getReloadableAssetViewTyped();


        /**
         * @brief Checks whether an asset reference can be bound to a different asset.
         *
         * @param [in] assetRef Reference to an asset to bind.
         * @return              `true` if assignment is possible, `false` otherwise.
         */
        virtual bool isAssignable([[maybe_unused]] const AssetRefBase&) const
        {
            return true;
        }

        /**
         * @brief Default constructor.
         */
        AssetRefBase() noexcept = default;

        /**
         * @brief Initialization constructor.
         *
         * @param [in] Path to the asset to bind.
         */
        AssetRefBase(AssetPath assetPath, bool loadOnDemand = false) noexcept;

        AssetRefBase(eastl::string_view assetPathStr, bool loadOnDemand = false) noexcept;

        //template <size_t N>
        //inline AssetRefBase(const char (&assetPathStr)[N], bool loadOnDemand = false) noexcept
        //    :
        //    AssetRefBase{AssetPath{eastl::string_view{assetPathStr, N}}, loadOnDemand}
        //{
        //}

        /**
         * @brief Initialization constructor.
         *
         * @param [in] Description of the asset to bind.
         */
        AssetRefBase(IAssetDescriptor::Ptr) noexcept;

        /**
         * @brief Copy constructor.
         *
         * @param [in] assetRef Reference to the asset to bind.
         */
        AssetRefBase(const AssetRefBase&) noexcept;

        /**
         * @brief Move constructor.
         *
         * @param [in] assetRef Reference to the asset to bind.
         */
        AssetRefBase(AssetRefBase&&) noexcept;

        /**
         * @brief Assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRefBase& operator=(const AssetRefBase&) noexcept;

        /**
         * @brief Move-assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRefBase& operator=(AssetRefBase&&) noexcept;

        AssetRefBase& operator=(std::nullptr_t) noexcept;

        IAssetDescriptor::Ptr getAssetDiscriptor() const { return m_assetDescriptor; };

    protected:
        IAssetDescriptor::Ptr m_assetDescriptor;

        NAU_COREASSETS_EXPORT friend Result<> parse(std::string_view assetPath, AssetRefBase& assetRef);
        NAU_COREASSETS_EXPORT friend std::string toString(const AssetRefBase& assetRef);
    };

    /**
     * @brief Encapsulates a reference to an asset.
     *
     * @tparam T Type of the asset the reference is bound to.
     */
    template <typename T = assets::GenericAssetTag>
    class AssetRef : public AssetRefBase
    {
        NAU_TYPEID(nau::AssetRef<T>)
        NAU_CLASS_BASE(AssetRefBase)

    public:
        static inline constexpr bool HasOwnRuntimeValue = true;

        using AssetRefBase::AssetRefBase;

        /**
         * @brief Copy constructor.
         *
         * @param [in] other Reference to the asset to bind.
         */
        AssetRef(const AssetRef<T>&) noexcept;

        /**
         * @brief Move constructor.
         *
         * @param [in] other Reference to the asset to bind.
         */
        AssetRef(AssetRef<T>&&) noexcept;

        AssetRef& operator=(std::nullptr_t) noexcept;

        /**
         * @brief Assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRef& operator=(const AssetRef&) noexcept;

        /**
         * @brief Move-assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRef& operator=(AssetRef&&) noexcept;

        /**
         * @brief Assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRef& operator=(const AssetRefBase&) noexcept;

        /**
         * @brief Move-assignment operator
         *
         * @param [in] assetRef Reference to the asset to bind.
         * @return              Resulted reference object.
         */
        AssetRef& operator=(AssetRefBase&&) noexcept;

        bool operator==(const AssetRef& other) noexcept
        {
            return this->m_assetDescriptor->getAssetPath() == other.m_assetDescriptor->getAssetPath();
        }

        bool isAssignable([[maybe_unused]] const AssetRefBase&) const override
        {
            return true;
        }
    };

    using AnimationAssetRef = AssetRef<assets::AnimationAssetTag>;
    using StaticMeshAssetRef = AssetRef<assets::StaticMeshAssetTag>;
    using SkinnedMeshAssetRef = AssetRef<assets::SkinnedMeshAssetTag>;
    using SkeletonAssetRef = AssetRef<assets::SkeletonAssetTag>;
    using TextureAssetRef = AssetRef<assets::TextureAssetTag>;
    using ShaderAssetRef = AssetRef<assets::ShaderAssetTag>;
    using MaterialAssetRef = AssetRef<assets::MaterialAssetTag>;
    using UiSceneAssetRef = AssetRef<assets::UiSceneAssetTag>;

    struct NAU_ABSTRACT_TYPE RuntimeAssetRefValue : virtual RuntimePrimitiveValue
    {
        NAU_INTERFACE(nau::RuntimeAssetRefValue, RuntimePrimitiveValue)

        virtual bool isAssignable(const AssetRefBase& assetRef) = 0;

        virtual bool setAssetRef(AssetRefBase assetRef) = 0;

        virtual AssetRefBase getAssetRef() const = 0;
    };

    template <std::derived_from<IAssetView> ViewType>
    async::Task<nau::Ptr<ViewType>> AssetRefBase::getAssetViewTyped()
    {
        const AssetViewDescription description{
            rtti::getTypeInfo<ViewType>()};

        IAssetView::Ptr assetView = co_await getAssetView(description);
        if (!assetView)
        {
            // NAU_LOG_WARNING(u8"getAssetView({}) returns nothing", description.viewApi->getTypeName());
            co_return nullptr;
        }

        if (!assetView->is<ViewType>())
        {
            // NAU_LOG_WARNING(u8"getAssetView({}) returns a view that does not support requested API", description.viewApi->getTypeName());
            co_return nullptr;
        }

        co_return assetView;
    }

    template <std::derived_from<IAssetView> ViewType>
    async::Task<ReloadableAssetView::Ptr> AssetRefBase::getReloadableAssetViewTyped()
    {
        const AssetViewDescription description{
            rtti::getTypeInfo<ViewType>()};

        ReloadableAssetView::Ptr assetView = co_await getReloadableAssetView(description);
        if (!assetView)
        {
            // NAU_LOG_WARNING(u8"getAssetView({}) returns nothing", description.viewApi->getTypeName());
            co_return nullptr;
        }

        co_return assetView;
    }

    template <typename T>
    AssetRef<T>::AssetRef(const AssetRef<T>& other) noexcept :
        AssetRefBase{static_cast<const AssetRefBase&>(other)}
    {
    }

    template <typename T>
    AssetRef<T>::AssetRef(AssetRef<T>&& other) noexcept :
        AssetRefBase{static_cast<AssetRefBase&&>(other)}
    {
    }

    template <typename T>
    AssetRef<T>& AssetRef<T>::operator=(std::nullptr_t) noexcept
    {
        static_cast<AssetRefBase&>(*this) = nullptr;
        return *this;
    }

    template <typename T>
    AssetRef<T>& AssetRef<T>::operator=(const AssetRef& other) noexcept
    {
        static_cast<AssetRefBase&>(*this) = static_cast<const AssetRefBase&>(other);

        return *this;
    }

    template <typename T>
    AssetRef<T>& AssetRef<T>::operator=(AssetRef&& other) noexcept
    {
        static_cast<AssetRefBase&>(*this) = static_cast<AssetRefBase&&>(other);
        return *this;
    }

    template <typename T>
    AssetRef<T>& AssetRef<T>::operator=(const AssetRefBase& other) noexcept
    {
        static_cast<AssetRefBase&>(*this) = other;
        return *this;
    }

    template <typename T>
    AssetRef<T>& AssetRef<T>::operator=(AssetRefBase&& other) noexcept
    {
        static_cast<AssetRefBase&>(*this) = std::move(other);
        return *this;
    }

}  // namespace nau

namespace nau::ser_detail
{

    /**
     */
    template <typename T>
    class RuntimeAssetRefValueImpl final : public NativePrimitiveRuntimeValueBase<RuntimeAssetRefValue>,
                                           public virtual RuntimeStringValue
    {
        using Base = NativePrimitiveRuntimeValueBase<RuntimeAssetRefValue>;

        NAU_CLASS_(RuntimeAssetRefValueImpl<T>, Base, RuntimeStringValue)

        using ValueType = std::decay_t<T>;

        static_assert(std::is_base_of_v<AssetRefBase, ValueType>);

        static inline constexpr bool IsMutable = !std::is_const_v<std::remove_reference_t<T>>;
        static inline constexpr bool IsReference = std::is_lvalue_reference_v<T>;

    public:
        RuntimeAssetRefValueImpl(T assetRef)
        requires(IsReference)
            :
            m_assetRef(assetRef)
        {
        }

        RuntimeAssetRefValueImpl(ValueType&& assetRef)
        requires(!IsReference)
            :
            m_assetRef(std::move(assetRef))
        {
        }

        RuntimeAssetRefValueImpl(const ValueType& assetRef)
        requires(!IsReference)
            :
            m_assetRef(assetRef)
        {
        }

        bool isMutable() const override
        {
            return IsMutable;
        }

        bool isAssignable(const AssetRefBase& assetRef) override
        {
            return IsMutable && (m_assetRef && m_assetRef.isAssignable(assetRef));
        }

        bool setAssetRef(AssetRefBase assetRef) override
        {
            if constexpr (IsMutable)
            {
                NAU_ASSERT(!m_assetRef || m_assetRef.isAssignable(assetRef));
                if (m_assetRef.isAssignable(assetRef))
                {
                    return false;
                }
                value_changes_scope;
                m_assetRef = std::move(assetRef);
                
            }

            return false;
        }

        AssetRefBase getAssetRef() const override
        {
            return m_assetRef;
        }

        Result<> setString(std::string_view str) override
        {
            value_changes_scope;
            [[maybe_unused]] Result<> parseResult = parse(str, m_assetRef);
            NauCheckResult(parseResult)

            return ResultSuccess;
        }

        std::string getString() const override
        {
            return toString(m_assetRef);
        }

    private:
        T m_assetRef;
    };

}  // namespace nau::ser_detail

namespace nau
{
    template <std::derived_from<AssetRefBase> T>
    RuntimeValue::Ptr makeValueRef(T& assetRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = ser_detail::RuntimeAssetRefValueImpl<T&>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), assetRef);
    }

    template <std::derived_from<AssetRefBase> T>
    RuntimeValue::Ptr makeValueRef(const T& assetRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = ser_detail::RuntimeAssetRefValueImpl<const T&>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), assetRef);
    }

    template <std::derived_from<AssetRefBase> T>
    RuntimeValue::Ptr makeValueCopy(const T& assetRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = ser_detail::RuntimeAssetRefValueImpl<T>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), assetRef);
    }

    template <std::derived_from<AssetRefBase> T>
    RuntimeValue::Ptr makeValueCopy(T&& assetRef, IMemAllocator::Ptr allocator = nullptr)
    {
        using Type = ser_detail::RuntimeAssetRefValueImpl<T>;

        return rtti::createInstanceWithAllocator<Type>(std::move(allocator), std::move(assetRef));
    }
}  // namespace nau
