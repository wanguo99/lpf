#!/bin/sh
# SPDX-License-Identifier: MIT

set -eu

make_cmd=${MAKE:-make}
defconfig=${LPF_KERNEL_MATRIX_DEFCONFIG:-ubuntu_x86_mock_modules_defconfig}
build_root=${LPF_KERNEL_MATRIX_BUILD_ROOT:-_build/kernel-matrix}
kernel_src_list=${KERNEL_SRC_LIST:-${KERNEL_SRC:-/lib/modules/$(uname -r)/build}}

log()
{
	printf '%s\n' "$*"
}

die()
{
	printf 'ERROR: %s\n' "$*" >&2
	exit 1
}

kernel_tag()
{
	path=$1
	base=$(basename "$path")

	if [ "$base" = "build" ] || [ "$base" = "source" ]; then
		base=$(basename "$(dirname "$path")")
	fi

	tag=$(printf '%s' "$base" | tr -c 'A-Za-z0-9._-' '_')
	if [ -z "$tag" ]; then
		tag="kernel"
	fi

	printf '%s\n' "$tag"
}

[ -n "$kernel_src_list" ] || die "KERNEL_SRC_LIST is empty"

log "LPF kernel module matrix build"
log "Defconfig: $defconfig"
log "Output root: $build_root"

index=0
for kernel_src in $kernel_src_list; do
	index=$((index + 1))
	[ -f "$kernel_src/Makefile" ] ||
		die "kernel build tree not found: $kernel_src"

	tag=$(kernel_tag "$kernel_src")
	output_dir="$build_root/$index-$tag"

	log ""
	log "[$index] Kernel source: $kernel_src"
	log "[$index] Output dir: $output_dir"

	"$make_cmd" "$defconfig"
	"$make_cmd" modules \
		KERNEL_SRC="$kernel_src" \
		MODULES_BUILD_DIR="$output_dir"
done

log ""
log "LPF kernel module matrix build passed"
