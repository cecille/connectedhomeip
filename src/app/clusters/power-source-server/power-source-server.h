/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <app/util/af-types.h>
#include <app/util/basic-types.h>
#include <lib/support/Span.h>
#include <platform/CHIPDeviceConfig.h>

class PowerSourceClusterInfoBase
{
public:
    PowerSourceClusterInfoBase(chip::EndpointId powerClusterEndpointId) : mClusterEndpoint(powerClusterEndpointId) {}
    chip::EndpointId GetClusterEndpointId() { return mClusterEndpoint; }
    virtual CHIP_ERROR SetEndpointList(chip::Span<chip::EndpointId> endpointList) { return CHIP_ERROR_NOT_IMPLEMENTED; }
    virtual chip::Span<chip::EndpointId> & GetEndpointList() { return mEmpty; }

private:
    chip::EndpointId mClusterEndpoint;
    chip::Span<chip::EndpointId> mEmpty;
};

template <size_t N>
class PowerSourceClusterInfo : public PowerSourceClusterInfoBase
{
public:
    PowerSourceClusterInfo(chip::EndpointId powerClusterEndpointId) : PowerSourceClusterInfoBase(powerClusterEndpointId) {}
    CHIP_ERROR SetEndpointList(chip::Span<chip::EndpointId> endpointList) override
    {
        if (endpointList.size() != mEndpointList.size())
        {
            // These should be identical, otherwise this is incorrectly templated
            return CHIP_ERROR_NO_MEMORY;
        }
        memcpy(mEndpointList.data(), endpointList.data(), endpointList.size())
    }
    chip::Span<chip::EndpointId> & GetEndpointList() override { return mEndpointList; }

private:
    chip::FixedSpan<chip::EndpointId, N> mEndpointList;
};

class PowerSourceServer
{
public:
    static PowerSourceServer & Instance();

    // Caller does not need to retain the span past the call point as these are copied into an internal storage
    CHIP_ERROR SetEndpointList(chip::EndpointId powerSourceClusterEndpoint, chip::Span<chip::EndpointId> endpointList);
    CHIP_ERROR GetEndpointList(chip::EndpointId PowerSourceClusterEndpoint);

private:
    // There's probably a good way to properly set this from zap or have this grow dynamically as dynamic endpoints are
    // finalized, but for now, let's just say 10 power clusters is probably ok.
    // TODO: is there a way to get a static count of endpoints? CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT?
    std::unique_ptr<PowerSourceClusterInfoBase> mPowerSourceClusterInfo[10] = {};
    int PowerSourceClusterEndpointIndex(chip::EndpointId endpointId);
};
