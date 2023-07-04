/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

#include "on-off-server.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/data-model/Nullable.h>
#include <app/reporting/reporting.h>
#include <app/util/af.h>
#include <app/util/config.h>
#include <app/util/error-mapping.h>
#include <app/util/util.h>

#ifdef EMBER_AF_PLUGIN_SCENES
#include <app/clusters/scenes-server/scenes-server.h>
#endif // EMBER_AF_PLUGIN_SCENES

#ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
#include <app/clusters/level-control/level-control.h>
#endif // EMBER_AF_PLUGIN_LEVEL_CONTROL

#include <platform/CHIPDeviceLayer.h>
#include <platform/PlatformManager.h>

using namespace chip;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::OnOff;
using chip::Protocols::InteractionModel::Status;

#ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
static bool LevelControlWithOnOffFeaturePresent(EndpointId endpoint)
{
    if (!emberAfContainsServer(endpoint, LevelControl::Id))
    {
        return false;
    }

    return LevelControlHasFeature(endpoint, LevelControl::Feature::kOnOff);
}
#endif // EMBER_AF_PLUGIN_LEVEL_CONTROL

static constexpr size_t kOnOffMaxEnpointCount =
    EMBER_AF_ON_OFF_CLUSTER_SERVER_ENDPOINT_COUNT + CHIP_DEVICE_CONFIG_DYNAMIC_ENDPOINT_COUNT;

#ifdef EMBER_AF_PLUGIN_SCENES
static EmberEventControl sceneHandlerEventControls[kOnOffMaxEnpointCount];
static void sceneOnOffCallback(EndpointId endpoint);

class DefaultOnOffSceneHandler : public scenes::DefaultSceneHandlerImpl
{
public:
    /// @brief Struct to keep track of the desired state of the OnOff attribute between ApplyScene and
    /// transition time expiration
    struct EndpointStatePair
    {
        EndpointStatePair(EndpointId endpoint = kInvalidEndpointId, bool status = false) : mEndpoint(endpoint), mState(status) {}
        EndpointId mEndpoint;
        bool mState;
    };

    /// @brief Struct holding an array of EndpointStatePair. Handles insertion, get and removal by EndpointID.
    /// TODO: Implement generic object to handle this boilerplate array manipulation
    struct StatePairBuffer
    {
        bool IsEmpty() const { return (mPairCount == 0); }

        CHIP_ERROR FindPair(const EndpointId endpoint, uint16_t & found_index) const
        {
            VerifyOrReturnError(!IsEmpty(), CHIP_ERROR_NOT_FOUND);
            for (found_index = 0; found_index < mPairCount; found_index++)
            {
                if (endpoint == mStatePairBuffer[found_index].mEndpoint)
                {
                    return CHIP_NO_ERROR;
                }
            }

            return CHIP_ERROR_NOT_FOUND;
        }

        CHIP_ERROR InsertPair(const EndpointStatePair & status)
        {
            uint16_t idx;
            CHIP_ERROR err = FindPair(status.mEndpoint, idx);

            if (CHIP_NO_ERROR == err)
            {
                mStatePairBuffer[idx] = status;
            }
            else if (mPairCount < MAX_ENDPOINT_COUNT)
            {
                // if not found, insert at the end
                mStatePairBuffer[mPairCount] = status;
                mPairCount++;
            }
            else
            {
                return CHIP_ERROR_NO_MEMORY;
            }

            return CHIP_NO_ERROR;
        }

        CHIP_ERROR GetPair(const EndpointId endpoint, EndpointStatePair & status) const
        {
            uint16_t idx;
            ReturnErrorOnFailure(FindPair(endpoint, idx));

            status = mStatePairBuffer[idx];
            return CHIP_NO_ERROR;
        }

        /// @brief Removes Pair and decrements Pair count if the endpoint existed in the array
        /// @param endpoint : endpoint id of the pair
        CHIP_ERROR RemovePair(const EndpointId endpoint)
        {
            uint16_t position;
            VerifyOrReturnValue(CHIP_NO_ERROR == FindPair(endpoint, position), CHIP_NO_ERROR);

            uint16_t nextPos = static_cast<uint16_t>(position + 1);
            uint16_t moveNum = static_cast<uint16_t>(mPairCount - nextPos);

            // Compress array after removal, if the removed position is not the last
            if (moveNum)
            {
                memmove(&mStatePairBuffer[position], &mStatePairBuffer[nextPos], sizeof(EndpointStatePair) * moveNum);
            }

            mPairCount--;
            // Clear last occupied position
            mStatePairBuffer[mPairCount].mEndpoint = kInvalidEndpointId;

            return CHIP_NO_ERROR;
        }

        uint16_t mPairCount;
        EndpointStatePair mStatePairBuffer[kOnOffMaxEnpointCount];
    };

