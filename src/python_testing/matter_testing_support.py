#
#    Copyright (c) 2022 Project CHIP Authors
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

import argparse
import asyncio
import builtins
import inspect
import json
import logging
import os
import pathlib
import re
import sys
import typing
import uuid
from aenum import Enum
from binascii import hexlify, unhexlify
from dataclasses import asdict as dataclass_asdict
from dataclasses import dataclass, field
from datetime import datetime, timedelta, timezone
from typing import List, Optional, Tuple

from chip.tlv import float32, uint

# isort: off

from chip import ChipDeviceCtrl  # Needed before chip.FabricAdmin
import chip.FabricAdmin  # Needed before chip.CertificateAuthority
import chip.CertificateAuthority

# isort: on
import chip.clusters as Clusters
import chip.logging
import chip.native
from chip.ChipStack import ChipStack
from chip.interaction_model import InteractionModelError, Status
from chip.storage import PersistentStorage
from mobly import asserts, base_test, signals, utils
from mobly.config_parser import ENV_MOBLY_LOGPATH, TestRunConfig
from mobly.test_runner import TestRunner

try:
    from matter_yamltests.hooks import TestRunnerHooks
except:
    class TestRunnerHooks(typing.types.NoneType):
        pass


# TODO: Add utility to commission a device if needed
# TODO: Add utilities to keep track of controllers/fabrics

logger = logging.getLogger("matter.python_testing")
logger.setLevel(logging.INFO)

DiscoveryFilterType = ChipDeviceCtrl.DiscoveryFilterType

_DEFAULT_ADMIN_VENDOR_ID = 0xFFF1
_DEFAULT_STORAGE_PATH = "admin_storage.json"
_DEFAULT_LOG_PATH = "/tmp/matter_testing/logs"
_DEFAULT_CONTROLLER_NODE_ID = 112233
_DEFAULT_DUT_NODE_ID = 0x12344321
_DEFAULT_TRUST_ROOT_INDEX = 1

# Mobly cannot deal with user config passing of ctypes objects,
# so we use this dict of uuid -> object to recover items stashed
# by reference.
_GLOBAL_DATA = {}


def stash_globally(o: object) -> str:
    id = str(uuid.uuid1())
    _GLOBAL_DATA[id] = o
    return id


def unstash_globally(id: str) -> object:
    return _GLOBAL_DATA.get(id)


def default_paa_rootstore_from_root(root_path: pathlib.Path) -> Optional[pathlib.Path]:
    """Attempt to find a PAA trust store following SDK convention at `root_path`

    This attempts to find {root_path}/credentials/development/paa-root-certs.

    Returns the fully resolved path on success or None if not found.
    """
    start_path = root_path.resolve()
    cred_path = start_path.joinpath("credentials")
    dev_path = cred_path.joinpath("development")
    paa_path = dev_path.joinpath("paa-root-certs")

    return paa_path.resolve() if all([path.exists() for path in [cred_path, dev_path, paa_path]]) else None


def get_default_paa_trust_store(root_path: pathlib.Path) -> pathlib.Path:
    """Attempt to find a PAA trust store starting at `root_path`.

    This tries to find by various heuristics, and goes up one level at a time
    until found. After a given number of levels, it will stop.

    This returns `root_path` if not PAA store is not found.
    """
    # TODO: Add heuristics about TH default PAA location
    cur_dir = pathlib.Path.cwd()
    max_levels = 10

    for level in range(max_levels):
        paa_trust_store_path = default_paa_rootstore_from_root(cur_dir)
        if paa_trust_store_path is not None:
            return paa_trust_store_path

        # Go back one level
        cur_dir = cur_dir.joinpath("..")
    else:
        # On not having found a PAA dir, just return current dir to avoid blow-ups
        return pathlib.Path.cwd()


def parse_pics(lines=typing.List[str]) -> dict[str, bool]:
    pics = {}
    for raw in lines:
        line, _, _ = raw.partition("#")
        line = line.strip()

        if not line:
            continue

        key, _, val = line.partition("=")
        val = val.strip()
        if val not in ["1", "0"]:
            raise ValueError('PICS {} must have a value of 0 or 1'.format(key))

        pics[key.strip().upper()] = (val == "1")
    return pics


def read_pics_from_file(filename: str) -> dict[str, bool]:
    """ Reads a dictionary of PICS from a file. """
    with open(filename, 'r') as f:
        lines = f.readlines()
        return parse_pics(lines)


def type_matches(received_value, desired_type):
    """ Checks if the value received matches the expected type.

        Handles unpacking Nullable and Optional types and
        compares list value types for non-empty lists.
    """
    if typing.get_origin(desired_type) == typing.Union:
        return any(type_matches(received_value, t) for t in typing.get_args(desired_type))
    elif typing.get_origin(desired_type) == list:
        if isinstance(received_value, list):
            # Assume an empty list is of the correct type
            return True if received_value == [] else any(type_matches(received_value[0], t) for t in typing.get_args(desired_type))
        else:
            return False
    elif desired_type == uint:
        return isinstance(received_value, int) and received_value >= 0
    elif desired_type == float32:
        return isinstance(received_value, float)
    else:
        return isinstance(received_value, desired_type)


