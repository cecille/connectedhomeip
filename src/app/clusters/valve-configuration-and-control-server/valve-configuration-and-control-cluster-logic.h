/**
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
/**
 * @file Cross-platform API to handle cluster-specific logic for the valve configuration and control cluster on a single endpoint.
 */

#pragma once

#include "valve-configuration-and-control-delegate.h"
#include "valve-configuration-and-control-matter-context.h"
#include <app-common/zap-generated/cluster-objects.h>
#include <app/cluster-building-blocks/QuieterReporting.h>
#include <app/data-model/Nullable.h>
#include <lib/core/CHIPError.h>
#include <lib/support/BitMask.h>
#include <system/SystemLayer.h>

namespace chip {
namespace app {
namespace Clusters {
namespace ValveConfigurationAndControl {

struct ClusterConformance
{
    constexpr inline bool HasFeature(Feature feature) const { return featureMap & to_underlying(feature); }
    uint32_t featureMap;
    bool supportsDefaultOpenLevel;
    bool supportsValveFault;
    bool supportsLevelStep;
    constexpr bool Valid() const
    {
        bool supportsLvl = HasFeature(Feature::kLevel);
        if (supportsDefaultOpenLevel & !supportsLvl)
        {
            return false;
        }
        if (supportsLevelStep & !supportsLvl)
        {
            return false;
        }
        return true;
    }
};

struct ClusterInitParameters
{
    DataModel::Nullable<ValveStateEnum> currentState = DataModel::NullNullable;
    DataModel::Nullable<Percent> currentLevel        = DataModel::NullNullable;
    BitMask<ValveFaultBitmap> valveFault             = 0u;
    uint8_t levelStep                                = 1u;
};
struct ClusterState
{
    DataModel::Nullable<ElapsedS> openDuration            = DataModel::NullNullable;
    DataModel::Nullable<ElapsedS> defaultOpenDuration     = DataModel::NullNullable;
    DataModel::Nullable<EpochUs> autoCloseTime            = DataModel::NullNullable;
    QuieterReportingAttribute<ElapsedS> remainingDuration = QuieterReportingAttribute<ElapsedS>();
    DataModel::Nullable<ValveStateEnum> currentState      = DataModel::NullNullable;
    DataModel::Nullable<ValveStateEnum> targetState       = DataModel::NullNullable;
    DataModel::Nullable<Percent> currentLevel             = DataModel::NullNullable;
    DataModel::Nullable<Percent> targetLevel              = DataModel::NullNullable;
    Percent defaultOpenLevel                              = 100u;
    BitMask<ValveFaultBitmap> valveFault                  = 0u;
    uint8_t levelStep                                     = 1u;
};

// Attribute sets are forced to go through this class so the attributes can be marked dirty appropriately.
// This cluster handles storage and marking dirty. The cluster logic should handle constraint checking.
// Conformance checking should be handled in the server layer.
class ClusterStateAttributes
{
public:
    explicit ClusterStateAttributes(MatterContext & matterContext) : mMatterContext(matterContext) {};
    void Init(ClusterInitParameters initialState);
    const ClusterState & GetState() { return mState; }

    CHIP_ERROR SetOpenDuration(const DataModel::Nullable<ElapsedS> & openDuration);
    CHIP_ERROR SetDefaultOpenDuration(const DataModel::Nullable<ElapsedS> & defaultOpenDuration);
    CHIP_ERROR SetAutoCloseTime(const DataModel::Nullable<EpochUs> & autoCloseTime);
    CHIP_ERROR SetRemainingDuration(const DataModel::Nullable<ElapsedS> & remainingDuration);
    CHIP_ERROR SetCurrentState(const DataModel::Nullable<ValveStateEnum> & currentState);
    CHIP_ERROR SetTargetState(const DataModel::Nullable<ValveStateEnum> & targetState);
    CHIP_ERROR SetCurrentLevel(const DataModel::Nullable<Percent> & currentLevel);
    CHIP_ERROR SetTargetLevel(const DataModel::Nullable<Percent> & targetLevel);
    CHIP_ERROR SetDefaultOpenLevel(const Percent defaultOpenLevel);
    CHIP_ERROR SetValveFault(const BitMask<ValveFaultBitmap> & valveFault);
    CHIP_ERROR SetLevelStep(const uint8_t levelStep);

    System::Clock::Milliseconds64 GetNextReportTimeForRemainingDuration();

private:
    const System::Clock::Milliseconds64 kRemainingDurationReportRate =
        std::chrono::duration_cast<System::Clock::Milliseconds64>(System::Clock::Seconds64(1));
    ClusterState mState;
    MatterContext & mMatterContext;
};

class ClusterLogic
{
public:
    // Instantiates a ClusterLogic class. The caller maintains ownership of the driver and the context, but provides them for use by
    // the ClusterLogic class.
    ClusterLogic(DelegateBase & clusterDriver, MatterContext & matterContext) :
        mClusterDriver(clusterDriver), mAttributes(ClusterStateAttributes(matterContext))
    {
        // TODO: remove these once the fields are used properly
        (void) mClusterDriver;
    }

    // Returns CHIP_ERROR_INCORRECT_STATE if the cluster has already been initialized.
    CHIP_ERROR Init(const ClusterInitParameters & initialState = ClusterInitParameters());

