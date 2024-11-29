// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <EASTL/unique_ptr.h>

#include "nau/network/napi/networking.h"

namespace nau
{
    /**
       @brief Empty test implementation, developers only usage
    */
    class NetworkingTest : public INetworking
    {
    public:
        NetworkingTest();
        ~NetworkingTest();

        bool applyConfig(const eastl::string& data) override;
        bool init() override;
        bool shutdown() override;
        bool update() override;

        const INetworkingIdentity& identity() const override;

        eastl::shared_ptr<INetworkingListener> createListener() override;
        eastl::shared_ptr<INetworkingConnector> createConnector() override;

        static eastl::unique_ptr<nau::INetworking> create()
        {
            return eastl::make_unique<NetworkingTest>();
        }
    };
}  // namespace nau