    StatePairBuffer mSceneEndpointStatePairs;
    // As per spec, 1 attribute is scenable in the on off cluster
    static constexpr uint8_t scenableAttributeCount = 1;

    DefaultOnOffSceneHandler() = default;
    ~DefaultOnOffSceneHandler() override {}

    // Default function for OnOff cluster, only puts the OnOff cluster ID in the span if supported on the given endpoint
    virtual void GetSupportedClusters(EndpointId endpoint, Span<ClusterId> & clusterBuffer) override
    {
        ClusterId * buffer = clusterBuffer.data();
        if (emberAfContainsServer(endpoint, OnOff::Id) && clusterBuffer.size() >= 1)
        {
            buffer[0] = OnOff::Id;
            clusterBuffer.reduce_size(1);
        }
        else
        {
            clusterBuffer.reduce_size(0);
        }
    }

    // Default function for OnOff cluster, only checks if OnOff is enabled on the endpoint
    bool SupportsCluster(EndpointId endpoint, ClusterId cluster) override
    {
        return (cluster == OnOff::Id) && (emberAfContainsServer(endpoint, OnOff::Id));
    }

    /// @brief Serialize the Cluster's EFS value
    /// @param endpoint target endpoint
    /// @param cluster  target cluster
    /// @param serializedBytes data to serialize into EFS
    /// @return CHIP_NO_ERROR if successfully serialized the data, CHIP_ERROR_INVALID_ARGUMENT otherwise
    CHIP_ERROR SerializeSave(EndpointId endpoint, ClusterId cluster, MutableByteSpan & serializedBytes) override
    {
        using AttributeValuePair = Scenes::Structs::AttributeValuePair::Type;

        bool currentValue;
        // read current on/off value
        EmberAfStatus status = Attributes::OnOff::Get(endpoint, &currentValue);
        if (status != EMBER_ZCL_STATUS_SUCCESS)
        {
            ChipLogError(Zcl, "ERR: reading on/off %x", status);
            return CHIP_ERROR_READ_FAILED;
        }

        AttributeValuePair pairs[scenableAttributeCount];

        pairs[0].attributeID    = Attributes::OnOff::Id;
        pairs[0].attributeValue = currentValue;

        app::DataModel::List<AttributeValuePair> attributeValueList(pairs);

        return EncodeAttributeValueList(attributeValueList, serializedBytes);
    }

    /// @brief Default EFS interaction when applying scene to the OnOff Cluster
    /// @param endpoint target endpoint
    /// @param cluster  target cluster
    /// @param serializedBytes Data from nvm
    /// @param timeMs transition time in ms
    /// @return CHIP_NO_ERROR if value as expected, CHIP_ERROR_INVALID_ARGUMENT otherwise
    CHIP_ERROR ApplyScene(EndpointId endpoint, ClusterId cluster, const ByteSpan & serializedBytes,
                          scenes::TransitionTimeMs timeMs) override
    {
        app::DataModel::DecodableList<Scenes::Structs::AttributeValuePair::DecodableType> attributeValueList;

        VerifyOrReturnError(cluster == OnOff::Id, CHIP_ERROR_INVALID_ARGUMENT);

        ReturnErrorOnFailure(DecodeAttributeValueList(serializedBytes, attributeValueList));

        size_t attributeCount = 0;
        ReturnErrorOnFailure(attributeValueList.ComputeSize(&attributeCount));
        VerifyOrReturnError(attributeCount <= scenableAttributeCount, CHIP_ERROR_BUFFER_TOO_SMALL);

        auto pair_iterator = attributeValueList.begin();
        while (pair_iterator.Next())
        {
            auto & decodePair = pair_iterator.GetValue();
            VerifyOrReturnError(decodePair.attributeID == Attributes::OnOff::Id, CHIP_ERROR_INVALID_ARGUMENT);
            ReturnErrorOnFailure(
                mSceneEndpointStatePairs.InsertPair(EndpointStatePair(endpoint, static_cast<bool>(decodePair.attributeValue))));
        }
        // Verify that the EFS was completely read
        CHIP_ERROR err = pair_iterator.GetStatus();
        if (CHIP_NO_ERROR != err)
        {
            mSceneEndpointStatePairs.RemovePair(endpoint);
            return err;
        }

        OnOffServer::Instance().scheduleTimerCallbackMs(sceneEventControl(endpoint), timeMs);

        return CHIP_NO_ERROR;
    }

private:
    /**
     * @brief Configures EventControl callback when setting On Off through scenes callback
     *
     * @param[in] endpoint endpoint to start timer for
     * @return EmberEventControl* configured event control
     */
    EmberEventControl * sceneEventControl(EndpointId endpoint)
    {
        EmberEventControl * controller =
            OnOffServer::Instance().getEventControl(endpoint, Span<EmberEventControl>(sceneHandlerEventControls));
        VerifyOrReturnValue(controller != nullptr, nullptr);

        controller->endpoint = endpoint;
        controller->callback = &sceneOnOffCallback;

        return controller;
    }
};
static DefaultOnOffSceneHandler sOnOffSceneHandler;

