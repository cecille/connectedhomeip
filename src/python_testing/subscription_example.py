#
#    Copyright (c) 2024 Project CHIP Authors
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

import chip.clusters as Clusters
from chip import ChipDeviceCtrl
import queue

from chip.testing.matter_testing import MatterBaseTest, async_test_body, default_matter_test_main


class TC_Subscription(MatterBaseTest):
    @async_test_body
    async def test_subscription(self):
        q = queue.Queue()

        def callback(path, transaction):
            print(f"got a subscription update for {path}")
            q.put(path)
        dev_ctrl: ChipDeviceCtrl = self.default_controller
        sub = await dev_ctrl.ReadAttribute(nodeid=self.dut_node_id, attributes=[()], reportInterval=[0, 30])
        sub.SetAttributeUpdateCallback(callback)
        await dev_ctrl.WriteAttribute(nodeid=self.dut_node_id, attributes=[(0, Clusters.BasicInformation.Attributes.NodeLabel("test"))])
        res = q.get(block=True, timeout=5)
        print(f'res = {res}')


if __name__ == "__main__":
    default_matter_test_main()