def utc_time_in_matter_epoch(desired_datetime: datetime = None):
    """ Returns the time in matter epoch in us.

        If desired_datetime is None, it will return the current time.
    """
    if desired_datetime is None:
        utc_native = datetime.now(tz=timezone.utc)
    else:
        utc_native = desired_datetime
    # Matter epoch is 0 hours, 0 minutes, 0 seconds on Jan 1, 2000 UTC
    utc_th_delta = utc_native - datetime(2000, 1, 1, 0, 0, 0, 0, timezone.utc)
    utc_th_us = int(utc_th_delta.total_seconds() * 1000000)
    return utc_th_us


def compare_time(received: int, offset: timedelta = timedelta(), utc: int = None, tolerance: timedelta = timedelta(seconds=5)) -> None:
    if utc is None:
        utc = utc_time_in_matter_epoch()

    expected = utc + offset.microseconds
    delta_us = abs(expected - received)
    delta = timedelta(microseconds=delta_us)
    asserts.assert_less(delta, tolerance, "Received time is out of tolerance")


class InternalTestRunnerHooks(TestRunnerHooks):

    def start(self, count: int):
        logging.info(f'Starting test set, running {count} tests')

    def stop(self, duration: int):
        logging.info(f'Finished test set, ran for {duration}ms')

    def test_start(self, filename: str, name: str, count: int):
        logging.info(f'Starting test from {filename}: {name} - {count} steps')

    def test_stop(self, exception: Exception, duration: int):
        logging.info(f'Finished test in {duration}ms')

    def step_skipped(self, name: str, expression: str):
        # TODO: Do we really need the expression as a string? We can evaluate this in code very easily
        logging.info(f'\t\t**** Skipping: {name}')

    def step_start(self, name: str):
        # The way I'm calling this, the name is already includes the step number, but it seems like it might be good to separate these
        logging.info(f'\t\t***** Test Step {name}')

    def step_success(self, logger, logs, duration: int, request):
        pass

    def step_failure(self, logger, logs, duration: int, request, received):
        # TODO: there's supposed to be some kind of error message here, but I have no idea where it's meant to come from in this API
        logging.info(f'\t\t***** Test Failure : ')

    def step_unknown(self):
        """
        This method is called when the result of running a step is unknown. For example during a dry-run.
        """
        pass


@dataclass
class MatterTestConfig:
    storage_path: pathlib.Path = None
    logs_path: pathlib.Path = None
    paa_trust_store_path: pathlib.Path = None
    ble_interface_id: int = None
    commission_only: bool = False

    admin_vendor_id: int = _DEFAULT_ADMIN_VENDOR_ID
    case_admin_subject: int = None
    global_test_params: dict = field(default_factory=dict)
    # List of explicit tests to run by name. If empty, all tests will run
    tests: List[str] = field(default_factory=list)

    commissioning_method: str = None
    discriminator: List[int] = None
    setup_passcode: List[int] = None
    commissionee_ip_address_just_for_testing: str = None
    maximize_cert_chains: bool = False

    qr_code_content: str = None
    manual_code: str = None

    wifi_ssid: str = None
    wifi_passphrase: str = None
    thread_operational_dataset: str = None

    # Node ID for basic DUT
    dut_node_id: List[int] = None
    # Node ID to use for controller/commissioner
    controller_node_id: int = _DEFAULT_CONTROLLER_NODE_ID
    # CAT Tags for default controller/commissioner
    controller_cat_tags: List[int] = field(default_factory=list)

    # Fabric ID which to use
    fabric_id: int = None
    # "Alpha" by default
    root_of_trust_index: int = _DEFAULT_TRUST_ROOT_INDEX

    # If this is set, we will reuse root of trust keys at that location
    chip_tool_credentials_path: pathlib.Path = None

    # Test timeout in seconds
    timeout: int = None

    # endpoint
    endpoint: int = 0


class MatterStackState:
    def __init__(self, config: MatterTestConfig):
        self._logger = logger
        self._config = config

        if not hasattr(builtins, "chipStack"):
            chip.native.Init(bluetoothAdapter=config.ble_interface_id)
            if config.storage_path is None:
                raise ValueError("Must have configured a MatterTestConfig.storage_path")
            self._init_stack(already_initialized=False, persistentStoragePath=config.storage_path)
            self._we_initialized_the_stack = True
        else:
            self._init_stack(already_initialized=True)
            self._we_initialized_the_stack = False

    def _init_stack(self, already_initialized: bool, **kwargs):
        if already_initialized:
            self._chip_stack = builtins.chipStack
            self._logger.warn(
                "Re-using existing ChipStack object found in current interpreter: "
                "storage path %s will be ignored!" % (self._config.storage_path)
            )
            # TODO: Warn that storage will not follow what we set in config
        else:
            self._chip_stack = ChipStack(**kwargs)
            builtins.chipStack = self._chip_stack

        self._storage = self._chip_stack.GetStorageManager()
        self._certificate_authority_manager = chip.CertificateAuthority.CertificateAuthorityManager(chipStack=self._chip_stack)
        self._certificate_authority_manager.LoadAuthoritiesFromStorage()

        if (len(self._certificate_authority_manager.activeCaList) == 0):
            self._logger.warn(
                "Didn't find any CertificateAuthorities in storage -- creating a new CertificateAuthority + FabricAdmin...")
            ca = self._certificate_authority_manager.NewCertificateAuthority(caIndex=self._config.root_of_trust_index)
            ca.maximizeCertChains = self._config.maximize_cert_chains
            ca.NewFabricAdmin(vendorId=0xFFF1, fabricId=self._config.fabric_id)
        elif (len(self._certificate_authority_manager.activeCaList[0].adminList) == 0):
            self._logger.warn("Didn't find any FabricAdmins in storage -- creating a new one...")
            self._certificate_authority_manager.activeCaList[0].NewFabricAdmin(vendorId=0xFFF1, fabricId=self._config.fabric_id)

    # TODO: support getting access to chip-tool credentials issuer's data

    def Shutdown(self):
        if self._we_initialized_the_stack:
            # Unfortunately, all the below are singleton and possibly
            # managed elsewhere so we have to be careful not to touch unless
            # we initialized ourselves.
            self._certificate_authority_manager.Shutdown()
            global_chip_stack = builtins.chipStack
            global_chip_stack.Shutdown()

    @property
    def certificate_authorities(self):
        return self._certificate_authority_manager.activeCaList

    @property
    def certificate_authority_manager(self):
        return self._certificate_authority_manager

    @property
    def storage(self) -> PersistentStorage:
        return self._storage

    @property
    def stack(self) -> ChipStack:
        return builtins.chipStack


