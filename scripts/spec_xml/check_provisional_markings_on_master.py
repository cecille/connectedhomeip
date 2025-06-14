#
#    Copyright (c) 2025 Project CHIP Authors
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
# This script gives a print out of the differences between the two specified spec
# versions and a description of the provisional elements in the later version.
# Right now, this is just in print form. The intent is to use this for new
# data model XML drops to show the differences. This was also used to double-check
# spec expectations before the 1.4 release and we should continue to do so going forward.

from chip.testing.spec_parsing import PrebuiltDataModelDirectory, build_xml_clusters
from enum import StrEnum
from typing import Any
import chip.testing.pics as pics


class ElementType(StrEnum):
    kFeatures = 'features'
    kAttributes = 'attributes'
    kAcceptedCommands = 'accepted commands'
    kGeneratedCommans = 'generated commands'
    kEvents = 'events'


def print_changes_between_revisions(old_revision: PrebuiltDataModelDirectory, new_revision: PrebuiltDataModelDirectory):
    # TODO: Do we want a way to mark the most recent certified?
    prior_clusters, _ = build_xml_clusters(old_revision)
    new_clusters, _ = build_xml_clusters(new_revision)

    same_ids = set(new_clusters.keys()).intersection(set(prior_clusters.keys()))
    for c in same_ids:
        if new_clusters[c].is_provisional and prior_clusters[c].is_provisional:
            continue

        def print_changes(new: dict[int, Any], old: dict[int, Any], element_type: ElementType):
            new_ids = set(new.keys()) - set(old.keys())

            def print_csv(e: int):

                pics_function = {ElementType.kFeatures: pics.feature_pics_str_from_mask,
                                 ElementType.kAttributes: pics.attribute_pics_str,
                                 ElementType.kAcceptedCommands: pics.accepted_cmd_pics_str,
                                 ElementType.kGeneratedCommans: pics.generated_cmd_pics_str,
                                 ElementType.kEvents: pics.event_pics_str}
                element_pics = pics_function[element_type](new_clusters[c].pics, e)
                old_conformance = str(old[e].conformance) if e in old.keys() else '-'
                added_changed = 'changed' if e in old.keys() else 'added'
                print(
                    f'{element_pics}\t{new_revision.dirname}\t{added_changed}\t{new_clusters[c].name}\t{element_type}\t{new[e].name}\t{old_conformance}\t{str(new[e].conformance)}')

            for e in new_ids:
                print_csv(e)
            conformance_change = [e for e in new.keys() if e in old.keys() and str(new[e].conformance) != str(old[e].conformance)]
            for e in conformance_change:
                print_csv(e)

        print_changes(new_clusters[c].features, prior_clusters[c].features, ElementType.kFeatures)
        print_changes(new_clusters[c].attributes, prior_clusters[c].attributes, ElementType.kAttributes)
        print_changes(new_clusters[c].accepted_commands, prior_clusters[c].accepted_commands, ElementType.kAcceptedCommands)
        print_changes(new_clusters[c].generated_commands, prior_clusters[c].generated_commands, ElementType.kGeneratedCommans)
        print_changes(new_clusters[c].events, prior_clusters[c].events, ElementType.kEvents)


if __name__ == "__main__":
    print('PICS\tRevision\tAdded/changed\tCluster\telement type\telement name\tOld conformance\tNew conformance')
    print_changes_between_revisions(PrebuiltDataModelDirectory.k1_3, PrebuiltDataModelDirectory.k1_4)
    print_changes_between_revisions(PrebuiltDataModelDirectory.k1_4, PrebuiltDataModelDirectory.k1_4_1)
    print_changes_between_revisions(PrebuiltDataModelDirectory.k1_4_1, PrebuiltDataModelDirectory.k1_4_2)