static void sceneOnOffCallback(EndpointId endpoint)
{
    DefaultOnOffSceneHandler::EndpointStatePair savedState;
    ReturnOnFailure(sOnOffSceneHandler.mSceneEndpointStatePairs.GetPair(endpoint, savedState));
    chip::CommandId command = (savedState.mState) ? Commands::On::Id : Commands::Off::Id;
    OnOffServer::Instance().setOnOffValue(endpoint, command, false);
    ReturnOnFailure(sOnOffSceneHandler.mSceneEndpointStatePairs.RemovePair(endpoint));
}
#endif // EMBER_AF_PLUGIN_SCENES

/**********************************************************
 * Attributes Definition
 *********************************************************/

static OnOffEffect * firstEffect = nullptr;
OnOffServer OnOffServer::instance;
static EmberEventControl gEventControls[kOnOffMaxEnpointCount];

/**********************************************************
 * Function definition
 *********************************************************/

static OnOffEffect * inst(EndpointId endpoint);

/**********************************************************
 * Matter timer scheduling glue logic
 *********************************************************/

void OnOffServer::timerCallback(System::Layer *, void * callbackContext)
{
    auto control = static_cast<EmberEventControl *>(callbackContext);
    (control->callback)(control->endpoint);
}

void OnOffServer::scheduleTimerCallbackMs(EmberEventControl * control, uint32_t delayMs)
{
    CHIP_ERROR err = DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(delayMs), timerCallback, control);

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "OnOff Server failed to schedule event: %" CHIP_ERROR_FORMAT, err.Format());
    }
}

void OnOffServer::cancelEndpointTimerCallback(EmberEventControl * control)
{
    DeviceLayer::SystemLayer().CancelTimer(timerCallback, control);
}

void OnOffServer::cancelEndpointTimerCallback(EndpointId endpoint)
{
    auto control = OnOffServer::getEventControl(endpoint, Span<EmberEventControl>(gEventControls));
    if (control)
    {
        cancelEndpointTimerCallback(control);
    }
}

void MatterOnOffClusterServerShutdownCallback(EndpointId endpoint)
{
    ChipLogProgress(Zcl, "Shuting down on/off server cluster on endpoint %d", endpoint);
    OnOffServer::Instance().cancelEndpointTimerCallback(endpoint);
}

/**********************************************************
 * OnOff Implementation
 *********************************************************/

OnOffServer & OnOffServer::Instance()
{
    return instance;
}

chip::scenes::SceneHandler * OnOffServer::GetSceneHandler()
{

#ifdef EMBER_AF_PLUGIN_SCENES
    return &sOnOffSceneHandler;
#else
    return nullptr;
#endif // EMBER_AF_PLUGIN_SCENES
}

bool OnOffServer::HasFeature(chip::EndpointId endpoint, Feature feature)
{
    bool success;
    uint32_t featureMap;
    success = (Attributes::FeatureMap::Get(endpoint, &featureMap) == EMBER_ZCL_STATUS_SUCCESS);

    return success ? ((featureMap & to_underlying(feature)) != 0) : false;
}

EmberAfStatus OnOffServer::getOnOffValue(chip::EndpointId endpoint, bool * currentOnOffValue)
{
    // read current on/off value
    EmberAfStatus status = Attributes::OnOff::Get(endpoint, currentOnOffValue);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogProgress(Zcl, "ERR: reading on/off %x", status);
    }

    ChipLogProgress(Zcl, "On/Off ep%d value: %d", endpoint, *currentOnOffValue);

    return status;
}

/** @brief On/off Cluster Set Value
 *
 * This function is called when the on/off value needs to be set, either through
 * normal channels or as a result of a level change.
 *
 * @param endpoint   Ver.: always
 * @param command   Ver.: always
 * @param initiatedByLevelChange   Ver.: always
 */
