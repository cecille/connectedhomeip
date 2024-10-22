// Copyrigh 2024 Google, All Rights Reserved.

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace google {
namespace matter {

class DrinksMachineSilabsDriver
{
public:
    DrinksMachineSilabsDriver() {}

    void Init();

    void SetPumpEnabled(uint8_t pump, bool enabled);

    static DrinksMachineSilabsDriver & GetInstance()
    {
        static DrinksMachineSilabsDriver sInstance;
        return sInstance;
    }
};

} // namespace matter
} // namespace google
