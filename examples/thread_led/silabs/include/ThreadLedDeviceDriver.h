// Copyrigh 2024 Google, All Rights Reserved.

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace google {
namespace matter {

class ThreadLedSilabsDriver
{
public:
    ThreadLedSilabsDriver() {}

    void Init();

    void SetPumpEnabled(uint8_t pump, bool enabled);

    static ThreadLedSilabsDriver & GetInstance()
    {
        static ThreadLedSilabsDriver sInstance;
        return sInstance;
    }
};

} // namespace matter
} // namespace google