EmberAfStatus OnOffServer::setOnOffValue(chip::EndpointId endpoint, chip::CommandId command, bool initiatedByLevelChange)
{
    EmberAfStatus status;
    bool currentValue, newValue;

    // read current on/off value
    status = Attributes::OnOff::Get(endpoint, &currentValue);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogProgress(Zcl, "ERR: reading on/off %x", status);
        return status;
    }

    // if the value is already what we want to set it to then do nothing
    if ((!currentValue && command == Commands::Off::Id) || (currentValue && command == Commands::On::Id))
    {
        ChipLogProgress(Zcl, "Endpoint %x On/off already set to new value", endpoint);
        return EMBER_ZCL_STATUS_SUCCESS;
    }

    // we either got a toggle, or an on when off, or an off when on,
    // so we need to swap the value
    newValue = !currentValue;
    ChipLogProgress(Zcl, "Toggle ep%x on/off from state %x to %x", endpoint, currentValue, newValue);

    // the sequence of updating on/off attribute and kick off level change effect should
    // be depend on whether we are turning on or off. If we are turning on the light, we
    // should update the on/off attribute before kicking off level change, if we are
    // turning off the light, we should do the opposite, that is kick off level change
    // before updating the on/off attribute.
    if (newValue) // Set On
    {
        if (SupportsLightingApplications(endpoint))
        {
            uint16_t onTime = 0;
            Attributes::OnTime::Get(endpoint, &onTime);

            if (onTime == 0)
            {
                ChipLogProgress(Zcl, "On Command - OffWaitTime :  0");
                Attributes::OffWaitTime::Set(endpoint, 0);

                // Stop timer on the endpoint
                EmberEventControl * event = getEventControl(endpoint, Span<EmberEventControl>(gEventControls));
                if (event != nullptr)
                {
                    cancelEndpointTimerCallback(event);
                    ChipLogProgress(Zcl, "On/Toggle Command - Stop Timer");
                }
            }

            Attributes::GlobalSceneControl::Set(endpoint, true);
        }

        // write the new on/off value
        status = Attributes::OnOff::Set(endpoint, newValue);
        if (status != EMBER_ZCL_STATUS_SUCCESS)
        {
            ChipLogProgress(Zcl, "ERR: writing on/off %x", status);
            return status;
        }

#ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
        // If initiatedByLevelChange is false, then we assume that the level change
        // ZCL stuff has not happened and we do it here
        if (!initiatedByLevelChange && LevelControlWithOnOffFeaturePresent(endpoint))
        {
            emberAfOnOffClusterLevelControlEffectCallback(endpoint, newValue);
        }
#endif
#ifdef EMBER_AF_PLUGIN_MODE_SELECT
        // If OnMode is not a null value, then change the current mode to it.
        if (emberAfContainsServer(endpoint, ModeSelect::Id) &&
            emberAfContainsAttribute(endpoint, ModeSelect::Id, ModeSelect::Attributes::OnMode::Id))
        {
            ModeSelect::Attributes::OnMode::TypeInfo::Type onMode;
            if (ModeSelect::Attributes::OnMode::Get(endpoint, onMode) == EMBER_ZCL_STATUS_SUCCESS && !onMode.IsNull())
            {
                ChipLogProgress(Zcl, "Changing Current Mode to %x", onMode.Value());
                status = ModeSelect::Attributes::CurrentMode::Set(endpoint, onMode.Value());
            }
        }
#endif
    }
    else // Set Off
    {
#ifdef EMBER_AF_PLUGIN_LEVEL_CONTROL
        // If initiatedByLevelChange is false, then we assume that the level change
        // ZCL stuff has not happened and we do it here
        if (!initiatedByLevelChange && LevelControlWithOnOffFeaturePresent(endpoint))
        {
            emberAfOnOffClusterLevelControlEffectCallback(endpoint, newValue);
        }
        else
#endif
        {
            // write the new on/off value
            status = Attributes::OnOff::Set(endpoint, newValue);
            if (status != EMBER_ZCL_STATUS_SUCCESS)
            {
                ChipLogProgress(Zcl, "ERR: writing on/off %x", status);
                return status;
            }

            if (SupportsLightingApplications(endpoint))
            {
                ChipLogProgress(Zcl, "Off completed. reset OnTime to  0");
                Attributes::OnTime::Set(endpoint, 0); // Reset onTime
            }
        }
    }

#ifdef EMBER_AF_PLUGIN_SCENES
    //  the scene has been changed (the value of on/off has changed) so
    //  the current scene as described in the attribute table is invalid,
    //  so mark it as invalid (just writes the valid/invalid attribute)

    Scenes::ScenesServer::Instance().MakeSceneInvalid(endpoint);
#endif // EMBER_AF_PLUGIN_SCENES

    // The returned status is based solely on the On/Off cluster.  Errors in the
    // Level Control and/or Scenes cluster are ignored.
    return EMBER_ZCL_STATUS_SUCCESS;
}

