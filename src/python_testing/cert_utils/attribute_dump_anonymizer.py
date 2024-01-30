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
import ast
import chip.clusters as Clusters
import click
import json
from pathlib import PurePosixPath


def outfile(infile: str) -> str:
    return PurePosixPath(infile).with_stem(f'{PurePosixPath(infile).stem}_anonymized')


def clean_txt_line(line):
    l = line.split(":")
    val = l[1].replace(',', '')
    try:
        val = int(val)
        newval = 0
    except:
        newval = "''"
    return f'{l[0]}: {newval},\n'


def isnum(attrtype: str):
    return attrtype in ["UINT", "INT", "FLOAT", "DOUBLE",]


@click.command()
@click.help_option('-h', '--help')
@click.option(
    '--txt',
    'txtfile',
    type=click.Path(exists=True),
    required=True)
@click.option(
    '--json',
    'jsonfile',
    type=click.Path(exists=True),
    required=True)
def main(txtfile: str, jsonfile: str):

    binfo = Clusters.BasicInformation.Attributes
    remove_attributes = [
        binfo.VendorName,
        binfo.VendorID,
        binfo.ProductName,
        binfo.ProductID,
        binfo.PartNumber,
        binfo.ProductURL,
        binfo.SerialNumber,
        binfo.UniqueID,
    ]

    txtfile_out = outfile(txtfile)
    jsonfile_out = outfile(jsonfile)

    with open(txtfile, "r") as fin:
        with open(txtfile_out, "w") as fout:
            remove_strs = [a.__name__ for a in remove_attributes]
            for line in fin:
                if any(remove_str in line for remove_str in remove_strs):
                    line = clean_txt_line(line)
                fout.write(line)

    with open(jsonfile, "r") as fin:
        with open(jsonfile_out, "w") as fout:
            remove_ids = [(a.cluster_id, a.attribute_id) for a in remove_attributes]
            jsontlv = json.load(fin)
            for endpoint_id, endpoint in jsontlv.items():
                for cluster_id_str, cluster in endpoint.items():
                    s = cluster_id_str.split(':')
                    cluster_id = int(s[0])
                    for attribute_id_str, attribute in cluster.items():
                        s = attribute_id_str.split(':')
                        attribute_id = int(s[0])
                        attribute_type = s[1]
                        if any(remove_cluster_id == cluster_id and remove_attribute_id == attribute_id for remove_cluster_id, remove_attribute_id in remove_ids):
                            # note: only handling strings and nums as that's all we need right now.
                            cluster[attribute_id_str] = 0 if isnum(attribute_type) else ""

            json.dump(jsontlv, fout, indent=2)


if __name__ == "__main__":
    main()
