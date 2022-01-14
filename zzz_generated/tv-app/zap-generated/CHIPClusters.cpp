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

#include "CHIPClusters.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <zap-generated/CHIPClientCallbacks.h>

namespace chip {

using namespace app::Clusters;
using namespace System;
using namespace Encoding::LittleEndian;

namespace Controller {

// TODO(#4502): onCompletion is not used by IM for now.
// TODO(#4503): length should be passed to commands when byte string is in argument list.
// TODO(#4503): Commands should take group id as an argument.

// Binding Cluster Commands
CHIP_ERROR BindingCluster::Bind(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                                chip::NodeId nodeId, chip::GroupId groupId, chip::EndpointId endpointId, chip::ClusterId clusterId)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, Binding::Commands::Bind::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // nodeId: nodeId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), nodeId));
    // groupId: groupId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), groupId));
    // endpointId: endpointNo
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), endpointId));
    // clusterId: clusterId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), clusterId));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR BindingCluster::Unbind(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                                  chip::NodeId nodeId, chip::GroupId groupId, chip::EndpointId endpointId,
                                  chip::ClusterId clusterId)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, Binding::Commands::Unbind::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // nodeId: nodeId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), nodeId));
    // groupId: groupId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), groupId));
    // endpointId: endpointNo
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), endpointId));
    // clusterId: clusterId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), clusterId));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

// GeneralCommissioning Cluster Commands
CHIP_ERROR GeneralCommissioningCluster::ArmFailSafe(Callback::Cancelable * onSuccessCallback,
                                                    Callback::Cancelable * onFailureCallback, uint16_t expiryLengthSeconds,
                                                    uint64_t breadcrumb, uint32_t timeoutMs)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, GeneralCommissioning::Commands::ArmFailSafe::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // expiryLengthSeconds: int16u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), expiryLengthSeconds));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));
    // timeoutMs: int32u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), timeoutMs));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR GeneralCommissioningCluster::CommissioningComplete(Callback::Cancelable * onSuccessCallback,
                                                              Callback::Cancelable * onFailureCallback)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         GeneralCommissioning::Commands::CommissioningComplete::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    // Command takes no arguments.

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR GeneralCommissioningCluster::SetRegulatoryConfig(Callback::Cancelable * onSuccessCallback,
                                                            Callback::Cancelable * onFailureCallback, uint8_t location,
                                                            chip::CharSpan countryCode, uint64_t breadcrumb, uint32_t timeoutMs)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         GeneralCommissioning::Commands::SetRegulatoryConfig::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // location: regulatoryLocationType
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), location));
    // countryCode: charString
    SuccessOrExit(err = writer->PutString(TLV::ContextTag(argSeqNumber++), countryCode));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));
    // timeoutMs: int32u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), timeoutMs));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

// NetworkCommissioning Cluster Commands
CHIP_ERROR NetworkCommissioningCluster::AddOrUpdateThreadNetwork(Callback::Cancelable * onSuccessCallback,
                                                                 Callback::Cancelable * onFailureCallback,
                                                                 chip::ByteSpan operationalDataset, uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         NetworkCommissioning::Commands::AddOrUpdateThreadNetwork::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // operationalDataset: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), operationalDataset));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR NetworkCommissioningCluster::AddOrUpdateWiFiNetwork(Callback::Cancelable * onSuccessCallback,
                                                               Callback::Cancelable * onFailureCallback, chip::ByteSpan ssid,
                                                               chip::ByteSpan credentials, uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         NetworkCommissioning::Commands::AddOrUpdateWiFiNetwork::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // ssid: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), ssid));
    // credentials: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), credentials));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR NetworkCommissioningCluster::ConnectNetwork(Callback::Cancelable * onSuccessCallback,
                                                       Callback::Cancelable * onFailureCallback, chip::ByteSpan networkID,
                                                       uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         NetworkCommissioning::Commands::ConnectNetwork::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // networkID: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), networkID));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR NetworkCommissioningCluster::RemoveNetwork(Callback::Cancelable * onSuccessCallback,
                                                      Callback::Cancelable * onFailureCallback, chip::ByteSpan networkID,
                                                      uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, NetworkCommissioning::Commands::RemoveNetwork::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // networkID: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), networkID));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR NetworkCommissioningCluster::ReorderNetwork(Callback::Cancelable * onSuccessCallback,
                                                       Callback::Cancelable * onFailureCallback, chip::ByteSpan networkID,
                                                       uint8_t networkIndex, uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         NetworkCommissioning::Commands::ReorderNetwork::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // networkID: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), networkID));
    // networkIndex: int8u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), networkIndex));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR NetworkCommissioningCluster::ScanNetworks(Callback::Cancelable * onSuccessCallback,
                                                     Callback::Cancelable * onFailureCallback, chip::ByteSpan ssid,
                                                     uint64_t breadcrumb)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, NetworkCommissioning::Commands::ScanNetworks::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // ssid: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), ssid));
    // breadcrumb: int64u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), breadcrumb));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

