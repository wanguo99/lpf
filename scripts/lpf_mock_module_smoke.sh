#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

module_dir=${LPF_MODULE_DIR:-_build/modules}
smoke_binary=${LPF_SMOKE_BINARY:-_build/tests/bin/lpf_mock_runtime_smoke}
expected_instance_devnode_mode=${LPF_EXPECT_INSTANCE_DEVNODE_MODE:-0660}
required_modules="osal lpf_configs lpf_core lpf_hw_mock_selftest lpf_dummy_service_selftest"
loaded_modules=

log()
{
	printf '%s\n' "$*"
}

die()
{
	printf 'ERROR: %s\n' "$*" >&2
	exit 1
}

is_loaded()
{
	awk -v name="$1" '$1 == name { found = 1 } END { exit found ? 0 : 1 }' \
		/proc/modules
}

module_path()
{
	printf '%s/%s.ko\n' "$module_dir" "$1"
}

load_module()
{
	module=$1
	path=$(module_path "$module")

	log "  INSMOD  $path"
	insmod "$path"
	loaded_modules="$module $loaded_modules"
}

wait_for_path()
{
	path=$1
	tries=${2:-50}

	while [ "$tries" -gt 0 ]; do
		[ -e "$path" ] && return 0
		tries=$((tries - 1))
		sleep 0.1
	done

	die "path did not appear: $path"
}

expect_path()
{
	path=$1

	[ -e "$path" ] || die "missing expected path: $path"
}

expect_absent()
{
	path=$1

	[ ! -e "$path" ] || die "unexpected path exists: $path"
}

expect_readable()
{
	path=$1

	[ -r "$path" ] || die "path is not readable: $path"
}

file_mode()
{
	path=$1

	mode=$(stat -c '%a' "$path" 2>/dev/null || stat -f '%Lp' "$path")
	printf '%s\n' "${mode#${mode%???}}"
}

expect_mode()
{
	path=$1
	expected=$2
	actual=$(file_mode "$path")

	[ "$actual" = "$expected" ] ||
		die "unexpected mode for $path: expected $expected, got $actual"
}

expect_not_world_writable()
{
	path=$1
	mode=$(file_mode "$path")
	other_digit=${mode#${mode%?}}

	case "$other_digit" in
		2|3|6|7)
			die "path is world-writable: $path mode=$mode"
			;;
	esac
}

expect_no_write_bits()
{
	path=$1
	mode=$(file_mode "$path")

	case "$mode" in
		2??|3??|6??|7??|?2?|?3?|?6?|?7?|??2|??3|??6|??7)
			die "path has write permission bits: $path mode=$mode"
			;;
	esac
}

expect_has_write_bits()
{
	path=$1
	mode=$(file_mode "$path")

	case "$mode" in
		2??|3??|6??|7??|?2?|?3?|?6?|?7?|??2|??3|??6|??7)
			return 0
			;;
	esac

	die "path has no write permission bits: $path mode=$mode"
}

check_procfs()
{
	for path in /proc/lpf/mcu /proc/lpf/led; do
		expect_path "$path"
		expect_readable "$path"
		expect_no_write_bits "$path"
	done
}

check_debugfs()
{
	if [ ! -d /sys/kernel/debug ]; then
		log "  SKIP    debugfs root is not mounted"
		return
	fi

	for path in /sys/kernel/debug/lpf/mcu /sys/kernel/debug/lpf/led; do
		expect_path "$path"
		expect_has_write_bits "$path"
	done
}

check_sysfs()
{
	for base in /sys/class/misc/lpf_mcu0 /sys/class/misc/lpf_led0 /sys/class/misc/lpf_led1; do
		expect_path "$base"
		for attr in name type index state capabilities driver soc last_error error_count open_count; do
			expect_readable "$base/$attr"
			expect_no_write_bits "$base/$attr"
		done
	done
}

check_devnode_modes()
{
	expect_mode /dev/lpf_ctl 666

	for path in /dev/lpf/mcu0 /dev/lpf/led0 /dev/lpf/led1; do
		expect_mode "$path" "$expected_instance_devnode_mode"
		expect_not_world_writable "$path"
	done
}

check_runtime_nodes()
{
	wait_for_path /dev/lpf_ctl
	wait_for_path /dev/lpf/mcu0
	wait_for_path /dev/lpf/led0
	wait_for_path /dev/lpf/led1

	expect_absent /dev/lpf/mcu1
	expect_absent /dev/lpf/led2

	check_devnode_modes
	check_sysfs
	check_procfs
	check_debugfs
}

run_user_smoke()
{
	[ -x "$smoke_binary" ] ||
		die "runtime smoke binary not found or not executable: $smoke_binary; run make tests first or set LPF_SMOKE_BINARY"

	log "  SMOKE   $smoke_binary"
	"$smoke_binary"
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

log "LPF mock module smoke test"
log "Module directory: $module_dir"
log "Runtime smoke binary: $smoke_binary"
log "Expected LPF instance devnode mode: $expected_instance_devnode_mode"

for module in $required_modules; do
	load_module "$module"
done

check_runtime_nodes
run_user_smoke

log "LPF mock module smoke test passed"
