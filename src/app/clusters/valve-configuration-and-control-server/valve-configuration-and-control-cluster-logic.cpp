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

#include "valve-configuration-and-control-cluster-logic.h"

#include <chrono>

#include <app-common/zap-generated/ids/Attributes.h>
#include <lib/core/CHIPError.h>
#include <platform/CHIPDeviceLayer.h>
#include <system/SystemClock.h>

namespace chip {
namespace app {
namespace Clusters {
namespace ValveConfigurationAndControl {

void ClusterStateAttributes::Init(ClusterInitParameters state)
{
    mState.currentLevel = state.currentLevel;
    mState.currentState = state.currentState;
    mState.valveFault   = state.valveFault;
    mState.levelStep    = state.levelStep;

    // Overwrite default initial state with stored persisted values if set.
    uint32_t defaultOpenDuration;
    if (mMatterContext.GetDefaultOpenDuration(defaultOpenDuration) == CHIP_NO_ERROR)
    {
        mState.defaultOpenDuration.SetNonNull(defaultOpenDuration);
    }
    mMatterContext.GetDefaultOpenLevel(mState.defaultOpenLevel);

    QuieterReportingPolicyFlags & policy = mState.remainingDuration.policy();
    policy = QuieterReportingPolicyFlags(to_underlying(QuieterReportingPolicyEnum::kMarkDirtyOnChangeToFromZero) |
                                         to_underlying(QuieterReportingPolicyEnum::kMarkDirtyOnIncrement));
}

CHIP_ERROR ClusterStateAttributes::SetRemainingDuration(const DataModel::Nullable<ElapsedS> & remainingDuration)
{
    System::Clock::Milliseconds64 now = System::SystemClock().GetMonotonicMilliseconds64();
    AttributeDirtyState dirtyState    = mState.remainingDuration.SetValue(
        remainingDuration, now, mState.remainingDuration.GetPredicateForSufficientTimeSinceLastDirty(kRemainingDurationReportRate));
    if (dirtyState == AttributeDirtyState::kMustReport)
    {
        mMatterContext.MarkDirty(Attributes::RemainingDuration::Id);
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterStateAttributes::SetOpenDuration(const DataModel::Nullable<ElapsedS> & openDuration)
{
    bool dirty          = openDuration != mState.openDuration;
    mState.openDuration = openDuration;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::OpenDuration::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetDefaultOpenDuration(const DataModel::Nullable<ElapsedS> & defaultOpenDuration)
{
    bool dirty = defaultOpenDuration != mState.defaultOpenDuration;
    ReturnErrorOnFailure(mMatterContext.StoreDefaultOpenDuration(defaultOpenDuration));
    mState.defaultOpenDuration = defaultOpenDuration;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::DefaultOpenDuration::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetAutoCloseTime(const DataModel::Nullable<EpochUs> & autoCloseTime)
{
    bool dirty           = autoCloseTime != mState.autoCloseTime;
    mState.autoCloseTime = autoCloseTime;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::AutoCloseTime::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetCurrentState(const DataModel::Nullable<ValveStateEnum> & currentState)
{
    bool dirty          = currentState != mState.currentState;
    mState.currentState = currentState;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::CurrentState::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetTargetState(const DataModel::Nullable<ValveStateEnum> & targetState)
{
    bool dirty         = targetState != mState.targetState;
    mState.targetState = targetState;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::TargetState::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetCurrentLevel(const DataModel::Nullable<Percent> & currentLevel)
{
    // TODO: Q quality
    bool dirty          = currentLevel != mState.currentLevel;
    mState.currentLevel = currentLevel;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::CurrentLevel::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetTargetLevel(const DataModel::Nullable<Percent> & targetLevel)
{
    bool dirty         = targetLevel != mState.targetLevel;
    mState.targetLevel = targetLevel;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::TargetLevel::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetDefaultOpenLevel(const Percent defaultOpenLevel)
{
    bool dirty = defaultOpenLevel != mState.defaultOpenLevel;
    ReturnErrorOnFailure(mMatterContext.StoreDefaultOpenLevel(defaultOpenLevel));
    mState.defaultOpenLevel = defaultOpenLevel;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::DefaultOpenLevel::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetValveFault(const BitMask<ValveFaultBitmap> & valveFault)
{
    bool dirty        = valveFault != mState.valveFault;
    mState.valveFault = valveFault;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::ValveFault::Id);
    }
    return CHIP_NO_ERROR;
}
CHIP_ERROR ClusterStateAttributes::SetLevelStep(const uint8_t levelStep)
{
    bool dirty       = levelStep != mState.levelStep;
    mState.levelStep = levelStep;
    if (dirty)
    {
        mMatterContext.MarkDirty(Attributes::LevelStep::Id);
    }
    return CHIP_NO_ERROR;
}

System::Clock::Milliseconds64 ClusterStateAttributes::GetNextReportTimeForRemainingDuration()
{
    return std::chrono::duration_cast<System::Clock::Milliseconds64>(mState.remainingDuration.GetLastReportTime()) +
        kRemainingDurationReportRate;
}

CHIP_ERROR ClusterLogic::Init(const ClusterInitParameters & initialState)
{
    mAttributes.Init(initialState);
    mDurationStarted = System::SystemClock().GetMonotonicMilliseconds64();

    mInitialized = true;
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterLogic::SetDefaultOpenDuration(const DataModel::Nullable<ElapsedS> & defaultOpenDuration)
{
    VerifyOrReturnError(mInitialized, CHIP_ERROR_INCORRECT_STATE);
    if (!defaultOpenDuration.IsNull() && defaultOpenDuration.Value() < 1)
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    return mAttributes.SetDefaultOpenDuration(defaultOpenDuration);
}

bool ClusterLogic::ValueCompliesWithLevelStep(const uint8_t value)
{
    if ((value != 100u) && ((value % mAttributes.GetState().levelStep) != 0))
    {
        return false;
    }
    return true;
}

CHIP_ERROR ClusterLogic::SetDefaultOpenLevel(const uint8_t defaultOpenLevel)
{
    VerifyOrReturnError(mInitialized, CHIP_ERROR_INCORRECT_STATE);
    if (defaultOpenLevel < 1 || defaultOpenLevel > 100)
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    VerifyOrReturnError(ValueCompliesWithLevelStep(defaultOpenLevel), CHIP_ERROR_INVALID_ARGUMENT);
    return mAttributes.SetDefaultOpenLevel(defaultOpenLevel);
}

CHIP_ERROR ClusterLogic::SetValveFault(const ValveFaultBitmap valveFault)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}
CHIP_ERROR ClusterLogic::ClearValveFault(const ValveFaultBitmap valveFault)
{
    return CHIP_ERROR_NOT_IMPLEMENTED;
}

CHIP_ERROR ClusterLogic::GetRealTargetLevel(const std::optional<Percent> & targetLevel, Percent & realTargetLevel)
{
    if (!targetLevel.has_value())
    {
        realTargetLevel = mAttributes.GetState().defaultOpenLevel;
        return CHIP_NO_ERROR;
    }
    // targetLevel has a value
    VerifyOrReturnError(ValueCompliesWithLevelStep(targetLevel.value()), CHIP_ERROR_INVALID_ARGUMENT);
    realTargetLevel = targetLevel.value();
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterLogic::HandleOpenLevel(const std::optional<Percent> & targetLevel)
{
    Percent realTargetLevel;
    Percent returnedCurrentLevel                 = 0;
    BitMask<ValveFaultBitmap> returnedValveFault = 0;
    ReturnErrorOnFailure(GetRealTargetLevel(targetLevel, realTargetLevel));

    CHIP_ERROR err = mClusterDriver.HandleOpenValve(realTargetLevel, returnedCurrentLevel, returnedValveFault);

    mAttributes.SetValveFault(returnedValveFault);
    if (err != CHIP_NO_ERROR)
    {
        return err;
    }

    mAttributes.SetTargetLevel(realTargetLevel);
    mAttributes.SetCurrentLevel(returnedCurrentLevel);
    mAttributes.SetTargetState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen));
    mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning));

    if (returnedCurrentLevel == realTargetLevel)
    {
        mAttributes.SetTargetLevel(DataModel::NullNullable);
        mAttributes.SetCurrentLevel(realTargetLevel);
        mAttributes.SetTargetState(DataModel::NullNullable);
        mAttributes.SetCurrentState(ValveStateEnum::kOpen);
    }
    else
    {
        // TODO: Need to start a timer to continue querying the device for updates. Or just let the delegate handle this?
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterLogic::HandleOpenNoLevel()
{
    ValveStateEnum returnedState                 = ValveStateEnum::kUnknownEnumValue;
    BitMask<ValveFaultBitmap> returnedValveFault = 0;

    // Per the spec, set these to transitioning regardless
    mAttributes.SetTargetState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen));
    mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning));

    CHIP_ERROR err = mClusterDriver.HandleOpenValve(returnedState, returnedValveFault);
    mAttributes.SetValveFault(returnedValveFault);
    if (err != CHIP_NO_ERROR)
    {
        // TODO: How should the target and current be set in this case?
        return err;
    }

    if (returnedState == ValveStateEnum::kOpen)
    {
        mAttributes.SetTargetState(DataModel::NullNullable);
        mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kOpen));
    }
    else
    {
        // TODO: Need to start a timer to continue querying the device for updates. Or just let the delegate handle this?
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterLogic::HandleOpenCommand(std::optional<DataModel::Nullable<ElapsedS>> openDuration,
                                           std::optional<Percent> targetLevel)
{
    // openDuration
    // - if this is omitted, fall back to defaultOpenDuration
    // - if this is NULL, remaining duration is NULL
    // - if this is a value, use that value
    // - if remaining duration is not null and TS is supported, set the autoCloseTime as appropriate
    // targetLevel
    // - if LVL is not supported
    //   - if this is omitted, that's correct
    //   - if this is supplied return error
    // - if LVL is supported
    //   - if this value is not supplied, use defaultOpenLevel if supported, otherwise 100
    //   - if this value is supplied, check against levelStep, error if not OK, otherwise set targetLevel
    VerifyOrReturnError(mInitialized, CHIP_ERROR_INCORRECT_STATE);

    DataModel::Nullable<ElapsedS> realOpenDuration;
    if (openDuration.has_value())
    {
        realOpenDuration = openDuration.value();
    }
    else
    {
        realOpenDuration = mAttributes.GetState().defaultOpenDuration;
    }

    if (mClusterDriver.GetDelegateType() != DelegateType::kLevel && targetLevel.has_value())
    {
        ChipLogError(Zcl, "Received request to open by level, but the valve does not support level")
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    if (mClusterDriver.GetDelegateType() == DelegateType::kLevel)
    {
        ReturnErrorOnFailure(HandleOpenLevel(targetLevel));
    }
    else
    {
        ReturnErrorOnFailure(HandleOpenNoLevel());
    }

    mAttributes.SetOpenDuration(realOpenDuration);
    mDurationStarted = System::SystemClock().GetMonotonicMilliseconds64();
    HandleUpdateRemainingDurationInternal();
    return CHIP_NO_ERROR;
}

CHIP_ERROR ClusterLogic::HandleCloseCommand()
{
    VerifyOrReturnError(mInitialized, CHIP_ERROR_INCORRECT_STATE);
    DeviceLayer::SystemLayer().CancelTimer(HandleUpdateRemainingDuration, this);
    return HandleCloseInternal();
}

CHIP_ERROR ClusterLogic::HandleCloseInternal()
{
    CHIP_ERROR err;
    BitMask<ValveFaultBitmap> faults;
    if (mClusterDriver.GetDelegateType() == DelegateType::kLevel)
    {
        Percent currentLevel;
        mAttributes.SetTargetLevel(0);
        mAttributes.SetTargetState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed));
        mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning));
        err = mClusterDriver.HandleCloseValve(currentLevel, faults);
        if (err == CHIP_NO_ERROR)
        {
            mAttributes.SetCurrentLevel(DataModel::Nullable<Percent>(currentLevel));
            if (currentLevel == 0)
            {
                mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed));
                mAttributes.SetTargetState(DataModel::NullNullable);
                mAttributes.SetTargetLevel(DataModel::NullNullable);
            }
            else
            {
                // TODO: start a timer here to query the delegate?
            }
        }
    }
    else
    {
        ValveStateEnum state;
        mAttributes.SetTargetState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kClosed));
        mAttributes.SetCurrentState(DataModel::Nullable<ValveStateEnum>(ValveStateEnum::kTransitioning));
        err = mClusterDriver.HandleCloseValve(state, faults);
        if (err == CHIP_NO_ERROR && state == ValveStateEnum::kClosed)
        {
            mAttributes.SetCurrentState(state);
            mAttributes.SetTargetState(DataModel::NullNullable);
        }
    }
    // If there was an error, we know nothing about the current state
    if (err != CHIP_NO_ERROR)
    {
        mAttributes.SetCurrentLevel(DataModel::NullNullable);
        mAttributes.SetCurrentState(DataModel::NullNullable);
        mAttributes.SetValveFault(faults);
        mAttributes.SetTargetLevel(DataModel::NullNullable);
        mAttributes.SetTargetState(DataModel::NullNullable);
    }

