/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include "ExtendedOTARequestorDriver.h"
#include "OTARequestorInterface.h"
#include <app/server/Server.h>

namespace chip {
namespace DeviceLayer {

namespace {

constexpr System::Clock::Seconds32 kUserConsentPollInterval = System::Clock::Seconds32(30);

} // namespace

bool ExtendedOTARequestorDriver::CanConsent()
{
    bool localConfigDisabled = false;
    VerifyOrdo(DeviceLayer::ConfigurationMgr().GetLocalConfigDisabled(localConfigDisabled) == CHIP_NO_ERROR,
               ChipLogProgress(SoftwareUpdate, "Failed to get local config disabled, assuming not disabled"));

    // User consent delegation SHALL NOT be used if a Node is configured with the LocalConfigDisabled attribute set to True
    return localConfigDisabled == false;
}

void ExtendedOTARequestorDriver::UpdateAvailable(const UpdateDescription & update, System::Clock::Seconds32 delay)
{
    VerifyOrDie(mRequestor != nullptr);

    if (update.userConsentNeeded == true && mUserConsentDelegate)
    {
        chip::ota::UserConsentSubject subject;
        CHIP_ERROR err = GetUserConsentSubject(subject, update);
        if (err != CHIP_NO_ERROR)
        {
            ChipLogError(SoftwareUpdate, "Failed to get user consent subject");
            return;
        }

        mDelayedActionTime = delay;
        HandleUserConsentState(mUserConsentDelegate->GetUserConsentState(subject));
        return;
    }

    DefaultOTARequestorDriver::UpdateAvailable(update, delay);
}

void ExtendedOTARequestorDriver::PollUserConsentState()
{
    HandleUserConsentState(mUserConsentDelegate->CheckDeferredUserConsentState());
}

CHIP_ERROR ExtendedOTARequestorDriver::GetUserConsentSubject(chip::ota::UserConsentSubject & subject,
                                                             const UpdateDescription & update)
{
    Optional<ProviderLocationType> lastUsedProvider;
    mRequestor->GetProviderLocation(lastUsedProvider);
    if (lastUsedProvider.HasValue())
    {
        subject.fabricIndex        = lastUsedProvider.Value().fabricIndex;
        subject.providerEndpointId = lastUsedProvider.Value().endpoint;
    }
    else
    {
        ChipLogError(SoftwareUpdate, "Last used provider is empty");
        return CHIP_ERROR_INTERNAL;
    }

    FabricInfo * fabricInfo = Server::GetInstance().GetFabricTable().FindFabricWithIndex(subject.fabricIndex);
    if (fabricInfo == nullptr)
    {
        ChipLogError(SoftwareUpdate, "Cannot find fabric");
        return CHIP_ERROR_INTERNAL;
    }
    subject.requestorNodeId = fabricInfo->GetPeerId().GetNodeId();

    ReturnErrorOnFailure(DeviceLayer::ConfigurationMgr().GetVendorId(subject.requestorVendorId));
    ReturnErrorOnFailure(DeviceLayer::ConfigurationMgr().GetProductId(subject.requestorProductId));
    ReturnErrorOnFailure(DeviceLayer::ConfigurationMgr().GetSoftwareVersion(subject.requestorCurrentVersion));
    subject.requestorTargetVersion = update.softwareVersion;
    subject.metadata               = update.metadataForRequestor;

    return CHIP_NO_ERROR;
}

void ExtendedOTARequestorDriver::HandleUserConsentState(chip::ota::UserConsentState userConsentState)
{
    ChipLogDetail(SoftwareUpdate, "User consent state: %s", mUserConsentDelegate->UserConsentStateToString(userConsentState));

    switch (userConsentState)
    {
    case chip::ota::UserConsentState::kGranted:
        ScheduleDelayedAction(
            mDelayedActionTime,
            [](System::Layer *, void * context) {
                static_cast<ExtendedOTARequestorDriver *>(context)->mRequestor->DownloadUpdate();
            },
            this);
        break;

    case chip::ota::UserConsentState::kDenied:
        ScheduleDelayedAction(
            mDelayedActionTime,
            [](System::Layer *, void * context) {
                static_cast<ExtendedOTARequestorDriver *>(context)->mRequestor->CancelImageUpdate();
            },
            this);
        break;

    case chip::ota::UserConsentState::kObtaining:
        SystemLayer().ScheduleWork(
            [](System::Layer *, void * context) {
                static_cast<ExtendedOTARequestorDriver *>(context)->mRequestor->DownloadUpdateDelayedOnUserConsent();
            },
            this);

        ScheduleDelayedAction(
            kUserConsentPollInterval,
            [](System::Layer *, void * context) { static_cast<ExtendedOTARequestorDriver *>(context)->PollUserConsentState(); },
            this);
        break;

    default:
        break;
    }
}

} // namespace DeviceLayer
} // namespace chip
