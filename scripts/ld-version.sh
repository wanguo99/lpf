#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# ld-version.sh - 提取链接器版本号
#
# 用法: ld-version.sh <ld-command>
#
# 输出格式: MAJOR*10000 + MINOR*100 + PATCHLEVEL

# 从标准输入读取链接器版本输出
# 支持 GNU ld 和 LLVM lld

if [ -z "$1" ]; then
	echo "Error: No linker specified." >&2
	echo "Usage: $0 <ld-command>" >&2
	exit 1
fi

# 获取链接器版本字符串
version_string=$("$@" --version 2>/dev/null | head -n 1)

if [ -z "$version_string" ]; then
	echo 0
	exit 0
fi

# 提取版本号
# GNU ld: "GNU ld (GNU Binutils) 2.35.1"
# LLVM lld: "LLD 12.0.0"
version=$(echo "$version_string" | sed -n 's/.* \([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/p')

if [ -z "$version" ]; then
	# 尝试匹配 MAJOR.MINOR 格式
	version=$(echo "$version_string" | sed -n 's/.* \([0-9][0-9]*\.[0-9][0-9]*\).*/\1.0/p')
fi

if [ -z "$version" ]; then
	echo 0
	exit 0
fi

# 分离版本号
MAJOR=$(echo $version | cut -d. -f1)
MINOR=$(echo $version | cut -d. -f2)
PATCHLEVEL=$(echo $version | cut -d. -f3)

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

# 输出版本号
echo $(($MAJOR * 10000 + $MINOR * 100 + $PATCHLEVEL))
