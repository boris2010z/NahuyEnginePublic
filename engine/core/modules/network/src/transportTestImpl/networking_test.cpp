// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "nau/network/transportTest/networking_test.h"

namespace nau
{
    NetworkingTest::NetworkingTest()
    {
    }

    NetworkingTest::~NetworkingTest()
    {
    }

    bool NetworkingTest::applyConfig(const eastl::string& data)
    {
        return true;
    }

    bool NetworkingTest::init()
    {
        return true;
    }

    bool NetworkingTest::shutdown()
    {
        return true;
    }

    bool NetworkingTest::update()
    {
        return true;
    }

    const INetworkingIdentity& NetworkingTest::identity() const
    {
        class NetworkingIdentity : public INetworkingIdentity
        {
            NetworkingIdentityType getType() const override
            {
                return NetworkingIdentityType::Local;
            }
            eastl::string toString() const override
            {
                return "NetworkingIdentity";
            }
        };
        static NetworkingIdentity s_id;
        return s_id;
    }

    eastl::shared_ptr<INetworkingListener> NetworkingTest::createListener()
    {
        return nullptr;
    }

    eastl::shared_ptr<INetworkingConnector> NetworkingTest::createConnector()
    {
        return nullptr;
    }

}  // namespace nau