    // TODO: Open question - is it better to get the types from the codegen? Attributes::<whatever>>::TypeInfo::
    const DataModel::Nullable<ElapsedS> & GetOpenDuration() { return mAttributes.GetState().openDuration; }
    const DataModel::Nullable<ElapsedS> & GetDefaultOpenDuration() { return mAttributes.GetState().defaultOpenDuration; }
    const DataModel::Nullable<EpochUs> & GetAutoCloseTime() { return mAttributes.GetState().autoCloseTime; }
    // This has an internal handler for the Q quality.
    const DataModel::Nullable<ElapsedS> & GetRemainingDuration();
    const DataModel::Nullable<ValveStateEnum> & GetCurrentState() { return mAttributes.GetState().currentState; };
    const DataModel::Nullable<ValveStateEnum> & GetTargetState() { return mAttributes.GetState().targetState; }
    const DataModel::Nullable<Percent> & GetCurrentLevel() { return mAttributes.GetState().currentLevel; }
    const DataModel::Nullable<Percent> & GetTargetLevel() { return mAttributes.GetState().targetLevel; }
    const Percent & GetDefaultOpenLevel() { return mAttributes.GetState().defaultOpenLevel; }
    const BitMask<ValveFaultBitmap> & GetValveFault() { return mAttributes.GetState().valveFault; }
    const uint8_t & GetLevelStep() { return mAttributes.GetState().levelStep; }
    const uint16_t & GetClusterRevision() { return kClusterRevision; }

    // All Set functions
    // Return CHIP_ERROR_INCORRECT_STATE if the class has not been initialized.
    // Return CHIP_ERROR_INVALID_ARGUMENT if the input value is out of range.
    // Returns CHIP_ERROR_PERSISTED_STORAGE_FAILED if the value could not not be stored in persistent storage.
    // Otherwise return CHIP_NO_ERROR and set the parameter value in the cluster state
    // Set functions are supplied for any values that can be set either internally by the device or externally
    // through a direct attribute write. Changes to attributes that happen as a side effect of cluster commands
    // are handled by the cluster command handlers.

    // DefaultOpenDuration can be set by the client using attribute write
    CHIP_ERROR SetDefaultOpenDuration(const DataModel::Nullable<ElapsedS> & defaultOpenDuration);

    // DefaultOpenLevel can be set by the client using attribute write
    CHIP_ERROR SetDefaultOpenLevel(const uint8_t defaultOpenLevel);

    // ValveFault can be set internally by the device.
    // Use the Set function to add a specific valve fault and the clear function to clear it.
    // Q: Should we push these through the delegate?
    CHIP_ERROR SetValveFault(const ValveFaultBitmap valveFault);
    CHIP_ERROR ClearValveFault(const ValveFaultBitmap valveFault);

    // Other ones that are set internally?
    // Current state
    // Current level

    // Returns CHIP_ERROR_INCORRECT_STATE if the class has not been initialized.
    // Returns CHIP_ERROR_INVALID_ARGUMENT if the input values are out is out of range or the targetLevel is supplied when LVL is
    // not supported.
    // Calls delegate HandleOpen function after validating the parameters
    CHIP_ERROR HandleOpenCommand(std::optional<DataModel::Nullable<ElapsedS>> openDuration, std::optional<Percent> targetLevel);

    // Returns CHIP_ERROR_INCORRECT_STATE if the class has not been initialized.
    // Calls delegate HandleClose function after validating the parameters and stops any open duration timers.
    CHIP_ERROR HandleCloseCommand();

private:
    // This cluster implements version 1 of the valve cluster. Do not change this revision without updating
    // the cluster to implement the newest features.
    // TODO: consider implementing the server such that multiple revisions can be supported
    static constexpr Attributes::ClusterRevision::TypeInfo::Type kClusterRevision = 1u;
    // Determines if the level value is allowed per the level step.
    bool ValueCompliesWithLevelStep(const uint8_t value);
    // Returns the target level to send to the delegate based on the targetLevel command field and the
    // defaults. Returns error if the supplied target level is invalid.
    CHIP_ERROR GetRealTargetLevel(const std::optional<Percent> & targetLevel, Percent & realTargetLevel);
    // Internal function call to handle open commands for devices that support the LVL feature.
    CHIP_ERROR HandleOpenLevel(const std::optional<Percent> & targetLevel);
    // Internal function call to handle open commands for devices that do not support the LVL feature.
    CHIP_ERROR HandleOpenNoLevel();

    // Wrapper for the timer function. Private member so it can call private functions.
    static void HandleUpdateRemainingDuration(System::Layer * systemLayer, void * context);
    // internal function called by wrapper
    void HandleUpdateRemainingDurationInternal();
    // Internal function called by HandleUpdateRemainingDuration to call the close function in the delegate and
    // set all the attributes back to their closed state.
    CHIP_ERROR HandleCloseInternal();

    bool mInitialized                              = false;
    System::Clock::Milliseconds64 mDurationStarted = System::Clock::Milliseconds64(0);

    DelegateBase & mClusterDriver;

    ClusterStateAttributes mAttributes;
};

} // namespace ValveConfigurationAndControl
} // namespace Clusters
} // namespace app
} // namespace chip
