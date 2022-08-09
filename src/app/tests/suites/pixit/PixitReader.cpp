/**
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

#include "PixitReader.h"

#include <lib/support/CodeUtils.h>

#include <fstream>
#include <sstream>

CHIP_ERROR PixitReader::UpdateFromFile(std::string filepath, PixitMap & map)
{
    std::ifstream f(filepath);
    VerifyOrDieWithMsg(f.is_open(), chipTool, "Error reading: %s", filepath.c_str());

    std::string line;
    std::string key;
    std::string value;
    uint16_t lineNumber = 0;
    while (std::getline(f, line))
    {
        if (line.empty())
        {
            continue;
        }

        std::stringstream ss(line);

        std::getline(ss, key, '=');
        VerifyOrDieWithMsg(!key.empty(), chipTool, "Missing PIXIT key at line %u", lineNumber + 1);
        // key = PICSNormalizer::Normalize(key);

        std::getline(ss, value);
        ChipLogError(chipTool, "%s: PIXIT value is '%s'", key.c_str(), value.c_str());

        lineNumber++;
    }

    return CHIP_NO_ERROR;
}

PixitReader::PixitMap PixitReader::PopulateDefaults()
{
    PixitMap map = {
        { "PIXIT.UNITTEST.UINT8-1", PixitTypeTransformer::PixitValueHolder::Create<uint8_t>(static_cast<uint8_t>(1)) },
        { "PIXIT.UNITTEST.UINT8-2", PixitTypeTransformer::PixitValueHolder::Create<uint8_t>(static_cast<uint8_t>(2)) },
        { "PIXIT.UNITTEST.UINT16-1", PixitTypeTransformer::PixitValueHolder::Create<uint16_t>(static_cast<uint16_t>(1)) },
        { "PIXIT.UNITTEST.UINT16-2", PixitTypeTransformer::PixitValueHolder::Create<uint16_t>(static_cast<uint16_t>(2)) },
        { "PIXIT.UNITTEST.UINT32-1", PixitTypeTransformer::PixitValueHolder::Create<uint32_t>(static_cast<uint32_t>(1)) },
        { "PIXIT.UNITTEST.UINT32-2", PixitTypeTransformer::PixitValueHolder::Create<uint32_t>(static_cast<uint32_t>(2)) },
        { "PIXIT.UNITTEST.UINT64-1", PixitTypeTransformer::PixitValueHolder::Create<uint64_t>(static_cast<uint64_t>(1)) },
        { "PIXIT.UNITTEST.UINT64-2", PixitTypeTransformer::PixitValueHolder::Create<uint64_t>(static_cast<uint64_t>(2)) },
        { "PIXIT.UNITTEST.INT8-1", PixitTypeTransformer::PixitValueHolder::Create<int8_t>(static_cast<int8_t>(1)) },
        { "PIXIT.UNITTEST.INT8-2", PixitTypeTransformer::PixitValueHolder::Create<int8_t>(static_cast<int8_t>(2)) },
        { "PIXIT.UNITTEST.INT16-1", PixitTypeTransformer::PixitValueHolder::Create<int16_t>(static_cast<int16_t>(1)) },
        { "PIXIT.UNITTEST.INT16-2", PixitTypeTransformer::PixitValueHolder::Create<int16_t>(static_cast<int16_t>(2)) },
        { "PIXIT.UNITTEST.INT32-1", PixitTypeTransformer::PixitValueHolder::Create<int32_t>(static_cast<int32_t>(1)) },
        { "PIXIT.UNITTEST.INT32-2", PixitTypeTransformer::PixitValueHolder::Create<int32_t>(static_cast<int32_t>(2)) },
        { "PIXIT.UNITTEST.INT64-1", PixitTypeTransformer::PixitValueHolder::Create<int64_t>(static_cast<int64_t>(1)) },
        { "PIXIT.UNITTEST.INT64-2", PixitTypeTransformer::PixitValueHolder::Create<int64_t>(static_cast<int64_t>(2)) },
        { "PIXIT.UNITTEST.STRING-1", PixitTypeTransformer::PixitValueHolder::Create<std::string>("string1") },
        { "PIXIT.UNITTEST.STRING-2", PixitTypeTransformer::PixitValueHolder::Create<std::string>("string2") },
        { "PIXIT.UNITTEST.SIZET-1", PixitTypeTransformer::PixitValueHolder::Create<size_t>(static_cast<size_t>(1)) },
        { "PIXIT.UNITTEST.SIZET-2", PixitTypeTransformer::PixitValueHolder::Create<size_t>(static_cast<size_t>(2)) },
        { "PIXIT.UNITTEST.BOOL-T", PixitTypeTransformer::PixitValueHolder::Create<bool>(true) },
        { "PIXIT.UNITTEST.BOOL-F", PixitTypeTransformer::PixitValueHolder::Create<bool>(false) },

    };

    return map;
}