def bytes_from_hex(hex: str) -> bytes:
    """Converts any `hex` string representation including `01:ab:cd` to bytes

    Handles any whitespace including newlines, which are all stripped.
    """
    return unhexlify("".join(hex.replace(":", "").replace(" ", "").split()))


def hex_from_bytes(b: bytes) -> str:
    """Converts a bytes object `b` into a hex string (reverse of bytes_from_hex)"""
    return hexlify(b).decode("utf-8")


@dataclass
class TestStep:
    test_plan_number: typing.Union[int, str]
    description: str
    is_commissioning: bool = False


class MatterBaseTest(base_test.BaseTestClass):

    def __init__(self, *args):
        super().__init__(*args)
        self.current_step_index = 0
        self.step_start_time = datetime.now(timezone.utc)
        self.step_skipped = False

    # ---- Functions that the derived classes should implement
    def get_test_steps(self) -> list[TestStep]:
        ''' Returns a list of test steps that will be called in order

            Test that derive from this base class should call each step
            in order using self.step(number), where number is the test_plan_numer
            from each TestStep.
        '''
        pass

    def get_test_desc(self) -> str:
        ''' Returns a description of this test

            Format:
            <Test plan reference> [<test plan number>] <test plan name>

            ex:
            133.1.1. [TC-ACL-1.1] Global attributes            
        '''
        return self.__class__.__name__

    @property
    def runner_hook(self) -> TestRunnerHooks:
        return unstash_globally(self.user_params.get("hooks"))

    @property
    def matter_test_config(self) -> MatterTestConfig:
        return unstash_globally(self.user_params.get("matter_test_config"))

    @property
    def default_controller(self) -> ChipDeviceCtrl:
        return unstash_globally(self.user_params.get("default_controller"))

    @property
    def matter_stack(self) -> MatterStackState:
        return unstash_globally(self.user_params.get("matter_stack"))

    @property
    def certificate_authority_manager(self) -> chip.CertificateAuthority.CertificateAuthorityManager:
        return unstash_globally(self.user_params.get("certificate_authority_manager"))

    @property
    def dut_node_id(self) -> int:
        return self.matter_test_config.dut_node_id[0]

    def check_pics(self, pics_key: str) -> bool:
        picsd = self.matter_test_config.pics
        pics_key = pics_key.strip().upper()
        return pics_key in picsd and picsd[pics_key]

    async def read_single_attribute(
            self, dev_ctrl: ChipDeviceCtrl, node_id: int, endpoint: int, attribute: object, fabricFiltered: bool = True) -> object:
        result = await dev_ctrl.ReadAttribute(node_id, [(endpoint, attribute)], fabricFiltered=fabricFiltered)
        data = result[endpoint]
        return list(data.values())[0][attribute]

    async def read_single_attribute_check_success(
            self, cluster: Clusters.ClusterObjects.ClusterCommand, attribute: Clusters.ClusterObjects.ClusterAttributeDescriptor,
            dev_ctrl: ChipDeviceCtrl = None, node_id: int = None, endpoint: int = None) -> object:
        if dev_ctrl is None:
            dev_ctrl = self.default_controller
        if node_id is None:
            node_id = self.dut_node_id
        if endpoint is None:
            endpoint = self.matter_test_config.endpoint

        result = await dev_ctrl.ReadAttribute(node_id, [(endpoint, attribute)])
        attr_ret = result[endpoint][cluster][attribute]
        err_msg = "Error reading {}:{}".format(str(cluster), str(attribute))
        asserts.assert_true(attr_ret is not None, err_msg)
        asserts.assert_false(isinstance(attr_ret, Clusters.Attribute.ValueDecodeFailure), err_msg)
        desired_type = attribute.attribute_type.Type
        asserts.assert_true(type_matches(attr_ret, desired_type),
                            'Returned attribute {} is wrong type expected {}, got {}'.format(attribute, desired_type, type(attr_ret)))
        return attr_ret

    async def read_single_attribute_expect_error(
            self, cluster: object, attribute: object,
            error: Status, dev_ctrl: ChipDeviceCtrl = None, node_id: int = None, endpoint: int = None) -> object:
        if dev_ctrl is None:
            dev_ctrl = self.default_controller
        if node_id is None:
            node_id = self.dut_node_id
        if endpoint is None:
            endpoint = self.matter_test_config.endpoint

        result = await dev_ctrl.ReadAttribute(node_id, [(endpoint, attribute)])
        attr_ret = result[endpoint][cluster][attribute]
        err_msg = "Did not see expected error when reading {}:{}".format(str(cluster), str(attribute))
        asserts.assert_true(attr_ret is not None, err_msg)
        asserts.assert_true(isinstance(attr_ret, Clusters.Attribute.ValueDecodeFailure), err_msg)
        asserts.assert_true(isinstance(attr_ret.Reason, InteractionModelError), err_msg)
        asserts.assert_equal(attr_ret.Reason.status, error, err_msg)
        return attr_ret

    async def send_single_cmd(
            self, cmd: Clusters.ClusterObjects.ClusterCommand,
            dev_ctrl: ChipDeviceCtrl = None, node_id: int = None, endpoint: int = None,
            timedRequestTimeoutMs: typing.Union[None, int] = None) -> object:
        if dev_ctrl is None:
            dev_ctrl = self.default_controller
        if node_id is None:
            node_id = self.dut_node_id

        result = await dev_ctrl.SendCommand(nodeid=node_id, endpoint=endpoint, payload=cmd, timedRequestTimeoutMs=timedRequestTimeoutMs)
        return result

    def on_fail(self, record):
        ''' Called by Mobly on test failure

            record is of type TestResultRecord
        '''
        if self.runner_hook:
            exception = record.termination_signal.exception
            step_duration = (datetime.now(timezone.utc) - self.step_start_time) / timedelta(microseconds=1)
            # This isn't QUITE the test duration because the commissioning is handled separately, but it's clsoe enough for now
            # This is already given in milliseconds
            test_duration = record.end_time - record.begin_time
            # TODO: I have no idea what logger, logs, request or received are. Hope None works becuase I have nothing to give
            self.runner_hook.step_failure(logger=None, logs=None, duration=step_duration, request=None, received=None)
            self.runner_hook.test_stop(exception=exception, duration=test_duration)

    def on_pass(self, record):
        ''' Called by Mobly on test pass

            record is of type TestResultRecord
        '''
        if self.runner_hook:
            # What is request? This seems like an implementation detail for the runner
            # TODO: As with failure, I have no idea what loger, logs or request are meant to be
            step_duration = (datetime.now(timezone.utc) - self.step_start_time) / timedelta(microseconds=1)
            test_duration = record.end_time - record.begin_time
            self.runner_hook.step_success(logger=None, logs=None, duration=step_duration, request=None)

        # TODO: this check could easily be annoying when doing dev. flag it somehow? Ditto with the in-order check
        try:
            steps = self.get_test_steps()
            all_steps_run = len(steps) == self.current_step_index
        except:
            # if we don't have a list of steps, assume they were all run
            all_steps_run = True

        if not all_steps_run:
            # The test is done, but we didn't execute all the steps
            asserts.fail("Test script error: Not all required steps were run")

        if self.runner_hook:
            self.runner_hook.test_stop(exception=None, duration=test_duration)

    def pics_guard(self, pics_condition: bool):
        if not pics_condition:
            try:
                steps = self.get_test_steps()
                num = steps[self.current_step_index].test_plan_number
            except:
                num = self.current_step_index

            if self.runner_hook:
                # TODO: what does name represent here? The wordy test name? The test plan number? The nnumber and name?
                # TODO: I very much do not want to have people passing in strings here. Do we really need the expression
                #       as a string? Does it get used by the TH?
                self.runner_hook.step_skipped(name=str(num), expression="")
            else:
                logging.info(f'**** Skipping: {num}')
            self.step_skipped = True

    def step(self, step: typing.Union[int, str]):
        try:
            steps = self.get_test_steps()
        except:
            asserts.fail("Test script failure: test steps not provided")

        if len(steps) <= self.current_step_index or steps[self.current_step_index].test_plan_number != step:
            asserts.fail(f'Unexpected test step: {step} - steps not called in order, or step does not exist')
        if self.current_step_index == 0 and self.runner_hook:
            filename = inspect.getfile(self.__class__)
            self.runner_hook.test_start(filename=filename, name=self.get_test_desc, count=len(steps))

        if self.runner_hook:
            # If we've reached the next step with no assertion and the step wasn't skipped, it passed
            if not self.step_skipped:
                # TODO: As with failure, I have no idea what loger, logs or request are meant to be
                step_duration = (datetime.now(timezone.utc) - self.step_start_time) / timedelta(microseconds=1)
                self.runner_hook.step_success(logger=None, logs=None, duration=step_duration, request=None)

            # TODO: it seems like the step start should take a number and a name
            name = f'{step} : {steps[self.current_step_index].description}'
            self.runner_hook.step_start(name=name)
        else:
            self.print_step(step, steps[self.current_step_index].description)

        self.step_start_time = utc_native = datetime.now(tz=timezone.utc)
        self.current_step_index = self.current_step_index + 1
        self.step_skipped = False

    def print_step(self, stepnum: typing.Union[int, str], title: str) -> None:
        logging.info('***** Test Step {} : {}'.format(stepnum, title))


