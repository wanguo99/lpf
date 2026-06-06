#!/usr/bin/env python3
"""
Generate kconfig.h from defconfig file
将 defconfig 转换为 C 头文件，包含所有 CONFIG_* 宏定义
"""

import sys
import os
import argparse
from datetime import datetime


def parse_defconfig(defconfig_path):
    """解析 defconfig 文件，返回配置字典"""
    configs = {}

    if not os.path.exists(defconfig_path):
        print(f"Error: defconfig file not found: {defconfig_path}")
        sys.exit(1)

    with open(defconfig_path, 'r') as f:
        for line in f:
            line = line.strip()

            # 跳过注释和空行
            if not line or line.startswith('#'):
                continue

            # 解析 CONFIG_XXX=y 或 CONFIG_XXX="value"
            if '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()

                # 移除引号
                if value.startswith('"') and value.endswith('"'):
                    value = value[1:-1]

                configs[key] = value

    return configs


def generate_header(configs, output_path, defconfig_name):
    """生成 kconfig.h 头文件"""

    header_guard = "KCONFIG_H"

    with open(output_path, 'w') as f:
        # 文件头注释
        f.write("/*\n")
        f.write(" * Automatically generated file - DO NOT EDIT\n")
        f.write(f" * Generated from: {defconfig_name}\n")
        f.write(f" * Generation time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write(" */\n\n")

        f.write(f"#ifndef {header_guard}\n")
        f.write(f"#define {header_guard}\n\n")

        # 按字母顺序排序输出
        sorted_keys = sorted(configs.keys())

        for key in sorted_keys:
            value = configs[key]

            # 处理不同类型的值
            if value == 'y':
                # 布尔型配置：CONFIG_XXX=y -> #define CONFIG_XXX 1
                f.write(f"#define {key} 1\n")
            elif value == 'n':
                # 明确禁用的配置：CONFIG_XXX=n -> #undef CONFIG_XXX
                f.write(f"#undef {key}\n")
            elif value.isdigit():
                # 数字型配置
                f.write(f"#define {key} {value}\n")
            elif value.startswith('0x'):
                # 十六进制数字
                f.write(f"#define {key} {value}\n")
            else:
                # 字符串型配置
                f.write(f'#define {key} "{value}"\n')

        f.write(f"\n#endif /* {header_guard} */\n")

    print(f"Generated: {output_path}")
    print(f"Total configs: {len(configs)}")


def main():
    parser = argparse.ArgumentParser(description='Generate kconfig.h from defconfig')
    parser.add_argument('defconfig', help='Input defconfig file path')
    parser.add_argument('output', help='Output kconfig.h file path')

    args = parser.parse_args()

    # 解析配置
    configs = parse_defconfig(args.defconfig)

    # 生成头文件
    defconfig_name = os.path.basename(args.defconfig)
    generate_header(configs, args.output, defconfig_name)


if __name__ == '__main__':
    main()
