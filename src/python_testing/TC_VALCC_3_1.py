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

import time

import chip.clusters as Clusters
from chip.clusters.Types import NullValue
from chip.interaction_model import InteractionModelError, Status
from matter_testing_support import MatterBaseTest, TestStep, default_matter_test_main, has_cluster, run_if_endpoint_matches
from mobly import asserts


class TC_VALCC_3_1(MatterBaseTest):
    async def read_valcc_attribute_expect_success(self, endpoint, attribute):
        cluster = Clusters.Objects.ValveConfigurationAndControl
        return await self.read_single_attribute_check_success(endpoint=endpoint, cluster=cluster, attribute=attribute)

    def desc_TC_VALCC_3_1(self) -> str:
        return "[TC-VALCC-3.1] Basic state functionality with DUT as Server"

    def steps_TC_VALCC_3_1(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=True),
            TestStep(2, "Send Open command"),
            TestStep(3, "Read CurrentState and TargetState attribute until CurrentState is Open",
                     "Target state is Open while CurrentState is Transitioning"),
            TestStep(4, "Read CurrentState and TargetState attribute", "CurrentState is Open, TargetState is NULL"),
            TestStep(5, "Send Close command"),
            TestStep(6, "Read CurrentState and TargetState attribute until CurrentState is Open",
                     "Target state is Closed while CurrentState is Transitioning"),
            TestStep(7, "Read CurrentState and TargetState attribute", "CurrentState is Closed, TargetState is NULL"),
        ]
        return steps

    @run_if_endpoint_matches(has_cluster(Clusters.ValveConfigurationAndControl))
    async def test_TC_VALCC_3_1(self):

        endpoint = self.matter_test_config.endpoint

        self.step(1)
        cluster = Clusters.ValveConfigurationAndControl
        attributes = cluster.Attributes

        self.step(2)
        try:
            await self.send_single_cmd(cmd=Clusters.Objects.ValveConfigurationAndControl.Commands.Open(), endpoint=endpoint)
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.Success, "Unexpected error returned")
            pass

        self.step(3)

        # Read target state first in case the current state goes to open between these calls.
        # The test re-reads target state after CurrentState goes to open.
        target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
        current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)

        while current_state_dut is cluster.Enums.ValveStateEnum.kTransitioning:
            asserts.assert_equal(target_state_dut, cluster.Enums.ValveStateEnum.kOpen, "TargetState is incorrect")
            time.sleep(1)

            target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
            current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)
            asserts.assert_true(current_state_dut is not NullValue, "CurrentState is null")
            asserts.assert_true(target_state_dut is not NullValue, "TargetState is null")

        self.step(4)
        current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)
        asserts.assert_equal(current_state_dut, cluster.Enums.ValveStateEnum.kOpen, "CurrentState is incorrect")
        target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
        asserts.assert_true(target_state_dut is NullValue, "TargetState is not null")

        self.step(5)
        try:
            await self.send_single_cmd(cmd=Clusters.Objects.ValveConfigurationAndControl.Commands.Close(), endpoint=endpoint)
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.Success, "Unexpected error returned")
            pass

        self.step(6)
        target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
        current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)

        while current_state_dut is cluster.Enums.ValveStateEnum.kTransitioning:
            asserts.assert_equal(target_state_dut, cluster.Enums.ValveStateEnum.kClosed, "TargetState is incorrect")
            time.sleep(1)

            target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
            current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)
            asserts.assert_true(current_state_dut is not NullValue, "CurrentState is null")
            asserts.assert_true(target_state_dut is not NullValue, "TargetState is null")

        self.step(7)
        current_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.CurrentState)
        asserts.assert_equal(current_state_dut, cluster.Enums.ValveStateEnum.kClosed, "CurrentState is incorrect")
        target_state_dut = await self.read_valcc_attribute_expect_success(endpoint=endpoint, attribute=attributes.TargetState)
        asserts.assert_true(target_state_dut is NullValue, "TargetState is not null")


if __name__ == "__main__":
    default_matter_test_main()
