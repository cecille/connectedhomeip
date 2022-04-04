/*
 *    Copyright (c) 2021 Project CHIP Authors
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

#pragma once

#include <controller/CommissioningDelegate.h>
#include <lib/core/CHIPError.h>
#include <lib/core/NodeId.h>
#include <lib/support/DLLUtil.h>
#include <stdint.h>

namespace chip {
namespace Controller {

/**
 * A delegate that can be notified of progress as a "pairing" (which might mean
 * "PASE session establishment" or "commissioning") proceeds.
 */
class DLL_EXPORT DevicePairingDelegate
{
public:
    virtual ~DevicePairingDelegate() {}

    enum Status : uint8_t
    {
        SecurePairingSuccess = 0,
        SecurePairingFailed,
    };

    /**
     * @brief
     *   Called when the pairing reaches a certain stage.
     *
     * @param status Current status of pairing
     */
    virtual void OnStatusUpdate(DevicePairingDelegate::Status status) {}

    /**
     * @brief
     *   Called when PASE session establishment is complete (with success or error)
     *
     * @param error Error cause, if any
     */
    virtual void OnPairingComplete(CHIP_ERROR error) {}

    /**
     * @brief
     *   Called when the pairing is deleted (with success or error)
     *
     * @param error Error cause, if any
     */
    virtual void OnPairingDeleted(CHIP_ERROR error) {}

    /**
     * @brief
     *   Called when the PASE session is fully established
     *
     * @param proxy CommissioneeDeviceProxy that just completed
     */
    virtual void OnPASESessionEstablished(CommissioneeDeviceProxy * proxy) {}

    /**
     * @brief
     *   Called when PASE session establishment has errored
     *
     * @param error Error cause, if any
     */
    virtual void OnPASESessionError(CHIP_ERROR error) {}

    /**
     * @brief
     *   Called when the CASE session is fully established
     *
     * @param proxy OperationalDeviceProxy that just completed
     */
    virtual void OnCASESessionEstablished(OperationalDeviceProxy * proxy) {}

    /**
     * @brief
     *   Called when CASE session establishment has errored
     *
     * @param error Error cause, if any
     */
    virtual void OnCASESessionError(CHIP_ERROR error) {}

    /**
     *   Called when the commissioning process is complete (with success or error)
     */
    virtual void OnCommissioningComplete(NodeId deviceId, CHIP_ERROR error) {}
    virtual void OnCommissioningSuccess(PeerId peerId) {}
    virtual void OnCommissioningFailure(PeerId peerId, CHIP_ERROR error, CommissioningStage stageFailed,
                                        Optional<Credentials::AttestationVerificationResult> additionalErrorInfo)
    {}

    virtual void OnCommissioningStatusUpdate(PeerId peerId, CommissioningStage stageCompleted, CHIP_ERROR error) {}
};

} // namespace Controller
} // namespace chip
