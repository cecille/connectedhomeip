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
#include <controller/CommissioningDelegate.h>
#include <lib/support/UnitTestContext.h>
#include <lib/support/UnitTestRegistration.h>

namespace {
void TestDstOffsets(nlTestSuite * inSuite, void * inContext)
{
    // Test the zero copy and non-zero copy versions of the dst offsets function
    app::Clusters::TimeSynchronization::Structs::DSTOffsetStruct::Type dstOffsets[3];
    // Fill with any identifiable values, including one null validUntil.
    dstOffsets[0].offset        = 1;
    dstOffsets[1].validStarting = 100;
    dstOffsets[1].validUntil    = chip::app::DataModel::MakeNullable(101);

    dstOffsets[1].offset        = 2;
    dstOffsets[1].validStarting = 200;
    dstOffsets[1].validUntil    = chip::app::DataModel::MakeNullable(201);

    dstOffsets[2].offset        = 3;
    dstOffsets[2].validStarting = 300;
    dstOffsets[2].validUntil.SetNull();

    app::DataModel::List<app::Clusters::TimeSynchronization::Structs::DSTOffsetStruct::Type> list3(dstOffsets, 3);

    CommissioningDelegate cd;
    cd.SetDSTOffsets(list3);

    // This function should have copied the data into an internal buffer, so the returned span should not contain our buffer
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() != dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 3);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[2] == dstOffsets[2]);

    // Try re-setting and make sure it's still good
    app::DataModel::List<app::Clusters::TimeSynchronization::Structs::DSTOffsetStruct::Type> list2(dstOffsets, 1);
    cd.SetDSTOffsets(list2);

    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() != dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 2);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);

    // Try using the zero copy
    cd.SetDSTOffsetsZeroCopy(list2);
    // This SHOULD be our buffer
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() == dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 2);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);
}

void TestTimeZone(nlTestSuite * inSuite, void * inContext)
{
    // Test the zero copy and non-zero copy versions of the dst offsets function
    app::Clusters::TimeSynchronization::Structs::TimeZoneStruct::Type timeZone[2];
    // Fill with any identifiable values, including one null validUntil.
    dstOffsets[0].offset        = 1;
    dstOffsets[1].validStarting = 100;
    dstOffsets[1].validUntil    = chip::app::DataModel::MakeNullable(101);

    dstOffsets[1].offset        = 2;
    dstOffsets[1].validStarting = 200;
    dstOffsets[1].validUntil    = chip::app::DataModel::MakeNullable(201);

    dstOffsets[2].offset        = 3;
    dstOffsets[2].validStarting = 300;
    dstOffsets[2].validUntil.SetNull();

    app::DataModel::List<app::Clusters::TimeSynchronization::Structs::DSTOffsetStruct::Type> list3(dstOffsets, 3);

    CommissioningDelegate cd;
    cd.SetDSTOffsets(list3);

    // This function should have copied the data into an internal buffer, so the returned span should not contain our buffer
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() != dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 3);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[2] == dstOffsets[2]);

    // Try re-setting and make sure it's still good
    app::DataModel::List<app::Clusters::TimeSynchronization::Structs::DSTOffsetStruct::Type> list2(dstOffsets, 1);
    cd.SetDSTOffsets(list2);

    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() != dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 2);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);

    // Try using the zero copy
    cd.SetDSTOffsetsZeroCopy(list2);
    // This SHOULD be our buffer
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data() == dstOffsets);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().size() == 2);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[0] == dstOffsets[0]);
    NL_TEST_ASSERT(inSuite, cd.GetDSTOffsets().data()[1] == dstOffsets[1]);
}

// clang-format off
const nlTest sTests[] =
{
    NL_TEST_DEF("Test DST offsets", TestDstOffsets),

    NL_TEST_SENTINEL()
};
// clang-format on

} // namespace

/**
 *  Main
 */
int TestCommissioningDelegate()
{
    // clang-format off
    nlTestSuite theSuite =
    {
        "CommissioningDelegateTests",
        &sTests[0],
        nullptr,
        nullptr
    };
    // clang-format on

    return chip::ExecuteTestsWithoutContext(&theSuite);
}

CHIP_REGISTER_TEST_SUITE(TestCommissioningDelegate);
