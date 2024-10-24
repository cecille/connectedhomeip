/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
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

#include "AppTask.h"
#include "AppConfig.h"

#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <credentials/DeviceAttestationCredsProvider.h>

#include <assert.h>

#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include <lib/support/CodeUtils.h>

#include <platform/CHIPDeviceLayer.h>

#include "DrinksMachineDeviceDriver.h"

#include <app-common/zap-generated/ids/Clusters.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-cluster-logic.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-delegate.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-server-disco.h>
#include <app/util/attribute-storage.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/KvsPersistentStorageDelegate.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ValveConfigurationAndControl;
using namespace chip::DeviceLayer;
using namespace chip::DeviceLayer::Silabs;
using namespace google::matter;

// HACK: on Silabs to remove the Example DAC provider. Dunno why it's even needed.
namespace chip {
namespace Credentials {
namespace Examples {

DeviceAttestationCredentialsProvider * GetExampleDACProvider()
{
    return nullptr;
}

} // namespace Examples
} // namespace Credentials
} // namespace chip

namespace {
class PrintOnlyDelegate : public NonLevelControlDelegate
{
public:
    PrintOnlyDelegate(EndpointId endpoint) : mEndpoint(endpoint) {}
    CHIP_ERROR HandleOpenValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) override
    {
        ChipLogError(NotSpecified, "\n\nVALVE IS OPENING on endpoint %u!!!!!\n\n", mEndpoint);
        state        = ValveStateEnum::kOpen;
        currentState = state;
        return CHIP_NO_ERROR;
    }
    ValveStateEnum GetCurrentValveState() override { return state; }
    CHIP_ERROR HandleCloseValve(ValveStateEnum & currentState, BitMask<ValveFaultBitmap> & valveFault) override
    {
        ChipLogError(NotSpecified, "\n\nVALVE IS CLOSING on endpoint %u!!!!!\n\n", mEndpoint);
        state        = ValveStateEnum::kClosed;
        currentState = state;
        return CHIP_NO_ERROR;
    }

private:
    ValveStateEnum state = ValveStateEnum::kClosed;
    EndpointId mEndpoint;
};

// TODO: Does this conflict with the kvs delegate that's statically allocated in the AppMain?
// I think as long as the manager is the same, we're ok, but need to confirm.
KvsPersistentStorageDelegate sStorage;

class NonLevelValveEndpoint
{
public:
    NonLevelValveEndpoint(EndpointId endpoint) :
        mEndpoint(endpoint), mContext(mEndpoint, sStorage), mDelegate(mEndpoint), mLogic(mDelegate, mContext),
        mInterface(mEndpoint, mLogic)
    {}
    CHIP_ERROR Init()
    {
        ReturnErrorOnFailure(mLogic.Init(kConformance, kInitParams));
        ReturnErrorOnFailure(mInterface.Init());
        return CHIP_NO_ERROR;
    }

private:
    const ClusterConformance kConformance = {
        .featureMap = 0, .supportsDefaultOpenLevel = false, .supportsValveFault = false, .supportsLevelStep = false
    };
    const ClusterInitParameters kInitParams = { .currentState = DataModel::MakeNullable(ValveStateEnum::kClosed),
                                                .currentLevel = DataModel::NullNullable,
                                                .valveFault   = 0,
                                                .levelStep    = 1 };
    EndpointId mEndpoint;
    MatterContext mContext;
    PrintOnlyDelegate mDelegate;
    ClusterLogic mLogic;
    Interface mInterface;
};

NonLevelValveEndpoint ep1(1);
NonLevelValveEndpoint ep2(2);
NonLevelValveEndpoint ep3(3);
NonLevelValveEndpoint ep4(4);
NonLevelValveEndpoint ep5(5);
NonLevelValveEndpoint ep6(6);

// from https://github.com/CHIP-Specifications/connectedhomeip-spec/blob/master/src/namespaces/Namespace-Common-Position.adoc
constexpr const uint8_t kNamespaceCommonPosition    = 0x8;
constexpr const uint8_t kNamespaceCommonPositionRow = 0x5;

const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp1TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("1"_span)) },
};
const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp2TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("2"_span)) },
};
const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp3TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("3"_span)) },
};
const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp4TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("4"_span)) },
};
const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp5TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("5"_span)) },
};
const Clusters::Descriptor::Structs::SemanticTagStruct::Type gEp6TagList[] = {
    { .namespaceID = kNamespaceCommonPosition,
      .tag         = kNamespaceCommonPositionRow,
      .label       = chip::MakeOptional(DataModel::Nullable<chip::CharSpan>("6"_span)) },
};
} // namespace

using namespace chip::TLV;
using namespace ::chip::DeviceLayer;

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::Init()
{
    // DrinksMachineSilabsDriver::GetInstance().Init();

    CHIP_ERROR err = BaseApplication::Init();
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("BaseApplication::Init() failed");
        appError(err);
    }
    google::matter::DrinkMachineMatterProviders::GetInstance().Init();

    return err;
}

CHIP_ERROR AppTask::StartAppTask()
{
    return BaseApplication::StartAppTask(AppTaskMain);
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;

    osMessageQueueId_t sAppEventQueue = *(static_cast<osMessageQueueId_t *>(pvParameter));
    chip::DeviceLayer::Silabs::GetPlatform().SetButtonsCb(AppTask::ButtonEventHandler);

    CHIP_ERROR err = sAppTask.Init();
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("AppTask.Init() failed");
        appError(err);
    }

#if !(defined(CHIP_CONFIG_ENABLE_ICD_SERVER) && CHIP_CONFIG_ENABLE_ICD_SERVER)
    sAppTask.StartStatusLEDTimer();
#endif

    ep1.Init();
    ep2.Init();
    ep3.Init();
    ep4.Init();
    ep5.Init();
    ep6.Init();
    SetTagList(/* endpoint= */ 1, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp1TagList));
    SetTagList(/* endpoint= */ 2, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp2TagList));
    SetTagList(/* endpoint= */ 3, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp3TagList));
    SetTagList(/* endpoint= */ 4, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp4TagList));
    SetTagList(/* endpoint= */ 5, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp5TagList));
    SetTagList(/* endpoint= */ 6, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp6TagList));

    SILABS_LOG("App Task started");

    while (true)
    {
        osStatus_t eventReceived = osMessageQueueGet(sAppEventQueue, &event, NULL, osWaitForever);
        while (eventReceived == osOK)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = osMessageQueueGet(sAppEventQueue, &event, NULL, 0);
        }
    }
}

void AppTask::ButtonEventHandler(uint8_t button, uint8_t btnAction)
{
    AppEvent button_event           = {};
    button_event.Type               = AppEvent::kEventType_Button;
    button_event.ButtonEvent.Action = btnAction;
    button_event.Handler            = BaseApplication::ButtonHandler;
    sAppTask.PostEvent(&button_event);
}
