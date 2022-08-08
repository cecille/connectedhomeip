/**
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include "PixitTypeTransformer.h"
#include <errno.h>
#include <string>

void PixitTypeTransformer::PrintOutOfRangeError(std::string key, std::string valueAsString, PixitTypeTransformer::PixitType type)
{
    ChipLogError(chipTool, "Value out of range for %s - read %s, expected %s type", key.c_str(), valueAsString.c_str(),
                 GetName(type).c_str());
}

const std::unordered_map<std::string, PixitTypeTransformer::PixitType> PixitTypeTransformer::mPixitType = {
    { "PIXIT.CNET.WIFI_1ST_ACCESSPOINT_SSID", PixitType::kString },
    { "PIXIT.CNET.WIFI_1ST_ACCESSPOINT_CREDENTIALS", PixitType::kString },
    { "PIXIT.CNET.WIFI_2ND_ACCESSPOINT_SSID", PixitType::kString },
    { "PIXIT.CNET.WIFI_2ND_ACCESSPOINT_CREDENTIALS", PixitType::kString },
    { "PIXIT.CNET.THREAD_1ST_OPERATIONALDATASET", PixitType::kString },
    { "PIXIT.CNET.THREAD_2ND_OPERATIONALDATASET", PixitType::kString },
    { "PIXIT.CNET.THREAD_2ND_OPERATIONALDATASET", PixitType::kString },
    { "PIXIT.CNET.ENDPOINT_WIFI", PixitType::kUint8 },
    { "PIXIT.CNET.ENDPOINT_THREAD", PixitType::kUint8 },
    { "PIXIT.CNET.ENDPOINT_ETHERNET", PixitType::kUint8 },

    // PIXITS used for testing
    { "PIXIT.UNITTEST.STRING", PixitType::kString },
    { "PIXIT.UNITTEST.UINT8", PixitType::kUint8 },
    { "PIXIT.UNITTEST.UINT16", PixitType::kUint16 },
    { "PIXIT.UNITTEST.UINT32", PixitType::kUint32 },
    { "PIXIT.UNITTEST.UINT64", PixitType::kUint64 },
    { "PIXIT.UNITTEST.INT8", PixitType::kInt8 },
    { "PIXIT.UNITTEST.INT16", PixitType::kInt16 },
    { "PIXIT.UNITTEST.INT32", PixitType::kInt32 },
    { "PIXIT.UNITTEST.INT64", PixitType::kInt64 },
};

bool PixitTypeTransformer::IsSigned(PixitType type)
{
    switch (type)
    {
    case PixitType::kInt8:
    case PixitType::kInt16:
    case PixitType::kInt32:
    case PixitType::kInt64:
        return true;
    default:
        return false;
    }
}

bool PixitTypeTransformer::IsUnsigned(PixitType type)
{
    switch (type)
    {
    case PixitType::kUint8:
    case PixitType::kUint16:
    case PixitType::kUint32:
    case PixitType::kUint64:
        return true;
    default:
        return false;
    }
}

std::string PixitTypeTransformer::GetName(PixitType type)
{
    switch (type)
    {
    case PixitType::kString:
        return "String";
    case PixitType::kInt8:
        return "int8_t";
    case PixitType::kInt16:
        return "int16_t";
    case PixitType::kInt32:
        return "int32_t";
    case PixitType::kInt64:
        return "int64_t";
    case PixitType::kUint8:
        return "uint8_t";
    case PixitType::kUint16:
        return "uint16_t";
    case PixitType::kUint32:
        return "uint32_t";
    case PixitType::kUint64:
        return "uint64_t";
    default:
        return "unknown";
    }
}

PixitTypeTransformer::PixitValueHolder PixitTypeTransformer::TransformToType(std::string key, std::string valueAsString)
{

    PixitTypeTransformer::PixitValueHolder ret;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::toupper(c); });
    auto it = mPixitType.find(key);
    if (it == mPixitType.end())
    {
        ChipLogError(chipTool, "Unable to find %s in list of supported PIXIT values", key.c_str());
        return ret;
    }

    PixitType pixitType = it->second;
    unsigned long long unsignedNumber;
    long long signedNumber;
    char * end = nullptr;
    errno      = 0;

    if (IsUnsigned(pixitType))
    {

        unsignedNumber = strtoull(valueAsString.c_str(), &end, 0);
        // stroull does some very creative interpretations of what a negative in an unsigned number means. If we encounter a
        // negative number in the parsed string, let's just assume that it's not good and error.
        if (valueAsString.find('-') != std::string::npos)
        {
            errno = ERANGE;
        }
    }
    else if (IsSigned(pixitType))
    {
        {
            signedNumber = strtoll(valueAsString.c_str(), &end, 0);
        }
    }

    if (end == valueAsString.c_str())
    {
        ChipLogError(chipTool, "Unable to parse value for %s - read %s, expected %s type", key.c_str(), valueAsString.c_str(),
                     GetName(pixitType).c_str());
        return ret;
    }
    if (errno == ERANGE)
    {
        PrintOutOfRangeError(key, valueAsString, pixitType);
        return ret;
    }

    CHIP_ERROR err = CHIP_NO_ERROR;
    switch (pixitType)
    {
    case PixitType::kString:
        ret.Set<std::string>(valueAsString);
        break;
    case PixitType::kUint8:
        err = SetPixitValueHolder<uint8_t>(unsignedNumber, ret);
        break;
    case PixitType::kUint16:
        err = SetPixitValueHolder<uint16_t>(unsignedNumber, ret);
        break;
    case PixitType::kUint32:
        err = SetPixitValueHolder<uint32_t>(unsignedNumber, ret);
        break;
    case PixitType::kUint64:
        err = SetPixitValueHolder<uint64_t>(unsignedNumber, ret);
        break;
    case PixitType::kInt8:
        err = SetPixitValueHolder<int8_t>(signedNumber, ret);
        break;
    case PixitType::kInt16:
        err = SetPixitValueHolder<int16_t>(signedNumber, ret);
        break;
    case PixitType::kInt32:
        err = SetPixitValueHolder<int32_t>(signedNumber, ret);
        break;
    case PixitType::kInt64:
        err = SetPixitValueHolder<int64_t>(signedNumber, ret);
        break;
    }
    if (err != CHIP_NO_ERROR)
    {
        PrintOutOfRangeError(key, valueAsString, pixitType);
    }
    return ret;
}