    mAttributes.SetOpenDuration(DataModel::NullNullable);
    mAttributes.SetRemainingDuration(DataModel::NullNullable);
    mAttributes.SetAutoCloseTime(DataModel::NullNullable);
    return err;
}

void ClusterLogic::HandleUpdateRemainingDuration(System::Layer * systemLayer, void * context)
{
    auto * logic = static_cast<ClusterLogic *>(context);
    logic->HandleUpdateRemainingDurationInternal();
}

void ClusterLogic::HandleUpdateRemainingDurationInternal()
{
    // Start by cancelling the timer in case this was called from a command handler
    // We will start a new timer if required.
    DeviceLayer::SystemLayer().CancelTimer(HandleUpdateRemainingDuration, this);

    if (mAttributes.GetState().openDuration.IsNull())
    {
        // I think this might be an error state - if openDuration is NULL, this timer shouldn't be on.
        mAttributes.SetRemainingDuration(DataModel::NullNullable);
        return;
    }

    // Setup a new timer to either send the next report or handle the close operation
    System::Clock::Milliseconds64 now      = System::SystemClock().GetMonotonicMilliseconds64();
    System::Clock::Seconds64 openDurationS = System::Clock::Seconds64(mAttributes.GetState().openDuration.ValueOr(0));
    System::Clock::Milliseconds64 closeTimeMs =
        mDurationStarted + std::chrono::duration_cast<System::Clock::Milliseconds64>(openDurationS);
    if (now >= closeTimeMs)
    {
        // Time's up, close the valve. Close handles setting the open and remaining duration
        HandleCloseInternal();
        return;
    }
    System::Clock::Milliseconds64 remainingMs     = closeTimeMs - now;
    System::Clock::Milliseconds64 nextReportTimer = mAttributes.GetNextReportTimeForRemainingDuration() - now;

    System::Clock::Milliseconds64 nextTimerTime = std::min(nextReportTimer, remainingMs);
    DeviceLayer::SystemLayer().StartTimer(std::chrono::duration_cast<System::Clock::Timeout>(nextTimerTime),
                                          HandleUpdateRemainingDuration, this);

    auto remainingS = std::chrono::round<System::Clock::Seconds32>(remainingMs);
    mAttributes.SetRemainingDuration(DataModel::Nullable<ElapsedS>(remainingS.count()));
}

const DataModel::Nullable<ElapsedS> & ClusterLogic::GetRemainingDuration()
{
    HandleUpdateRemainingDurationInternal();
    return mAttributes.GetState().remainingDuration.value();
}

} // namespace ValveConfigurationAndControl
} // namespace Clusters
} // namespace app
} // namespace chip