def generate_mobly_test_config(matter_test_config: MatterTestConfig):
    test_run_config = TestRunConfig()
    # We use a default name. We don't use Mobly YAML configs, so that we can be
    # freestanding without relying
    test_run_config.testbed_name = "MatterTest"

    log_path = matter_test_config.logs_path
    log_path = _DEFAULT_LOG_PATH if log_path is None else log_path
    if ENV_MOBLY_LOGPATH in os.environ:
        log_path = os.environ[ENV_MOBLY_LOGPATH]

    test_run_config.log_path = log_path
    # TODO: For later, configure controllers
    test_run_config.controller_configs = {}

    test_run_config.user_params = matter_test_config.global_test_params

    return test_run_config


def _find_test_class():
    """Finds the test class in a test script.
    Walk through module members and find the subclass of MatterBaseTest. Only
    one subclass is allowed in a test script.
    Returns:
      The test class in the test module.
    Raises:
      SystemExit: Raised if the number of test classes is not exactly one.
    """
    subclasses = utils.find_subclasses_in_module([MatterBaseTest], sys.modules['__main__'])
    subclasses = [c for c in subclasses if c.__name__ != "MatterBaseTest"]
    if len(subclasses) != 1:
        print(
            'Exactly one subclass of `MatterBaseTest` should be in the main file. Found %s.' %
            str([subclass.__name__ for subclass in subclasses]))
        sys.exit(1)

    return subclasses[0]


