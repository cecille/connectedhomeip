/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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
#include <AppMain.h>
#include <fan-manager.h>
#include <data-model-providers/codegen/CodegenDataModelProvider.h>

#include <app/clusters/identify-server/IdentifyCluster.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>


#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
#include <imgui_ui/ui.h>
#include <imgui_ui/windows/connectivity.h>
#include <imgui_ui/windows/fan_control.h>
#include <imgui_ui/windows/humidity_measurement.h>
#include <imgui_ui/windows/qrcode.h>
#include <imgui_ui/windows/temperature_measurement.h>
#endif

#include "platform/DefaultTimerDelegate.h"
#include "app/server-cluster/ServerClusterInterfaceRegistry.h"

#define FAN_CONTROL_ENDPOINT 1

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

class MyIdentifyDelegate : public chip::app::Clusters::IdentifyDelegate
{
public:
    void OnIdentifyStart(chip::app::Clusters::IdentifyCluster & cluster) override
    {
        // Your logic to start identification (e.g., start blinking an LED)
    }

    void OnIdentifyStop(chip::app::Clusters::IdentifyCluster & cluster) override
    {
        // Your logic to stop identification (e.g., stop blinking an LED)
    }

    void OnTriggerEffect(chip::app::Clusters::IdentifyCluster & cluster) override
    {
        // Your logic to trigger a specific effect
    }

    bool IsTriggerEffectEnabled() const override { return true; }
};

namespace {
MyIdentifyDelegate gMyIdentifyDelegate;
DefaultTimerDelegate gTimerDelegate;

chip::app::RegisteredServerCluster<chip::app::Clusters::IdentifyCluster> gIdentifyCluster(
    chip::app::Clusters::IdentifyCluster::Config(FAN_CONTROL_ENDPOINT, gTimerDelegate)
        .WithIdentifyType(chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator)
        .WithDelegate(&gMyIdentifyDelegate));
} //namespace

// The MatterPostAttributeChangeCallback is offloaded to the main Air Purifier Manager class.
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    if (FanManager::GetInstance() != nullptr)
    {
        FanManager::GetInstance()->PostAttributeChangeCallback(attributePath.mEndpointId, attributePath.mClusterId,
                                                                       attributePath.mAttributeId, type, size, value);
    }
}

// Initialize the Air Purifier Manager and set up the endpoint composition tree.
void ApplicationInit()
{
    FanManager::InitInstance(EndpointId(FAN_CONTROL_ENDPOINT));
    CHIP_ERROR err = chip::app::CodegenDataModelProvider::Instance().Registry().Register(gIdentifyCluster.Registration());
    VerifyOrDie(err == CHIP_NO_ERROR);
}

void ApplicationShutdown()
{
    ChipLogDetail(NotSpecified, "Air Purifier: ApplicationShutdown()");
}

int main(int argc, char * argv[])
{
    if (ChipLinuxAppInit(argc, argv) != 0)
    {
        return -1;
    }

#if defined(CHIP_IMGUI_ENABLED) && CHIP_IMGUI_ENABLED
    example::Ui::ImguiUi ui;

    ui.AddWindow(std::make_unique<example::Ui::Windows::QRCode>());
    ui.AddWindow(std::make_unique<example::Ui::Windows::Connectivity>());
    ui.AddWindow(std::make_unique<example::Ui::Windows::FanControl>(chip::EndpointId(FAN_CONTROL_ENDPOINT)));
    ui.AddWindow(std::make_unique<example::Ui::Windows::HumidityMeasurement>(chip::EndpointId(RELATIVE_HUMIDITY_SENSOR_ENDPOINT)));
    ui.AddWindow(std::make_unique<example::Ui::Windows::TemperatureMeasurement>(chip::EndpointId(TEMPERATURE_SENSOR_ENDPOINT)));
    ChipLinuxAppMainLoop(&ui);
#else
    ChipLinuxAppMainLoop();
#endif

    return 0;
}
