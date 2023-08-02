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

import sys
import os
import logging

import chip.clusters as Clusters
from chip.utils import CommissioningBuildingBlocks
from matter_testing_support import MatterBaseTest, async_test_body, default_matter_test_main
from mobly import asserts

# This needs to be run using --commissioning-method flag to get the first python fabric loaded.


class FillNMinus1Fabrics(MatterBaseTest):
    @async_test_body
    async def test_FillNMinus1Fabrics(self):
        dev_ctrl = self.default_controller
        node_id = self.dut_node_id

        # Read the supported number of fabrics and the number of fabrics already commissioned
        ret = await dev_ctrl.ReadAttribute(nodeid=node_id, attributes=[(0, Clusters.OperationalCredentials.Attributes.SupportedFabrics), (0, Clusters.OperationalCredentials.Attributes.CommissionedFabrics)])
        supported_fabrics = ret[0][Clusters.OperationalCredentials][Clusters.OperationalCredentials.Attributes.SupportedFabrics]
        initial_commissioned_fabric_count = ret[0][Clusters.OperationalCredentials][Clusters.OperationalCredentials.Attributes.CommissionedFabrics]
        num_to_commission = supported_fabrics - initial_commissioned_fabric_count - 1

        commissioned_fabric_count = initial_commissioned_fabric_count
        for i in range(num_to_commission):
            new_certificate_authority = self.certificate_authority_manager.NewCertificateAuthority()
            new_fabric_admin = new_certificate_authority.NewFabricAdmin(vendorId=0xFFF1, fabricId=i+1)
            new_admin_ctrl = new_fabric_admin.NewController(nodeId=112233)

            await CommissioningBuildingBlocks.AddNOCForNewFabricFromExisting(
                commissionerDevCtrl=dev_ctrl, newFabricDevCtrl=new_admin_ctrl,
                existingNodeId=self.dut_node_id, newNodeId=self.dut_node_id)

            commissioned_fabric_count = await self.read_single_attribute_check_success(cluster=Clusters.OperationalCredentials, attribute=Clusters.OperationalCredentials.Attributes.CommissionedFabrics)
            asserts.assert_equal(commissioned_fabric_count, initial_commissioned_fabric_count +
                                 i + 1, f'Unsuccessful commissioning of fabric {i}')

        asserts.assert_equal(commissioned_fabric_count, supported_fabrics - 1, 'Incorrect number of fabrics commissioned')


if __name__ == "__main__":
    default_matter_test_main()