def int_decimal_or_hex(s: str) -> int:
    val = int(s, 0)
    if val < 0:
        raise ValueError("Negative values not supported")
    return val


def byte_string_from_hex(s: str) -> bytes:
    return unhexlify(s.replace(":", "").replace(" ", "").replace("0x", ""))


def int_from_manual_code(s: str) -> int:
    regex = r"^([0-9]{11}|[0-9]{21})$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid manual code format, does not match %s" % regex)

    return int(s, 10)


def int_named_arg(s: str) -> Tuple[str, int]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):((?P<hex_value>0x[0-9a-fA-F_]+)|(?P<decimal_value>-?\d+))$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid int argument format, does not match %s" % regex)

    name = match.group("name")
    if match.group("hex_value"):
        value = int(match.group("hex_value"), 0)
    else:
        value = int(match.group("decimal_value"), 10)
    return (name, value)


def str_named_arg(s: str) -> Tuple[str, str]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):(?P<value>.*)$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid string argument format, does not match %s" % regex)

    return (match.group("name"), match.group("value"))


def float_named_arg(s: str) -> Tuple[str, float]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):(?P<value>.*)$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid float argument format, does not match %s" % regex)

    name = match.group("name")
    value = float(match.group("value"))

    return (name, value)


def json_named_arg(s: str) -> Tuple[str, object]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):(?P<value>.*)$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid JSON argument format, does not match %s" % regex)

    name = match.group("name")
    value = json.loads(match.group("value"))

    return (name, value)


def bool_named_arg(s: str) -> Tuple[str, bool]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):((?P<truth_value>true|false)|(?P<decimal_value>[01]))$"
    match = re.match(regex, s.lower())
    if not match:
        raise ValueError("Invalid bool argument format, does not match %s" % regex)

    name = match.group("name")
    if match.group("truth_value"):
        value = True if match.group("truth_value") == "true" else False
    else:
        value = int(match.group("decimal_value")) != 0

    return (name, value)


def bytes_as_hex_named_arg(s: str) -> Tuple[str, bytes]:
    regex = r"^(?P<name>[a-zA-Z_0-9.]+):(?P<value>[0-9a-fA-F:]+)$"
    match = re.match(regex, s)
    if not match:
        raise ValueError("Invalid bytes as hex argument format, does not match %s" % regex)

    name = match.group("name")
    value_str = match.group("value")
    value_str = value_str.replace(":", "")
    if len(value_str) % 2 != 0:
        raise ValueError("Byte string argument value needs to be event number of hex chars")
    value = unhexlify(value_str)

    return (name, value)


def root_index(s: str) -> int:
    CHIP_TOOL_COMPATIBILITY = {
        "alpha": 1,
        "beta": 2,
        "gamma": 3
    }

    for name, id in CHIP_TOOL_COMPATIBILITY.items():
        if s.lower() == name:
            return id
    else:
        root_index = int(s)
        if root_index == 0:
            raise ValueError("Only support root index >= 1")
        return root_index


