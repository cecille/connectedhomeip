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

import inspect
import logging
from dataclasses import dataclass

import chip.clusters as Clusters
import chip.discovery as Discovery
from chip import ChipUtility
from chip.exceptions import ChipStackError
from chip.interaction_model import InteractionModelError, Status
from matter_testing_support import MatterBaseTest, async_test_body, default_matter_test_main, type_matches
from mobly import asserts


class TC_IDM_9_1(MatterBaseTest):

    @async_test_body
    async def test_TC_IDM_9_1(self):
        self.print_step(0, "Commissioning - already done")

        self.print_step("1a", "Send cmd with out of range uint16 (OpenCommissioningWindow discriminator)")

        # Need to plumb through the pake verifier generation code here to use open commissioning window with bad parameters
        # TODO:add

        self.print_step("1b", "Send cmd with out of range octstr (OpenCommissioningWindow salt too large)")
        # TODO: add

        self.print_step("1c", "Send cmd with out of range octstr (OpenCommissioningWindow salt too small)")
        # TODO: add

        self.print_step("1d", "Send cmd with out of range string (SetRegulatoryConfig country code too long)")
        regulatory_location = await self.read_single_attribute_check_success(cluster=Clusters.GeneralCommissioning, attribute=Clusters.GeneralCommissioning.Attributes.LocationCapability)
        try:
            await self.send_single_cmd(cmd=Clusters.GeneralCommissioning.Commands.SetRegulatoryConfig(
                newRegulatoryConfig=regulatory_location, countryCode="XXX", breadcrumb=1))
            asserts.fail("Received Success response when an CONSTRAINT_ERROR was expected")
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.ConstraintError, "Received incorrect error from SetRegulatoryConfig command")

        self.print_step("1e", "Send cmd with out of range string (SetRegulatoryConfig country code too long)")
        try:
            await self.send_single_cmd(cmd=Clusters.GeneralCommissioning.Commands.SetRegulatoryConfig(
                newRegulatoryConfig=regulatory_location, countryCode="X", breadcrumb=1))
            asserts.fail("Received Success response when an CONSTRAINT_ERROR was expected")
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.ConstraintError, "Received incorrect error from SetRegulatoryConfig command")

        self.print_step("1f", "Send cmd with out of range enum (SetRegulatoryConfig location)")
        try:
            await self.send_single_cmd(cmd=Clusters.GeneralCommissioning.Commands.SetRegulatoryConfig(
                newRegulatoryConfig=Clusters.GeneralCommissioning.Enums.RegulatoryLocationTypeEnum.kUnknownEnumValue, countryCode="XX", breadcrumb=1))
            asserts.fail("Received Success response when an CONSTRAINT_ERROR was expected")
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.ConstraintError, "Received incorrect error from SetRegulatoryConfig command")


if __name__ == "__main__":
    default_matter_test_main()
