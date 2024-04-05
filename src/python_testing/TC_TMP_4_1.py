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

import chip.clusters as Clusters
from matter_testing_support import MatterBaseTest, async_test_body, default_matter_test_main
from mobly import asserts


class TC_TMP_4_1(MatterBaseTest):

    def pics_TC_TMP_4_1(self) -> list[str]:
        return ["TMP.C"]

    @async_test_body
    async def test_TC_TMP_4_1(self):
        self.print_step(
            1, "DUT retries information from the TMP client using any viable mechanism (read / subscribe / wildcard subscription)")
        input("Press Enter when done.\n")

        self.print_step(
            2, "Please press yes is you TOTALLy promise you're going to do SOMETHING with the data")
        while True:
            answer = input("I promise to do anything with this data (y/n)")
            if answer in ["y", "n", "Y", "N"]:
                break
            print("please press either y or n")

        asserts.assert_equal(answer.lower(), "y",
                             "You need to do things with the data")


if __name__ == "__main__":
    default_matter_test_main()