void OnOffServer::initOnOffServer(chip::EndpointId endpoint)
{
#ifndef IGNORE_ON_OFF_CLUSTER_START_UP_ON_OFF
    // StartUp behavior relies on OnOff and StartUpOnOff attributes being non-volatile.
    if (SupportsLightingApplications(endpoint) && areStartUpOnOffServerAttributesNonVolatile(endpoint))
    {
        // Read the StartUpOnOff attribute and set the OnOff attribute as per
        // following from zcl 7 14-0127-20i-zcl-ch-3-general.doc.
        // 3.8.2.2.5	StartUpOnOff Attribute
        // The StartUpOnOff attribute SHALL define the desired startup behavior of a
        // lamp device when it is supplied with power and this state SHALL be
        // reflected in the OnOff attribute.  The values of the StartUpOnOff
        // attribute are listed below.
        // Table 3 46. Values of the StartUpOnOff Attribute
        // Value      Action on power up
        // 0x00       Set the OnOff attribute to 0 (off).
        // 0x01       Set the OnOff attribute to 1 (on).
        // 0x02       If the previous value of the OnOff attribute is equal to 0,
        //            set the OnOff attribute to 1.If the previous value of the OnOff
        //            attribute is equal to 1, set the OnOff attribute to 0 (toggle).
        // 0x03-0xfe  These values are reserved.  No action.
        // 0xff      This value cannot happen.
        // null       Set the OnOff attribute to its previous value.

        bool onOffValueForStartUp = false;
        EmberAfStatus status      = getOnOffValueForStartUp(endpoint, onOffValueForStartUp);
        if (status == EMBER_ZCL_STATUS_SUCCESS)
        {
            status = setOnOffValue(endpoint, onOffValueForStartUp, true);
        }

#ifdef EMBER_AF_PLUGIN_SCENES
        // Registers Scene handlers for the On/Off cluster on the server
        app::Clusters::Scenes::ScenesServer::Instance().RegisterSceneHandler(OnOffServer::Instance().GetSceneHandler());
#endif

#ifdef EMBER_AF_PLUGIN_MODE_SELECT
        // If OnMode is not a null value, then change the current mode to it.
        if (onOffValueForStartUp && emberAfContainsServer(endpoint, ModeSelect::Id) &&
            emberAfContainsAttribute(endpoint, ModeSelect::Id, ModeSelect::Attributes::OnMode::Id))
        {
            ModeSelect::Attributes::OnMode::TypeInfo::Type onMode;
            if (ModeSelect::Attributes::OnMode::Get(endpoint, onMode) == EMBER_ZCL_STATUS_SUCCESS && !onMode.IsNull())
            {
                ChipLogProgress(Zcl, "Changing Current Mode to %x", onMode.Value());
                status = ModeSelect::Attributes::CurrentMode::Set(endpoint, onMode.Value());
            }
        }
#endif
    }
#endif // IGNORE_ON_OFF_CLUSTER_START_UP_ON_OFF

    emberAfPluginOnOffClusterServerPostInitCallback(endpoint);
}

/** @brief Get the OnOff value when server starts.
 *
 * This function determines how StartUpOnOff affects the OnOff value when the server starts.
 *
 * @param endpoint   Ver.: always
 * @param onOffValueForStartUp Ver.: always
 */
EmberAfStatus OnOffServer::getOnOffValueForStartUp(chip::EndpointId endpoint, bool & onOffValueForStartUp)
{
    app::DataModel::Nullable<OnOff::OnOffStartUpOnOff> startUpOnOff;
    EmberAfStatus status = Attributes::StartUpOnOff::Get(endpoint, startUpOnOff);
    if (status == EMBER_ZCL_STATUS_SUCCESS)
    {
        // Initialise updated value to 0
        bool updatedOnOff = false;
        status            = Attributes::OnOff::Get(endpoint, &updatedOnOff);
        if (status == EMBER_ZCL_STATUS_SUCCESS)
        {
            if (!startUpOnOff.IsNull())
            {
                switch (startUpOnOff.Value())
                {
                case OnOff::OnOffStartUpOnOff::kOff:
                    updatedOnOff = false; // Off
                    break;
                case OnOff::OnOffStartUpOnOff::kOn:
                    updatedOnOff = true; // On
                    break;
                case OnOff::OnOffStartUpOnOff::kTogglePreviousOnOff:
                    updatedOnOff = !updatedOnOff;
                    break;
                default:
                    // All other values 0x03- 0xFE are reserved - no action.
                    break;
                }
            }
            onOffValueForStartUp = updatedOnOff;
        }
    }
    return status;
}

bool OnOffServer::offCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath)
{
    EmberAfStatus status = setOnOffValue(commandPath.mEndpointId, Commands::Off::Id, false);

    commandObj->AddStatus(commandPath, app::ToInteractionModelStatus(status));
    return true;
}

bool OnOffServer::onCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath)
{
    EmberAfStatus status = setOnOffValue(commandPath.mEndpointId, Commands::On::Id, false);

    commandObj->AddStatus(commandPath, app::ToInteractionModelStatus(status));
    return true;
}

bool OnOffServer::toggleCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath)
{
    EmberAfStatus status = setOnOffValue(commandPath.mEndpointId, Commands::Toggle::Id, false);

    commandObj->AddStatus(commandPath, app::ToInteractionModelStatus(status));
    return true;
}

