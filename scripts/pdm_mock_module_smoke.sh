#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

module_dir=${PDM_MODULE_DIR:-_build/modules}
config_file=${PDM_CONFIG_FILE:-include/config/auto.conf}
required_modules="osal pdm"
loaded_modules=

log()
{
	printf '%s
' "$*"
}

die()
{
	printf 'ERROR: %s
' "$*" >&2
	exit 1
}

is_loaded()
{
	awk -v name="$1" '$1 == name { found = 1 } END { exit found ? 0 : 1 }' 		/proc/modules
}

module_path()
{
	printf '%s/%s.ko
' "$module_dir" "$1"
}

load_module()
{
	module=$1
	path=$(module_path "$module")

	log "  INSMOD  $path"
	insmod "$path"
	loaded_modules="$module $loaded_modules"
}

expect_path()
{
	path=$1

	[ -e "$path" ] || die "missing expected path: $path"
}

expect_readable()
{
	path=$1

	[ -r "$path" ] || die "path is not readable: $path"
}

mock_devices_enabled()
{
	[ -r "$config_file" ] && grep -q '^CONFIG_PDM_MOCK_DEVICES=y$' "$config_file"
}

check_mock_devices()
{
	expect_path /sys/class/pdm
	expect_path /sys/class/pdm/mcu0
	expect_path /sys/class/pdm/led0
	expect_path /dev/pdm/mcu0
	expect_path /dev/pdm/led0
}

check_pdm_bus()
{
	expect_path /sys/bus/pdm
	expect_path /sys/bus/pdm/devices
	expect_path /sys/bus/pdm/drivers
	expect_readable /sys/bus/pdm/uevent
	expect_path /dev/pdm_ctl

	if mock_devices_enabled; then
		check_mock_devices
	fi
}

cleanup()
{
	status=$?

	for module in $loaded_modules; do
		if is_loaded "$module"; then
			log "  RMMOD   $module"
			rmmod "$module" || status=$?
		fi
	done

	exit "$status"
}

trap cleanup EXIT INT TERM

[ -d "$module_dir" ] || die "module directory not found: $module_dir"
module_dir=$(cd "$module_dir" && pwd)

for module in $required_modules; do
	[ -f "$(module_path "$module")" ] || die "missing module: $(module_path "$module")"
done

for module in $required_modules; do
	if is_loaded "$module"; then
		die "module is already loaded, refusing to own unload: $module"
	fi
done

if [ "$(id -u)" != "0" ]; then
	die "module smoke test requires root; run with sudo or as root"
fi

log "PDM core module smoke test"
log "Module directory: $module_dir"

for module in $required_modules; do
	load_module "$module"
done

check_pdm_bus

log "PDM core module smoke test passed"
