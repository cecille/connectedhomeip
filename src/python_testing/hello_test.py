#
#    Copyright (c) 2022 Project CHIP Authors
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import logging

import chip.clusters as Clusters
from chip.interaction_model import Status
from matter_testing_support import MatterBaseTest, async_test_body, default_matter_test_main, TestStep
from mobly import asserts


class HelloTest(MatterBaseTest):
    '''
        For cert tests, this represents a single Test case.

        The naming convention we normally use is to use the test case designation with 
        underscores. Ex. TC-DA-1.7 -> TC_DA_1_7
    '''

    # Test harness integration functions
    # To allow your tests to be run through the certification test harness, please
    # implement the functions below to allow the test harness to get meta-data about
    # your test.

    def get_test_steps(self) -> list[TestStep]:
        ''' Returns a list of test steps that will be called in order

            The test body (below) should run through these steps by calling self.step(<#>)
            The base test will verify that the steps are called in order and that all
            test steps are called.
        '''
        return [TestStep(1, "Commissioning - already done", is_commissioning=True),
                TestStep(2, "Verify vendor name is TEST_VENDOR"),
                TestStep(3, "Verify invalid endpoint"),
                TestStep(4, "PICS guard demo")]

    def get_test_desc(self) -> str:
        ''' Returns a description of this test

            Format:
            <Test plan reference> [<test plan number>] <test plan name>

            ex:
            133.1.1. [TC-ACL-1.1] Global attributes            
        '''
        return "0.0.0. [TC-HELLOTEST-1.1] Hello test"

    @async_test_body
    async def test_HelloTest(self):
        '''
            Main test body

            Any function that starts with the string "test" is run by the mobly runner.
            Test functions will nearly always be async in order to use the ChipDeviceCtrl
            functions which are all async. In order to use async functions with the mobly
            runner, you need to decorate the function with @async_test_body

            To integrate with the test harness, please implement a single test_ function per class
        '''
        # commissioning step - commissioning is already done if required, so just move on
        self.step(1)

        self.step(2)
        dev_ctrl = self.default_controller
        vendor_name = await self.read_single_attribute(
            dev_ctrl,
            self.dut_node_id,
            0,
            Clusters.BasicInformation.Attributes.VendorName
        )

        logging.info("Found VendorName: %s" % (vendor_name))
        asserts.assert_equal(vendor_name, "TEST_VENDOR", "VendorName must be TEST_VENDOR!")

        self.step(3)
        dev_ctrl = self.default_controller
        result = await self.read_single_attribute(
            dev_ctrl,
            self.dut_node_id,
            9999,
            Clusters.BasicInformation.Attributes.ProductName
        )
        asserts.assert_true(isinstance(result, Clusters.Attribute.ValueDecodeFailure), "Should fail to read on endpoint 9999")
        asserts.assert_equal(result.Reason.status, Status.UnsupportedEndpoint, "Failure reason should be UnsupportedEndpoint")

        self.step(4)
        # If your test has a PICS-guarded step, use pics_guard with the boolean to properly handle the rquired
        # test harness callbacks and prints if it gets skipped
        if self.pics_guard(self.check_pics('NONEXISTANT.S.A0000')):
            asserts.fail('This step should not be done')


if __name__ == "__main__":
    default_matter_test_main()
