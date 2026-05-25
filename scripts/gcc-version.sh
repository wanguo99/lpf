#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# gcc-version.sh - 提取 GCC 版本号
#
# 用法: gcc-version.sh <gcc-command>
#        gcc-version.sh -p <gcc-command>  (打印完整版本)
#
# 输出格式:
#   默认: MAJOR*10000 + MINOR*100 + PATCHLEVEL
#   -p:   MAJOR.MINOR.PATCHLEVEL

if [ "$1" = "-p" ]; then
	with_patchlevel=1
	shift
fi

compiler="$*"

if [ ${#compiler} -eq 0 ]; then
	echo "Error: No compiler specified." >&2
	echo "Usage: $0 <gcc-command>" >&2
	exit 1
fi

# 检查编译器是否存在
if ! command -v $compiler >/dev/null 2>&1; then
	echo 0
	exit 0
fi

# 获取版本信息
MAJOR=$(echo __GNUC__ | $compiler -E -x c - | tail -n 1)
MINOR=$(echo __GNUC_MINOR__ | $compiler -E -x c - | tail -n 1)
PATCHLEVEL=$(echo __GNUC_PATCHLEVEL__ | $compiler -E -x c - | tail -n 1)

# 检查是否为有效数字
if ! [ "$MAJOR" -eq "$MAJOR" ] 2>/dev/null; then
	echo 0
	exit 0
fi

if ! [ "$MINOR" -eq "$MINOR" ] 2>/dev/null; then
	MINOR=0
fi

if ! [ "$PATCHLEVEL" -eq "$PATCHLEVEL" ] 2>/dev/null; then
	PATCHLEVEL=0
fi

# 输出版本
if [ -n "$with_patchlevel" ]; then
	echo $MAJOR.$MINOR.$PATCHLEVEL
else
	echo $(($MAJOR * 10000 + $MINOR * 100 + $PATCHLEVEL))
fi
