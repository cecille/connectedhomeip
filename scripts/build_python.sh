#!/usr/bin/env bash

#
# Copyright (c) 2021 Project CHIP Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

_normpath() {
    python -c "import os.path; print(os.path.normpath('$@'))"
}

echo_green() {
    echo -e "\033[0;32m$*\033[0m"
}

echo_blue() {
    echo -e "\033[1;34m$*\033[0m"
}

echo_bold_white() {
    echo -e "\033[1;37m$*\033[0m"
}

CHIP_ROOT=$(_normpath "$(dirname "$0")/..")
OUTPUT_ROOT="$CHIP_ROOT/out/python_lib"
ENVIRONMENT_ROOT="$CHIP_ROOT/out/python_env"

declare chip_detail_logging=false
declare enable_pybindings=false
declare chip_mdns
declare case_retry_delta
declare install_wheel=no
declare extra_args=""

help() {

    echo "Usage: $file_name [ options ... ] [ -chip_detail_logging ChipDetailLoggingValue  ] [ -chip_mdns ChipMDNSValue  ] [-enable_pybindings EnableValue]"

    echo "General Options:
  -h, --help                Display this information.
Input Options:
  -d, --chip_detail_logging ChipDetailLoggingValue          Specify ChipDetailLoggingValue as true or false.
                                                            By default it is false.
  -m, --chip_mdns           ChipMDNSValue                   Specify ChipMDNSValue as platform or minimal.
                                                            By default it is minimal.
  -p, --enable_pybindings   EnableValue                     Specify whether to enable pybindings as python controller.

  -t, --time_between_case_retries MRPActiveRetryInterval    Specify MRPActiveRetryInterval value
                                                            Default is 300 ms
  -a, --extra_args extra_args                               Extra GN args
  -i, --install_wheel no|build-env|separate                 Where to install the Python wheel
                                                            no: Do not install
                                                            build-env: install to virtual env for build matter
                                                            separate: install to another virtual env (out/python_env)
  --extra_packages PACKAGES                                 Install extra Python packages from PyPI
  --include_yamltests                                       Whether to install the matter_yamltests wheel.
  -z, --pregen_dir DIRECTORY                                Directory where generated zap files have been pre-generated.
"
}

file_name=${0##*/}

while (($#)); do
    case $1 in
    --help | -h)
        help
        exit 1
        ;;
    --chip_detail_logging | -d)
        chip_detail_logging=$2
        shift
        ;;
    --chip_mdns | -m)
        chip_mdns=$2
        shift
        ;;
    --enable_pybindings | -p)
        enable_pybindings=$2
        shift
        ;;
    --time_between_case_retries | -t)
        chip_case_retry_delta=$2
        shift
        ;;
    --install_wheel | -i)
        install_wheel=$2
        shift
        ;;
    --extra_packages)
        extra_packages=$2
        shift
        ;;
    --include_yamltests)
        include_yamltests="yes"
        ;;
    --pregen_dir | -z)
        pregen_dir=$2
        ;;
    --extra_args | -a)
        extra_args=$2
        shift
        ;;
    -*)
        help
        echo "Unknown Option \"$1\""
        exit 1
        ;;
    esac
    shift
done

# Print input values
echo "Input values: chip_detail_logging = $chip_detail_logging , chip_mdns = \"$chip_mdns\", enable_pybindings = $enable_pybindings, chip_case_retry_delta=\"$chip_case_retry_delta\", pregen_dir=\"$pregen_dir\""

# Ensure we have a compilation environment
source "$CHIP_ROOT/scripts/activate.sh"

# Generates ninja files
[[ -n "$chip_mdns" ]] && chip_mdns_arg="chip_mdns=\"$chip_mdns\"" || chip_mdns_arg=""
[[ -n "$chip_case_retry_delta" ]] && chip_case_retry_arg="chip_case_retry_delta=$chip_case_retry_delta" || chip_case_retry_arg=""
[[ -n "$pregen_dir" ]] && pregen_dir_arg="chip_code_pre_generated_directory=\"$pregen_dir\"" || pregen_dir_arg=""

