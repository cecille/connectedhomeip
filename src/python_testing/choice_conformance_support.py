from conformance_support import Choice
from global_attribute_ids import GlobalAttributeIds
from matter_testing_support import AttributePathLocation, ProblemNotice, ProblemSeverity
from spec_parsing_support import XmlCluster

class ChoiceConformanceProblemNotice(ProblemNotice):
    def __init__(self, location:AttributePathLocation, choice: Choice, count:int):
        problem = f'Problem with choice conformance {choice} - {count} selected'
        super().__init__(test_name='Choice conformance', location=location, severity=ProblemSeverity.ERROR, problem=problem, spec_location='')
        self.choice = choice
        self.count = count

def evaluate_feature_choice_conformance(endpoint_id:int , cluster_id: int, xml_clusters: dict[int, XmlCluster], feature_map:uint, attribute_list: list[uint], all_command_list:list[uint])) -> list[ChoiceConformanceProblemNotice]:
    all_features = [1 << i for i in range(32)]
    all_features = [f for f in all_features if f in xml_clusters[cluster_id].features.keys()]

    problems: list[ChoiceConformanceProblemNotice] = []

    # Other pieces of the 10.2 test check for unknown features, so just remove them here to check choice conformance
    counts: dict[Choice, int] = {}
    for f in all_features:
        xml_feature = xml_clusters[cluster_id].features[f]
        conformance_decision = xml_feature.conformance(feature_map, attribute_list, all_command_list)
        choice = conformance_decision.get_choice()
        if choice:
            counts[choice] = counts.get(choice, 0)
            if feature_map & f:
                counts[choice] += 1

    location = AttributePathLocation(endpoint_id=endpoint_id, cluster_id=cluster_id,
                                     attribute_id=GlobalAttributeIds.FEATURE_MAP_ID)
    for choice, count in counts.items():
        if count == 0 or (not choice.more and count > 1):
            problems.append(ChoiceConformanceProblemNotice(location, choice, count))

    return problems

def evaluate_attribute_choice_conformance(endpoint_id:int , cluster_id: int, xml_clusters: dict[int, XmlCluster], feature_map:uint, attribute_list: list[uint], all_command_list:list[uint])) -> list[ChoiceConformanceProblemNotice]:
    all_features = [1 << i for i in range(32)]
    all_features = [f for f in all_features if f in xml_clusters[cluster_id].features.keys()]

    problems: list[ChoiceConformanceProblemNotice] = []

    # Other pieces of the 10.2 test check for unknown features, so just remove them here to check choice conformance
    counts: dict[Choice, int] = {}
    for f in all_features:
        xml_feature = xml_clusters[cluster_id].features[f]
        conformance_decision = xml_feature.conformance(feature_map, [], [])
        choice = conformance_decision.get_choice()
        if choice:
            counts[choice] = counts.get(choice, 0)
            if feature_map & f:
                counts[choice] += 1

    location = AttributePathLocation(endpoint_id=endpoint_id, cluster_id=cluster_id,
                                     attribute_id=GlobalAttributeIds.FEATURE_MAP_ID)
    for choice, count in counts.items():
        if count == 0 or (not choice.more and count > 1):
            problems.append(ChoiceConformanceProblemNotice(location, choice, count))

    return problems