// OperationalCredentials Cluster Commands
CHIP_ERROR OperationalCredentialsCluster::AddNOC(Callback::Cancelable * onSuccessCallback, Callback::Cancelable * onFailureCallback,
                                                 chip::ByteSpan NOCValue, chip::ByteSpan ICACValue, chip::ByteSpan IPKValue,
                                                 chip::NodeId caseAdminNode, uint16_t adminVendorId)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId, OperationalCredentials::Commands::AddNOC::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // NOCValue: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), NOCValue));
    // ICACValue: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), ICACValue));
    // IPKValue: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), IPKValue));
    // caseAdminNode: nodeId
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), caseAdminNode));
    // adminVendorId: int16u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), adminVendorId));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::AddTrustedRootCertificate(Callback::Cancelable * onSuccessCallback,
                                                                    Callback::Cancelable * onFailureCallback,
                                                                    chip::ByteSpan rootCertificate)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::AddTrustedRootCertificate::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // rootCertificate: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), rootCertificate));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::AttestationRequest(Callback::Cancelable * onSuccessCallback,
                                                             Callback::Cancelable * onFailureCallback,
                                                             chip::ByteSpan attestationNonce)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::AttestationRequest::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // attestationNonce: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), attestationNonce));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::CertificateChainRequest(Callback::Cancelable * onSuccessCallback,
                                                                  Callback::Cancelable * onFailureCallback, uint8_t certificateType)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::CertificateChainRequest::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // certificateType: int8u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), certificateType));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::OpCSRRequest(Callback::Cancelable * onSuccessCallback,
                                                       Callback::Cancelable * onFailureCallback, chip::ByteSpan CSRNonce)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::OpCSRRequest::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // CSRNonce: octetString
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), CSRNonce));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::RemoveFabric(Callback::Cancelable * onSuccessCallback,
                                                       Callback::Cancelable * onFailureCallback, uint8_t fabricIndex)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::RemoveFabric::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // fabricIndex: int8u
    SuccessOrExit(err = writer->Put(TLV::ContextTag(argSeqNumber++), fabricIndex));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

CHIP_ERROR OperationalCredentialsCluster::UpdateFabricLabel(Callback::Cancelable * onSuccessCallback,
                                                            Callback::Cancelable * onFailureCallback, chip::CharSpan label)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    TLV::TLVWriter * writer = nullptr;
    uint8_t argSeqNumber    = 0;

    // Used when encoding non-empty command. Suppress error message when encoding empty commands.
    (void) writer;
    (void) argSeqNumber;

    VerifyOrReturnError(mDevice != nullptr, CHIP_ERROR_INCORRECT_STATE);

    app::CommandPathParams cmdParams = { mEndpoint, /* group id */ 0, mClusterId,
                                         OperationalCredentials::Commands::UpdateFabricLabel::Id,
                                         (app::CommandPathFlags::kEndpointIdValid) };

    CommandSenderHandle sender(
        Platform::New<app::CommandSender>(mDevice->GetInteractionModelDelegate(), mDevice->GetExchangeManager()));

    VerifyOrReturnError(sender != nullptr, CHIP_ERROR_NO_MEMORY);

    SuccessOrExit(err = sender->PrepareCommand(cmdParams));

    VerifyOrExit((writer = sender->GetCommandDataIBTLVWriter()) != nullptr, err = CHIP_ERROR_INCORRECT_STATE);
    // label: charString
    SuccessOrExit(err = writer->PutString(TLV::ContextTag(argSeqNumber++), label));

    SuccessOrExit(err = sender->FinishCommand());

    // #6308: This is a temporary solution before we fully support IM on application side and should be replaced by IMDelegate.
    mDevice->AddIMResponseHandler(sender.get(), onSuccessCallback, onFailureCallback);

    SuccessOrExit(err = mDevice->SendCommands(sender.get(), mTimeout));

    // We have successfully sent the command, and the callback handler will be responsible to free the object, release the object
    // now.
    sender.release();
exit:
    return err;
}

} // namespace Controller
} // namespace chip
