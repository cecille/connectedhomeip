#
#    Copyright (c) 2023 Project CHIP Authors
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

# === BEGIN CI TEST ARGUMENTS ===
# test-runner-runs: run1
# test-runner-run/run1/app: ${ALL_CLUSTERS_APP}
# test-runner-run/run1/factoryreset: True
# test-runner-run/run1/quiet: True
# test-runner-run/run1/app-args: --discriminator 1234 --KVS kvs1 --trace-to json:${TRACE_APP}.json
# test-runner-run/run1/script-args: --storage-path admin_storage.json --commissioning-method on-network --discriminator 1234 --passcode 20202021 --trace-to json:${TRACE_TEST_JSON}.json --trace-to perfetto:${TRACE_TEST_PERFETTO}.perfetto
# === END CI TEST ARGUMENTS ===

import logging

import chip.clusters as Clusters
from chip.interaction_model import InteractionModelError, Status
from matter_testing_support import MatterBaseTest, TestStep, default_matter_test_main, has_attribute, run_if_endpoint_matches
from mobly import asserts


class TC_VALCC_3_4(MatterBaseTest):
    async def read_valcc_attribute_expect_success(self, endpoint, attribute):
        cluster = Clusters.Objects.ValveConfigurationAndControl
        return await self.read_single_attribute_check_success(endpoint=endpoint, cluster=cluster, attribute=attribute)

    def desc_TC_VALCC_3_4(self) -> str:
        return "[TC-VALCC-3.4] LevelStep behavior with DUT as Server"

    def steps_TC_VALCC_3_4(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=True),
            TestStep(2, "Read AttributeList attribute"),
            TestStep(3, "Verify LevelStep is supported"),
            TestStep(4, "Read LevelStep attribute"),
            TestStep(5, "Verify the supported level values using Open Command"),
            TestStep(6, "Send Close command"),
        ]
        return steps

    @run_if_endpoint_matches(has_attribute(Clusters.ValveConfigurationAndControl.Attributes.LevelStep))
    async def test_TC_VALCC_3_4(self):

        endpoint = self.matter_test_config.endpoint

        self.step(1)
        attributes = Clusters.ValveConfigurationAndControl.Attributes

        self.step(2)
        self.step(3)
        # Steps 2 and three are handled by the decorator

        self.step(4)
        levelStep = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.LevelStep)

        self.step(5)

        for levelValue in range(1, 100):
            try:
                await self.send_single_cmd(cmd=Clusters.Objects.ValveConfigurationAndControl.Commands.Open(targetLevel=levelValue), endpoint=endpoint)

                if levelValue % levelStep != 0:
                    asserts.fail("Received Success response when an CONSTRAINT_ERROR was expected")

            except InteractionModelError as e:
                if levelValue % levelStep != 0:
                    asserts.assert_equal(e.status, Status.ConstraintError,
                                         "Unexpected error returned when an CONSTRAINT_ERROR was expected")
                else:
                    asserts.fail("Unexpected error returned")
                pass

        self.step(6)
        try:
            await self.send_single_cmd(cmd=Clusters.Objects.ValveConfigurationAndControl.Commands.Close(), endpoint=endpoint)
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.Success, "Unexpected error returned")
            pass


if __name__ == "__main__":
    default_matter_test_main()
