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

from spec_parsing_support import build_xml_clusters, build_xml_device_types, PrebuiltDataModelDirectory


def get_changes(old, new):
    added = [e.name for id, e in new.items() if id not in old.keys()]
    removed = [e.name for id, e in old.items() if id not in new.keys()]
    same_ids = set(new.keys()).intersection(set(old.keys()))

    return added, removed, same_ids


def str_changes(element, added, removed, change_ids, old, new):
    if not added and not removed and not change_ids:
        return []

    ret = []
    if added:
        ret.append(f'\t{element} added: {added}')
    if removed:
        ret.append(f'\t{element} removed: {removed}')
    if change_ids:
        ret.append(f'\t{element} changed:')
    for id in change_ids:
        name = old[id].name if old[id].name == new[id].name else f'{new[id].name} (previously {old[id].name})'
        ret.append(f'\t\t{name}')
        ret.append(f'\t\t\t{old[id]}')
        ret.append(f'\t\t\t{new[id]}')
    return ret


def str_element_changes(element, old, new):
    added, removed, same_ids = get_changes(old, new)
    change_ids = [id for id in same_ids if old[id] != new[id]]
    return str_changes(element, added, removed, change_ids, old, new)


def diff_clusters():
    prior_clusters, problems = build_xml_clusters(PrebuiltDataModelDirectory.k1_3)
    new_clusters, problems = build_xml_clusters(PrebuiltDataModelDirectory.k1_4)

    additional_clusters, removed_clusters, same_cluster_ids = get_changes(prior_clusters, new_clusters)

    print('\n\nClusters newly added in 1.4')
    print(additional_clusters)
    print('\n\nClusters removed since 1.3')
    print(removed_clusters)

    for cid in same_cluster_ids:
        new = new_clusters[cid]
        old = prior_clusters[cid]

        name = old.name if old.name == new.name else f'{new.name} (previously {old.name})'

        changes = []
        if old.revision != new.revision:
            changes.append(f'\tRevision change - old: {old.revision} new: {new.revision}')
        changes.extend(str_element_changes('Features', old.features, new.features))
        changes.extend(str_element_changes('Attributes', old.attributes, new.attributes))
        changes.extend(str_element_changes('Accepted Commands', old.accepted_commands, new.accepted_commands))
        changes.extend(str_element_changes('Generated Commands', old.generated_commands, new.generated_commands))
        changes.extend(str_element_changes('Events', old.events, new.events))

        if changes:
            print(f'\n\nCluster {name}')
            print('\n'.join(changes))


def diff_device_types():
    prior_device_types, problems = build_xml_device_types(PrebuiltDataModelDirectory.k1_3)
    new_device_types, problems = build_xml_device_types(PrebuiltDataModelDirectory.k1_4)

    additional_device_types, removed_device_types, same_device_type_ids = get_changes(prior_device_types, new_device_types)

    print('\n\nDevice Types newly added in 1.4')
    print(additional_device_types)
    print('\n\nDevice Types removed since 1.3')
    print(removed_device_types)

    for cid in same_device_type_ids:
        new = new_device_types[cid]
        old = prior_device_types[cid]

        name = old.name if old.name == new.name else f'{new.name} (previously {old.name})'

        changes = []
        if old.revision != new.revision:
            changes.append(f'\tRevision change - old: {old.revision} new: {new.revision}')
        changes.extend(str_element_changes('Server Clusters', old.server_clusters, new.server_clusters))
        changes.extend(str_element_changes('Client Clusters', old.client_clusters, new.client_clusters))

        if changes:
            print(f'\n\nDevice Type {name}')
            print('\n'.join(changes))


def main():
    diff_clusters()
    diff_device_types()


if __name__ == "__main__":
    main()
