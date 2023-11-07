/*
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
#pragma once

#include <app/AppConfig.h>
#include <lib/core/CHIPError.h>

class ICDListener;

namespace chip {
namespace app {

/**
 * The ICDManager implements the ICDListener functions and is always subscribed to the ICDNotifier
 * This allows other Matter modules to inform the ICDManager that it needs to go and may have to stay in Active Mode,
 * outside of its standard ActiveModeDuration and IdleModeDuration, without being tightly coupled the  application data model
 *
 * This implementation also allows other modules to implement an ICDListener and subscribe to ICDNotifier
 * to couple behaviours with the ICD cycles. In such cases, ICD_MAX_NOTIFICATION_SUBSCRIBERS need to be adjusted
 */

static_assert(ICD_MAX_NOTIFICATION_SUBSCRIBERS > 0, "At least 1 Subscriber is required for the ICD Manager");

class ICDListener
{
public:
    enum class KeepActiveFlags : uint8_t
    {
        kCommissioningWindowOpen = 0x01,
        kFailSafeArmed           = 0x02,
        kExchangeContextOpen     = 0x03,
    };

    enum class ICDManagementEvents : uint8_t
    {
        kTableUpdated              = 0x01,
        kStayActiveRequestReceived = 0x02,
    };

    virtual ~ICDListener() {}

    /**
     * @brief This function is called for all subscribers of the ICDNotifier when it calls BroadcastNetworkActivityNotification
     * It notifies the subscriber that a NetworkActivity occurred. For example, a message sent or received.
     */
    virtual void OnNetworkActivity() = 0;

    /**
     * @brief This function is called for all subscribers of the ICDNotifier when it calls BroadcastActiveRequestNotification
     * It informs the subscriber that there is a need to place and keep the ICD in its Active Mode.
     *
     * @param request : Identity the request source
     */
    virtual void OnKeepActiveRequest(KeepActiveFlags request) = 0;

    /**
     * @brief This function is called for all subscribers of the ICDNotifier when it calls BroadcastActiveRequestWithdrawal
     * It informs the subscriber that a previous request no longer needs ICD to maintain its Active Mode.
     *
     * @param request : The request source
     */
    virtual void OnActiveRequestWithdrawal(KeepActiveFlags request) = 0;

    /**
     * @brief This function is called for all subscribers of the ICDNotifier when it calls BroadcastICDManagementEvent
     * It informs the subscriber that an ICD Management action has happened and needs to be processed
     *
     * @param event : The event type
     */
    virtual void OnICDManagementServerEvent(ICDManagementEvents event){};
};

class ICDNotifier
{
public:
    ~ICDNotifier();
    CHIP_ERROR Subscribe(ICDListener * subscriber);
    void Unsubscribe(ICDListener * subscriber);

    /**
     * The following Broacast* methods triggers all the registered ICDSubscribers related callback
     * For thread-safety reason (mostly of the ICDManager, which is a full time subscriber),
     * Those functions require to be called from the Chip Task Context, or by holding the chip stack lock.
     */
    void BroadcastNetworkActivityNotification();
    void BroadcastActiveRequestNotification(ICDListener::KeepActiveFlags request);
    void BroadcastActiveRequestWithdrawal(ICDListener::KeepActiveFlags request);
    void BroadcastICDManagementEvent(ICDListener::ICDManagementEvents event);

    static ICDNotifier & GetInstance() { return sICDNotifier; }

private:
    static ICDNotifier sICDNotifier;
    ICDListener * mSubscribers[ICD_MAX_NOTIFICATION_SUBSCRIBERS] = {};
};

} // namespace app
} // namespace chip