bool OnOffServer::offWithEffectCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                       const Commands::OffWithEffect::DecodableType & commandData)
{
    auto effectId             = commandData.effectIdentifier;
    auto effectVariant        = commandData.effectVariant;
    chip::EndpointId endpoint = commandPath.mEndpointId;
    Status status             = Status::Success;

    if (SupportsLightingApplications(endpoint))
    {
#ifdef EMBER_AF_PLUGIN_SCENES
        FabricIndex fabric = commandObj->GetAccessingFabricIndex();
#endif // EMBER_AF_PLUGIN_SCENES
        bool globalSceneControl = false;
        OnOff::Attributes::GlobalSceneControl::Get(endpoint, &globalSceneControl);

        bool isOnBeforeCommand = false;
        OnOff::Attributes::OnOff::Get(endpoint, &isOnBeforeCommand);

        if (globalSceneControl)
        {
#ifdef EMBER_AF_PLUGIN_SCENES
            GroupId groupId = ZCL_SCENES_GLOBAL_SCENE_GROUP_ID;
            if (commandObj->GetExchangeContext()->IsGroupExchangeContext())
            {
                groupId = commandObj->GetExchangeContext()->GetSessionHandle()->AsIncomingGroupSession()->GetGroupId();
            }
            Scenes::ScenesServer::Instance().StoreCurrentScene(fabric, endpoint, groupId, ZCL_SCENES_GLOBAL_SCENE_SCENE_ID);
#endif // EMBER_AF_PLUGIN_SCENES
            OnOff::Attributes::GlobalSceneControl::Set(endpoint, false);
        }

        // Only apply effect if OnOff is on
        if (isOnBeforeCommand)
        {
            OnOffEffect * effect = inst(endpoint);

            if (effect != nullptr && effect->mOffWithEffectTrigger != nullptr)
            {
                effect->mEffectIdentifier = effectId;
                effect->mEffectVariant    = effectVariant;

                effect->mOffWithEffectTrigger(effect);
            }
        }

        status = app::ToInteractionModelStatus(setOnOffValue(endpoint, Commands::Off::Id, false));
    }
    else
    {
        status = Status::UnsupportedCommand;
    }

    commandObj->AddStatus(commandPath, status);
    return true;
}

bool OnOffServer::OnWithRecallGlobalSceneCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath)
{
    chip::EndpointId endpoint = commandPath.mEndpointId;

    if (!SupportsLightingApplications(endpoint))
    {
        commandObj->AddStatus(commandPath, Status::UnsupportedCommand);
        return true;
    }

#ifdef EMBER_AF_PLUGIN_SCENES
    FabricIndex fabric = commandObj->GetAccessingFabricIndex();
#endif // EMBER_AF_PLUGIN_SCENES

    bool globalSceneControl = false;
    OnOff::Attributes::GlobalSceneControl::Get(endpoint, &globalSceneControl);

    if (globalSceneControl)
    {
        commandObj->AddStatus(commandPath, Status::Success);
        return true;
    }

#ifdef EMBER_AF_PLUGIN_SCENES
    GroupId groupId = ZCL_SCENES_GLOBAL_SCENE_GROUP_ID;
    if (commandObj->GetExchangeContext()->IsGroupExchangeContext())
    {
        groupId = commandObj->GetExchangeContext()->GetSessionHandle()->AsIncomingGroupSession()->GetGroupId();
    }

    Scenes::ScenesServer::Instance().RecallScene(fabric, endpoint, groupId, ZCL_SCENES_GLOBAL_SCENE_SCENE_ID);
#endif // EMBER_AF_PLUGIN_SCENES

    OnOff::Attributes::GlobalSceneControl::Set(endpoint, true);
    setOnOffValue(endpoint, Commands::On::Id, false);

    commandObj->AddStatus(commandPath, Status::Success);
    return true;
}

uint32_t OnOffServer::calculateNextWaitTimeMS()
{
    const chip::System::Clock::Timestamp currentTime = chip::System::SystemClock().GetMonotonicTimestamp();
    chip::System::Clock::Timestamp waitTime          = UPDATE_TIME_MS;
    chip::System::Clock::Timestamp latency;

    if (currentTime > nextDesiredOnWithTimedOffTimestamp)
    {
        latency = currentTime - nextDesiredOnWithTimedOffTimestamp;
        if (latency >= UPDATE_TIME_MS)
            waitTime = chip::System::Clock::Milliseconds32(1);
        else
            waitTime -= latency;
    }

    nextDesiredOnWithTimedOffTimestamp += UPDATE_TIME_MS;

    return (uint32_t) waitTime.count();
}

