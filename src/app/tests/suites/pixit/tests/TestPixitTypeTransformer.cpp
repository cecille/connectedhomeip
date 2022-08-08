/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include <app/tests/suites/pixit/PixitTypeTransformer.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/UnitTestRegistration.h>

#include <stdlib.h>

#include <nlunit-test.h>

namespace {

using namespace chip;

void TestStringPixit(nlTestSuite * inSuite, std::string pixit)
{
    std::string val = "testtest";

    PixitTypeTransformer::PixitValueHolder ret = PixitTypeTransformer::TransformToType(pixit, val);
    NL_TEST_ASSERT(inSuite, ret.Valid());
    NL_TEST_ASSERT(inSuite, ret.Is<std::string>());
    NL_TEST_ASSERT(inSuite, ret.Get<std::string>() == val);
}

template <typename T>
void TestNumberPixit(nlTestSuite * inSuite, std::string pixit)
{
    T testVal                 = 65;
    T hexVal                  = 0x1F;
    std::string testValStr    = std::to_string(testVal);
    std::string hexValStr     = "0x1F";
    std::string invalidValStr = "hhhhh";

    std::string biggestValStr  = std::to_string(std::numeric_limits<T>::max());
    std::string smallestValStr = std::to_string(std::numeric_limits<T>::min());

    std::string tooBigValStr   = biggestValStr;
    std::string tooSmallValStr = smallestValStr;

    // Dirty hack to add 1 to the long long number (used for everything)
    // Only works because there are no numbers ending in 9
    tooBigValStr.back() = tooBigValStr.back() + 1;
    if (std::numeric_limits<T>::min() < 0)
    {
        tooSmallValStr.back() = tooSmallValStr.back() + 1;
    }
    else
    {
        tooSmallValStr = "-1";
    }

    PixitTypeTransformer::PixitValueHolder val;
    val = PixitTypeTransformer::TransformToType(pixit, testValStr);
    NL_TEST_ASSERT(inSuite, val.Valid());
    NL_TEST_ASSERT(inSuite, val.Get<T>() == testVal);

    val = PixitTypeTransformer::TransformToType(pixit, hexValStr);
    NL_TEST_ASSERT(inSuite, val.Valid());
    NL_TEST_ASSERT(inSuite, val.Get<T>() == hexVal);

    val = PixitTypeTransformer::TransformToType(pixit, biggestValStr);
    NL_TEST_ASSERT(inSuite, val.Valid());
    NL_TEST_ASSERT(inSuite, val.Get<T>() == std::numeric_limits<T>::max());

    val = PixitTypeTransformer::TransformToType(pixit, smallestValStr);
    NL_TEST_ASSERT(inSuite, val.Valid());
    NL_TEST_ASSERT(inSuite, val.Get<T>() == std::numeric_limits<T>::min());

    val = PixitTypeTransformer::TransformToType(pixit, tooBigValStr);
    NL_TEST_ASSERT(inSuite, !val.Valid());

    val = PixitTypeTransformer::TransformToType(pixit, tooSmallValStr);
    NL_TEST_ASSERT(inSuite, !val.Valid());

    val = PixitTypeTransformer::TransformToType(pixit, invalidValStr);
    NL_TEST_ASSERT(inSuite, !val.Valid());
}

void TestStringPixit(nlTestSuite * inSuite, void * inContext)
{
    TestStringPixit(inSuite, "PIXIT.UNITTEST.STRING");
}

void TestPixitLowercase(nlTestSuite * inSuite, void * inContext)
{
    TestStringPixit(inSuite, "PIXIT.UNITTEST.string");
}

void TestUint8Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<uint8_t>(inSuite, "PIXIT.UNITTEST.UINT8");
}

void TestUint16Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<uint16_t>(inSuite, "PIXIT.UNITTEST.UINT16");
}

void TestUint32Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<uint32_t>(inSuite, "PIXIT.UNITTEST.UINT32");
}

void TestUint64Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<uint64_t>(inSuite, "PIXIT.UNITTEST.UINT64");
}

void TestInt8Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<int8_t>(inSuite, "PIXIT.UNITTEST.INT8");
}

void TestInt16Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<int16_t>(inSuite, "PIXIT.UNITTEST.INT16");
}

void TestInt32Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<int32_t>(inSuite, "PIXIT.UNITTEST.INT32");
}

void TestInt64Pixit(nlTestSuite * inSuite, void * inContext)
{
    TestNumberPixit<int64_t>(inSuite, "PIXIT.UNITTEST.INT64");
}

void TestIncorrectPixit(nlTestSuite * inSuite, void * inContext)
{
    PixitTypeTransformer::PixitValueHolder val = PixitTypeTransformer::TransformToType("PIXIT.UNITTEST.UNUSED", "testsarefun");
    NL_TEST_ASSERT(inSuite, !val.Valid());
}

} // namespace

/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {
    NL_TEST_DEF("TestStringPixit", TestStringPixit),       NL_TEST_DEF("TestPixitLowercase", TestPixitLowercase),
    NL_TEST_DEF("TestUint8Pixit", TestUint8Pixit),         NL_TEST_DEF("TestUint16Pixit", TestUint16Pixit),
    NL_TEST_DEF("TestUint32Pixit", TestUint32Pixit),       NL_TEST_DEF("TestUint64Pixit", TestUint64Pixit),
    NL_TEST_DEF("TestInt8Pixit", TestInt8Pixit),           NL_TEST_DEF("TestInt16Pixit", TestInt16Pixit),
    NL_TEST_DEF("TestInt32Pixit", TestInt32Pixit),         NL_TEST_DEF("TestInt64Pixit", TestInt64Pixit),
    NL_TEST_DEF("TestIncorrectPixit", TestIncorrectPixit), NL_TEST_SENTINEL()
};

nlTestSuite sSuite = {
    "TestPixit",
    &sTests[0],
    nullptr,
    nullptr,
};

int TestPixitTypeTransformer()
{
    nlTestRunner(&sSuite, nullptr);
    return (nlTestRunnerStats(&sSuite));
}

CHIP_REGISTER_TEST_SUITE(TestPixitTypeTransformer);
