// Copyrigh 2024 Google, All Rights Reserved.

#include <algorithm>

#include <cmsis_os2.h>
#include <stdint.h>

#include "DrinksMachineDeviceDriver.h"

#include "gpiointerrupt.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "silabs_utils.h"

#include <lib/support/CodeUtils.h>

// ========= Pin definitions (local) =======
// Right side pin 8
#define PUMP_0_PORT gpioPortC
#define PUMP_0_OUT_PIN 0
// Right side pin 7
#define PUMP_1_PORT gpioPortC
#define PUMP_1_OUT_PIN 1
// Right side pin 6
#define PUMP_2_PORT gpioPortC
#define PUMP_2_OUT_PIN 2
// Left side pin 8
#define PUMP_3_PORT gpioPortD
#define PUMP_3_OUT_PIN 5
// Left side pin 7
#define PUMP_4_PORT gpioPortD
#define PUMP_4_OUT_PIN 4
// Left side pin 6
#define PUMP_5_PORT gpioPortB
#define PUMP_5_OUT_PIN 4

// ============ Start of driver code =============

namespace google {
namespace matter {

namespace {

struct PortAndPin
{
    PortAndPin(unsigned the_port, unsigned the_pin) : port(the_port), pin(the_pin) {}

    unsigned port;
    unsigned pin;
};

const PortAndPin gPumpMappings[] = { PortAndPin{ PUMP_0_PORT, PUMP_0_OUT_PIN }, PortAndPin{ PUMP_1_PORT, PUMP_1_OUT_PIN },
                                     PortAndPin{ PUMP_2_PORT, PUMP_2_OUT_PIN }, PortAndPin{ PUMP_3_PORT, PUMP_3_OUT_PIN },
                                     PortAndPin{ PUMP_4_PORT, PUMP_4_OUT_PIN }, PortAndPin{ PUMP_5_PORT, PUMP_5_OUT_PIN } };

const size_t kNumPumps = sizeof(gPumpMappings) / sizeof(PortAndPin);
} // namespace

// Singleton instance.

void DrinksMachineSilabsDriver::Init()
{
    /* Enable GPIO in CMU */
    CMU_ClockEnable(cmuClock_GPIO, true);

    /* Initialize GPIO interrupt dispatcher */
    GPIOINT_Init();

    for (size_t i = 0; i < kNumPumps; ++i)
    {
        GPIO_PinModeSet(gPumpMappings[i].port, gPumpMappings[i].pin, gpioModePushPull, 0);
    }
}

void DrinksMachineSilabsDriver::SetPumpEnabled(uint8_t pump, bool enabled)
{
    if (pump >= kNumPumps)
    {
        SILABS_LOG("Unknown pump number %u", pump);
        return;
    }

    PortAndPin port_and_pin = gPumpMappings[pump];

    if (enabled)
    {
        SILABS_LOG("Setting pump %u enabled", pump);
        GPIO_PinOutSet(port_and_pin.port, port_and_pin.pin);
    }
    else
    {
        SILABS_LOG("Setting pump %u disabled", pump);
        GPIO_PinOutClear(port_and_pin.port, port_and_pin.pin);
    }
}

} // namespace matter
} // namespace google