bool OnOffServer::OnWithTimedOffCommand(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                        const Commands::OnWithTimedOff::DecodableType & commandData)
{
    BitFlags<OnOffControl> onOffControl = commandData.onOffControl;
    uint16_t onTime                     = commandData.onTime;
    uint16_t offWaitTime                = commandData.offWaitTime;
    Status status                       = Status::Success;
    chip::EndpointId endpoint           = commandPath.mEndpointId;
    bool isOn                           = false;
    uint16_t currentOffWaitTime         = MAX_TIME_VALUE;
    uint16_t currentOnTime              = 0;

    EmberEventControl * event = configureEventControl(endpoint);
    VerifyOrExit(event != nullptr, status = Status::UnsupportedEndpoint);
    VerifyOrExit(SupportsLightingApplications(endpoint), status = Status::UnsupportedCommand);

    OnOff::Attributes::OnOff::Get(endpoint, &isOn);

    // OnOff is off and the commands is only accepted if on
    if (onOffControl.Has(OnOffControl::kAcceptOnlyWhenOn) && !isOn)
    {
        commandObj->AddStatus(commandPath, Status::Success);
        return true;
    }

    OnOff::Attributes::OffWaitTime::Get(endpoint, &currentOffWaitTime);
    OnOff::Attributes::OnTime::Get(endpoint, &currentOnTime);

    if (currentOffWaitTime > 0 && !isOn)
    {
        uint16_t newOffWaitTime = currentOffWaitTime < offWaitTime ? currentOffWaitTime : offWaitTime;
        OnOff::Attributes::OffWaitTime::Set(endpoint, newOffWaitTime);

        currentOffWaitTime = newOffWaitTime;
    }
    else
    {
        uint16_t newOnTime = currentOnTime > onTime ? currentOnTime : onTime;
        OnOff::Attributes::OnTime::Set(endpoint, newOnTime);

        OnOff::Attributes::OffWaitTime::Set(endpoint, offWaitTime);
        setOnOffValue(endpoint, Commands::On::Id, false);

        currentOnTime      = newOnTime;
        currentOffWaitTime = offWaitTime;
    }

    ChipLogProgress(Zcl, "On Time:  %d | off wait Time: %d", currentOnTime, currentOffWaitTime);

    if (currentOnTime < MAX_TIME_VALUE && currentOffWaitTime < MAX_TIME_VALUE)
    {
        nextDesiredOnWithTimedOffTimestamp = chip::System::SystemClock().GetMonotonicTimestamp() + UPDATE_TIME_MS;
        scheduleTimerCallbackMs(configureEventControl(endpoint), (uint32_t) UPDATE_TIME_MS.count());
    }

exit:
    commandObj->AddStatus(commandPath, status);
    return true;
}

/**
 * @brief Updates OnOff values after timer is finished
 *
 * @param[in] endpoint endpoint associated with the finished timer
 */
void OnOffServer::updateOnOffTimeCommand(chip::EndpointId endpoint)
{
    ChipLogProgress(Zcl, "Timer callback - Entering callbackc");

    bool isOn = false;
    OnOff::Attributes::OnOff::Get(endpoint, &isOn);

    if (isOn) // OnOff On case
    {
        // Restart Timer
        scheduleTimerCallbackMs(configureEventControl(endpoint), calculateNextWaitTimeMS());

        // Update onTime values
        uint16_t onTime = MIN_TIME_VALUE;
        OnOff::Attributes::OnTime::Get(endpoint, &onTime);
        ChipLogProgress(Zcl, "Timer callback - On Time:  %d", onTime);

        if (onTime > 0)
        {
            onTime--;
            OnOff::Attributes::OnTime::Set(endpoint, onTime);
        }

        if (onTime == 0)
        {
            ChipLogProgress(Zcl, "Timer callback - Turning off OnOff");

            OnOff::Attributes::OffWaitTime::Set(endpoint, 0);
            setOnOffValue(endpoint, Commands::Off::Id, false);
        }
    }
    else // OnOff Off Case
    {
        uint16_t offWaitTime = 0;
        OnOff::Attributes::OffWaitTime::Get(endpoint, &offWaitTime);

        // Validate before decreasing counter
        if (offWaitTime > 0)
        {
            offWaitTime--;
            OnOff::Attributes::OffWaitTime::Set(endpoint, offWaitTime);
        }

        ChipLogProgress(Zcl, "Timer Callback - wait Off Time:  %d", offWaitTime);

        // Validate if necessary to restart timer
        if (offWaitTime > 0)
        {
            // Restart Timer
            scheduleTimerCallbackMs(configureEventControl(endpoint), calculateNextWaitTimeMS());
        }
        else
        {
            ChipLogProgress(Zcl, "Timer  Callback - wait Off Time cycle finished");

            // Stop timer on the endpoint
            cancelEndpointTimerCallback(getEventControl(endpoint, Span<EmberEventControl>(gEventControls)));
        }
    }
}

#ifndef IGNORE_ON_OFF_CLUSTER_START_UP_ON_OFF
bool OnOffServer::areStartUpOnOffServerAttributesNonVolatile(EndpointId endpoint)
{
    return !emberAfIsKnownVolatileAttribute(endpoint, OnOff::Id, Attributes::OnOff::Id) &&
        !emberAfIsKnownVolatileAttribute(endpoint, OnOff::Id, Attributes::StartUpOnOff::Id);
}
#endif // IGNORE_ON_OFF_CLUSTER_START_UP_ON_OFF

