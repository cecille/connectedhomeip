/**
 *
 *    Copyright (c) 2024 Project CHIP Authors
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
 *
 */

#include "valve-configuration-and-control-server-disco.h"

#include <app-common/zap-generated/cluster-objects.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/AttributeAccessInterface.h>
#include <app/AttributeAccessInterfaceRegistry.h>
#include <app/CommandHandlerInterface.h>
#include <app/CommandHandlerInterfaceRegistry.h>
#include <app/ConcreteCommandPath.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-cluster-logic.h>
#include <app/data-model/Encode.h>
#include <app/util/config.h>
#include <lib/core/CHIPError.h>

namespace chip {
namespace app {
namespace Clusters {
namespace ValveConfigurationAndControl {

using namespace Attributes;
using namespace Commands;
using namespace Protocols::InteractionModel;
namespace {

Status TranslateErrorToStatus(CHIP_ERROR err) {
    if (err == CHIP_NO_ERROR)
    {
        return Status::Success;
    }
    if (err == CHIP_ERROR_INCORRECT_STATE) {
        return Status::ConstraintError;
    }
    return Status::Failure;
}

CHIP_ERROR TranslateErrorToIMError(CHIP_ERROR err)
{
    if (err == CHIP_NO_ERROR)
    {
        return err;
    }
    return CHIP_ERROR_IM_GLOBAL_STATUS_VALUE(TranslateErrorToStatus(err));
}
} // namespace

CHIP_ERROR Interface::Read(const ConcreteReadAttributePath & aPath, AttributeValueEncoder & aEncoder)
{
    switch (aPath.mAttributeId)
    {
    case OpenDuration::Id: {
        return aEncoder.Encode(mClusterLogic.GetOpenDuration());
    }
    case DefaultOpenDuration::Id: {
        return aEncoder.Encode(mClusterLogic.GetDefaultOpenDuration());
    }
    case AutoCloseTime::Id: {
        return aEncoder.Encode(mClusterLogic.GetAutoCloseTime());
    }
    case RemainingDuration::Id: {
        return aEncoder.Encode(mClusterLogic.GetRemainingDuration());
    }
    case CurrentState::Id: {
        return aEncoder.Encode(mClusterLogic.GetCurrentState());
    }
    case TargetState::Id: {
        return aEncoder.Encode(mClusterLogic.GetTargetState());
    }
    case CurrentLevel::Id: {
        return aEncoder.Encode(mClusterLogic.GetCurrentLevel());
    }
    case TargetLevel::Id: {
        return aEncoder.Encode(mClusterLogic.GetTargetLevel());
    }
    case DefaultOpenLevel::Id: {
        return aEncoder.Encode(mClusterLogic.GetDefaultOpenLevel());
    }
    case ValveFault::Id: {
        return aEncoder.Encode(mClusterLogic.GetValveFault());
    }
    case LevelStep::Id: {
        return aEncoder.Encode(mClusterLogic.GetLevelStep());
    }
    case FeatureMap::Id: {
        return aEncoder.Encode(0);
    }
    case ClusterRevision::Id: {
        return aEncoder.Encode(mClusterLogic.GetClusterRevision());
    }
    default:
        return CHIP_IM_GLOBAL_STATUS(UnsupportedAttribute);
    }
}

CHIP_ERROR Interface::Write(const ConcreteDataAttributePath & aPath, AttributeValueDecoder & aDecoder)
{
    switch (aPath.mAttributeId)
    {
    case DefaultOpenDuration::Id: {
        DefaultOpenDuration::TypeInfo::Type val;
        ReturnErrorOnFailure(aDecoder.Decode(val));
        return TranslateErrorToIMError(mClusterLogic.SetDefaultOpenDuration(val));
    }
    break;
    case DefaultOpenLevel::Id: {
        DefaultOpenLevel::TypeInfo::Type val;
        ReturnErrorOnFailure(aDecoder.Decode(val));
        return TranslateErrorToIMError(mClusterLogic.SetDefaultOpenLevel(val));
    }
    default:
        return CHIP_IM_GLOBAL_STATUS(UnsupportedWrite);
    }
}

// CommandHandlerInterface
void Interface::InvokeCommand(HandlerContext & handlerContext)
{
    switch (handlerContext.mRequestPath.mCommandId)
    {
    case Open::Id:
        HandleCommand<Open::DecodableType>(
            handlerContext, [&logic = mClusterLogic](HandlerContext & ctx, const auto & commandData) {
                // TODO: I used optional in the lower layers because I think we want to move to std::optional in general
                // So here, I need to change over. But I can also change the Logic cluster to use Optional
                CHIP_ERROR err =
                    logic.HandleOpenCommand(commandData.openDuration.std_optional(), commandData.targetLevel.std_optional());
                ctx.mCommandHandler.AddStatus(ctx.mRequestPath, TranslateErrorToStatus(err));
            });
        return;
    case Close::Id:
        HandleCommand<Close::DecodableType>(handlerContext,
                                            [&logic = mClusterLogic](HandlerContext & ctx, const auto & commandData) {
                                                CHIP_ERROR err = logic.HandleCloseCommand();
                                                ctx.mCommandHandler.AddStatus(ctx.mRequestPath, TranslateErrorToStatus(err));
                                            });
        return;
    }
}

CHIP_ERROR Interface::Init()
{
    AttributeAccessInterfaceRegistry::Instance().Register(this);
    CommandHandlerInterfaceRegistry::Instance().RegisterCommandHandler(this);
    return CHIP_NO_ERROR;
}

} // namespace ValveConfigurationAndControl
} // namespace Clusters
} // namespace app
} // namespace chip
