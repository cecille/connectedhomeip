/*
 *
 *    Copyright (c) 2024 Project CHIP Authors
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
#include <air-purifier-manager.h>
#include <pigpiod_if2.h>
#include <AppMain.h>
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

// The MatterPostAttributeChangeCallback is offloaded to the main Air Purifier Manager class.
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    if (AirPurifierManager::GetInstance() != nullptr)
    {
        AirPurifierManager::GetInstance()->PostAttributeChangeCallback(attributePath.mEndpointId, attributePath.mClusterId,
                                                                       attributePath.mAttributeId, type, size, value);
    }
}
namespace {
class NonLevelPrintOnlyDelegate : public NonLevelControlDelegate
{
public:
    NonLevelPrintOnlyDelegate(EndpointId endpoint) : mEndpoint(endpoint) {}
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

class LevelPrintOnlyDelegate : public LevelControlDelegate
{
public:
    LevelPrintOnlyDelegate(EndpointId endpoint) : mLevel(0), mEndpoint(endpoint){}

    Percent GetCurrentValveLevel() { return mLevel; }
    CHIP_ERROR HandleOpenValve(const Percent targetLevel, Percent & currentLevel,
                                       BitMask<ValveFaultBitmap> & valveFault)
    {
        ChipLogError(NotSpecified, "\n\nVALVE IS OPENING on endpoint %u!!!!!\n\n", mEndpoint);
        mLevel = targetLevel;
        currentLevel = targetLevel;
        return CHIP_NO_ERROR;
    }
    CHIP_ERROR HandleCloseValve(Percent & currentLevel, BitMask<ValveFaultBitmap> & valveFault)
    {
        ChipLogError(NotSpecified, "\n\nVALVE IS CLOSING on endpoint %u!!!!!\n\n", mEndpoint);
        mLevel = 0;
        currentLevel = 0;
        return CHIP_NO_ERROR;
    }

private:
    Percent mLevel;
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
    NonLevelPrintOnlyDelegate mDelegate;
    ClusterLogic mLogic;
    Interface mInterface;
};

class LevelValveEndpoint
{
    public:
    LevelValveEndpoint(EndpointId endpoint) :
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
        .featureMap = 2, .supportsDefaultOpenLevel = false, .supportsValveFault = false, .supportsLevelStep = false
    };
    const ClusterInitParameters kInitParams = { .currentState = DataModel::MakeNullable(ValveStateEnum::kClosed),
                                                .currentLevel = DataModel::MakeNullable(Percent(0)),
                                                .valveFault   = 0,
                                                .levelStep    = 1 };
    EndpointId mEndpoint;
    MatterContext mContext;
    LevelPrintOnlyDelegate mDelegate;
    ClusterLogic mLogic;
    Interface mInterface;
};

NonLevelValveEndpoint ep1(1);
LevelValveEndpoint ep2(2);

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

int sPigpio = -1;
} // namespace

void ShutdownGpio(){
    if (sPigpio != -1) {
	    pigpio_stop(sPigpio);
	    sPigpio = -1;
    }
}

bool SetupGpio(int num, unsigned mode) {
    if (set_mode(sPigpio, num, mode) != 0) {
	    ChipLogError(NotSpecified, "Error setting gpio mode %d", num);
	    ShutdownGpio();
	    return false;
    }
    if (set_pull_up_down(sPigpio, num, PI_PUD_UP) != 0) {
	    ChipLogError(NotSpecified, "Error setting PUD %d", num);
	    ShutdownGpio();
	    return false;
    }
    return true;
}

void ApplicationInit()
{
    ChipLogError(NotSpecified, "App init!!!");
    chip::DeviceLayer::PersistedStorage::KeyValueStoreManager & kvsManager = DeviceLayer::PersistedStorage::KeyValueStoreMgr();
    sStorage.Init(&kvsManager);

    ep1.Init();
    ep2.Init();
    // TODO: Can we pull these from the command line or something so these can be swapped on the fly?
    SetTagList(/* endpoint= */ 1, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp1TagList));
    SetTagList(/* endpoint= */ 2, Span<const Clusters::Descriptor::Structs::SemanticTagStruct::Type>(gEp2TagList));

    AirPurifierManager::InitInstance(EndpointId(3));

    sPigpio = pigpio_start(nullptr, nullptr);
    if (sPigpio < 0) {
	    ChipLogError(NotSpecified, "Unable to start PI GPIO handler");
	    return;
    }
    if (!SetupGpio(20, PI_OUTPUT)) {
	    return;
    }
    if (!SetupGpio(26, PI_OUTPUT)) {
	    return;
    }
    if (!SetupGpio(13, PI_INPUT)) {
	    return;
    }
    if (!SetupGpio(12, PI_INPUT)) {
	    return;
    }
}

void ApplicationShutdown()
{
    ChipLogDetail(NotSpecified, "ApplicationShutdown()");
}

int main(int argc, char * argv[])
{
    if (ChipLinuxAppInit(argc, argv) != 0)
    {
        return -1;
    }

    ChipLinuxAppMainLoop();
    return 0;
}
