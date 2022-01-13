/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

// THIS FILE IS GENERATED BY ZAP

// Prevent multiple inclusion
#pragma once

#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>

#include <controller/CHIPCluster.h>
#include <lib/core/CHIPCallback.h>
#include <lib/support/Span.h>

namespace chip {
namespace Controller {

class DLL_EXPORT OtaSoftwareUpdateProviderCluster : public ClusterBase
{
public:
    OtaSoftwareUpdateProviderCluster() : ClusterBase(app::Clusters::OtaSoftwareUpdateProvider::Id) {}
    ~OtaSoftwareUpdateProviderCluster() {}

    // Cluster Commands
    CHIP_ERROR ApplyUpdateRequest(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                                  chip::ByteSpan updateToken, uint32_t newVersion);
    CHIP_ERROR NotifyUpdateApplied(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                                   chip::ByteSpan updateToken, uint32_t softwareVersion);
    CHIP_ERROR QueryImage(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                          chip::VendorId vendorId, uint16_t productId, uint32_t softwareVersion, uint8_t protocolsSupported,
                          uint16_t hardwareVersion, chip::CharSpan location, bool requestorCanConsent,
                          chip::ByteSpan metadataForProvider);
};

class DLL_EXPORT OnOffCluster : public ClusterBase
{
public:
    OnOffCluster() : ClusterBase(app::Clusters::OnOff::Id) {}
    ~OnOffCluster() {}
};

} // namespace Controller
} // namespace chip
