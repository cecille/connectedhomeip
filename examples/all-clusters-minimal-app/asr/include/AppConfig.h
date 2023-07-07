/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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

// ---- Lighting Example App Config ----

#define APP_TASK_NAME "APP"

#define APP_EVENT_QUEUE_SIZE 10
#define APP_TASK_STACK_SIZE (1024 * 4)
#define APP_WAIT_LOOP 1000

#define MATTER_DEVICE_NAME "ASR-Clusters-m"

// Time it takes in ms for the simulated actuator to move from one
// state to another.
#define ACTUATOR_MOVEMENT_PERIOS_MS 2000

// ASR Logging
#ifdef __cplusplus
extern "C" {
#endif

void appError(int err);
void ASR_LOG(const char * aFormat, ...);

#ifdef __cplusplus
}

#include <lib/core/CHIPError.h>
void appError(CHIP_ERROR error);
#endif
