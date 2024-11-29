// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include "nau/rtti/rtti_impl.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"
#include "nau/utils/scope_guard.h"

namespace nau
{
    class ServiceProviderImpl final : public ServiceProvider,
                                      public core_detail::IServiceProviderInitialization
    {
        NAU_RTTI_CLASS(nau::ServiceProviderImpl, ServiceProvider, core_detail::IServiceProviderInitialization)
    public:
        ServiceProviderImpl();

        ~ServiceProviderImpl();

    private:
        struct ServiceInstanceEntry
        {
            void* const serviceInstance;
            ServiceAccessor* const accessor;

            ServiceInstanceEntry(void* inServiceInstance, ServiceAccessor* inAccessor) :
                serviceInstance(inServiceInstance),
                accessor(inAccessor)
            {
            }

            operator void*() const
            {
                return serviceInstance;
            }
        };

        void* findInternal(const rtti::TypeInfo&) override;

        void findAllInternal(const rtti::TypeInfo&, void (*)(void* instancePtr, void*), void*, ServiceAccessor::GetApiMode) override;

        void addServiceAccessorInternal(ServiceAccessor::Ptr, IClassDescriptor::Ptr) override;

        void addClass(IClassDescriptor::Ptr&& descriptor) override;

        eastl::vector<IClassDescriptor::Ptr> findClasses(const rtti::TypeInfo& type) override;

        eastl::vector<IClassDescriptor::Ptr> findClasses(eastl::span<const rtti::TypeInfo*>, bool anyType) override;

        bool hasApiInternal(const rtti::TypeInfo&) override;

        void setInitializationProxy(const IServiceInitialization& source, IServiceInitialization* proxy) override;

        async::Task<> preInitServices() override;

        async::Task<> initServices() override;

        async::Task<> shutdownServices() override;

        async::Task<> initServicesInternal(async::Task<> (*)(IServiceInitialization&));

        template<typename T>
        T& getInitializationInstance(T* instance);

        eastl::list<ServiceAccessor::Ptr> m_accessors;
        eastl::unordered_map<rtti::TypeIndex, ServiceInstanceEntry> m_instances;
        eastl::vector<IClassDescriptor::Ptr> m_classDescriptors;
        eastl::unordered_map<const IServiceInitialization*, IServiceInitialization*> m_initializationProxy;
        std::shared_mutex m_mutex;
        bool m_isDisposed = false;
    };
}  // namespace nau
