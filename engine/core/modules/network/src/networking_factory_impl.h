// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <EASTL/map.h>
#include <EASTL/string.h>

#include "nau/network/napi/networking_factory.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/service/service.h"

namespace nau
{

    /**
    @brief Factory, register implementations and create INetworking instances
    */
    class NetworkingFactoryImpl final : public IServiceInitialization,
                                        public NetworkingFactory
    {
        NAU_RTTI_CLASS(nau::NetworkingFactoryImpl, NetworkingFactory)

    public:
        virtual async::Task<> preInitService() override;
        virtual async::Task<> initService() override;

        virtual eastl::unique_ptr<INetworking> create(const eastl::string& name) override;

        using TCreateMethod = eastl::unique_ptr<INetworking> (*)();
        TCreateMethod m_CreateFunc;

        /**
        @brief Singleton, register
        */
        static bool Register(const eastl::string name, TCreateMethod createFunc)
        {
            if (auto it = s_methods.find(name); it == s_methods.end())
            {
                s_methods[name] = createFunc;
                return true;
            }
            return false;
        }

        /**
        @brief Singleton, create
        */
        static eastl::unique_ptr<INetworking> Create(const eastl::string& name)
        {
            if (auto it = s_methods.find(name); it != s_methods.end())
                return it->second();

            return nullptr;
        }

    private:
        static eastl::map<eastl::string, TCreateMethod> s_methods;
    };
}  // namespace nau