gn --root="$CHIP_ROOT" gen "$OUTPUT_ROOT" --args="chip_detail_logging=$chip_detail_logging enable_pylib=$enable_pybindings enable_rtti=$enable_pybindings chip_project_config_include_dirs=[\"//config/python\"] $chip_mdns_arg $chip_case_retry_arg $pregen_dir_arg $extra_args"

echo "GN Invocation:" gn --root="$CHIP_ROOT" gen "$OUTPUT_ROOT" --args="chip_detail_logging=$chip_detail_logging enable_pylib=$enable_pybindings enable_rtti=$enable_pybindings chip_project_config_include_dirs=[\"//config/python\"] $chip_mdns_arg $chip_case_retry_arg $pregen_dir_arg $extra_args"

function ninja_target() {
    # Print the ninja target required to build a gn label.
    local GN_LABEL="$1"
    local NINJA_TARGET="$(gn ls "$OUTPUT_ROOT" --as=output "$GN_LABEL")"
    echo "$NINJA_TARGET"
}

function wheel_output_dir() {
    # Print the wheel output directory for a pw_python_package or
    # pw_python_distribution. The label must end in "._build_wheel".
    local GN_LABEL="$1"
    local NINJA_TARGET="$(ninja_target "$GN_LABEL")"
    local WHEEL_DIR="$OUTPUT_ROOT"/"$(dirname "$NINJA_TARGET")/$(basename -s .stamp "$NINJA_TARGET")"
    echo "$WHEEL_DIR"
}

# Compile Python wheels
ninja -C "$OUTPUT_ROOT" python_wheels

# Add wheels from chip_python_wheel_action templates.
if [ "$enable_pybindings" == true ]; then
    WHEEL=("$OUTPUT_ROOT"/pybindings/pycontroller/pychip-*.whl)
else
    WHEEL=("$OUTPUT_ROOT"/controller/python/chip*.whl)
fi

if [ -n "$include_yamltests" ]; then
    YAMLTESTS_GN_LABEL="//scripts:matter_yamltests_distribution._build_wheel"

    # Add wheels from pw_python_package or pw_python_distribution templates.
    WHEEL+=(
        "$(ls -tr "$(wheel_output_dir "$YAMLTESTS_GN_LABEL")"/*.whl | head -n 1)"
    )
fi

if [ -n "$extra_packages" ]; then
    WHEEL+=("$extra_packages")
fi

if [ "$install_wheel" = "no" ]; then
    exit 0
elif [ "$install_wheel" = "separate" ]; then
    # Create a virtual environment that has access to the built python tools
    python3 -m venv --clear "$ENVIRONMENT_ROOT"

    source "$ENVIRONMENT_ROOT"/bin/activate
    "$ENVIRONMENT_ROOT"/bin/python -m pip install --upgrade pip
    "$ENVIRONMENT_ROOT"/bin/pip install --upgrade --force-reinstall "${WHEEL[@]}"
    # Add the matter idl parser wheel required for chip-repl server
    "$ENVIRONMENT_ROOT"/bin/pip install --upgrade --force-reinstall "$OUTPUT_ROOT"/python/obj/scripts/py_matter_idl/matter_idl._build_wheel/matter_idl-0.0.1-py3-none-any.whl

    echo ""
    echo_green "Compilation completed and WHL package installed in: "
    echo_blue "  $ENVIRONMENT_ROOT"
    echo ""
    echo_green "To use please run:"
    echo_bold_white "  source $ENVIRONMENT_ROOT/bin/activate"
elif [ "$install_wheel" = "build-env" ]; then
    pip install --force-reinstall "${WHEEL[@]}"
    # Add the matter idl parser wheel required for chip-repl server
    pip install --upgrade --force-reinstall "$OUTPUT_ROOT"/python/obj/scripts/py_matter_idl/matter_idl._build_wheel/matter_idl-0.0.1-py3-none-any.whl

    echo ""
    echo_green "Compilation completed and WHL package installed in virtualenv for building sdk"
    echo ""
    echo_green "To use please run:"
    echo_bold_white "  source $CHIP_ROOT/scripts/activate.sh"
fi