def populate_commissioning_args(args: argparse.Namespace, config: MatterTestConfig) -> bool:
    if args.commissioning_method is None:
        return True

    config.commissioning_method = args.commissioning_method
    config.commission_only = args.commission_only

    if args.dut_node_id is None:
        print("error: When --commissioning-method present, --dut-node-id is mandatory!")
        return False
    config.dut_node_id = args.dut_node_id

    if args.discriminator is None and (args.qr_code is None and args.manual_code is None):
        print("error: Missing --discriminator when no --qr-code/--manual-code present!")
        return False
    config.discriminator = args.discriminator

    if args.passcode is None and (args.qr_code is None and args.manual_code is None):
        print("error: Missing --passcode when no --qr-code/--manual-code present!")
        return False
    config.setup_passcode = args.passcode

    if args.qr_code is not None and args.manual_code is not None:
        print("error: Cannot have both --qr-code and --manual-code present!")
        return False

    if len(config.discriminator) != len(config.setup_passcode):
        print("error: supplied number of discriminators does not match number of passcodes")
        return False

    if len(config.dut_node_id) > len(config.discriminator):
        print("error: More node IDs provided than discriminators")
        return False

    if len(config.dut_node_id) < len(config.discriminator):
        missing = len(config.discriminator) - len(config.dut_node_id)
        for i in range(missing):
            config.dut_node_id.append(config.dut_node_id[-1] + 1)

    if len(config.dut_node_id) != len(set(config.dut_node_id)):
        print("error: Duplicate values in node id list")
        return False

    if len(config.discriminator) != len(set(config.discriminator)):
        print("error: Duplicate value in discriminator list")
        return False

    # TODO: this should also allow multiple once QR and manual codes are supported.
    config.qr_code_content = args.qr_code
    config.manual_code = args.manual_code

    config.root_of_trust_index = args.root_index
    # Follow root of trust index if ID not provided to have same behavior as legacy
    # chip-tool that fabricID == commissioner_name == root of trust index
    config.fabric_id = args.fabric_id if args.fabric_id is not None else config.root_of_trust_index

    if args.chip_tool_credentials_path is not None and not args.chip_tool_credentials_path.exists():
        print("error: chip-tool credentials path %s doesn't exist!" % args.chip_tool_credentials_path)
        return False
    config.chip_tool_credentials_path = args.chip_tool_credentials_path

    if config.commissioning_method == "ble-wifi":
        if args.wifi_ssid is None:
            print("error: missing --wifi-ssid <SSID> for --commissioning-method ble-wifi!")
            return False

        if args.wifi_passphrase is None:
            print("error: missing --wifi-passphrase <passphrasse> for --commissioning-method ble-wifi!")
            return False

        config.wifi_ssid = args.wifi_ssid
        config.wifi_passphrase = args.wifi_passphrase
    elif config.commissioning_method == "ble-thread":
        if args.thread_dataset_hex is None:
            print("error: missing --thread-dataset-hex <DATASET_HEX> for --commissioning-method ble-thread!")
            return False
        config.thread_operational_dataset = args.thread_dataset_hex
    elif config.commissioning_method == "on-network-ip":
        if args.ip_addr is None:
            print("error: missing --ip-addr <IP_ADDRESS> for --commissioning-method on-network-ip")
            return False
        config.commissionee_ip_address_just_for_testing = args.ip_addr

    if args.case_admin_subject is None:
        # Use controller node ID as CASE admin subject during commissioning if nothing provided
        config.case_admin_subject = config.controller_node_id
    else:
        # If a CASE admin subject is provided, then use that
        config.case_admin_subject = args.case_admin_subject

    return True


def convert_args_to_matter_config(args: argparse.Namespace) -> MatterTestConfig:
    config = MatterTestConfig()

    # Populate commissioning config if present, exiting on error
    if not populate_commissioning_args(args, config):
        sys.exit(1)

    config.storage_path = pathlib.Path(_DEFAULT_STORAGE_PATH) if args.storage_path is None else args.storage_path
    config.logs_path = pathlib.Path(_DEFAULT_LOG_PATH) if args.logs_path is None else args.logs_path
    config.paa_trust_store_path = args.paa_trust_store_path
    config.ble_interface_id = args.ble_interface_id
    config.pics = {} if args.PICS is None else read_pics_from_file(args.PICS)
    config.timeout = 90 if args.timeout is None else args.timeout

    config.controller_node_id = args.controller_node_id

    # Accumulate all command-line-passed named args
    all_global_args = []
    argsets = [item for item in (args.int_arg, args.float_arg, args.string_arg, args.json_arg,
                                 args.hex_arg, args.bool_arg) if item is not None]
    for argset in argsets:
        all_global_args.extend(argset)

    config.global_test_params = {}
    for name, value in all_global_args:
        config.global_test_params[name] = value

    # Embed the rest of the config in the global test params dict which will be passed to Mobly tests
    config.global_test_params["meta_config"] = {k: v for k, v in dataclass_asdict(config).items() if k != "global_test_params"}

    return config