/**
 * @brief event control object for an endpoint
 *
 * @param[in] endpoint target endpoint
 * @param[in] eventControlArray Array where to find the event control
 * @param[in] eventControlArraySize Size of the event control array
 * @return EmberEventControl* configured event control
 */
EmberEventControl * OnOffServer::getEventControl(EndpointId endpoint, const Span<EmberEventControl> & eventControlArray)
{
    uint16_t index = emberAfGetClusterServerEndpointIndex(endpoint, OnOff::Id, EMBER_AF_ON_OFF_CLUSTER_SERVER_ENDPOINT_COUNT);
    if (index >= eventControlArray.size())
    {
        return nullptr;
    }

    return &eventControlArray[index];
}

/**
 * @brief Configures EventControl callback when using XY colors
 *
 * @param[in] endpoint endpoint to start timer for
 * @return EmberEventControl* configured event control
 */
EmberEventControl * OnOffServer::configureEventControl(EndpointId endpoint)
{
    EmberEventControl * controller = getEventControl(endpoint, Span<EmberEventControl>(gEventControls));
    VerifyOrReturnError(controller != nullptr, nullptr);

    controller->endpoint = endpoint;
    controller->callback = &onOffWaitTimeOffEventHandler;

    return controller;
}

/**********************************************************
 * OnOffEffect Implementation
 *********************************************************/

static OnOffEffect * inst(EndpointId endpoint)
{
    OnOffEffect * current = firstEffect;
    while (current != nullptr && current->mEndpoint != endpoint)
    {
        current = current->next();
    }

    return current;
}

static inline void reg(OnOffEffect * inst)
{
    inst->setNext(firstEffect);
    firstEffect = inst;
}

static inline void unreg(OnOffEffect * inst)
{
    if (firstEffect == inst)
    {
        firstEffect = firstEffect->next();
    }
    else
    {
        OnOffEffect * previous = firstEffect;
        OnOffEffect * current  = firstEffect->next();

        while (current != nullptr && current != inst)
        {
            previous = current;
            current  = current->next();
        }

        if (current != nullptr)
        {
            previous->setNext(current->next());
        }
    }
}

OnOffEffect::OnOffEffect(chip::EndpointId endpoint, OffWithEffectTriggerCommand offWithEffectTrigger,
                         OnOffEffectIdentifier effectIdentifier, uint8_t effectVariant) :
    mEndpoint(endpoint),
    mOffWithEffectTrigger(offWithEffectTrigger), mEffectIdentifier(effectIdentifier), mEffectVariant(effectVariant)
{
    reg(this);
};

OnOffEffect::~OnOffEffect()
{
    unreg(this);
};

/**********************************************************
 * Callbacks Implementation
 *********************************************************/

bool emberAfOnOffClusterOffCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                    const Commands::Off::DecodableType & commandData)
{
    return OnOffServer::Instance().offCommand(commandObj, commandPath);
}

bool emberAfOnOffClusterOnCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                   const Commands::On::DecodableType & commandData)
{
    return OnOffServer::Instance().onCommand(commandObj, commandPath);
}

bool emberAfOnOffClusterToggleCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                       const Commands::Toggle::DecodableType & commandData)
{
    return OnOffServer::Instance().toggleCommand(commandObj, commandPath);
}

bool emberAfOnOffClusterOffWithEffectCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                              const Commands::OffWithEffect::DecodableType & commandData)
{
    return OnOffServer::Instance().offWithEffectCommand(commandObj, commandPath, commandData);
}

bool emberAfOnOffClusterOnWithRecallGlobalSceneCallback(app::CommandHandler * commandObj,
                                                        const app::ConcreteCommandPath & commandPath,
                                                        const Commands::OnWithRecallGlobalScene::DecodableType & commandData)
{
    return OnOffServer::Instance().OnWithRecallGlobalSceneCommand(commandObj, commandPath);
}

bool emberAfOnOffClusterOnWithTimedOffCallback(app::CommandHandler * commandObj, const app::ConcreteCommandPath & commandPath,
                                               const Commands::OnWithTimedOff::DecodableType & commandData)
{
    return OnOffServer::Instance().OnWithTimedOffCommand(commandObj, commandPath, commandData);
}

void emberAfOnOffClusterServerInitCallback(chip::EndpointId endpoint)
{
    OnOffServer::Instance().initOnOffServer(endpoint);
}

void onOffWaitTimeOffEventHandler(chip::EndpointId endpoint)
{
    OnOffServer::Instance().updateOnOffTimeCommand(endpoint);
}

void emberAfPluginOnOffClusterServerPostInitCallback(EndpointId endpoint) {}

void MatterOnOffPluginServerInitCallback() {}
