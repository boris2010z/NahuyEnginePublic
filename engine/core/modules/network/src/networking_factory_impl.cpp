// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "networking_factory_impl.h"

namespace nau
{
    eastl::map<eastl::string, NetworkingFactoryImpl::TCreateMethod> NetworkingFactoryImpl::s_methods;

    async::Task<> NetworkingFactoryImpl::preInitService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> NetworkingFactoryImpl::initService()
    {
        return async::Task<>::makeResolved();
    }

    eastl::unique_ptr<INetworking> NetworkingFactoryImpl::create(const eastl::string& name)
    {
        return NetworkingFactoryImpl::Create(name);
    }
}  // namespace nau