def parse_matter_test_args(argv: List[str]) -> MatterTestConfig:
    parser = argparse.ArgumentParser(description='Matter standalone Python test')

    basic_group = parser.add_argument_group(title="Basic arguments", description="Overall test execution arguments")

    basic_group.add_argument('--tests',
                             '--test_case',
                             action="store",
                             nargs='+',
                             type=str,
                             metavar='test_a test_b...',
                             help='A list of tests in the test class to execute.')

    basic_group.add_argument('--storage-path', action="store", type=pathlib.Path,
                             metavar="PATH", help="Location for persisted storage of instance")
    basic_group.add_argument('--logs-path', action="store", type=pathlib.Path, metavar="PATH", help="Location for test logs")
    paa_path_default = get_default_paa_trust_store(pathlib.Path.cwd())
    basic_group.add_argument('--paa-trust-store-path', action="store", type=pathlib.Path, metavar="PATH", default=paa_path_default,
                             help="PAA trust store path (default: %s)" % str(paa_path_default))
    basic_group.add_argument('--ble-interface-id', action="store", type=int,
                             metavar="INTERFACE_ID", help="ID of BLE adapter (from hciconfig)")
    basic_group.add_argument('-N', '--controller-node-id', type=int_decimal_or_hex,
                             metavar='NODE_ID',
                             default=_DEFAULT_CONTROLLER_NODE_ID,
                             help='NodeID to use for initial/default controller (default: %d)' % _DEFAULT_CONTROLLER_NODE_ID)
    basic_group.add_argument('-n', '--dut-node-id', '--nodeId', type=int_decimal_or_hex,
                             metavar='NODE_ID', default=[_DEFAULT_DUT_NODE_ID],
                             help='Node ID for primary DUT communication, '
                             'and NodeID to assign if commissioning (default: %d)' % _DEFAULT_DUT_NODE_ID, nargs="+")
    basic_group.add_argument('--endpoint', type=int, default=0, help="Endpoint under test")
    basic_group.add_argument('--timeout', type=int, default=90, help="Test timeout in seconds")
    basic_group.add_argument("--PICS", help="PICS file path", type=str)

    commission_group = parser.add_argument_group(title="Commissioning", description="Arguments to commission a node")

    commission_group.add_argument('-m', '--commissioning-method', type=str,
                                  metavar='METHOD_NAME',
                                  choices=["on-network", "ble-wifi", "ble-thread", "on-network-ip"],
                                  help='Name of commissioning method to use')
    commission_group.add_argument('-d', '--discriminator', type=int_decimal_or_hex,
                                  metavar='LONG_DISCRIMINATOR',
                                  help='Discriminator to use for commissioning', nargs="+")
    commission_group.add_argument('-p', '--passcode', type=int_decimal_or_hex,
                                  metavar='PASSCODE',
                                  help='PAKE passcode to use', nargs="+")
    commission_group.add_argument('-i', '--ip-addr', type=str,
                                  metavar='RAW_IP_ADDRESS',
                                  help='IP address to use (only for method "on-network-ip". ONLY FOR LOCAL TESTING!', nargs="+")

    commission_group.add_argument('--wifi-ssid', type=str,
                                  metavar='SSID',
                                  help='Wi-Fi SSID for ble-wifi commissioning')
    commission_group.add_argument('--wifi-passphrase', type=str,
                                  metavar='PASSPHRASE',
                                  help='Wi-Fi passphrase for ble-wifi commissioning')

    commission_group.add_argument('--thread-dataset-hex', type=byte_string_from_hex,
                                  metavar='OPERATIONAL_DATASET_HEX',
                                  help='Thread operational dataset as a hex string for ble-thread commissioning')

    commission_group.add_argument('--admin-vendor-id', action="store", type=int_decimal_or_hex, default=_DEFAULT_ADMIN_VENDOR_ID,
                                  metavar="VENDOR_ID",
                                  help="VendorID to use during commissioning (default 0x%04X)" % _DEFAULT_ADMIN_VENDOR_ID)
    commission_group.add_argument('--case-admin-subject', action="store", type=int_decimal_or_hex,
                                  metavar="CASE_ADMIN_SUBJECT",
                                  help="Set the CASE admin subject to an explicit value (default to commissioner Node ID)")

    commission_group.add_argument('--commission-only', action="store_true", default=False,
                                  help="If true, test exits after commissioning without running subsequent tests")

    code_group = parser.add_mutually_exclusive_group(required=False)

    code_group.add_argument('-q', '--qr-code', type=str,
                            metavar="QR_CODE", help="QR setup code content (overrides passcode and discriminator)")
    code_group.add_argument('--manual-code', type=int_from_manual_code,
                            metavar="MANUAL_CODE", help="Manual setup code content (overrides passcode and discriminator)")

    fabric_group = parser.add_argument_group(
        title="Fabric selection", description="Fabric selection for single-fabric basic usage, and commissioning")
    fabric_group.add_argument('-f', '--fabric-id', type=int_decimal_or_hex,
                              metavar='FABRIC_ID',
                              help='Fabric ID on which to operate under the root of trust')

    fabric_group.add_argument('-r', '--root-index', type=root_index,
                              metavar='ROOT_INDEX_OR_NAME', default=_DEFAULT_TRUST_ROOT_INDEX,
                              help='Root of trust under which to operate/commission for single-fabric basic usage. '
                              'alpha/beta/gamma are aliases for 1/2/3. Default (%d)' % _DEFAULT_TRUST_ROOT_INDEX)

    fabric_group.add_argument('-c', '--chip-tool-credentials-path', type=pathlib.Path,
                              metavar='PATH',
                              help='Path to chip-tool credentials file root')

    args_group = parser.add_argument_group(title="Config arguments", description="Test configuration global arguments set")
    args_group.add_argument('--int-arg', nargs='*', type=int_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for an integer as hex or decimal (e.g. -2 or 0xFFFF_1234)")
    args_group.add_argument('--bool-arg', nargs='*', type=bool_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for an boolean value (e.g. true/false or 0/1)")
    args_group.add_argument('--float-arg', nargs='*', type=float_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for a floating point value (e.g. -2.1 or 6.022e23)")
    args_group.add_argument('--string-arg', nargs='*', type=str_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for a string value")
    args_group.add_argument('--json-arg', nargs='*', type=json_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for JSON stored as a list or dict")
    args_group.add_argument('--hex-arg', nargs='*', type=bytes_as_hex_named_arg, metavar="NAME:VALUE",
                            help="Add a named test argument for an octet string in hex (e.g. 0011cafe or 00:11:CA:FE)")

    if not argv:
        argv = sys.argv[1:]

    return convert_args_to_matter_config(parser.parse_known_args(argv)[0])


def async_test_body(body):
    """Decorator required to be applied whenever a `test_*` method is `async def`.

    Since Mobly doesn't support asyncio directly, and the test methods are called
    synchronously, we need a mechanism to allow an `async def` to be converted to
    a asyncio-run synchronous method. This decorator does the wrapping.
    """

    def async_runner(self: MatterBaseTest, *args, **kwargs):
        runner_with_timeout = asyncio.wait_for(body(self, *args, **kwargs), timeout=self.matter_test_config.timeout)
        return asyncio.run(runner_with_timeout)

    return async_runner


class CommissionDeviceTest(MatterBaseTest):
    """Test class auto-injected at the start of test list to commission a device when requested"""

    def test_run_commissioning(self):
        conf = self.matter_test_config
        for i in range(len(conf.dut_node_id)):
            logging.info("Starting commissioning for root index %d, fabric ID 0x%016X, node ID 0x%016X" %
                         (conf.root_of_trust_index, conf.fabric_id, conf.dut_node_id[i]))
            logging.info("Commissioning method: %s" % conf.commissioning_method)

            if not self._commission_device(i):
                raise signals.TestAbortAll("Failed to commission node")

    def _commission_device(self, i) -> bool:
        dev_ctrl = self.default_controller
        conf = self.matter_test_config

        # TODO: support by manual code and QR

        if conf.commissioning_method == "on-network":
            return dev_ctrl.CommissionOnNetwork(
                nodeId=conf.dut_node_id[i],
                setupPinCode=conf.setup_passcode[i],
                filterType=DiscoveryFilterType.LONG_DISCRIMINATOR,
                filter=conf.discriminator[i]
            )
        elif conf.commissioning_method == "ble-wifi":
            return dev_ctrl.CommissionWiFi(
                conf.discriminator[i],
                conf.setup_passcode[i],
                conf.dut_node_id[i],
                conf.wifi_ssid,
                conf.wifi_passphrase
            )
        elif conf.commissioning_method == "ble-thread":
            return dev_ctrl.CommissionThread(
                conf.discriminator[i],
                conf.setup_passcode[i],
                conf.dut_node_id[i],
                conf.thread_operational_dataset
            )
        elif conf.commissioning_method == "on-network-ip":
            logging.warning("==== USING A DIRECT IP COMMISSIONING METHOD NOT SUPPORTED IN THE LONG TERM ====")
            return dev_ctrl.CommissionIP(
                ipaddr=conf.commissionee_ip_address_just_for_testing,
                setupPinCode=conf.setup_passcode[i], nodeid=conf.dut_node_id[i]
            )
        else:
            raise ValueError("Invalid commissioning method %s!" % conf.commissioning_method)


def default_matter_test_main(argv=None, **kwargs):
    """Execute the test class in a test module.
    This is the default entry point for running a test script file directly.
    In this case, only one test class in a test script is allowed.
    To make your test script executable, add the following to your file:
    .. code-block:: python
      from matter_testing_support.py import default_matter_test_main
      ...
      if __name__ == '__main__':
        default_matter_test_main.main()
    Args:
      argv: A list that is then parsed as command line args. If None, defaults to sys.argv
    """

    matter_test_config = parse_matter_test_args(argv)

    # Allow override of command line from optional arguments
    if not matter_test_config.controller_cat_tags and "controller_cat_tags" in kwargs:
        matter_test_config.controller_cat_tags = kwargs["controller_cat_tags"]

    # Find the test class in the test script.
    test_class = _find_test_class()

    # This is required in case we need any testing with maximized certificate chains.
    # We need *all* issuers from the start, even for default controller, to use
    # maximized chains, before MatterStackState init, others some stale certs
    # may not chain properly.
    if "maximize_cert_chains" in kwargs:
        matter_test_config.maximize_cert_chains = kwargs["maximize_cert_chains"]

    hooks = InternalTestRunnerHooks()

    run_tests(test_class, matter_test_config, hooks)


def run_tests(test_class: typing.Type[MatterBaseTest], matter_test_config: MatterTestConfig, hooks: TestRunnerHooks) -> None:

    # Load test config file.
    test_config = generate_mobly_test_config(matter_test_config)

    # Parse test specifiers if exist.
    tests = None
    if len(matter_test_config.tests) > 0:
        tests = matter_test_config.tests

    stack = MatterStackState(matter_test_config)
    test_config.user_params["matter_stack"] = stash_globally(stack)

    # TODO: Steer to right FabricAdmin!
    # TODO: If CASE Admin Subject is a CAT tag range, then make sure to issue NOC with that CAT tag

    default_controller = stack.certificate_authorities[0].adminList[0].NewController(
        nodeId=matter_test_config.controller_node_id,
        paaTrustStorePath=str(matter_test_config.paa_trust_store_path),
        catTags=matter_test_config.controller_cat_tags
    )
    test_config.user_params["default_controller"] = stash_globally(default_controller)

    test_config.user_params["matter_test_config"] = stash_globally(matter_test_config)

    test_config.user_params["certificate_authority_manager"] = stash_globally(stack.certificate_authority_manager)

    test_config.user_params["hooks"] = stash_globally(hooks)

    # Execute the test class with the config
    ok = True

    runner = TestRunner(log_dir=test_config.log_path,
                        testbed_name=test_config.testbed_name)

    with runner.mobly_logger():
        if matter_test_config.commissioning_method is not None:
            runner.add_test_class(test_config, CommissionDeviceTest, None)

        # Add the tests selected unless we have a commission-only request
        if not matter_test_config.commission_only:
            runner.add_test_class(test_config, test_class, tests)

        if hooks:
            # Right now, we only support running a single test class at once,
            # but it's relatively easy to exapand that to make the test process faster
            # TODO: support a list of tests
            hooks.start(count=1)
            # Mobly gives the test run time in seconds, lets be a bit more precise
            runner_start_time = datetime.now(timezone.utc)

        try:
            runner.run()
            ok = runner.results.is_all_pass and ok
        except TimeoutError:
            ok = False
        except signals.TestAbortAll:
            ok = False
        except Exception:
            logging.exception('Exception when executing %s.', test_config.testbed_name)
            ok = False

    if hooks:
        duration = (datetime.now(timezone.utc) - runner_start_time) / timedelta(microseconds=1)
        hooks.stop(duration=duration)

    # Shutdown the stack when all done
    stack.Shutdown()

    if ok:
        logging.info("Final result: PASS !")
    else:
        logging.error("Final result: FAIL !")
        sys.exit(1)
