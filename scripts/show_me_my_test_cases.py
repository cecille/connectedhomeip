import click
import json
from pathlib import Path


@click.command()
@click.option('--pics-tool-json', help="json file downloaded from the PICS tool", required=True, type=click.Path(exists=True))
@click.option('--test-event-list', help="list of test case TCIDs for the test event, one per line no quotes", required=True, type=click.Path(exists=True))
def main(pics_tool_json: Path, test_event_list: Path):
    with open(test_event_list, 'r') as te_list_file:
        te_list = te_list_file.read().splitlines()

    with open(pics_tool_json, 'r') as pics_json_file:
        my_cert_list_json = json.load(pics_json_file)
        my_cert_list = [tc['tcid'] for tc in my_cert_list_json]

    my_te_list = set(my_cert_list).intersection(set(te_list))

    for t in sorted(my_te_list):
        print(t)


if __name__ == "__main__":
    main()
